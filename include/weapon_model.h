#ifndef _CS2_AWP_MODES_WEAPON_MODEL_H_
#define _CS2_AWP_MODES_WEAPON_MODEL_H_

#include "main.h"
#include <vector>
#include <map>

// Weapon info structure
struct WeaponInfo
{
    CsItem Item;
    const char* DisplayName;
    const char* WeaponName;  // weapon_ak47 format
    WeaponType Type;
};

// Weapon lists by type (matching K4-Arenas WeaponModel.cs)
class WeaponModel
{
public:
    // Get weapons by type
    static const std::vector<WeaponInfo>& GetRifles();
    static const std::vector<WeaponInfo>& GetSnipers();
    static const std::vector<WeaponInfo>& GetShotguns();
    static const std::vector<WeaponInfo>& GetSMGs();
    static const std::vector<WeaponInfo>& GetLMGs();
    static const std::vector<WeaponInfo>& GetPistols();

    // Get weapons by type enum
    static const std::vector<WeaponInfo>& GetWeaponsByType(WeaponType type);

    // Find weapon by name
    static const WeaponInfo* FindWeapon(const char* weaponName);
    static const WeaponInfo* FindWeaponByItem(CsItem item);

    // Initialize weapon lists
    static void Initialize();

private:
    static std::vector<WeaponInfo> s_Rifles;
    static std::vector<WeaponInfo> s_Snipers;
    static std::vector<WeaponInfo> s_Shotguns;
    static std::vector<WeaponInfo> s_SMGs;
    static std::vector<WeaponInfo> s_LMGs;
    static std::vector<WeaponInfo> s_Pistols;
    static bool s_bInitialized;
};

#endif // _CS2_AWP_MODES_WEAPON_MODEL_H_
