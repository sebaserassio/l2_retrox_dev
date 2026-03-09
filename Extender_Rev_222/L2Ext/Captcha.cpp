#include "StdAfx.h"
#include "Captcha.h"
#include "HtmlCache.h"
#include "PlayerAction.h"
#include "DB.h"
#include "JailSystem.h"

CCaptcha g_Captcha;

CCaptcha::CCaptcha()
{
}

CCaptcha::~CCaptcha()
{
}

vector<INT32> g_CaptchaClasses;

void CCaptcha::Init()
{
	m_Enabled = FALSE;
	m_MobsKilled = 20;
	m_Chance = 70;
	m_skillId = 5041;
	m_Times_Village = 3;
	m_Times_Jail = 10;
	m_Captcha_Mode = 0;
	m_RewardActive = 0;
	m_RewardID = 0;
	m_RewardQuanity = 0;
	m_TimeoutTime = 30;
	m_TimeoutTimeRecaptcha = 250;
	VKickX = 0;
	VKickY = 0;
	VKickZ = 0;
	m_JailTime = 60;
	m_Enabled_Recaptcha = FALSE;
	m_Enabled_Recaptcha_Only = FALSE;
	m_DoubleJail = 0;
	m_ReplaceETC = 0;

	LoadINI();
	if(m_Enabled)
	{
		g_Log.Add(CLog::Blue, "[%s] Feature is enabled.", __FUNCTION__);
	}
}

/* INI section
[CaptchaSystem]
Enabled=1
;ReplaceETCBuffStatusIcon
ReplaceETCBuffIcon=0
;Quanity of Mobs Per Cycle of Captcha
MobsPerCheck=40
;Quanity of Mobs Per Cycle of Captcha For Pole Classes
MobsPerCheckPole=40
;Possibility After Mobs Killed
ChancePerMobsRound=50
;Protection Skill During Captcha
SkillID=5041
;Wrong Times until Send Player To Village
TimesForVillage=3
VillageKickPos=0/0/0
;Wrong Times until Send Player To Jail
TimesForJail=7
;Enable/Disable Captcha Reward
RewardActive=0
;Item Reward ID
RewardID=0
;Quanity Reward
RewardQuanity=0
;Classes For Check Captcha By Time
CaptchaClasses=1 0 0 0
;Time For Check Captcha By Time
CaptchaClassesTime=900
;Check For Classes Captcha By Time if they are into dualbox
CaptchaClassesDualbox=1
;Check Distance Between Players
CaptchaDistanceHwid=300
;TimeoutTime For Captcha
TimeoutTime=30
;JailTime into seconds
JailTime=60
;Enable Recaptcha Captcha
EnableRecaptcha=True
;Only Recaptcha Show (Must be enabled first)
EnableRecaptchaOnly=True
;Timeout Time for Re-Captcha
RecaptchaTimeoutTime=250
;Link For Re-Captcha
RecaptchaLink=http://l2gold.cc/captcha.php?player=<?PlayerName?>&uniqueid=<?UniqueID?>
;Double Time on EveryCaptcha Mistake
DoubleTime=1
;Captcha Mode (Only pick 1 mode) (0=Numbers/1=Colors/2=Math)
CaptchaMode=0
*/
//g_SocketDB.GetHWIDCount(pSocket->pED->longHWID.hash)
//pUser->pSocket->

void CCaptcha::LoadINI()
{
	const TCHAR * section = _T("CaptchaSystem");
	m_Enabled = GetPrivateProfileInt(section, _T("Enabled"), 0, g_ConfigFile);
	m_MobsKilled = GetPrivateProfileInt(section, _T("MobsPerCheck"), 1, g_ConfigFile);
	m_MobsKilledPole = GetPrivateProfileInt(section, _T("MobsPerCheckPole"), 1, g_ConfigFile);
	m_Chance = GetPrivateProfileInt(section, _T("ChancePerMobsRound"), 1, g_ConfigFile);
	m_skillId = GetPrivateProfileInt(section, _T("SkillID"), 0, g_ConfigFile);
	m_Times_Village = GetPrivateProfileInt(section, _T("TimesForVillage"), 0, g_ConfigFile);
	m_Times_Jail = GetPrivateProfileInt(section, _T("TimesForJail"), 0, g_ConfigFile);
	m_Captcha_Mode = GetPrivateProfileInt(section, _T("CaptchaMode"), 0, g_ConfigFile);
	m_RewardActive = GetPrivateProfileInt(section, _T("RewardActive"), 0, g_ConfigFile);
	m_RewardID = GetPrivateProfileInt(section, _T("RewardID"), 0, g_ConfigFile);
	m_RewardQuanity = GetPrivateProfileInt(section, _T("RewardQuanity"), 0, g_ConfigFile);

	m_TimeoutTime = GetPrivateProfileInt(section, _T("TimeoutTime"), 30, g_ConfigFile);
	m_TimeoutTimeRecaptcha = GetPrivateProfileInt(section, _T("RecaptchaTimeoutTime"), 30, g_ConfigFile);
	m_JailTime = GetPrivateProfileInt(section, _T("JailTime"), 60, g_ConfigFile);

	m_Enabled_Recaptcha = GetPrivateProfileInt(section, _T("EnableRecaptcha"), 0, g_ConfigFile);
	m_Enabled_Recaptcha_Only = GetPrivateProfileInt(section, _T("EnableRecaptchaOnly"), 0, g_ConfigFile);
	m_DoubleJail = GetPrivateProfileInt(section, _T("DoubleTime"), 0, g_ConfigFile);

	m_ReplaceETC = GetPrivateProfileInt(section, _T("ReplaceETCBuffIcon"), 0, g_ConfigFile);

	memset(m_TemplateLink, 0, sizeof(m_TemplateLink));

	TCHAR sTemp[0x8000];
	memset(sTemp, 0, sizeof(sTemp));


	TCHAR temp[0x8000];

	m_TestHWID.clear();
	if(GetPrivateProfileString(section, _T("HWIDList"), 0, temp, 0x8000, g_ConfigFile))
	{
		wstring limpio;
		wstringstream hwidStreamTmp(temp);
		hwidStreamTmp >> limpio;

		limpio = Utils::ReplaceStringW(limpio, L"{", L"", true);
		limpio = Utils::ReplaceStringW(limpio, L"}", L"", true);
		limpio = Utils::ReplaceStringW(limpio, L";", L" ", true);

		wstringstream hwidStream(limpio);
		wstring wHwid;
		while(hwidStream >> wHwid)
		{
			HWID hwid(wHwid);
			m_TestHWID.push_back(hwid);
		}
	}


	if(GetPrivateProfileString(section, _T("VillageKickPos"), 0, sTemp, 8192, g_ConfigFile))
	{
		tstring pos = sTemp;
		pos = Utils::ReplaceString(pos, _T("/"), _T(" "), true);
		tstringstream sstr;
		sstr << pos;
		sstr >> VKickX >> VKickY >> VKickZ;
	}else
	{
		VKickX = 0;
		VKickY = 0;
		VKickZ = 0;
	}

	g_CaptchaClasses.clear();
	if(GetPrivateProfileString(section, _T("CaptchaClasses"), 0, sTemp, 0x4000, g_ConfigFile))
	{
		wstringstream classStream;
		classStream << sTemp;
		int classID = 0;
		while(classStream >> classID)
		{
			g_CaptchaClasses.push_back(classID);
		}
	}

	GetPrivateProfileString(section, _T("RecaptchaLink"), 0, m_TemplateLink, 260, g_ConfigFile);

}


bool CCaptcha::IsTestHWID(LPBYTE lpHwid)
{
	guard;
	bool ret = false;

	HWID hwid(lpHwid);

	if(m_TestHWID.size() > 0)
	{
		for(UINT n=0;n<m_TestHWID.size();n++)
		{
			if(m_TestHWID[n].IsEqual(hwid))
			{
				ret = true;
				break;
			}
		}
	}else
	{
		ret = true;
	}

	unguard;
	return ret;
}

void CCaptcha::OnDie(CCreature *pKiller, CNPC *pNpc)
{
	guard;

	if( m_Enabled && pNpc->ValidCreature() )
	{
		if( !pNpc->IsZZoldagu() && !pNpc->IsBoss() && !pNpc->IsSummon() && !pNpc->IsPet() )
		{
			if(User *pUser = pKiller->SafeCastUser())
			{
				if( pUser->pSD->nKarma == 0 )
				{
					if(pUser->IsNowTrade())
						pUser->TradeCancel();

					if( pUser->pSD->bGuilty == 0 && pUser->pED->PedirCaptcha)
					{
						pUser->pED->mobsKilled++;
						int nMobsKilled = pUser->pED->mobsKilled;
						
						if( (nMobsKilled >= m_MobsKilled) && (pUser->pSD->nWeaponType != 4) )
						{
							int nChance = g_MTRandom.RandInt(99);
							if(nChance < m_Chance)
							{
								pUser->pED->captchaProtectionTimeout = 0;
								pUser->pED->captchaProtectionTimeSent = 0;

								DoCaptcha(pUser,true);
							}
							pUser->pED->mobsKilled = 0;
						}
						else if( (nMobsKilled >= m_MobsKilledPole) && (pUser->pSD->nWeaponType == 4) )
						{
							int nChance = g_MTRandom.RandInt(99);
							if(nChance < m_Chance)
							{
								pUser->pED->captchaProtectionTimeout = 0;
								pUser->pED->captchaProtectionTimeSent = 0;

								DoCaptcha(pUser,true);
							}
							pUser->pED->mobsKilled = 0;
						}
					}
				}
			}
		}
	}
	unguard;
}

void CCaptcha::ActivateTimer(User *pUser, bool recaptcha)
{
	guard;
	if(pUser->ValidCreature() && pUser->olympiadUser.GetState() == 0 && !pUser->pSD->nIsObserver)
	{
		UINT timetoshow = 0;
		if(!recaptcha)
		{
			if(m_TimeoutTime)
			{
				pUser->pED->captchaProtectionTimeout = static_cast<UINT>(time(0) + m_TimeoutTime);
				timetoshow = m_TimeoutTime;
			}
		}
		else
		{
			if(m_TimeoutTimeRecaptcha)
			{
				pUser->pED->captchaProtectionTimeout = static_cast<UINT>(time(0) + m_TimeoutTimeRecaptcha);
				timetoshow = m_TimeoutTimeRecaptcha;
			}	
		}

		pUser->pED->captchaPeriod = 1;
		//2634	1	Captcha System: You have $s1 second(s) to solve this question.	0	79	9B	B0	FF			0	0	0	0	0		none

		wstring announce = _T("Captcha System: You have $s1 second(s) to solve this question.");
		announce = Utils::ReplaceString(announce, _T("$s1"), Utils::IntToWString(timetoshow));
		pUser->SendSystemMessage(announce.c_str());

	//	CSystemMessage msg(2634);
	//	msg.AddNumber(timetoshow);
	//	pUser->SendSystemMessage(&msg);
	}
	unguard;
}

void CCaptcha::DoPunishment(User *pUser, bool timeoff = 0)
{
	guard;
	if(pUser->ValidUser())
	{
		if(timeoff)
		{
			pUser->pED->captchaProtectionTimeout = 0;
			pUser->pED->captchaProtectionTimeSent = 0;
			pUser->pED->captchaPeriod = 0;
			pUser->pED->captchaWrongTimes = 0;


			//2636	1	Captcha System: Time Up! You will teleported back to town.	0	79	9B	B0	FF			0	0	0	0	0		none
			//pUser->SendSystemMessage(2636);

			pUser->SendSystemMessage(L"Captcha System: Time Up! You will teleported back to town.");

			PlayerAction::OnTeleportToLocation(pUser, VKickX, VKickY, VKickZ, false);
			//DoCaptcha(pUser,false);
		}
		else
		{
			pUser->pED->captchaPeriod = 1;
			pUser->pED->captchaWrongTimes = pUser->pED->captchaWrongTimes + 1;

			if( pUser->pED->captchaWrongTimes == m_Times_Village )
			{
				pUser->pED->captchaProtectionTimeout = 0;
				pUser->pED->captchaProtectionTimeSent = 0;

				//2629	1	Captcha System: Multiple Wrong Answer! You will teleported back to Town!	0	79	9B	B0	FF			0	0	0	0	0		none
				//pUser->SendSystemMessage(2629);
				pUser->SendSystemMessage(L"Captcha System: Multiple Wrong Answer! You will teleported back to Town!");
				PlayerAction::OnTeleportToLocation(pUser, VKickX, VKickY, VKickZ, false);
				//DoCaptcha(pUser,true);
				//paei sto village
			}
			else
			{
				pUser->pED->captchaProtectionTimeout = 0;
				pUser->pED->captchaProtectionTimeSent = 0;

				//2640	1	Captcha System: Be carefull you may get punished!	0	79	9B	B0	FF			0	0	0	0	0		none
				//pUser->SendSystemMessage(2640);
				pUser->SendSystemMessage(L"Captcha System: Be carefull you may get punished!");
				DoCaptcha(pUser,true);
			}
		}
		g_DB.RequestSaveCaptchaStatus(pUser->nDBID, pUser->pED->mobsKilled, pUser->pED->captchaWrongTimes, pUser->pED->captchaPeriod, 0, 0, pUser->pED->captchaJail);
	}
	unguard;
}

void CCaptcha::EnterWorld(User *pUser)
{
	guard;
	if(pUser->ValidUser())
	{
		//do something
	}
	unguard;
}

void CCaptcha::DoCaptcha(User *pUser, bool timeoff = false, bool recaptcha)
{
	if(pUser->ValidUser())
	{
		if( pUser->pED->captchaProtectionTimeout > 0 )
			return;

		//Block Damage Or Movement From Char
		if( g_Captcha.GetSkillId() > 0 )
		{
			pUser->pED->captchaPeriod = 1;
			pUser->AcquireSkill(g_Captcha.GetSkillId(), 1);
			pUser->ValidateParameters();
			pUser->SendETCBuffStatus();
		}
		//Save it on DB
		g_DB.RequestSaveCaptchaStatus(pUser->nDBID, pUser->pED->mobsKilled, pUser->pED->captchaWrongTimes, pUser->pED->captchaPeriod, 0, 0, pUser->pED->captchaJail);

		//Start of Captcha Choose
		int nRandomMode = 0;

		if( m_Enabled_Recaptcha ) 
		{ 
			if( m_Enabled_Recaptcha_Only || recaptcha ) 
			{ 
				nRandomMode = 3; 
			} 
			else 
			{ 
				nRandomMode = g_MTRandom.RandInt(3); 
			} 
		} 
		else if(m_Captcha_Mode)
		{
			nRandomMode = m_Captcha_Mode;
		}
		else 
		{ 
			nRandomMode = g_MTRandom.RandInt(2); 
		}
		//End of Captcha Choose

		if(timeoff)
		{
			if( nRandomMode == 3 )
			{
				ActivateTimer(pUser,true);
			}
			else
			{
				ActivateTimer(pUser);
			}
		}

		if( nRandomMode == 0 ) //numbers captcha
		{
			//Create Random String
			int nRandomQuestion = g_MTRandom.RandInt(99);
			int nRandomQuestionWrong1 = g_MTRandom.RandInt(99);
			int nRandomQuestionWrong2 = g_MTRandom.RandInt(99);

			wstring qCorrentQuestion = Utils::RandomString(6);
			wstring qWrongQuestion1 = Utils::RandomString(6);
			wstring qWrongQuestion2 = Utils::RandomString(6);

			pUser->pED->captchaQuestion = qCorrentQuestion;
			pUser->pED->captchaWQuestion1 = qWrongQuestion1;
			pUser->pED->captchaWQuestion2 = qWrongQuestion2;

			int nRandomPos = g_MTRandom.RandInt(2);

			//Show Captcha Page
			const WCHAR* wHTML = g_HtmlCache.Get(L"captcha_page1.htm");
			if(wHTML)
			{
				wstring html(wHTML);
				html = Utils::ReplaceString(html, L"<?captcha_question?>", nRandomQuestion, true);
	
				if( nRandomPos == 0 )
				{
					html = Utils::ReplaceString(html, L"<?captcha_answer1?>", nRandomQuestion, true);
					html = Utils::ReplaceString(html, L"<?captcha_answer2?>", nRandomQuestionWrong1, true);
					html = Utils::ReplaceString(html, L"<?captcha_answer3?>", nRandomQuestionWrong2, true);
					html = Utils::ReplaceString(html, L"<?captcha_answer1h?>", qCorrentQuestion, true); //html bypass
					html = Utils::ReplaceString(html, L"<?captcha_answer2h?>", qWrongQuestion1, true); //html bypass
					html = Utils::ReplaceString(html, L"<?captcha_answer3h?>", qWrongQuestion2, true); //html bypass
				}
				else if( nRandomPos == 1 )
				{
					html = Utils::ReplaceString(html, L"<?captcha_answer1?>", nRandomQuestionWrong1, true);
					html = Utils::ReplaceString(html, L"<?captcha_answer2?>", nRandomQuestion, true);
					html = Utils::ReplaceString(html, L"<?captcha_answer3?>", nRandomQuestionWrong2, true);
					html = Utils::ReplaceString(html, L"<?captcha_answer1h?>", qWrongQuestion1, true); //html bypass
					html = Utils::ReplaceString(html, L"<?captcha_answer2h?>", qCorrentQuestion, true); //html bypass
					html = Utils::ReplaceString(html, L"<?captcha_answer3h?>", qWrongQuestion2, true); //html bypass
				}
				else if( nRandomPos == 2 )
				{
					html = Utils::ReplaceString(html, L"<?captcha_answer1?>", nRandomQuestionWrong1, true);
					html = Utils::ReplaceString(html, L"<?captcha_answer2?>", nRandomQuestionWrong2, true);
					html = Utils::ReplaceString(html, L"<?captcha_answer3?>", nRandomQuestion, true);
					html = Utils::ReplaceString(html, L"<?captcha_answer1h?>", qWrongQuestion1, true); //html bypass
					html = Utils::ReplaceString(html, L"<?captcha_answer2h?>", qWrongQuestion2, true); //html bypass
					html = Utils::ReplaceString(html, L"<?captcha_answer3h?>", qCorrentQuestion, true); //html bypass
				}
	
				pUser->ShowHTML(L"captcha_page1.htm", html.c_str(), 0);
			}
		}
		else if( nRandomMode == 1 ) //colors captcha
		{
			//blue = 0, green = 1, orange = 2, yellow = 3, purple = 4, grey = 5, red = 6
			int nRandomColor = g_MTRandom.RandInt(6);
			//Set The Correct Random Number
			//int nRandomColorCorrect = g_MTRandom.RandInt(99);
			//pUser->pED->captchaQuestion = nRandomColorCorrect;
			//Set The Falso Random Number
			//int nRandomColorFalse1 = PickNumber(99,nRandomColorCorrect);
			//int nRandomColorFalse2 = PickNumber2(99,nRandomColorCorrect,nRandomColorFalse1);

			wstring qCorrentQuestion = Utils::RandomString(6);
			wstring qWrongQuestion1 = Utils::RandomString(6);
			wstring qWrongQuestion2 = Utils::RandomString(6);

			pUser->pED->captchaQuestion = qCorrentQuestion;
			pUser->pED->captchaWQuestion1 = qWrongQuestion1;
			pUser->pED->captchaWQuestion2 = qWrongQuestion2;




			g_Log.Add(CLog::Blue, "[%s] [%S]", __FUNCTION__, qCorrentQuestion.c_str());
			g_Log.Add(CLog::Blue, "[%s] [%S]", __FUNCTION__, qWrongQuestion1.c_str());
			g_Log.Add(CLog::Blue, "[%s] [%S]", __FUNCTION__, qWrongQuestion2.c_str());


			wstring wColorBlue = PickColor(1);
			wstring wColorGreen = PickColor(2);
			wstring wColorOrange = PickColor(3);
			wstring wColorYellow = PickColor(4);
			wstring wColorPurple = PickColor(5);
			wstring wColorGrey = PickColor(6);
			wstring wColorRed = PickColor(7);

			wstring wColorButtonBlue = L"icon2.random.but2";
			wstring wColorButtonGreen = L"icon2.random.but1";
			wstring wColorButtonOrange = L"icon2.random.but4";
			wstring wColorButtonYellow = L"icon2.random.but7";
			wstring wColorButtonPurple = L"icon2.random.but5";
			wstring wColorButtonGrey = L"icon2.random.but3";
			wstring wColorButtonRed = L"icon2.random.but6";

			wstring wColorButtonDownBlue = L"icon2.random.but2";
			wstring wColorButtonDownGreen = L"icon2.random.but1";
			wstring wColorButtonDownOrange = L"icon2.random.but4";
			wstring wColorButtonDownYellow = L"icon2.random.but7";
			wstring wColorButtonDownPurple = L"icon2.random.but5";
			wstring wColorButtonDownGrey = L"icon2.random.but3";
			wstring wColorButtonDownRed = L"icon2.random.but6";

			//Show Captcha Page
			const WCHAR* wHTML = g_HtmlCache.Get(L"captcha_page2.htm");
			if(wHTML)
			{
				wstring wColor = L"";
				wstring wButton = L"";
				wstring wButtonDown = L"";
				wstring wColor2 = L"";
				wstring wButton2 = L"";
				wstring wButton2Down = L"";
				wstring wColor3 = L"";
				wstring wButton3 = L"";
				wstring wButton3Down = L"";

				if( nRandomColor == 0 )	{ wColor = wColorBlue; wButton = wColorButtonBlue; wButtonDown = wColorButtonDownBlue; }
				else if( nRandomColor == 1 ) { wColor = wColorGreen; wButton = wColorButtonGreen; wButtonDown = wColorButtonDownGreen; }
				else if( nRandomColor == 2 ) { wColor = wColorOrange; wButton = wColorButtonOrange; wButtonDown = wColorButtonDownOrange; }
				else if( nRandomColor == 3 ) { wColor = wColorYellow; wButton = wColorButtonYellow; wButtonDown = wColorButtonDownYellow; }
				else if( nRandomColor == 4 ) { wColor = wColorPurple; wButton = wColorButtonPurple; wButtonDown = wColorButtonDownPurple; }
				else if( nRandomColor == 5 ) { wColor = wColorGrey; wButton = wColorButtonGrey; wButtonDown = wColorButtonDownGrey; }
				else if( nRandomColor == 6 ) { wColor = wColorRed; wButton = wColorButtonRed; wButtonDown = wColorButtonDownRed; }

				wstring html(wHTML);
				html = Utils::ReplaceString(html, L"<?captcha_question?>", wColor, true);

				int nRandomPos = g_MTRandom.RandInt(2);
				if( nRandomPos == 0 )
				{
					//Correct Button
					html = Utils::ReplaceString(html, L"<?captcha_a1_color?>", wColor, true);
					html = Utils::ReplaceString(html, L"<?captcha_a1_number?>", qCorrentQuestion, true);
					html = Utils::ReplaceString(html, L"<?captcha_a1_button?>", wButton, true);
					html = Utils::ReplaceString(html, L"<?captcha_a1_button2?>", wButtonDown, true);
					
					//False Button1
					int nQuestionRandom2 = PickNumber(6,nRandomColor);
					if( nQuestionRandom2 == 0 )	{ wColor2 = wColorBlue; wButton2 = wColorButtonBlue; wButton2Down = wColorButtonDownBlue; }
					else if( nQuestionRandom2 == 1 ) { wColor2 = wColorGreen; wButton2 = wColorButtonGreen; wButton2Down = wColorButtonDownGreen; }
					else if( nQuestionRandom2 == 2 ) { wColor2 = wColorOrange; wButton2 = wColorButtonOrange; wButton2Down = wColorButtonDownOrange; }
					else if( nQuestionRandom2 == 3 ) { wColor2 = wColorYellow; wButton2 = wColorButtonYellow; wButton2Down = wColorButtonDownYellow; }
					else if( nQuestionRandom2 == 4 ) { wColor2 = wColorPurple; wButton2 = wColorButtonPurple; wButton2Down = wColorButtonDownPurple; }
					else if( nQuestionRandom2 == 5 ) { wColor2 = wColorGrey; wButton2 = wColorButtonGrey; wButton2Down = wColorButtonDownGrey; }
					else if( nQuestionRandom2 == 6 ) { wColor2 = wColorRed; wButton2 = wColorButtonRed; wButton2Down = wColorButtonDownRed; }

					html = Utils::ReplaceString(html, L"<?captcha_a2_color?>", wColor2, true);
					html = Utils::ReplaceString(html, L"<?captcha_a2_number?>", qWrongQuestion1, true);
					html = Utils::ReplaceString(html, L"<?captcha_a2_button?>", wButton2, true);
					html = Utils::ReplaceString(html, L"<?captcha_a2_button2?>", wButtonDown, true);

					//False Button2
					int nQuestionRandom3 = PickNumber2(6,nRandomColor,nQuestionRandom2);
					if( nQuestionRandom3 == 0 )	{ wColor3 = wColorBlue; wButton3 = wColorButtonBlue; wButton3Down = wColorButtonDownBlue; }
					else if( nQuestionRandom3 == 1 ) { wColor3 = wColorGreen; wButton3 = wColorButtonGreen; wButton3Down = wColorButtonDownGreen; }
					else if( nQuestionRandom3 == 2 ) { wColor3 = wColorOrange; wButton3 = wColorButtonOrange; wButton3Down = wColorButtonDownOrange; }
					else if( nQuestionRandom3 == 3 ) { wColor3 = wColorYellow; wButton3 = wColorButtonYellow; wButton3Down = wColorButtonDownYellow; }
					else if( nQuestionRandom3 == 4 ) { wColor3 = wColorPurple; wButton3 = wColorButtonPurple; wButton3Down = wColorButtonDownPurple; }
					else if( nQuestionRandom3 == 5 ) { wColor3 = wColorGrey; wButton3 = wColorButtonGrey; wButton3Down = wColorButtonDownGrey; }
					else if( nQuestionRandom3 == 6 ) { wColor3 = wColorRed; wButton3 = wColorButtonRed; wButton3Down = wColorButtonDownRed; }

					html = Utils::ReplaceString(html, L"<?captcha_a3_color?>", wColor3, true);
					html = Utils::ReplaceString(html, L"<?captcha_a3_number?>", qWrongQuestion2, true);
					html = Utils::ReplaceString(html, L"<?captcha_a3_button?>", wButton3, true);
					html = Utils::ReplaceString(html, L"<?captcha_a3_button2?>", wButtonDown, true);
				}
				else if( nRandomPos == 1 )
				{
					//False Button1
					int nQuestionRandom2 = PickNumber(6,nRandomColor);
					if( nQuestionRandom2 == 0 )	{ wColor2 = wColorBlue; wButton2 = wColorButtonBlue; wButton2Down = wColorButtonDownBlue; }
					else if( nQuestionRandom2 == 1 ) { wColor2 = wColorGreen; wButton2 = wColorButtonGreen; wButton2Down = wColorButtonDownGreen; }
					else if( nQuestionRandom2 == 2 ) { wColor2 = wColorOrange; wButton2 = wColorButtonOrange; wButton2Down = wColorButtonDownOrange; }
					else if( nQuestionRandom2 == 3 ) { wColor2 = wColorYellow; wButton2 = wColorButtonYellow; wButton2Down = wColorButtonDownYellow; }
					else if( nQuestionRandom2 == 4 ) { wColor2 = wColorPurple; wButton2 = wColorButtonPurple; wButton2Down = wColorButtonDownPurple; }
					else if( nQuestionRandom2 == 5 ) { wColor2 = wColorGrey; wButton2 = wColorButtonGrey; wButton2Down = wColorButtonDownGrey; }
					else if( nQuestionRandom2 == 6 ) { wColor2 = wColorRed; wButton2 = wColorButtonRed; wButton2Down = wColorButtonDownRed; }

					html = Utils::ReplaceString(html, L"<?captcha_a1_color?>", wColor2, true);
					html = Utils::ReplaceString(html, L"<?captcha_a1_number?>", qWrongQuestion1, true);
					html = Utils::ReplaceString(html, L"<?captcha_a1_button?>", wButton2, true);
					html = Utils::ReplaceString(html, L"<?captcha_a1_button2?>", wButtonDown, true);

					//Correct Button
					html = Utils::ReplaceString(html, L"<?captcha_a2_color?>", wColor, true);
					html = Utils::ReplaceString(html, L"<?captcha_a2_number?>", qCorrentQuestion, true);
					html = Utils::ReplaceString(html, L"<?captcha_a2_button?>", wButton, true);
					html = Utils::ReplaceString(html, L"<?captcha_a2_button2?>", wButtonDown, true);

					//False Button2
					int nQuestionRandom3 = PickNumber2(6,nRandomColor,nQuestionRandom2);
					if( nQuestionRandom3 == 0 )	{ wColor3 = wColorBlue; wButton3 = wColorButtonBlue; wButton3Down = wColorButtonDownBlue; }
					else if( nQuestionRandom3 == 1 ) { wColor3 = wColorGreen; wButton3 = wColorButtonGreen; wButton3Down = wColorButtonDownGreen; }
					else if( nQuestionRandom3 == 2 ) { wColor3 = wColorOrange; wButton3 = wColorButtonOrange; wButton3Down = wColorButtonDownOrange; }
					else if( nQuestionRandom3 == 3 ) { wColor3 = wColorYellow; wButton3 = wColorButtonYellow; wButton3Down = wColorButtonDownYellow; }
					else if( nQuestionRandom3 == 4 ) { wColor3 = wColorPurple; wButton3 = wColorButtonPurple; wButton3Down = wColorButtonDownPurple; }
					else if( nQuestionRandom3 == 5 ) { wColor3 = wColorGrey; wButton3 = wColorButtonGrey; wButton3Down = wColorButtonDownGrey; }
					else if( nQuestionRandom3 == 6 ) { wColor3 = wColorRed; wButton3 = wColorButtonRed; wButton3Down = wColorButtonDownRed; }

					html = Utils::ReplaceString(html, L"<?captcha_a3_color?>", wColor3, true);
					html = Utils::ReplaceString(html, L"<?captcha_a3_number?>", qWrongQuestion2, true);
					html = Utils::ReplaceString(html, L"<?captcha_a3_button?>", wButton3, true);
					html = Utils::ReplaceString(html, L"<?captcha_a3_button2?>", wButtonDown, true);
					
				}
				else if( nRandomPos == 2 )
				{
					//False Button1
					int nQuestionRandom2 = PickNumber(6,nRandomColor);
					if( nQuestionRandom2 == 0 )	{ wColor2 = wColorBlue; wButton2 = wColorButtonBlue; wButton2Down = wColorButtonDownBlue; }
					else if( nQuestionRandom2 == 1 ) { wColor2 = wColorGreen; wButton2 = wColorButtonGreen; wButton2Down = wColorButtonDownGreen; }
					else if( nQuestionRandom2 == 2 ) { wColor2 = wColorOrange; wButton2 = wColorButtonOrange; wButton2Down = wColorButtonDownOrange; }
					else if( nQuestionRandom2 == 3 ) { wColor2 = wColorYellow; wButton2 = wColorButtonYellow; wButton2Down = wColorButtonDownYellow; }
					else if( nQuestionRandom2 == 4 ) { wColor2 = wColorPurple; wButton2 = wColorButtonPurple; wButton2Down = wColorButtonDownPurple; }
					else if( nQuestionRandom2 == 5 ) { wColor2 = wColorGrey; wButton2 = wColorButtonGrey; wButton2Down = wColorButtonDownGrey; }
					else if( nQuestionRandom2 == 6 ) { wColor2 = wColorRed; wButton2 = wColorButtonRed; wButton2Down = wColorButtonDownRed; }

					html = Utils::ReplaceString(html, L"<?captcha_a1_color?>", wColor2, true);
					html = Utils::ReplaceString(html, L"<?captcha_a1_number?>", qWrongQuestion1, true);
					html = Utils::ReplaceString(html, L"<?captcha_a1_button?>", wButton2, true);
					html = Utils::ReplaceString(html, L"<?captcha_a1_button2?>", wButtonDown, true);

					//False Button2
					int nQuestionRandom3 = PickNumber2(6,nRandomColor,nQuestionRandom2);
					if( nQuestionRandom3 == 0 )	{ wColor3 = wColorBlue; wButton3 = wColorButtonBlue; wButton3Down = wColorButtonDownBlue; }
					else if( nQuestionRandom3 == 1 ) { wColor3 = wColorGreen; wButton3 = wColorButtonGreen; wButton3Down = wColorButtonDownGreen; }
					else if( nQuestionRandom3 == 2 ) { wColor3 = wColorOrange; wButton3 = wColorButtonOrange; wButton3Down = wColorButtonDownOrange; }
					else if( nQuestionRandom3 == 3 ) { wColor3 = wColorYellow; wButton3 = wColorButtonYellow; wButton3Down = wColorButtonDownYellow; }
					else if( nQuestionRandom3 == 4 ) { wColor3 = wColorPurple; wButton3 = wColorButtonPurple; wButton3Down = wColorButtonDownPurple; }
					else if( nQuestionRandom3 == 5 ) { wColor3 = wColorGrey; wButton3 = wColorButtonGrey; wButton3Down = wColorButtonDownGrey; }
					else if( nQuestionRandom3 == 6 ) { wColor3 = wColorRed; wButton3 = wColorButtonRed; wButton3Down = wColorButtonDownRed; }

					html = Utils::ReplaceString(html, L"<?captcha_a2_color?>", wColor3, true);
					html = Utils::ReplaceString(html, L"<?captcha_a2_number?>", qWrongQuestion2, true);
					html = Utils::ReplaceString(html, L"<?captcha_a2_button?>", wButton3, true);
					html = Utils::ReplaceString(html, L"<?captcha_a2_button2?>", wButtonDown, true);

					//Correct Button
					html = Utils::ReplaceString(html, L"<?captcha_a3_color?>", wColor, true);
					html = Utils::ReplaceString(html, L"<?captcha_a3_number?>", qCorrentQuestion, true);
					html = Utils::ReplaceString(html, L"<?captcha_a3_button?>", wButton, true);
					html = Utils::ReplaceString(html, L"<?captcha_a3_button2?>", wButtonDown, true);
				}
				pUser->ShowHTML(L"captcha_page2.htm", html.c_str(), 0);
			}
		}
		else if( nRandomMode == 2 ) //maths captcha
		{
			int Argument1 = g_MTRandom.RandInt(10);
			int Argument2 = g_MTRandom.RandInt(10);
			int CorrectAnswer = Argument1 + Argument2;
			//pUser->pED->captchaQuestion = CorrectAnswer;

			wstring qCorrentQuestion = Utils::RandomString(6);
			wstring qWrongQuestion1 = Utils::RandomString(6);
			wstring qWrongQuestion2 = Utils::RandomString(6);

			pUser->pED->captchaQuestion = qCorrentQuestion;
			pUser->pED->captchaWQuestion1 = qWrongQuestion1;
			pUser->pED->captchaWQuestion2 = qWrongQuestion2;

			//Show Captcha Page
			const WCHAR* wHTML = g_HtmlCache.Get(L"captcha_page3.htm");
			if(wHTML)
			{
				wstring html(wHTML);
				html = Utils::ReplaceString(html, L"<?captcha_arg1?>", Argument1, true);
				html = Utils::ReplaceString(html, L"<?captcha_arg2?>", Argument2, true);

				int nRandomPos = g_MTRandom.RandInt(2);

				int nAnswerFalse1 = PickNumber2(20, 0, CorrectAnswer);
				int nAnswerFalse2 = PickNumber3(20, 0, CorrectAnswer, nAnswerFalse1);

				if( nRandomPos == 0 )
				{
					html = Utils::ReplaceString(html, L"<?captcha_ans1?>", CorrectAnswer, true);
					html = Utils::ReplaceString(html, L"<?captcha_ans2?>", nAnswerFalse1, true);
					html = Utils::ReplaceString(html, L"<?captcha_ans3?>", nAnswerFalse2, true);

					html = Utils::ReplaceString(html, L"<?captcha_ans1h?>", qCorrentQuestion, true);
					html = Utils::ReplaceString(html, L"<?captcha_ans2h?>", qWrongQuestion1, true);
					html = Utils::ReplaceString(html, L"<?captcha_ans3h?>", qWrongQuestion2, true);
				}
				else if( nRandomPos == 1 )
				{
					html = Utils::ReplaceString(html, L"<?captcha_ans1?>", nAnswerFalse1, true);
					html = Utils::ReplaceString(html, L"<?captcha_ans2?>", CorrectAnswer, true);
					html = Utils::ReplaceString(html, L"<?captcha_ans3?>", nAnswerFalse2, true);

					html = Utils::ReplaceString(html, L"<?captcha_ans1h?>", qWrongQuestion1, true);
					html = Utils::ReplaceString(html, L"<?captcha_ans2h?>", qCorrentQuestion, true);
					html = Utils::ReplaceString(html, L"<?captcha_ans3h?>", qWrongQuestion2, true);
				}
				else if( nRandomPos == 2 )
				{
					html = Utils::ReplaceString(html, L"<?captcha_ans1?>", nAnswerFalse1, true);
					html = Utils::ReplaceString(html, L"<?captcha_ans2?>", nAnswerFalse2, true);
					html = Utils::ReplaceString(html, L"<?captcha_ans3?>", CorrectAnswer, true);

					html = Utils::ReplaceString(html, L"<?captcha_ans1h?>", qWrongQuestion1, true);
					html = Utils::ReplaceString(html, L"<?captcha_ans2h?>", qWrongQuestion2, true);
					html = Utils::ReplaceString(html, L"<?captcha_ans3h?>", qCorrentQuestion, true);
				}
				
				pUser->ShowHTML(L"captcha_page3.htm", html.c_str(), 0);
			}
		}
		else if( nRandomMode == 3 ) //site captcha (Re-Captcha)
		{
			pUser->pED->captchaQuestionR = 1;

			//Show Captcha Page
			const WCHAR* wHTML = g_HtmlCache.Get(L"captcha_page4.htm");
			if(wHTML)
			{

				UINT UniqueID = g_MTRandom.RandInt(999999);
				wstring PlayerName = pUser->pSD->wszName;

				pUser->pED->captchaUniqueID = UniqueID;

				//m_TemplateLink

				wstring html(wHTML);

				wstring link = Utils::ReplaceString(m_TemplateLink, L"<?PlayerName?>", PlayerName, true); 
				wstring link2 = Utils::ReplaceString(link, L"<?UniqueID?>", UniqueID, true);

				html = Utils::ReplaceString(html, L"<?url_to_solve?>", link2, true);
				
				pUser->ShowHTML(L"captcha_page4.htm", html.c_str(), 0);
			}
		}
		/*else if( nRandomMode == 4 ) //captcha file
		{
			pUser->pED->captchaQuestion = 1;

			//Show Captcha Page
			const WCHAR* wHTML = g_HtmlCache.Get(L"captcha_page5.htm");
			if(wHTML)
			{

				UINT UniqueID = g_MTRandom.RandInt(999999);
				wstring PlayerName = pUser->pSD->wszName;

				pUser->pED->captchaUniqueID = UniqueID;

				wstring html(wHTML);

				wstring link = Utils::ReplaceString(m_TemplateLink, L"<?PlayerName?>", PlayerName, true); 
				wstring link2 = Utils::ReplaceString(link, L"<?UniqueID?>", UniqueID, true);

				html = Utils::ReplaceString(html, L"<?url_to_solve?>", link2, true);
				
				pUser->ShowHTML(L"captcha_page5.htm", html.c_str(), 0);
			}
		}*/
		g_DB.RequestSaveCaptchaStatus(pUser->nDBID, pUser->pED->mobsKilled, pUser->pED->captchaWrongTimes, pUser->pED->captchaPeriod, pUser->pED->captchaQuestionR, pUser->pED->captchaUniqueID, pUser->pED->captchaJail);
	}
}

void CCaptcha::Validate(User *pUser, wstring answer, bool recaptcha)
{
	guard;
	if(pUser->ValidUser())
	{
		wstring correctanswer = pUser->pED->captchaQuestion;


//		g_Log.Add(CLog::Error, "[%s] 00000000 [%S] [%S]", __FUNCTION__, answer.c_str(), correctanswer.c_str());


		if( !recaptcha )
		{
			if( answer == correctanswer)
			{
				ReleasePlayer(pUser);
			}
			else
			{
				DoPunishment(pUser,false);
			}
		}
		else
		{
			//Recaptcha Here

			//Ask DB to check if the answer is 0
			g_DB.RequestCaptchaStatusSite(pUser->pSD->nDBID);
		}
		//g_Log.Add(CLog::Blue, "[%s] Captcha Save, Mobs Killed[%d] Captcha Wrong Times[%d] Captcha Mode[%d]", __FUNCTION__, pUser->pED->mobsKilled, pUser->pED->captchaWrongTimes, pUser->pED->captchaPeriod);
	}
	unguard;
}

void CCaptcha::ReleasePlayer(User *pUser)
{
	if(pUser->ValidUser())
	{
//		if( g_Captcha.GetSkillId() > 0 )
//		{
			pUser->pED->captchaPeriod = 0;
			pUser->pED->captchaWrongTimes = 0;
			pUser->pED->captchaProtectionTimeout = 0;
			pUser->pED->captchaProtectionTimeSent = 0;
			//pUser->DeleteSkill(g_Captcha.GetSkillId());
	//		pUser->ValidateParameters();
			//2631	1	Captcha System: Correct Answer! Have a nice day!	0	79	9B	B0	FF			0	0	0	0	0		none
//			pUser->SendSystemMessage(2631);

			pUser->SendSystemMessage(L"Captcha System: Correct Answer! Have a nice day!");

		//	pUser->SendETCBuffStatus();
//		}

		//Give Reward
		if(m_RewardActive)
		{
//			pUser->AddItemToInventory(m_RewardID, m_RewardQuanity, false, 0);
		}
		
		g_DB.RequestSaveCaptchaStatus(pUser->nDBID, pUser->pED->mobsKilled, pUser->pED->captchaWrongTimes, pUser->pED->captchaPeriod, 0, 0, pUser->pED->captchaJail);
	}
}

int CCaptcha::PickNumber(int max, int unwanted)
{
	int nRandomNumber = g_MTRandom.RandInt(max);
	if ( nRandomNumber == unwanted )
	{
		return PickNumber(max,unwanted);
	}
	return nRandomNumber;
}

int CCaptcha::PickNumber2(int max, int unwanted1, int unwanted2)
{
	int nRandomNumber = g_MTRandom.RandInt(max);
	if ( (nRandomNumber == unwanted1) || (nRandomNumber == unwanted2) )
	{
		return PickNumber2(max,unwanted1,unwanted2);
	}
	return nRandomNumber;
}

int CCaptcha::PickNumber3(int max, int unwanted1, int unwanted2, int unwanted3)
{
	int nRandomNumber = g_MTRandom.RandInt(max);
	if ( (nRandomNumber == unwanted1) || (nRandomNumber == unwanted2) || (nRandomNumber == unwanted3) )
	{
		return PickNumber3(max,unwanted1,unwanted2,unwanted3);
	}
	return nRandomNumber;
}

void CCaptcha::DoReport(User *pUser, User *pTarget)
{
	if(pUser->ValidUser())
	{
		if(pTarget->ValidUser())
		{
			pTarget->pED->captchareporttimes++;
			//g_DB.RequestSaveJailStatus(pTarget->nDBID, pTarget->pED->captchareporttimes);
		}
	}
}


wstring CCaptcha::PickColor(int numberPick)
{
	wstring color;

	wstring wblue1 = L"Azul / Blue";
	wstring wblue2 = L"Azul / Blue";
	wstring wblue3 = L"Azul / Blue";
	wstring wblue4 = L"Azul / Blue";
	wstring wblue5 = L"Azul / Blue";
	wstring wblue6 = L"Azul / Blue";
	wstring wblue7 = L"Azul / Blue";
	wstring wblue8 = L"Azul / Blue";
	//wstring wblue9 = L"<font color=00FFFF>COLOUR</font>";
	//wstring wblue10 = L"<font color=0088FF>COLOUR</font>";
	//wstring wblue11 = L"<font color=0000FF>COLOUR</font>";

	wstring wColorGreen1 = L"Verde / Green";
	wstring wColorGreen2 = L"Verde / Green";
	wstring wColorGreen3 = L"Verde / Green";
	wstring wColorGreen4 = L"Verde / Green";
	wstring wColorGreen5 = L"Verde / Green";
	wstring wColorGreen6 = L"Verde / Green";
	wstring wColorGreen7 = L"Verde / Green";
	wstring wColorGreen8 = L"Verde / Green";
	wstring wColorGreen9 = L"Verde / Green";
	wstring wColorGreen10 = L"Verde / Green";
	wstring wColorGreen11 = L"Verde / Green";
	wstring wColorGreen12 = L"Verde / Green";
	wstring wColorGreen13 = L"Verde / Green";
	wstring wColorGreen14 = L"Verde / Green";
	wstring wColorGreen15 = L"Verde / Green";

	wstring wColorOrange1 = L"Naranja / Orange";
	wstring wColorOrange2 = L"Naranja / Orange";
	wstring wColorOrange3 = L"Naranja / Orange";
	wstring wColorOrange4 = L"Naranja / Orange";
	wstring wColorOrange5 = L"Naranja / Orange";
	wstring wColorOrange6 = L"Naranja / Orange";
	wstring wColorOrange7 = L"Naranja / Orange";
	wstring wColorOrange8 = L"Naranja / Orange";
	wstring wColorOrange9 = L"Naranja / Orange";
	wstring wColorOrange10 = L"Naranja / Orange";
	wstring wColorOrange11 = L"Naranja / Orange";
	wstring wColorOrange12 = L"Naranja / Orange";
	wstring wColorOrange13 = L"Naranja / Orange";
	wstring wColorOrange14 = L"Naranja / Orange";
	wstring wColorOrange15 = L"Naranja / Orange";
	wstring wColorOrange16 = L"Naranja / Orange";
	wstring wColorOrange17 = L"Naranja / Orange";
	wstring wColorOrange18 = L"Naranja / Orange";
	wstring wColorOrange19 = L"Naranja / Orange";
	wstring wColorOrange20 = L"Naranja / Orange";
	wstring wColorOrange21 = L"Naranja / Orange";
	wstring wColorOrange22 = L"Naranja / Orange";

	wstring wColorYellow1 = L"Amarillo / Yellow";
	wstring wColorYellow2 = L"Amarillo / Yellow";
	wstring wColorYellow3 = L"Amarillo / Yellow";
	wstring wColorYellow4 = L"Amarillo / Yellow";
	wstring wColorYellow5 = L"Amarillo / Yellow";
	wstring wColorYellow6 = L"Amarillo / Yellow";
	wstring wColorYellow7 = L"Amarillo / Yellow";
	wstring wColorYellow8 = L"Amarillo / Yellow";
	wstring wColorYellow9 = L"Amarillo / Yellow";
	wstring wColorYellow10 = L"Amarillo / Yellow";
	wstring wColorYellow11 = L"Amarillo / Yellow";
	wstring wColorYellow12 = L"Amarillo / Yellow";

	wstring wColorPurple1 = L"Purpura / Purple";
	wstring wColorPurple2 = L"Purpura / Purple";
	wstring wColorPurple3 = L"Purpura / Purple";
	wstring wColorPurple4 = L"Purpura / Purple";
	wstring wColorPurple5 = L"Purpura / Purple";
	wstring wColorPurple6 = L"Purpura / Purple";
	wstring wColorPurple7 = L"Purpura / Purple";

	wstring wColorGrey1 = L"Gris / Grey";
	wstring wColorGrey2 = L"Gris / Grey";
	wstring wColorGrey3 = L"Gris / Grey";
	wstring wColorGrey4 = L"Gris / Grey";
	wstring wColorGrey5 = L"Gris / Grey";
	wstring wColorGrey6 = L"Gris / Grey";
	wstring wColorGrey7 = L"Gris / Grey";
	wstring wColorGrey8 = L"Gris / Grey";
	wstring wColorGrey9 = L"Gris / Grey";
	wstring wColorGrey10 = L"Gris / Grey";
	wstring wColorGrey11 = L"Gris / Grey";
	wstring wColorGrey12 = L"Gris / Grey";

	wstring wColorRed1 = L"Rojo / Red";
	wstring wColorRed2 = L"Rojo / Red";
	wstring wColorRed3 = L"Rojo / Red";
	wstring wColorRed4 = L"Rojo / Red";
	wstring wColorRed5 = L"Rojo / Red";
	wstring wColorRed6 = L"Rojo / Red";
	wstring wColorRed7 = L"Rojo / Red";


	if(numberPick == 1) //Blue
	{
		int nRand = rand() % 8 + 1;
		if( nRand == 1) { color = wblue1; } else if( nRand == 2) { color = wblue2; } else if( nRand == 3) { color = wblue3; } else if( nRand == 4) { color = wblue4; } else if( nRand == 5) { color = wblue5; } else if( nRand == 6) { color = wblue6; } else if( nRand == 7) { color = wblue7; } else if( nRand == 8) { color = wblue8; }

	}
	else if(numberPick == 2) //Green
	{
		int nRand = rand() % 15 + 1;
		if( nRand == 1) { color = wColorGreen1; } else if( nRand == 2) { color = wColorGreen2; } else if( nRand == 3) { color = wColorGreen3; } else if( nRand == 4) { color = wColorGreen4; } else if( nRand == 5) { color = wColorGreen5; } else if( nRand == 6) { color = wColorGreen6; } else if( nRand == 7) { color = wColorGreen7; } else if( nRand == 8) { color = wColorGreen8; } else if( nRand == 9) { color = wColorGreen9; } else if( nRand == 10) { color = wColorGreen10; } else if( nRand == 11) { color = wColorGreen11; } else if( nRand == 12) { color = wColorGreen12; } else if( nRand == 13) { color = wColorGreen13; } else if( nRand == 14) { color = wColorGreen14; } else if( nRand == 15) { color = wColorGreen15; }	
	}
	else if(numberPick == 3) //Orange
	{
		int nRand = rand() % 22 + 1;
		if( nRand == 1) { color = wColorOrange1; } else if( nRand == 2) { color = wColorOrange2; } else if( nRand == 3) { color = wColorOrange3; } else if( nRand == 4) { color = wColorOrange4; } else if( nRand == 5) { color = wColorOrange5; } else if( nRand == 6) { color = wColorOrange6; } else if( nRand == 7) { color = wColorOrange7; } else if( nRand == 8) { color = wColorOrange8; } else if( nRand == 9) { color = wColorOrange9; } else if( nRand == 10) { color = wColorOrange10; } else if( nRand == 11) { color = wColorOrange11; } else if( nRand == 12) { color = wColorOrange12; } else if( nRand == 13) { color = wColorOrange13; } else if( nRand == 14) { color = wColorOrange14; } else if( nRand == 15) { color = wColorOrange15; } else if( nRand == 16) { color = wColorOrange16; } else if( nRand == 17) { color = wColorOrange17; } else if( nRand == 18) { color = wColorOrange18; } else if( nRand == 19) { color = wColorOrange19; } else if( nRand == 20) { color = wColorOrange20; } else if( nRand == 21) { color = wColorOrange21; } else if( nRand == 22) { color = wColorOrange22; }
	}
	else if(numberPick == 4) //Yellow
	{
		int nRand = rand() % 12 + 1;
		if( nRand == 1) { color = wColorYellow1; } else if( nRand == 2) { color = wColorYellow2; } else if( nRand == 3) { color = wColorYellow3; } else if( nRand == 4) { color = wColorYellow4; } else if( nRand == 5) { color = wColorYellow5; } else if( nRand == 6) { color = wColorYellow6; } else if( nRand == 7) { color = wColorYellow7; } else if( nRand == 8) { color = wColorYellow8; } else if( nRand == 9) { color = wColorYellow9; } else if( nRand == 10) { color = wColorYellow10; } else if( nRand == 11) { color = wColorYellow11; } else if( nRand == 12) { color = wColorYellow12; }
	}
	else if(numberPick == 5) //Purple
	{
		int nRand = rand() % 7 + 1;
		if( nRand == 1) { color = wColorPurple1; } else if( nRand == 2) { color = wColorPurple2; } else if( nRand == 3) { color = wColorPurple3; } else if( nRand == 4) { color = wColorPurple4; } else if( nRand == 5) { color = wColorPurple5; } else if( nRand == 6) { color = wColorPurple6; } else if( nRand == 7) { color = wColorPurple7; }
	}
	else if(numberPick == 6) //Grey
	{
		int nRand = rand() % 12 + 1;
		if( nRand == 1) { color = wColorGrey1; } else if( nRand == 2) { color = wColorGrey2; } else if( nRand == 3) { color = wColorGrey3; } else if( nRand == 4) { color = wColorGrey4; } else if( nRand == 5) { color = wColorGrey5; } else if( nRand == 6) { color = wColorGrey6; } else if( nRand == 7) { color = wColorGrey7; } else if( nRand == 8) { color = wColorGrey8; } else if( nRand == 9) { color = wColorGrey9; } else if( nRand == 10) { color = wColorGrey10; } else if( nRand == 11) { color = wColorGrey11; } else if( nRand == 12) { color = wColorGrey12; }
	}
	else if(numberPick == 7) //Red
	{
		int nRand = rand() % 7 + 1;
		if( nRand == 1) { color = wColorRed1; } else if( nRand == 2) { color = wColorRed2; } else if( nRand == 3) { color = wColorRed3; } else if( nRand == 4) { color = wColorRed4; } else if( nRand == 5) { color = wColorRed5; } else if( nRand == 6) { color = wColorRed6; } else if( nRand == 7) { color = wColorRed7; }
	}

	return color;
}


			//wstring wColorBlue = L"Blue";
			//wstring wColorGreen = L"Green";
			//wstring wColorOrange = L"Orange";
			//wstring wColorYellow = L"Yellow";
			//wstring wColorPurple = L"Purple";
			//wstring wColorGrey = L"Grey";
			//wstring wColorRed = L"Red";