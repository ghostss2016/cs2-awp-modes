#include "main.h"
#include "config.h"
#include "weapon_model.h"
#include "round_type.h"
#include "arena_player.h"
#include "arena.h"
#include "arenas.h"
#include "arena_finder.h"
#include "events.h"
#include "commands.h"
#include "database.h"
#include "arena_menus.h"

#include <ISmmPlugin.h>
#include <igameevents.h>
#include <iserver.h>
#include <entity2/entitysystem.h>
#include "schemasystem.h"
#include <CCSPlayerController.h>

// Plugin class
class CS2AWPModes : public ISmmPlugin, public IMetamodListener
{
public:
    bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late);
    bool Unload(char* error, size_t maxlen);
    bool Pause(char* error, size_t maxlen) { return true; }
    bool Unpause(char* error, size_t maxlen) { return true; }
    void AllPluginsLoaded();

    const char* GetAuthor() { return "K4ryuu (ported by Claude)"; }
    const char* GetName() { return "CS2 AWP Modes"; }
    const char* GetDescription() { return "1v1 Arena system for CS2"; }
    const char* GetURL() { return "https://github.com/KitsuneLab-Development/K4-Arenas"; }
    const char* GetLicense() { return "GPL-3.0"; }
    const char* GetVersion() { return PLUGIN_VERSION; }
    const char* GetDate() { return __DATE__; }
    const char* GetLogTag() { return "AWP_MODES"; }

private:
    void Hook_GameFrame(bool simulating, bool bFirstTick, bool bLastTick);
    void Hook_ClientPutInServer(CPlayerSlot slot, char const* pszName, int type, uint64 xuid);
    void Hook_ClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason,
        const char* pszName, uint64 xuid, const char* pszNetworkID);
};

// Global instances
CS2AWPModes g_Plugin;
PLUGIN_EXPOSE(CS2AWPModes, g_Plugin);

// Global pointers (defined by us)
IVEngineServer2* g_pEngine = nullptr;
IGameEventManager2* g_pGameEventManager = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CGlobalVars* gpGlobals = nullptr;
IUtilsApi* g_pUtils = nullptr;
IPlayersApi* g_pPlayers = nullptr;
IMenusApi* g_pMenus = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;

// Note: g_pSchemaSystem, g_pSource2Server, g_pSource2GameClients are provided by SDK interfaces.a
// We don't define them here to avoid multiple definition errors

// Arena system globals
Arenas* g_pArenas = nullptr;
std::queue<ArenaPlayer*> g_WaitingPlayers;
std::queue<ArenaPlayer*> g_RankingQueue;
ArenasConfig g_Config;
bool g_bIsBetweenRounds = false;
CGameRules* g_pGameRules = nullptr;
std::map<int, ArenaPlayer*> g_PlayerSlots;

// Required by entitysystem.h inline functions
CGameEntitySystem* GameEntitySystem()
{
    return reinterpret_cast<CGameEntitySystem*>(g_pEntitySystem);
}

// Source hooks
SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);
SH_DECL_HOOK4_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0,
    CPlayerSlot, char const*, int, uint64);
SH_DECL_HOOK5_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0,
    CPlayerSlot, ENetworkDisconnectionReason, const char*, uint64, const char*);

// Hook handlers
void CS2AWPModes::Hook_GameFrame(bool simulating, bool bFirstTick, bool bLastTick)
{
    OnGameFrame(simulating, bFirstTick, bLastTick);
}

void CS2AWPModes::Hook_ClientPutInServer(CPlayerSlot slot, char const* pszName, int type, uint64 xuid)
{
    OnClientPutInServer(slot.Get());
}

void CS2AWPModes::Hook_ClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason,
    const char* pszName, uint64 xuid, const char* pszNetworkID)
{
    OnClientDisconnect(slot.Get());
}

bool CS2AWPModes::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
    PLUGIN_SAVEVARS();

    GET_V_IFACE_CURRENT(GetEngineFactory, g_pEngine, IVEngineServer2, INTERFACEVERSION_VENGINESERVER);
    GET_V_IFACE_CURRENT(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetServerFactory, g_pSource2GameClients, ISource2GameClients, SOURCE2GAMECLIENTS_INTERFACE_VERSION);

    // Load config
    char configPath[512];
    g_SMAPI->PathFormat(configPath, sizeof(configPath), "%s/addons/configs/cs2-awp-modes.cfg",
                        g_SMAPI->GetBaseDir());

    if (!LoadConfig(configPath, g_Config))
    {
        META_CONPRINTF("[CS2AWPModes] Warning: Failed to load config from %s, using defaults\n", configPath);
    }

    if (g_Config.AWPMode.Mode != "awp_arena" && g_Config.AWPMode.Mode != "awp_rotation")
    {
        META_CONPRINTF("[CS2AWPModes] Unknown AWPMode '%s'; falling back to awp_arena\n",
            g_Config.AWPMode.Mode.c_str());
        g_Config.AWPMode.Mode = "awp_arena";
        g_Config.AWPMode.AllowScout = false;
        g_Config.AWPMode.AllowDeagle = false;
    }

    // Initialize weapon model and the round catalogue after config loading.
    // This is important for AWP policy: the player preference list must never
    // be built from the generic rifle/SMG catalogue before the AWP config is read.
    WeaponModel::Initialize();
    RoundTypeManager::Instance().Initialize();

    if (RoundType* awp = RoundTypeManager::Instance().FindByName("k4.rounds.awp"))
    {
        awp->Armor = g_Config.AWPMode.Armor;
        awp->Helmet = g_Config.AWPMode.Helmet;
    }

    META_CONPRINTF("[CS2AWPModes] mode=%s scout=%s deagle=%s knife=%s round_time=%d\n",
        g_Config.AWPMode.Mode.c_str(),
        g_Config.AWPMode.AllowScout ? "on" : "off",
        g_Config.AWPMode.AllowDeagle ? "on" : "off",
        g_Config.AWPMode.AllowKnife ? "on" : "off",
        g_Config.AWPMode.RoundTime);

    // Add custom round types from config
    for (const auto& rt : g_Config.CustomRoundTypes)
    {
        RoundType customRound(
            rt.TranslationName.c_str(),
            rt.TeamSize,
            CsItem::None,  // Will be set from weapon name
            CsItem::None,
            rt.UsePreferredPrimary,
            (WeaponType)rt.PrimaryPreference,
            rt.UsePreferredSecondary,
            rt.Armor,
            rt.Helmet,
            rt.EnabledByDefault
        );
        RoundTypeManager::Instance().AddRoundType(customRound);
    }

    // Create arena system
    g_pArenas = new Arenas();

    // Register hooks
    SH_ADD_HOOK(IServerGameDLL, GameFrame, g_pSource2Server,
        SH_MEMBER(this, &CS2AWPModes::Hook_GameFrame), true);
    SH_ADD_HOOK(IServerGameClients, ClientPutInServer, g_pSource2GameClients,
        SH_MEMBER(this, &CS2AWPModes::Hook_ClientPutInServer), true);
    SH_ADD_HOOK(IServerGameClients, ClientDisconnect, g_pSource2GameClients,
        SH_MEMBER(this, &CS2AWPModes::Hook_ClientDisconnect), true);

    META_CONPRINTF("[CS2AWPModes] Plugin loaded (version %s)\n", PLUGIN_VERSION);

    if (late)
    {
        // Late load - get existing pointers
        // This will be set in AllPluginsLoaded
    }

    return true;
}

void CS2AWPModes::AllPluginsLoaded()
{
    // Get cs2-menus-new API via MetaFactory
    int ret;
    g_pUtils = (IUtilsApi*)g_SMAPI->MetaFactory(Utils_INTERFACE, &ret, nullptr);
    if (ret == META_IFACE_FAILED || !g_pUtils)
    {
        META_CONPRINTF("[CS2AWPModes] ERROR: Failed to get IUtilsApi interface!\n");
        return;
    }

    g_pPlayers = (IPlayersApi*)g_SMAPI->MetaFactory(PLAYERS_INTERFACE, &ret, nullptr);
    if (ret == META_IFACE_FAILED || !g_pPlayers)
    {
        META_CONPRINTF("[CS2AWPModes] ERROR: Failed to get IPlayersApi interface!\n");
        return;
    }

    g_pMenus = (IMenusApi*)g_SMAPI->MetaFactory(Menus_INTERFACE, &ret, nullptr);
    if (ret == META_IFACE_FAILED || !g_pMenus)
    {
        META_CONPRINTF("[CS2AWPModes] Warning: Failed to get IMenusApi interface (menus disabled)\n");
    }

    // Get globals from Utils API
    gpGlobals = g_pUtils->GetCGlobalVars();
    g_pGameEventManager = g_pUtils->GetGameEventManager();
    g_pEntitySystem = g_pUtils->GetCEntitySystem();
    g_pGameEntitySystem = g_pUtils->GetCGameEntitySystem();

    if (!g_pGameEventManager)
    {
        META_CONPRINTF("[CS2AWPModes] ERROR: Failed to get IGameEventManager2!\n");
        return;
    }

    // Initialize events
    InitializeEvents();

    // Register commands
    RegisterCommands();

    // Initialize database if configured
    if (!IsDatabaseConfigDefault(g_Config))
    {
        if (DatabaseManager::Instance().Initialize(g_Config.Database))
        {
            DatabaseManager::Instance().CreateTables();
            META_CONPRINTF("[CS2AWPModes] Database connected\n");
        }
        else
        {
            META_CONPRINTF("[CS2AWPModes] Warning: Database connection failed\n");
        }
    }

    // Note: Arena finding moved to round_prestart event when map entities are loaded

    // Hook map start for safe map name access
    g_pUtils->MapStartHook(g_PLID, [](const char* mapName) {
        extern char g_szCurrentMap[256];
        extern bool g_bArenasDetected;
        extern bool g_bWaitingForArenas;
        extern float g_flMapLoadTime;

        META_CONPRINTF("[CS2AWPModes] MapStartHook: map='%s'\n", mapName ? mapName : "NULL");

        // Reset detection flags for new map
        g_bArenasDetected = false;
        g_bWaitingForArenas = false;
        g_flMapLoadTime = -1.0f;

        // Store map name safely
        if (mapName && mapName[0])
        {
            strncpy(g_szCurrentMap, mapName, sizeof(g_szCurrentMap) - 1);
            g_szCurrentMap[sizeof(g_szCurrentMap) - 1] = '\0';
        }
        else
        {
            g_szCurrentMap[0] = '\0';
        }
    });

    META_CONPRINTF("[CS2AWPModes] All plugins loaded, plugin ready\n");
}

bool CS2AWPModes::Unload(char* error, size_t maxlen)
{
    // Unregister hooks
    SH_REMOVE_HOOK(IServerGameDLL, GameFrame, g_pSource2Server,
        SH_MEMBER(this, &CS2AWPModes::Hook_GameFrame), true);
    SH_REMOVE_HOOK(IServerGameClients, ClientPutInServer, g_pSource2GameClients,
        SH_MEMBER(this, &CS2AWPModes::Hook_ClientPutInServer), true);
    SH_REMOVE_HOOK(IServerGameClients, ClientDisconnect, g_pSource2GameClients,
        SH_MEMBER(this, &CS2AWPModes::Hook_ClientDisconnect), true);

    // Shutdown events
    ShutdownEvents();

    // Unregister commands
    UnregisterCommands();

    // Shutdown database
    DatabaseManager::Instance().Shutdown();

    // Cleanup arena system
    if (g_pArenas)
    {
        delete g_pArenas;
        g_pArenas = nullptr;
    }

    // Clear queues
    while (!g_WaitingPlayers.empty()) g_WaitingPlayers.pop();
    while (!g_RankingQueue.empty()) g_RankingQueue.pop();

    // Clear player slots
    for (auto& pair : g_PlayerSlots)
    {
        if (pair.second)
            delete pair.second;
    }
    g_PlayerSlots.clear();

    META_CONPRINTF("[CS2AWPModes] Plugin unloaded\n");
    return true;
}

// Helper functions implementation
CCSPlayerController* GetPlayerController(int slot)
{
    if (slot < 0 || slot >= 64)
        return nullptr;
    return CCSPlayerController::FromSlot(slot);
}

CCSPlayerPawn* GetPlayerPawn(CCSPlayerController* controller)
{
    if (!controller)
        return nullptr;
    // Get pawn handle and resolve it
    CHandle<CCSPlayerPawn> hPawn = controller->m_hPlayerPawn();
    if (!hPawn.IsValid())
        return nullptr;
    return hPawn.Get();
}

int GetPlayerSlot(CCSPlayerController* controller)
{
    if (!controller)
        return -1;
    // Iterate through slots to find matching controller
    for (int i = 0; i < 64; i++)
    {
        if (CCSPlayerController::FromSlot(i) == controller)
            return i;
    }
    return -1;
}

uint64_t GetSteamID64(CCSPlayerController* controller)
{
    if (!controller)
        return 0;
    return controller->m_steamID();
}

bool IsValidPlayer(CCSPlayerController* controller)
{
    if (!controller)
        return false;
    int slot = GetPlayerSlot(controller);
    if (slot < 0 || !g_pPlayers)
        return false;
    return !g_pPlayers->IsFakeClient(slot) && g_pPlayers->IsInGame(slot);
}

void PrintToChat(CCSPlayerController* player, const char* format, ...)
{
    if (!player || !g_pUtils)
        return;

    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    int slot = GetPlayerSlot(player);
    if (slot >= 0)
    {
        g_pUtils->PrintToChat(slot, buffer);
    }
}

void PrintToChatAll(const char* format, ...)
{
    if (!g_pUtils)
        return;

    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    g_pUtils->PrintToChatAll(buffer);
}

void PrintToCenterHtml(CCSPlayerController* player, int duration, const char* format, ...)
{
    if (!player || !g_pUtils)
        return;

    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    int slot = GetPlayerSlot(player);
    if (slot >= 0)
    {
        g_pUtils->PrintToCenterHtml(slot, duration, buffer);
    }
}

void PrintToCenterHtmlAll(int duration, const char* format, ...)
{
    if (!g_pUtils)
        return;

    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    g_pUtils->PrintToCenterHtmlAll(duration, buffer);
}

// Weapon name lookup
const char* GetWeaponName(CsItem item)
{
    switch (item)
    {
        case CsItem::AK47: return "weapon_ak47";
        case CsItem::M4A4: return "weapon_m4a1";
        case CsItem::M4A1S: return "weapon_m4a1_silencer";
        case CsItem::AWP: return "weapon_awp";
        case CsItem::Scout: return "weapon_ssg08";
        case CsItem::Deagle: return "weapon_deagle";
        case CsItem::Glock: return "weapon_glock";
        case CsItem::USP: return "weapon_usp_silencer";
        case CsItem::P250: return "weapon_p250";
        case CsItem::Mac10: return "weapon_mac10";
        case CsItem::MP9: return "weapon_mp9";
        case CsItem::UMP: return "weapon_ump45";
        case CsItem::Nova: return "weapon_nova";
        case CsItem::XM1014: return "weapon_xm1014";
        case CsItem::M249: return "weapon_m249";
        case CsItem::Negev: return "weapon_negev";
        case CsItem::Knife: return "weapon_knife";
        // Add more as needed
        default: return "";
    }
}

CsItem GetWeaponFromName(const char* name)
{
    if (!name) return CsItem::None;

    if (strcmp(name, "weapon_ak47") == 0) return CsItem::AK47;
    if (strcmp(name, "weapon_m4a1") == 0) return CsItem::M4A4;
    if (strcmp(name, "weapon_m4a1_silencer") == 0) return CsItem::M4A1S;
    if (strcmp(name, "weapon_awp") == 0) return CsItem::AWP;
    if (strcmp(name, "weapon_ssg08") == 0) return CsItem::Scout;
    if (strcmp(name, "weapon_deagle") == 0) return CsItem::Deagle;
    if (strcmp(name, "weapon_glock") == 0) return CsItem::Glock;
    if (strcmp(name, "weapon_usp_silencer") == 0) return CsItem::USP;
    // Add more as needed

    return CsItem::None;
}
