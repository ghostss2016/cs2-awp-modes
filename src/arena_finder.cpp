#include "main.h"
#include "arena_finder.h"
#include "arenas.h"
#include "arena.h"
#include "CBaseEntity.h"
#include <algorithm>
#include <cmath>

// Union-Find implementation
UnionFind::UnionFind(int n)
    : m_Parent(n)
    , m_Rank(n, 0)
{
    for (int i = 0; i < n; i++)
        m_Parent[i] = i;
}

int UnionFind::Find(int x)
{
    if (m_Parent[x] != x)
        m_Parent[x] = Find(m_Parent[x]);  // Path compression
    return m_Parent[x];
}

void UnionFind::Unite(int x, int y)
{
    int px = Find(x);
    int py = Find(y);
    if (px == py)
        return;

    // Union by rank
    if (m_Rank[px] < m_Rank[py])
        std::swap(px, py);
    m_Parent[py] = px;
    if (m_Rank[px] == m_Rank[py])
        m_Rank[px]++;
}

bool UnionFind::Connected(int x, int y)
{
    return Find(x) == Find(y);
}

float ArenaFinder::Distance(const Vector& a, const Vector& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

std::vector<ArenaSpawnPoint> ArenaFinder::GetMapSpawns()
{
    std::vector<ArenaSpawnPoint> spawns;
    spawns.reserve(64); // Pre-allocate to avoid reallocations

    if (!g_pGameEntitySystem)
    {
        META_CONPRINTF("[CS2AWPModes] ERROR: g_pGameEntitySystem is NULL!\n");
        return spawns;
    }

    // Use entity identity list to iterate through entities and check designer name
    CEntityIdentity* pEntity = g_pGameEntitySystem->m_EntityList.m_pFirstActiveEntity;
    int entitiesChecked = 0;
    int spawnEntitiesFound = 0;

    for (; pEntity; pEntity = pEntity->m_pNext)
    {
        entitiesChecked++;

        if (!pEntity->m_designerName.IsValid())
            continue;

        const char* designerName = pEntity->m_designerName.String();
        if (!designerName)
            continue;

        ArenaSpawnPoint spawn;
        spawn.ArenaID = -1;

        if (strcmp(designerName, "info_player_terrorist") == 0)
        {
            spawn.Team = TEAM_T;
            spawnEntitiesFound++;
        }
        else if (strcmp(designerName, "info_player_counterterrorist") == 0)
        {
            spawn.Team = TEAM_CT;
            spawnEntitiesFound++;
        }
        else
        {
            continue;
        }

        CEntityInstance* entity = pEntity->m_pInstance;
        if (!entity)
            continue;

        // Get position from entity using CBaseEntity accessors
        CBaseEntity* baseEntity = (CBaseEntity*)entity;
        if (!baseEntity)
            continue;

        // Double-check pointers to avoid crashes
        CBodyComponent* bodyComp = baseEntity->m_CBodyComponent();
        if (!bodyComp)
            continue;

        CGameSceneNode* sceneNode = bodyComp->m_pSceneNode();
        if (!sceneNode)
            continue;

        // Copy position data carefully
        Vector origin = sceneNode->m_vecAbsOrigin();
        QAngle angles = sceneNode->m_angAbsRotation();

        // Validate data before adding
        if (origin.x == 0 && origin.y == 0 && origin.z == 0)
            continue; // Skip invalid spawn

        spawn.Origin = origin;
        spawn.Angles = angles;

        spawns.push_back(spawn);
    }

    META_CONPRINTF("[CS2AWPModes] Checked %d entities, found %d spawn entities, added %d spawns\n",
        entitiesChecked, spawnEntitiesFound, (int)spawns.size());

    return spawns;
}

std::vector<std::pair<std::vector<ArenaSpawnPoint>, std::vector<ArenaSpawnPoint>>>
ArenaFinder::ClusterSpawns(const std::vector<ArenaSpawnPoint>& spawns, float maxDistance)
{
    std::vector<std::pair<std::vector<ArenaSpawnPoint>, std::vector<ArenaSpawnPoint>>> result;

    if (spawns.empty())
        return result;

    // Separate spawns by team
    std::vector<ArenaSpawnPoint> tSpawns, ctSpawns;
    for (const auto& spawn : spawns)
    {
        if (spawn.Team == TEAM_T)
            tSpawns.push_back(spawn);
        else if (spawn.Team == TEAM_CT)
            ctSpawns.push_back(spawn);
    }

    if (tSpawns.empty() || ctSpawns.empty())
        return result;

    // Use Union-Find to cluster nearby spawns
    int totalSpawns = tSpawns.size() + ctSpawns.size();
    UnionFind uf(totalSpawns);

    // Build distance matrix and unite nearby spawns
    std::vector<ArenaSpawnPoint> allSpawns;
    allSpawns.insert(allSpawns.end(), tSpawns.begin(), tSpawns.end());
    allSpawns.insert(allSpawns.end(), ctSpawns.begin(), ctSpawns.end());

    for (int i = 0; i < totalSpawns; i++)
    {
        for (int j = i + 1; j < totalSpawns; j++)
        {
            float dist = Distance(allSpawns[i].Origin, allSpawns[j].Origin);
            if (dist < maxDistance)
            {
                uf.Unite(i, j);
            }
        }
    }

    // Group spawns by cluster
    std::map<int, std::vector<int>> clusters;
    for (int i = 0; i < totalSpawns; i++)
    {
        int root = uf.Find(i);
        clusters[root].push_back(i);
    }

    // For each cluster, separate T and CT spawns
    for (auto& pair : clusters)
    {
        std::vector<ArenaSpawnPoint> clusterT, clusterCT;

        for (int idx : pair.second)
        {
            if (idx < (int)tSpawns.size())
                clusterT.push_back(allSpawns[idx]);
            else
                clusterCT.push_back(allSpawns[idx]);
        }

        // Only add if we have at least one spawn per team
        if (!clusterT.empty() && !clusterCT.empty())
        {
            result.push_back({clusterT, clusterCT});
        }
    }

    return result;
}

// Access map name from events.cpp
extern char g_szCurrentMap[256];

int ArenaFinder::FindArenas(Arenas* arenas)
{
    if (!arenas)
        return 0;

    // Clear existing arenas
    arenas->ClearArenas();

    // Get map spawns
    std::vector<ArenaSpawnPoint> spawns = GetMapSpawns();

    // Count CT and T spawns
    int ctCount = 0, tCount = 0;
    for (const auto& spawn : spawns)
    {
        if (spawn.Team == TEAM_CT)
            ctCount++;
        else if (spawn.Team == TEAM_T)
            tCount++;
    }

    const char* mapName = g_szCurrentMap[0] ? g_szCurrentMap : "unknown";

    if (spawns.empty() || ctCount == 0 || tCount == 0)
    {
        META_CONPRINTF("[CS2AWPModes] No spawn points detected on map: %s\n", mapName);
        return 0;
    }

    META_CONPRINTF("[CS2AWPModes] Detected %d CT spawns and %d T spawns on map %s\n", ctCount, tCount, mapName);

    // Cluster spawns into arena pairs
    auto clusters = ClusterSpawns(spawns, DEFAULT_MAX_DISTANCE);

    if (clusters.empty())
    {
        META_CONPRINTF("[CS2AWPModes] WARNING: No arenas were created. Players will not be able to spawn.\n");
        return 0;
    }

    // Create arenas from clusters
    int arenaID = 1;
    int maxPairSize = 0;

    for (auto& cluster : clusters)
    {
        Arena* arena = new Arena(arenaID);

        // Add T spawns to team 1
        for (auto& spawn : cluster.first)
        {
            spawn.ArenaID = arenaID;
            arena->AddSpawn(spawn, 1);
        }

        // Add CT spawns to team 2
        for (auto& spawn : cluster.second)
        {
            spawn.ArenaID = arenaID;
            arena->AddSpawn(spawn, 2);
        }

        int pairSize = std::min((int)cluster.first.size(), (int)cluster.second.size());
        if (pairSize > maxPairSize)
            maxPairSize = pairSize;

        META_CONPRINTF("[CS2AWPModes] Arena %d: CT Spawns: %d, T Spawns: %d\n",
            arenaID, (int)cluster.second.size(), (int)cluster.first.size());

        arenas->AddArena(arena);
        arenaID++;
    }

    int arenaCount = arenas->Count();
    META_CONPRINTF("[CS2AWPModes] Successfully setup %d arena(s) on map %s!\n", arenaCount, mapName);

    if (maxPairSize > 1)
        META_CONPRINTF("[CS2AWPModes] Supported arena modes: 1v1-%dv%d\n", maxPairSize, maxPairSize);
    else
        META_CONPRINTF("[CS2AWPModes] Supported arena modes: 1v1\n");

    return arenaCount;
}
