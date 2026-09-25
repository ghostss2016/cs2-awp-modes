// Include main.h first for SDK types
#include "main.h"
#include "commands.h"
#include "arena_player.h"
#include "arena.h"
#include "arenas.h"
#include "config.h"
#include "arena_menus.h"
#include "weapon_model.h"
#include "round_type.h"
#include <algorithm>
#include <vector>

extern Arenas* g_pArenas;
extern std::queue<ArenaPlayer*> g_WaitingPlayers;
extern ArenasConfig g_Config;
extern PluginId g_PLID;

// Command callback wrapper
static bool OnGunsCommand(int slot, const char* content)
{
    Command_Guns(slot);
    return false;  // Don't print to chat
}

static bool OnRoundsCommand(int slot, const char* content)
{
    Command_Rounds(slot);
    return false;
}

static bool OnChallengeCommand(int slot, const char* content)
{
    Command_Challenge(slot);
    return false;
}

static bool OnAFKCommand(int slot, const char* content)
{
    Command_AFK(slot);
    return false;
}

static bool OnQueueCommand(int slot, const char* content)
{
    Command_Queue(slot);
    return false;
}

static bool OnChallengeAcceptCommand(int slot, const char* content)
{
    Command_ChallengeAccept(slot);
    return false;
}

static bool OnChallengeDeclineCommand(int slot, const char* content)
{
    Command_ChallengeDecline(slot);
    return false;
}

void RegisterCommands()
{
    if (!g_pUtils)
        return;

    // Register guns command
    g_pUtils->RegCommand(g_PLID, {}, g_Config.Commands.GunsCommands, OnGunsCommand);

    // Register rounds command
    g_pUtils->RegCommand(g_PLID, {}, g_Config.Commands.RoundsCommands, OnRoundsCommand);

    // Register challenge command
    g_pUtils->RegCommand(g_PLID, {}, g_Config.Commands.ChallengeCommands, OnChallengeCommand);

    // Register challenge accept command
    g_pUtils->RegCommand(g_PLID, {}, g_Config.Commands.ChallengeAcceptCommands, OnChallengeAcceptCommand);

    // Register challenge decline command
    g_pUtils->RegCommand(g_PLID, {}, g_Config.Commands.ChallengeDeclineCommands, OnChallengeDeclineCommand);

    // Register AFK command
    g_pUtils->RegCommand(g_PLID, {}, g_Config.Commands.AFKCommands, OnAFKCommand);

    // Register queue command
    g_pUtils->RegCommand(g_PLID, {}, g_Config.Commands.QueueCommands, OnQueueCommand);

}

void UnregisterCommands()
{
    // Commands are automatically unregistered via ClearAllHooks
    if (g_pUtils)
        g_pUtils->ClearAllHooks(g_PLID);
}

bool CommandHelper(CCSPlayerController* player, CommandUsage usage, const char* permission)
{
    switch (usage)
    {
        case CommandUsage::CLIENT_ONLY:
            if (!player || !IsValidPlayer(player))
                return false;
            break;

        case CommandUsage::SERVER_ONLY:
            if (player)
            {
                PrintToChat(player, "[AWP Modes] This command can only be used from server console.");
                return false;
            }
            break;

        case CommandUsage::CLIENT_AND_SERVER:
            break;
    }

    return true;
}

void Command_Guns(int slot)
{
    CCSPlayerController* controller = GetPlayerController(slot);
    if (!controller)
        return;

    if (!CommandHelper(controller, CommandUsage::CLIENT_ONLY))
        return;

    if (!g_pArenas)
        return;

    ArenaPlayer* player = g_pArenas->GetPlayer(slot);
    if (!player)
    {
        PrintToChat(controller, "[AWP Modes] You are not in the arena system.");
        return;
    }

    // Show weapon type selection menu
    PrintToChat(controller, "[AWP Modes] Use: !guns rifle, !guns sniper, !guns pistol, etc.");
}

void Command_Rounds(int slot)
{
    CCSPlayerController* controller = GetPlayerController(slot);
    if (!controller)
        return;

    if (!CommandHelper(controller, CommandUsage::CLIENT_ONLY))
        return;

    if (!g_pArenas)
        return;

    ArenaPlayer* player = g_pArenas->GetPlayer(slot);
    if (!player)
    {
        PrintToChat(controller, "[AWP Modes] You are not in the arena system.");
        return;
    }

    player->ShowRoundMenu();
}

void Command_Challenge(int slot)
{
    CCSPlayerController* controller = GetPlayerController(slot);
    if (!controller)
        return;

    if (!CommandHelper(controller, CommandUsage::CLIENT_ONLY))
        return;

    if (!g_pArenas)
        return;

    ArenaPlayer* player = g_pArenas->GetPlayer(slot);
    if (!player)
    {
        PrintToChat(controller, "[AWP Modes] You are not in the arena system.");
        return;
    }

    if (g_pArenas->FindChallenge(player))
    {
        PrintToChat(controller, "[AWP Modes] You are already in a challenge.");
        return;
    }

    ArenaMenuManager::Instance().ShowChallengeMenu(player);
}

void Command_AFK(int slot)
{
    CCSPlayerController* controller = GetPlayerController(slot);
    if (!controller)
        return;

    if (!CommandHelper(controller, CommandUsage::CLIENT_ONLY))
        return;

    if (!g_pArenas)
        return;

    ArenaPlayer* player = g_pArenas->GetPlayer(slot);
    if (!player)
    {
        PrintToChat(controller, "[AWP Modes] You are not in the arena system.");
        return;
    }

    bool newAFK = !player->IsAFK();
    player->SetAFK(newAFK);

    if (newAFK)
    {
        PrintToChat(controller, "[AWP Modes] You are now AFK. Use !afk to rejoin.");

        std::queue<ArenaPlayer*> tempQueue;
        while (!g_WaitingPlayers.empty())
        {
            ArenaPlayer* p = g_WaitingPlayers.front();
            g_WaitingPlayers.pop();
            if (p != player)
                tempQueue.push(p);
        }
        g_WaitingPlayers = tempQueue;
    }
    else
    {
        PrintToChat(controller, "[AWP Modes] You have rejoined the queue.");
        g_WaitingPlayers.push(player);
    }
}

void Command_Queue(int slot)
{
    CCSPlayerController* controller = GetPlayerController(slot);
    if (!controller)
        return;

    if (!CommandHelper(controller, CommandUsage::CLIENT_ONLY))
        return;

    if (!g_pArenas)
        return;

    ArenaPlayer* player = g_pArenas->GetPlayer(slot);
    if (!player)
    {
        PrintToChat(controller, "[AWP Modes] You are not in the arena system.");
        return;
    }

    int position = 0;
    std::queue<ArenaPlayer*> tempQueue = g_WaitingPlayers;
    while (!tempQueue.empty())
    {
        position++;
        if (tempQueue.front() == player)
        {
            PrintToChat(controller, "[AWP Modes] Your queue position: %d", position);
            return;
        }
        tempQueue.pop();
    }

    Arena* arena = g_pArenas->GetPlayerArena(player);
    if (arena)
    {
        PrintToChat(controller, "[AWP Modes] You are in Arena %d", arena->GetArenaID());
        return;
    }

    PrintToChat(controller, "[AWP Modes] You are not in the queue.");
}

void Command_ChallengeAccept(int slot)
{
    CCSPlayerController* controller = GetPlayerController(slot);
    if (!controller)
        return;

    if (!CommandHelper(controller, CommandUsage::CLIENT_ONLY))
        return;

    if (!g_pArenas)
        return;

    ArenaPlayer* player = g_pArenas->GetPlayer(slot);
    if (!player)
    {
        PrintToChat(controller, "[AWP Modes] You are not in the arena system.");
        return;
    }

    // Check if already in challenge
    if (g_pArenas->FindChallenge(player))
    {
        PrintToChat(controller, "[AWP Modes] You are already in a challenge.");
        return;
    }

    // Find who challenged this player (someone whose ChallengeTarget == this slot)
    ArenaPlayer* challenger = nullptr;
    for (auto& [pslot, p] : g_pArenas->GetAllPlayers())
    {
        if (p && p->IsValid() && p->GetChallengeTarget() == slot)
        {
            challenger = p;
            break;
        }
    }

    if (!challenger)
    {
        PrintToChat(controller, "[AWP Modes] You have no pending challenge.");
        return;
    }

    // Create the actual challenge
    ChallengeModel challenge(challenger, player, 0, 0);
    g_pArenas->AddChallenge(challenge);

    // Clear challenge target
    challenger->SetChallengeTarget(-1);

    PrintToChat(controller, "[AWP Modes] Challenge accepted! Match will start next round.");
    if (challenger->IsValid())
        PrintToChat(challenger->GetController(), "[AWP Modes] Your challenge was accepted!");
}

void Command_ChallengeDecline(int slot)
{
    CCSPlayerController* controller = GetPlayerController(slot);
    if (!controller)
        return;

    if (!CommandHelper(controller, CommandUsage::CLIENT_ONLY))
        return;

    if (!g_pArenas)
        return;

    ArenaPlayer* player = g_pArenas->GetPlayer(slot);
    if (!player)
    {
        PrintToChat(controller, "[AWP Modes] You are not in the arena system.");
        return;
    }

    // Find who challenged this player
    ArenaPlayer* challenger = nullptr;
    for (auto& [pslot, p] : g_pArenas->GetAllPlayers())
    {
        if (p && p->IsValid() && p->GetChallengeTarget() == slot)
        {
            challenger = p;
            break;
        }
    }

    if (!challenger)
    {
        PrintToChat(controller, "[AWP Modes] You have no pending challenge.");
        return;
    }

    // Clear the challenge target
    challenger->SetChallengeTarget(-1);

    PrintToChat(controller, "[AWP Modes] Challenge declined.");
    if (challenger->IsValid())
        PrintToChat(challenger->GetController(), "[AWP Modes] Your challenge was declined.");
}

void Command_ForceArena(int slot)
{
    // Admin command - not implemented yet
}

void Command_ReloadConfig(int slot)
{
    // Admin command - not implemented yet
}
