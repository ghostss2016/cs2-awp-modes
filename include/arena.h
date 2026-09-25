#ifndef _CS2_AWP_MODES_ARENA_H_
#define _CS2_AWP_MODES_ARENA_H_

#include "main.h"
#include "arena_player.h"
#include "round_type.h"
#include <vector>

// Special arena IDs (matching K4-Arenas)
#define ARENA_ID_WARMUP     -1
#define ARENA_ID_CHALLENGE  -2

// Spawn point structure
struct ArenaSpawnPoint
{
    Vector Origin;
    QAngle Angles;
    CsTeam Team;
    int ArenaID;
};

// Arena result enum
enum class ArenaResult
{
    None,
    Team1Win,
    Team2Win,
    Draw
};

// Arena class (matching K4-Arenas ArenaModel.cs)
class Arena
{
public:
    Arena(int arenaID);
    ~Arena();

    // Arena ID
    int GetArenaID() const { return m_iArenaID; }
    bool IsWarmupArena() const { return m_iArenaID == ARENA_ID_WARMUP; }
    bool IsChallengeArena() const { return m_iArenaID == ARENA_ID_CHALLENGE; }
    bool IsSpecialArena() const { return m_iArenaID < 0; }

    // Teams
    std::vector<ArenaPlayer*>& GetTeam1() { return m_Team1; }
    std::vector<ArenaPlayer*>& GetTeam2() { return m_Team2; }
    const std::vector<ArenaPlayer*>& GetTeam1() const { return m_Team1; }
    const std::vector<ArenaPlayer*>& GetTeam2() const { return m_Team2; }

    void SetTeam1(const std::vector<ArenaPlayer*>& team);
    void SetTeam2(const std::vector<ArenaPlayer*>& team);
    void ClearTeams();

    // Spawn points
    std::vector<ArenaSpawnPoint>& GetSpawnsTeam1() { return m_SpawnsTeam1; }
    std::vector<ArenaSpawnPoint>& GetSpawnsTeam2() { return m_SpawnsTeam2; }
    void AddSpawn(const ArenaSpawnPoint& spawn, int team);

    // Round type
    RoundType* GetRoundType() const { return m_pRoundType; }
    void SetRoundType(RoundType* roundType) { m_pRoundType = roundType; }

    // Round state
    bool HasFinished() const { return m_bHasFinished; }
    void SetFinished(bool finished) { m_bHasFinished = finished; }

    bool HasRealPlayers() const;
    int GetAliveCount(int team) const;

    // Result
    ArenaResult GetResult() const { return m_Result; }
    void SetResult(ArenaResult result) { m_Result = result; }

    // Find player in this arena
    ArenaPlayer* FindPlayer(CCSPlayerController* controller);
    ArenaPlayer* FindPlayer(int slot);

    // Get opponent team for a player
    std::vector<ArenaPlayer*>* GetOpponents(ArenaPlayer* player);

    // Get team for a player (1 or 2, 0 if not found)
    int GetPlayerTeam(ArenaPlayer* player) const;

    // Setup round (teleport, give weapons, etc)
    void SetupRound();

    // Handle round end
    void HandleRoundEnd();

    // Teleport player to spawn
    void TeleportToSpawn(ArenaPlayer* player, int team, int index);

private:
    int m_iArenaID;

    std::vector<ArenaPlayer*> m_Team1;
    std::vector<ArenaPlayer*> m_Team2;

    std::vector<ArenaSpawnPoint> m_SpawnsTeam1;
    std::vector<ArenaSpawnPoint> m_SpawnsTeam2;

    RoundType* m_pRoundType;
    bool m_bHasFinished;
    ArenaResult m_Result;
};

#endif // _CS2_AWP_MODES_ARENA_H_
