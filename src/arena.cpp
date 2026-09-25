#include "arena.h"
#include "config.h"
#include "weapon_model.h"

extern ArenasConfig g_Config;

Arena::Arena(int arenaID)
    : m_iArenaID(arenaID)
    , m_pRoundType(nullptr)
    , m_bHasFinished(false)
    , m_Result(ArenaResult::None)
{
}

Arena::~Arena()
{
    ClearTeams();
}

void Arena::SetTeam1(const std::vector<ArenaPlayer*>& team)
{
    m_Team1 = team;
}

void Arena::SetTeam2(const std::vector<ArenaPlayer*>& team)
{
    m_Team2 = team;
}

void Arena::ClearTeams()
{
    m_Team1.clear();
    m_Team2.clear();
}

void Arena::AddSpawn(const ArenaSpawnPoint& spawn, int team)
{
    if (team == 1)
        m_SpawnsTeam1.push_back(spawn);
    else if (team == 2)
        m_SpawnsTeam2.push_back(spawn);
}

bool Arena::HasRealPlayers() const
{
    // Check if arena has any non-bot players
    for (auto* player : m_Team1)
    {
        if (player && player->IsValid())
        {
            CCSPlayerController* controller = player->GetController();
            if (controller && !g_pPlayers->IsFakeClient(player->GetSlot()))
                return true;
        }
    }
    for (auto* player : m_Team2)
    {
        if (player && player->IsValid())
        {
            CCSPlayerController* controller = player->GetController();
            if (controller && !g_pPlayers->IsFakeClient(player->GetSlot()))
                return true;
        }
    }
    return false;
}

int Arena::GetAliveCount(int team) const
{
    int count = 0;
    const std::vector<ArenaPlayer*>& players = (team == 1) ? m_Team1 : m_Team2;

    for (auto* player : players)
    {
        if (player && player->IsValid() && player->IsAlive())
        {
            count++;
        }
    }
    return count;
}

ArenaPlayer* Arena::FindPlayer(CCSPlayerController* controller)
{
    if (!controller)
        return nullptr;

    for (auto* player : m_Team1)
    {
        if (player && player->GetController() == controller)
            return player;
    }
    for (auto* player : m_Team2)
    {
        if (player && player->GetController() == controller)
            return player;
    }
    return nullptr;
}

ArenaPlayer* Arena::FindPlayer(int slot)
{
    for (auto* player : m_Team1)
    {
        if (player && player->GetSlot() == slot)
            return player;
    }
    for (auto* player : m_Team2)
    {
        if (player && player->GetSlot() == slot)
            return player;
    }
    return nullptr;
}

std::vector<ArenaPlayer*>* Arena::GetOpponents(ArenaPlayer* player)
{
    if (!player)
        return nullptr;

    // Check which team player is on
    for (auto* p : m_Team1)
    {
        if (p == player)
            return &m_Team2;
    }
    for (auto* p : m_Team2)
    {
        if (p == player)
            return &m_Team1;
    }
    return nullptr;
}

int Arena::GetPlayerTeam(ArenaPlayer* player) const
{
    if (!player)
        return 0;

    for (auto* p : m_Team1)
    {
        if (p == player)
            return 1;
    }
    for (auto* p : m_Team2)
    {
        if (p == player)
            return 2;
    }
    return 0;
}

void Arena::SetupRound()
{
    if (!m_pRoundType)
        return;

    // Call start function if special round
    if (m_pRoundType->StartFunction)
    {
        std::vector<CCSPlayerController*> team1Controllers, team2Controllers;
        for (auto* p : m_Team1)
            if (p && p->IsValid())
                team1Controllers.push_back(p->GetController());
        for (auto* p : m_Team2)
            if (p && p->IsValid())
                team2Controllers.push_back(p->GetController());

        m_pRoundType->StartFunction(&team1Controllers, &team2Controllers);
    }

    // Setup each player
    int spawnIndex1 = 0, spawnIndex2 = 0;

    for (auto* player : m_Team1)
    {
        if (!player || !player->IsValid())
            continue;

        player->ResetRoundStats();
        player->SetSafe(true);

        // Switch to T team (2)
        if (player->GetTeam() != 2)
            player->SwitchTeam(2);

        // Teleport to spawn
        if (spawnIndex1 < (int)m_SpawnsTeam1.size())
        {
            TeleportToSpawn(player, 1, spawnIndex1++);
        }

        // Strip weapons
        player->StripWeapons();

        // Give weapons based on round type
        if (m_pRoundType->PrimaryWeapon != CsItem::None)
        {
            player->GiveWeapon(m_pRoundType->PrimaryWeapon);
        }
        else if (m_pRoundType->UsePreferredPrimary)
        {
            CsItem preferred = player->GetWeaponPreference(m_pRoundType->PrimaryPreference);
            if (preferred != CsItem::None)
                player->GiveWeapon(preferred);
        }

        if (m_pRoundType->SecondaryWeapon != CsItem::None)
        {
            player->GiveWeapon(m_pRoundType->SecondaryWeapon);
        }
        else if (m_pRoundType->UsePreferredSecondary)
        {
            CsItem preferred = player->GetWeaponPreference(WeaponType::Pistol);
            if (preferred != CsItem::None)
                player->GiveWeapon(preferred);
        }

        // Give armor
        if (m_pRoundType->Armor)
        {
            player->GiveArmor(m_pRoundType->Helmet);
        }

        // Give knife
        player->GiveWeapon(CsItem::Knife);

        // Update arena tag
        char tag[64];
        snprintf(tag, sizeof(tag), "Arena %d |", m_iArenaID);
        player->SetArenaTag(tag);
    }

    // Same for team 2
    for (auto* player : m_Team2)
    {
        if (!player || !player->IsValid())
            continue;

        player->ResetRoundStats();
        player->SetSafe(true);

        // Switch to CT team (3)
        if (player->GetTeam() != 3)
            player->SwitchTeam(3);

        if (spawnIndex2 < (int)m_SpawnsTeam2.size())
        {
            TeleportToSpawn(player, 2, spawnIndex2++);
        }

        player->StripWeapons();

        if (m_pRoundType->PrimaryWeapon != CsItem::None)
        {
            player->GiveWeapon(m_pRoundType->PrimaryWeapon);
        }
        else if (m_pRoundType->UsePreferredPrimary)
        {
            CsItem preferred = player->GetWeaponPreference(m_pRoundType->PrimaryPreference);
            if (preferred != CsItem::None)
                player->GiveWeapon(preferred);
        }

        if (m_pRoundType->SecondaryWeapon != CsItem::None)
        {
            player->GiveWeapon(m_pRoundType->SecondaryWeapon);
        }
        else if (m_pRoundType->UsePreferredSecondary)
        {
            CsItem preferred = player->GetWeaponPreference(WeaponType::Pistol);
            if (preferred != CsItem::None)
                player->GiveWeapon(preferred);
        }

        if (m_pRoundType->Armor)
        {
            player->GiveArmor(m_pRoundType->Helmet);
        }

        player->GiveWeapon(CsItem::Knife);

        char tag[64];
        snprintf(tag, sizeof(tag), "Arena %d |", m_iArenaID);
        player->SetArenaTag(tag);
    }

    m_bHasFinished = false;
    m_Result = ArenaResult::None;
}

void Arena::HandleRoundEnd()
{
    if (m_bHasFinished)
        return;

    m_bHasFinished = true;

    // Call end function if special round
    if (m_pRoundType && m_pRoundType->EndFunction)
    {
        std::vector<CCSPlayerController*> team1Controllers, team2Controllers;
        for (auto* p : m_Team1)
            if (p && p->IsValid())
                team1Controllers.push_back(p->GetController());
        for (auto* p : m_Team2)
            if (p && p->IsValid())
                team2Controllers.push_back(p->GetController());

        m_pRoundType->EndFunction(&team1Controllers, &team2Controllers);
    }

    // Determine result based on alive players
    int alive1 = GetAliveCount(1);
    int alive2 = GetAliveCount(2);

    if (alive1 > alive2)
        m_Result = ArenaResult::Team1Win;
    else if (alive2 > alive1)
        m_Result = ArenaResult::Team2Win;
    else
        m_Result = ArenaResult::Draw;
}

void Arena::TeleportToSpawn(ArenaPlayer* player, int team, int index)
{
    if (!player || !player->IsValid())
        return;

    const std::vector<ArenaSpawnPoint>& spawns = (team == 1) ? m_SpawnsTeam1 : m_SpawnsTeam2;
    if (index < 0 || index >= (int)spawns.size())
        return;

    const ArenaSpawnPoint& spawn = spawns[index];

    // Teleport player using the new Teleport method
    player->Teleport(spawn.Origin, spawn.Angles);

    // Also set health to 100
    player->SetHealth(100);
}
