#ifndef _CS2_AWP_MODES_ROUND_TYPE_H_
#define _CS2_AWP_MODES_ROUND_TYPE_H_

#include "main.h"
#include <vector>
#include <functional>

class CCSPlayerController;

// Start/End function type for special rounds
using RoundFunction = std::function<void(std::vector<CCSPlayerController*>*, std::vector<CCSPlayerController*>*)>;

// Round type structure (matching K4-Arenas ArenaRoundTypeModel.cs)
struct RoundType
{
    int ID;
    std::string Name;              // Translation key like "k4.rounds.rifle"
    int TeamSize;
    CsItem PrimaryWeapon;          // Specific weapon or None
    CsItem SecondaryWeapon;        // Specific weapon or None
    bool UsePreferredPrimary;      // Use player's preferred weapon
    WeaponType PrimaryPreference;  // Weapon type for preferred
    bool UsePreferredSecondary;
    bool Armor;
    bool Helmet;
    bool EnabledByDefault;
    RoundFunction StartFunction;   // Optional special round start
    RoundFunction EndFunction;     // Optional special round end

    RoundType(const char* name, int teamSize, CsItem primary, CsItem secondary,
              bool usePreferredPrimary = false, WeaponType primaryPref = WeaponType::Unknown,
              bool usePreferredSecondary = false, bool armor = true, bool helmet = true,
              bool enabledByDefault = true, RoundFunction startFunc = nullptr,
              RoundFunction endFunc = nullptr);
};

// Round type manager
class RoundTypeManager
{
public:
    static RoundTypeManager& Instance();

    // Get all round types
    std::vector<RoundType>& GetRoundTypes() { return m_RoundTypes; }
    const std::vector<RoundType>& GetRoundTypes() const { return m_RoundTypes; }

    // Find round type by ID
    RoundType* FindByID(int id);

    // Find round type by name
    RoundType* FindByName(const char* name);

    // Add custom round type
    void AddRoundType(const RoundType& roundType);

    // Add special round type (with start/end functions)
    int AddSpecialRoundType(const char* name, int teamSize, bool enabledByDefault,
                            RoundFunction startFunc, RoundFunction endFunc);

    // Remove special round type
    void RemoveSpecialRoundType(int id);

    // Clear all round types
    void ClearRoundTypes();

    // Reset to default round types
    void ResetRoundTypes();

    // Initialize default round types
    void Initialize();

private:
    RoundTypeManager();

    std::vector<RoundType> m_RoundTypes;
    int m_NextID;

    // Default round types
    static RoundType s_Rifle;
    static RoundType s_Sniper;
    static RoundType s_Shotgun;
    static RoundType s_Pistol;
    static RoundType s_Scout;
    static RoundType s_AWP;
    static RoundType s_Deagle;
    static RoundType s_SMG;
    static RoundType s_LMG;
    static RoundType s_Knife;
    static RoundType s_TwoVSTwo;
    static RoundType s_ThreeVSThree;
    static RoundType s_HeadshotOnly;
    static RoundType s_ZeusOnly;
    static RoundType s_ScoutOnly;
    static RoundType s_DeagleOnly;
    static RoundType s_KnifeOnly;
};

#endif // _CS2_AWP_MODES_ROUND_TYPE_H_
