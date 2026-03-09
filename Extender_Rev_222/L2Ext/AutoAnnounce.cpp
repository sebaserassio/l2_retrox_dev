#include "stdafx.h"
#include "AutoAnnounce.h"
#include "NpcDb.h"
#include "ObjectDB.h"
#include "TerritoryData.h"
#include "DB.h"
#include "AccountDB.h"
#include "ChatMobile.h"
#include "HtmlCache.h"

CAutoAnnounce g_AutoAnnounce;


wstring AnnounceStealMsg;
bool AnnounceSteal = false;
INT32 m_AnnounceStealMsgId = 0;


UINT m_SendMsgPickUp;
UINT m_AnnouncePickUpMsgItemId;
wstring m_AnnounceOnPickUpClan;
wstring m_AnnounceOnPickUpNormal;









//-------

    // Persistencia de puntos de clanes
    std::map<INT32, ClanPointsInfo> m_clanPoints;
    std::wstring m_clanPointsFile;


static void TrimRight(std::wstring& s)
{
    while(!s.empty())
    {
        wchar_t c = s[s.size() - 1];
        if(c == L'\r' || c == L'\n' || c == L' ' || c == L'\t')
            s.erase(s.size() - 1);
        else
            break;
    }
}

void CAutoAnnounce::LoadClanPoints()
{
    AUTO_LOCK(m_lock);

    m_clanPoints.clear();
    if(m_clanPointsFile.empty())
        return;

    FILE* f = NULL;
    _wfopen_s(&f, m_clanPointsFile.c_str(), L"rt, ccs=UNICODE"); // UTF-16LE
    if(!f)
        return;

    wchar_t lineBuf[8192];
    while(fgetws(lineBuf, (int)(sizeof(lineBuf)/sizeof(lineBuf[0])), f))
    {
        std::wstring line = lineBuf;
        TrimRight(line);
        if(line.empty()) continue;
        if(line[0] == L'#') continue;

        // Formato: pledgeId \t points \t clanName
        size_t p1 = line.find(L'\t');
        if(p1 == std::wstring::npos) continue;
        size_t p2 = line.find(L'\t', p1 + 1);
        if(p2 == std::wstring::npos) continue;

        std::wstring sId   = line.substr(0, p1);
        std::wstring sPts  = line.substr(p1 + 1, p2 - (p1 + 1));
        std::wstring sName = line.substr(p2 + 1);
        TrimRight(sName);

        INT32 pledgeId = (INT32)_wtoi(sId.c_str());
        INT32 pts      = (INT32)_wtoi(sPts.c_str());

        if(pledgeId <= 0) continue;
        if(pts < 0) pts = 0;

        ClanPointsInfo info;
        info.clanName = sName;
        info.points   = pts;
        m_clanPoints[pledgeId] = info;
    }

    fclose(f);

    g_Log.Add(CLog::Blue, "Loaded clan points: [%d] clans from [%S].",
              (int)m_clanPoints.size(), m_clanPointsFile.c_str());
}

void CAutoAnnounce::SaveClanPoints()
{
    if(m_clanPointsFile.empty())
        return;

    // Armamos un snapshot con lock para no iterar mientras cambia
    std::vector<ClanPointsRow> snapshot;

    {
        AUTO_LOCK(m_lock);
        snapshot.reserve(m_clanPoints.size());

        std::map<INT32, ClanPointsInfo>::const_iterator it;
        for(it = m_clanPoints.begin(); it != m_clanPoints.end(); ++it)
        {
            ClanPointsRow r;
            r.pledgeId = it->first;
            r.points   = it->second.points;
            r.clanName = it->second.clanName;
            snapshot.push_back(r);
        }
    }

    std::wstring tmpFile = m_clanPointsFile + L".tmp";

    FILE* f = NULL;
    _wfopen_s(&f, tmpFile.c_str(), L"wt, ccs=UNICODE"); // UTF-16LE
    if(!f)
        return;

    fputws(L"# pledgeId\tpoints\tclanName\r\n", f);

    for(size_t i=0;i<snapshot.size();i++)
    {
        fwprintf(f, L"%d\t%d\t%s\r\n",
                 snapshot[i].pledgeId,
                 snapshot[i].points,
                 snapshot[i].clanName.c_str());
    }

    fclose(f);

    MoveFileExW(tmpFile.c_str(), m_clanPointsFile.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
}

void CAutoAnnounce::AddClanPoints(INT32 pledgeId, const std::wstring& clanName, INT32 delta, INT32& outTotal)
{
    outTotal = 0;
    if(pledgeId <= 0 || delta <= 0)
        return;

    {
        AUTO_LOCK(m_lock);

        ClanPointsInfo& info = m_clanPoints[pledgeId];
        if(!clanName.empty())
            info.clanName = clanName; // refresca nombre

        info.points += delta;
        outTotal = info.points;
    }

    // Boss kills son raros: guardar “en el acto” está OK y simplifica mucho
    SaveClanPoints();
}


//-

struct ClanRankRow
{
    std::wstring clanName;
    INT32 points;
};

// sort desc por puntos
static bool ClanRankSort(const ClanRankRow& a, const ClanRankRow& b)
{
    if(a.points != b.points)
        return (a.points > b.points);
    // desempate por nombre para que sea estable
    return (a.clanName < b.clanName);
}

void CAutoAnnounce::ShowClanRanking(User* pUser)
{
    guard;

    if(!pUser || !pUser->ValidUser())
    {
        unguard;
        return;
    }

    const WCHAR* wHtml = g_HtmlCache.Get(L"voni001.htm");
    if(!wHtml)
    {
        g_Log.Add(CLog::Error, "[%s] Cannot find voni001.htm!", __FUNCTION__);
        unguard;
        return;
    }

    std::wstring html(wHtml);

    // 1) snapshot del map (con lock) -> vector
    std::vector<ClanRankRow> rows;
    {
        AUTO_LOCK(m_lock);

        rows.reserve(m_clanPoints.size());

        std::map<INT32, ClanPointsInfo>::const_iterator it;
        for(it = m_clanPoints.begin(); it != m_clanPoints.end(); ++it)
        {
            const ClanPointsInfo& info = it->second;
            if(info.points <= 0)
                continue;

            ClanRankRow r;
            r.clanName = info.clanName;
            r.points   = info.points;

            // fallback si nombre vacío
            if(r.clanName.empty())
                r.clanName = L"-";

            rows.push_back(r);
        }
    }

    // 2) ordenar desc por puntos
    std::sort(rows.begin(), rows.end(), ClanRankSort);

    // 3) armar HTML <tr>...
    const int kMaxRows = 20; // top 20 (cambialo)
    std::wstringstream ss;

    int shown = 0;
    for(size_t i = 0; i < rows.size() && shown < kMaxRows; ++i, ++shown)
    {
        ss << L"<tr><td>"
           << rows[i].clanName
           << L"</td><td>"
           << rows[i].points
           << L"</td></tr>\r\n";
    }

    std::wstring rowsHtml = ss.str();
    if(rowsHtml.empty())
        rowsHtml = L"<tr><td>-</td><td>0</td></tr>\r\n";

    // 4) reemplazar placeholder
    // (tu Utils::ReplaceString ya lo usás en otros lados)
    html = Utils::ReplaceString(html, L"<?clan_rows?>", rowsHtml.c_str(), true);

    // 5) mostrar
    pUser->ShowHTML(L"voni001Rank.htm", html.c_str(), 0);

    unguard;
}


//-------




CAutoAnnounce::CAutoAnnounce()
{
}

CAutoAnnounce::~CAutoAnnounce()
{
}


extern std::vector<std::pair<std::wstring, int> > AnnounceRaidBossAnticipado;

void CAutoAnnounce::Init()
{
	m_reload = FALSE;
	m_enabled = FALSE;
	mDarNoblesPorMatarBarakiel = FALSE;
	mDarEntradaPorMatarDarion = FALSE;
	m_data.clear();
	m_data2.clear();
	m_BossList.clear();
	m_BossEpicList.clear();
	LoadINI();
	if(m_enabled)
	{
		g_Log.Add(CLog::Blue, "Auto Announce feature is enabled. Loaded [%d] announces. Loaded [%d] static announces.", m_data.size(), m_data2.size());
	}
}

/*
[AutoAnnounce]
Enabled=1
AnnounceCount=2
Announce1_Message=Welcome to ServerName Lineage II Interlude server, hope you enjoy.
Announce1_ShowOnEnterWorld=1
Announce1_Interval=0
Announce2_Message=Please remember to vote for us, more info at www.servername.com thx in advance.
Announce2_ShowOnEnterWorld=1
Announce2_Interval=300

StaticAnnounceCount=2
StaticAnnounce1_Message=blablabla
StaticAnnounce1_Critical=1
StaticAnnounce1_DayFlag=1111111
StaticAnnounce1_Time=23:20
StaticAnnounce2_Message=blablabla
StaticAnnounce2_Critical=0
StaticAnnounce2_DayFlag=1111111
StaticAnnounce2_Time=23:20
*/

void CAutoAnnounce::LoadINI()
{
	const TCHAR* section = _T("AutoAnnounce");
	m_enabled = GetPrivateProfileInt(section, _T("Enabled"), 0, g_ConfigFile);
	UINT count = GetPrivateProfileInt(section, _T("AnnounceCount"), 0, g_ConfigFile);
	TCHAR temp[16384] = { 0 };
	m_BossAnnounce[0] = 0;
	m_BossAnnounce2[0] = 0;




	AnnounceOnBossAnticipoTime = GetPrivateProfileInt(section, _T("AnnounceOnBossAnticipoTime"), 0, g_ConfigFile);
	if(GetPrivateProfileString(section, _T("AnnounceOnBossSpawnAnticipo"), 0, temp, 16384, g_ConfigFile))
	{
		AnnounceOnBossSpawnAnticipo = temp;
	}



	AnnounceSteal = GetPrivateProfileInt(section, _T("AnnounceSteal"), 0, g_ConfigFile);
	memset(temp, 0, sizeof(temp));
	if(GetPrivateProfileString(section, _T("AnnounceStealMsg"), 0, temp, 8192, g_ConfigFile))
	{
		AnnounceStealMsg = temp;
	}
	
	m_AnnounceStealMsgId = GetPrivateProfileInt(section, _T("AnnounceStealMsgId"), 2679, g_ConfigFile);



	if(GetPrivateProfileString(section, _T("AnnounceOnBossSpawn"), 0, temp, 16384, g_ConfigFile))
	{
		m_BossAnnounce = temp;
	}
	if(GetPrivateProfileString(section, _T("AnnounceBossList"), 0, temp, 0x8000, g_ConfigFile))
	{
		wstringstream npcStream(temp);
		wstring npcName;
		while(npcStream >> npcName)
		{
			INT32 npcId = g_ObjectDB.GetClassIdFromName(npcName.c_str());
						//INT32 npcId = 0;
			if(npcId > 0)
			{
				m_BossList.push_back(npcId);


				bool comprobar = false;
				for (vector<pair<wstring, int>>::iterator Iter = AnnounceRaidBossAnticipado.begin(); Iter != AnnounceRaidBossAnticipado.end(); ++Iter) {
					if (Iter->first == npcName)
					{
						//Iter->second = 0;
						comprobar = true;
					}
				}

				if(!comprobar)
				{
					AnnounceRaidBossAnticipado.push_back(std::make_pair(npcName, 0));

				}




			}			
		}
	}


	if(GetPrivateProfileString(section, _T("AnnounceOnBossEpicSpawn"), 0, temp, 16384, g_ConfigFile))
	{
		m_BossAnnounce2 = temp;
	}
	if(GetPrivateProfileString(section, _T("AnnounceBossEpicList"), 0, temp, 0x8000, g_ConfigFile))
	{
		wstringstream npcStream(temp);
		wstring npcName;
		while(npcStream >> npcName)
		{
			INT32 npcId = g_ObjectDB.GetClassIdFromName(npcName.c_str());
						//INT32 npcId = 0;
			if(npcId > 0)
			{
				m_BossEpicList.push_back(npcId);
			}			
		}
	}

	memset(temp, 0, sizeof(temp));
	if(GetPrivateProfileString(section, _T("ClanPointsFile"), 0, temp, 16384, g_ConfigFile))
		m_clanPointsFile = temp;
	else
		m_clanPointsFile = L"clan_points_raid.txt";

	LoadClanPoints();



	//AnnouncePickUpMsgItemId=3435
	//SendMsgIdPickUp=2913
	//AnnouncePickUpNormal=Event Info: El player $pj a levantado el drop de evento.
	//AnnouncePickUpClan=Event Info: El player $pj del clan $clan a levantado el drop de evento.

	m_SendMsgPickUp = GetPrivateProfileInt(section, _T("SendMsgIdPickUp"), 2913, g_ConfigFile);
	m_AnnouncePickUpMsgItemId = GetPrivateProfileInt(section, _T("AnnouncePickUpMsgItemId"), 0, g_ConfigFile);

	if(GetPrivateProfileString(section, _T("AnnouncePickUpClan"), 0, temp, 16384, g_ConfigFile))
	{
		m_AnnounceOnPickUpClan = temp;
	}
	if(GetPrivateProfileString(section, _T("AnnouncePickUpNormal"), 0, temp, 16384, g_ConfigFile))
	{
		m_AnnounceOnPickUpNormal = temp;
	}





	m_SendMsgKill = GetPrivateProfileInt(section, _T("AnnounceBossMsgKillEnabled"), 0, g_ConfigFile);

	if(GetPrivateProfileString(section, _T("AnnounceOnBossKillClan"), 0, temp, 16384, g_ConfigFile))
	{
		m_AnnounceOnBossKillClan = temp;
	}
	if(GetPrivateProfileString(section, _T("AnnounceOnBossKill"), 0, temp, 16384, g_ConfigFile))
	{
		m_AnnounceOnBossKill = temp;
	}

	if(GetPrivateProfileString(section, _T("AnnounceOnBossKillNobless"), 0, temp, 16384, g_ConfigFile))
	{
		m_AnnounceOnBossKillNobless = temp;
	}


	if(GetPrivateProfileString(section, _T("AnnounceOnBossKillDarion"), 0, temp, 16384, g_ConfigFile))
	{
		m_AnnounceOnBossKillDarion = temp;
	}



	if(GetPrivateProfileString(section, _T("AnnounceOnBossEpicKill"), 0, temp, 16384, g_ConfigFile))
	{
		m_AnnounceOnBossEpicKill = temp;
	}
	if(GetPrivateProfileString(section, _T("AnnounceOnBossEpicKillClan"), 0, temp, 16384, g_ConfigFile))
	{
		m_AnnounceOnBossEpicKillClan = temp;
	}

	m_SendMsgKillEpicId = GetPrivateProfileInt(section, _T("SendMsgKillEpicId"), 2679, g_ConfigFile);
	m_SendMsgKillId = GetPrivateProfileInt(section, _T("SendMsgKillId"), 2759, g_ConfigFile);


	mDarNoblesPorMatarBarakiel = GetPrivateProfileInt(section, _T("DarNoblesPorMatarBarakiel"), 0, g_ConfigFile);

	mDarEntradaPorMatarDarion = GetPrivateProfileInt(section, _T("DarEntradaPorMatarDarion"), 0, g_ConfigFile);


	AnnounceOnBossKillVipEnable = GetPrivateProfileInt(section, _T("BossKillGetVipEnable"), 0, g_ConfigFile);
	AnnounceOnBossKillVipNpcId = GetPrivateProfileInt(section, _T("BossKillGetVipNpcId"), 0, g_ConfigFile);
	AnnounceOnBossKillVipTimespamp = GetPrivateProfileInt(section, _T("BossKillGetVipTimestamp"), 0, g_ConfigFile);
	if(GetPrivateProfileString(section, _T("BossKillGetVipMessage"), 0, temp, 16384, g_ConfigFile))
	{
		AnnounceOnBossKillVip = temp;
	}


	AnnounceOnBossKillStarsEnable = GetPrivateProfileInt(section, _T("BossKillGetStarsEnable"), 0, g_ConfigFile);
	AnnounceOnBossKillStarsNpcId = GetPrivateProfileInt(section, _T("BossKillGetStarsNpcId"), 0, g_ConfigFile);
	AnnounceOnBossKillStarsItemId = GetPrivateProfileInt(section, _T("BossKillGetStarsItemId"), 0, g_ConfigFile);
	AnnounceOnBossKillStarsItemCount = GetPrivateProfileInt(section, _T("BossKillGetStarsItemCount"), 0, g_ConfigFile);

	if(GetPrivateProfileString(section, _T("BossKillGetStarsMessage"), 0, temp, 16384, g_ConfigFile))
	{
		AnnounceOnBossKillStars = temp;
	}



	m_SendMsg = GetPrivateProfileInt(section, _T("AnnounceBossMsgEnabled"), 0, g_ConfigFile);
	m_SendMsgEpicId = GetPrivateProfileInt(section, _T("SendMsgEpicId"), 2679, g_ConfigFile);
	m_SendMsgId = GetPrivateProfileInt(section, _T("SendMsgId"), 2759, g_ConfigFile);

	for(UINT n = 0;n<count;n++)
	{
		AutoAnnounceData aad;
		aad.nextAnnounceTime = 0;
		{
			wstringstream keyStream;
			keyStream << L"Announce" << (n+1) <<L"_Message";
			if(GetPrivateProfileString(section, keyStream.str().c_str(), 0, temp, 8192, g_ConfigFile))
			{
				aad.announce = temp;
			}
		}
		{
			wstringstream keyStream;
			keyStream << L"Announce" << (n+1) <<L"_ShowOnEnterWorld";
			aad.onEnterWorld = GetPrivateProfileInt(section, keyStream.str().c_str(), 0, g_ConfigFile);
		}
		{
			wstringstream keyStream;
			keyStream << L"Announce" << (n+1) <<L"_Interval";
			aad.interval = GetPrivateProfileInt(section, keyStream.str().c_str(), 0, g_ConfigFile);
		}
		{
			wstringstream keyStream;
			keyStream << L"Announce" << (n+1) <<L"_CritAnnounce";
			aad.critAnnounce = GetPrivateProfileInt(section, keyStream.str().c_str(), 0, g_ConfigFile);
		}
		if(temp[0] != '\0')
		{
			m_data.push_back(aad);
		}
	}
	UINT count2 = GetPrivateProfileInt(section, _T("StaticAnnounceCount"), 0, g_ConfigFile);
	for(UINT n = 0;n<count2;n++)
	{
		StaticAnnounceData sad;
		sad.jobdone = false;
		sad.nextjobreset = 0;

		{
			wstringstream keyStream;
			keyStream << L"StaticAnnounce" << (n+1) <<L"_Message";
			if(GetPrivateProfileString(section, keyStream.str().c_str(), 0, temp, 8192, g_ConfigFile))
			{
				sad.announce = temp;
			}
		}
		{
			wstringstream keyStream;
			keyStream << L"StaticAnnounce" << (n+1) <<L"_Critical";
			sad.critAnnounce = GetPrivateProfileInt(section, keyStream.str().c_str(), 0, g_ConfigFile);
		}
		{
			wstringstream keyStream;
			keyStream << L"StaticAnnounce" << (n+1) <<L"_Time";
			if(GetPrivateProfileString(section, keyStream.str().c_str(), 0, temp, 8192, g_ConfigFile))
			{
				UINT hour, minute;

				wstring param(temp);
				param = Utils::ReplaceString(param, L";", L" ", true); 

				wstringstream paramStream(param);
				paramStream >> hour >> minute;
				
				//g_Log.Add(CLog::Blue, "hour[%d] min[%d]", hour, minute);

				sad.hour = hour;
				sad.min = minute;
			}
		}

		{
			wstringstream keyStream;
			keyStream << L"StaticAnnounce" << (n+1) <<L"_DayFlag";
			UINT dayFlag = GetPrivateProfileInt(section, keyStream.str().c_str(), 0, g_ConfigFile);
			for(UINT m = 0; m < 7; m++)
			{
				if((dayFlag % 10) == 1)
				{
					sad.dayFlag[m] = true;
				}else
				{
					sad.dayFlag[m] = false;
				}
				dayFlag /= 10;
			}
		}

		if(temp[0] != '\0')
		{
			m_data2.push_back(sad);
		}
	}
}

void CAutoAnnounce::TimerExpired()
{
	CTL;
	if(m_enabled || m_reload)
	{
		AUTO_LOCK(m_lock);
		if(m_reload)
		{
			Init();
			if(!m_enabled)
			{
				return;
			}
		}


		time_t currentTime = time(0);

		for (vector<pair<wstring, int>>::iterator Iter = AnnounceRaidBossAnticipado.begin(); Iter != AnnounceRaidBossAnticipado.end(); ++Iter) {


			if(currentTime == Iter->second - AnnounceOnBossAnticipoTime)
			{
				//g_Log.Add(CLog::Error, "[%s] Announce faltan 5 minutos!!!![%S] ", __FUNCTION__, Iter->first.c_str());



				//AnnounceOnBossAnticipoTime=60
				//AnnounceOnBossSpawnAnticipo=Raidboss Info: $bossName Aparecera en 1 minuto.

					int npcId = g_ObjectDB.GetClassIdFromName(Iter->first.c_str());
					wstring npcName = g_NpcDb.GetClientNameByClassId(npcId);
					wstring announce = AnnounceOnBossSpawnAnticipo;
					size_t pos = announce.find(L"$bossName");
					while(pos != wstring::npos)
					{
						announce = announce.replace(pos, 9, npcName);
						pos = announce.find(L"$bossName");
					}

					if(m_SendMsg)
					{
						CSystemMessage msg(m_SendMsgId);
						msg.AddText(announce.c_str());
						L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());

						g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", announce.c_str(), m_SendMsgId);
					}
					else
					{
						//pUser->pSocket->Send("cddSS", 0x4A, 0, 0xA, L"", data.announce.c_str());
						CHAR buff[8190] = { 0 };
						int len = Assemble(buff, sizeof(buff), "cddSS", 0x4A, 0, 0xA, L"", announce.c_str());
						L2Server::BroadcastToAllUser(len, buff);

						g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", announce.c_str(), SAY_ANNOUNCEMENT);
					}


			}
		}


		for(UINT n=0;n<m_data.size();n++)
		{
			AutoAnnounceData& data = m_data[n];
			if(data.interval > 0)
			{
				if(currentTime > data.nextAnnounceTime)
				{
					//announce and set new next announce time
					data.nextAnnounceTime = currentTime + data.interval;

					if(!data.critAnnounce)
					{
						Utils::BroadcastToAllUser_Announce(data.announce.c_str());
					}
					else
					{
						Utils::BroadcastToAllUser_Announce(data.announce.c_str());
						//CHAR pck[8190];
						//int nSize = Assemble(pck, 8190, "cddSS", 0x4A, 0, 18, L"", data.announce.c_str());
						//L2Server::BroadcastToAllUser(nSize, pck);
					}
				}
			}
		}

		tm ti;
		GetTimeInfo(ti);
		//Static Announces
		for(UINT n=0;n<m_data2.size();n++)
		{
			StaticAnnounceData& data = m_data2[n];
			if(!m_data2[n].jobdone)
			{
				if(data.dayFlag[ti.tm_wday])
				{
					//g_Log.Add(CLog::Blue, "HERE1");
					//g_Log.Add(CLog::Blue, "TM H[%d], D H[%d], TM M[%d], D M[%d]", ti.tm_hour, data.hour, ti.tm_min, data.min);
					if( (ti.tm_hour == data.hour) && (ti.tm_min == data.min) )
					{
						m_data2[n].jobdone = true;
						m_data2[n].nextjobreset = time(0) + 70;
						if(!data.critAnnounce)
							Utils::BroadcastToAllUser_Announce(data.announce.c_str());
						else
						{
							//CHAR pck[8190];
							//int nSize = Assemble(pck, 8190, "cddSS", 0x4A, 0, 18, L"", data.announce.c_str());
							//L2Server::BroadcastToAllUser(nSize, pck);

							Utils::BroadcastToAllUser_Announce(data.announce.c_str());


							//CSystemMessage msg(2679);
							//msg.AddText(data.announce.c_str());
							//L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());
						}
					}
				}
			}

			if(m_data2[n].nextjobreset < time(0))
			{
				m_data2[n].jobdone = false;
			}
		}
	}
}

void CAutoAnnounce::OnEnterWorld(User *pUser)
{
	CTL;
	if(m_enabled)
	{
		if(pUser->ValidUser())
		{
			AUTO_LOCK(m_lock);
			for(UINT n=0;n<m_data.size();n++)
			{
				AutoAnnounceData& data = m_data[n];
				if(data.onEnterWorld)
				{
					if(!data.critAnnounce)
					{
						int MsgId = 0;
						std::wstring wAnnounce2;

						const std::wstring wDelimiter = L";;;";
						size_t nPos = wDelimiter.size();
						size_t nDelimiterPos = std::wstring(data.announce.c_str()).find(wDelimiter);
						if (nDelimiterPos != std::wstring::npos)
						{
							std::wstringstream ss(std::wstring(data.announce.c_str()).substr(0, nDelimiterPos));
							ss >> MsgId;
							wAnnounce2 = std::wstring(data.announce.c_str()).substr(nDelimiterPos + nPos);
						}
						else
						{
							wAnnounce2 = data.announce.c_str();
						}

						if (MsgId == 0)
						{
							pUser->pSocket->Send("cddSS", 0x4A, 0, 0xA, L"", data.announce.c_str());

							//g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", data.announce.c_str(), SAY_ANNOUNCEMENT);
						}
						else
						{
							CSystemMessage msg(MsgId);
							msg.AddText(wAnnounce2.c_str());
							pUser->SendSystemMessage(&msg);

							//g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", wAnnounce2.c_str(), MsgId);
						}
						
					}
					else
					{
						int MsgId = 0;
						std::wstring wAnnounce2;

						const std::wstring wDelimiter = L";;;";
						size_t nPos = wDelimiter.size();
						size_t nDelimiterPos = std::wstring(data.announce.c_str()).find(wDelimiter);
						if (nDelimiterPos != std::wstring::npos)
						{
							std::wstringstream ss(std::wstring(data.announce.c_str()).substr(0, nDelimiterPos));
							ss >> MsgId;
							wAnnounce2 = std::wstring(data.announce.c_str()).substr(nDelimiterPos + nPos);
						}
						else
						{
							wAnnounce2 = data.announce.c_str();
						}

						if (MsgId == 0)
						{
							pUser->pSocket->Send("cddSS", 0x4A, 0, 0xA, L"", data.announce.c_str());

						//	g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", data.announce.c_str(), SAY_ANNOUNCEMENT);
						}
						else
						{
							CSystemMessage msg(MsgId);
							msg.AddText(wAnnounce2.c_str());
							pUser->SendSystemMessage(&msg);

						//	g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", wAnnounce2.c_str(), MsgId);
						}
					}
				}
			}		
		}
	}
}

void CAutoAnnounce::OnBossEnterWorld(UINT npcClassId)
{
	CTL;
	if(m_enabled)
	{

		if(npcClassId == 1025283)			//[lilith] = 1025283
		{
			g_DoorDB.OpenDoorWall(L"innadrile_castle_wall_001");
			g_DoorDB.OpenDoorWall(L"innadrile_castle_wall_002");

			g_DoorDB.OpenDoor(L"innadrile_castle_outter_001");
			g_DoorDB.OpenDoor(L"innadrile_castle_outter_002");
			g_DoorDB.OpenDoor(L"innadrile_castle_inner_001");
			g_DoorDB.OpenDoor(L"innadrile_castle_inner_002");
			g_DoorDB.OpenDoor(L"innadrile_castle_station");
			g_DoorDB.OpenDoor(L"innadrile_castle_small_001");
			g_DoorDB.OpenDoor(L"innadrile_castle_small_002");
		}


		bool affected = false;
		bool epic = false;
		for(UINT n=0;n<m_BossList.size();n++)
		{
			if(m_BossList[n] == npcClassId)
			{
				affected = true;
				break;
			}
		}

		for(UINT n=0;n<m_BossEpicList.size();n++)
		{
			if(m_BossEpicList[n] == npcClassId)
			{
				affected = true;
				epic = true;
				break;
			}
		}

		if(affected)
		{
			//make an announce
			wstring npcName = g_NpcDb.GetClientNameByClassId(npcClassId);
			wstring announce = m_BossAnnounce;
			wstring announce2 = m_BossAnnounce2;
			size_t pos = announce.find(L"$bossName");
			while(pos != wstring::npos)
			{
				announce = announce.replace(pos, 9, npcName);
				pos = announce.find(L"$bossName");
			}

			pos = announce2.find(L"$bossName");
			while(pos != wstring::npos)
			{
				announce2 = announce2.replace(pos, 9, npcName);
				pos = announce2.find(L"$bossName");
			}


			
			wstring DBnpcName = g_NpcDb.GetDbNameByClassId(npcClassId);
			for (vector<pair<wstring, int>>::iterator Iter = AnnounceRaidBossAnticipado.begin(); Iter != AnnounceRaidBossAnticipado.end(); ++Iter) {
				if (Iter->first == DBnpcName)
				{
					Iter->second = 0;
					break;
				}
			}



			if(m_SendMsg)
			{
				if(epic)
				{
					CSystemMessage msg(m_SendMsgEpicId);
					//msg.AddText(npcName.c_str());
					msg.AddText(announce2.c_str());
					L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());

					g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", announce2.c_str(), m_SendMsgEpicId);
					
				}
				else
				{
					CSystemMessage msg(m_SendMsgId);
					msg.AddText(announce.c_str());
					L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());

					g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", announce.c_str(), m_SendMsgId);
				}
			}
			else
			{
				//pUser->pSocket->Send("cddSS", 0x4A, 0, 0xA, L"", data.announce.c_str());
				CHAR buff[8190] = { 0 };
				int len = Assemble(buff, sizeof(buff), "cddSS", 0x4A, 0, 0xA, L"", announce.c_str());
				L2Server::BroadcastToAllUser(len, buff);

				g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", announce.c_str(), SAY_ANNOUNCEMENT);
			}
		}
	}
}




static void ReplaceAll(std::wstring& s, const std::wstring& from, const std::wstring& to)
{
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::wstring::npos)
    {
        s.replace(pos, from.length(), to);
        pos += to.length();
    }
}

void CAutoAnnounce::OnBossKillWorld(UINT npcClassId, CCreature *pKiller)
{
	CTL;
	if(m_enabled)
	{
		bool affected = false;
		bool epic = false;
		for(UINT n=0;n<m_BossList.size();n++)
		{
			if(m_BossList[n] == npcClassId)
			{
				affected = true;
				break;
			}
		}

		for(UINT n=0;n<m_BossEpicList.size();n++)
		{
			if(m_BossEpicList[n] == npcClassId)
			{
				affected = true;
				epic = true;
				break;
			}
		}

		int nPledgeID = 0;

		if(affected)
		{
			if(m_SendMsgKill && pKiller->ValidCreature())
			{

				wstring DBnpcName = g_NpcDb.GetDbNameByClassId(npcClassId);
				for (vector<pair<wstring, int>>::iterator Iter = AnnounceRaidBossAnticipado.begin(); Iter != AnnounceRaidBossAnticipado.end(); ++Iter) {
					if (Iter->first == DBnpcName)
					{
						Iter->second = -1;
						break;
					}
				}

				User *pUser = pKiller->GetUser();
				if (pUser->ValidUser())
				{
					bool nTieneClan = false;
					wstring npcName = g_NpcDb.GetClientNameByClassId(npcClassId);
					wstring AnnounceOnBossKillClan =  m_AnnounceOnBossKillClan;
					wstring AnnounceOnBossKill =  m_AnnounceOnBossKill;
					wstring AnnounceOnBossEpicKill =  m_AnnounceOnBossEpicKill;
					wstring AnnounceOnBossEpicKillClan =  m_AnnounceOnBossEpicKillClan;
					wstring nClanName = L"";

					wstring AnnounceOnBossKillNobless =  m_AnnounceOnBossKillNobless;

					wstring AnnounceOnBossKillDarion =  m_AnnounceOnBossKillDarion;

					CPledge *pPledge = pKiller->GetUser()->GetPledge();
					CPledgeSharedData *pSD = pPledge->GetSharedData();
					if(pSD)
					{
						if(pPledge)
						{
							nClanName = pPledge->pSD->wszClanName;
							nTieneClan = true;
							nPledgeID = pPledge->pSD->nDBID;
						}
					}

					if(epic)
					{
						if(nTieneClan)
						{
							wstringstream name;
							name << pKiller->pSD->wszName;
							wstring charname;
							name >> charname;

							wstring announce = AnnounceOnBossEpicKillClan;
							size_t pos = announce.find(L"$bossName");
							while(pos != wstring::npos)
							{
								announce = announce.replace(pos, 9, npcName);
								pos = announce.find(L"$bossName");
							}

							pos = announce.find(L"$clan");
							while(pos != wstring::npos)
							{
								announce = announce.replace(pos, 5, nClanName);
								pos = announce.find(L"$clan");
							}

							pos = announce.find(L"$pj");
							while(pos != wstring::npos)
							{
								announce = announce.replace(pos, 3, charname);
								pos = announce.find(L"$pj");
							}

							CSystemMessage msg(m_SendMsgKillEpicId);
							msg.AddText(announce.c_str());
							L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());

							g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", announce.c_str(), m_SendMsgKillEpicId);
						}
						else
						{
							wstringstream name;
							name << pKiller->pSD->wszName;
							wstring charname;
							name >> charname;

							wstring announce = AnnounceOnBossEpicKill;
							size_t pos = announce.find(L"$bossName");
							while(pos != wstring::npos)
							{
								announce = announce.replace(pos, 9, npcName);
								pos = announce.find(L"$bossName");
							}

							pos = announce.find(L"$pj");
							while(pos != wstring::npos)
							{
								announce = announce.replace(pos, 3, charname);
								pos = announce.find(L"$pj");
							}

							CSystemMessage msg(m_SendMsgKillEpicId);
							msg.AddText(announce.c_str());
							L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());

							g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", announce.c_str(), m_SendMsgKillEpicId);
						}
					}
					else
					{
						if(nTieneClan)
						{
							wstringstream name;
							name << pKiller->pSD->wszName;
							wstring charname;
							name >> charname;

							wstring announce = AnnounceOnBossKillClan;
							size_t pos = announce.find(L"$bossName");
							while(pos != wstring::npos)
							{
								announce = announce.replace(pos, 9, npcName);
								pos = announce.find(L"$bossName");
							}

							pos = announce.find(L"$clan");
							while(pos != wstring::npos)
							{
								announce = announce.replace(pos, 5, nClanName);
								pos = announce.find(L"$clan");
							}

							pos = announce.find(L"$pj");
							while(pos != wstring::npos)
							{
								announce = announce.replace(pos, 3, charname);
								pos = announce.find(L"$pj");
							}

							CSystemMessage msg(m_SendMsgKillId);
							msg.AddText(announce.c_str());
							L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());

							g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", announce.c_str(), m_SendMsgKillId);
						}
						else
						{
							wstringstream name;
							name << pKiller->pSD->wszName;
							wstring charname;
							name >> charname;

							wstring announce = AnnounceOnBossKill;
							size_t pos = announce.find(L"$bossName");
							while(pos != wstring::npos)
							{
								announce = announce.replace(pos, 9, npcName);
								pos = announce.find(L"$bossName");
							}

							pos = announce.find(L"$pj");
							while(pos != wstring::npos)
							{
								announce = announce.replace(pos, 3, charname);
								pos = announce.find(L"$pj");
							}

							CSystemMessage msg(m_SendMsgKillId);
							msg.AddText(announce.c_str());
							L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());

							g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", announce.c_str(), m_SendMsgKillId);
						}
					}


if(nTieneClan && nPledgeID > 0)
{
    int points = 0;

    if(npcClassId == 1025286) // anakim
        points = 15;
    else if(npcClassId == 1025283) // lilith
        points = 15;
	else if(epic)
		points = 25;
	else
		points = 1;

    if(points > 0)
    {
        INT32 total = 0;
        AddClanPoints((INT32)nPledgeID, nClanName, (INT32)points, total);

        std::wstring announce = L"El clan $clan sumo $points puntos en el ranking. Total: $total.";

        ReplaceAll(announce, L"$clan", nClanName);

        wchar_t buf[32];

        _itow(points, buf, 10);
        ReplaceAll(announce, L"$points", buf);

        _itow(total, buf, 10);
        ReplaceAll(announce, L"$total", buf);

        CSystemMessage msg(2902);
        msg.AddText(announce.c_str());
        L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());

        g_Logger.Add(L"Clan[%s][%d] mato raid y gano [%d] puntos (total=%d)",
                     nClanName.c_str(), nPledgeID, points, total);
    }
}



							if(npcClassId == 1025325 && mDarNoblesPorMatarBarakiel)
							{


								//map<UINT, double> accumulatedContribute = pNpc->pContributeData->accumulatedContribute;


								if(CParty *pParty = pUser->GetParty())
								{
									UserVector vUsers;
									pParty->GetAllMembersEx(vUsers);
									for(UINT n=0;n<vUsers.size();n++)
									{
										if(User *pMember = vUsers[n]->SafeCastUser())
										{
											if(!pMember->pSD->nNobless && pMember->pSD->nAlive)
											{
												g_DB.RequestSaveNoblessType(pMember, 1);
												pMember->AddItemToInventory(7694, 1);	//noblesse tiara
												typedef void(*f)(LPBYTE, INT32, bool);
												f(0x431360L)(pMember->pSD->OneTimeQuestBitMap, 247, true);
												g_DB.SaveQuestInfo(pMember);
												//2545	1	a,Congratulations! You've became a noblesse.\0	0	79	9B	B0	FF	a,	a,	0	0	0	0	0	a,	a,none\0
												pMember->pSocket->SendSystemMessage(2545);
												pMember->PlaySound(L"ItemSound.quest_finish", 1, pMember->nObjectID, (int)pMember->pSD->Pos.x, (int)pMember->pSD->Pos.y, (int)pMember->pSD->Pos.z);

												wstringstream name;
												name << pMember->pSD->wszName;
												wstring charname;
												name >> charname;

												wstring announce = AnnounceOnBossKillNobless;
												size_t pos = announce.find(L"$pj");
												while(pos != wstring::npos)
												{
													announce = announce.replace(pos, 3, charname);
													pos = announce.find(L"$pj");
												}

												CSystemMessage msg(2902);
												msg.AddText(announce.c_str());
												L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());

												g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", announce.c_str(), 2902);
											}
										}
									}
								}
								else
								{
									if(!pUser->pSD->nNobless)
									{
										g_DB.RequestSaveNoblessType(pUser, 1);
										pUser->AddItemToInventory(7694, 1);	//noblesse tiara
										typedef void(*f)(LPBYTE, INT32, bool);
										f(0x431360L)(pUser->pSD->OneTimeQuestBitMap, 247, true);
										g_DB.SaveQuestInfo(pUser);
										//2545	1	a,Congratulations! You've became a noblesse.\0	0	79	9B	B0	FF	a,	a,	0	0	0	0	0	a,	a,none\0
										pUser->pSocket->SendSystemMessage(2545);
										pUser->PlaySound(L"ItemSound.quest_finish", 1, pUser->nObjectID, (int)pUser->pSD->Pos.x, (int)pUser->pSD->Pos.y, (int)pUser->pSD->Pos.z);

										wstringstream name;
										name << pUser->pSD->wszName;
										wstring charname;
										name >> charname;

										wstring announce = AnnounceOnBossKillNobless;
										size_t pos = announce.find(L"$pj");
										while(pos != wstring::npos)
										{
											announce = announce.replace(pos, 3, charname);
											pos = announce.find(L"$pj");
										}

										CSystemMessage msg(2902);
										msg.AddText(announce.c_str());
										L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());

										g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", announce.c_str(), 2902);
									}
								}

								  
								g_TerritoryChecker.AddZonePvP2(1025325, 91272, -86103, -2712);
							}

							if(AnnounceOnBossKillVipEnable && npcClassId == AnnounceOnBossKillVipNpcId)		//DARION
							{
								if(CParty *pParty = pUser->GetParty())
								{
									UserVector vUsers;
									pParty->GetAllMembersEx(vUsers);
									for(UINT n=0;n<vUsers.size();n++)
									{
										if(User *pMember = vUsers[n]->SafeCastUser())
										{
											if(pMember->pSD->nAlive)
											{
												//AGREGAR VIP

												pMember->SocialAction(SocialActionLevelUp);

												if(pMember->pSD->vipLevel > 0)
												{
													g_Logger.Add(L"User[%s] DARION se agrego tiempo VIP level[%d]", pMember->pSD->wszName, 1);
													pMember->pSD->vipTimestamp += AnnounceOnBossKillVipTimespamp;
												}
												else
												{
													g_Logger.Add(L"User[%s] DARION se le dio estado VIP level[%d]", pMember->pSD->wszName, 1);
													pMember->pSD->vipTimestamp = time(0) + AnnounceOnBossKillVipTimespamp;
												}

												pMember->pSD->vipLevel = 1;
												g_DB.RequestSaveVipStatus(pMember->nDBID, 1, pMember->pSD->vipTimestamp);
												g_AccountDB.SetVIPLevel(pMember->pSocket->accountId, 1, pMember->pSD->vipTimestamp);

												wstringstream name;
												name << pMember->pSD->wszName;
												wstring charname;
												name >> charname;

												wstring announce = AnnounceOnBossKillVip;
												size_t pos = announce.find(L"$pj");
												while(pos != wstring::npos)
												{
													announce = announce.replace(pos, 3, charname);
													pos = announce.find(L"$pj");
												}

												CSystemMessage msg(2902);
												msg.AddText(announce.c_str());
												L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());

												g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", announce.c_str(), 2902);
											}
										}
									}
								}
								else
								{
									//AGREGAR VIP
									pUser->SocialAction(SocialActionLevelUp);
									
									if(pUser->pSD->vipLevel > 0)
									{
										g_Logger.Add(L"User[%s] DARION se agrego tiempo VIP level[%d]", pUser->pSD->wszName, 1);
										pUser->pSD->vipTimestamp += AnnounceOnBossKillVipTimespamp;
									}
									else
									{
										g_Logger.Add(L"User[%s] DARION se le dio estado VIP level[%d]", pUser->pSD->wszName, 1);
										pUser->pSD->vipTimestamp = time(0) + AnnounceOnBossKillVipTimespamp;
									}

									pUser->pSD->vipLevel = 1;
									g_DB.RequestSaveVipStatus(pUser->nDBID, 1, pUser->pSD->vipTimestamp);
									g_AccountDB.SetVIPLevel(pUser->pSocket->accountId, 1, pUser->pSD->vipTimestamp);

									wstringstream name;
									name << pUser->pSD->wszName;
									wstring charname;
									name >> charname;

									wstring announce = AnnounceOnBossKillVip;
									size_t pos = announce.find(L"$pj");
									while(pos != wstring::npos)
									{
										announce = announce.replace(pos, 3, charname);
										pos = announce.find(L"$pj");
									}

									CSystemMessage msg(2902);
									msg.AddText(announce.c_str());
									L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());

									g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", announce.c_str(), 2902);
								}
							}


							if(AnnounceOnBossKillStarsEnable && npcClassId == AnnounceOnBossKillStarsNpcId)		//DARION
							{
								if(CParty *pParty = pUser->GetParty())
								{
									UserVector vUsers;
									pParty->GetAllMembersEx(vUsers);
									for(UINT n=0;n<vUsers.size();n++)
									{
										if(User *pMember = vUsers[n]->SafeCastUser())
										{
											if(pMember->pSD->nAlive)
											{
												//AGREGAR VIP

												pMember->SocialAction(SocialActionLevelUp);

												pMember->AddItemToInventory(AnnounceOnBossKillStarsItemId, AnnounceOnBossKillStarsItemCount);	//items

												wstringstream name;
												name << pMember->pSD->wszName;
												wstring charname;
												name >> charname;

												wstring announce = AnnounceOnBossKillStars;
												size_t pos = announce.find(L"$pj");
												while(pos != wstring::npos)
												{
													announce = announce.replace(pos, 3, charname);
													pos = announce.find(L"$pj");
												}

												CSystemMessage msg(2902);
												msg.AddText(announce.c_str());
												L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());

												g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", announce.c_str(), 2902);
											}
										}
									}
								}
								else
								{
									//AGREGAR VIP
									pUser->SocialAction(SocialActionLevelUp);
									
									pUser->AddItemToInventory(AnnounceOnBossKillStarsItemId, AnnounceOnBossKillStarsItemCount);	//items


									wstringstream name;
									name << pUser->pSD->wszName;
									wstring charname;
									name >> charname;

									wstring announce = AnnounceOnBossKillStars;
									size_t pos = announce.find(L"$pj");
									while(pos != wstring::npos)
									{
										announce = announce.replace(pos, 3, charname);
										pos = announce.find(L"$pj");
									}

									CSystemMessage msg(2902);
									msg.AddText(announce.c_str());
									L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());

									g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", announce.c_str(), 2902);
								}
							}







							//[darion] = 1025601
							if(npcClassId == 1025601 && mDarEntradaPorMatarDarion)
							{
								if(CParty *pParty = pUser->GetParty())
								{
									UserVector vUsers;
									pParty->GetAllMembersEx(vUsers);
									for(UINT n=0;n<vUsers.size();n++)
									{
										if(User *pMember = vUsers[n]->SafeCastUser())
										{
											CItem *pItem = pMember->inventory.GetFirstItemByClassID(9641);
											if(pItem->IsValidItem())
											{

											}else
											{
												if(pMember->pSD->nAlive)
												{
													pMember->AddItemToInventory(9641, 1);	//entrada al siege

													wstringstream name;
													name << pMember->pSD->wszName;
													wstring charname;
													name >> charname;

													wstring announce = AnnounceOnBossKillDarion;
													size_t pos = announce.find(L"$pj");
													while(pos != wstring::npos)
													{
														announce = announce.replace(pos, 3, charname);
														pos = announce.find(L"$pj");
													}

													CSystemMessage msg(2902);
													msg.AddText(announce.c_str());
													L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());

													g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", announce.c_str(), 2902);
												}
											}
										}
									}
								}
								else
								{
									CItem *pItem = pUser->inventory.GetFirstItemByClassID(9641);
									if(pItem->IsValidItem())
									{

									}else
									{
										pUser->AddItemToInventory(9641, 1);	//entrada al siege

										wstringstream name;
										name << pUser->pSD->wszName;
										wstring charname;
										name >> charname;

										wstring announce = AnnounceOnBossKillDarion;
										size_t pos = announce.find(L"$pj");
										while(pos != wstring::npos)
										{
											announce = announce.replace(pos, 3, charname);
											pos = announce.find(L"$pj");
										}

										CSystemMessage msg(2902);
										msg.AddText(announce.c_str());
										L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());

										g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", announce.c_str(), 2902);
									}
								}
							}


				}

			}
			else
			{

			}
		}
	}
}