#ifndef _CS2_AWP_MODES_COMMANDS_H_
#define _CS2_AWP_MODES_COMMANDS_H_

#include "main.h"

class CCSPlayerController;

// Command usage type
enum class CommandUsage
{
    CLIENT_ONLY,
    SERVER_ONLY,
    CLIENT_AND_SERVER
};

// Register all commands
void RegisterCommands();
void UnregisterCommands();

// Command helper - validate command usage
bool CommandHelper(CCSPlayerController* player, CommandUsage usage, const char* permission = nullptr);

// Chat command callbacks (matching K4-Arenas PluginCommands.cs)
void Command_Guns(int slot);
void Command_Rounds(int slot);
void Command_Challenge(int slot);
void Command_ChallengeAccept(int slot);
void Command_ChallengeDecline(int slot);
void Command_AFK(int slot);
void Command_Queue(int slot);

// Admin commands
void Command_ForceArena(int slot);
void Command_ReloadConfig(int slot);

#endif // _CS2_AWP_MODES_COMMANDS_H_
