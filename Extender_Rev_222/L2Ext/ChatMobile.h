#ifndef CHAT_MOBILE_H
#define CHAT_MOBILE_H

#include "StdAfx.h"
#include <winsock2.h>
#include <windows.h>
#include <deque>
#include <string>

#pragma comment(lib, "ws2_32.lib")

// ---------------------------------------------------------------------
// Logging opcional
// ---------------------------------------------------------------------
#ifndef LOGE
#  define LOGE(...)  do{/* no-op */}while(0)
#endif
#ifndef LOGI
#  define LOGI(...)  do{/* no-op */}while(0)
#endif

// ---------------------------------------------------------------------
// Wrapper x64 de Utils::BroadCastToAllUser(const char* fmt, ...)
// Dirección por defecto: la que mostraste (ajustá si difiere en tu build).
// Podés overridear con -DL2_BROADCAST_ADDR=0x0000000000XXXXXXXXui64
// ---------------------------------------------------------------------
#ifndef L2_BROADCAST_ADDR
#  define L2_BROADCAST_ADDR 0x0000000000822F40ui64
#endif

// ---------------------------------------------------------------------
// API
// ---------------------------------------------------------------------
class ChatMobile
{
public:
    ChatMobile();

    // Arranca el servidor TCP mono-conexión (acepta/recibe en hilos propios)
    void Init();                                     // crea listen + receiver threads
    void StartSingleConnectionServer(int port);      // bloquea en accept en su thread
    void Stop();                                     // cierra todo (opcional)

    // Enviar del juego -> proxy (eco hacia tu PHP)
    void EnviarMensajeJuegoAlProxy(const WCHAR* wszName, const WCHAR* msg, int nType);

    // Drenar cola y hacer broadcast dentro de un hilo del engine
    // LLAMAR periódicamente desde IOThread/Main/timer del server.
    void PumpFromEngine();

    // (Opcional) hook directo si querés inyectar desde engine sin cola
    void EnviarMensajeAppToServer_INTERNAL(const WCHAR* wszChar, const WCHAR* wszMsg);

    // Entrada desde el proxy (receiver thread): solo encola
    void EnqueueFromProxy(const WCHAR* name, const WCHAR* text);

    // Hilos
    static DWORD WINAPI ChatServerThread(LPVOID);
    static DWORD WINAPI ProxyReceiverThread(LPVOID);
private:


    // Helpers
    static void Utf8ToWideBounded(const char* srcUtf8, int srcLen, WCHAR* dst, int dstMax);
    static void WideToUtf8Send(SOCKET s, const WCHAR* wstr); // util usada por EnviarMensajeJuegoAlProxy

    // Estado sockets
    volatile LONG m_running;       // 0/1
};


// ---------------------------------------------------------------------
// Globals (compat con tu código existente)
// ---------------------------------------------------------------------
extern SOCKET g_clientSock;
extern ChatMobile g_ChatMobile;

#endif // CHAT_MOBILE_H
