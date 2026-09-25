#ifndef _CS2_AWP_MODES_ARENA_MENUS_H_
#define _CS2_AWP_MODES_ARENA_MENUS_H_

#include "main.h"

class ArenaPlayer;

// Menu types
enum class ArenaMenuType
{
    None,
    WeaponRifle,
    WeaponSniper,
    WeaponShotgun,
    WeaponSMG,
    WeaponLMG,
    WeaponPistol,
    RoundPreference,
    Challenge
};

// Menu manager (using cs2-menus-new API)
class ArenaMenuManager
{
public:
    static ArenaMenuManager& Instance();

    // Show weapon selection menu
    void ShowWeaponMenu(ArenaPlayer* player, WeaponType type);

    // Show round preference menu
    void ShowRoundMenu(ArenaPlayer* player);

    // Show challenge menu (list of players to challenge)
    void ShowChallengeMenu(ArenaPlayer* player);

    // Close any open menu
    void CloseMenu(ArenaPlayer* player);

private:
    ArenaMenuManager();

    // Menu callbacks
    static void OnWeaponMenuSelect(int slot, int item, const char* value);
    static void OnRoundMenuSelect(int slot, int item, const char* value);
    static void OnChallengeMenuSelect(int slot, int item, const char* value);
};

#endif // _CS2_AWP_MODES_ARENA_MENUS_H_
