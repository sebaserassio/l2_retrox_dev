#include "stdafx.h"
#include "NpcSpawnEvent.h"
#include "ChatMobile.h"


IVector posTeleportZone[10];

CNpcSpawnEvent g_NpcSpawnEvent;

void CNpcSpawnEvent::Init()
{
	g_Log.Add(CLog::Blue, "[Load NpcSpawnEvent]");
	Lock();

		m_eventInfo.clear();

		m_lpEvent = new Evento();
		m_lpEvent->enabled = FALSE;
		m_lpEvent->estado = StateDetenido;

		m_enabled = FALSE;
		LoadINI();
		if(m_enabled)
		{
			g_Log.Add(CLog::Blue, "[%S] Feature is enabled, loaded [%d] events", L"NpcSpawnEvent", m_eventInfo.size());
		}
		else
		{
			g_Log.Add(CLog::Blue, "[%S] Feature is Disabled");
		}
	Unlock();
}

void CNpcSpawnEvent::LoadINI()
{
	TCHAR g_DinamicRatesFile[260];
	GetCurrentDirectory(MAX_PATH,g_DinamicRatesFile);

	lstrcat(g_DinamicRatesFile, m_ConfigsIlExt);

	lstrcat(g_DinamicRatesFile, TEXT("\\NpcSpawnEvent.ini"));
	const TCHAR* section = _T("NpcSpawnEvent");
	m_enabled = GetPrivateProfileInt(section, _T("Enabled"), FALSE, g_DinamicRatesFile);
	INT32 count = GetPrivateProfileInt(section, _T("NpcSpawnEventCount"), 0, g_DinamicRatesFile);
	if(count > 0)
	{
		for(int n=1;n<=count;n++)
		{
			tstringstream sectionEx;
			sectionEx << "SpawnEvent_" << n ;

			Evento* pInfo = new Evento();
			pInfo->enabled = GetPrivateProfileInt(sectionEx.str().c_str(), _T("Enabled"), FALSE, g_DinamicRatesFile);

			UINT dayFlag = GetPrivateProfileInt(sectionEx.str().c_str(), _T("DayFlag"), 0, g_DinamicRatesFile);
			for(UINT m = 1; m < 8; m++)
			{
				if((dayFlag % 10) == 1)
				{
					pInfo->dayFlag[m-1] = true;
				}else
				{
					pInfo->dayFlag[m-1] = false;
				}
				dayFlag /= 10;
			}
			pInfo->startHour = GetPrivateProfileInt(sectionEx.str().c_str(), _T("StartHour"), 14, g_DinamicRatesFile);
			pInfo->startMinute = GetPrivateProfileInt(sectionEx.str().c_str(), _T("StartMinute"), 30, g_DinamicRatesFile);

			pInfo->endHour = GetPrivateProfileInt(sectionEx.str().c_str(), _T("EndHour"), 14, g_DinamicRatesFile);
			pInfo->endMinute = GetPrivateProfileInt(sectionEx.str().c_str(), _T("EndMinute"), 30, g_DinamicRatesFile);


			pInfo->m_AnnounceInitMsgId = GetPrivateProfileInt(sectionEx.str().c_str(), _T("AnnounceInitMsgId"), 2901, g_DinamicRatesFile);
			pInfo->m_AnnounceEndMsgId = GetPrivateProfileInt(sectionEx.str().c_str(), _T("AnnounceEndMsgId"), 2901, g_DinamicRatesFile);

			TCHAR temp[16384] = { 0 };
			if(GetPrivateProfileString(sectionEx.str().c_str(), _T("AnnounceInitMsg"), 0, temp, 16384, g_DinamicRatesFile))
			{
				pInfo->m_AnnounceInitMsg = temp;
			}

			if(GetPrivateProfileString(sectionEx.str().c_str(), _T("AnnounceEndMsg"), 0, temp, 16384, g_DinamicRatesFile))
			{
				pInfo->m_AnnounceEndMsg = temp;
			}

			if(GetPrivateProfileString(sectionEx.str().c_str(), _T("EventName"), 0, temp, 16384, g_DinamicRatesFile))
			{
				pInfo->m_EventName = temp;
			}

			if (GetPrivateProfileString(sectionEx.str().c_str(), _T("Teleports"), 0, temp, 16384, g_DinamicRatesFile))
			{
				wstring respawn = temp;

				//g_Log.Add(CLog::Error, "[%S]", respawn.c_str());

				respawn = Parser::Replace(respawn, L';', L' ');
				respawn = Parser::Replace(respawn, L'{', L' ');
				respawn = Parser::Replace(respawn, L'}', L' ');


				//g_Log.Add(CLog::Error, "[%S]", respawn.c_str());

				wstringstream respawnStream(respawn);
				IVector pos;
				int i = 0;
				while (respawnStream >> pos.x >> pos.y >> pos.z)
				{



					pInfo->posTeleport[i].x = pos.x;
					pInfo->posTeleport[i].y = pos.y; // Corrige el valor para pos.y y pos.z
					pInfo->posTeleport[i].z = pos.z;
					pos.x = pos.y = pos.z = 0;
					i++;
				}
			}


			m_eventInfo.push_back(pInfo);
		}
	}
}


void CNpcSpawnEvent::EnterWorld(User* pUser)
{
	if(pUser->ValidUser())
	{
		if(m_lpEvent->enabled == TRUE && m_lpEvent->estado == StateIniciado)
		{
			CSystemMessage msg1(m_lpEvent->m_AnnounceInitMsgId);
			msg1.AddText(m_lpEvent->m_AnnounceInitMsg.c_str());
			pUser->pSocket->SendSystemMessage(&msg1);
		}
	}
}



void CNpcSpawnEvent::TimerExpired()
{
	guard;
	if(m_enabled)
	{
		Lock();
		if(m_lpEvent->estado == StateMensajeInit)
		{
			CSystemMessage msg(m_lpEvent->m_AnnounceInitMsgId);
			msg.AddText(m_lpEvent->m_AnnounceInitMsg.c_str());
			L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());

			g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", m_lpEvent->m_AnnounceInitMsg.c_str(), m_lpEvent->m_AnnounceInitMsgId);

			m_lpEvent->estado = StateIniciado;
			
			EnableNpcPosEvent(m_lpEvent->m_EventName.c_str());

			for (int i = 0; i < 10; ++i) {
				posTeleportZone[i] = m_lpEvent->posTeleport[i];

					//g_Log.Add(CLog::Error, "posTeleportZone x[%d]", posTeleportZone[i].x);
					//g_Log.Add(CLog::Error, "posTeleportZone y[%d]",  posTeleportZone[i].y);
					//g_Log.Add(CLog::Error, "posTeleportZone z[%d]",  posTeleportZone[i].z);


					//g_Log.Add(CLog::Error, "posTeleport x[%d]", m_lpEvent->posTeleport[i].x);
					//g_Log.Add(CLog::Error, "posTeleport y[%d]", m_lpEvent->posTeleport[i].y);
					//g_Log.Add(CLog::Error, "posTeleport z[%d]", m_lpEvent->posTeleport[i].z);

			}


		}else if(m_lpEvent->estado == StateIniciado)
		{
			tm ti; time_t rawtime;
			time ( &rawtime );
			localtime_s(&ti, &rawtime );
			if(m_lpEvent->endHour == ti.tm_hour && m_lpEvent->endMinute == ti.tm_min)
			{
				m_lpEvent->estado = StateMensajeFin;
			}
		}else if(m_lpEvent->estado == StateMensajeFin)
		{
			CSystemMessage msg(m_lpEvent->m_AnnounceEndMsgId);
			msg.AddText(m_lpEvent->m_AnnounceEndMsg.c_str());
			L2Server::BroadcastToAllUser(msg.GetSize(), msg.GetBuff());


			g_ChatMobile.EnviarMensajeJuegoAlProxy(L"", m_lpEvent->m_AnnounceEndMsg.c_str(), m_lpEvent->m_AnnounceEndMsgId);


			m_lpEvent->estado = StateDetenido;

			DisableNpcPosEvent(m_lpEvent->m_EventName.c_str());

			m_lpEvent = new Evento();

		}else
		{
			tm ti; time_t rawtime;
			time ( &rawtime );
			localtime_s(&ti, &rawtime );



			for(list<Evento*>::iterator Iter = m_eventInfo.begin();Iter!=m_eventInfo.end();Iter++)
			{
				Evento* pInfo = (*Iter);
				if(pInfo)
				{
					if(pInfo->enabled)
					{
						if(pInfo->dayFlag[ti.tm_wday])
						{

							bool activarEnvento = false;

							time_t hora_local = time(0);
							tm* fecha_hora = localtime(&hora_local);
							INT32 dia = fecha_hora->tm_mday;
							INT32 mes = fecha_hora->tm_mon;  // Sumar 1 porque los meses comienzan en 0
							INT32 anio = fecha_hora->tm_year;  // Sumar 1900 porque los años comienzan en 1900

							//struct tm time_in = {0, 0, 0, dia, mes, anio}; // 00:00:00 del 25 de mayo de 2023
							//time_t time_unix = mktime(&time_in);

							struct tm time_in2 = {0, pInfo->startMinute, pInfo->startHour, dia, mes, anio}; // 00:00:00 del 25 de mayo de 2023
							time_t tiempo_start = mktime(&time_in2);
	
							struct tm time_in3 = {0, pInfo->endMinute, pInfo->endHour, dia, mes, anio}; // 00:00:00 del 25 de mayo de 2023
							time_t tiempo_end = mktime(&time_in3);
						

							if(hora_local > tiempo_start && hora_local < tiempo_end)
							{
								activarEnvento = true;
							}

							if(activarEnvento)
							{
								m_lpEvent = pInfo;
								m_lpEvent->estado = StateMensajeInit;

								break;
							}
						}
					}
				}
			}
		}
		Unlock();
	}
	unguard;
}