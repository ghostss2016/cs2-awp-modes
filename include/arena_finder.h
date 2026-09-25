#ifndef _CS2_AWP_MODES_ARENA_FINDER_H_
#define _CS2_AWP_MODES_ARENA_FINDER_H_

#include "main.h"
#include "arena.h"
#include <vector>
#include <map>

// Union-Find data structure for spawn pairing
class UnionFind
{
public:
    UnionFind(int n);

    int Find(int x);
    void Unite(int x, int y);
    bool Connected(int x, int y);

private:
    std::vector<int> m_Parent;
    std::vector<int> m_Rank;
};

// Arena finder class (matching K4-Arenas ArenaFinder.cs)
// Uses clustering algorithm to pair spawn points into arenas
class ArenaFinder
{
public:
    // Find and create arenas from map spawn points
    // Returns number of arenas found
    static int FindArenas(Arenas* arenas);

    // Get all spawn points on the map
    static std::vector<ArenaSpawnPoint> GetMapSpawns();

    // Cluster spawn points into arena pairs
    // Uses Union-Find algorithm to group nearby spawns
    static std::vector<std::pair<std::vector<ArenaSpawnPoint>, std::vector<ArenaSpawnPoint>>>
        ClusterSpawns(const std::vector<ArenaSpawnPoint>& spawns, float maxDistance);

    // Calculate distance between two points
    static float Distance(const Vector& a, const Vector& b);

    // Default max distance for spawn pairing
    static constexpr float DEFAULT_MAX_DISTANCE = 1500.0f;
};

#endif // _CS2_AWP_MODES_ARENA_FINDER_H_
