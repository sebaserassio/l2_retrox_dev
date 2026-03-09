#pragma once
#include "CCreature.h"
#include "xstd.h"



class ItemDropStruct {
public:
	int itemType;
	int amountMin;
	int amountMax;
	long double chance;
};

class ItemDropMultiStruct {
public:
	xstd::vector<ItemDropStruct*> *pItemDropList;
	double DropProb;
//	bool m_bChanceAffected;
};



//0x238
struct NpcDataInfo
{
  /* 0x0000 */ int walk_speed;
  /* 0x0004 */ int swim_walk_speed;
  /* 0x0008 */ int fly_walk_speed2;
  /* 0x000C */ int fly_walk_speed;
  /* 0x0010 */ int run_speed;
  /* 0x0014 */ int swim_run_speed;
  /* 0x0018 */ int fly_run_speed2;
  /* 0x001C */ int fly_run_speed;
  /* 0x0000 */ //int m_SpeedByME[2][4];
  /* 0x0020 */ int m_RegenByME[2][4];
  /* 0x0040 */ int m_NoiseRangeByME[2][4];
  /* 0x0060 */ xstd::vector<__int64> m_SkillList;
  /* 0x0080 */ unsigned char padding0x0080[0x00C0 - 0x0080];
  /* 0x00C0 */ xstd::vector<ItemDropStruct> m_CorpseMakeList;
  /* 0x00E0 */ xstd::vector<ItemDropMultiStruct> m_ItemAdditionalMakeListMulti;
  /* 0x0100 */ map<const wstring, const wstring> aiParams;
  /* 0x0118 */ unsigned char padding0x0118[0x0128 - 0x0118];
  /* 0x0128 */ double m_dCollisionHeight[2];
  /* 0x0138 */ double m_dCollisionRadius[2];
  /* 0x0148 */ unsigned char padding0x0148[0x0238 - 0x0148];
  /* 0x0238 */
};


static void checkCNPCClassConst()
{
//	static_assert(offsetof(CNPCClassConst, m_CorpseMakeList) == 0x007C);
	CompileTimeOffsetCheck(NpcDataInfo, aiParams, 0x0100);

}

class CNPC: public CCreature
{
public:
	/* 1500 */ LPVOID _unkn1500[32];
	/* 1600 */ LPVOID _unkn1600[32];	
	/* 1700 */ LPVOID _unkn1700[32];
	/* 1800 */ LPVOID _unkn1800[32];
	/* 1900 */ LPVOID _unkn1900[26];
	/* 19D0 */ NpcDataInfo* pND;

	void TimerExpired(int id);
	void KillNPC();
	void TeleportToLocation(int x, int y, int z, int param0);
	void ValidateOrgHPMP();
	static CNPC* GetNPCByServerId(LPUINT pSID);
};
