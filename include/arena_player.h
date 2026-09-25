#ifndef _CS2_AWP_MODES_ARENA_PLAYER_H_
#define _CS2_AWP_MODES_ARENA_PLAYER_H_

#include "main.h"
#include "round_type.h"
#include <map>
#include <vector>
#include <string>

class CCSPlayerController;
class Arena;

// Arena player class (matching K4-Arenas ArenaPlayerModel.cs)
class ArenaPlayer
{
public:
    ArenaPlayer(CCSPlayerController* controller);
    ~ArenaPlayer();

    // Validity check
    bool IsValid() const;

    // Controller access
    CCSPlayerController* GetController() const { return m_pController; }
    int GetSlot() const { return m_iSlot; }
    uint64_t GetSteamID64() const { return m_uSteamID64; }

    // Arena tag (shown in clan tag)
    const std::string& GetArenaTag() const { return m_ArenaTag; }
    void SetArenaTag(const std::string& tag);

    // AFK status
    bool IsAFK() const { return m_bAFK; }
    void SetAFK(bool afk);

    // Loaded from database
    bool IsLoaded() const { return m_bLoaded; }
    void SetLoaded(bool loaded) { m_bLoaded = loaded; }

    // Safety flag (prevent damage briefly after spawn)
    bool IsSafe() const { return m_bPlayerIsSafe; }
    void SetSafe(bool safe) { m_bPlayerIsSafe = safe; }

    // Center message (HUD)
    const std::string& GetCenterMessage() const { return m_CenterMessage; }
    void SetCenterMessage(const std::string& msg) { m_CenterMessage = msg; }

    // Weapon preferences
    std::map<WeaponType, CsItem>& GetWeaponPreferences() { return m_WeaponPreferences; }
    void SetWeaponPreference(WeaponType type, CsItem weapon);
    CsItem GetWeaponPreference(WeaponType type) const;

    // Round preferences
    std::vector<RoundType*>& GetRoundPreferences() { return m_RoundPreferences; }
    void AddRoundPreference(RoundType* roundType);
    void AddRoundPreferenceNoSave(RoundType* roundType);  // For DB loading
    void RemoveRoundPreference(RoundType* roundType);
    bool HasRoundPreference(RoundType* roundType) const;
    void ClearRoundPreferences();

    // Challenge data
    int GetChallengeTarget() const { return m_iChallengeTarget; }
    void SetChallengeTarget(int slot) { m_iChallengeTarget = slot; }

    // Player stats for current round
    void ResetRoundStats();
    void AddDamageDealt(int damage) { m_iDamageDealt += damage; m_iTotalDamageDealt += damage; }
    int GetDamageDealt() const { return m_iDamageDealt; }

    // Ranking
    int GetRankPosition() const { return m_iRankPosition; }
    void SetRankPosition(int rank) { m_iRankPosition = rank; }
    int GetWins() const { return m_iWins; }
    void SetWins(int wins) { m_iWins = wins; }
    int GetLosses() const { return m_iLosses; }
    void SetLosses(int losses) { m_iLosses = losses; }
    int GetCurrentStreak() const { return m_iCurrentStreak; }
    void SetCurrentStreak(int streak) { m_iCurrentStreak = streak; }
    int GetBestStreak() const { return m_iBestStreak; }
    void SetBestStreak(int streak) { m_iBestStreak = streak; }
    int GetRoundKills() const { return m_iRoundKills; }
    void SetRoundKills(int kills) { m_iRoundKills = kills; }
    int GetMapWins() const { return m_iMapWins; }
    void SetMapWins(int wins) { m_iMapWins = wins; }
    int GetMapLosses() const { return m_iMapLosses; }
    void SetMapLosses(int losses) { m_iMapLosses = losses; }
    int GetMapKills() const { return m_iMapKills; }
    void SetMapKills(int kills) { m_iMapKills = kills; }
    int GetMapDeaths() const { return m_iMapDeaths; }
    void SetMapDeaths(int deaths) { m_iMapDeaths = deaths; }
    int GetTotalDamageDealt() const { return m_iTotalDamageDealt; }
    void SetTotalDamageDealt(int damage) { m_iTotalDamageDealt = damage; }

    void AddRoundKill() { m_iRoundKills++; m_iMapKills++; }
    void AddMapDeath() { m_iMapDeaths++; }
    void RecordWin();
    void RecordLoss();
    void ResetMapStats();

    // Equipment handling
    void GiveWeapon(CsItem weapon);
    void GiveArmor(bool helmet);
    void StripWeapons();
    void SetHealth(int health);
    int GetHealth() const;
    bool IsAlive() const;

    // Movement
    void Teleport(const Vector& position, const QAngle& angles);

    // Team switching
    void SwitchTeam(int team);  // 2 = T, 3 = CT
    int GetTeam() const;

    // Menu handling (using cs2-menus-new)
    void ShowWeaponMenu(WeaponType type);
    void ShowRoundMenu();
    void CloseMenu();

private:
    void SaveRoundPreferencesToDB();

    CCSPlayerController* m_pController;
    int m_iSlot;
    uint64_t m_uSteamID64;

    std::string m_ArenaTag;
    bool m_bAFK;
    bool m_bLoaded;
    bool m_bPlayerIsSafe;
    std::string m_CenterMessage;

    std::map<WeaponType, CsItem> m_WeaponPreferences;
    std::vector<RoundType*> m_RoundPreferences;

    int m_iChallengeTarget;
    int m_iDamageDealt;

    // Ranking fields
    int m_iRankPosition;
    int m_iWins;
    int m_iLosses;
    int m_iCurrentStreak;
    int m_iBestStreak;
    int m_iRoundKills;
    int m_iMapWins;
    int m_iMapLosses;
    int m_iMapKills;
    int m_iMapDeaths;
    int m_iTotalDamageDealt;
};

#endif // _CS2_AWP_MODES_ARENA_PLAYER_H_
