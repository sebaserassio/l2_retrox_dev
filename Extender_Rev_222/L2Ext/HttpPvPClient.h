#pragma once

// HttpPvPClient.h
#pragma once

#include <windows.h>
#include <wininet.h>
#include <deque>
#include <string>

#pragma comment(lib, "wininet.lib")

class HttpPvPClient {
public:
    // Constructor: inicializa cola y lanza el worker
    HttpPvPClient();
    // Destructor: detiene worker y libera recursos
    ~HttpPvPClient();


	BOOL GX_ENABLE;
    // URL base del endpoint HTTP
    wstring GX_BASE_DOMAIN;
    wstring GX_SERVER_KEY;
    wstring GX_SERVER_NAME;

    // Encola un evento PvP (killer, victim) para enviarlo al servidor HTTP
    void SendPvPEvent(const std::wstring& killer, const std::wstring& victim);

	void SendRegisterPlayerEvent(const std::wstring& name, const std::wstring& alias, const std::wstring& token);

	static bool CustomsBypass(User* pUser, wstring wBypass);


	static void Init();

private:


    HANDLE hThread_;               // Hilo único de envío
    HANDLE hEvent_;                // Evento para señalar nuevos datos
    CRITICAL_SECTION csQueue_;     // Protege acceso a la cola
    std::deque<std::wstring> queue_; // Cola de URLs en wide char
    volatile bool stop_;           // Señal de paro para el worker

    // Entry point estático para el hilo
    static DWORD WINAPI ThreadProc(LPVOID param);
    // Loop del worker que envía peticiones HTTP en cola
    DWORD worker();

};


extern HttpPvPClient httpSender;