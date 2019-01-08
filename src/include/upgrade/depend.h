//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name depend.h - The dependencies header file. */
//
//      (c) Copyright 2000-2019 by Vladi Belperchinov-Shabanski and Andrettin
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//

#ifndef __DEPEND_H__
#define __DEPEND_H__

/*----------------------------------------------------------------------------
--  Documentation
----------------------------------------------------------------------------*/

/**
**  @struct DependRule depend.h
**
**  \#include "upgrade/depend.h"
**
**  This structure is used define the requirements of upgrades or
**  unit-types. The structure is used to define the base (the wanted)
**  upgrade or unit-type and the requirements upgrades or unit-types.
**  The requirements could be combination of and-rules and or-rules.
**
**  This structure is very complex because nearly everything has two
**  meanings.
**
**  The depend-rule structure members:
**
**  DependRule::Next
**
**    Next rule in hash chain for the base upgrade/unit-type.
**    Next and-rule for the requirements.
**
**  DependRule::Count
**
**    If DependRule::Type is DependRuleUnitType, the counter is
**    how many units of the unit-type are required, if zero no unit
**    of this unit-type is allowed. if DependRule::Type is
**    DependRuleUpgrade, for a non-zero counter the upgrade must be
**    researched, for a zero counter the upgrade must be unresearched.
**
**  DependRule::Type
**
**    Type of the rule, DependRuleUnitType for an unit-type,
**    DependRuleUpgrade for an upgrade.
**
**  DependRule::Kind
**
**    Contains the element of rule. Depending on DependRule::Type.
**
**  DependRule::Kind::UnitType
**
**    An unit-type pointer.
**
**  DependRule::Kind::Upgrade
**
**    An upgrade pointer.
**
**  DependRule::Rule
**
**    For the base upgrade/unit-type the rules which must be meet.
**    For the requirements alternative or-rules.
**
*/

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CAge;
class CConfigData;
class CPlayer;
class CSeason;
class CTrigger;
class CUnitType;
class CUnit;
class CUpgrade;
class ButtonAction;

enum {
	DependRuleUnitType,		/// Kind is a unit-type
	DependRuleUpgrade,		/// Kind is an upgrade
	DependRuleAge,			/// Kind is an age
	DependRuleSeason,		/// Kind is a season
	DependRuleTrigger		/// Kind is a trigger
};

/// Dependency rule
class DependRule
{
public:
	static void ProcessConfigData(const CConfigData *config_data, const int rule_type, const std::string &target);

	DependRule *Next;			/// next hash chain, or rules
	unsigned char Count;		/// how many required
	char Type;					/// a unit-type, upgrade or etc.
	union {
		const CUnitType *UnitType;	/// unit-type pointer
		const CUpgrade  *Upgrade;	/// upgrade pointer
		const CAge *Age;			/// age pointer
		const CSeason *Season;		/// season pointer
		const CTrigger *Trigger;	/// trigger pointer
	} Kind;						/// required object
	DependRule *Rule;			/// requirements, and rule
};

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/// Register CCL features for dependencies
extern void DependenciesCclRegister();
/// Init the dependencies
extern void InitDependencies();
/// Cleanup dependencies module
extern void CleanDependencies();

/// Print all unit dependencies into string
extern std::string PrintDependencies(const CPlayer &player, const ButtonAction &button);
extern void AddDependency(const int rule_type, const std::string &target, const int required_rule_type, const std::string &required, const int count, const int or_flag, const bool is_predependency);
/// Check a dependency by identifier
extern bool CheckDependByIdent(const CPlayer &player, const int rule_type, const std::string &target, bool ignore_units = false, bool is_predependency = false, bool is_neutral_use = false);
extern bool CheckDependByIdent(const CUnit &unit, const int rule_type, const std::string &target, bool ignore_units = false, bool is_predependency = false);
/// Check a dependency by unit type
extern bool CheckDependByType(const CPlayer &player, const CUnitType &type, bool ignore_units = false, bool is_predependency = false);
extern bool CheckDependByType(const CUnit &unit, const CUnitType &type, bool ignore_units = false, bool is_predependency = false);

#endif
