#include "round_type.h"
#include "config.h"

extern ArenasConfig g_Config;

// RoundType constructor
RoundType::RoundType(const char* name, int teamSize, CsItem primary, CsItem secondary,
                     bool usePreferredPrimary, WeaponType primaryPref,
                     bool usePreferredSecondary, bool armor, bool helmet,
                     bool enabledByDefault, RoundFunction startFunc, RoundFunction endFunc)
    : ID(-1)  // Will be set by manager
    , Name(name)
    , TeamSize(teamSize)
    , PrimaryWeapon(primary)
    , SecondaryWeapon(secondary)
    , UsePreferredPrimary(usePreferredPrimary)
    , PrimaryPreference(primaryPref)
    , UsePreferredSecondary(usePreferredSecondary)
    , Armor(armor)
    , Helmet(helmet)
    , EnabledByDefault(enabledByDefault)
    , StartFunction(startFunc)
    , EndFunction(endFunc)
{
}

// Static round type definitions (matching K4-Arenas)
RoundType RoundTypeManager::s_Rifle("k4.rounds.rifle", 1, CsItem::None, CsItem::None, true, WeaponType::Rifle, true, true, true, true);
RoundType RoundTypeManager::s_Sniper("k4.rounds.sniper", 1, CsItem::None, CsItem::None, true, WeaponType::Sniper, true, true, true, true);
RoundType RoundTypeManager::s_Shotgun("k4.rounds.shotgun", 1, CsItem::None, CsItem::None, true, WeaponType::Shotgun, true, true, true, true);
RoundType RoundTypeManager::s_Pistol("k4.rounds.pistol", 1, CsItem::None, CsItem::None, false, WeaponType::Unknown, true, true, true, true);
RoundType RoundTypeManager::s_Scout("k4.rounds.scout", 1, CsItem::Scout, CsItem::None, false, WeaponType::Unknown, true, true, true, true);
RoundType RoundTypeManager::s_AWP("k4.rounds.awp", 1, CsItem::AWP, CsItem::None, false, WeaponType::Unknown, true, true, true, true);
RoundType RoundTypeManager::s_Deagle("k4.rounds.deagle", 1, CsItem::None, CsItem::Deagle, false, WeaponType::Unknown, false, false, false, true);
RoundType RoundTypeManager::s_SMG("k4.rounds.smg", 1, CsItem::None, CsItem::None, true, WeaponType::SMG, true, true, true, true);
RoundType RoundTypeManager::s_LMG("k4.rounds.lmg", 1, CsItem::None, CsItem::None, true, WeaponType::LMG, true, true, true, true);
RoundType RoundTypeManager::s_Knife("k4.rounds.knife", 1, CsItem::None, CsItem::None, false, WeaponType::Unknown, false, false, false, true);
RoundType RoundTypeManager::s_TwoVSTwo("k4.rounds.2vs2", 2, CsItem::None, CsItem::None, true, WeaponType::Unknown, true, true, true, true);
RoundType RoundTypeManager::s_ThreeVSThree("k4.rounds.3vs3", 3, CsItem::None, CsItem::None, true, WeaponType::Unknown, true, true, true, true);

// Special fun round types
RoundType RoundTypeManager::s_HeadshotOnly("Headshot Only", 1, CsItem::None, CsItem::None, true, WeaponType::Rifle, true, true, true, true);
RoundType RoundTypeManager::s_ZeusOnly("Zeus Only", 1, CsItem::Zeus, CsItem::None, false, WeaponType::Unknown, false, false, false, true);
RoundType RoundTypeManager::s_ScoutOnly("Scout Only", 1, CsItem::Scout, CsItem::None, false, WeaponType::Unknown, false, true, true, true);
RoundType RoundTypeManager::s_DeagleOnly("Deagle Only", 1, CsItem::None, CsItem::Deagle, false, WeaponType::Unknown, false, true, true, true);
RoundType RoundTypeManager::s_KnifeOnly("Knife", 1, CsItem::None, CsItem::None, false, WeaponType::Unknown, false, false, false, true);

RoundTypeManager& RoundTypeManager::Instance()
{
    static RoundTypeManager instance;
    return instance;
}

RoundTypeManager::RoundTypeManager()
    : m_NextID(0)
{
}

void RoundTypeManager::Initialize()
{
    ResetRoundTypes();
}

RoundType* RoundTypeManager::FindByID(int id)
{
    for (auto& rt : m_RoundTypes)
    {
        if (rt.ID == id)
            return &rt;
    }
    return nullptr;
}

RoundType* RoundTypeManager::FindByName(const char* name)
{
    for (auto& rt : m_RoundTypes)
    {
        if (rt.Name == name)
            return &rt;
    }
    return nullptr;
}

void RoundTypeManager::AddRoundType(const RoundType& roundType)
{
    RoundType newRound = roundType;
    newRound.ID = m_NextID++;
    m_RoundTypes.push_back(newRound);
}

int RoundTypeManager::AddSpecialRoundType(const char* name, int teamSize, bool enabledByDefault,
                                          RoundFunction startFunc, RoundFunction endFunc)
{
    RoundType specialRound(name, teamSize, CsItem::None, CsItem::None,
                          false, WeaponType::Unknown, false, false, false,
                          enabledByDefault, startFunc, endFunc);
    specialRound.ID = m_NextID++;
    m_RoundTypes.push_back(specialRound);
    return specialRound.ID;
}

void RoundTypeManager::RemoveSpecialRoundType(int id)
{
    m_RoundTypes.erase(
        std::remove_if(m_RoundTypes.begin(), m_RoundTypes.end(),
            [id](const RoundType& rt) { return rt.ID == id; }),
        m_RoundTypes.end()
    );
}

void RoundTypeManager::ClearRoundTypes()
{
    m_RoundTypes.clear();
    m_NextID = 0;
}

void RoundTypeManager::ResetRoundTypes()
{
    ClearRoundTypes();

    // AWP server policy: never expose the generic K4 round catalogue here.
    // Player preferences are built from this list, so stale database entries
    // cannot bring rifles/SMGs back into an AWP server after a reload.
    AddRoundType(s_AWP);

    if (g_Config.AWPMode.Mode == "awp_rotation")
    {
        if (g_Config.AWPMode.AllowScout)
            AddRoundType(s_ScoutOnly);
        if (g_Config.AWPMode.AllowDeagle)
            AddRoundType(s_DeagleOnly);
    }

    if (g_Config.AWPMode.AllowKnife)
        AddRoundType(s_KnifeOnly);
}
