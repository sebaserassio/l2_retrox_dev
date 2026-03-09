#pragma once


struct PKPvPUserInfo
{
	wstring name;
	UINT classId;
	INT32 level;
	INT32 pvpAmount;
	INT32 pkAmount;
	wstring nameClan;
	wstring nameAlly;
	INT32 crestClan;
	INT32 crestAlly;
};

class CPKPvPStatus
{
	BOOL m_Enabled;
	CSLock m_Lock;
	UINT m_RefreshTimeout;
	time_t m_RefreshTime;
	vector<PKPvPUserInfo> m_DBPK;
	vector<PKPvPUserInfo> m_DBPvP;
	vector<PKPvPUserInfo> m_DBDaily;
	vector<PKPvPUserInfo> m_DBSemana;
	vector<wstring> m_PagesPvP;
	vector<wstring> m_PagesPK;
	vector<wstring> m_PagesDaily;
	vector<wstring> m_PagesSemana;
	void LoadINI();
public:
	void Init();
	void TimerExpired();
	void OnDBLoadPK(UINT charId, wstring name, UINT classId, INT32 level, INT32 pvp, INT32 pk, wstring nameClan, wstring nameAlly, UINT crestClan, UINT crestAlly);
	void OnDBLoadPvP(UINT charId, wstring name, UINT classId, INT32 level, INT32 pvp, INT32 pk, wstring nameClan, wstring nameAlly, UINT crestClan, UINT crestAlly);
	void OnDBLoadDaily(UINT charId, wstring name, UINT classId, INT32 level, INT32 pvp, INT32 pk, wstring nameClan, wstring nameAlly, UINT crestClan, UINT crestAlly);
	void OnDBLoadSemana(UINT charId, wstring name, UINT classId, INT32 level, INT32 pvp, INT32 pk, wstring nameClan, wstring nameAlly, UINT crestClan, UINT crestAlly);

	
	void RequestPKStatus(User *pUser, UINT page = 0);
	void RequestPvPStatus(User *pUser, UINT page = 0);
	void RequestDailyStatus(User *pUser, UINT page = 0);
	void RequestSemanaStatus(User *pUser, UINT page = 0);
	

	void GeneratePagesPK();
	void GeneratePagesPvP();
	void GeneratePagesDaily();
	void GeneratePagesSemana();

	

	void SendHeroListPvP(User *pUser, INT32 desde, INT32 cantidad);
	void SendHeroListPK(User *pUser, INT32 desde, INT32 cantidad);
	void SendHeroListDaily(User *pUser, INT32 desde, INT32 cantidad);
	void SendHeroListSemana(User *pUser, INT32 desde, INT32 cantidad);

};

extern CPKPvPStatus g_PKPvPStatus;