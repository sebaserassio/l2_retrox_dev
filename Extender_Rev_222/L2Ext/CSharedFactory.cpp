#include "StdAfx.h"
#include <windows.h>

#include <cstdio>

#define IMAGEBASE 0x400000
#define arg_0  0x08
#define arg_8  0x10
#define arg_10 0x18
#define arg_18 0x20

LPINT lpCreatureDataCount = (LPINT)0xBF7170;
CCreatureSharedFactory *g_CreatureSharedFactory = (CCreatureSharedFactory*)0xE4120A0;


UINT contarErrores = 0;
CreatureSharedData* pSD_Rescate[10000];


void CSharedFactory::Initialize()
{
	ExtendCreatureSharedData();
	ExtendPledgeSharedData();
	ReallocateAbnormalTable();
	ReallocateQuests();
}




// Si querés fijar la capacidad desde build, define DESIRED_CREATURE_COUNT en tu proyecto.
// #define DESIRED_CREATURE_COUNT 70000

// Lee la capacidad deseada.
// - Si definís DESIRED_CREATURE_COUNT, usa ese valor (clamp a 70.000).
// - Si no, deja la capacidad actual.

/*
static int ReadDesiredCreatureCount()
{
#ifdef DESIRED_CREATURE_COUNT
    int desired = DESIRED_CREATURE_COUNT;
    if (desired < *lpCreatureDataCount) desired = *lpCreatureDataCount;
    if (desired > 70000) desired = 70000; // techo seguro por ThreadSafeLibrary
    return desired;
#else
    return *lpCreatureDataCount; // no cambiar capacidad si no definiste nada
#endif
}*/

// Reserva un bloque de memoria RW. Usamos VirtualAlloc para simplificar.
static void* AllocRW(SIZE_T bytes)
{
    return VirtualAlloc(NULL, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}
static void FreeRW(void* p)
{
    if (p) VirtualFree(p, 0, MEM_RELEASE);
}

// Aumenta la capacidad "en frío": debe llamarse ANTES de crear criaturas.
static BOOL ExpandPool_Cold(CCreatureSharedFactory* f, int newCount)
{
    if (!f) return FALSE;
    const int oldCount = *lpCreatureDataCount;
    if (newCount <= oldCount) return TRUE;

    // Debe ejecutarse cuando aún no hay criaturas
    if (f->addedCount != 0 || f->removedCount != 0) {
        g_Log.Add(CLog::Error, "[ExpandPool_Cold] Too late: added=%d removed=%d",
                  f->addedCount, f->removedCount);
        return FALSE;
    }

    SIZE_T sdBytes = (SIZE_T)newCount * sizeof(CreatureSharedData);
    SIZE_T ixBytes = (SIZE_T)newCount * sizeof(DWORD);

    CreatureSharedData* newSD = (CreatureSharedData*)AllocRW(sdBytes);
    DWORD*              newIX = (DWORD*)AllocRW(ixBytes);
    if (!newSD || !newIX) {
        g_Log.Add(CLog::Error, "[ExpandPool_Cold] alloc failed sd=%p ix=%p (newCount=%d)", newSD, newIX, newCount);
        FreeRW(newSD); FreeRW(newIX);
        return FALSE;
    }

    ZeroMemory(newSD, sdBytes);
    ZeroMemory(newIX, ixBytes);

    // Inicializamos la free-list 0..N-1
    for (int i = 0; i < newCount; ++i) {
        newSD[i].nServerIndex = i;
        newSD[i].nSharedIndex = -1;
        newSD[i].nNextIndex   = (i + 1 < newCount) ? (i + 1) : -1;
        newIX[i] = -1; // libre
    }

    f->_lock.Lock(__FILEW__, __LINE__);

    // Si el exe ya tenía memoria previa, liberala (si procede).
    if (f->lpCreatureData) FreeRW(f->lpCreatureData);
    if (f->lpCreatureIndex) FreeRW(f->lpCreatureIndex);

    f->lpCreatureData  = newSD;
    f->lpCreatureIndex = newIX;

    f->firstIndex    = (newCount > 0) ? 0 : -1;
    f->lastIndex     = (newCount > 0) ? (newCount - 1) : -1;
    f->addedCount    = 0;
    f->removedCount  = 0;
    f->totalCount    = newCount;
    f->remainCount   = newCount;

    *lpCreatureDataCount = newCount; // MUY IMPORTANTE: mantener sincronizado

    f->_lock.Unlock();

    g_Log.Add(CLog::Blue, "[ExpandPool_Cold] capacity=%d (was %d)", newCount, oldCount);
    return TRUE;
}

/*
// --------------------------------------------------------------------------
// Initialize: tus parches + creación del semáforo (una vez)
void CSharedFactory::Initialize()
{
    // Subir capacidad en frío (opcional, clamp interno a 70k si usás DESIRED_CREATURE_COUNT)
    {
        CCreatureSharedFactory* f = g_CreatureSharedFactory;
        int desired = ReadDesiredCreatureCount();
        ExpandPool_Cold(f, desired);
    }

    // Inicializar el semáforo de slots libres (si usás la versión con semáforo)
    InitFreeSlotsSemaphore(g_CreatureSharedFactory);


    // El resto de tus parches
    ExtendCreatureSharedData();
    ExtendPledgeSharedData();
    ReallocateAbnormalTable();
    ReallocateQuests();
    // ReallocatePaperDoll(); // si aplica
}
*/



CreatureSharedData* CCreatureSharedFactoryAllocateWrapper(CCreatureSharedFactory* pInstance, CCreature *pCreature)
{
	return pInstance->Allocate(pCreature);
}

BOOL CCreatureSharedFactoryFreeWrapper(CCreatureSharedFactory* pInstance, int serverIndex)
{
	return pInstance->Free(serverIndex);
}

void CSharedFactory::ExtendCreatureSharedData()
{
	g_HookManager.WriteCall(0x4D68AA, CCreatureSharedFactoryAllocateWrapper, 0);
	g_HookManager.WriteCall(0x4D7CF8, CCreatureSharedFactoryAllocateWrapper, 0);
	g_HookManager.WriteCall(0x4D5267, CCreatureSharedFactoryFreeWrapper, 0);

	//Extending CSharedData in CSharedFactory
	DWORD NewSize = sizeof(CreatureSharedData);
		//Constructor
	g_HookManager.WriteMemoryDWORD(0x4D2EC9 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x4D2EF7 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x4D2F07 , NewSize);
		//Destructor
	g_HookManager.WriteMemoryDWORD(0x4D335C , NewSize);
	g_HookManager.WriteMemoryDWORD(0x4D3348 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x4D333D , NewSize);
		//Changes in other funcs
	g_HookManager.WriteMemoryDWORD(0x4E149E, NewSize);
	g_HookManager.WriteMemoryDWORD(0x4E60F5 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x611953 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x611985 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x611AB9 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x611C71 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x6D8494 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x6DB6B3 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x7890E7 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x78915F , NewSize);
	g_HookManager.WriteMemoryDWORD(0x7892D0 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x78933D , NewSize);
	g_HookManager.WriteMemoryDWORD(0x7893DE , NewSize);
	g_HookManager.WriteMemoryDWORD(0x7893F5 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x78B14C , NewSize);
	g_HookManager.WriteMemoryDWORD(0x78C14E , NewSize);
	g_HookManager.WriteMemoryDWORD(0x78C93D , NewSize);
	g_HookManager.WriteMemoryDWORD(0x78CA6A , NewSize);
	g_HookManager.WriteMemoryDWORD(0x78CA81 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x841142 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x841B00 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x87959A , NewSize);
	g_HookManager.WriteMemoryDWORD(0x8A04DF , NewSize);
	g_HookManager.WriteMemoryDWORD(0x8A17FD , NewSize);
	g_HookManager.WriteMemoryDWORD(0x8A210D , NewSize);
	g_HookManager.WriteMemoryDWORD(0x8A29B4 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x8A3059 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x8B1BDC , NewSize);
	g_HookManager.WriteMemoryDWORD(0x8B241A , NewSize);
	g_HookManager.WriteMemoryDWORD(0x8B2C47 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x8B36CB , NewSize);
	g_HookManager.WriteMemoryDWORD(0x8B3EF6 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x8B4721 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x8B4C8C , NewSize);
	g_HookManager.WriteMemoryDWORD(0x8B5566 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x8B633D , NewSize);
	g_HookManager.WriteMemoryDWORD(0x8B6C45 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x8BDFE9 , NewSize);
	g_HookManager.WriteMemoryDWORD(0x8C0530 , NewSize);
	// .text:000000000078CA89                 mov     dword ptr [rcx+rax-8B8h], 0FFFFFFFFh
	g_HookManager.WriteMemoryDWORD(0x78CA8C, (DWORD)((DWORD)0x58 - NewSize) );
}

void CSharedFactory::ExtendPledgeSharedData()
{
	//Extending CPledgeSharedData
	DWORD dSize = sizeof(CPledgeSharedData);
	g_HookManager.WriteMemoryDWORD(0x762DD9, dSize);
	g_HookManager.WriteMemoryDWORD(0x762E07, dSize);
	g_HookManager.WriteMemoryDWORD(0x762E17, dSize);
	g_HookManager.WriteMemoryDWORD(0x76324D, dSize);
	g_HookManager.WriteMemoryDWORD(0x763258, dSize);
	g_HookManager.WriteMemoryDWORD(0x76326C, dSize);
	g_HookManager.WriteMemoryDWORD(0x78D3CD, dSize);  
	g_HookManager.WriteMemoryDWORD(0x78D4FA, dSize); 
	g_HookManager.WriteMemoryDWORD(0x78D511, dSize); 
	dSize = dSize - 0x124;
	g_HookManager.WriteMemoryDWORD(0x76B73B, dSize); //Clearing new memory size
}

void CSharedFactory::ReallocateAbnormalTable()
{
	DWORD NewAddr = offsetof(CreatureSharedData, CreatureSharedData::AbnormalTable);
	BYTE MaxAbnormals = 60;
	
	//Fix AddAbnormalStatus 
	g_HookManager.WriteMemoryDWORD(0x4D96E7, NewAddr);
	g_HookManager.WriteMemoryBYTE(0x4D9701, MaxAbnormals);
	g_HookManager.WriteMemoryDWORD(0x4D970C, NewAddr);
	g_HookManager.WriteMemoryDWORD(0x4D9714, (NewAddr+4));
	//Fix DeleteAllAbnormalStatus
	g_HookManager.WriteMemoryDWORD(0x4D234F, NewAddr);
	g_HookManager.WriteMemoryBYTE(0x4D2354, MaxAbnormals);
	//Fix DeleteAbnormalStatus
	g_HookManager.WriteMemoryDWORD(0x4D95AE, NewAddr);
	g_HookManager.WriteMemoryBYTE(0x4D95C2, MaxAbnormals);
	g_HookManager.WriteMemoryDWORD(0x4D95CD, NewAddr);
	//Fix InfectedSkillBy
		//AddAbnormal
	g_HookManager.WriteMemoryDWORD(0x5065E7, NewAddr);
	g_HookManager.WriteMemoryBYTE(0x506601, MaxAbnormals);
	g_HookManager.WriteMemoryDWORD(0x50660D, NewAddr);
	g_HookManager.WriteMemoryDWORD(0x506615, (NewAddr+4));
		//Delete Abnormal
	g_HookManager.WriteMemoryDWORD(0x505AD5, NewAddr);
	g_HookManager.WriteMemoryBYTE(0x505AF0, MaxAbnormals);
	g_HookManager.WriteMemoryDWORD(0x505AFC, NewAddr);
	//AddAbnormal2 - Check Amount of buffs
	//g_HookManager.WriteJump( 0x5057FB, (DWORD)CheckBuffAmount, 4);
	//Fix SendAbnormalStatus
	g_HookManager.WriteMemoryBYTE(0x827D54, MaxAbnormals);
	//Fix SpelledInfoChanged 
	g_HookManager.WriteMemoryBYTE(0x850F7B, MaxAbnormals);
	//Fix CCreature::CCreature() - Delete All Abnormals
	g_HookManager.WriteMemoryDWORD(0x4D6C46, NewAddr);
	g_HookManager.WriteMemoryBYTE(0x4D6C4B, MaxAbnormals);
	//Fix CCreature::CCreature(&other) - Delete All Abnormals
	g_HookManager.WriteMemoryDWORD(0x4D817F, NewAddr);
	g_HookManager.WriteMemoryBYTE(0x4D8184, MaxAbnormals);
	//Fix CCreature::DispelByCategory
	g_HookManager.WriteMemoryDWORD(0x5018A3, NewAddr);
	g_HookManager.WriteMemoryBYTE(0x5018B6, MaxAbnormals);
	g_HookManager.WriteMemoryDWORD(0x5018BE, NewAddr);
	//Fix CCreature::DispelAll
	g_HookManager.WriteMemoryDWORD(0x502555, NewAddr);
	g_HookManager.WriteMemoryBYTE(0x50256F, MaxAbnormals);
	g_HookManager.WriteMemoryDWORD(0x502578, NewAddr);
	//Fix CCreature::DispelAllByGM
	g_HookManager.WriteMemoryDWORD(0x502ECD, NewAddr);
	g_HookManager.WriteMemoryBYTE(0x502EE0, MaxAbnormals);
	g_HookManager.WriteMemoryDWORD(0x502EE8, NewAddr);
	//Fix CCreature::DispelByName
	g_HookManager.WriteMemoryDWORD(0x5039B8, NewAddr);
	g_HookManager.WriteMemoryBYTE(0x5039CF, MaxAbnormals);
	g_HookManager.WriteMemoryDWORD(0x5039D8, NewAddr);
	//Fix CCreature::DispelBySlot
	g_HookManager.WriteMemoryDWORD(0x504553, NewAddr);
	g_HookManager.WriteMemoryBYTE(0x504566, MaxAbnormals);
	g_HookManager.WriteMemoryDWORD(0x50456F, NewAddr);
	//Fix User::DeleteYongmaAbnormalStatus
	g_HookManager.WriteMemoryDWORD(0x831964, NewAddr);
	g_HookManager.WriteMemoryBYTE(0x831977, MaxAbnormals);
	g_HookManager.WriteMemoryDWORD(0x831980, NewAddr);
	//Fix User::OnChangeSubjobEnd - Delete All Abnormals
	g_HookManager.WriteMemoryDWORD(0x854191, NewAddr);
	g_HookManager.WriteMemoryBYTE(0x854196, MaxAbnormals);
}

void CSharedFactory::ReallocateQuests()
{
	CreatureSharedData test;
	int count = sizeof(test.Quests) / sizeof(QuestData);
	int index = (int)&test.Quests - (int)&test;
	int countMem = count * 16;
	int countMemSmall = count*8;
	int indexEnd = index + countMem;
	// inline bool CSharedCreatureData::SetJournal(int nQuestID , int nJournal)
	// .text:00000000004298D5                 lea     rax, [rdi+1E4h]
	g_HookManager.WriteMemoryDWORD(0x4298D8, index);
	// .text:00000000004298DC                 lea     rcx, [rdi+2E4h]
	g_HookManager.WriteMemoryDWORD(0x4298DF, indexEnd);
	// inline bool  CSharedCreatureData::SetFlagJournal(int nQuestID , int nFlag) 
	// .text:0000000000429A85                 lea     rdx, [rsi+1E4h]
	g_HookManager.WriteMemoryDWORD(0x429A88, index);
	// .text:0000000000429A8C                 lea     rax, [rsi+2E4h]
	g_HookManager.WriteMemoryDWORD(0x429A8F, indexEnd);
	// inline bool  CSharedCreatureData::ResetFlagJournal(int nQuestID , int nFlag)
	// .text:0000000000429C45                 lea     rdx, [rsi+1E4h]
	g_HookManager.WriteMemoryDWORD(0x429C48, index);
	// .text:0000000000429C4C                 lea     rax, [rsi+2E4h]
	g_HookManager.WriteMemoryDWORD(0x429C4F, indexEnd);
	// inline bool CSharedCreatureData::RemoveMemo(int nQuestID )
	// .text:0000000000429DFE                 lea     rax, [rdi+1E4h]
	g_HookManager.WriteMemoryDWORD(0x429E01, index);
	// .text:0000000000429E05                 lea     rcx, [rdi+2E4h]
	g_HookManager.WriteMemoryDWORD(0x429E08, indexEnd);
	// inline bool  CSharedCreatureData::HaveMemo(int nQuestID)
	// .text:0000000000431C60                 lea     rdx, [rdi+1E4h]
	g_HookManager.WriteMemoryDWORD(0x431C63, index);
	// .text:0000000000431C6A                 lea     r8, [rdi+2E4h]
	g_HookManager.WriteMemoryDWORD(0x431C6D, indexEnd);
	// inline bool  CSharedCreatureData::SetMemoState(int nQuestID , int nSlot, int nState)
	// .text:0000000000431F27                 lea     rbx, [r13+1E4h]
	g_HookManager.WriteMemoryDWORD(0x431F2A, index);
	// .text:0000000000431F31                 lea     rdi, [r13+2E4h]
	g_HookManager.WriteMemoryDWORD(0x431F34, indexEnd);
	// bool CSCharacterInfoPacket(CSocket* pSocket, const PUCHAR packet)
	// .text:000000000047B08A                 mov     eax, [rax+22Ch]
	g_HookManager.WriteMemoryDWORD(0x47B08C, index + 0x48);
	// .text:000000000047B097                 mov     eax, [rcx+228h]
	g_HookManager.WriteMemoryDWORD(0x47B099, index + 0x44);
	// .text:000000000047B0A4                 mov     eax, [rdx+224h]
	g_HookManager.WriteMemoryDWORD(0x47B0A6, index + 0x40);
	
	// .text:000000000047B0AE                 mov     eax, [rdi+21Ch]
	g_HookManager.WriteMemoryDWORD(0x47B0B0, index + 0x38);
	// .text:000000000047B0B8                 mov     eax, [r8+218h]
	g_HookManager.WriteMemoryDWORD(0x47B0BB, index + 0x34);
	// .text:000000000047B0C3                 mov     eax, [r9+214h]
	g_HookManager.WriteMemoryDWORD(0x47B0C6, index + 0x30);
	// .text:000000000047B0CE                 mov     eax, [r10+20Ch]
	g_HookManager.WriteMemoryDWORD(0x47B0D1, index + 0x28);
	// .text:000000000047B0D9                 mov     eax, [r11+208h]
	g_HookManager.WriteMemoryDWORD(0x47B0DC, index + 0x24);
	// .text:000000000047B0E4                 mov     eax, [rbx+204h]
	g_HookManager.WriteMemoryDWORD(0x47B0E6, index + 0x20);
	// .text:000000000047B0EE                 mov     eax, [rsi+1FCh]
	g_HookManager.WriteMemoryDWORD(0x47B0F0, index + 0x18);
	// .text:000000000047B0F8                 mov     eax, [rbp+1F8h]
	g_HookManager.WriteMemoryDWORD(0x47B0FA, index + 0x14);
	// .text:000000000047B102                 mov     eax, [r12+1F4h]
	g_HookManager.WriteMemoryDWORD(0x47B106, index + 0x10);
	// .text:000000000047B10E                 mov     eax, [r13+1ECh]
	g_HookManager.WriteMemoryDWORD(0x47B111, index + 0x08);
	// .text:000000000047B119                 mov     eax, [r14+1E8h]
	g_HookManager.WriteMemoryDWORD(0x47B11C, index + 0x04);
	// .text:000000000047B124                 mov     r9d, [r15+1E4h]
	g_HookManager.WriteMemoryDWORD(0x47B127, index + 0x00);
	// void g_DB.SaveQuestInfo(User *pUser)
	// .text:0000000000538980                 mov     eax, [rbx+r8+1F0h]
	g_HookManager.WriteMemoryDWORD(0x538984, index + 0x0C);
	// .text:000000000053898C                 mov     eax, [rbx+r8+1ECh]
	g_HookManager.WriteMemoryDWORD(0x538990, index + 0x08);
	// .text:0000000000538998                 mov     eax, [rbx+r8+1E8h]
	g_HookManager.WriteMemoryDWORD(0x53899C, index + 0x04);
	// .text:00000000005389A4                 mov     r9d, [rbx+r8+1E4h]
	g_HookManager.WriteMemoryDWORD(0x5389A8, index + 0x00);
	// .text:00000000005389C2                 cmp     rbx, 100h
	g_HookManager.WriteMemoryDWORD(0x5389C5, countMem);
	// static bool LoadQuestPacket(CDBSocket *socket, const PUCHAR packet)
	// .text:0000000000552840                 mov     edi, 1F0h
	g_HookManager.WriteMemoryDWORD(0x552841, index + 0x0C);
	// .text:0000000000552848                 lea     ebp, [rbx+10h]
	g_HookManager.WriteMemoryBYTE(0x552848, 0xBD);
	g_HookManager.WriteMemoryDWORD(0x552849, count);
	g_HookManager.NopMemory(0x55284D, 2);
	// .text:0000000000552857                 lea     rax, [rbx+rdx+1F0h]
	g_HookManager.WriteMemoryDWORD(0x55285B, index + 0x0C);
	// .text:000000000055285F                 lea     rcx, [rbx+rdx+1ECh]
	g_HookManager.WriteMemoryDWORD(0x552863, index + 0x08);
	// .text:0000000000552867                 lea     r9, [rbx+rdx+1E8h]
	g_HookManager.WriteMemoryDWORD(0x55286B, index + 0x04);
	// .text:000000000055286F                 lea     r8, [rbx+rdx+1E4h]
	g_HookManager.WriteMemoryDWORD(0x552873, index + 0x00);
	// CSharedCreatureData::GetMemoState(int nQuestID, int nSlot) 
	// .text:00000000006D04FF                 lea     rax, [rbx+1E4h]
	g_HookManager.WriteMemoryDWORD(0x6D0502, index);
	// .text:00000000006D0506                 lea     rcx, [rbx+2E4h]
	g_HookManager.WriteMemoryDWORD(0x6D0509, indexEnd);
	// void User::SendQuestList()
	// quest list
	// .text:000000000081EBF8                 mov     r9d, [rbx+rax+1E4h]
	g_HookManager.WriteMemoryDWORD(0x81EBFC, index);
	// .text:000000000081EC00                 mov     r8d, [rbx+rax+1F0h]
	g_HookManager.WriteMemoryDWORD(0x81EC04, index+0x0C);
	// .text:000000000081EC0D                 mov     edx, 80h
	g_HookManager.WriteMemoryDWORD(0x81EC0E, countMemSmall);				// increase buffer, which is used for assembled packet with list of quests
	// .text:000000000081EC36                 cmp     rbx, 100h
	g_HookManager.WriteMemoryDWORD(0x81EC39, countMem);
	// move rest of the list
	// .text:000000000081EC43                 mov     [rsp+0DB8h+var_CC8], r15b
	g_HookManager.WriteMemoryDWORD(0x81EC47, 0x70 + countMemSmall);
	// .text:000000000081EC4D                 mov     r8d, 0C7Fh      ; size_t
	g_HookManager.WriteMemoryDWORD(0x81EC4F, 0xCFF - countMemSmall);
	// .text:000000000081EC53                 lea     rcx, [rsp+0DB8h+var_CC7]
	g_HookManager.WriteMemoryDWORD(0x81EC57, 0x71 + countMemSmall);
	// quest items list
	// .text:000000000081EDAB                 mov     edx, 0C80h
	g_HookManager.WriteMemoryDWORD(0x81EDAC, 0xD00-countMemSmall);		// whole original buffer 0xD00 (0xC80 + 0x80)
	// .text:000000000081EDB6                 lea     rcx, [rsp+rax+0DB8h+var_CC8]
	g_HookManager.WriteMemoryDWORD(0x81EDBA, 0x70 + countMemSmall);
	// CSocket->Send
	// .text:000000000081F012                 lea     rdx, [rsp+0DB8h+var_CC8]
	g_HookManager.WriteMemoryDWORD(0x81F016, 0x70 + countMemSmall);
	// .text:000000000081F059                 lea     rcx, [rsp+0DB8h+var_CC8]
	g_HookManager.WriteMemoryDWORD(0x81F05D, 0x70 + countMemSmall);
	// void User::OnChangeSubJobEnd(bool bCreate)
	// .text:0000000000855077                 lea     rax, [rbp+1E4h]
	g_HookManager.WriteMemoryDWORD(0x85507A, index);
	// .text:000000000085507E                 lea     rcx, [rbp+2E4h]
	g_HookManager.WriteMemoryDWORD(0x855081, indexEnd);
}


void CSharedFactory::CheckIndexesForCreature()
{
	CCreatureSharedFactory *g_SD = (CCreatureSharedFactory*)0xE4120A0;
	
	g_Log.Add(CLog::Blue, "var1[%d] var2[%d]", g_SD->firstIndex, g_SD->lastIndex);
	CreatureSharedData* pSD = (CreatureSharedData*)g_SD->lpCreatureData;
	for(int n=0;n<70000;n++)
	{
		g_Log.Add(CLog::Blue, "[%d] index[%d] index2[%d] addr[%p]", n, pSD[n].nSharedIndex, pSD[n].nNextIndex, pSD[n]);
	}
}



CreatureSharedData* CCreatureSharedFactory::Allocate(CCreature *pCreature)
{
	if(pCreature)
	{
		_lock.Lock(__FILEW__, __LINE__);
		int freeIndex = firstIndex;
		
		if(freeIndex == -1)
		{
			g_Log.Add(CLog::Error, "[%s] Shared Memory runout!", __FUNCTION__);
			CRASH_MACRO;
			
		}

		if(freeIndex < 0 || freeIndex >= (*lpCreatureDataCount))
		{
			g_Log.Add(CLog::Error, "[%s] Shared Memory free index invalid [%d]", __FUNCTION__, freeIndex);
			CRASH_MACRO;
		}

		CreatureSharedData* pSD = &lpCreatureData[freeIndex];
		firstIndex = pSD->nNextIndex;





		if(firstIndex < 0 || firstIndex >= (*lpCreatureDataCount))
		{
			g_Log.Add(CLog::Error, "[%s] Shared Memory next index invalid [%d]", __FUNCTION__, firstIndex);

			pSD = pSD_Rescate[contarErrores];
			contarErrores++;

			g_Log.Add(CLog::Error, "[%s] Shared Memory next index invalid [%d] contarErrores[%d]", __FUNCTION__, firstIndex, contarErrores);

		}

//		if(firstIndex < 0 || firstIndex >= (*lpCreatureDataCount))
//		{
//			g_Log.Add(CLog::Error, "[%s] Shared Memory next index invalid [%d]", __FUNCTION__, firstIndex);
//			CRASH_MACRO;
//		}

		memset(pSD, 0, sizeof(CreatureSharedData));

		CreatureSharedData* pNextSD = &lpCreatureData[firstIndex];
		pNextSD->nSharedIndex = -1;
		lpCreatureIndex[freeIndex] = pCreature->nObjectID;
		pSD->nSharedIndex = -1;
		pSD->nNextIndex = -1;
		pSD->nServerIndex = freeIndex;

		int lockIndex = freeIndex & 8191;
		if(lockIndex >= 0)
		{
			CIOCriticalSection* lpLocks = (*(CIOCriticalSection**)((INT64)lpCenteredLock + (INT64)(((INT64)lockIndex) * 8)));
			pCreature->lpSharedDataLock = lpLocks;
			addedCount++;
			remainCount--;
		}else
		{
			g_Log.Add(CLog::Error, "[%s] Invalid Lock Index[%d]!", __FUNCTION__, lockIndex);
			CRASH_MACRO;
		}		
		_lock.Unlock();

		return pSD;
	}

	return NULL;
}



BOOL CCreatureSharedFactory::Free(int serverIndex)
{
	_lock.Lock(__FILEW__, __LINE__);
	if(serverIndex > -1)
	{
		lpCreatureIndex[serverIndex] = -1;
		CreatureSharedData* pCurrentSD = &lpCreatureData[serverIndex];
		CreatureSharedData* pLastSD = &lpCreatureData[lastIndex];
		pCurrentSD->nNextIndex = pLastSD->nNextIndex;
		pLastSD->nNextIndex = serverIndex;
		pLastSD->nSharedIndex = lastIndex;
		lastIndex = serverIndex;
		removedCount++;
		remainCount++;
	}else
		g_Log.Add(CLog::Error, "[%s] Invalid index[%d]", __FUNCTION__, serverIndex);
	_lock.Unlock();

	return TRUE;
}



/*

// Parámetros de espera (podés tunearlos)
static const DWORD kAllocWaitMs  = 3000;   // cuánto esperamos por vez
static HANDLE gFreeSlotsSem      = NULL;   // semáforo de “slots libres”

// --------------------------------------------------------------------------
// Checker seguro (no lee 70000 fijo)
void CSharedFactory::CheckIndexesForCreature()
{
    CCreatureSharedFactory *f = g_CreatureSharedFactory;
    if (!f) return;

    const int total = *lpCreatureDataCount;
    g_Log.Add(CLog::Blue, "firstIndex[%d] lastIndex[%d] capacity[%d] remain[%d]",
              f->firstIndex, f->lastIndex, total, f->remainCount);

    CreatureSharedData* pSD = f->lpCreatureData;
    for (int n = 0; n < total; ++n)
    {
        g_Log.Add(CLog::Blue, "[%d] shared[%d] next[%d] addr[%p]",
                  n, pSD[n].nSharedIndex, pSD[n].nNextIndex, &pSD[n]);
    }
}

// Crear el semáforo una sola vez.
// Máximo = capacidad actual, Conteo inicial = remainCount actual.
static void InitFreeSlotsSemaphore(CCreatureSharedFactory* f)
{
    if (gFreeSlotsSem) return;
    LONG maximum = (LONG)(*lpCreatureDataCount);
    LONG initial = (f->remainCount > 0) ? (LONG)f->remainCount : (LONG)(*lpCreatureDataCount);
    // si remainCount aún no está seteado, arranca con “capacidad”
    if (maximum < 1) maximum = 1;
    if (initial < 0) initial = 0;
    if (initial > maximum) initial = maximum;

    gFreeSlotsSem = CreateSemaphoreW(NULL, initial, maximum, NULL);
//    if (!gFreeSlotsSem) {
//        g_Log.Add(CLog::Error, L"[InitFreeSlotsSemaphore] CreateSemaphoreW failed: %u", GetLastError());
//    } else {
//        g_Log.Add(CLog::Blue, L"[InitFreeSlotsSemaphore] init=%d max=%d", (int)initial, (int)maximum);
//    }
}

// --------------------------------------------------------------------------
// Allocate/Free robustos con semáforo
CreatureSharedData* CCreatureSharedFactory::Allocate(CCreature *pCreature)
{
    if (!pCreature) return NULL;
    if (!gFreeSlotsSem) InitFreeSlotsSemaphore(this);

    for (;;)
    {
        // 1) Chequear la free-list bajo lock
        _lock.Lock(__FILEW__, __LINE__);
        const int total = *lpCreatureDataCount;
        int freeIndex = firstIndex;

        if (freeIndex >= 0 && freeIndex < total)
        {
            CreatureSharedData* pSD = &lpCreatureData[freeIndex];

            // pop head
            const int nextIndex = pSD->nNextIndex;
            firstIndex = nextIndex;
            if (firstIndex == -1) lastIndex = -1;

            ZeroMemory(pSD, sizeof(CreatureSharedData));
            pSD->nServerIndex = freeIndex;
            pSD->nSharedIndex = -1;
            pSD->nNextIndex   = -1;

            lpCreatureIndex[freeIndex] = pCreature->nObjectID;

            int lockIndex = (freeIndex & 8191);
            CIOCriticalSection* lpLocks =
                (*(CIOCriticalSection**)((INT64)lpCenteredLock + (INT64)(((INT64)lockIndex) * 8)));
            pCreature->lpSharedDataLock = lpLocks;

            addedCount++;
            if (remainCount > 0) remainCount--;

            _lock.Unlock();

            // Si usás semáforo, **no** hay que tocarlo aquí; el “permiso” lo consume el que esperó.
            return pSD;
        }

        // Lista vacía (o corrupta)
        _lock.Unlock();

        // 2) Esperar a que aparezca un slot libre
        if (gFreeSlotsSem) {
            DWORD wait = WaitForSingleObject(gFreeSlotsSem, kAllocWaitMs);
            if (wait != WAIT_OBJECT_0) {
                static DWORD lastLog = 0;
                DWORD now = GetTickCount();
                if (now - lastLog > 5000) { // throttle logs cada 5s
                    g_Log.Add(CLog::Error, "[%s] runout (cap=%d remain=%d) – sigo esperando",
                              __FUNCTION__, *lpCreatureDataCount, remainCount);
                    lastLog = now;
                }
                // volver a intentar
            }
        } else {
            Sleep(1); // fallback si no hay semáforo
        }
    }
}

BOOL CCreatureSharedFactory::Free(int serverIndex)
{
    if (!gFreeSlotsSem) InitFreeSlotsSemaphore(this);

    _lock.Lock(__FILEW__, __LINE__);

    const int total = *lpCreatureDataCount;
    if (serverIndex < 0 || serverIndex >= total) {
        g_Log.Add(CLog::Error, "[%s] Invalid index[%d] (total=%d)", __FUNCTION__, serverIndex, total);
        _lock.Unlock();
        return FALSE;
    }
    if (lpCreatureIndex[serverIndex] == -1) {
        g_Log.Add(CLog::Error, "[%s] Double free idx=%d", __FUNCTION__, serverIndex);
        _lock.Unlock();
        return FALSE;
    }

    lpCreatureIndex[serverIndex] = -1;

    CreatureSharedData* sd = &lpCreatureData[serverIndex];
    sd->nSharedIndex = -1;

    // push-front
    sd->nNextIndex = firstIndex;
    firstIndex = serverIndex;
    if (lastIndex == -1) lastIndex = serverIndex;

    removedCount++;
    remainCount++;

    _lock.Unlock();

    // Despertar a quien espera
    if (gFreeSlotsSem) ReleaseSemaphore(gFreeSlotsSem, 1, NULL);
    return TRUE;
}

*/