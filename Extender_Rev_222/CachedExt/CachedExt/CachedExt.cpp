#include "CachedExt.h"
#include "AdminPacketHandler.h"
#include "ManorFix.h"

void CachedExt::SetHook(BYTE OpCode, DWORD dwBaseAddr, DWORD dwTargetAddr, int nNops)
{
	HANDLE Server = OpenProcess(PROCESS_ALL_ACCESS | PROCESS_VM_READ | PROCESS_VM_WRITE, false, GetCurrentProcessId());
	if(Server)
	{
		DWORD dwOldProtect = NULL;
		SIZE_T BytesWritten = NULL;
		int nTempSize = 5+nNops;
		BYTE *buffer = new BYTE[nTempSize];
		if(nNops==0)
		{
			ZeroMemory(buffer,5);
			buffer[0]=OpCode;
			dwTargetAddr-=dwBaseAddr+5;
			memcpy(buffer+1,&dwTargetAddr,4);
		}else
		{
			ZeroMemory(buffer, nTempSize);
			buffer[0]=OpCode;
			dwTargetAddr-=dwBaseAddr+5;
			memcpy(buffer+1,&dwTargetAddr,4);
			for(int n=5; n < nTempSize; n++)
				buffer[n]=0x90;
		}

		if(!VirtualProtect((LPVOID)dwBaseAddr, (nTempSize), PAGE_EXECUTE_READWRITE, &dwOldProtect))
		{
			//Error while setting new protect
		}
		if (!WriteProcessMemory(Server, (LPVOID)(dwBaseAddr), buffer, (nTempSize), &BytesWritten))
		{
			ExitProcess(0);
		}
		if(!VirtualProtect((LPVOID)dwBaseAddr, (nTempSize), dwOldProtect, &dwOldProtect))
		{
			//Error while setting old protect
		}

		delete [] buffer;
	}
	CloseHandle(Server);
}

BOOL WINAPI ExitApp(HWND hWnd)
{
	DestroyWindow(hWnd);
	LPBOOL lP = 0;
	(*lP) = 0x666;
	return FALSE;
}

HANDLE hMutex = 0;

LONG WINAPI ExpFilter(EXCEPTION_POINTERS* pExp, DWORD dwExpCode)
{
	typedef LONG (__thiscall*f)(EXCEPTION_POINTERS*);
	LONG ret = f(0x431660L)(pExp);
	ExitProcess(dwExpCode);
	return EXCEPTION_EXECUTE_HANDLER;
}

LRESULT CALLBACK ExceptionHandlerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	__try
	{
		typedef LRESULT (CALLBACK*f)(HWND, UINT, WPARAM, LPARAM);
		return f(0x4229C0L)(hWnd, message, wParam, lParam);
	}__except(ExpFilter(GetExceptionInformation(), GetExceptionCode()))
	{
	}
	return 0;
}

int _cdecl ExceptionHandlerIOThread_Common(HANDLE hCompletionPort)
{
	__try
	{
		typedef int (_cdecl*f)(HANDLE);
		return f(0x441450L)(hCompletionPort);
	}__except(ExpFilter(GetExceptionInformation(), GetExceptionCode()))
	{
	}
	return 0;
}

int _cdecl ExceptionHandlerWaitThread(HANDLE hCompletionPort)
{
	__try
	{
		typedef int (_cdecl*f)(HANDLE);
		return f(0x4410F0L)(hCompletionPort);
	}__except(ExpFilter(GetExceptionInformation(), GetExceptionCode()))
	{
	}
	return 0;
}

UINT g_IODeadlockCheckerThreadId = 0;
UINT g_IODeadlockCheckerThreadIndex = 0;
CRITICAL_SECTION g_IODeadlockCheckerLock;

class CIODeadlockChecker
{
public:
	static void __thiscall TimerExpired(LPVOID param, int id)
	{
		EnterCriticalSection(&g_IODeadlockCheckerLock);
		//g_Log.Add(CLog::Blue, "[%s] [%d] [%d] [%d]", __FUNCTION__, GetThreadIndex(), GetCurrentThreadId(), GetTickCount());
		DWORD threadId = GetCurrentThreadId();
		if(threadId != g_IODeadlockCheckerThreadId)
		{
			UINT threadIndex = GetThreadIndex();
			LPUINT lpLastTick = (LPUINT)0x8FEBC0L;
			//g_Log.Add(CLog::Blue, "[IODeadlockChecker] Switching threadId[%d] -> [%d] index[%d] -> [%d] CurrentTick[%d] LastTick[%d]", g_IODeadlockCheckerThreadId, threadId, g_IODeadlockCheckerThreadIndex, threadIndex, (UINT)GetTickCount(), (*lpLastTick));
			g_IODeadlockCheckerThreadId = threadId;
			g_IODeadlockCheckerThreadIndex = threadIndex;
		}

		typedef void(__thiscall *f)(LPVOID, int);
		f(0x442250L)(param, id);
		LeaveCriticalSection(&g_IODeadlockCheckerLock);
	};
	static DWORD WINAPI GetDiff()
	{
		DWORD diff = 0;
		EnterCriticalSection(&g_IODeadlockCheckerLock);
		LPUINT lpLastTick = (LPUINT)0x8FEBC0L;
		UINT currentTick = GetTickCount();
		UINT lastTick = (*lpLastTick);
		if(currentTick < INT_MAX && lastTick > INT_MAX)
		{
			//rolled back to 0
			(*lpLastTick) = currentTick;
			lastTick = currentTick;
		}

		diff = currentTick - lastTick;
		LeaveCriticalSection(&g_IODeadlockCheckerLock);
	//	g_Log.Add(CLog::Blue, "[%s] Diff[%d] index[%d]", __FUNCTION__, diff, GetThreadIndex());
		return diff;
	};
};


void CachedExt::Initialize()
{
	DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX); SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);

	hMutex = CreateMutex(NULL, false, _T("Global\\CacheD"));
	if(GetLastError() == ERROR_ALREADY_EXISTS)
	{
		MessageBox(NULL, _T("CacheD is already running!"), _T("Interlude Extension"), MB_OK);
		ExitProcess(-1);
		return;
	}

	//Was Commented For some reason??? - BETA
	InitializeCriticalSection(&g_IODeadlockCheckerLock);
	WriteMemoryDWORD(0x00513FB8, (DWORD)CIODeadlockChecker::TimerExpired);
	SetHook(0xE8, 0x44121C, (DWORD)CIODeadlockChecker::GetDiff, 1);
	WriteMemoryDWORD(0x441229, 1200000); //80000
	WriteMemoryDWORD(0x43D780, 1200000); //FRED NEW
	WriteMemoryDWORD(0x43D8B1, 1200000); //FRED NEW


	WriteMemoryDWORD(0x00423171, (DWORD)ExceptionHandlerWndProc);
	SetHook(0xE8, 0x004416A1, (DWORD)ExceptionHandlerIOThread_Common, 0);
	SetHook(0xE8, 0x00441705, (DWORD)ExceptionHandlerIOThread_Common, 0);
	SetHook(0xE8, 0x00424E48, (DWORD)ExceptionHandlerWaitThread, 0);

	//Commented Here?
	//MovLogFiles(L"LinErrBak");
	Quests::Initialize();
	ClassExtender::Initialize();
	PacketHandler::Initialize();
	CAdminPacketHandler::Init();
	CManorFix::Init();
	SetHook(0xE8, 0x422C67, (DWORD)ExitApp, 1);


	///////HACER QUE LOS PRIVATES STORES COMPREN Y VENDAN POR OTRO ITEM DIFERENTE A LA ADENA/////
	//NUEVA FUNCION GETADENA MODIFICADA
	//CWareHouse__GetAdena
	//4B96C0 HASTA 004B9A09
	//el array de bytes es la funcion CWareHouse__GetAdena ya modificada con el arreglo para los items superiores a 255 o sea para un DWORD
	WriteMemoryBYTESS(0x0041B680, "6A FF 68 FB D5 4F 00 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 20 53 55 56 57 33 FF 8B F1 89 7C 24 20 F6 05 F4 DB F4 01 01 89 74 24 24 BD 01 00 00 00 75 1C 09 2D F4 DB F4 01 68 74 13 54 00 89 6C 24 3C E8 E6 5D FE FF 83 C4 04 A3 F0 DB F4 01 A1 B8 DD F4 01 64 8B 0D 2C 00 00 00 8B 1C 81 8B 83 08 00 00 00 8B 0C 85 78 A6 8E 00 8D 51 01 89 14 85 78 A6 8E 00 69 C0 E8 03 00 00 03 C1 C7 04 85 B0 4D 8D 00 74 13 54 00 8B 44 24 40 89 38 89 78 04 F6 05 F4 DB F4 01 02 89 7C 24 38 89 6C 24 20 75 1E 83 0D F4 DB F4 01 02 68 EC D4 53 00 C6 44 24 3C 03 E8 75 5D FE FF 83 C4 04 A3 EC DB F4 01 8B 83 08 00 00 00 8B 0C 85 78 A6 8E 00 8D 51 01 89 14 85 78 A6 8E 00 69 C0 E8 03 00 00 03 C1 C7 04 85 B0 4D 8D 00 EC D4 53 00 39 7E 60 C6 44 24 38 04 C6 44 24 13 01 89 7C 24 1C 0F 8E 18 02 00 00 8D 46 40 8D 6E 34 89 44 24 18 89 6C 24 14 EB 06 8B 6C 24 14 33 FF 80 7C 24 13 00 0F 84 F7 01 00 00 8B 4C 24 18 57 57 E8 C0 17 04 00 8B 45 04 8B 38 8B F5 89 7C 24 2C 89 74 24 28 90 85 F6 8B 6D 04 74 06 3B 74 24 14 74 12 6A 00 6A 00 6A 00 6A 00 6A 00 E8 99 BA 0B 00 83 C4 14 3B FD 0F 84 86 01 00 00 85 F6 75 0D 56 56 56 56 56 E8 80 BA 0B 00 83 C4 14 3B 7E 04 75 29 6A 00 6A 00 6A 00 6A 00 6A 00 E8 69 BA 0B 00 83 C4 14 3B 7E 04 75 12 6A 00 6A 00 6A 00 6A 00 6A 00 E8 52 BA 0B 00 83 C4 14 F6 05 F4 DB F4 01 04 8B 7F 10 75 7C 83 0D F4 DB F4 01 04 8B 0D 84 E7 8C 00 85 C9 C6 44 24 38 05 75 57 6A 10 E8 E5 BB 0B 00 8B F0 83 C4 04 85 F6 74 3D 68 6C 3E 50 00 68 F0 E1 2A 00 6A 00 6A 04 6A 00 6A FF FF 15 E4 30 50 00 6A 00 6A 00 6A 00 6A 02 50 89 06 FF 15 E0 30 50 00 89 46 04 05 E4 B9 2A 00 89 46 08 C7 46 0C 00 00 00 00 EB 02 33 F6 8B CE 89 0D 84 E7 8C 00 68 8C 7E 53 00 E8 77 5B FE FF A3 E8 DB F4 01 8B 83 08 00 00 00 8B 0C 85 78 A6 8E 00 8D 51 01 89 14 85 78 A6 8E 00 69 C0 E8 03 00 00 03 C1 C7 04 85 B0 4D 8D 00 8C 7E 53 00 8B CF E8 51 76 02 00 3D 53 25 00 00 90 90 90 74 2D 8B 83 08 00 00 00 83 04 85 78 A6 8E 00 FF 8D 4C 24 28 C6 44 24 38 04 E8 AB 79 04 00 8B 7C 24 2C 8B 74 24 28 8B 6C 24 14 E9 AA FE FF FF 8B 74 24 40 8B 0E 85 C9 74 13 8B 56 04 8B 01 8B 00 6A 00 52 6A 56 68 3C 3E 50 00 FF D0 85 FF 89 3E C7 46 04 1A 00 00 00 74 15 8B 17 8B 42 04 6A 1A 68 A7 00 00 00 68 3C 3E 50 00 8B CF FF D0 8B 83 08 00 00 00 83 04 85 78 A6 8E 00 FF C6 44 24 13 00 C6 44 24 38 04 8B 74 24 18 8B CE E8 58 18 04 00 8B 44 24 1C 8B 4C 24 24 83 44 24 14 0C 83 C0 01 83 C6 20 3B 41 60 89 44 24 1C 89 74 24 18 0F 8C F8 FD FF FF 8B 83 08 00 00 00 83 C9 FF 01 0C 85 78 A6 8E 00 01 0C 85 78 A6 8E 00 8B 4C 24 30 8B 44 24 40 5F 5E 5D 5B 64 89 0D 00 00 00 00 83 C4 2C C2 04 00 CC CC CC CC CC CC");
	// Pongo jumps hacia 0x0041B680 donde era adminHandlers__packet56_GetCharacters2Packet y ahora es GETADENA 

	WriteMemoryBYTESS(0x004C520C, "E8 6F 64 F5 FF");
	WriteMemoryBYTESS(0x004C590C, "E8 6F 5D F5 FF");
	WriteMemoryBYTESS(0x004C5B5C, "E8 1F 5B F5 FF");

	TCHAR File[MAX_PATH];
	GetCurrentDirectory(MAX_PATH,File);
	lstrcat(File, L"\\l2cache.ini");
	int item_id = GetPrivateProfileInt(_T("Mi_Custom"), _T("PrivateStore_ItemId"), 57, File);
	CachedExt::WriteMemoryDWORD(0x0041b8e0, item_id);
	/////////////////////////////////////////////////////////////////////////////////////////////


	int auctionDays = GetPrivateProfileInt(_T("Mi_Custom"), _T("AuctionAgitDays"), 7, File);
	CachedExt::WriteMemoryDWORD(0x486220+2, 86400 * auctionDays);
	CachedExt::WriteMemoryBYTE(0x486377+2, (BYTE)auctionDays);
	CachedExt::WriteMemoryBYTE(0x48637C+1, (BYTE)auctionDays);
	CachedExt::WriteMemoryBYTE(0x486381+1, (BYTE)auctionDays);
}

void CachedExt::WriteMemoryBYTES(DWORD Address, VOID *bytes, int len)
{
	DWORD OLDPROTECT;
	SIZE_T BW;
	HANDLE Server = OpenProcess(PROCESS_ALL_ACCESS | PROCESS_VM_READ | PROCESS_VM_WRITE, false, GetCurrentProcessId());
	if ( Server )
	{
		VirtualProtectEx(Server, (LPVOID)Address, len, PAGE_WRITECOPY, &OLDPROTECT);
		WriteProcessMemory(Server,(LPVOID)Address,bytes,len,&BW);
		VirtualProtectEx(Server, (LPVOID)Address, len, OLDPROTECT, &OLDPROTECT);
	}
	CloseHandle(Server);
}

void CachedExt::WriteMemoryQWORD(DWORD Address, INT64 value)     { WriteMemoryBYTES(Address, &value, 8); }
void CachedExt::WriteMemoryDWORD(DWORD Address, DWORD value)     { WriteMemoryBYTES(Address, &value, 4); }
void CachedExt::WriteMemoryWORD(DWORD Address, WORD value)      { WriteMemoryBYTES(Address, &value, 2); }
void CachedExt::WriteMemoryBYTE(DWORD Address, BYTE value)       { WriteMemoryBYTES(Address, &value, 1); }



void CachedExt::WriteMemoryBYTESS(uintptr_t addr, const void* data, size_t len)
{
	DWORD OLDPROTECT;
	SIZE_T BW;
	HANDLE Server = OpenProcess(MAXIMUM_ALLOWED, false, GetCurrentProcessId());		//con este funciona en el server 2003
	//HANDLE Server = OpenProcess(PROCESS_ALL_ACCESS | PROCESS_VM_READ | PROCESS_VM_WRITE, false, GetCurrentProcessId());
	if ( Server )
	{
		VirtualProtectEx(Server, (LPVOID)addr, len, PAGE_WRITECOPY, &OLDPROTECT);
		WriteProcessMemory(Server,(LPVOID)addr, data, len, &BW);
		VirtualProtectEx(Server, (LPVOID)addr, len, OLDPROTECT, &OLDPROTECT);
	}
	CloseHandle(Server);
}

void CachedExt::WriteMemoryBYTESS(uintptr_t addr, const char* text)
{
	std::basic_string<unsigned char> buf;
	char* end = 0;

	for (;; text = end)
	{
		unsigned long value = strtoul(text, &end, 16);
		if (text == end)
			break;
		buf.push_back((unsigned char)value);
	}

	if (buf.size() > 0)
	{
		WriteMemoryBYTESS(addr, buf.c_str(), buf.size());
	}
}

void CachedExt::WriteMemoryBYTESS(uintptr_t addr, size_t len, ...)
{
	if (len <= 0) return;

	BYTE* bytes = (BYTE*)alloca(len);
	va_list ap;
	va_start(ap, len);

	for (int i = 0; i < len; i++)
	{
		bytes[i] = va_arg(ap, BYTE);
	}
	va_end(ap);

	WriteMemoryBYTESS(addr, bytes, len);
}

