#include "StdAfx.h"
#include "TeleportBypass.h"
#include "PlayerAction.h"
#include "Geo.h"
#include "FactionSystem.h"
#include "FactionCounter.h"
#include <iomanip> 
#include "PvPRank.h"



CTeleportBypass g_TeleportBypass;

extern double teleportEpicPer;

CTeleportBypass::CTeleportBypass()
{
	//aden_town:1500:147450:26741:-2204
};


static void PickTeleportDestination(const TeleportData& td, int& outX, int& outY, int& outZ)
{
    if (!td.spawns.empty())
    {
        // rand() ya lo usas en otros lados; compatible con VC++2005
        int idx = (int)(rand() % td.spawns.size());
        outX = td.spawns[idx].x;
        outY = td.spawns[idx].y;
        outZ = td.spawns[idx].z;
    }
    else
    {
        outX = td.x;
        outY = td.y;
        outZ = td.z;
    }
}

static bool ParseSpawnList(const std::string& line, std::vector<TeleportSpawn>& out)
{
    out.clear();

    const std::string key = "Spawns=";
    std::string::size_type p = line.find(key);
    if (p == std::string::npos)
        return false;

    // Desde el '=' hasta el final de la línea
    p += key.size();

    // Recorremos tokens {x/y/z}
    while (true)
    {
        std::string::size_type l = line.find('{', p);
        if (l == std::string::npos) break;
        std::string::size_type r = line.find('}', l + 1);
        if (r == std::string::npos) break;

        // contenido entre llaves: "x/y/z"
        std::string triplet = line.substr(l + 1, r - (l + 1));

        // parse x/y/z
        int sx = 0, sy = 0, sz = 0;
        if (sscanf(triplet.c_str(), "%d/%d/%d", &sx, &sy, &sz) == 3)
        {
            out.push_back(TeleportSpawn(sx, sy, sz));
        }

        p = r + 1;
    }

    return !out.empty();
}

void CTeleportBypass::Init()
{
	mData.clear();
	ReadData();
	g_Log.Add(CLog::Blue, "[%s] Loaded [%d] location(s).", __FUNCTION__, mData.size());
}

void CTeleportBypass::ReadData()
{
    guard;

    ifstream file(g_ChangeFolders.TeleportData_txt.c_str());
    if (file)
    {
        int nTeleports = 0;
        string sLine;
        mData.clear();

        while (getline(file, sLine))
        {
            if (sLine.find("//") == 0)
                continue;

            if (sLine.find("begin") == 0)
            {
                string Name = Parser::ParseOptionString(sLine, "name");
                int price   = Parser::ParseOptionInt(sLine, "price");
                int x       = Parser::ParseOptionInt(sLine, "x_loc");
                int y       = Parser::ParseOptionInt(sLine, "y_loc");
                int z       = Parser::ParseOptionInt(sLine, "z_loc");
                int min_level = Parser::ParseOptionInt(sLine, "min_level");
                int max_level = Parser::ParseOptionInt(sLine, "max_level");
                int required_item_id    = Parser::ParseOptionInt(sLine, "required_item_id");
                int required_item_count = Parser::ParseOptionInt(sLine, "required_item_count");
                int castle_id = Parser::ParseOptionInt(sLine, "castle_id");
                int faction_id= Parser::ParseOptionInt(sLine, "faction_id");

                int HourInit   = Parser::ParseOptionInt(sLine, "HourInit", 99);
                int HourEnd    = Parser::ParseOptionInt(sLine, "HourEnd", 99);
                int MinuteInit = Parser::ParseOptionInt(sLine, "MinuteInit", 99);
                int MinuteEnd  = Parser::ParseOptionInt(sLine, "MinuteEnd", 99);

                if (Name.size() > 0)
                {
                    try
                    {
                        TeleportData td;
                        td.x = x; td.y = y; td.z = z;
                        td.max_level = max_level;
                        td.min_level = min_level;
                        td.required_item_count = required_item_count;
                        td.required_item_id = required_item_id;
                        td.price = price;
                        td.castle_id = castle_id;
                        td.faction_id = faction_id;

                        td.hourInit = HourInit;
                        td.hourEnd = HourEnd;
                        td.minuteInit = MinuteInit;
                        td.minuteEnd = MinuteEnd;

                        // NUEVO: parsear lista de spawns, si existe
                        std::vector<TeleportSpawn> tmp;
                        if (ParseSpawnList(sLine, tmp))
                            td.spawns.swap(tmp);

                        WCHAR wTmpName[8192]; memset(wTmpName, 0, sizeof(wTmpName));
                        AnsiToUnicode((PCHAR)Name.c_str(), 8190, wTmpName, 8190);
                        wstring wName = wTmpName;

                        if (mData.find(wName) == mData.end())
                        {
                            mData.insert(std::make_pair(wName, td));
                        }
                        else
                        {
                            g_Log.Add(CLog::Error, "[%s] Teleport[%s] already exists. Duplicated entry.", __FUNCTION__, Name.c_str());
                        }
                    }
                    catch (...)
                    {
                        g_Log.Add(CLog::Error, "[%s] Exception while loading [%s][%d] teleport!", __FUNCTION__, Name.c_str(), nTeleports);
                    }
                    nTeleports++;
                }
            }
        }
        g_Log.Add(CLog::Blue, "[%s] Loaded [%d] teleports to database.", __FUNCTION__, mData.size());
    }
    else
    {
        g_Log.Add(CLog::Error, "[%s] Cannot find file[..\\Script\\TeleportData.txt]", __FUNCTION__);
    }

    unguard;
}



// Devuelve true si todos los campos de tiempo son válidos (0..23 / 0..59)
static inline bool TeleportHasTimeWindow(const TeleportData& td)
{
    return (td.hourInit   >= 0 && td.hourInit   <= 23) &&
           (td.minuteInit >= 0 && td.minuteInit <= 59) &&
           (td.hourEnd    >= 0 && td.hourEnd    <= 23) &&
           (td.minuteEnd  >= 0 && td.minuteEnd  <= 59);
}

static inline int ToMinutes(int h, int m)
{
    if (h < 0) h = 0; else if (h > 23) h = 23;
    if (m < 0) m = 0; else if (m > 59) m = 59;
    return h*60 + m;
}

static inline bool TeleportAllowedAt(const TeleportData& td, int hour, int minute)
{
    const int start = ToMinutes(td.hourInit,  td.minuteInit);
    const int end   = ToMinutes(td.hourEnd,   td.minuteEnd);
    const int now   = ToMinutes(hour, minute);

    if (start <= end)      return (now >= start) && (now <= end);   // mismo día
    else                   return (now >= start) || (now <= end);   // cruza medianoche
}



void CTeleportBypass::RequestTeleport(User *pUser, wstring wName)
{
	map<wstring, TeleportData>::iterator Iter = mData.find(wName);
	if(Iter!=mData.end())
	{
		if(!pUser->IsInCombatMode())
		{

			time_t now = time(0);


			SYSTEMTIME st;
			::GetLocalTime(&st);

			// Solo verifica si el horario está bien seteado (no 99 ni fuera de rango)
			if (TeleportHasTimeWindow(Iter->second))
			{
				if (!TeleportAllowedAt(Iter->second, (int)st.wHour, (int)st.wMinute))
				{
					std::wstringstream ss;
					ss << L"Este teleporte esta disponible de "
					   << std::setw(2) << std::setfill(L'0') << Iter->second.hourInit   << L":"
					   << std::setw(2) << std::setfill(L'0') << Iter->second.minuteInit << L" a "
					   << std::setw(2) << std::setfill(L'0') << Iter->second.hourEnd    << L":"
					   << std::setw(2) << std::setfill(L'0') << Iter->second.minuteEnd  << L".";

					std::wstring texto = ss.str();

					CSystemMessage msg(SystemMessage::S1_POPUP);
					msg.AddText(texto.c_str());
					pUser->pSocket->SendSystemMessage(&msg);
					return;
				}
			}


			if(Iter->second.max_level == 88)
			{
				if(!g_PvPRankEvent.IsActive())
				{
					int totaltiempo = pUser->pED->m_TimeResTeleport - now;
					std::wstringstream wss1;
					wss1 << "El evento de PVP Ranking no esta habilitado en este momento.";
					wstring value4 = wss1.str();
					const WCHAR * value3 = value4.c_str();

					CSystemMessage msg2(2907);
					msg2.AddText(value3);
					pUser->SendSystemMessage(&msg2);
					return;
				}
			}


			if(pUser->pED->m_TimeResTeleport > now && (wName == L"eventClan_1" || wName == L"eventClan_2" || wName == L"eventClan_3" || wName == L"eventClan_4" || wName == L"eventClan_5"))
			{
				int totaltiempo = pUser->pED->m_TimeResTeleport - now;
				std::wstringstream wss1;
				wss1 << "Tienes penalizacion de teleport por " << totaltiempo << " segundos.";
				wstring value4 = wss1.str();
				const WCHAR * value3 = value4.c_str();

				CSystemMessage msg2(2907);
				msg2.AddText(value3);
				pUser->SendSystemMessage(&msg2);
				return;
			}

			if(g_NpcMaker.BlockTeleportNpcPosEx(wName))
			{
				std::wstringstream wss1;
				wss1 << L"Ya no puedes usar el teleport al Epic Boss porque en estos momentos tiene menos del " << teleportEpicPer*100 << L" per de HP. En caso que se regenere HP y tenga mas que este limite, podras volver a viajar.";
				wstring value4 = wss1.str();
				const WCHAR * value3 = value4.c_str();

				CSystemMessage msg2(SystemMessage::S1_POPUP);
				msg2.AddText(value3);
				pUser->pSocket->SendSystemMessage(&msg2);
				return;
			}

			if( pUser->pSD->nLevel < Iter->second.min_level )
			{
				//2558	1	a,Your character level is to low.\0	0	79	9B	B0	FF	a,	a,	0	0	0	0	0	a,	a,none\0
				pUser->pSocket->SendSystemMessage(2558);
				return;
			}

			if( pUser->pSD->nLevel > Iter->second.max_level )
			{
				//2559	1	a,Your character level is to high.\0	0	79	9B	B0	FF	a,	a,	0	0	0	0	0	a,	a,none\0
				pUser->pSocket->SendSystemMessage(2559);
				return;
			}
			if(Iter->second.castle_id > 0)
			{
				CResidence *pCastle = g_ResidenceDB.GetResidence(Iter->second.castle_id);
				if(pCastle->IsCastle())
				{
					if(pCastle->IsSiegeTime())
					{
						//2560	1	a,You cannot use this teleport during the castle siege.\0	0	79	9B	B0	FF	a,	a,	0	0	0	0	0	a,	a,none\0
						pUser->pSocket->SendSystemMessage(2560);
						return;
					}
				}
			}
			if(pUser->olympiadUser.GetState() != 0)
			{
				//2561	1	a,You cannot use this teleport while participing in olympiad games!\0	0	79	9B	B0	FF	a,	a,	0	0	0	0	0	a,	a,none\0
				pUser->pSocket->SendSystemMessage(2561);
				return;
			}

			/*pUser->pSD->pExData->TeleportBypassData.Set(wName);
			//ask user
			CPacket pck;
			pck.WriteC(0xED);
			pck.WriteD(MsgID); //msg id 2413	1	You will be moved to ($s1). Do you wish to continue?	4	79	9B	B0	FF			0	0	0	0	0		popup

			pck.WriteD(1); //number of parameters
			pck.WriteD(7); //location
			pck.WriteD(Iter->second.x); // x
			pck.WriteD(Iter->second.y); // x
			pck.WriteD(Iter->second.z); // x
			pck.WriteD(9000); //timeout
			pck.WriteD(0); //dunno
			pUser->GetSocket()->Send("b", pck.GetSize(), pck.GetBuff());*/

			if( pUser->inventory.GetAdenaAmount() < Iter->second.price )
			{
				//2563	1	a,You don't have enough adena to pay for our service!\0	0	79	9B	B0	FF	a,	a,	0	0	0	0	0	a,	a,none\0
				pUser->pSocket->SendSystemMessage(2563);
				return;
			}

			if(Iter->second.required_item_id > 0 && Iter->second.required_item_count > 0)
			{
				if(!pUser->inventory.HaveItemByClassID(Iter->second.required_item_id, Iter->second.required_item_count))
				{
					//2564	1	a,You don't have enough item(s) to pay for our service!\0	0	79	9B	B0	FF	a,	a,	0	0	0	0	0	a,	a,none\0
					pUser->pSocket->SendSystemMessage(2564);
					return;
				}
				pUser->DeleteItemInInventory(Iter->second.required_item_id, Iter->second.required_item_count);
			}

			if(Iter->second.price > 0)
				pUser->DeleteItemInInventory(57, Iter->second.price);

			bool cantp = true;
			if(Iter->second.faction_id > 0)
			{
				if( pUser->pSD->nCursedOwner )
				{
					cantp = false;
				}
				else
				{
					if( g_FactionCounter.GetRedTeamCount() > g_FactionCounter.GetGreenTeamCount() )
					{
						pUser->pED->nFactionID = 2; //Green
						g_FactionCounter.IncreaseGreenTeam();
					}
					else if( g_FactionCounter.GetGreenTeamCount() > g_FactionCounter.GetRedTeamCount() )
					{
						pUser->pED->nFactionID = 1; //Red
						g_FactionCounter.IncreaseRedTeam();
					}
					else
					{
						int nRand = rand() % 2 + 1;
						pUser->pED->nFactionID = nRand;

						if( nRand == 1 )
						{
							g_FactionCounter.IncreaseRedTeam();
						}
						else if( nRand == 2 )
						{
							g_FactionCounter.IncreaseGreenTeam();
						}
					}
				}
			}

			if( cantp )
			{
                int tx, ty, tz;
                PickTeleportDestination(Iter->second, tx, ty, tz);


				pUser->SendCleanGARBAGE();

				pUser->StopMove();
                PlayerAction::OnTeleportToLocation(pUser, tx, ty, CGeo::GetInstance()->GetZLoc(tx, ty, tz), false);
			}

			//pUser->StopMove();
			//PlayerAction::OnTeleportToLocation(pUser, Iter->second.x, Iter->second.y, CGeo::GetInstance()->GetZLoc(Iter->second.x, Iter->second.y, Iter->second.z), false);

		}else
		{
			//2562	1	a,You cannot use teleport in combat mode!\0	0	79	9B	B0	FF	a,	a,	0	0	0	0	0	a,	a,none\0
			pUser->pSocket->SendSystemMessage(2562);
		}

	}
}

void CTeleportBypass::Teleport(User *pUser, wstring wName)
{
	map<wstring, TeleportData>::iterator Iter = mData.find(wName);
	if(Iter!=mData.end())
	{
		if(!pUser->IsInCombatMode())
		{
			if( pUser->pSD->nLevel < Iter->second.min_level )
			{
				//2558	1	a,Your character level is to low.\0	0	79	9B	B0	FF	a,	a,	0	0	0	0	0	a,	a,none\0
				pUser->pSocket->SendSystemMessage(2558);
				return;
			}

			if( pUser->pSD->nLevel > Iter->second.max_level )
			{
				//2559	1	a,Your character level is to high.\0	0	79	9B	B0	FF	a,	a,	0	0	0	0	0	a,	a,none\0
				pUser->pSocket->SendSystemMessage(2559);
				return;
			}

			if( pUser->inventory.GetAdenaAmount() < Iter->second.price )
			{
				//2563	1	a,You don't have enough adena to pay for our service!\0	0	79	9B	B0	FF	a,	a,	0	0	0	0	0	a,	a,none\0
				pUser->pSocket->SendSystemMessage(2563);
				return;
			}

			if(Iter->second.required_item_id > 0 && Iter->second.required_item_count > 0)
			{
				if(!pUser->inventory.HaveItemByClassID(Iter->second.required_item_id, Iter->second.required_item_count))
				{
					//2564	1	a,You don't have enough item(s) to pay for our service!\0	0	79	9B	B0	FF	a,	a,	0	0	0	0	0	a,	a,none\0
					pUser->pSocket->SendSystemMessage(2564);
					return;
				}
				pUser->DeleteItemInInventory(Iter->second.required_item_id, Iter->second.required_item_count);
			}

			if(Iter->second.price > 0)
				pUser->DeleteItemInInventory(57, Iter->second.price);

			bool cantp = true;
			if(Iter->second.faction_id > 0)
			{
				if( pUser->pSD->nCursedOwner )
				{
					cantp = false;
				}
				else
				{
					if( g_FactionCounter.GetRedTeamCount() > g_FactionCounter.GetGreenTeamCount() )
					{
						pUser->pED->nFactionID = 2; //Green
						g_FactionCounter.IncreaseGreenTeam();
					}
					else if( g_FactionCounter.GetGreenTeamCount() > g_FactionCounter.GetRedTeamCount() )
					{
						pUser->pED->nFactionID = 1; //Red
						g_FactionCounter.IncreaseRedTeam();
					}
					else
					{
						int nRand = rand() % 2 + 1;
						pUser->pED->nFactionID = nRand;

						if( nRand == 1 )
						{
							g_FactionCounter.IncreaseRedTeam();
						}
						else if( nRand == 2 )
						{
							g_FactionCounter.IncreaseGreenTeam();
						}
					}
				}
			}

			if( cantp )
			{
                int tx, ty, tz;
                PickTeleportDestination(Iter->second, tx, ty, tz);

				pUser->StopMove();
                PlayerAction::OnTeleportToLocation(pUser, tx, ty, CGeo::GetInstance()->GetZLoc(tx, ty, tz), false);
			}
		}else
		{
			//2562	1	a,You cannot use teleport in combat mode!\0	0	79	9B	B0	FF	a,	a,	0	0	0	0	0	a,	a,none\0
			pUser->pSocket->SendSystemMessage(2562);
		}
	}else
		g_Log.Add(CLog::Error, "[%s] User[%S] requested invalid teleport name[%S]", __FUNCTION__, pUser->pSD->wszName, wName.c_str());
}