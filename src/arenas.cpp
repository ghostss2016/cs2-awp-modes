#include "arenas.h"
#include "config.h"
#include "round_type.h"
#include <algorithm>
#include <random>

extern ArenasConfig g_Config;
extern std::queue<ArenaPlayer*> g_WaitingPlayers;
extern std::queue<ArenaPlayer*> g_RankingQueue;

Arenas::Arenas()
{
}

Arenas::~Arenas()
{
    ClearArenas();
}

Arena* Arenas::GetArena(int arenaID)
{
    for (auto* arena : m_ArenaList)
    {
        if (arena && arena->GetArenaID() == arenaID)
            return arena;
    }
    return nullptr;
}

void Arenas::AddArena(Arena* arena)
{
    if (arena)
        m_ArenaList.push_back(arena);
}

void Arenas::RemoveArena(int arenaID)
{
    m_ArenaList.erase(
        std::remove_if(m_ArenaList.begin(), m_ArenaList.end(),
            [arenaID](Arena* arena) {
                if (arena && arena->GetArenaID() == arenaID)
                {
                    delete arena;
                    return true;
                }
                return false;
            }),
        m_ArenaList.end()
    );
}

void Arenas::ClearArenas()
{
    for (auto* arena : m_ArenaList)
    {
        if (arena)
            delete arena;
    }
    m_ArenaList.clear();
}

ArenaPlayer* Arenas::FindPlayer(CCSPlayerController* controller)
{
    if (!controller)
        return nullptr;

    for (auto* arena : m_ArenaList)
    {
        if (!arena)
            continue;
        ArenaPlayer* player = arena->FindPlayer(controller);
        if (player)
            return player;
    }

    // Check all players map
    int slot = GetPlayerSlot(controller);
    auto it = m_AllPlayers.find(slot);
    if (it != m_AllPlayers.end())
        return it->second;

    return nullptr;
}

ArenaPlayer* Arenas::FindPlayer(int slot)
{
    auto it = m_AllPlayers.find(slot);
    if (it != m_AllPlayers.end())
        return it->second;

    for (auto* arena : m_ArenaList)
    {
        if (!arena)
            continue;
        ArenaPlayer* player = arena->FindPlayer(slot);
        if (player)
            return player;
    }
    return nullptr;
}

ArenaPlayer* Arenas::FindPlayer(uint64_t steamID64)
{
    for (auto& pair : m_AllPlayers)
    {
        if (pair.second && pair.second->GetSteamID64() == steamID64)
            return pair.second;
    }
    return nullptr;
}

std::vector<ArenaPlayer*>* Arenas::FindOpponents(ArenaPlayer* player)
{
    if (!player)
        return nullptr;

    Arena* arena = GetPlayerArena(player);
    if (arena)
        return arena->GetOpponents(player);

    return nullptr;
}

Arena* Arenas::GetPlayerArena(ArenaPlayer* player)
{
    if (!player)
        return nullptr;

    for (auto* arena : m_ArenaList)
    {
        if (!arena)
            continue;
        if (arena->FindPlayer(player->GetSlot()))
            return arena;
    }
    return nullptr;
}

int Arenas::GetPlayerArenaID(ArenaPlayer* player)
{
    Arena* arena = GetPlayerArena(player);
    return arena ? arena->GetArenaID() : -1;
}

void Arenas::AddPlayer(ArenaPlayer* player)
{
    if (player)
        m_AllPlayers[player->GetSlot()] = player;
}

void Arenas::RemovePlayer(int slot)
{
    auto it = m_AllPlayers.find(slot);
    if (it != m_AllPlayers.end())
    {
        // Don't delete here, handled elsewhere
        m_AllPlayers.erase(it);
    }
}

ArenaPlayer* Arenas::GetPlayer(int slot)
{
    auto it = m_AllPlayers.find(slot);
    return (it != m_AllPlayers.end()) ? it->second : nullptr;
}

ChallengeModel* Arenas::FindChallenge(ArenaPlayer* player)
{
    if (!player)
        return nullptr;

    for (auto& challenge : m_Challenges)
    {
        if (challenge.Player1 == player || challenge.Player2 == player)
            return &challenge;
    }
    return nullptr;
}

void Arenas::AddChallenge(const ChallengeModel& challenge)
{
    m_Challenges.push_back(challenge);
}

void Arenas::RemoveChallenge(ArenaPlayer* player)
{
    m_Challenges.erase(
        std::remove_if(m_Challenges.begin(), m_Challenges.end(),
            [player](const ChallengeModel& c) {
                return c.Player1 == player || c.Player2 == player;
            }),
        m_Challenges.end()
    );
}

void Arenas::SetupArenas()
{
    // Clear finished state on all arenas
    for (auto* arena : m_ArenaList)
    {
        if (arena)
        {
            arena->ClearTeams();
            arena->SetFinished(false);
        }
    }

    // Ensure challenge arena exists (uses spawns from arena 1)
    if (!GetArena(ARENA_ID_CHALLENGE) && !m_ArenaList.empty())
    {
        Arena* firstArena = nullptr;
        for (auto* arena : m_ArenaList)
        {
            if (arena && arena->GetArenaID() >= 1)
            {
                firstArena = arena;
                break;
            }
        }

        if (firstArena)
        {
            Arena* challengeArena = new Arena(ARENA_ID_CHALLENGE);
            // Copy spawns from first arena
            for (auto& spawn : firstArena->GetSpawnsTeam1())
            {
                challengeArena->AddSpawn(spawn, 1);
            }
            for (auto& spawn : firstArena->GetSpawnsTeam2())
            {
                challengeArena->AddSpawn(spawn, 2);
            }
            m_ArenaList.push_back(challengeArena);
        }
    }
}

// Helper function to get common round type between two players/teams
static RoundType* GetCommonRoundType(const std::vector<RoundType*>& prefs1,
                                     const std::vector<RoundType*>& prefs2,
                                     bool allowMulti)
{
    std::vector<RoundType*> common;

    // Find intersection
    for (auto* rt1 : prefs1)
    {
        for (auto* rt2 : prefs2)
        {
            if (rt1 && rt2 && rt1->ID == rt2->ID)
            {
                // Skip multi-player rounds if not allowed
                if (!allowMulti && rt1->TeamSize > 1)
                    continue;
                common.push_back(rt1);
                break;
            }
        }
    }

    if (common.empty())
    {
        // Use default round type
        RoundType* defaultRound = RoundTypeManager::Instance().FindByName(g_Config.DefaultWeapons.DefaultRound.c_str());
        if (defaultRound)
            return defaultRound;

        // Fallback to first available
        auto& allTypes = RoundTypeManager::Instance().GetRoundTypes();
        for (auto& rt : allTypes)
        {
            if (rt.TeamSize == 1 || allowMulti)
                return &rt;
        }
        return nullptr;
    }

    // Random selection from common rounds
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, common.size() - 1);
    return common[dis(gen)];
}

void Arenas::DistributePlayers(std::queue<ArenaPlayer*>& waitingQueue,
                               std::queue<ArenaPlayer*>& rankingQueue)
{
    // First, handle challenges using the dedicated challenge arena
    Arena* challengeArena = GetArena(ARENA_ID_CHALLENGE);
    for (auto& challenge : m_Challenges)
    {
        if (!challenge.Player1 || !challenge.Player2)
            continue;
        if (!challenge.Player1->IsValid() || !challenge.Player2->IsValid())
            continue;

        // Use dedicated challenge arena (ID=-2)
        if (challengeArena && challengeArena->GetTeam1().empty())
        {
            // Set up challenge arena
            std::vector<ArenaPlayer*> team1 = {challenge.Player1};
            std::vector<ArenaPlayer*> team2 = {challenge.Player2};
            challengeArena->SetTeam1(team1);
            challengeArena->SetTeam2(team2);

            // Get common round type
            RoundType* roundType = GetCommonRoundType(
                challenge.Player1->GetRoundPreferences(),
                challenge.Player2->GetRoundPreferences(),
                false
            );
            challengeArena->SetRoundType(roundType);

            challenge.ChallengeArena = challengeArena;

            // Update player arena tags to show Challenge
            challenge.Player1->SetArenaTag("Challenge |");
            challenge.Player2->SetArenaTag("Challenge |");
        }
    }

    // Build sorted player list from both queues
    // rankingQueue players (have rank positions set) go in, then waitingQueue (losers/new)
    std::vector<ArenaPlayer*> sortedPlayers;

    // Drain ranking queue first
    while (!rankingQueue.empty())
    {
        ArenaPlayer* p = rankingQueue.front();
        rankingQueue.pop();
        if (p && p->IsValid() && !p->IsAFK() && !FindChallenge(p))
            sortedPlayers.push_back(p);
    }

    // Then waiting queue
    while (!waitingQueue.empty())
    {
        ArenaPlayer* p = waitingQueue.front();
        waitingQueue.pop();
        if (p && p->IsValid() && !p->IsAFK() && !FindChallenge(p))
            sortedPlayers.push_back(p);
    }

    // Assign rank positions if not set (new players get worst rank)
    for (auto* p : sortedPlayers)
    {
        if (p->GetRankPosition() <= 0)
            p->SetRankPosition(9999);
    }

    // Sort by rank position (lower = better = higher arena)
    std::sort(sortedPlayers.begin(), sortedPlayers.end(),
        [](ArenaPlayer* a, ArenaPlayer* b) {
            return a->GetRankPosition() < b->GetRankPosition();
        });

    // Distribute to arenas in pairs
    int arenaIndex = 0;
    int playerIndex = 0;

    while (playerIndex + 1 < (int)sortedPlayers.size() && arenaIndex < (int)m_ArenaList.size())
    {
        Arena* arena = m_ArenaList[arenaIndex];
        if (!arena || arena->IsSpecialArena() || !arena->GetTeam1().empty())
        {
            arenaIndex++;
            continue;
        }

        ArenaPlayer* player1 = sortedPlayers[playerIndex];
        ArenaPlayer* player2 = sortedPlayers[playerIndex + 1];

        // Assign rank positions based on arena
        player1->SetRankPosition(arenaIndex * 2 + 1);
        player2->SetRankPosition(arenaIndex * 2 + 2);

        std::vector<ArenaPlayer*> team1 = {player1};
        std::vector<ArenaPlayer*> team2 = {player2};
        arena->SetTeam1(team1);
        arena->SetTeam2(team2);

        RoundType* roundType = GetCommonRoundType(
            player1->GetRoundPreferences(),
            player2->GetRoundPreferences(),
            false
        );
        arena->SetRoundType(roundType);

        // Set arena tags
        char tag[64];
        snprintf(tag, sizeof(tag), "Arena %d |", arena->GetArenaID());
        player1->SetArenaTag(tag);
        player2->SetArenaTag(tag);

        playerIndex += 2;
        arenaIndex++;
    }

    // Remaining odd player goes back to waiting queue
    if (playerIndex < (int)sortedPlayers.size())
    {
        waitingQueue.push(sortedPlayers[playerIndex]);
    }
}
