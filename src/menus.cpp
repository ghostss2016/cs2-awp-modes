// Include main.h first for SDK types needed by menus.h
#include "main.h"
#include "arena_menus.h"
#include "arena_player.h"
#include "arenas.h"
#include "weapon_model.h"
#include "round_type.h"
#include "config.h"

extern Arenas* g_pArenas;
extern ArenasConfig g_Config;

ArenaMenuManager& ArenaMenuManager::Instance()
{
    static ArenaMenuManager instance;
    return instance;
}

ArenaMenuManager::ArenaMenuManager()
{
}

void ArenaMenuManager::ShowWeaponMenu(ArenaPlayer* player, WeaponType type)
{
    if (!player || !player->IsValid() || !g_pMenus)
        return;

    int slot = player->GetSlot();

    // Get weapons for this type
    const std::vector<WeaponInfo>& weapons = WeaponModel::GetWeaponsByType(type);
    if (weapons.empty())
        return;

    // Build menu title
    const char* typeName = "Weapons";
    switch (type)
    {
        case WeaponType::Rifle: typeName = "Rifles"; break;
        case WeaponType::Sniper: typeName = "Snipers"; break;
        case WeaponType::Shotgun: typeName = "Shotguns"; break;
        case WeaponType::SMG: typeName = "SMGs"; break;
        case WeaponType::LMG: typeName = "LMGs"; break;
        case WeaponType::Pistol: typeName = "Pistols"; break;
        default: break;
    }

    char title[64];
    snprintf(title, sizeof(title), "Select %s", typeName);

    // Create menu
    Menu menu;
    g_pMenus->SetTitleMenu(menu, title);
    g_pMenus->SetExitMenu(menu, true);

    // Add weapon options
    CsItem currentPref = player->GetWeaponPreference(type);
    for (const auto& weapon : weapons)
    {
        char optionText[128];
        if (weapon.Item == currentPref)
            snprintf(optionText, sizeof(optionText), "[*] %s", weapon.DisplayName);
        else
            snprintf(optionText, sizeof(optionText), "%s", weapon.DisplayName);

        // sBack = weapon name (for callback), sText = display text
        g_pMenus->AddItemMenu(menu, weapon.WeaponName, optionText);
    }

    // Set callback
    g_pMenus->SetCallback(menu, [](const char* szBack, const char* szFront, int iItem, int iSlot) {
        ArenaMenuManager::OnWeaponMenuSelect(iSlot, iItem, szBack);
    });

    g_pMenus->DisplayPlayerMenu(menu, slot, true, true);
}

void ArenaMenuManager::ShowRoundMenu(ArenaPlayer* player)
{
    if (!player || !player->IsValid() || !g_pMenus)
        return;

    int slot = player->GetSlot();

    Menu menu;
    g_pMenus->SetTitleMenu(menu, "Round Preferences");
    g_pMenus->SetExitMenu(menu, true);

    // Add all round types
    auto& roundTypes = RoundTypeManager::Instance().GetRoundTypes();
    for (auto& rt : roundTypes)
    {
        char optionText[128];
        bool hasPreference = player->HasRoundPreference(&rt);

        snprintf(optionText, sizeof(optionText), "[%s] %s",
                 hasPreference ? "X" : " ",
                 rt.Name.c_str());

        // Store round ID as value
        char value[16];
        snprintf(value, sizeof(value), "%d", rt.ID);
        g_pMenus->AddItemMenu(menu, value, optionText);
    }

    g_pMenus->SetCallback(menu, [](const char* szBack, const char* szFront, int iItem, int iSlot) {
        ArenaMenuManager::OnRoundMenuSelect(iSlot, iItem, szBack);
    });

    g_pMenus->DisplayPlayerMenu(menu, slot, true, true);
}

void ArenaMenuManager::ShowChallengeMenu(ArenaPlayer* player)
{
    if (!player || !player->IsValid() || !g_pMenus || !g_pArenas)
        return;

    int slot = player->GetSlot();

    Menu menu;
    g_pMenus->SetTitleMenu(menu, "Challenge Player");
    g_pMenus->SetExitMenu(menu, true);

    int itemCount = 0;

    // Add all available players
    for (auto& pair : g_pArenas->GetAllPlayers())
    {
        ArenaPlayer* target = pair.second;
        if (!target || target == player || !target->IsValid() || target->IsAFK())
            continue;

        // Check if target is already in a challenge
        if (g_pArenas->FindChallenge(target))
            continue;

        CCSPlayerController* controller = target->GetController();
        if (!controller)
            continue;

        // Get player name from IPlayersApi
        const char* name = g_pPlayers ? g_pPlayers->GetPlayerName(target->GetSlot()) : "Unknown";
        char value[16];
        snprintf(value, sizeof(value), "%d", target->GetSlot());
        g_pMenus->AddItemMenu(menu, value, name);
        itemCount++;
    }

    if (itemCount == 0)
    {
        PrintToChat(player->GetController(), "[AWP Modes] No players available for challenge.");
        return;
    }

    g_pMenus->SetCallback(menu, [](const char* szBack, const char* szFront, int iItem, int iSlot) {
        ArenaMenuManager::OnChallengeMenuSelect(iSlot, iItem, szBack);
    });

    g_pMenus->DisplayPlayerMenu(menu, slot, true, true);
}

void ArenaMenuManager::CloseMenu(ArenaPlayer* player)
{
    if (!player || !g_pMenus)
        return;

    g_pMenus->ClosePlayerMenu(player->GetSlot());
}

void ArenaMenuManager::OnWeaponMenuSelect(int slot, int item, const char* value)
{
    if (!g_pArenas || !value)
        return;

    ArenaPlayer* player = g_pArenas->GetPlayer(slot);
    if (!player || !player->IsValid())
        return;

    // Find weapon by name
    const WeaponInfo* weapon = WeaponModel::FindWeapon(value);
    if (!weapon)
        return;

    // Set preference
    player->SetWeaponPreference(weapon->Type, weapon->Item);

    PrintToChat(player->GetController(), "[AWP Modes] %s preference set to: %s",
                weapon->Type == WeaponType::Rifle ? "Rifle" :
                weapon->Type == WeaponType::Sniper ? "Sniper" :
                weapon->Type == WeaponType::Shotgun ? "Shotgun" :
                weapon->Type == WeaponType::SMG ? "SMG" :
                weapon->Type == WeaponType::LMG ? "LMG" : "Pistol",
                weapon->DisplayName);
}

void ArenaMenuManager::OnRoundMenuSelect(int slot, int item, const char* value)
{
    if (!g_pArenas || !value)
        return;

    ArenaPlayer* player = g_pArenas->GetPlayer(slot);
    if (!player || !player->IsValid())
        return;

    int roundID = atoi(value);
    RoundType* roundType = RoundTypeManager::Instance().FindByID(roundID);
    if (!roundType)
        return;

    // Toggle preference
    if (player->HasRoundPreference(roundType))
    {
        // Check if this is the last preference
        if (player->GetRoundPreferences().size() <= 1)
        {
            PrintToChat(player->GetController(), "[AWP Modes] You must have at least one round type enabled.");
            return;
        }

        player->RemoveRoundPreference(roundType);
        PrintToChat(player->GetController(), "[AWP Modes] %s disabled.", roundType->Name.c_str());
    }
    else
    {
        player->AddRoundPreference(roundType);
        PrintToChat(player->GetController(), "[AWP Modes] %s enabled.", roundType->Name.c_str());
    }

    // Reshow menu
    Instance().ShowRoundMenu(player);
}

void ArenaMenuManager::OnChallengeMenuSelect(int slot, int item, const char* value)
{
    if (!g_pArenas || !value)
        return;

    ArenaPlayer* player = g_pArenas->GetPlayer(slot);
    if (!player || !player->IsValid())
        return;

    int targetSlot = atoi(value);
    ArenaPlayer* target = g_pArenas->GetPlayer(targetSlot);
    if (!target || !target->IsValid())
    {
        PrintToChat(player->GetController(), "[AWP Modes] Player no longer available.");
        return;
    }

    // Check if already in challenge
    if (g_pArenas->FindChallenge(player))
    {
        PrintToChat(player->GetController(), "[AWP Modes] You are already in a challenge.");
        return;
    }

    if (g_pArenas->FindChallenge(target))
    {
        PrintToChat(player->GetController(), "[AWP Modes] That player is already in a challenge.");
        return;
    }

    // Check if target already has pending challenge
    if (target->GetChallengeTarget() >= 0)
    {
        PrintToChat(player->GetController(), "[AWP Modes] That player already has a pending challenge.");
        return;
    }

    // Set pending challenge (Player1 challenges Player2)
    // Store challenger's slot in target's ChallengeTarget so target knows who challenged them
    player->SetChallengeTarget(targetSlot);

    PrintToChat(player->GetController(), "[AWP Modes] Challenge sent! Waiting for response...");
    PrintToChat(target->GetController(), "[AWP Modes] You have been challenged! Use !caccept or !cdecline.");
}
