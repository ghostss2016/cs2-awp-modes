#ifndef _CS2_AWP_MODES_DATABASE_H_
#define _CS2_AWP_MODES_DATABASE_H_

#include "main.h"
#include "config.h"
#include "sql_mm.h"
#include "mysql_mm.h"
#include <functional>
#include <mutex>
#include <queue>

class ArenaPlayer;

class DatabaseManager
{
public:
    static DatabaseManager& Instance();
    void SetSQLInterface(ISQLInterface* pInterface);
    bool Initialize(const DatabaseSettings& settings);
    void Shutdown();
    bool IsAvailable() const { return m_bConnected; }
    void CreateTables();
    void LoadPlayer(uint64_t steamID64, ArenaPlayer* player);
    void SavePlayerWeaponPreference(uint64_t steamID64, WeaponType type, CsItem weapon);
    void SavePlayerRoundPreferences(uint64_t steamID64, const std::vector<int>& roundIDs);
    void PurgeOldData(int days);

private:
    DatabaseManager();
    ~DatabaseManager();

    ISQLInterface* m_pSQLInterface;
    IMySQLClient* m_pMySQLClient;
    IMySQLConnection* m_pConnection;
    bool m_bConnected;
    std::string m_TablePrefix;
};

#endif
