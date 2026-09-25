#ifndef _CS2_AWP_MODES_MAIN_H_
#define _CS2_AWP_MODES_MAIN_H_

#include <ISmmPlugin.h>
#include <igameevents.h>
#include <mathlib/vector.h>
#include <const.h>
#include <iserver.h>
#include <sh_vector.h>
#include <vector>
#include <queue>
#include <map>
#include <string>
#include <functional>
#include <memory>

// Source 2 SDK types (needed before menus.h)
#include <platform.h>
#include <tier0/basetypes.h>
#include <tier0/platform.h>
#include <mathlib/vector.h>
#include <entity2/entitysystem.h>
#include <entity2/entityidentity.h>
#include "schemasystem.h"

// Forward declarations
class CCSPlayerController;
class CCSPlayerPawn;
class CGameRules;

// cs2-menus-new API
#include <menus.h>

#define PLUGIN_VERSION "1.0.1"

// Global pointers
extern IVEngineServer2* g_pEngine;
extern IGameEventManager2* g_pGameEventManager;
extern CGameEntitySystem* g_pGameEntitySystem;
extern ISchemaSystem* g_pSchemaSystem;
extern CGlobalVars* gpGlobals;
extern IUtilsApi* g_pUtils;
extern IPlayersApi* g_pPlayers;
extern IMenusApi* g_pMenus;
extern ISource2Server* g_pSource2Server;
extern ISource2GameClients* g_pSource2GameClients;
extern CEntitySystem* g_pEntitySystem;

// Forward declarations for our classes
class ArenaPlayer;
class Arena;
class Arenas;
struct RoundType;
struct WeaponInfo;
struct ArenasConfig;

// Global arena system
extern Arenas* g_pArenas;
extern std::queue<ArenaPlayer*> g_WaitingPlayers;
extern std::queue<ArenaPlayer*> g_RankingQueue;
extern ArenasConfig g_Config;
extern bool g_bIsBetweenRounds;
extern CGameRules* g_pGameRules;

// Slot to player mapping
extern std::map<int, ArenaPlayer*> g_PlayerSlots;

// Helper functions
CCSPlayerController* GetPlayerController(int slot);
CCSPlayerPawn* GetPlayerPawn(CCSPlayerController* controller);
int GetPlayerSlot(CCSPlayerController* controller);
uint64_t GetSteamID64(CCSPlayerController* controller);
bool IsValidPlayer(CCSPlayerController* controller);
void PrintToChat(CCSPlayerController* player, const char* format, ...);
void PrintToChatAll(const char* format, ...);
void PrintToCenterHtml(CCSPlayerController* player, int duration, const char* format, ...);
void PrintToCenterHtmlAll(int duration, const char* format, ...);

// Convert userid from event to player slot
// In Source 2, userid = (serial << 8) | slot
inline int GetSlotFromUserId(int userid)
{
    return userid & 0xFF;
}

// Team enum
enum CsTeam
{
    TEAM_NONE = 0,
    TEAM_SPECTATOR = 1,
    TEAM_T = 2,
    TEAM_CT = 3
};

// Weapon type enum (matching K4-Arenas)
enum class WeaponType
{
    Unknown = 0,
    Rifle,
    Sniper,
    Shotgun,
    SMG,
    LMG,
    Pistol
};

// CsItem enum (common CS2 weapons)
enum class CsItem
{
    None = 0,
    // Pistols
    Deagle = 1,
    Elite = 2,
    FiveSeven = 3,
    Glock = 4,
    P250 = 6,
    Tec9 = 30,
    USP = 61,
    CZ75 = 63,
    Revolver = 64,
    // SMGs
    Mac10 = 17,
    MP7 = 33,
    MP9 = 34,
    P90 = 19,
    Bizon = 26,
    MP5SD = 23,
    UMP = 24,
    // Rifles
    AK47 = 7,
    AUG = 8,
    Famas = 10,
    GalilAR = 13,
    M4A4 = 16,
    M4A1S = 60,
    SG556 = 39,
    // Snipers
    AWP = 9,
    Scout = 40,
    G3SG1 = 11,
    SCAR20 = 38,
    // Shotguns
    MAG7 = 27,
    Nova = 35,
    XM1014 = 25,
    Sawedoff = 29,
    // LMG
    M249 = 14,
    Negev = 28,
    // Special
    Knife = 42,
    Zeus = 31
};

// Get weapon name from CsItem
const char* GetWeaponName(CsItem item);
CsItem GetWeaponFromName(const char* name);

// Schema offsets for CCSPlayerPawn (Linux)
// These need to be updated when game updates
#ifdef _WIN32
#define OFFSET_ITEM_SERVICES    0x10F0
#define OFFSET_ARMOR_VALUE      0x1538
#define OFFSET_HAS_HELMET       0x40
#define OFFSET_HEALTH           0x344
#define OFFSET_CLAN_NAME        0x7F0
#else
#define OFFSET_ITEM_SERVICES    0x10F0
#define OFFSET_ARMOR_VALUE      0x1538
#define OFFSET_HAS_HELMET       0x40
#define OFFSET_HEALTH           0x344
#define OFFSET_CLAN_NAME        0x7F0
#endif

// Metamod plugin globals
PLUGIN_GLOBALVARS();

#endif // _CS2_AWP_MODES_MAIN_H_
