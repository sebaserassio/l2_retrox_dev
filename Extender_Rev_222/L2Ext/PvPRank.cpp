#include "stdafx.h"
#include "PvPRank.h"
#include "ChatMobile.h"
#include <algorithm>
#include <vector>
#include <map>
#include <wchar.h>
#include <stdlib.h>
#include <time.h>

CPvPRankEvent g_PvPRankEvent;

static bool PvPRank_PodiumLess(
    const std::pair<std::wstring, INT32>& a,
    const std::pair<std::wstring, INT32>& b)
{
    if (a.second != b.second)
        return a.second > b.second;
    return _wcsicmp(a.first.c_str(), b.first.c_str()) < 0;
}

void CPvPRankEvent::Init()
{
  
}

void CPvPRankEvent::LoadINI()
{

}

void CPvPRankEvent::SyncUserPoints(User* pUser)
{
  
}

void CPvPRankEvent::EnterWorld(User* pUser)
{

}

void CPvPRankEvent::OnPvPKill(User* pKiller, User* pVictim)
{
 
}

void CPvPRankEvent::AddPointsForKill(User* killer, const wchar_t* killerName, INT32 pts)
{
 
}

void CPvPRankEvent::MakePodium(std::vector< std::pair<std::wstring, INT32> >& out)
{
 
}

void CPvPRankEvent::GiveRewardToName(const wchar_t* wszName, UINT itemId, INT64 count)
{

}

void CPvPRankEvent::AwardPrizesAndReset()
{
 
}

void CPvPRankEvent::TimerExpired()
{
 
}

BOOL CPvPRankEvent::IsActive()
{

 return false;
}