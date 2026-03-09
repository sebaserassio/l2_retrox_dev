#pragma once





extern "C"
{
	void HookGetIOBuffer();
};

struct MemoryBotData
{
	int id;
    int memoryaddr;
    BYTE Data[256];  // Asegúrate de tener suficiente espacio
    int cantidad;
};

struct MemoryChangeData
{
	int id;
	wstring dllName;
    int memoryaddr;
    BYTE Data[256];  // Asegúrate de tener suficiente espacio
    int cantidad;
};




struct CustomRaidBossData
{
	int id;
    wstring nameServer;
    wstring nameClient;
    int level;
	int estado;		//0 muerto //1 vivo
	int fechaSpawn;
	int fechaMuerto;
	wstring ultimoGolpe;
	wstring ultimoGolpeClan;

	CustomRaidBossData() : id(0), level(0), estado(0), fechaSpawn(0), fechaMuerto(0) {};
};


class NuevasFunciones
{
public:
	static void Init();
	static void LoadINI();
	static void VerificarEstructuras();
	static int NPCBuySellListAssemble(CHAR *pBuf, int nBufLen, char *format, short nItemType1, int nObjectID, int nItemID, int nCount, short nItemType2, short nUnknown1, int nBodyPart, short nEnchantLevel, short nUnknown2, short nUnknown3, int nPrice);


	static void setCharacterName(int charId, const std::wstring& newCharName);

	static void AddItemToInventorySpoilCount(CCreature* this1, int itemId, int amount, bool isPick, int enchanted, int blessed, int eroded);

	static void AddItemToInventoryNpcAIDrop(CCreature* this1, int itemId, int amount, bool isPick, int enchanted, int blessed, int eroded);

	static void CSkillInfo__CubicActivate(CSkillInfo *pSkillInfo, CCreature *pSource, LPVOID pCubicInfo, CObject *pTarget);

	static bool CustomsBypass(User* pUser, wstring wBypass);
	static void IncreaseUserPKCount(CDB* pInstance, User* pUser);

	static void EnviarCrackInterfaceCelestine(CUserSocket *pSocket);
	static void EnviarHooksAlCliente(CUserSocket *pSocket);

	static void CCastle__BanNonDefendingPlayers(CCastle *this_, bool bIsMidVictory);

	static int User__ExpDown(User *this_, int nExpDec);

	static void Add_Hook(CLog* pInstance, int Type, const wchar_t *format, va_list va);
	static void AddAnsi_Hook(CLog* pInstance, int Type, const char *format, va_list va);
	static void CerrarHallesAutomaticamente(CUserSocket* pSocket, int nId);

	static void RequestSaveCharacterInfoPVPCOUNT(CDB* pInstance, User *pUser, bool notActive, bool force);
	static void RequestSaveCharacterInfoPKCOUNT(CDB* pInstance, User *pUser, bool notActive, bool force);

	static User* GetUser(CSocket* pSocket);


	static void NuevasFunciones::CHolyThing__SendWarningMessage(DWORD instance);
	static void NuevasFunciones::CResidence_SetPledge(DWORD instance, CPledge *pPledge);
	static WCHAR* NuevasFunciones::CPledge__GetPledgeName(CPledge *pPledge);

	static CIOBuffer* NuevasFunciones::GetIOBuffer();

	static bool NuevasFunciones::CheckBowCondition(CAttackAction *this_, CCreature *pActor, int nMpCon, int nReuseDelay, bool bSurely, CSharedItemData *spItem);


	static CItem* NuevasFunciones::GetNextItemByClassID(CInventory *this_, int nBeforeIndex);
	static CItem* NuevasFunciones::GetFirstItemByClassID(CInventory*this_, int nClassId);
	static CItem* NuevasFunciones::GetFirstItemByClassIDEnchanted(CInventory*this_, int nClassId, int nEnchanted);
	static CItem* NuevasFunciones::GetNextMinEnchantedItemByClassID(CInventory*this_, int nClassId, int nBeforeIndex);
	static CItem* NuevasFunciones::GetMinEnchantedItemByClassID(CInventory*this_, int nClassId);

	static bool NuevasFunciones::NpcAnnounceEx(DWORD *pSocket, const unsigned char* pPacket);

	static void NuevasFunciones::User_SendInSight(User* pUser, CObject* pObject, bool bNewlyCreated);

	std::vector<std::pair<int, CustomRaidBossData*> > RaidBossDataMap;
	void AddBossState(int id);
	void SetBossState(int id, int estado, int fechaSpawn, int fechaMuerto, wstring ultimoGolpe, wstring ultimoGolpeClan);
	CustomRaidBossData* GetBossState(int id);

	class DropRaidsCustom
	{

	};


	class BuilderCommand
	{
	public:
		static bool TvTInit(User* pUser, wstring wParams);
		static bool TvTStop(User* pUser, wstring wParams);

		static bool DoorInfo(User* pUser, wstring wParams);
		static bool OpenDoor(User *pUser, wstring wParams);
		static bool CloseDoor(User *pUser, wstring wParams);

		static bool OpenDoorWall(User *pUser, wstring wParams);
		static bool CloseDoorWall(User *pUser, wstring wParams);


		static bool ShowAdminPanel(User *pUser, wstring wsParams);
		static bool VerBuffs(User *pUser, wstring wsParams);
		static bool NpcName(User *pUser, wstring wParams);
		static bool SetClass(User *pUser, wstring wsParams);
		static bool CustomTeleport(User *pUser, wstring wParams);
		static bool GetAllUser(User *pUser, std::wstring wsParams);
		static bool CustomResurrect(User *pUser, wstring wParams);
		static bool GetTerritory(User *pUser, wstring wParams);
		static bool KillNc(User *pUser, wstring wParams);
		static bool EventManager(User *pUser, wstring wParams);
		static bool TestAdrenaline(User *pUser, wstring wsParams);

		static bool SetTimeClanPenaltyZone(User *pUser, wstring wsParams);
		static bool BlockChat(User *pUser, wstring wsParams);

		static bool ChangeName(User *pUser, wstring wParams);

		static bool TestBot(User *pUser, wstring wParams);

		
	};

};



