#include "main.h"
#include "arena_player.h"
#include "config.h"
#include "weapon_model.h"
#include "database.h"
#include "arena_menus.h"
#include <CCSPlayerController.h>
#include <services.h>

extern ArenasConfig g_Config;

ArenaPlayer::ArenaPlayer(CCSPlayerController* controller)
    : m_pController(controller)
    , m_iSlot(-1)
    , m_uSteamID64(0)
    , m_bAFK(false)
    , m_bLoaded(false)
    , m_bPlayerIsSafe(false)
    , m_iChallengeTarget(-1)
    , m_iDamageDealt(0)
    , m_iRankPosition(0)
    , m_iWins(0)
    , m_iLosses(0)
    , m_iCurrentStreak(0)
    , m_iBestStreak(0)
    , m_iRoundKills(0)
    , m_iMapWins(0)
    , m_iMapLosses(0)
    , m_iMapKills(0)
    , m_iMapDeaths(0)
    , m_iTotalDamageDealt(0)
{
    if (controller)
    {
        m_iSlot = ::GetPlayerSlot(controller);
        m_uSteamID64 = ::GetSteamID64(controller);
    }

    // Initialize default weapon preferences
    m_WeaponPreferences[WeaponType::Rifle] = CsItem::AK47;
    m_WeaponPreferences[WeaponType::Sniper] = CsItem::AWP;
    m_WeaponPreferences[WeaponType::Shotgun] = CsItem::Nova;
    m_WeaponPreferences[WeaponType::SMG] = CsItem::MP9;
    m_WeaponPreferences[WeaponType::LMG] = CsItem::M249;
    m_WeaponPreferences[WeaponType::Pistol] = CsItem::Glock;

    // Initialize default round preferences (all enabled by default)
    auto& roundTypes = RoundTypeManager::Instance().GetRoundTypes();
    for (auto& rt : roundTypes)
    {
        if (rt.EnabledByDefault)
        {
            m_RoundPreferences.push_back(&rt);
        }
    }
}

ArenaPlayer::~ArenaPlayer()
{
}

bool ArenaPlayer::IsValid() const
{
    if (!m_pController)
        return false;

    // Check if player is still connected
    if (!g_pPlayers)
        return false;

    return g_pPlayers->IsConnected(m_iSlot);
}

void ArenaPlayer::SetArenaTag(const std::string& tag)
{
    m_ArenaTag = tag;

    // Update clan tag in game via schema (like VIP_Tag does)
    if (!g_Config.Compatibility.DisableClantags && m_pController && g_pUtils && IsValid())
    {
        // m_ArenaTag is a member so c_str() stays valid
        m_pController->m_szClan() = CUtlSymbolLarge(m_ArenaTag.c_str());
        g_pUtils->SetStateChanged((CBaseEntity*)m_pController, "CCSPlayerController", "m_szClan");
        META_CONPRINTF("[CS2AWPModes] SetArenaTag: slot=%d tag='%s'\n", m_iSlot, m_ArenaTag.c_str());
    }
}

void ArenaPlayer::SetAFK(bool afk)
{
    m_bAFK = afk;

    if (afk && m_pController)
    {
        // Move to spectator team
        // Using schema to change team
    }
}

void ArenaPlayer::SetWeaponPreference(WeaponType type, CsItem weapon)
{
    m_WeaponPreferences[type] = weapon;

    // Save to database if available
    if (DatabaseManager::Instance().IsAvailable() && m_uSteamID64 != 0)
    {
        DatabaseManager::Instance().SavePlayerWeaponPreference(m_uSteamID64, type, weapon);
    }
}

CsItem ArenaPlayer::GetWeaponPreference(WeaponType type) const
{
    auto it = m_WeaponPreferences.find(type);
    if (it != m_WeaponPreferences.end())
        return it->second;
    return CsItem::None;
}

void ArenaPlayer::AddRoundPreference(RoundType* roundType)
{
    if (!roundType)
        return;

    // Check if already exists
    for (auto* rt : m_RoundPreferences)
    {
        if (rt->ID == roundType->ID)
            return;
    }

    m_RoundPreferences.push_back(roundType);
    SaveRoundPreferencesToDB();
}

void ArenaPlayer::AddRoundPreferenceNoSave(RoundType* roundType)
{
    if (!roundType)
        return;

    // Check if already exists
    for (auto* rt : m_RoundPreferences)
    {
        if (rt->ID == roundType->ID)
            return;
    }

    m_RoundPreferences.push_back(roundType);
    // Don't save - used during DB loading
}

void ArenaPlayer::RemoveRoundPreference(RoundType* roundType)
{
    if (!roundType)
        return;

    m_RoundPreferences.erase(
        std::remove_if(m_RoundPreferences.begin(), m_RoundPreferences.end(),
            [roundType](RoundType* rt) { return rt->ID == roundType->ID; }),
        m_RoundPreferences.end()
    );
    SaveRoundPreferencesToDB();
}

bool ArenaPlayer::HasRoundPreference(RoundType* roundType) const
{
    if (!roundType)
        return false;

    for (auto* rt : m_RoundPreferences)
    {
        if (rt->ID == roundType->ID)
            return true;
    }
    return false;
}

void ArenaPlayer::ClearRoundPreferences()
{
    m_RoundPreferences.clear();
}

void ArenaPlayer::SaveRoundPreferencesToDB()
{
    if (!DatabaseManager::Instance().IsAvailable() || m_uSteamID64 == 0)
        return;

    std::vector<int> roundIDs;
    for (auto* rt : m_RoundPreferences)
    {
        roundIDs.push_back(rt->ID);
    }
    DatabaseManager::Instance().SavePlayerRoundPreferences(m_uSteamID64, roundIDs);
}

void ArenaPlayer::ResetRoundStats()
{
    m_iDamageDealt = 0;
    m_iRoundKills = 0;
}

void ArenaPlayer::RecordWin()
{
    m_iWins++;
    m_iMapWins++;
    m_iCurrentStreak++;
    if (m_iCurrentStreak > m_iBestStreak)
        m_iBestStreak = m_iCurrentStreak;
}

void ArenaPlayer::RecordLoss()
{
    m_iLosses++;
    m_iMapLosses++;
    m_iCurrentStreak = 0;
}

void ArenaPlayer::ResetMapStats()
{
    m_iMapWins = 0;
    m_iMapLosses = 0;
    m_iMapKills = 0;
    m_iMapDeaths = 0;
    m_iTotalDamageDealt = 0;
}

void ArenaPlayer::SwitchTeam(int team)
{
    if (!m_pController || !IsValid())
        return;

    // Use CCSPlayerController::ChangeTeam
    m_pController->ChangeTeam(team);
}

int ArenaPlayer::GetTeam() const
{
    if (!m_pController)
        return 0;

    // Validate controller is still valid using connected check
    if (!IsValid())
        return 0;

    return m_pController->m_iTeamNum();
}

void ArenaPlayer::GiveWeapon(CsItem weapon)
{
    if (!m_pController || weapon == CsItem::None)
        return;

    const char* weaponName = GetWeaponName(weapon);
    if (!weaponName || weaponName[0] == '\0')
        return;

    // Get pawn for item services
    CCSPlayerPawn* pawn = GetPlayerPawn(m_pController);
    if (!pawn)
        return;

    // Use schema-based CCSPlayer_ItemServices
    CCSPlayer_ItemServices* pItemServices = pawn->m_pItemServices();
    if (!pItemServices)
        return;

    pItemServices->GiveNamedItem(weaponName);
}

void ArenaPlayer::GiveArmor(bool helmet)
{
    if (!m_pController || !g_pUtils)
        return;

    CCSPlayerPawn* pawn = GetPlayerPawn(m_pController);
    if (!pawn)
        return;

    // Set armor value using schema
    pawn->m_ArmorValue() = 100;
    g_pUtils->SetStateChanged((CBaseEntity*)pawn, "CCSPlayerPawn", "m_ArmorValue");

    // Set helmet via item services using schema
    CCSPlayer_ItemServices* pItemServices = pawn->m_pItemServices();
    if (pItemServices)
    {
        pItemServices->m_bHasHelmet() = helmet;
        g_pUtils->SetStateChanged((CBaseEntity*)pawn, "CBasePlayerPawn", "m_pItemServices");
    }
}

void ArenaPlayer::StripWeapons()
{
    if (!m_pController || !g_pPlayers)
        return;

    // Use API to remove all weapons
    g_pPlayers->RemoveWeapons(m_iSlot);
}

void ArenaPlayer::SetHealth(int health)
{
    if (!m_pController || !g_pUtils)
        return;

    CCSPlayerPawn* pawn = GetPlayerPawn(m_pController);
    if (!pawn)
        return;

    // Set health
    int* pHealth = (int*)((uintptr_t)pawn + OFFSET_HEALTH);
    *pHealth = health;
    g_pUtils->SetStateChanged((CBaseEntity*)pawn, "CBaseEntity", "m_iHealth");
}

int ArenaPlayer::GetHealth() const
{
    if (!m_pController)
        return 0;

    CCSPlayerPawn* pawn = GetPlayerPawn(m_pController);
    if (!pawn)
        return 0;

    int* pHealth = (int*)((uintptr_t)pawn + OFFSET_HEALTH);
    return *pHealth;
}

bool ArenaPlayer::IsAlive() const
{
    return GetHealth() > 0;
}

void ArenaPlayer::Teleport(const Vector& position, const QAngle& angles)
{
    if (!m_pController || !g_pPlayers)
        return;

    Vector velocity(0, 0, 0);
    g_pPlayers->Teleport(m_iSlot, &position, &angles, &velocity);
}

void ArenaPlayer::ShowWeaponMenu(WeaponType type)
{
    ArenaMenuManager::Instance().ShowWeaponMenu(this, type);
}

void ArenaPlayer::ShowRoundMenu()
{
    ArenaMenuManager::Instance().ShowRoundMenu(this);
}

void ArenaPlayer::CloseMenu()
{
    ArenaMenuManager::Instance().CloseMenu(this);
}
