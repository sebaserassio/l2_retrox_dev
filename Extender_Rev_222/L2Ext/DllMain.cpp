#include "StdAfx.h"
#include "Packets.h"
#include "LogControl.h"
#include "MenuExtension.h"
#include "PostLoad.h"
#include "BuildCmdHandler.h"
#include "Augmentation.h"
#include "CreatureAction.h"
#include "PlayerAction.h"
#include "NpcAction.h"
#include "PetAction.h"
#include "OtherAction.h"
#include "MPCC.h"
#include "IOBufferExt.h"
#include "HWID.h"
#include "NuevasFunciones.h"
#include "Hair2Slot.h"
#include "ChangeFolders.h"
#include "ChatMobile.h"

#pragma optimize("", off)

bool g_Initialized = false;

#include "VMProtect/VMProtectSDK.h"

DllExport BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved)
{
	VMProtectBeginUltra("DllMain::DllMain");

	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			if(!g_Initialized)
			{
				g_Initialized = true;

				LPCWSTR lpCommandLine =  GetCommandLineW();
				BOOL resetKey = 0;
				BOOL hwid = 0;
				if(lpCommandLine)
				{
					INT32 argCount = 0;
					LPWSTR* lpArgs = CommandLineToArgvW(lpCommandLine, &argCount);
					for(INT32 n=0;n<argCount;n++)
					{
						LPWSTR lpArg = lpArgs[n];
						try
						{
							wstring arg(lpArg);
							size_t pos = arg.find(L"-");
							while(pos != wstring::npos)
							{
								arg.erase(pos, 1);
								pos = arg.find(L"-");
							}
							pos = arg.find(L" ");
							while(pos != wstring::npos)
							{
								arg.erase(pos, 1);
								pos = arg.find(L" ");
							}
							if(arg == L"resetkey")
							{
								resetKey = TRUE;
							}
							if(arg == L"hwid")
							{
								hwid = TRUE;
							}
						}catch(...)
						{

						}
					}
				}

				if(hwid)
				{
					std::ofstream o("hwid.txt");

					o << g_HWID.GetHash() << std::endl;
				}

				g_ChangeFolders.Init();

				g_SplashScreen.Init(NULL, hInstance, IDB_BITMAP2);
				g_SplashScreen.Show();


				g_ChatMobile.Init();

				L2Ext::BaseData();

				L2Ext::FixNPCnItemID();
				L2Ext::EtcHook();
				g_CallStack.Init();
				User::ExtInit();
				CUserSocket::ExtInit();
				CPostLoad::Initialize();
				CSharedFactory::Initialize();
				ClassExtender::Initialize();
				PlayerAction::Initialize();
				NpcAction::Initialize();
				PetAction::Initialize();
				CreatureAction::Initialize();
				COtherAction::Initialize();
				CL2Time::Initialize();
				BuffSlots::Initialize();
				g_Augmentation.Initialize();
				Quests::Initialize();
				CLogControl::Initialize();
				CPacketFix::Initialize();
				CMPCC::Initialize();
				CFreightSystem::Initialize();
				CSkillFx::Initialize();
				CBuilderCommand::Initialize();
				CMenuExtension::Install();
				InitializeSkillEnchanting();
				Hair2Slot::Initialize();

	#ifdef EXTEND_IOBUFFER
				CIOBufferExt::Init();
	#endif



//				ShowMessage("CreatureSharedData", "CSkillInfo Size %p - CSkillInfo Original Size %p", sizeof(CSkillInfo), offsetof(CSkillInfo, CSkillInfo::n164));

	//			ShowMessage("CreatureSharedData", "Size %p, lastVar %p", sizeof(CreatureSharedData), offsetof(CreatureSharedData, CreatureSharedData::_nextFreeVariable));
	//			ShowMessage("CCreature", "Size %p, lastVar %p", sizeof(CCreature), offsetof(CCreature, CCreature::_unkn14F8));
	//			ShowMessage("User", "Size %p, lastVar %p", sizeof(User), offsetof(User, User::nObservationTime));
				DisableThreadLibraryCalls(hInstance);
			}
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			break;
		}
	}


	return true;
	VMProtectEnd();
}


#pragma optimize("", on)