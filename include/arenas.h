#ifndef _CS2_AWP_MODES_AWP_MODES_H_
#define _CS2_AWP_MODES_AWP_MODES_H_

#include "main.h"
#include "arena.h"
#include <vector>
#include <map>

// Challenge model
struct ChallengeModel
{
    ArenaPlayer* Player1;
    ArenaPlayer* Player2;
    int Player1Placement;  // Queue position before challenge
    int Player2Placement;
    Arena* ChallengeArena;

    ChallengeModel(ArenaPlayer* p1, ArenaPlayer* p2, int place1, int place2)
        : Player1(p1), Player2(p2), Player1Placement(place1), Player2Placement(place2),
          ChallengeArena(nullptr) {}
};

// Arenas manager class (matching K4-Arenas ArenasModel.cs)
class Arenas
{
public:
    Arenas();
    ~Arenas();

    // Arena list access
    std::vector<Arena*>& GetArenaList() { return m_ArenaList; }
    const std::vector<Arena*>& GetArenaList() const { return m_ArenaList; }

    // Get arena count
    int Count() const { return (int)m_ArenaList.size(); }

    // Get arena by ID
    Arena* GetArena(int arenaID);

    // Add/remove arenas
    void AddArena(Arena* arena);
    void RemoveArena(int arenaID);
    void ClearArenas();

    // Find player in any arena
    ArenaPlayer* FindPlayer(CCSPlayerController* controller);
    ArenaPlayer* FindPlayer(int slot);
    ArenaPlayer* FindPlayer(uint64_t steamID64);

    // Find opponents for a player
    std::vector<ArenaPlayer*>* FindOpponents(ArenaPlayer* player);

    // Get arena containing player
    Arena* GetPlayerArena(ArenaPlayer* player);
    int GetPlayerArenaID(ArenaPlayer* player);

    // All players tracking (slot -> ArenaPlayer)
    void AddPlayer(ArenaPlayer* player);
    void RemovePlayer(int slot);
    ArenaPlayer* GetPlayer(int slot);
    std::map<int, ArenaPlayer*>& GetAllPlayers() { return m_AllPlayers; }

    // Challenges
    std::vector<ChallengeModel>& GetChallenges() { return m_Challenges; }
    ChallengeModel* FindChallenge(ArenaPlayer* player);
    void AddChallenge(const ChallengeModel& challenge);
    void RemoveChallenge(ArenaPlayer* player);

    // Setup arenas for new round
    void SetupArenas();

    // Distribute players from queue to arenas
    void DistributePlayers(std::queue<ArenaPlayer*>& waitingQueue,
                          std::queue<ArenaPlayer*>& rankingQueue);

private:
    std::vector<Arena*> m_ArenaList;
    std::map<int, ArenaPlayer*> m_AllPlayers;
    std::vector<ChallengeModel> m_Challenges;
};

#endif // _CS2_AWP_MODES_AWP_MODES_H_
