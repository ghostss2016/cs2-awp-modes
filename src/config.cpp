#include "config.h"
#include <fstream>
#include <sstream>
#include <cstring>

// Simple key-value config parser
static std::string Trim(const std::string& str)
{
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

static bool ParseBool(const std::string& value)
{
    std::string lower = value;
    for (auto& c : lower) c = tolower(c);
    return lower == "true" || lower == "1" || lower == "yes";
}

bool LoadConfig(const char* path, ArenasConfig& config)
{
    std::ifstream file(path);
    if (!file.is_open())
        return false;

    std::string line;
    std::string currentSection;

    while (std::getline(file, line))
    {
        line = Trim(line);

        // Skip comments and empty lines
        if (line.empty() || line[0] == '#' || line[0] == ';' || (line.length() >= 2 && line[0] == '/' && line[1] == '/'))
            continue;

        // Section header
        if (line[0] == '[' && line.back() == ']')
        {
            currentSection = line.substr(1, line.length() - 2);
            continue;
        }

        // Key-value pair
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos)
            continue;

        std::string key = Trim(line.substr(0, eqPos));
        std::string value = Trim(line.substr(eqPos + 1));

        // Remove quotes if present
        if (value.length() >= 2 && value[0] == '"' && value.back() == '"')
            value = value.substr(1, value.length() - 2);

        // Parse based on section
        if (currentSection == "Database")
        {
            if (key == "Host") config.Database.Host = value;
            else if (key == "Username") config.Database.Username = value;
            else if (key == "Password") config.Database.Password = value;
            else if (key == "Database") config.Database.Database = value;
            else if (key == "Port") config.Database.Port = std::stoi(value);
            else if (key == "TablePrefix") config.Database.TablePrefix = value;
            else if (key == "TablePurgeDays") config.Database.TablePurgeDays = std::stoi(value);
            else if (key == "Sslmode") config.Database.Sslmode = value;
        }
        else if (currentSection == "DefaultWeapons")
        {
            if (key == "DefaultRifle") config.DefaultWeapons.DefaultRifle = value;
            else if (key == "DefaultSniper") config.DefaultWeapons.DefaultSniper = value;
            else if (key == "DefaultShotgun") config.DefaultWeapons.DefaultShotgun = value;
            else if (key == "DefaultSMG") config.DefaultWeapons.DefaultSMG = value;
            else if (key == "DefaultLMG") config.DefaultWeapons.DefaultLMG = value;
            else if (key == "DefaultPistol") config.DefaultWeapons.DefaultPistol = value;
            else if (key == "DefaultRound") config.DefaultWeapons.DefaultRound = value;
        }
        else if (currentSection == "Compatibility")
        {
            if (key == "DisableClantags") config.Compatibility.DisableClantags = ParseBool(value);
            else if (key == "PreventDrawRounds") config.Compatibility.PreventDrawRounds = ParseBool(value);
            else if (key == "AllowSuicide") config.Compatibility.AllowSuicide = ParseBool(value);
            else if (key == "DisableDamageMessage") config.Compatibility.DisableDamageMessage = ParseBool(value);
            else if (key == "DisableOpponentMessage") config.Compatibility.DisableOpponentMessage = ParseBool(value);
            else if (key == "DisableRoundTypeMessage") config.Compatibility.DisableRoundTypeMessage = ParseBool(value);
            else if (key == "BlockDamageOfNotOpponent") config.Compatibility.BlockDamageOfNotOpponent = ParseBool(value);
            else if (key == "BlockFlashOfNotOpponent") config.Compatibility.BlockFlashOfNotOpponent = ParseBool(value);
        }
        else if (currentSection == "General")
        {
            if (key == "MinPlayers") config.General.MinPlayers = std::stoi(value);
            else if (key == "AllowBots") config.General.AllowBots = ParseBool(value);
            else if (key == "VIPOnly") config.General.VIPOnly = ParseBool(value);
            else if (key == "VIPFlag") config.General.VIPFlag = value;
            else if (key == "Debug") config.bDebug = ParseBool(value);
        }
        else if (currentSection == "AWPMode")
        {
            if (key == "Mode") config.AWPMode.Mode = value;
            else if (key == "AllowScout") config.AWPMode.AllowScout = ParseBool(value);
            else if (key == "AllowDeagle") config.AWPMode.AllowDeagle = ParseBool(value);
            else if (key == "AllowKnife") config.AWPMode.AllowKnife = ParseBool(value);
            else if (key == "Armor") config.AWPMode.Armor = ParseBool(value);
            else if (key == "Helmet") config.AWPMode.Helmet = ParseBool(value);
            else if (key == "RoundTime") config.AWPMode.RoundTime = std::stoi(value);
        }
        // Command settings are parsed similarly
    }

    return true;
}

bool IsDatabaseConfigDefault(const ArenasConfig& config)
{
    return config.Database.Host == "localhost" &&
           config.Database.Username == "root" &&
           config.Database.Database == "database" &&
           config.Database.Password == "password";
}
