#include "stats_server.h"

IStatsServer::IStatsServer(CGameContext *pGameServer)
{
    pGameServer = m_pGameServer;
    m_Init = false;
}