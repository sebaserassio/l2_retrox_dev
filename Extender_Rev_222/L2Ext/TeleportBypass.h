#pragma once

struct TeleportSpawn {
    INT32 x, y, z;
    TeleportSpawn() : x(0), y(0), z(0) {}
    TeleportSpawn(INT32 _x, INT32 _y, INT32 _z) : x(_x), y(_y), z(_z) {}
};

struct TeleportData
{
	int x;
	int y;
	int z;
	int price;
	int min_level;
	int max_level;
	int required_item_id;
	int required_item_count;
	int castle_id;
	int faction_id;

	INT32 hourInit;
	INT32 minuteInit;

	INT32 hourEnd;
	INT32 minuteEnd;

    // NUEVO: lista de spawns alternativos
    std::vector<TeleportSpawn> spawns;

	void Clear()
	{
		x = 0;
		y = 0;
		z = 0;
		price = 0;
		min_level = 0;
		max_level = 0;
		required_item_id = 0;
		required_item_count = 0;
		castle_id = 0;
		faction_id = 0;

		hourInit = 99; 
		minuteInit = 99;
		hourEnd  = 99; 
		minuteEnd  = 99;

		spawns.clear();
	}
};

class CTeleportBypass
{
	map<wstring, TeleportData> mData;
public:
	static const int MsgID = 2413;
	CTeleportBypass();
	void Init();
	void ReadData();
	void RequestTeleport(User *pUser, wstring wName);
	void Teleport(User *pUser, wstring wName);
};

extern CTeleportBypass g_TeleportBypass;