#pragma once

class CPvPRankEvent : public CThreadSafe
{
	enum EventState
	{
		StateDetenido    = 0,
		StateMensajeInit = 1,
		StateIniciado    = 2,
		StateMensajeFin  = 3,
		StateKickDelay   = 4   // <<< NUEVO
	};

    struct Evento
    {
        bool enabled;
        bool dayFlag[7];
        INT32 startHour;
        INT32 startMinute;
        INT32 endHour;
        INT32 endMinute;
        EventState estado;
        std::wstring m_AnnounceInitMsg;
        std::wstring m_AnnounceEndMsg;
        INT32 m_AnnounceInitMsgId;
        INT32 m_AnnounceEndMsgId;

        // (opcional si ya lo tienes)
        std::wstring m_EventName;
        IVector posTeleport[8]; // si ya existe, conserva tu definición real
    };

    // --- Nuevos tipos auxiliares ---
    struct Prize
    {
        UINT itemId;
        INT64 count;
    };

    // Comparador case-insensitive para nombres (L2 trata nombres case-insensitive)
    struct WLessInsensitive
    {
        bool operator()(const std::wstring& a, const std::wstring& b) const
        {
            return _wcsicmp(a.c_str(), b.c_str()) < 0;
        }
    };

private:
    BOOL m_enabled;
    std::list<Evento*> m_eventInfo;
    void LoadINI();
    Evento* m_lpEvent;

	time_t m_kickAt;     // instante (epoch) en que debemos patear
	IVector m_KickOutLoc; // loc para KickOutFromRange
	INT32 m_kx1, m_kx2, m_kz1, m_kz2; // bounds usados en KickOutFromRange

    // --- NUEVO: ranking y premios ---
    std::map<std::wstring, INT32, WLessInsensitive> m_scores; // nombre -> puntos
    std::vector<Prize> m_prizes;                               // premios por puesto (1..N)
    INT32 m_killPtsMin;                                        // ej. 1
    INT32 m_killPtsMax;                                        // ej. 3
    INT32 m_topN;                                              // ej. 50 (límite de podio)
    BOOL  m_rewardOnlineOnly;                                  // opcional: premiar solo si está online

    // Helpers
    void AddPointsForKill(User*, const wchar_t* killerName, INT32 pts);
    void AwardPrizesAndReset();
    void GiveRewardToName(const wchar_t* wszName, UINT itemId, INT64 count);
    void MakePodium(std::vector< std::pair<std::wstring, INT32> >& out); // top ordenado

public:
	BOOL IsActive();

    void Init();
    void TimerExpired();

    void EnterWorld(User* pUser);
	void SyncUserPoints(User* pUser);

    // --- NUEVO: llámalo desde tu hook de PvP kill ---
    void OnPvPKill(User* pKiller, User* pVictim);
};


extern CPvPRankEvent g_PvPRankEvent;