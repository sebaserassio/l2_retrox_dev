#include "StdAfx.h"
#include "CursedWeaponSystem.h"
#include "Packets.h"
#include "AIO.h"
#include "TerritoryData.h"
#include "VisualArmor.h"
#include "PlayerCustomizer.h"
#include "SubStack.h"
#include "Utils.h"
#include "OfflineBuffer.h"
#include "AccountDB.h"
#include "Hair2Slot.h"

extern UINT g_ClanLeaderTitleColor;
extern UINT g_ClanLeaderNicknameColor;
extern UINT g_GMTitleColor;
extern UINT g_GMNicknameColor;
extern UINT g_DefaultTitleColor;

extern INT32 g_MinEnchantLevelAuraHero;

extern User* g_CharInfoUser[16];

// Helpers para leer varargs sin desalinear (VC++ 2005 friendly)
static __forceinline BYTE  ARG_C(va_list& ap){ return (BYTE)va_arg(ap, int); }
static __forceinline WORD  ARG_H(va_list& ap){ return (WORD)va_arg(ap, int); }
static __forceinline DWORD ARG_D(va_list& ap){ return (DWORD)va_arg(ap, DWORD); }
static __forceinline double ARG_F(va_list& ap){ return va_arg(ap, double); }
static __forceinline PWCHAR ARG_S(va_list& ap){ return va_arg(ap, PWCHAR); }
static __forceinline INT32 ARG_D32(va_list& ap){ return (INT32)va_arg(ap, INT32); } // por simetría con tus INT32 locales

void CPacketFix::CharInfoFix(CUserSocket *pSocket, const char *format, ...)
{
	int race = 0, clase = 0, sex = 0;
	double ColRadius = 5, ColHeight = 30;

	User *pUser = 0;
	UINT threadIndex = GetThreadIndex();
	if(threadIndex < 16){
		pUser = g_CharInfoUser[threadIndex];
	}

	CPacket Packet;
	va_list tag;
	va_start(tag, format);

	/* c */ Packet.WriteC(ARG_C(tag));
	/* d */ Packet.WriteD(ARG_D(tag)); // X
	/* d */ Packet.WriteD(ARG_D(tag)); // Y
	/* d */ Packet.WriteD(ARG_D(tag)); // Z
	DWORD heading = ARG_D(tag);
	/* d */ Packet.WriteD(heading);    // Heading
	/* d */ UINT nObjectID = ARG_D(tag); // ObjectID

	if(pUser == 0){
		pUser = User::GetUserBySID(&nObjectID);
		if(!pUser->ValidUser()){
			va_end(tag);
			return;
		}
	}

	TerritoryData& td = pUser->pSD->pExData->territoryData;

	Packet.WriteD(nObjectID);

	PWCHAR wName = ARG_S(tag);
	CNickNameChangeTerritory *pTerritory = NULL;
	if(int TerritoryID = pUser->pSD->nInNickNameChangeZone){
		pTerritory = g_TerritoryChecker.GetTerritory(TerritoryID)->SafeCastNickNameChange();
		if(pTerritory){
			PWCHAR wNewName = (PWCHAR)pTerritory->GetNickName();
			if(wNewName){ wName = wNewName; }
		}
	}
	/* S */ Packet.WriteS(wName); // nickname

	DWORD Race = ARG_D(tag);
	if(pUser->pED->fakeRace != UINT_MAX){ Race = pUser->pED->fakeRace; }

	DWORD Sex  = ARG_D(tag);
	if(pUser->pED->fakeSex  != UINT_MAX){ Sex  = pUser->pED->fakeSex; }

	DWORD BaseClass = ARG_D(tag);

	if(pUser->pED->actualizarApariencias)
	{
		int custom_sex = g_AccountDB.GetCustomSex(pUser->pSD->nDBID);
		if (custom_sex > -1) Sex = custom_sex;

		int customSkin = g_AccountDB.GetCustomSkin(pUser->pSD->nDBID);
		if (customSkin > -1) BaseClass = customSkin;

		if ( (BaseClass >= 0 && BaseClass <= 9) || (BaseClass >= 88 && BaseClass <= 93) )        Race=0;
		else if ( (BaseClass >= 10 && BaseClass <= 17) || (BaseClass >= 94 && BaseClass <= 98) ) Race=0;
		else if ( (BaseClass >= 18 && BaseClass <= 30) || (BaseClass >= 99 && BaseClass <= 105)) Race=1;
		else if ( (BaseClass >= 31 && BaseClass <= 43) || (BaseClass >= 106 && BaseClass <= 112))Race=2;
		else if ( (BaseClass >= 44 && BaseClass <= 52) || (BaseClass >= 113 && BaseClass <= 116))Race=3;
		else if ( (BaseClass >= 53 && BaseClass <= 57) || (BaseClass >= 117 && BaseClass <= 118))Race=4;

		DWORD Magic = 0;
		if(CCategoryDataDB::IsInCategory(BaseClass, CCategoryDataDB::MAGE_GROUP)) Magic = 1;

		ColRadius = g_World.GetRadius(Race, Sex, Magic);
		ColHeight = g_World.GetHeight(Race, Sex, Magic);

		pUser->pED->actualizarApariencias = false;
		pUser->pED->custom_sex = Sex;
		pUser->pED->customSkin = BaseClass;
		pUser->pED->Race = Race;
		pUser->pED->ColRadius = ColRadius;
		pUser->pED->ColHeight = ColHeight;
	}
	else
	{
		Sex      = pUser->pED->custom_sex;
		BaseClass= pUser->pED->customSkin;
		Race     = pUser->pED->Race;
		ColRadius= pUser->pED->ColRadius;
		ColHeight= pUser->pED->ColHeight;
	}

	/* d */ Packet.WriteD(Race); // race
	/* d */ Packet.WriteD(Sex);  // sex

	if(g_SubStack.IsEnabled())
	{
		UINT charIndex = UINT_MAX;
		for(UINT n=0;n<7;n++){
			if(pUser->pSocket->characterDBID[n] == pUser->nDBID){ charIndex = n; break; }
		}
		if(charIndex != UINT_MAX && charIndex < 7){
			if(pUser->pSocket->pED->baseClass[charIndex] != UINT_MAX){
				BaseClass = pUser->pSocket->pED->baseClass[charIndex];
			}
		}
	}

	/* d */ Packet.WriteD(BaseClass);

	INT32 dhairId  = ARG_D32(tag);
	INT32 headId   = ARG_D32(tag);
	INT32 rhandId  = ARG_D32(tag);
	INT32 lhandId  = ARG_D32(tag);
	INT32 glovesId = ARG_D32(tag);
	INT32 chestId  = ARG_D32(tag);
	INT32 legsId   = ARG_D32(tag);
	INT32 feetId   = ARG_D32(tag);
	INT32 backId   = ARG_D32(tag);
	INT32 lrhandId = ARG_D32(tag);
	INT32 hairId   = ARG_D32(tag);
	INT32 faceId   = 0; // interlude addon - 0

	Hair2Slot::GetItemIDCharInfo(pUser, dhairId, hairId, faceId);

	if(!pUser->pED->inZoneNoVisualEffect)
	{
		if(g_VisualArmor.IsEnabled()){
			VisualArmorUser& vUser = pUser->pED->visualArmorUser;
			if(!pUser->olympiadUser.IsInside()){
				if(vUser.armorId > 0){
					if(vUser.armorType == 0 || vUser.armorType == pUser->pSD->nArmorType){
						if(vUser.chestSlotId  > 0) chestId  = vUser.chestSlotId;
						if(vUser.legsSlotId   > 0) legsId   = vUser.legsSlotId;
						if(vUser.glovesSlotId > 0) glovesId = vUser.glovesSlotId;
						if(vUser.feetSlotId   > 0) feetId   = vUser.feetSlotId;
						if(vUser.hairSlotId   > 0 && hairId == 0) hairId = vUser.hairSlotId;
					}
				}
			}
		}
	}

	if(pUser->pED->tvtUser.status == TvT::UserFighting || pUser->pED->tvtUser.status == TvT::UserPreparing){
		if(pUser->pED->tvtUser.flag){
			rhandId = 6718;
			lrhandId = 6718;
		}
	}
	if(pUser->pED->BlockAppeZoneId){
		dhairId = headId = rhandId = lhandId = glovesId = chestId = legsId = feetId = backId = lrhandId = hairId = 0;
		faceId = 0;
	}
	if(pUser->pED->m_CtfUserFlag>0){
		lrhandId = pUser->pED->m_CtfUserFlag;
		rhandId = 0; lhandId = 0;
	}

	/* d */ Packet.WriteD(dhairId);
	/* d */ Packet.WriteD(headId);
	/* d */ Packet.WriteD(rhandId);
	/* d */ Packet.WriteD(lhandId);
	/* d */ Packet.WriteD(glovesId);
	/* d */ Packet.WriteD(chestId);
	/* d */ Packet.WriteD(legsId);
	/* d */ Packet.WriteD(feetId);
	/* d */ Packet.WriteD(backId);
	/* d */ Packet.WriteD(lrhandId);
	/* d */ Packet.WriteD(hairId);
	/* d */ Packet.WriteD(faceId);

	int nAugmentation = pUser->GetAugmentationID();
	/* h */ Packet.WriteH(0);
	/* h */ Packet.WriteH(0);
	/* h */ Packet.WriteH(0);
	/* h */ Packet.WriteH(0);
	/* d */ Packet.WriteD(nAugmentation); // Augmentation id
	/* h */ Packet.WriteH(0);
	/* h */ Packet.WriteH(0);
	/* h */ Packet.WriteH(0);
	/* h */ Packet.WriteH(0);
	/* h */ Packet.WriteH(0);
	/* h */ Packet.WriteH(0);
	/* h */ Packet.WriteH(0);
	/* h */ Packet.WriteH(0);
	/* h */ Packet.WriteH(0);
	/* h */ Packet.WriteH(0);
	/* h */ Packet.WriteH(0);
	/* h */ Packet.WriteH(0);
	/* d */ Packet.WriteD(nAugmentation); // Augmentation ID
	/* h */ Packet.WriteH(0);
	/* h */ Packet.WriteH(0);
	/* h */ Packet.WriteH(0);
	/* h */ Packet.WriteH(0);

	/* d */ Packet.WriteD(ARG_D(tag));
	/* d */ Packet.WriteD(ARG_D(tag));
	/* d */ Packet.WriteD(ARG_D(tag));
	/* d */ Packet.WriteD(ARG_D(tag));
	/* d */ Packet.WriteD(ARG_D(tag));
	/* d */ Packet.WriteD(ARG_D(tag));
	/* d */ Packet.WriteD(ARG_D(tag));
	/* d */ Packet.WriteD(ARG_D(tag));
	/* d */ Packet.WriteD(ARG_D(tag));
	/* d */ Packet.WriteD(ARG_D(tag));
	/* d */ Packet.WriteD(ARG_D(tag));
	/* d */ Packet.WriteD(ARG_D(tag));
	/* d */ Packet.WriteD(ARG_D(tag));
	/* d */ Packet.WriteD(ARG_D(tag));

	/* f */ Packet.WriteF(ARG_F(tag));
	/* f */ Packet.WriteF(ARG_F(tag));

	double radius = ARG_F(tag);
	double height = ARG_F(tag);

	if(pUser->pED->fakeRace != UINT_MAX || pUser->pED->fakeSex != UINT_MAX){
		DWORD Magic = 0;
		if(CCategoryDataDB::IsInCategory(BaseClass, CCategoryDataDB::MAGE_GROUP)) Magic = 1;
		radius = g_World.GetRadius(Race, Sex, Magic);
		height = g_World.GetHeight(Race, Sex, Magic);
	}else{
		radius = ColRadius;
		height = ColHeight;
	}

	/* f */ Packet.WriteF(radius); // collision_radius
	/* f */ Packet.WriteF(height); // collision_height

	/* d */ Packet.WriteD(ARG_D(tag)); // hair_style
	/* d */ Packet.WriteD(ARG_D(tag)); // hair_color
	/* d */ Packet.WriteD(ARG_D(tag)); // face

	if(pTerritory && pTerritory->IsHidePledge())
	{
		PWCHAR wUserTitleOriginal = ARG_S(tag); // title (consumimos)
		if(td.territoryId[SHOW_HP_TITLE] != 0){
			int nCurrent_hp = (int)pUser->pSD->fHP;
			int nCurrent_cp = (int)pUser->pSD->fCP;
			int total_life = nCurrent_hp + nCurrent_cp;
			wstring wUserTitleHP = L"CP/HP: [" + Utils::IntToWString(total_life) + L"]";
			if(pUser->pED->tvtUser.status == TvT::UserFighting || pUser->pED->tvtUser.status == TvT::UserPreparing){
				if(pUser->pED->tvtUser.flag) wUserTitleHP = L"[FLAG]";
			}
			const WCHAR* wUserTitle = wUserTitleHP.c_str();
			/* S */ Packet.WriteS(wUserTitle);
		}else{
			if(pUser->pED->inZonePvPRank){
				wstring wUserTitlePvP = L"Puntos: [" + Utils::IntToWString(pUser->pED->puntosPvP) + L"]";
				const WCHAR* wUserTitle = wUserTitlePvP.c_str();
				/* S */ Packet.WriteS(wUserTitle);
			}else{
				/* S */ Packet.WriteS(L"");
			}
		}

		(void)ARG_D(tag); /* pledge_id (descartado) */
		/* d */ Packet.WriteD(0);
		(void)ARG_D(tag); /* pledge_crest_id (descartado) */
		/* d */ Packet.WriteD(0);
		(void)ARG_D(tag); /* ally_id (descartado) */
		/* d */ Packet.WriteD(0);
		(void)ARG_D(tag); /* ally_crest_id (descartado) */
		/* d */ Packet.WriteD(0);
	}
	else
	{
		PWCHAR wUserTitleOriginal = ARG_S(tag);
		if(td.territoryId[SHOW_HP_TITLE] != 0){
			int nCurrent_hp = (int)pUser->pSD->fHP;
			int nCurrent_cp = (int)pUser->pSD->fCP;
			int total_life = nCurrent_hp + nCurrent_cp;
			wstring wUserTitleHP = L"CP/HP: [" + Utils::IntToWString(total_life) + L"]";
			if(pUser->pED->tvtUser.status == TvT::UserFighting || pUser->pED->tvtUser.status == TvT::UserPreparing){
				if(pUser->pED->tvtUser.flag) wUserTitleHP = L"[FLAG]";
			}
			const WCHAR* wUserTitle = wUserTitleHP.c_str();
			/* S */ Packet.WriteS(wUserTitle);
		}
		else if( (pUser->pSD->activateOfflineShopTime > 0) && (pUser->pED->nofflinesellbuffprice > 0) )
		{
			const WCHAR * wUserTitle = pUser->pED->offlineselltitle.c_str();
			Packet.WriteS(wUserTitle);
		}
		else
		{
			if(pUser->pED->tvtUser.status == TvT::UserFighting || pUser->pED->tvtUser.status == TvT::UserPreparing){
				if(pUser->pED->tvtUser.flag){
					// Packet.WriteS(L"[FLAG]"); // si quisieras forzar
					Packet.WriteS(wUserTitleOriginal);
				}else{
					Packet.WriteS(wUserTitleOriginal);
				}
			}else{
				if(pUser->pED->inZonePvPRank){
					wstring wUserTitlePvP = L"Puntos: [" + Utils::IntToWString(pUser->pED->puntosPvP) + L"]";
					const WCHAR* wUserTitle = wUserTitlePvP.c_str();
					Packet.WriteS(wUserTitle);
				}else{
					Packet.WriteS(wUserTitleOriginal);
				}
			}
		}

		/* d */ Packet.WriteD(ARG_D(tag)); // pledge_id
		/* d */ Packet.WriteD(ARG_D(tag)); // pledge_crest_id
		/* d */ Packet.WriteD(ARG_D(tag)); // ally_id
		/* d */ Packet.WriteD(ARG_D(tag)); // ally_crest_id
	}

	DWORD _relation = ARG_D(tag);
	/* d */ Packet.WriteD(_relation);
	/* c */ Packet.WriteC(ARG_C(tag)); // is sit
	/* c */ Packet.WriteC(ARG_C(tag)); // is run
	/* c */ Packet.WriteC(ARG_C(tag)); // is combat
	/* c */ Packet.WriteC(ARG_C(tag)); // is alike dead
	/* c */ Packet.WriteC(ARG_C(tag)); // hide
	/* c */ Packet.WriteC(ARG_C(tag)); // mount type

	BYTE isprivatestore = ARG_C(tag);
	if(g_OfflineBuffer.IsOfflineBuffer(pUser)) isprivatestore = 0x01;
	/* c */ Packet.WriteC(isprivatestore);

	// --- Cubics: cliente espera H(count) + count*H ---
	WORD cubicCount = (WORD)va_arg(tag, int); // ¡PROMOCIÓN!
	DWORD dBuffSize = ARG_D(tag);
	PUCHAR pCubics  = (PUCHAR)va_arg(tag, PUCHAR);

	// Si la zona oculta efectos visuales, no enviar cubics
	if (pUser->pED->inZoneNoVisualEffect)
	{
		// Importante: ya consumimos los va_args; solo anunciamos 0 al cliente
		Packet.WriteH(0);
	}
	else
	{
		// Cota de seguridad por si llega basura
		if (cubicCount > 3) cubicCount = 3;

		Packet.WriteH(cubicCount);

		// Si nos mandan un blob (estilo viejo), reinterpretamos como array de WORDs.
		const WORD* wList   = (const WORD*)pCubics;
		UINT availWords     = (pCubics && dBuffSize >= 2) ? (dBuffSize / 2) : 0;
		UINT emit           = (cubicCount <= availWords) ? cubicCount : availWords;

		for (UINT i = 0; i < emit; ++i)
			Packet.WriteH(wList[i]);

		for (UINT i = emit; i < cubicCount; ++i)
			Packet.WriteH(0);
	}
	// --- fin cubics ---

	if(pTerritory && pTerritory->IsHidePledge()){
		(void)ARG_C(tag); /* consumimos el flag original */
		/* c */ Packet.WriteC(0); // isInPartyMatchRoom
	}else{
		/* c */ Packet.WriteC(ARG_C(tag)); // isInPartyMatchRoom
	}

	/* d */ DWORD AbnormalVisualEffect = ARG_D(tag);
	if(pUser->pSD->pExData->SkillMod.semiInvisible){ AbnormalVisualEffect |= 0x100000; }
	if(pUser->pED->spawnProtection == 2){ AbnormalVisualEffect |= 0x200000; }
	Packet.WriteD(AbnormalVisualEffect);

	/* c */ Packet.WriteC(ARG_C(tag));
	/* h */ Packet.WriteH(ARG_H(tag));
	/* d */ Packet.WriteD(ARG_D(tag));
	/* d */ Packet.WriteD(ARG_D(tag));
	/* d */ Packet.WriteD(ARG_D(tag));
	/* c */ Packet.WriteC(ARG_C(tag));

	/* c */ BYTE bTeam = ARG_C(tag);
	if(pUser->pSD->DuelInfo.DuelID && pUser->pSD->DuelInfo.Fighting){
		bTeam = pUser->pSD->DuelInfo.DuelTeam;
	}
	if(pUser->pED->tvtUser.status == TvT::UserFighting || pUser->pED->tvtUser.status == TvT::UserPreparing){
		bTeam = pUser->pED->tvtUser.team;
	}
	else if(pTerritory && pTerritory->GetTeam()){
		bTeam = pTerritory->GetTeam();
	}
	if(pUser->pED->inEventPvp > 0){ bTeam = pUser->pED->inEventPvp; }

	Packet.WriteC(bTeam);
	/* d */ Packet.WriteD(ARG_D(tag));
	/* c */ Packet.WriteC(ARG_C(tag));

	BYTE is_hero = ARG_C(tag);
	bool armaHeroAura = false;

	if (g_MinEnchantLevelAuraHero > 0){
		CSharedItemData *pData = pUser->GetEquipedWeapon();
		if(pData){
			CItem *pItem = pUser->inventory.GetItemBySID(pData->nObjectID);
			if(pItem->IsValid(VT_ITEMWEAPON)){
				if (pItem->pII->nCrystalType == 5 && pItem->pSID->nEnchantLevel >= g_MinEnchantLevelAuraHero){
					is_hero = 1;
					armaHeroAura = true;
				}
			}
		}
	}

	if(!pUser->pED->inZoneNoVisualEffect){
		if(pUser->pSD->nHero == 2 && !armaHeroAura)
			Packet.WriteC(pUser->pED->customVerAuraHero);
		else if(armaHeroAura)
			Packet.WriteC(pUser->pED->customVerAuraHero);
		else
			Packet.WriteC(is_hero);
	}else{
		Packet.WriteC(0x00); // NO aura hero en masivos
	}

	/* c */ Packet.WriteC(ARG_C(tag));
	/* d */ Packet.WriteD(ARG_D(tag));
	/* d */ Packet.WriteD(ARG_D(tag));
	/* d */ Packet.WriteD(ARG_D(tag));

	DWORD NickNameColor = ARG_D(tag);
	if(pUser->pSD->nIsPledgeLeader && g_ClanLeaderNicknameColor != 0) NickNameColor = g_ClanLeaderNicknameColor;
	if(pUser->pSD->nBuilder && g_GMNicknameColor != 0)               NickNameColor = g_GMNicknameColor;
	if(pUser->pED->nicknameColor != 0)                                NickNameColor = pUser->pED->nicknameColor;
	if(pUser->pED->nameColorEx   != 0)                                NickNameColor = pUser->pED->nameColorEx;
	if(pUser->pED->aioId > 0 && g_AIOSystem.IsEnabled())              NickNameColor = pUser->pED->aioNicknameColor;
	if(pTerritory){ if(DWORD Color = pTerritory->GetColor()) NickNameColor = Color; }

	if(td.territoryId[FACTION_ZONE] != 0){
		if(pUser->pED->nFactionID == 1)      NickNameColor = (DWORD)255;   // Red
		else if(pUser->pED->nFactionID == 2) NickNameColor = (DWORD)65280; // Green
	}

	/* d */ Packet.WriteD(NickNameColor);

	va_end(tag);

	// C5 + IL things
	Packet.WriteD(heading);
	Packet.WriteD(pUser->GetPledgeClass());
	Packet.WriteD(pUser->GetPledgeType());

	DWORD titleColor = g_DefaultTitleColor;
	if(pUser->pSD->nIsPledgeLeader && g_ClanLeaderTitleColor != 0) titleColor = g_ClanLeaderTitleColor;
	if(g_PlayerCustomizer.IsEnabled()) titleColor = g_PlayerCustomizer.GetPvpTitleColor(pUser->pSD->nPvpCount);
	if(pUser->pSD->nBuilder && g_GMTitleColor != 0) titleColor = g_GMTitleColor;
	if(pUser->pED->titleColor   != 0) titleColor = pUser->pED->titleColor;
	if(pUser->pED->titleColorEx != 0) titleColor = pUser->pED->titleColorEx;

	if(td.territoryId[SHOW_HP_TITLE] != 0){
		int nCurrent_hp = (int)pUser->pSD->fHP;
		int nCurrent_cp = (int)pUser->pSD->fCP;
		int total_life  = nCurrent_hp + nCurrent_cp;
		int nMax_hp     = (int)pUser->pSD->MaxHP;
		int nMax_cp     = (int)pUser->pSD->MaxCP;
		int Maxtotal_life = nMax_hp + nMax_cp;

		int n50perhp = (int)(Maxtotal_life * 0.50);
		int n35perhp = (int)(Maxtotal_life * 0.35);

		if( (total_life <= n50perhp) && (total_life > n35perhp) ) titleColor = 33023; // Orange
		else if( total_life <= n35perhp )                         titleColor = 255;   // Red
	}

	if( (pUser->pSD->activateOfflineShopTime > 0) && (pUser->pED->nofflinesellbuffprice > 0) )
		titleColor = g_OfflineBuffer.GetOfflineBufferColor();

	if(pUser->pED->tvtUser.status == TvT::UserFighting || pUser->pED->tvtUser.status == TvT::UserPreparing){
		if(pUser->pED->tvtUser.flag) titleColor = 65535; // Yellow
	}

	if(pUser->pED->inZonePvPRank) titleColor = 65535;

	Packet.WriteD(titleColor);
	Packet.WriteD(pUser->pED->cursedWeaponLevel);
	Packet.WriteD(pUser->GetPledgeReputation());

	if(td.territoryId[NOCHARSHOWZONE] == 0)
	{
		int InstanceId_Other = 0;
		if(CUserSocket *pUS = pSocket->CastUserSocket()){
			if(User *pUserA = pSocket->CastUserSocket()->GetUser()){
				if(pUserA->ValidUser()){
					InstanceId_Other = pUserA->pED->InstanceId;
				}
			}
		}
		if (pUser->pED->InstanceId == InstanceId_Other)
			pSocket->Send("b", Packet.GetSize(), Packet.GetBuff());
	}
}

void CPacketFix::CharInfoBCFix(INT64 dwAddr, int nObjID, int nRange, FVector *pLocation, const char *format, ...)
{
    int race = 0;
    int clase = 0;
    int sex = 0;
    double ColRadius = 5;
    double ColHeight = 30;

    User *pUser = 0;
    UINT threadIndex = GetThreadIndex();
    if (threadIndex < 16) {
        pUser = g_CharInfoUser[threadIndex];
    }

    CPacket Packet;
    va_list tag;
    va_start(tag, format);

    // c
    int c0 = va_arg(tag, int);
    Packet.WriteC((BYTE)c0);

    // d d d
    Packet.WriteD(va_arg(tag, DWORD)); // X
    Packet.WriteD(va_arg(tag, DWORD)); // Y
    Packet.WriteD(va_arg(tag, DWORD)); // Z

    DWORD heading = va_arg(tag, DWORD);
    Packet.WriteD(heading); // Heading

    UINT nObjectID = va_arg(tag, DWORD); // ObjectID

    if (pUser == 0) {
        pUser = User::GetUserBySID(&nObjectID);
        if (!pUser->ValidUser()) {
            va_end(tag);
            return;
        }
    }

    TerritoryData& td = pUser->pSD->pExData->territoryData;

    Packet.WriteD(nObjectID);

    PWCHAR wName = va_arg(tag, PWCHAR);
    CNickNameChangeTerritory *pTerritory = NULL;
    if (int TerritoryID = pUser->pSD->nInNickNameChangeZone) {
        pTerritory = g_TerritoryChecker.GetTerritory(TerritoryID)->SafeCastNickNameChange();
        if (pTerritory) {
            PWCHAR wNewName = (PWCHAR)pTerritory->GetNickName();
            if (wNewName) {
                wName = wNewName;
            }
        }
    }
    Packet.WriteS(wName); // nickname

    DWORD Race = va_arg(tag, DWORD);
    if (pUser->pED->fakeRace != UINT_MAX)
        Race = pUser->pED->fakeRace;

    DWORD Sex = va_arg(tag, DWORD);
    if (pUser->pED->fakeSex != UINT_MAX)
        Sex = pUser->pED->fakeSex;

    DWORD BaseClass = va_arg(tag, DWORD);

    if (pUser->pED->actualizarApariencias)
    {
        int custom_sex = g_AccountDB.GetCustomSex(pUser->pSD->nDBID);
        if (custom_sex > -1) Sex = custom_sex;

        int customSkin = g_AccountDB.GetCustomSkin(pUser->pSD->nDBID);
        if (customSkin > -1) BaseClass = customSkin;

        if ( (BaseClass >= 0 && BaseClass <= 9) || (BaseClass >= 88 && BaseClass <= 93) )
            Race = 0;
        else if ( (BaseClass >= 10 && BaseClass <= 17) || (BaseClass >= 94 && BaseClass <= 98) )
            Race = 0;
        else if ( (BaseClass >= 18 && BaseClass <= 30) || (BaseClass >= 99 && BaseClass <= 105) )
            Race = 1;
        else if ( (BaseClass >= 31 && BaseClass <= 43) || (BaseClass >= 106 && BaseClass <= 112) )
            Race = 2;
        else if ( (BaseClass >= 44 && BaseClass <= 52) || (BaseClass >= 113 && BaseClass <= 116) )
            Race = 3;
        else if ( (BaseClass >= 53 && BaseClass <= 57) || (BaseClass >= 117 && BaseClass <= 118) )
            Race = 4;

        DWORD Magic = 0;
        if (CCategoryDataDB::IsInCategory(BaseClass, CCategoryDataDB::MAGE_GROUP))
            Magic = 1;

        ColRadius = g_World.GetRadius(Race, Sex, Magic);
        ColHeight = g_World.GetHeight(Race, Sex, Magic);

        pUser->pED->actualizarApariencias = false;
        pUser->pED->custom_sex = Sex;
        pUser->pED->customSkin = BaseClass;
        pUser->pED->Race = Race;
        pUser->pED->ColRadius = ColRadius;
        pUser->pED->ColHeight = ColHeight;
    }
    else
    {
        Sex       = pUser->pED->custom_sex;
        BaseClass = pUser->pED->customSkin;
        Race      = pUser->pED->Race;
        ColRadius = pUser->pED->ColRadius;
        ColHeight = pUser->pED->ColHeight;
    }

    Packet.WriteD(Race); // race
    Packet.WriteD(Sex);  // sex

    if (g_SubStack.IsEnabled()) {
        UINT charIndex = UINT_MAX;
        for (UINT n = 0; n < 7; n++) {
            if (pUser->pSocket->characterDBID[n] == pUser->nDBID) {
                charIndex = n;
                break;
            }
        }
        if (charIndex != UINT_MAX && charIndex < 7) {
            if (pUser->pSocket->pED->baseClass[charIndex] != UINT_MAX)
                BaseClass = pUser->pSocket->pED->baseClass[charIndex];
        }
    }
    Packet.WriteD(BaseClass);

    INT32 dhairId  = va_arg(tag, INT32);
    INT32 headId   = va_arg(tag, INT32);
    INT32 rhandId  = va_arg(tag, INT32);
    INT32 lhandId  = va_arg(tag, INT32);
    INT32 glovesId = va_arg(tag, INT32);
    INT32 chestId  = va_arg(tag, INT32);
    INT32 legsId   = va_arg(tag, INT32);
    INT32 feetId   = va_arg(tag, INT32);
    INT32 backId   = va_arg(tag, INT32);
    INT32 lrhandId = va_arg(tag, INT32);
    INT32 hairId   = va_arg(tag, INT32);
    INT32 faceId   = 0; // interlude addon

    Hair2Slot::GetItemIDCharInfo(pUser, dhairId, hairId, faceId);

    if (!pUser->pED->inZoneNoVisualEffect) {
        if (g_VisualArmor.IsEnabled()) {
            VisualArmorUser& vUser = pUser->pED->visualArmorUser;
            if (vUser.armorId > 0) {
                if (!pUser->olympiadUser.IsInside()) {
                    if (vUser.armorType == 0 || vUser.armorType == pUser->pSD->nArmorType) {
                        if (vUser.chestSlotId > 0)  chestId  = vUser.chestSlotId;
                        if (vUser.legsSlotId  > 0)  legsId   = vUser.legsSlotId;
                        if (vUser.glovesSlotId> 0)  glovesId = vUser.glovesSlotId;
                        if (vUser.feetSlotId  > 0)  feetId   = vUser.feetSlotId;
                        if (vUser.hairSlotId  > 0 && hairId == 0) hairId = vUser.hairSlotId;
                    }
                }
            }
        }
    }

    if (pUser->pED->tvtUser.status == TvT::UserFighting || pUser->pED->tvtUser.status == TvT::UserPreparing) {
        if (pUser->pED->tvtUser.flag) {
            rhandId = 6718;
            lrhandId = 6718;
        }
    }
    if (pUser->pED->BlockAppeZoneId) {
        dhairId = headId = rhandId = lhandId = glovesId = chestId = legsId = feetId = backId = lrhandId = hairId = 0;
        faceId = 0;
    }
    if (pUser->pED->m_CtfUserFlag > 0) {
        lrhandId = pUser->pED->m_CtfUserFlag;
        rhandId = 0; lhandId = 0;
    }

    Packet.WriteD(dhairId);
    Packet.WriteD(headId);
    Packet.WriteD(rhandId);
    Packet.WriteD(lhandId);
    Packet.WriteD(glovesId);
    Packet.WriteD(chestId);
    Packet.WriteD(legsId);
    Packet.WriteD(feetId);
    Packet.WriteD(backId);
    Packet.WriteD(lrhandId);
    Packet.WriteD(hairId);
    Packet.WriteD(faceId);

    UINT nAugmentation = pUser->GetAugmentationID();
    Packet.WriteH(0); Packet.WriteH(0); Packet.WriteH(0); Packet.WriteH(0);
    Packet.WriteD(nAugmentation);
    Packet.WriteH(0); Packet.WriteH(0); Packet.WriteH(0); Packet.WriteH(0);
    Packet.WriteH(0); Packet.WriteH(0); Packet.WriteH(0); Packet.WriteH(0);
    Packet.WriteH(0); Packet.WriteH(0); Packet.WriteH(0); Packet.WriteH(0);
    Packet.WriteD(nAugmentation);
    Packet.WriteH(0); Packet.WriteH(0); Packet.WriteH(0); Packet.WriteH(0);

    Packet.WriteD(va_arg(tag, DWORD));
    Packet.WriteD(va_arg(tag, DWORD));
    Packet.WriteD(va_arg(tag, DWORD));
    Packet.WriteD(va_arg(tag, DWORD));
    Packet.WriteD(va_arg(tag, DWORD));
    Packet.WriteD(va_arg(tag, DWORD));
    Packet.WriteD(va_arg(tag, DWORD));
    Packet.WriteD(va_arg(tag, DWORD));
    Packet.WriteD(va_arg(tag, DWORD));
    Packet.WriteD(va_arg(tag, DWORD));
    Packet.WriteD(va_arg(tag, DWORD));
    Packet.WriteD(va_arg(tag, DWORD));
    Packet.WriteD(va_arg(tag, DWORD));
    Packet.WriteD(va_arg(tag, DWORD));

    Packet.WriteF(va_arg(tag, double));
    Packet.WriteF(va_arg(tag, double));

    double radius = va_arg(tag, double);
    double heightD = va_arg(tag, double);

    if (pUser->pED->fakeRace != UINT_MAX || pUser->pED->fakeSex != UINT_MAX) {
        DWORD Magic = 0;
        if (CCategoryDataDB::IsInCategory(BaseClass, CCategoryDataDB::MAGE_GROUP))
            Magic = 1;
        radius  = g_World.GetRadius(Race, Sex, Magic);
        heightD = g_World.GetHeight(Race, Sex, Magic);
    } else {
        radius  = ColRadius;
        heightD = ColHeight;
    }
    Packet.WriteF(radius);
    Packet.WriteF(heightD);

    Packet.WriteD(va_arg(tag, DWORD)); // hair_style
    Packet.WriteD(va_arg(tag, DWORD)); // hair_color
    Packet.WriteD(va_arg(tag, DWORD)); // face

    if (pTerritory && pTerritory->IsHidePledge())
    {
        PWCHAR wUserTitleOriginal = va_arg(tag, PWCHAR); // consumir

        if (td.territoryId[SHOW_HP_TITLE] != 0)
        {
            int nCurrent_hp = static_cast<int>((int)pUser->pSD->fHP);
            int nCurrent_cp = static_cast<int>((int)pUser->pSD->fCP);
            int total_life  = nCurrent_hp + nCurrent_cp;

            wstring wUserTitleHP = L"CP/HP: [" + Utils::IntToWString(total_life) + L"]";
            if (pUser->pED->tvtUser.status == TvT::UserFighting || pUser->pED->tvtUser.status == TvT::UserPreparing) {
                if (pUser->pED->tvtUser.flag) wUserTitleHP = L"[FLAG]";
            }
            const WCHAR* wUserTitle = wUserTitleHP.c_str();
            Packet.WriteS(wUserTitle);
        }
        else
        {
            if (pUser->pED->inZonePvPRank) {
                wstring wUserTitlePvP = L"Puntos: [" + Utils::IntToWString(pUser->pED->puntosPvP) + L"]";
                Packet.WriteS(wUserTitlePvP.c_str());
            } else {
                Packet.WriteS(L"");
            }
        }

        (void)va_arg(tag, DWORD); Packet.WriteD(0); // pledge_id
        (void)va_arg(tag, DWORD); Packet.WriteD(0); // pledge_crest_id
        (void)va_arg(tag, DWORD); Packet.WriteD(0); // ally_id
        (void)va_arg(tag, DWORD); Packet.WriteD(0); // ally_crest_id
    }
    else
    {
        PWCHAR wUserTitleOriginal = va_arg(tag, PWCHAR);
        if (td.territoryId[SHOW_HP_TITLE] != 0)
        {
            int nCurrent_hp = static_cast<int>((int)pUser->pSD->fHP);
            int nCurrent_cp = static_cast<int>((int)pUser->pSD->fCP);
            int total_life  = nCurrent_hp + nCurrent_cp;

            wstring wUserTitleHP = L"CP/HP: [" + Utils::IntToWString(total_life) + L"]";
            if (pUser->pED->tvtUser.status == TvT::UserFighting || pUser->pED->tvtUser.status == TvT::UserPreparing) {
                if (pUser->pED->tvtUser.flag) wUserTitleHP = L"[FLAG]";
            }
            Packet.WriteS(wUserTitleHP.c_str());
        }
        else if ((pUser->pSD->activateOfflineShopTime > 0) && (pUser->pED->nofflinesellbuffprice > 0))
        {
            Packet.WriteS(pUser->pED->offlineselltitle.c_str());
        }
        else
        {
            if (pUser->pED->tvtUser.status == TvT::UserFighting || pUser->pED->tvtUser.status == TvT::UserPreparing)
            {
                if (pUser->pED->tvtUser.flag) {
                    // Packet.WriteS(L"[FLAG]");
                    Packet.WriteS(wUserTitleOriginal);
                } else {
                    Packet.WriteS(wUserTitleOriginal);
                }
            }
            else
            {
                if (pUser->pED->inZonePvPRank) {
                    wstring wUserTitlePvP = L"Puntos: [" + Utils::IntToWString(pUser->pED->puntosPvP) + L"]";
                    Packet.WriteS(wUserTitlePvP.c_str());
                } else {
                    Packet.WriteS(wUserTitleOriginal);
                }
            }
        }

        Packet.WriteD(va_arg(tag, DWORD)); // pledge_id
        Packet.WriteD(va_arg(tag, DWORD)); // pledge_crest_id
        Packet.WriteD(va_arg(tag, DWORD)); // ally_id
        Packet.WriteD(va_arg(tag, DWORD)); // ally_crest_id
    }

    // relation + flags
    DWORD _relation = va_arg(tag, DWORD);
    Packet.WriteD(_relation);

    int sit     = va_arg(tag, int);
    int run     = va_arg(tag, int);
    int combat  = va_arg(tag, int);
    int alike   = va_arg(tag, int);
    int hide    = va_arg(tag, int);
    int mount   = va_arg(tag, int);
    Packet.WriteC((BYTE)sit);
    Packet.WriteC((BYTE)run);
    Packet.WriteC((BYTE)combat);
    Packet.WriteC((BYTE)alike);
    Packet.WriteC((BYTE)hide);
    Packet.WriteC((BYTE)mount);

    int isprivatestore = va_arg(tag, int);
    if (g_OfflineBuffer.IsOfflineBuffer(pUser))
        isprivatestore = 0x01;
    Packet.WriteC((BYTE)isprivatestore);

    // --- Cubics (cliente: H count + count*H) ---
    WORD cubicCount = (WORD)va_arg(tag, int); // leer como int por promoción
    DWORD dBuffSize = va_arg(tag, DWORD);
    PUCHAR pCubics  = va_arg(tag, PUCHAR);

    if (pUser->pED->inZoneNoVisualEffect)
    {
        // consumidos los args, pero no mostramos nada
        Packet.WriteH(0);
    }
    else
    {
        if (cubicCount > 3) cubicCount = 3; // cota de sanidad
        Packet.WriteH(cubicCount);

        const WORD* wList = (const WORD*)pCubics;
        UINT availWords   = (pCubics && dBuffSize >= 2) ? (dBuffSize / 2) : 0;
        UINT emit         = (cubicCount <= availWords) ? cubicCount : availWords;

        for (UINT i = 0; i < emit; ++i)
            Packet.WriteH(wList[i]);
        for (UINT i = emit; i < cubicCount; ++i)
            Packet.WriteH(0);
    }
    // --- fin cubics ---

    if (pTerritory && pTerritory->IsHidePledge())
    {
        (void)va_arg(tag, int); // isInPartyMatchRoom (consumir)
        Packet.WriteC((BYTE)0);
    }
    else
    {
        int inPartyMatch = va_arg(tag, int);
        Packet.WriteC((BYTE)inPartyMatch);
    }

    DWORD AbnormalVisualEffect = va_arg(tag, DWORD);
    if (pUser->pSD->pExData->SkillMod.semiInvisible)
        AbnormalVisualEffect |= 0x100000; // stealth
    if (pUser->pED->spawnProtection == 2)
        AbnormalVisualEffect |= 0x200000;
    Packet.WriteD(AbnormalVisualEffect);

    int c1 = va_arg(tag, int);
    WORD h0 = (WORD)va_arg(tag, int);
    Packet.WriteC((BYTE)c1);
    Packet.WriteH(h0);

    Packet.WriteD(va_arg(tag, DWORD));
    Packet.WriteD(va_arg(tag, DWORD));
    Packet.WriteD(va_arg(tag, DWORD));

    int c2 = va_arg(tag, int);
    BYTE bTeam = (BYTE)va_arg(tag, int);

    if (pUser->pSD->DuelInfo.DuelID && pUser->pSD->DuelInfo.Fighting)
        bTeam = pUser->pSD->DuelInfo.DuelTeam;
    if (pUser->pED->tvtUser.status == TvT::UserFighting || pUser->pED->tvtUser.status == TvT::UserPreparing)
        bTeam = pUser->pED->tvtUser.team;
    else if (pTerritory && pTerritory->GetTeam())
        bTeam = pTerritory->GetTeam();

    if (pUser->pED->inEventPvp > 0)
        bTeam = pUser->pED->inEventPvp;

    Packet.WriteC((BYTE)c2);
    Packet.WriteC(bTeam);

    Packet.WriteD(va_arg(tag, DWORD));

    int c3 = va_arg(tag, int);
    Packet.WriteC((BYTE)c3);

    int is_hero = va_arg(tag, int);
    bool armaHeroAura = false;

    if (g_MinEnchantLevelAuraHero > 0) {
        CSharedItemData *pData = pUser->GetEquipedWeapon();
        if (pData) {
            CItem *pItem = pUser->inventory.GetItemBySID(pData->nObjectID);
            if (pItem->IsValid(VT_ITEMWEAPON)) {
                if (pItem->pII->nCrystalType == 5 && pItem->pSID->nEnchantLevel >= g_MinEnchantLevelAuraHero) {
                    is_hero = 1;
                    armaHeroAura = true;
                }
            }
        }
    }

    if (!pUser->pED->inZoneNoVisualEffect) {
        if (pUser->pSD->nHero == 2 && !armaHeroAura)
            Packet.WriteC((BYTE)pUser->pED->customVerAuraHero);
        else if (armaHeroAura)
            Packet.WriteC((BYTE)pUser->pED->customVerAuraHero);
        else
            Packet.WriteC((BYTE)is_hero);
    } else {
        Packet.WriteC(0x00); // no aura en masivos
    }

    int c4 = va_arg(tag, int);
    Packet.WriteC((BYTE)c4);

    Packet.WriteD(va_arg(tag, DWORD));
    Packet.WriteD(va_arg(tag, DWORD));
    Packet.WriteD(va_arg(tag, DWORD));

    DWORD NickNameColor = va_arg(tag, DWORD);
    if (pUser->pSD->nIsPledgeLeader && g_ClanLeaderNicknameColor != 0)
        NickNameColor = g_ClanLeaderNicknameColor;
    if (pUser->pSD->nBuilder && g_GMNicknameColor != 0)
        NickNameColor = g_GMNicknameColor;
    if (pUser->pED->nicknameColor != 0)
        NickNameColor = pUser->pED->nicknameColor;
    if (pUser->pED->nameColorEx != 0)
        NickNameColor = pUser->pED->nameColorEx;
    if (pUser->pED->aioId > 0) {
        if (g_AIOSystem.IsEnabled())
            NickNameColor = pUser->pED->aioNicknameColor;
    }
    if (pTerritory) {
        if (DWORD Color = pTerritory->GetColor())
            NickNameColor = Color;
    }
    if (td.territoryId[FACTION_ZONE] != 0) {
        if (pUser->pED->nFactionID == 1)
            NickNameColor = (DWORD)255; // Red
        else if (pUser->pED->nFactionID == 2)
            NickNameColor = (DWORD)65280; // Green
    }

    Packet.WriteD(NickNameColor);

    va_end(tag);

    // C5 + IL tail
    Packet.WriteD(heading);
    Packet.WriteD(pUser->GetPledgeClass());
    Packet.WriteD(pUser->GetPledgeType());

    DWORD titleColor = g_DefaultTitleColor;
    if (pUser->pSD->nIsPledgeLeader && g_ClanLeaderTitleColor != 0)
        titleColor = g_ClanLeaderTitleColor;

    if (g_PlayerCustomizer.IsEnabled())
        titleColor = g_PlayerCustomizer.GetPvpTitleColor(pUser->pSD->nPvpCount);

    if (pUser->pSD->nBuilder && g_GMTitleColor != 0)
        titleColor = g_GMTitleColor;

    if (pUser->pED->titleColor != 0)
        titleColor = pUser->pED->titleColor;
    if (pUser->pED->titleColorEx != 0)
        titleColor = pUser->pED->titleColorEx;

    if (td.territoryId[SHOW_HP_TITLE] != 0)
    {
        int nCurrent_hp = static_cast<int>((int)pUser->pSD->fHP);
        int nCurrent_cp = static_cast<int>((int)pUser->pSD->fCP);
        int total_life  = nCurrent_hp + nCurrent_cp;

        int nMax_hp = static_cast<int>((int)pUser->pSD->MaxHP);
        int nMax_cp = static_cast<int>((int)pUser->pSD->MaxCP);
        int Maxtotal_life = nMax_hp + nMax_cp;

        int n50perhp = (int)(Maxtotal_life * 0.50);
        int n35perhp = (int)(Maxtotal_life * 0.35);

        if ((total_life <= n50perhp) && (total_life > n35perhp))
            titleColor = 33023; // Orange
        else if (total_life <= n35perhp)
            titleColor = 255;   // Red
    }

    if ((pUser->pSD->activateOfflineShopTime > 0) && (pUser->pED->nofflinesellbuffprice > 0))
        titleColor = g_OfflineBuffer.GetOfflineBufferColor();

    if (pUser->pED->tvtUser.status == TvT::UserFighting || pUser->pED->tvtUser.status == TvT::UserPreparing) {
        if (pUser->pED->tvtUser.flag)
            titleColor = 65535; // Yellow
    }

    if (pUser->pED->inZonePvPRank)
        titleColor = 65535;

    Packet.WriteD(titleColor);
    Packet.WriteD(pUser->pED->cursedWeaponLevel);
    Packet.WriteD(pUser->GetPledgeReputation());

    if (td.territoryId[NOCHARSHOWZONE] == 0) {
        BroadcastToNeighborExceptSelf(nObjID, nRange, pLocation, (int)Packet.GetSize(), Packet.GetBuff());
    }
}
