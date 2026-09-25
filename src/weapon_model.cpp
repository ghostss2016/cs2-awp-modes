#include "weapon_model.h"

// Static member definitions
std::vector<WeaponInfo> WeaponModel::s_Rifles;
std::vector<WeaponInfo> WeaponModel::s_Snipers;
std::vector<WeaponInfo> WeaponModel::s_Shotguns;
std::vector<WeaponInfo> WeaponModel::s_SMGs;
std::vector<WeaponInfo> WeaponModel::s_LMGs;
std::vector<WeaponInfo> WeaponModel::s_Pistols;
bool WeaponModel::s_bInitialized = false;

void WeaponModel::Initialize()
{
    if (s_bInitialized)
        return;

    // Rifles (matching K4-Arenas WeaponModel.cs)
    s_Rifles = {
        {CsItem::AK47, "AK-47", "weapon_ak47", WeaponType::Rifle},
        {CsItem::M4A4, "M4A4", "weapon_m4a1", WeaponType::Rifle},
        {CsItem::M4A1S, "M4A1-S", "weapon_m4a1_silencer", WeaponType::Rifle},
        {CsItem::GalilAR, "Galil AR", "weapon_galilar", WeaponType::Rifle},
        {CsItem::Famas, "FAMAS", "weapon_famas", WeaponType::Rifle},
        {CsItem::SG556, "SG 553", "weapon_sg556", WeaponType::Rifle},
        {CsItem::AUG, "AUG", "weapon_aug", WeaponType::Rifle},
    };

    // Snipers
    s_Snipers = {
        {CsItem::AWP, "AWP", "weapon_awp", WeaponType::Sniper},
        {CsItem::Scout, "SSG 08", "weapon_ssg08", WeaponType::Sniper},
        {CsItem::G3SG1, "G3SG1", "weapon_g3sg1", WeaponType::Sniper},
        {CsItem::SCAR20, "SCAR-20", "weapon_scar20", WeaponType::Sniper},
    };

    // Shotguns
    s_Shotguns = {
        {CsItem::Nova, "Nova", "weapon_nova", WeaponType::Shotgun},
        {CsItem::XM1014, "XM1014", "weapon_xm1014", WeaponType::Shotgun},
        {CsItem::MAG7, "MAG-7", "weapon_mag7", WeaponType::Shotgun},
        {CsItem::Sawedoff, "Sawed-Off", "weapon_sawedoff", WeaponType::Shotgun},
    };

    // SMGs
    s_SMGs = {
        {CsItem::MP9, "MP9", "weapon_mp9", WeaponType::SMG},
        {CsItem::Mac10, "MAC-10", "weapon_mac10", WeaponType::SMG},
        {CsItem::MP7, "MP7", "weapon_mp7", WeaponType::SMG},
        {CsItem::UMP, "UMP-45", "weapon_ump45", WeaponType::SMG},
        {CsItem::P90, "P90", "weapon_p90", WeaponType::SMG},
        {CsItem::Bizon, "PP-Bizon", "weapon_bizon", WeaponType::SMG},
        {CsItem::MP5SD, "MP5-SD", "weapon_mp5sd", WeaponType::SMG},
    };

    // LMGs
    s_LMGs = {
        {CsItem::M249, "M249", "weapon_m249", WeaponType::LMG},
        {CsItem::Negev, "Negev", "weapon_negev", WeaponType::LMG},
    };

    // Pistols
    s_Pistols = {
        {CsItem::Glock, "Glock-18", "weapon_glock", WeaponType::Pistol},
        {CsItem::USP, "USP-S", "weapon_usp_silencer", WeaponType::Pistol},
        {CsItem::P250, "P250", "weapon_p250", WeaponType::Pistol},
        {CsItem::Deagle, "Desert Eagle", "weapon_deagle", WeaponType::Pistol},
        {CsItem::FiveSeven, "Five-SeveN", "weapon_fiveseven", WeaponType::Pistol},
        {CsItem::Tec9, "Tec-9", "weapon_tec9", WeaponType::Pistol},
        {CsItem::CZ75, "CZ75-Auto", "weapon_cz75a", WeaponType::Pistol},
        {CsItem::Revolver, "R8 Revolver", "weapon_revolver", WeaponType::Pistol},
        {CsItem::Elite, "Dual Berettas", "weapon_elite", WeaponType::Pistol},
    };

    s_bInitialized = true;
}

const std::vector<WeaponInfo>& WeaponModel::GetRifles()
{
    return s_Rifles;
}

const std::vector<WeaponInfo>& WeaponModel::GetSnipers()
{
    return s_Snipers;
}

const std::vector<WeaponInfo>& WeaponModel::GetShotguns()
{
    return s_Shotguns;
}

const std::vector<WeaponInfo>& WeaponModel::GetSMGs()
{
    return s_SMGs;
}

const std::vector<WeaponInfo>& WeaponModel::GetLMGs()
{
    return s_LMGs;
}

const std::vector<WeaponInfo>& WeaponModel::GetPistols()
{
    return s_Pistols;
}

const std::vector<WeaponInfo>& WeaponModel::GetWeaponsByType(WeaponType type)
{
    switch (type)
    {
        case WeaponType::Rifle: return s_Rifles;
        case WeaponType::Sniper: return s_Snipers;
        case WeaponType::Shotgun: return s_Shotguns;
        case WeaponType::SMG: return s_SMGs;
        case WeaponType::LMG: return s_LMGs;
        case WeaponType::Pistol: return s_Pistols;
        default:
            static std::vector<WeaponInfo> empty;
            return empty;
    }
}

const WeaponInfo* WeaponModel::FindWeapon(const char* weaponName)
{
    if (!weaponName)
        return nullptr;

    // Search all weapon lists
    for (const auto& weapon : s_Rifles)
        if (strcmp(weapon.WeaponName, weaponName) == 0)
            return &weapon;
    for (const auto& weapon : s_Snipers)
        if (strcmp(weapon.WeaponName, weaponName) == 0)
            return &weapon;
    for (const auto& weapon : s_Shotguns)
        if (strcmp(weapon.WeaponName, weaponName) == 0)
            return &weapon;
    for (const auto& weapon : s_SMGs)
        if (strcmp(weapon.WeaponName, weaponName) == 0)
            return &weapon;
    for (const auto& weapon : s_LMGs)
        if (strcmp(weapon.WeaponName, weaponName) == 0)
            return &weapon;
    for (const auto& weapon : s_Pistols)
        if (strcmp(weapon.WeaponName, weaponName) == 0)
            return &weapon;

    return nullptr;
}

const WeaponInfo* WeaponModel::FindWeaponByItem(CsItem item)
{
    // Search all weapon lists
    for (const auto& weapon : s_Rifles)
        if (weapon.Item == item)
            return &weapon;
    for (const auto& weapon : s_Snipers)
        if (weapon.Item == item)
            return &weapon;
    for (const auto& weapon : s_Shotguns)
        if (weapon.Item == item)
            return &weapon;
    for (const auto& weapon : s_SMGs)
        if (weapon.Item == item)
            return &weapon;
    for (const auto& weapon : s_LMGs)
        if (weapon.Item == item)
            return &weapon;
    for (const auto& weapon : s_Pistols)
        if (weapon.Item == item)
            return &weapon;

    return nullptr;
}
