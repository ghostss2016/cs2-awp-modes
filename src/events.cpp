#include "events.h"
#include "common_gamedata.h"
#include "arena_player.h"
#include "arena.h"
#include "arenas.h"
#include "arena_finder.h"
#include "config.h"
#include "database.h"
#include "round_type.h"
#include <CBaseEntity.h>
#include <CCSPlayerController.h>
#include <CGameRules.h>
#include <dlfcn.h>
#include <cstring>
#include <algorithm>

// =============================================================================
// Pattern scanning for signature-based function finding
// =============================================================================
static void* FindPattern(void* base, size_t len, const char* pattern, const char* mask)
{
    if (!base || !pattern || !mask) return nullptr;
    size_t patternLen = strlen(mask);
    if (!patternLen || patternLen > len) return nullptr;
    uint8_t* pBase = (uint8_t*)base;

    for (size_t i = 0; i <= len - patternLen; i++)
    {
        bool found = true;
        for (size_t j = 0; j < patternLen; j++)
        {
            if (mask[j] == 'x' && pBase[i + j] != (uint8_t)pattern[j])
            {
                found = false;
                break;
            }
        }
        if (found)
            return &pBase[i];
    }
    return nullptr;
}

// =============================================================================
// CCSGameRules_TerminateRound - End the round
// =============================================================================

// CSRoundEndReason is defined in globaltypes.h (included via CGameRules.h)

// Function pointer type for CCSGameRules_TerminateRound
typedef void (*CCSGameRules_TerminateRound_t)(void*, float, int, void*, void*);

// Globals for TerminateRound
static CCSGameRules_TerminateRound_t g_pTerminateRound = nullptr;
static bool g_bTerminateRoundInitialized = false;
static bool g_bTerminateRoundInitFailed = false;

// Initialize CCSGameRules_TerminateRound
static bool InitTerminateRound()
{
    if (g_bTerminateRoundInitialized || g_bTerminateRoundInitFailed)
        return g_pTerminateRound != nullptr;

    g_bTerminateRoundInitialized = true;

    // Get server module
    void* serverHandle = dlopen("libserver.so", RTLD_NOW | RTLD_NOLOAD);
    if (!serverHandle)
        serverHandle = dlopen("csgo/bin/linuxsteamrt64/libserver.so", RTLD_NOW | RTLD_NOLOAD);

    if (!serverHandle)
    {
        META_CONPRINTF("[CS2AWPModes] TerminateRound: Failed to find libserver.so\n");
        g_bTerminateRoundInitFailed = true;
        return false;
    }

    // Get module base address
    Dl_info info;
    if (!dladdr(dlsym(serverHandle, "CreateInterface"), &info))
    {
        META_CONPRINTF("[CS2AWPModes] TerminateRound: Failed to get module info\n");
        dlclose(serverHandle);
        g_bTerminateRoundInitFailed = true;
        return false;
    }

    void* pBase = info.dli_fbase;
    size_t moduleSize = 100 * 1024 * 1024;  // 100MB search range

    // Signature for CCSGameRules_TerminateRound (Linux)
    const auto& signature = FleetGamedata::pattern("Plugins/Arenas/TerminateRound");
    const char* terminatePattern = signature.bytes.c_str();
    const char* terminateMask = signature.mask.c_str();

    uint8_t* terminatePtr = (uint8_t*)FindPattern(pBase, moduleSize, terminatePattern, terminateMask);
    if (!terminatePtr)
    {
        META_CONPRINTF("[CS2AWPModes] TerminateRound: Failed to find CCSGameRules_TerminateRound signature\n");
        dlclose(serverHandle);
        g_bTerminateRoundInitFailed = true;
        return false;
    }
    g_pTerminateRound = (CCSGameRules_TerminateRound_t)terminatePtr;

    META_CONPRINTF("[CS2AWPModes] TerminateRound: CCSGameRules_TerminateRound at %p\n", g_pTerminateRound);

    dlclose(serverHandle);
    return true;
}

// Get CCSGameRules pointer
static CCSGameRules* GetGameRulesInternal()
{
    if (!g_pEntitySystem) return nullptr;

    // Use UTIL_FindEntityByClassname from CBaseEntity.h (SchemaEntity)
    CEntityInstance* pProxy = UTIL_FindEntityByClassname("cs_gamerules");
    if (!pProxy) return nullptr;

    CCSGameRulesProxy* pGameRulesProxy = static_cast<CCSGameRulesProxy*>((CBaseEntity*)pProxy);
    return pGameRulesProxy->m_pGameRules();
}

// Terminate round with specified reason
static bool TerminateRound(CSRoundEndReason reason, float delay = 5.0f)
{
    if (!g_pTerminateRound)
    {
        META_CONPRINTF("[CS2AWPModes] TerminateRound: Not initialized, cannot terminate round!\n");
        return false;
    }

    CCSGameRules* pGameRules = GetGameRulesInternal();
    if (!pGameRules)
    {
        META_CONPRINTF("[CS2AWPModes] TerminateRound: GameRules not found!\n");
        return false;
    }

    g_pTerminateRound(pGameRules, delay, (int)reason, nullptr, nullptr);

    META_CONPRINTF("[CS2AWPModes] TerminateRound: Round terminated with reason %d\n", (int)reason);
    return true;
}

extern Arenas* g_pArenas;
extern std::queue<ArenaPlayer*> g_WaitingPlayers;
extern std::queue<ArenaPlayer*> g_RankingQueue;
extern ArenasConfig g_Config;
extern bool g_bIsBetweenRounds;
extern CGameRules* g_pGameRules;
extern std::map<int, ArenaPlayer*> g_PlayerSlots;

// Global waiting flag for arena detection (reset by MapStartHook)
bool g_bWaitingForArenas = false;
float g_flMapLoadTime = -1.0f;

ArenaEventListener* g_pEventListener = nullptr;

ArenaEventListener::ArenaEventListener()
    : m_bRegistered(false)
{
}

ArenaEventListener::~ArenaEventListener()
{
    UnregisterEvents();
}

void ArenaEventListener::RegisterEvents()
{
    if (m_bRegistered || !g_pGameEventManager)
        return;

    g_pGameEventManager->AddListener(this, "round_prestart", true);
    g_pGameEventManager->AddListener(this, "round_start", true);
    g_pGameEventManager->AddListener(this, "round_end", true);
    g_pGameEventManager->AddListener(this, "round_freeze_end", true);
    g_pGameEventManager->AddListener(this, "player_spawn", true);
    g_pGameEventManager->AddListener(this, "player_death", true);
    g_pGameEventManager->AddListener(this, "player_hurt", true);
    g_pGameEventManager->AddListener(this, "player_blind", true);
    g_pGameEventManager->AddListener(this, "player_disconnect", true);
    g_pGameEventManager->AddListener(this, "player_team", true);

    m_bRegistered = true;
}

void ArenaEventListener::UnregisterEvents()
{
    if (!m_bRegistered || !g_pGameEventManager)
        return;

    g_pGameEventManager->RemoveListener(this);
    m_bRegistered = false;
}

void ArenaEventListener::FireGameEvent(IGameEvent* event)
{
    if (!event)
        return;

    const char* eventName = event->GetName();
    if (!eventName)
        return;

    if (strcmp(eventName, "round_prestart") == 0)
        OnRoundPrestart(event);
    else if (strcmp(eventName, "round_start") == 0)
        OnRoundStart(event);
    else if (strcmp(eventName, "round_end") == 0)
        OnRoundEnd(event);
    else if (strcmp(eventName, "round_freeze_end") == 0)
        OnRoundFreezeEnd(event);
    else if (strcmp(eventName, "player_spawn") == 0)
        OnPlayerSpawn(event);
    else if (strcmp(eventName, "player_death") == 0)
        OnPlayerDeath(event);
    else if (strcmp(eventName, "player_hurt") == 0)
        OnPlayerHurt(event);
    else if (strcmp(eventName, "player_blind") == 0)
        OnPlayerBlind(event);
    else if (strcmp(eventName, "player_disconnect") == 0)
        OnPlayerDisconnect(event);
    else if (strcmp(eventName, "player_team") == 0)
        OnPlayerTeam(event);
}

// Track current map name to detect map changes (set by MapStartHook in main.cpp)
char g_szCurrentMap[256] = "";
bool g_bArenasDetected = false;

// Called BEFORE round_start - K4-Arenas uses this for player distribution
void ArenaEventListener::OnRoundPrestart(IGameEvent* event)
{
    if (!g_pArenas)
        return;

    // Map name is set by MapStartHook in main.cpp
    // Arena finding is done in OnGameFrame after MapStartHook sets g_szCurrentMap

    // Setup arenas
    g_pArenas->SetupArenas();

    // Distribute players from queues (happens here, not in round_start!)
    g_pArenas->DistributePlayers(g_WaitingPlayers, g_RankingQueue);
}

void ArenaEventListener::OnRoundStart(IGameEvent* event)
{
    g_bIsBetweenRounds = false;

    if (!g_pArenas)
        return;

    // Setup each arena (player distribution already happened in round_prestart)
    for (auto* arena : g_pArenas->GetArenaList())
    {
        if (arena && !arena->GetTeam1().empty() && !arena->GetTeam2().empty())
        {
            arena->SetupRound();

            // Helper lambda to show round info via center HTML
            auto showRoundInfo = [&](ArenaPlayer* player) {
                if (!player || !player->IsValid())
                    return;

                std::string html;

                // Arena info line
                if (arena->IsChallengeArena())
                    html += "<font color='#FFD700'>Challenge Match</font><br>";
                else
                    html += "<font color='#00BFFF'>Arena " + std::to_string(arena->GetArenaID()) + "</font><br>";

                // Opponent names
                if (!g_Config.Compatibility.DisableOpponentMessage)
                {
                    auto* opponents = arena->GetOpponents(player);
                    if (opponents && !opponents->empty())
                    {
                        std::string opponentNames;
                        for (auto* opp : *opponents)
                        {
                            if (opp && opp->IsValid() && g_pPlayers)
                            {
                                if (!opponentNames.empty())
                                    opponentNames += ", ";
                                const char* name = g_pPlayers->GetPlayerName(opp->GetSlot());
                                if (name)
                                    opponentNames += name;
                            }
                        }
                        html += "<font color='#FF4444'>vs " + opponentNames + "</font><br>";
                    }
                }

                // Round type
                if (!g_Config.Compatibility.DisableRoundTypeMessage && arena->GetRoundType())
                {
                    html += "<font color='#AAAAAA'>" + arena->GetRoundType()->Name + "</font>";
                }

                PrintToCenterHtml(player->GetController(), 5, "%s", html.c_str());
            };

            // Show info to all players in arena
            for (auto* player : arena->GetTeam1())
                showRoundInfo(player);
            for (auto* player : arena->GetTeam2())
                showRoundInfo(player);
        }
    }
}

void ArenaEventListener::OnRoundEnd(IGameEvent* event)
{
    g_bIsBetweenRounds = true;

    if (!g_pArenas)
        return;

    int totalArenas = (int)g_pArenas->GetArenaList().size();

    // Handle each arena's round end
    for (auto* arena : g_pArenas->GetArenaList())
    {
        if (arena)
        {
            arena->HandleRoundEnd();

            // Queue players for next round based on result
            ArenaResult result = arena->GetResult();

            // Calculate target arena number for announcements
            // Arena index in list corresponds roughly to arena number
            int currentArenaID = arena->GetArenaID();

            if (result == ArenaResult::Team1Win)
            {
                // Team1 wins
                for (auto* player : arena->GetTeam1())
                {
                    if (player && player->IsValid() && !player->IsAFK())
                    {
                        player->RecordWin();
                        int newRank = std::max(1, player->GetRankPosition() - 2);
                        player->SetRankPosition(newRank);
                        g_RankingQueue.push(player);

                        // Win announcement
                        int targetArena = std::max(1, currentArenaID - 1);
                        PrintToCenterHtml(player->GetController(), 5,
                            "<font color='#00FF00' size='5'>VICTORY!</font><br>"
                            "<font color='#FFD700'>Moving to Arena %d</font><br>"
                            "<font color='#AAAAAA'>Streak: %d</font>",
                            targetArena, player->GetCurrentStreak());

                        // Win streak announcement (server-wide for 5+)
                        if (player->GetCurrentStreak() >= 3 && g_pPlayers)
                        {
                            const char* playerName = g_pPlayers->GetPlayerName(player->GetSlot());
                            char streakMsg[256];
                            snprintf(streakMsg, sizeof(streakMsg),
                                " \x04[AWP Modes]\x01 %s is on a \x06%d win streak!\x01",
                                playerName ? playerName : "Unknown", player->GetCurrentStreak());

                            if (player->GetCurrentStreak() >= 5)
                                PrintToChatAll("%s", streakMsg);
                        }
                    }
                }
                // Team2 loses
                for (auto* player : arena->GetTeam2())
                {
                    if (player && player->IsValid() && !player->IsAFK())
                    {
                        // Check for streak break before recording loss
                        int oldStreak = player->GetCurrentStreak();
                        player->RecordLoss();
                        player->SetRankPosition(player->GetRankPosition() + 2);
                        g_RankingQueue.push(player);

                        // Loss announcement
                        int targetArena = std::min(totalArenas, currentArenaID + 1);
                        PrintToCenterHtml(player->GetController(), 5,
                            "<font color='#FF4444' size='5'>DEFEAT</font><br>"
                            "<font color='#AAAAAA'>Dropped to Arena %d</font>",
                            targetArena);

                        // Streak broken announcement
                        if (oldStreak >= 3 && !arena->GetTeam1().empty() && g_pPlayers)
                        {
                            ArenaPlayer* winner = arena->GetTeam1()[0];
                            if (winner && winner->IsValid())
                            {
                                const char* winnerName = g_pPlayers->GetPlayerName(winner->GetSlot());
                                const char* loserName = g_pPlayers->GetPlayerName(player->GetSlot());
                                PrintToChatAll(" \x04[AWP Modes]\x01 %s ended %s's \x06%d win streak!\x01",
                                    winnerName ? winnerName : "Unknown",
                                    loserName ? loserName : "Unknown", oldStreak);
                            }
                        }
                    }
                }
            }
            else if (result == ArenaResult::Team2Win)
            {
                // Team1 loses
                for (auto* player : arena->GetTeam1())
                {
                    if (player && player->IsValid() && !player->IsAFK())
                    {
                        int oldStreak = player->GetCurrentStreak();
                        player->RecordLoss();
                        player->SetRankPosition(player->GetRankPosition() + 2);
                        g_RankingQueue.push(player);

                        int targetArena = std::min(totalArenas, currentArenaID + 1);
                        PrintToCenterHtml(player->GetController(), 5,
                            "<font color='#FF4444' size='5'>DEFEAT</font><br>"
                            "<font color='#AAAAAA'>Dropped to Arena %d</font>",
                            targetArena);

                        // Streak broken announcement
                        if (oldStreak >= 3 && !arena->GetTeam2().empty() && g_pPlayers)
                        {
                            ArenaPlayer* winner = arena->GetTeam2()[0];
                            if (winner && winner->IsValid())
                            {
                                const char* winnerName = g_pPlayers->GetPlayerName(winner->GetSlot());
                                const char* loserName = g_pPlayers->GetPlayerName(player->GetSlot());
                                PrintToChatAll(" \x04[AWP Modes]\x01 %s ended %s's \x06%d win streak!\x01",
                                    winnerName ? winnerName : "Unknown",
                                    loserName ? loserName : "Unknown", oldStreak);
                            }
                        }
                    }
                }
                // Team2 wins
                for (auto* player : arena->GetTeam2())
                {
                    if (player && player->IsValid() && !player->IsAFK())
                    {
                        player->RecordWin();
                        int newRank = std::max(1, player->GetRankPosition() - 2);
                        player->SetRankPosition(newRank);
                        g_RankingQueue.push(player);

                        int targetArena = std::max(1, currentArenaID - 1);
                        PrintToCenterHtml(player->GetController(), 5,
                            "<font color='#00FF00' size='5'>VICTORY!</font><br>"
                            "<font color='#FFD700'>Moving to Arena %d</font><br>"
                            "<font color='#AAAAAA'>Streak: %d</font>",
                            targetArena, player->GetCurrentStreak());

                        if (player->GetCurrentStreak() >= 3 && g_pPlayers)
                        {
                            const char* playerName = g_pPlayers->GetPlayerName(player->GetSlot());
                            char streakMsg[256];
                            snprintf(streakMsg, sizeof(streakMsg),
                                " \x04[AWP Modes]\x01 %s is on a \x06%d win streak!\x01",
                                playerName ? playerName : "Unknown", player->GetCurrentStreak());

                            if (player->GetCurrentStreak() >= 5)
                                PrintToChatAll("%s", streakMsg);
                        }
                    }
                }
            }
            else  // Draw
            {
                for (auto* player : arena->GetTeam1())
                {
                    if (player && player->IsValid() && !player->IsAFK())
                        g_RankingQueue.push(player);
                }
                for (auto* player : arena->GetTeam2())
                {
                    if (player && player->IsValid() && !player->IsAFK())
                        g_RankingQueue.push(player);
                }
            }
        }
    }

    // Handle challenges
    auto& challenges = g_pArenas->GetChallenges();
    for (auto it = challenges.begin(); it != challenges.end(); )
    {
        if (it->ChallengeArena)
        {
            // Return players to their original queue positions
            // (simplified - actual implementation would restore exact positions)
            if (it->Player1 && it->Player1->IsValid())
                g_WaitingPlayers.push(it->Player1);
            if (it->Player2 && it->Player2->IsValid())
                g_WaitingPlayers.push(it->Player2);
            it = challenges.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void ArenaEventListener::OnRoundFreezeEnd(IGameEvent* event)
{
    // Remove player safety flags
    if (!g_pArenas)
        return;

    for (auto& pair : g_pArenas->GetAllPlayers())
    {
        if (pair.second)
            pair.second->SetSafe(false);
    }
}

void ArenaEventListener::OnPlayerSpawn(IGameEvent* event)
{
    int userid = event->GetInt("userid");
    if (!g_pPlayers)
        return;

    int slot = GetSlotFromUserId(userid);
    if (slot < 0)
        return;

    ArenaPlayer* player = g_pArenas ? g_pArenas->GetPlayer(slot) : nullptr;
    if (player)
    {
        player->SetSafe(true);
    }
}

void ArenaEventListener::OnPlayerDeath(IGameEvent* event)
{
    int victimId = event->GetInt("userid");
    int attackerId = event->GetInt("attacker");

    if (!g_pPlayers || !g_pArenas)
        return;

    int victimSlot = GetSlotFromUserId(victimId);
    int attackerSlot = GetSlotFromUserId(attackerId);

    ArenaPlayer* victim = g_pArenas->GetPlayer(victimSlot);
    ArenaPlayer* attacker = (attackerSlot >= 0) ? g_pArenas->GetPlayer(attackerSlot) : nullptr;

    if (!victim)
        return;

    // Track kills and deaths
    if (attacker)
    {
        attacker->AddRoundKill();
    }
    if (victim)
    {
        victim->AddMapDeath();
    }

    // Check if arena should end
    Arena* arena = g_pArenas->GetPlayerArena(victim);
    if (arena && !arena->HasFinished())
    {
        int playerTeam = arena->GetPlayerTeam(victim);
        int alive = arena->GetAliveCount(playerTeam);

        if (alive <= 0)
        {
            // Team eliminated
            arena->SetFinished(true);
            arena->SetResult(playerTeam == 1 ? ArenaResult::Team2Win : ArenaResult::Team1Win);

            // Check if all arenas finished
            bool allFinished = true;
            for (auto* a : g_pArenas->GetArenaList())
            {
                if (a && a->HasRealPlayers() && !a->HasFinished())
                {
                    allFinished = false;
                    break;
                }
            }

            if (allFinished)
            {
                // Initialize TerminateRound if not done yet
                if (!g_bTerminateRoundInitialized)
                    InitTerminateRound();

                // Terminate round - use Draw reason for arena mode
                // (doesn't matter which team wins since arenas handle scoring internally)
                TerminateRound(Draw, 0.1f);
            }
        }
    }
}

void ArenaEventListener::OnPlayerHurt(IGameEvent* event)
{
    int victimId = event->GetInt("userid");
    int attackerId = event->GetInt("attacker");
    int damage = event->GetInt("dmg_health");

    if (!g_pPlayers || !g_pArenas)
        return;

    int victimSlot = GetSlotFromUserId(victimId);
    int attackerSlot = GetSlotFromUserId(attackerId);

    ArenaPlayer* victim = (victimSlot >= 0) ? g_pArenas->GetPlayer(victimSlot) : nullptr;
    ArenaPlayer* attacker = (attackerSlot >= 0) ? g_pArenas->GetPlayer(attackerSlot) : nullptr;

    // Block damage from non-opponents (if enabled)
    if (g_Config.Compatibility.BlockDamageOfNotOpponent && victim && attacker)
    {
        Arena* victimArena = g_pArenas->GetPlayerArena(victim);
        Arena* attackerArena = g_pArenas->GetPlayerArena(attacker);

        // Different arenas or not opponents in same arena
        if (victimArena != attackerArena)
        {
            // Restore victim health (damage was already applied, so we compensate)
            if (victim->IsValid())
            {
                int currentHealth = victim->GetHealth();
                victim->SetHealth(currentHealth + damage);
            }
            return;
        }

        // Check if they're on the same team (shouldn't damage teammates)
        if (victimArena)
        {
            int victimTeam = victimArena->GetPlayerTeam(victim);
            int attackerTeam = victimArena->GetPlayerTeam(attacker);
            if (victimTeam == attackerTeam && victimTeam != 0)
            {
                if (victim->IsValid())
                {
                    int currentHealth = victim->GetHealth();
                    victim->SetHealth(currentHealth + damage);
                }
                return;
            }
        }
    }

    if (attacker)
    {
        attacker->AddDamageDealt(damage);
    }

    // Check for headshot-only round
    Arena* victimArena = (victim) ? g_pArenas->GetPlayerArena(victim) : nullptr;
    if (victimArena)
    {
        RoundType* rt = victimArena->GetRoundType();
        if (rt && rt->Name == "Headshot Only")
        {
            int hitgroup = event->GetInt("hitgroup");
            if (hitgroup != 1) // 1 = head
            {
                // Not a headshot - restore health
                if (victim && victim->IsValid())
                {
                    int currentHealth = victim->GetHealth();
                    victim->SetHealth(currentHealth + damage);
                }
            }
        }
    }

    // Show damage message using center HTML (K4-Arenas style)
    if (!g_Config.Compatibility.DisableDamageMessage && attacker && attacker->IsValid())
    {
        // Update player's center message with damage dealt
        int totalDamage = attacker->GetDamageDealt();
        int remainingHealth = victim ? victim->GetHealth() : 0;

        // Format: "Damage: X | Enemy HP: Y"
        PrintToCenterHtml(attacker->GetController(), 3,
            "<font color='#FF6600'>Damage: %d</font> | <font color='#66FF66'>Enemy HP: %d</font>",
            totalDamage, remainingHealth);
    }
}

// Block flash effect from players not in your arena
void ArenaEventListener::OnPlayerBlind(IGameEvent* event)
{
    if (!g_Config.Compatibility.BlockFlashOfNotOpponent)
        return;

    int victimId = event->GetInt("userid");
    int attackerId = event->GetInt("attacker");

    if (!g_pPlayers || !g_pArenas)
        return;

    int victimSlot = GetSlotFromUserId(victimId);
    int attackerSlot = GetSlotFromUserId(attackerId);

    ArenaPlayer* victim = (victimSlot >= 0) ? g_pArenas->GetPlayer(victimSlot) : nullptr;
    ArenaPlayer* attacker = (attackerSlot >= 0) ? g_pArenas->GetPlayer(attackerSlot) : nullptr;

    if (!victim || !attacker)
        return;

    Arena* victimArena = g_pArenas->GetPlayerArena(victim);
    Arena* attackerArena = g_pArenas->GetPlayerArena(attacker);

    // Different arenas - remove blind effect
    if (victimArena != attackerArena)
    {
        // Set flash duration to 0 via schema accessor
        CCSPlayerPawn* pawn = GetPlayerPawn(victim->GetController());
        if (pawn)
        {
            pawn->m_flFlashDuration() = 0.0f;
            if (g_pUtils)
                g_pUtils->SetStateChanged((CBaseEntity*)pawn, "CCSPlayerPawnBase", "m_flFlashDuration");
        }
    }
}

void ArenaEventListener::OnPlayerDisconnect(IGameEvent* event)
{
    int userid = event->GetInt("userid");
    if (!g_pPlayers)
        return;

    int slot = GetSlotFromUserId(userid);
    OnClientDisconnect(slot);
}

void ArenaEventListener::OnPlayerTeam(IGameEvent* event)
{
    // Intentionally empty: team changes are managed by Arena::SetupRound() which
    // assigns players to CT/T teams. We listen for this event to potentially block
    // unauthorized manual team switches in the future, but currently no action is needed
    // because the arena system overrides team assignments each round via SwitchTeam().
}

void InitializeEvents()
{
    if (g_pEventListener)
        return;

    g_pEventListener = new ArenaEventListener();
    g_pEventListener->RegisterEvents();
}

void ShutdownEvents()
{
    if (g_pEventListener)
    {
        delete g_pEventListener;
        g_pEventListener = nullptr;
    }
}

// Static variables for periodic updates
static float g_flLastClantagUpdate = 0.0f;

// Debug counter to avoid spamming
static int g_iDebugFrameCount = 0;

void OnGameFrame(bool simulating, bool bFirstTick, bool bLastTick)
{
    // Try to get globals if not set yet
    if (g_pUtils)
    {
        if (!gpGlobals)
        {
            gpGlobals = g_pUtils->GetCGlobalVars();
            if (gpGlobals)
                META_CONPRINTF("[CS2AWPModes] gpGlobals obtained from Utils API\n");
        }
        if (!g_pGameEntitySystem)
        {
            g_pGameEntitySystem = g_pUtils->GetCGameEntitySystem();
            if (g_pGameEntitySystem)
                META_CONPRINTF("[CS2AWPModes] g_pGameEntitySystem obtained from Utils API\n");
        }
        if (!g_pEntitySystem)
        {
            g_pEntitySystem = g_pUtils->GetCEntitySystem();
        }
    }

    // Debug output - only first frame
    static bool g_bFirstFrameLogged = false;
    if (!g_bFirstFrameLogged)
    {
        META_CONPRINTF("[CS2AWPModes] First GameFrame: gpGlobals=%p g_pArenas=%p g_pGameEntitySystem=%p\n",
            (void*)gpGlobals, (void*)g_pArenas, (void*)g_pGameEntitySystem);
        g_bFirstFrameLogged = true;
    }

    if (!gpGlobals)
        return;

    float curTime = gpGlobals->curtime;

    // Map name is set by MapStartHook in main.cpp (safe, no gpGlobals->mapname access)
    // Check if we need to find arenas (only once per map load)
    // Use frame counter instead of time to wait for entities
    static int g_iFrameCounter = 0;

    if (!g_bArenasDetected && g_pArenas && g_szCurrentMap[0])
    {
        if (!g_bWaitingForArenas)
        {
            g_bWaitingForArenas = true;
            g_iFrameCounter = 0;
            META_CONPRINTF("[CS2AWPModes] Map detected: %s, waiting for entities to initialize...\n", g_szCurrentMap);
        }
        else
        {
            g_iFrameCounter++;
            // Wait 300 frames (~5 seconds at 60fps) before scanning
            if (g_iFrameCounter >= 300)
            {
                g_bArenasDetected = true;
                g_bWaitingForArenas = false;
                META_CONPRINTF("[CS2AWPModes] Finding arenas on map %s (after %d frames)...\n", g_szCurrentMap, g_iFrameCounter);
                int arenaCount = ArenaFinder::FindArenas(g_pArenas);
                META_CONPRINTF("[CS2AWPModes] Found %d arenas on map %s\n", arenaCount, g_szCurrentMap);
            }
        }
    }

    // Process database callbacks
    // sql_mm handles callbacks internally

    // Force update clan tags every second (K4-Arenas behavior)
    if (!g_Config.Compatibility.DisableClantags && g_pArenas && curTime - g_flLastClantagUpdate >= 1.0f)
    {
        g_flLastClantagUpdate = curTime;

        for (auto& pair : g_pArenas->GetAllPlayers())
        {
            ArenaPlayer* player = pair.second;
            if (!player || !player->IsValid() || player->IsAFK())
                continue;

            // Get current arena
            Arena* arena = g_pArenas->GetPlayerArena(player);
            std::string newTag;

            if (arena)
            {
                if (arena->IsChallengeArena())
                    newTag = "Challenge |";
                else if (arena->IsWarmupArena())
                    newTag = "Warmup |";
                else
                    newTag = "Arena " + std::to_string(arena->GetArenaID()) + " |";
            }
            else
            {
                // Player in queue
                newTag = "Queue |";
            }

            // Update if changed
            if (player->GetArenaTag() != newTag)
            {
                player->SetArenaTag(newTag);
            }
        }
    }

    // Update player HUD (center HTML)
    if (g_pArenas && !g_bIsBetweenRounds)
    {
        for (auto& pair : g_pArenas->GetAllPlayers())
        {
            ArenaPlayer* player = pair.second;
            if (!player || !player->IsValid() || player->IsAFK())
                continue;

            Arena* arena = g_pArenas->GetPlayerArena(player);
            if (!arena || arena->HasFinished())
                continue;

            // Build HUD string
            std::string hud;

            // Arena number
            if (arena->IsChallengeArena())
                hud += "<font color='#FFD700'>Challenge Match</font>";
            else
                hud += "<font color='#00BFFF'>Arena " + std::to_string(arena->GetArenaID()) + "</font>";

            // Streak info
            if (player->GetCurrentStreak() >= 2)
                hud += " <font color='#FF6600'>" + std::to_string(player->GetCurrentStreak()) + " streak</font>";

            hud += "<br>";

            // Opponent info
            auto* opponents = arena->GetOpponents(player);
            if (opponents && !opponents->empty() && g_pPlayers)
            {
                for (auto* opp : *opponents)
                {
                    if (opp && opp->IsValid())
                    {
                        const char* oppName = g_pPlayers->GetPlayerName(opp->GetSlot());
                        int oppHp = opp->GetHealth();
                        hud += "<font color='#FF4444'>vs " + std::string(oppName ? oppName : "?") + "</font>";
                        if (oppHp > 0 && oppHp < 100)
                            hud += " <font color='#66FF66'>HP:" + std::to_string(oppHp) + "</font>";
                    }
                }
            }

            // Round type
            if (arena->GetRoundType())
                hud += "<br><font color='#AAAAAA'>" + arena->GetRoundType()->Name + "</font>";

            PrintToCenterHtml(player->GetController(), 1, "%s", hud.c_str());
        }
    }
}

void OnClientPutInServer(int slot)
{
    META_CONPRINTF("[CS2AWPModes] OnClientPutInServer: slot=%d\n", slot);

    if (!g_pArenas || !g_pPlayers)
    {
        META_CONPRINTF("[CS2AWPModes] OnClientPutInServer: g_pArenas=%p g_pPlayers=%p\n", (void*)g_pArenas, (void*)g_pPlayers);
        return;
    }

    CCSPlayerController* controller = GetPlayerController(slot);
    if (!controller)
    {
        META_CONPRINTF("[CS2AWPModes] OnClientPutInServer: controller is NULL\n");
        return;
    }

    // Check if bot
    if (g_pPlayers->IsFakeClient(slot) && !g_Config.General.AllowBots)
    {
        META_CONPRINTF("[CS2AWPModes] OnClientPutInServer: skipping bot\n");
        return;
    }

    META_CONPRINTF("[CS2AWPModes] OnClientPutInServer: creating ArenaPlayer\n");

    // Create arena player
    ArenaPlayer* player = new ArenaPlayer(controller);
    g_pArenas->AddPlayer(player);
    g_PlayerSlots[slot] = player;

    // Add to waiting queue
    g_WaitingPlayers.push(player);

    META_CONPRINTF("[CS2AWPModes] OnClientPutInServer: player added to queue, setting tag\n");

    // Set waiting tag
    player->SetArenaTag("Waiting |");

    // Notify player
    PrintToChat(controller, "[AWP Modes] You have been added to the queue. Position: %d", (int)g_WaitingPlayers.size());
    META_CONPRINTF("[CS2AWPModes] OnClientPutInServer: done, queue size=%d\n", (int)g_WaitingPlayers.size());

    // Load from database if available
    if (DatabaseManager::Instance().IsAvailable())
    {
        uint64_t steamID64 = player->GetSteamID64();
        DatabaseManager::Instance().LoadPlayer(steamID64, player);
    }
}

void OnClientDisconnect(int slot)
{
    if (!g_pArenas)
        return;

    ArenaPlayer* player = g_pArenas->GetPlayer(slot);
    if (!player)
        return;

    // Remove from arena if in one
    Arena* arena = g_pArenas->GetPlayerArena(player);
    if (arena)
    {
        // Remove from team
        auto& team1 = arena->GetTeam1();
        auto& team2 = arena->GetTeam2();
        team1.erase(std::remove(team1.begin(), team1.end(), player), team1.end());
        team2.erase(std::remove(team2.begin(), team2.end(), player), team2.end());
    }

    // Remove from challenges
    g_pArenas->RemoveChallenge(player);

    // Remove from queues
    std::queue<ArenaPlayer*> tempQueue;
    while (!g_WaitingPlayers.empty())
    {
        ArenaPlayer* p = g_WaitingPlayers.front();
        g_WaitingPlayers.pop();
        if (p != player)
            tempQueue.push(p);
    }
    g_WaitingPlayers = tempQueue;

    tempQueue = std::queue<ArenaPlayer*>();
    while (!g_RankingQueue.empty())
    {
        ArenaPlayer* p = g_RankingQueue.front();
        g_RankingQueue.pop();
        if (p != player)
            tempQueue.push(p);
    }
    g_RankingQueue = tempQueue;

    // Remove from tracking
    g_pArenas->RemovePlayer(slot);
    g_PlayerSlots.erase(slot);

    // Delete player object
    delete player;
}
