#include "database.h"
#include "arena_player.h"
#include "round_type.h"
#include <sstream>

DatabaseManager& DatabaseManager::Instance() { static DatabaseManager i; return i; }

DatabaseManager::DatabaseManager() : m_pSQLInterface(nullptr), m_pMySQLClient(nullptr), m_pConnection(nullptr), m_bConnected(false) {}
DatabaseManager::~DatabaseManager() { Shutdown(); }

void DatabaseManager::SetSQLInterface(ISQLInterface* p) { m_pSQLInterface = p; if(p) m_pMySQLClient = p->GetMySQLClient(); }

bool DatabaseManager::Initialize(const DatabaseSettings& s) {
  if (!m_pMySQLClient) { META_CONPRINTF("[CS2AWPModes] SQL interface not available\n"); return false; }
  MySQLConnectionInfo info; info.host = s.Host.c_str(); info.user = s.Username.c_str();
  info.pass = s.Password.c_str(); info.database = s.Database.c_str(); info.port = s.Port; info.timeout = 60;
  m_pConnection = m_pMySQLClient->CreateMySQLConnection(info);
  if (!m_pConnection) { META_CONPRINTF("[CS2AWPModes] Failed to create connection\n"); return false; }
  m_TablePrefix = s.TablePrefix;
  m_pConnection->Connect([this](bool ok) { if(ok) { m_bConnected = true; CreateTables(); } });
  return true;
}

void DatabaseManager::Shutdown() { if(m_pConnection) { m_pConnection->Destroy(); m_pConnection = nullptr; } m_bConnected = false; }

void DatabaseManager::CreateTables() {
  if (!m_bConnected || !m_pConnection) return;
  std::stringstream ss;
  ss << "CREATE TABLE IF NOT EXISTS " << m_TablePrefix << "k4_arenas (steamid64 BIGINT UNIQUE, rifle INT, sniper INT, shotgun INT, smg INT, lmg INT, pistol INT, rounds VARCHAR(256), lastseen TIMESTAMP)";
  m_pConnection->Query(ss.str().c_str(), [](ISQLQuery*){});
}

void DatabaseManager::LoadPlayer(uint64_t sid, ArenaPlayer* p) {
  if (!m_bConnected || !p || !m_pConnection) return;
  std::string dr; auto& rt = RoundTypeManager::Instance().GetRoundTypes();
  for (auto& r : rt) { if(r.EnabledByDefault) { if(!dr.empty()) dr += ","; dr += std::to_string(r.ID); } }
  char q[512]; snprintf(q, 512, "INSERT INTO %sk4_arenas (steamid64, lastseen, rifle, sniper, shotgun, smg, lmg, pistol, rounds) VALUES (%lu, NOW(), %d, %d, %d, %d, %d, %d, '%s') ON DUPLICATE KEY UPDATE lastseen=NOW()", m_TablePrefix.c_str(), sid, 7, 9, 35, 19, 14, 4, dr.c_str());
  m_pConnection->Query(q, [](ISQLQuery*){});
  snprintf(q, 512, "SELECT rifle, sniper, shotgun, smg, lmg, pistol, rounds FROM %sk4_arenas WHERE steamid64=%lu", m_TablePrefix.c_str(), sid);
  m_pConnection->Query(q, [p](ISQLQuery* qr) {
    if(!qr || !p) return; auto r = qr->GetResultSet(); if(!r || !r->MoreRows()) return; r->FetchRow();
    if(!r->IsNull(0)) p->SetWeaponPreference(WeaponType::Rifle, (CsItem)r->GetInt(0));
    if(!r->IsNull(1)) p->SetWeaponPreference(WeaponType::Sniper, (CsItem)r->GetInt(1));
    if(!r->IsNull(2)) p->SetWeaponPreference(WeaponType::Shotgun, (CsItem)r->GetInt(2));
    if(!r->IsNull(3)) p->SetWeaponPreference(WeaponType::SMG, (CsItem)r->GetInt(3));
    if(!r->IsNull(4)) p->SetWeaponPreference(WeaponType::LMG, (CsItem)r->GetInt(4));
    if(!r->IsNull(5)) p->SetWeaponPreference(WeaponType::Pistol, (CsItem)r->GetInt(5));
    // Load round preferences from column 6 (rounds)
    if(!r->IsNull(6)) {
      const char* roundsStr = r->GetString(6);
      if(roundsStr && roundsStr[0]) {
        p->ClearRoundPreferences();
        std::string rounds(roundsStr);
        std::stringstream ss(rounds);
        std::string token;
        while(std::getline(ss, token, ',')) {
          if(!token.empty()) {
            int roundId = std::stoi(token);
            RoundType* rt = RoundTypeManager::Instance().FindByID(roundId);
            if(rt) p->AddRoundPreferenceNoSave(rt);
          }
        }
      }
    }
    p->SetLoaded(true);
  });
}

void DatabaseManager::SavePlayerWeaponPreference(uint64_t sid, WeaponType t, CsItem w) {
  if (!m_bConnected || !m_pConnection) return;
  const char* c = nullptr;
  switch(t) { case WeaponType::Rifle: c="rifle"; break; case WeaponType::Sniper: c="sniper"; break;
    case WeaponType::Shotgun: c="shotgun"; break; case WeaponType::SMG: c="smg"; break;
    case WeaponType::LMG: c="lmg"; break; case WeaponType::Pistol: c="pistol"; break; default: return; }
  char q[256]; snprintf(q, 256, "UPDATE %sk4_arenas SET %s=%d WHERE steamid64=%lu", m_TablePrefix.c_str(), c, (int)w, sid);
  m_pConnection->Query(q, [](ISQLQuery*){});
}

void DatabaseManager::SavePlayerRoundPreferences(uint64_t sid, const std::vector<int>& ids) {
  if (!m_bConnected || !m_pConnection) return;
  std::string r; for(int id : ids) { if(!r.empty()) r += ","; r += std::to_string(id); }
  char q[512]; snprintf(q, 512, "UPDATE %sk4_arenas SET rounds='%s' WHERE steamid64=%lu", m_TablePrefix.c_str(), r.c_str(), sid);
  m_pConnection->Query(q, [](ISQLQuery*){});
}

void DatabaseManager::PurgeOldData(int d) {
  if (!m_bConnected || !m_pConnection || d <= 0) return;
  char q[256]; snprintf(q, 256, "DELETE FROM %sk4_arenas WHERE lastseen < DATE_SUB(NOW(), INTERVAL %d DAY)", m_TablePrefix.c_str(), d);
  m_pConnection->Query(q, [](ISQLQuery*){});
}
