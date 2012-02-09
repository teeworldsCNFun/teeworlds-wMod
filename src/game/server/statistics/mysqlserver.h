#ifndef GAME_SERVER_MYSQLSERVER_H
#define GAME_SERVER_MYSQLSERVER_H

#include <game/server/gamecontext.h>
#include "stats_server.h"

#include <mysql_connection.h>
#include <cppconn/driver.h>
/*#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>*/

class CSqlServer : public IStatsServer
{
public:
    CSqlServer(CGameContext *pGameServer);
    void OnInit();
    int GetId(const char* Name, const char* Password);
    int GetId(const char* Ip, const char* Pseudo, const char* Clan, const int Country);
    Player GetPlayer(int id);
    Stats GetStats(int id);
    Upgrade GetUpgrade(int id);
    Conf GetConf(int id);
    AllStats GetAll(int id);
    void WriteStats(int id, AllStats container);
private:
    bool Connect();
    void Disconnect();
    sql::Driver *m_pDriver;
    sql::Connection *m_pConnection;
    sql::Statement *m_pStatement;
};

#endif