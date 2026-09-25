#ifndef _CS2_AWP_MODES_CONFIG_H_
#define _CS2_AWP_MODES_CONFIG_H_

#include <string>
#include <vector>
#include <map>

// Database settings
struct DatabaseSettings
{
    std::string Host = "localhost";
    std::string Username = "root";
    std::string Password = "password";
    std::string Database = "database";
    int Port = 3306;
    std::string TablePrefix = "";
    int TablePurgeDays = 30;
    std::string Sslmode = "Preferred";
};

// Default weapon settings
struct DefaultWeaponSettings
{
    std::string DefaultRifle = "weapon_ak47";
    std::string DefaultSniper = "weapon_awp";
    std::string DefaultShotgun = "weapon_nova";
    std::string DefaultSMG = "weapon_mp9";
    std::string DefaultLMG = "weapon_m249";
    std::string DefaultPistol = "weapon_glock";
    std::string DefaultRound = "k4.rounds.awp";
};

// Command settings
struct CommandSettings
{
    std::vector<std::string> GunsCommands = {"guns", "gun", "weapons", "weapon", "weaponpref"};
    std::vector<std::string> RoundsCommands = {"rounds", "round", "roundpref"};
    std::vector<std::string> ChallengeCommands = {"challenge", "duel", "1v1"};
    std::vector<std::string> ChallengeAcceptCommands = {"caccept", "ca", "accept"};
    std::vector<std::string> ChallengeDeclineCommands = {"cdecline", "cd", "decline"};
    std::vector<std::string> AFKCommands = {"afk", "spec", "spectate"};
    std::vector<std::string> QueueCommands = {"queue", "q"};
};

// Compatibility settings
struct CompatibilitySettings
{
    bool DisableClantags = false;
    bool PreventDrawRounds = false;
    bool AllowSuicide = false;
    bool DisableDamageMessage = false;
    bool DisableOpponentMessage = false;
    bool DisableRoundTypeMessage = false;
    bool BlockDamageOfNotOpponent = true;   // Block damage from players not in same arena
    bool BlockFlashOfNotOpponent = true;    // Block flash effect from players not in same arena
};

// General settings
struct GeneralSettings
{
    int MinPlayers = 2;
    bool AllowBots = true;
    bool VIPOnly = false;
    std::string VIPFlag = "@css/vip";
};

// AWP mode policy. The first release intentionally keeps the policy small and
// deterministic: the arena engine can only expose the round types selected
// here, so an old player preference cannot silently re-enable rifles or SMGs.
struct AWPModeSettings
{
    // awp_arena: AWP-only ladder; awp_rotation: AWP + selected side rounds.
    std::string Mode = "awp_arena";
    bool AllowScout = false;
    bool AllowDeagle = false;
    bool AllowKnife = true;
    bool Armor = true;
    bool Helmet = true;
    int RoundTime = 60;
};

// Round type config reader
struct RoundTypeConfig
{
    std::string TranslationName;
    int TeamSize = 1;
    std::string PrimaryWeapon = "";
    std::string SecondaryWeapon = "";
    bool UsePreferredPrimary = false;
    int PrimaryPreference = 0; // WeaponType as int
    bool UsePreferredSecondary = false;
    bool Armor = true;
    bool Helmet = true;
    bool EnabledByDefault = true;
};

// Main config structure
struct ArenasConfig
{
    DatabaseSettings Database;
    DefaultWeaponSettings DefaultWeapons;
    CommandSettings Commands;
    CompatibilitySettings Compatibility;
    GeneralSettings General;
    AWPModeSettings AWPMode;
    std::vector<RoundTypeConfig> CustomRoundTypes;

    bool bDebug = false;
};

// Config loading
bool LoadConfig(const char* path, ArenasConfig& config);
bool IsDatabaseConfigDefault(const ArenasConfig& config);

#endif // _CS2_AWP_MODES_CONFIG_H_
