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
/**@name upgrade.cpp - The upgrade/allow functions. */
//
//      (c) Copyright 1999-2007 by Vladi Belperchinov-Shabanski and Jimmy Salmon
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <string>
#include <vector>
#include <map>

#include "stratagus.h"

#include "upgrade.h"

//Wyrmgus start
#include "action/action_build.h"
//Wyrmgus end
#include "action/action_train.h"
//Wyrmgus start
#include "../ai/ai_local.h"
//Wyrmgus end
#include "commands.h"
#include "depend.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "interface.h"
#include "iolib.h"
#include "map.h"
#include "player.h"
#include "script.h"
//Wyrmgus start
#include "settings.h"
#include "tileset.h"
//Wyrmgus end
#include "unit.h"
#include "unit_find.h"
//Wyrmgus start
#include "ui.h"
#include "unit_manager.h"
//Wyrmgus end
#include "unittype.h"
#include "util.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

//Wyrmgus start
//static void AllowUnitId(CPlayer &player, int id, int units);
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CUpgrade *> AllUpgrades;           /// The main user useable upgrades

/// How many upgrades modifiers supported
//Wyrmgus start
//#define UPGRADE_MODIFIERS_MAX (UpgradeMax * 4)
/// Upgrades modifiers
//static CUpgradeModifier *UpgradeModifiers[UPGRADE_MODIFIERS_MAX];
CUpgradeModifier *UpgradeModifiers[UPGRADE_MODIFIERS_MAX];
//Wyrmgus end
/// Number of upgrades modifiers used
//Wyrmgus start
//static int NumUpgradeModifiers;
int NumUpgradeModifiers;
//Wyrmgus end

std::map<std::string, CUpgrade *> Upgrades;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/


CUnitStats::~CUnitStats()
{
	delete [] this->Variables;
}

const CUnitStats &CUnitStats::operator = (const CUnitStats &rhs)
{
	for (unsigned int i = 0; i < MaxCosts; ++i) {
		this->Costs[i] = rhs.Costs[i];
		this->Storing[i] = rhs.Storing[i];
		//Wyrmgus start
		this->ImproveIncomes[i] = rhs.ImproveIncomes[i];
		//Wyrmgus end
	}
	delete [] this->Variables;
	const unsigned int size = UnitTypeVar.GetNumberVariable();
	this->Variables = new CVariable[size];

	std::copy(rhs.Variables, rhs.Variables + size, this->Variables);
	return *this;
}

bool CUnitStats::operator == (const CUnitStats &rhs) const
{
	for (int i = 0; i != MaxCosts; ++i) {
		if (this->Costs[i] != rhs.Costs[i]) {
			return false;
		}
		if (this->Storing[i] != rhs.Storing[i]) {
			return false;
		}
		//Wyrmgus start
		if (this->ImproveIncomes[i] != rhs.ImproveIncomes[i]) {
			return false;
		}
		//Wyrmgus end
	}
	for (unsigned int i = 0; i != UnitTypeVar.GetNumberVariable(); ++i) {
		if (this->Variables[i] != rhs.Variables[i]) {
			return false;
		}
	}
	return true;
}

bool CUnitStats::operator != (const CUnitStats &rhs) const
{
	return !(*this == rhs);
}

CUpgrade::CUpgrade(const std::string &ident) :
	Ident(ident), ID(0)
{
	memset(this->Costs, 0, sizeof(this->Costs));
	//Wyrmgus start
	memset(this->GrandStrategyCosts, 0, sizeof(this->GrandStrategyCosts));
	//Wyrmgus end
}

CUpgrade::~CUpgrade()
{
}

/**
**  Create a new upgrade
**
**  @param ident  Upgrade identifier
*/
CUpgrade *CUpgrade::New(const std::string &ident)
{
	CUpgrade *upgrade = Upgrades[ident];
	if (upgrade) {
		return upgrade;
	} else {
		upgrade = new CUpgrade(ident);
		Upgrades[ident] = upgrade;
		upgrade->ID = AllUpgrades.size();
		AllUpgrades.push_back(upgrade);
		return upgrade;
	}
}

/**
**  Get an upgrade
**
**  @param ident  Upgrade identifier
**
**  @return       Upgrade pointer or NULL if not found.
*/
CUpgrade *CUpgrade::Get(const std::string &ident)
{
	CUpgrade *upgrade = Upgrades[ident];
	if (!upgrade) {
		DebugPrint("upgrade not found: %s\n" _C_ ident.c_str());
	}
	return upgrade;
}

/**
**  Init upgrade/allow structures
*/
void InitUpgrades()
{
}

/**
**  Cleanup the upgrade module.
*/
void CleanUpgrades()
{
	//  Free the upgrades.
	while (AllUpgrades.empty() == false) {
		CUpgrade *upgrade = AllUpgrades.back();
		AllUpgrades.pop_back();
		delete upgrade;
	}
	Upgrades.clear();

	//
	//  Free the upgrade modifiers.
	//
	for (int i = 0; i < NumUpgradeModifiers; ++i) {
		delete UpgradeModifiers[i];
	}
	NumUpgradeModifiers = 0;
}

/**
**  Save state of the dependencies to file.
**
**  @param file  Output file.
*/
void SaveUpgrades(CFile &file)
{
	file.printf("\n-- -----------------------------------------\n");
	file.printf("-- MODULE: upgrades\n\n");

	//
	//  Save the allow
	//
	for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
		file.printf("DefineUnitAllow(\"%s\", ", UnitTypes[i]->Ident.c_str());
		for (int p = 0; p < PlayerMax; ++p) {
			if (p) {
				file.printf(", ");
			}
			file.printf("%d", Players[p].Allow.Units[i]);
		}
		file.printf(")\n");
	}
	file.printf("\n");

	//
	//  Save the upgrades
	//
	for (std::vector<CUpgrade *>::size_type j = 0; j < AllUpgrades.size(); ++j) {
		file.printf("DefineAllow(\"%s\", \"", AllUpgrades[j]->Ident.c_str());
		for (int p = 0; p < PlayerMax; ++p) {
			file.printf("%c", Players[p].Allow.Upgrades[j]);
		}
		file.printf("\")\n");
	}
}

/*----------------------------------------------------------------------------
--  Ccl part of upgrades
----------------------------------------------------------------------------*/

/**
**  Define a new upgrade modifier.
**
**  @param l  List of modifiers.
*/
static int CclDefineModifier(lua_State *l)
{
	const int args = lua_gettop(l);

	CUpgradeModifier *um = new CUpgradeModifier;

	memset(um->ChangeUpgrades, '?', sizeof(um->ChangeUpgrades));
	memset(um->ApplyTo, '?', sizeof(um->ApplyTo));
	um->Modifier.Variables = new CVariable[UnitTypeVar.GetNumberVariable()];
	um->ModifyPercent = new int[UnitTypeVar.GetNumberVariable()];
	memset(um->ModifyPercent, 0, UnitTypeVar.GetNumberVariable() * sizeof(int));

	um->UpgradeId = UpgradeIdByIdent(LuaToString(l, 1));

	for (int j = 1; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		const char *key = LuaToString(l, j + 1, 1);
#if 0 // To be removed. must modify lua file.
		if (!strcmp(key, "attack-range")) {
			key = "AttackRange";
		} else if (!strcmp(key, "sight-range")) {
			key = "SightRange";
		} else if (!strcmp(key, "basic-damage")) {
			key = "BasicDamage";
		} else if (!strcmp(key, "piercing-damage")) {
			key = "PiercingDamage";
		} else if (!strcmp(key, "armor")) {
			key = "Armor";
		} else if (!strcmp(key, "hit-points")) {
			key = "HitPoints";
		}
#endif
		if (!strcmp(key, "regeneration-rate")) {
			um->Modifier.Variables[HP_INDEX].Increase = LuaToNumber(l, j + 1, 2);
		} else if (!strcmp(key, "cost")) {
			if (!lua_istable(l, j + 1) || lua_rawlen(l, j + 1) != 2) {
				LuaError(l, "incorrect argument");
			}
			const char *value = LuaToString(l, j + 1, 1);
			const int resId = GetResourceIdByName(l, value);
			um->Modifier.Costs[resId] = LuaToNumber(l, j + 1, 2);
		} else if (!strcmp(key, "storing")) {
			if (!lua_istable(l, j + 1) || lua_rawlen(l, j + 1) != 2) {
				LuaError(l, "incorrect argument");
			}
			const char *value = LuaToString(l, j + 1, 1);
			const int resId = GetResourceIdByName(l, value);
			um->Modifier.Storing[resId] = LuaToNumber(l, j + 1, 2);
		//Wyrmgus start
		} else if (!strcmp(key, "improve-production")) {
			const char *value = LuaToString(l, j + 1, 2);
			const int resId = GetResourceIdByName(l, value);
			um->Modifier.ImproveIncomes[resId] = LuaToNumber(l, j + 1, 3);
		//Wyrmgus end
		} else if (!strcmp(key, "allow-unit")) {
			const char *value = LuaToString(l, j + 1, 2);

			if (!strncmp(value, "unit-", 5)) {
				um->ChangeUnits[UnitTypeIdByIdent(value)] = LuaToNumber(l, j + 1, 3);
			} else {
				LuaError(l, "unit expected");
			}
		} else if (!strcmp(key, "allow")) {
			const char *value = LuaToString(l, j + 1, 2);
			if (!strncmp(value, "upgrade-", 8)) {
				um->ChangeUpgrades[UpgradeIdByIdent(value)] = LuaToNumber(l, j + 1, 3);
			} else {
				LuaError(l, "upgrade expected");
			}
		} else if (!strcmp(key, "apply-to")) {
			const char *value = LuaToString(l, j + 1, 2);
			um->ApplyTo[UnitTypeIdByIdent(value)] = 'X';
		} else if (!strcmp(key, "convert-to")) {
			const char *value = LuaToString(l, j + 1, 2);
			um->ConvertTo = UnitTypeByIdent(value);
		//Wyrmgus start
		} else if (!strcmp(key, "research-speed")) {
			um->SpeedResearch = LuaToNumber(l, j + 1, 2);
		} else if (!strcmp(key, "change-civilization-to")) {
			const char *civilization_name = LuaToString(l, j + 1, 2);
			um->ChangeCivilizationTo = PlayerRaces.GetRaceIndexByName(civilization_name);
			
			if (um->ChangeCivilizationTo == -1) {
				LuaError(l, "invalid civilization name '%s'" _C_ civilization_name);
			}
		//Wyrmgus end
		} else {
			int index = UnitTypeVar.VariableNameLookup[key]; // variable index;
			if (index != -1) {
				if (lua_rawlen(l, j + 1) == 3) {
					const char *value = LuaToString(l, j + 1, 3);
					if (!strcmp(value, "Percent")) {
						um->ModifyPercent[index] = LuaToNumber(l, j + 1, 2);
					}
				} else {
					lua_rawgeti(l, j + 1, 2);
					if (lua_istable(l, -1)) {
						DefineVariableField(l, um->Modifier.Variables + index, -1);
					} else if (lua_isnumber(l, -1)) {
						um->Modifier.Variables[index].Enable = 1;
						um->Modifier.Variables[index].Value = LuaToNumber(l, -1);
						um->Modifier.Variables[index].Max = LuaToNumber(l, -1);
					} else {
						LuaError(l, "bad argument type for '%s'\n" _C_ key);
					}
					lua_pop(l, 1);
				}
			} else {
				LuaError(l, "wrong tag: %s" _C_ key);
			}
		}
	}

	UpgradeModifiers[NumUpgradeModifiers++] = um;
	
	//Wyrmgus start
	//set the upgrade's civilization and class here, for lack of a better place
	if (!AllUpgrades[um->UpgradeId]->Class.empty()) { //if class is defined, then use this upgrade to help build the classes table, and add this upgrade to the civilization class table (if the civilization is defined)
		int class_id = -1;
		for (unsigned int i = 0; i != UnitTypeClassMax; ++i) {
			if (UpgradeClasses[i] == AllUpgrades[um->UpgradeId]->Class) {
				class_id = i;
				break;
			}
			if (UpgradeClasses[i].empty()) { //if reached a blank slot, then the class isn't recorded yet; do so now
				UpgradeClasses[i] = AllUpgrades[um->UpgradeId]->Class;
				SetUpgradeClassStringToIndex(AllUpgrades[um->UpgradeId]->Class, i);
				class_id = i;
				break;
			}
		}
		if (!AllUpgrades[um->UpgradeId]->Civilization.empty()) {
			int civilization_id = PlayerRaces.GetRaceIndexByName(AllUpgrades[um->UpgradeId]->Civilization.c_str());
			if (civilization_id != -1 && class_id != -1) {
				PlayerRaces.CivilizationClassUpgrades[civilization_id][class_id] = um->UpgradeId;
			}
		}
	}
	//Wyrmgus end

	return 0;
}

/**
**  Define which units are allowed and how much.
*/
static int CclDefineUnitAllow(lua_State *l)
{
	const int args = lua_gettop(l);

	const char *ident = LuaToString(l, 0 + 1);

	if (strncmp(ident, "unit-", 5)) {
		DebugPrint(" wrong ident %s\n" _C_ ident);
		return 0;
	}
	int id = UnitTypeIdByIdent(ident);

	int i = 0;
	for (int j = 1; j < args && i < PlayerMax; ++j) {
		AllowUnitId(Players[i], id, LuaToNumber(l, j + 1));
		++i;
	}
	return 0;
}

/**
**  Define which units/upgrades are allowed.
*/
static int CclDefineAllow(lua_State *l)
{
	const int UnitMax = 65536; /// How many units supported
	const int args = lua_gettop(l);

	for (int j = 0; j < args; ++j) {
		const char *ident = LuaToString(l, j + 1);
		++j;
		const char *ids = LuaToString(l, j + 1);

		int n = strlen(ids);
		if (n > PlayerMax) {
			fprintf(stderr, "%s: Allow string too long %d\n", ident, n);
			n = PlayerMax;
		}

		if (!strncmp(ident, "unit-", 5)) {
			int id = UnitTypeIdByIdent(ident);
			for (int i = 0; i < n; ++i) {
				if (ids[i] == 'A') {
					AllowUnitId(Players[i], id, UnitMax);
				} else if (ids[i] == 'F') {
					AllowUnitId(Players[i], id, 0);
				}
			}
		} else if (!strncmp(ident, "upgrade-", 8)) {
			int id = UpgradeIdByIdent(ident);
			for (int i = 0; i < n; ++i) {
				AllowUpgradeId(Players[i], id, ids[i]);
			}
		} else {
			DebugPrint(" wrong ident %s\n" _C_ ident);
		}
	}
	return 0;
}

//Wyrmgus start
/**
** Acquire an ability
*/
static int CclAcquireAbility(lua_State *l)
{
	LuaCheckArgs(l, 2);
	
	if (lua_isnil(l, 1)) {
		return 0;
	}
	
	lua_pushvalue(l, 1);
	CUnit *unit = &UnitManager.GetSlotUnit(LuaToNumber(l, -1));
	lua_pop(l, 1);
	const char *ident = LuaToString(l, 2);
	if (!strncmp(ident, "upgrade-", 8)) {
		AbilityAcquire(*unit, CUpgrade::Get(ident));
	} else {
		DebugPrint(" wrong ident %s\n" _C_ ident);
	}
	return 0;
}

/**
** Set the unit's trait
*/
static int CclAcquireTrait(lua_State *l)
{
	LuaCheckArgs(l, 2);
	
	if (lua_isnil(l, 1)) {
		return 0;
	}
	
	lua_pushvalue(l, 1);
	CUnit *unit = &UnitManager.GetSlotUnit(LuaToNumber(l, -1));
	lua_pop(l, 1);
	const char *ident = LuaToString(l, 2);
	if (!strncmp(ident, "upgrade-", 8)) {
		TraitAcquire(*unit, CUpgrade::Get(ident));
		unit->Trait = ident;
	} else if (ident == "") {
		if (!unit->Trait.empty()) { //remove previous trait, if any
			if (!GameSettings.NoRandomness || unit->Type->BoolFlag[HERO_INDEX].value) { // if in no randomness setting, only change trait modifiers if the unit is a hero
				IndividualUpgradeLost(*unit, CUpgrade::Get(unit->Trait));
			}
		}
		unit->Trait = ident;
	} else {
		DebugPrint(" wrong ident %s\n" _C_ ident);
	}
	return 0;
}

/**
** Add an individual upgrade to the unit
*/
static int CclAcquireIndividualUpgrade(lua_State *l)
{
	LuaCheckArgs(l, 2);
	lua_pushvalue(l, 1);
	CUnit *unit = &UnitManager.GetSlotUnit(LuaToNumber(l, -1));
	lua_pop(l, 1);
	const char *ident = LuaToString(l, 2);
	if (!strncmp(ident, "upgrade-", 8)) {
		IndividualUpgradeAcquire(*unit, CUpgrade::Get(ident));
	} else {
		DebugPrint(" wrong ident %s\n" _C_ ident);
	}
	return 0;
}
//Wyrmgus end

/**
**  Register CCL features for upgrades.
*/
void UpgradesCclRegister()
{
	lua_register(Lua, "DefineModifier", CclDefineModifier);
	lua_register(Lua, "DefineAllow", CclDefineAllow);
	lua_register(Lua, "DefineUnitAllow", CclDefineUnitAllow);
	//Wyrmgus start
	lua_register(Lua, "AcquireAbility", CclAcquireAbility);
	lua_register(Lua, "AcquireTrait", CclAcquireTrait);
	lua_register(Lua, "AcquireIndividualUpgrade", CclAcquireIndividualUpgrade);
	//Wyrmgus end
}

/*----------------------------------------------------------------------------
-- General/Map functions
----------------------------------------------------------------------------*/

// AllowStruct and UpgradeTimers will be static in the player so will be
// load/saved with the player struct

/**
**  UnitType ID by identifier.
**
**  @param ident  The unit-type identifier.
**  @return       Unit-type ID (int) or -1 if not found.
*/
int UnitTypeIdByIdent(const std::string &ident)
{
	const CUnitType *type = UnitTypeByIdent(ident);

	if (type) {
		return type->Slot;
	}
	DebugPrint(" fix this %s\n" _C_ ident.c_str());
	Assert(0);
	return -1;
}

/**
**  Upgrade ID by identifier.
**
**  @param ident  The upgrade identifier.
**  @return       Upgrade ID (int) or -1 if not found.
*/
int UpgradeIdByIdent(const std::string &ident)
{
	const CUpgrade *upgrade = CUpgrade::Get(ident);

	if (upgrade) {
		return upgrade->ID;
	}
	DebugPrint(" fix this %s\n" _C_ ident.c_str());
	return -1;
}

/*----------------------------------------------------------------------------
-- Upgrades
----------------------------------------------------------------------------*/

/**
**  Convert unit-type to.
**
**  @param player  For this player.
**  @param src     From this unit-type.
**  @param dst     To this unit-type.
*/
static void ConvertUnitTypeTo(CPlayer &player, const CUnitType &src, CUnitType &dst)
{
	//Wyrmgus start
	player.Allow.Units[src.Slot] = 0; //forbid the previous unit type when converting
	
	if (player.AiEnabled && GameCycle > 0) {
		//if is AI player, convert all requests from the old unit type to the new one; FIXME: if already has requests of the new unit type, then the count of the old one should be added to the new one, instead of merely changing the type of the old one to the new one
		for (unsigned int i = 0; i < player.Ai->UnitTypeRequests.size(); ++i) {
			if (player.Ai->UnitTypeRequests[i].Type->Slot == src.Slot) {
				player.Ai->UnitTypeRequests[i].Type = &dst;
			}
		}

		for (unsigned int i = 0; i < player.Ai->UpgradeToRequests.size(); ++i) {
			if (player.Ai->UpgradeToRequests[i]->Slot == src.Slot) {
				player.Ai->UpgradeToRequests[i] = &dst;
			}
		}
		
		for (unsigned int i = 0; i < player.Ai->Force.Size(); ++i) {
			AiForce &force = player.Ai->Force[i];

			for (unsigned int j = 0; j < force.UnitTypes.size(); ++j) {
				if (force.UnitTypes[j].Type->Slot == src.Slot) {
					force.UnitTypes[j].Type = &dst;
				}
			}
		}

		for (unsigned int i = 0; i < player.Ai->UnitTypeBuilt.size(); ++i) {
			if (player.Ai->UnitTypeBuilt[i].Type->Slot == src.Slot) {
				player.Ai->UnitTypeBuilt[i].Type = &dst;
			}
		}
	}
	//Wyrmgus end
	for (int i = 0; i < player.GetUnitCount(); ++i) {
		CUnit &unit = player.GetUnit(i);

		//  Convert already existing units to this type.
		if (unit.Type == &src) {
			CommandTransformIntoType(unit, dst);
			//  Convert trained units to this type.
			//  FIXME: what about buildings?
		//Wyrmgus start
//		} else {
		} else if (GameCycle > 0) {
		//Wyrmgus end
			for (size_t j = 0; j < unit.Orders.size(); ++j) {
				if (unit.Orders[j]->Action == UnitActionTrain) {
					COrder_Train &order = *static_cast<COrder_Train *>(unit.Orders[j]);

					if (&order.GetUnitType() == &src) {
						order.ConvertUnitType(unit, dst);
					}
				//Wyrmgus start
				// convert building orders as well
				} else if (unit.Orders[j]->Action == UnitActionBuild) {
					COrder_Build &order = *static_cast<COrder_Build *>(unit.Orders[j]);

					if (&order.GetUnitType() == &src) {
						order.ConvertUnitType(unit, dst);
					}
				//Wyrmgus end
				}
			}
		}
	}
}

/**
**  Apply the modifiers of an upgrade.
**
**  This function will mark upgrade done and do all required modifications
**  to unit types and will modify allow/forbid maps
**
**  @param player  Player that get all the upgrades.
**  @param um      Upgrade modifier that do the effects
*/
static void ApplyUpgradeModifier(CPlayer &player, const CUpgradeModifier *um)
{
	Assert(um);

	int pn = player.Index;

	//Wyrmgus start
	if (um->SpeedResearch != 0) {
		player.SpeedResearch += um->SpeedResearch;
	}
	if (um->ChangeCivilizationTo != -1) {
		player.Race = um->ChangeCivilizationTo;
		
		//if the civilization of the person player changed, update the UI
		if (ThisPlayer && ThisPlayer->Index == player.Index) {
			LoadCursors(PlayerRaces.Name[player.Race]);
			UI.Load();
		}
		
		// set random faction from new civilization
		player.SetRandomFaction();
	}
	//Wyrmgus end

	for (int z = 0; z < UpgradeMax; ++z) {
		// allow/forbid upgrades for player.  only if upgrade is not acquired

		// FIXME: check if modify is allowed

		if (player.Allow.Upgrades[z] != 'R') {
			if (um->ChangeUpgrades[z] == 'A') {
				player.Allow.Upgrades[z] = 'A';
			}
			if (um->ChangeUpgrades[z] == 'F') {
				player.Allow.Upgrades[z] = 'F';
			}
			// we can even have upgrade acquired w/o costs
			if (um->ChangeUpgrades[z] == 'R') {
				player.Allow.Upgrades[z] = 'R';
			}
		}
	}

	for (size_t z = 0; z < UnitTypes.size(); ++z) {
		CUnitStats &stat = UnitTypes[z]->Stats[pn];
		// add/remove allowed units

		// FIXME: check if modify is allowed

		player.Allow.Units[z] += um->ChangeUnits[z];

		Assert(um->ApplyTo[z] == '?' || um->ApplyTo[z] == 'X');

		// this modifier should be applied to unittype id == z
		if (um->ApplyTo[z] == 'X') {

			// If Sight range is upgraded, we need to change EVERY unit
			// to the new range, otherwise the counters get confused.
			if (um->Modifier.Variables[SIGHTRANGE_INDEX].Value) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*UnitTypes[z], unitupgrade);
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];
					if (unit.Player->Index == pn && !unit.Removed) {
						MapUnmarkUnitSight(unit);
						//Wyrmgus start
//						unit.CurrentSightRange = stat.Variables[SIGHTRANGE_INDEX].Max +
						unit.CurrentSightRange = unit.Variable[SIGHTRANGE_INDEX].Max +
						//Wyrmgus end
												 um->Modifier.Variables[SIGHTRANGE_INDEX].Value;
						//Wyrmgus start
						UpdateUnitSightRange(unit);
						//Wyrmgus end
						MapMarkUnitSight(unit);
					}
				}
			}
			
			//Wyrmgus start
			// if a unit type's supply is changed, we need to update the player's supply accordingly
			if (um->Modifier.Variables[SUPPLY_INDEX].Value) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*UnitTypes[z], unitupgrade);
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];
					if (unit.Player->Index == pn && unit.IsAlive()) {
						unit.Player->Supply += um->Modifier.Variables[SUPPLY_INDEX].Value;
					}
				}
			}
			
			// if a unit type's demand is changed, we need to update the player's demand accordingly
			if (um->Modifier.Variables[DEMAND_INDEX].Value) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*UnitTypes[z], unitupgrade);
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];
					if (unit.Player->Index == pn && unit.IsAlive()) {
						unit.Player->Demand += um->Modifier.Variables[DEMAND_INDEX].Value;
					}
				}
			}
			//Wyrmgus end
			
			// upgrade costs :)
			for (unsigned int j = 0; j < MaxCosts; ++j) {
				stat.Costs[j] += um->Modifier.Costs[j];
				stat.Storing[j] += um->Modifier.Storing[j];
				//Wyrmgus start
				if (um->Modifier.ImproveIncomes[j]) {
					if (!stat.ImproveIncomes[j]) {
						stat.ImproveIncomes[j] += DefaultIncomes[j] + um->Modifier.ImproveIncomes[j];
					} else {
						stat.ImproveIncomes[j] += um->Modifier.ImproveIncomes[j];
					}
					//update player's income
					std::vector<CUnit *> unitupgrade;
					FindUnitsByType(*UnitTypes[z], unitupgrade);
					if (unitupgrade.size() > 0) {
						player.Incomes[j] = std::max(player.Incomes[j], stat.ImproveIncomes[j]);
					}
				}
				//Wyrmgus end
			}

			int varModified = 0;
			for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
				varModified |= um->Modifier.Variables[j].Value
							   | um->Modifier.Variables[j].Max
							   | um->Modifier.Variables[j].Increase
							   | um->Modifier.Variables[j].Enable
							   | um->ModifyPercent[j];
				stat.Variables[j].Enable |= um->Modifier.Variables[j].Enable;
				if (um->ModifyPercent[j]) {
					stat.Variables[j].Value += stat.Variables[j].Value * um->ModifyPercent[j] / 100;
					stat.Variables[j].Max += stat.Variables[j].Max * um->ModifyPercent[j] / 100;
				} else {
					stat.Variables[j].Value += um->Modifier.Variables[j].Value;
					stat.Variables[j].Max += um->Modifier.Variables[j].Max;
					stat.Variables[j].Increase += um->Modifier.Variables[j].Increase;
				}

				stat.Variables[j].Max = std::max(stat.Variables[j].Max, 0);
				clamp(&stat.Variables[j].Value, 0, stat.Variables[j].Max);
			}

			// And now modify ingame units
			if (varModified) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*UnitTypes[z], unitupgrade, true);
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];

					if (unit.Player->Index != player.Index) {
						continue;
					}
					for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
						unit.Variable[j].Enable |= um->Modifier.Variables[j].Enable;
						if (um->ModifyPercent[j]) {
							unit.Variable[j].Value += unit.Variable[j].Value * um->ModifyPercent[j] / 100;
							unit.Variable[j].Max += unit.Variable[j].Max * um->ModifyPercent[j] / 100;
						} else {
							unit.Variable[j].Value += um->Modifier.Variables[j].Value;
							unit.Variable[j].Increase += um->Modifier.Variables[j].Increase;
						}

						unit.Variable[j].Max += um->Modifier.Variables[j].Max;
						unit.Variable[j].Max = std::max(unit.Variable[j].Max, 0);
						if (unit.Variable[j].Max > 0) {
							clamp(&unit.Variable[j].Value, 0, unit.Variable[j].Max);
						}
					}
					//Wyrmgus start
					//change variation if current one becomes forbidden
					VariationInfo *current_varinfo = UnitTypes[z]->VarInfo[unit.Variation];
					if (current_varinfo) {
						bool ForbiddenUpgrade = false;
						for (int u = 0; u < VariationMax; ++u) {
							if (!current_varinfo->UpgradesForbidden[u].empty() && um->UpgradeId == CUpgrade::Get(current_varinfo->UpgradesForbidden[u])->ID) {
								ForbiddenUpgrade = true;
								break;
							}
						}
						if (ForbiddenUpgrade == true) {
							int TypeVariationCount = 0;
							int LocalTypeVariations[VariationMax];
							for (int i = 0; i < VariationMax; ++i) {
								VariationInfo *varinfo = UnitTypes[z]->VarInfo[i];
								if (!varinfo) {
									continue;
								}
								if (!varinfo->Tileset.empty() && varinfo->Tileset != Map.Tileset->Name) {
									continue;
								}
								bool UpgradesCheck = true;
								for (int u = 0; u < VariationMax; ++u) {
									if (!varinfo->UpgradesRequired[u].empty() && UpgradeIdentAllowed(player, varinfo->UpgradesRequired[u].c_str()) != 'R' && unit.LearnedAbilities[CUpgrade::Get(varinfo->UpgradesRequired[u])->ID] == false) {
										UpgradesCheck = false;
										break;
									}
									if (!varinfo->UpgradesForbidden[u].empty() && (UpgradeIdentAllowed(player, varinfo->UpgradesForbidden[u].c_str()) == 'R' || unit.LearnedAbilities[CUpgrade::Get(varinfo->UpgradesForbidden[u])->ID] == true)) {
										UpgradesCheck = false;
										break;
									}
								}
								if (UpgradesCheck == false) {
									continue;
								}
								if (current_varinfo && varinfo->VariationId.find(current_varinfo->VariationId) != std::string::npos) { // if this variation includes the ident of the one incompatible with this upgrade, choose it automatically
									unit.Variation = i;
									TypeVariationCount = 0;
									break;
								}
								LocalTypeVariations[TypeVariationCount] = i;
								TypeVariationCount += 1;
							}
							if (TypeVariationCount > 0) {
								unit.Variation = LocalTypeVariations[SyncRand(TypeVariationCount)];
							}
						}
					}
					//Wyrmgus end
				}
			}
			if (um->ConvertTo) {
				ConvertUnitTypeTo(player, *UnitTypes[z], *um->ConvertTo);
			}
		}
	}
}

/**
**  Remove the modifiers of an upgrade.
**
**  This function will unmark upgrade as done and undo all required modifications
**  to unit types and will modify allow/forbid maps back
**
**  @param player  Player that get all the upgrades.
**  @param um      Upgrade modifier that do the effects
*/
static void RemoveUpgradeModifier(CPlayer &player, const CUpgradeModifier *um)
{
	Assert(um);

	int pn = player.Index;

	if (um->SpeedResearch != 0) {
		player.SpeedResearch -= um->SpeedResearch;
	}
	//Wyrmgus start
	if (um->ChangeCivilizationTo != -1) {
		player.Race = PlayerRaces.GetRaceIndexByName(AllUpgrades[um->UpgradeId]->Civilization.c_str()); // restore old civilization
		
		//if the civilization of the person player changed, update the UI
		if (ThisPlayer && ThisPlayer->Index == player.Index) {
			LoadCursors(PlayerRaces.Name[player.Race]);
			UI.Load();
		}
		
		// set random faction from the old civilization
		player.SetRandomFaction();
	}
	//Wyrmgus end

	for (int z = 0; z < UpgradeMax; ++z) {
		// allow/forbid upgrades for player.  only if upgrade is not acquired

		// FIXME: check if modify is allowed

		if (player.Allow.Upgrades[z] != 'R') {
			if (um->ChangeUpgrades[z] == 'A') {
				player.Allow.Upgrades[z] = 'F';
			}
			if (um->ChangeUpgrades[z] == 'F') {
				player.Allow.Upgrades[z] = 'A';
			}
			// we can even have upgrade acquired w/o costs
			if (um->ChangeUpgrades[z] == 'R') {
				player.Allow.Upgrades[z] = 'A';
			}
		}
	}

	for (size_t z = 0; z < UnitTypes.size(); ++z) {
		CUnitStats &stat = UnitTypes[z]->Stats[pn];
		// add/remove allowed units

		// FIXME: check if modify is allowed

		player.Allow.Units[z] -= um->ChangeUnits[z];

		Assert(um->ApplyTo[z] == '?' || um->ApplyTo[z] == 'X');

		// this modifier should be applied to unittype id == z
		if (um->ApplyTo[z] == 'X') {

			// If Sight range is upgraded, we need to change EVERY unit
			// to the new range, otherwise the counters get confused.
			if (um->Modifier.Variables[SIGHTRANGE_INDEX].Value) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*UnitTypes[z], unitupgrade);
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];
					if (unit.Player->Index == pn && !unit.Removed) {
						MapUnmarkUnitSight(unit);
						//Wyrmgus start
//						unit.CurrentSightRange = stat.Variables[SIGHTRANGE_INDEX].Max -
						unit.CurrentSightRange = unit.Variable[SIGHTRANGE_INDEX].Max -
						//Wyrmgus end
												 um->Modifier.Variables[SIGHTRANGE_INDEX].Value;
						//Wyrmgus start
						UpdateUnitSightRange(unit);
						//Wyrmgus end
						MapMarkUnitSight(unit);
					}
				}
			}
			
			//Wyrmgus start
			// if a unit type's supply is changed, we need to update the player's supply accordingly
			if (um->Modifier.Variables[SUPPLY_INDEX].Value) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*UnitTypes[z], unitupgrade);
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];
					if (unit.Player->Index == pn && unit.IsAlive()) {
						unit.Player->Supply -= um->Modifier.Variables[SUPPLY_INDEX].Value;
					}
				}
			}
			
			// if a unit type's demand is changed, we need to update the player's demand accordingly
			if (um->Modifier.Variables[DEMAND_INDEX].Value) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*UnitTypes[z], unitupgrade);
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];
					if (unit.Player->Index == pn && unit.IsAlive()) {
						unit.Player->Demand -= um->Modifier.Variables[DEMAND_INDEX].Value;
					}
				}
			}
			//Wyrmgus end
			
			// upgrade costs :)
			for (unsigned int j = 0; j < MaxCosts; ++j) {
				stat.Costs[j] -= um->Modifier.Costs[j];
				stat.Storing[j] -= um->Modifier.Storing[j];
				//Wyrmgus start
				stat.ImproveIncomes[j] -= um->Modifier.ImproveIncomes[j];
				//if this was the highest improve income, search for another
				if (player.Incomes[j] && (stat.ImproveIncomes[j] + um->Modifier.ImproveIncomes[j]) == player.Incomes[j]) {
					int m = DefaultIncomes[j];

					for (int k = 0; k < player.GetUnitCount(); ++k) {
						m = std::max(m, player.GetUnit(k).Type->Stats[player.Index].ImproveIncomes[j]);
					}
					player.Incomes[j] = m;
				}
				//Wyrmgus end
			}

			int varModified = 0;
			for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
				varModified |= um->Modifier.Variables[j].Value
							   | um->Modifier.Variables[j].Max
							   | um->Modifier.Variables[j].Increase
							   | um->Modifier.Variables[j].Enable
							   | um->ModifyPercent[j];
				stat.Variables[j].Enable |= um->Modifier.Variables[j].Enable;
				if (um->ModifyPercent[j]) {
					stat.Variables[j].Value = stat.Variables[j].Value * 100 / (100 + um->ModifyPercent[j]);
					stat.Variables[j].Max = stat.Variables[j].Max * 100 / (100 + um->ModifyPercent[j]);
				} else {
					stat.Variables[j].Value -= um->Modifier.Variables[j].Value;
					stat.Variables[j].Max -= um->Modifier.Variables[j].Max;
					stat.Variables[j].Increase -= um->Modifier.Variables[j].Increase;
				}

				stat.Variables[j].Max = std::max(stat.Variables[j].Max, 0);
				clamp(&stat.Variables[j].Value, 0, stat.Variables[j].Max);
			}

			// And now modify ingame units
			if (varModified) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*UnitTypes[z], unitupgrade, true);
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];

					if (unit.Player->Index != player.Index) {
						continue;
					}
					for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
						unit.Variable[j].Enable |= um->Modifier.Variables[j].Enable;
						if (um->ModifyPercent[j]) {
							unit.Variable[j].Value = unit.Variable[j].Value * 100 / (100 + um->ModifyPercent[j]);
							unit.Variable[j].Max = unit.Variable[j].Max * 100 / (100 + um->ModifyPercent[j]);
						} else {
							unit.Variable[j].Value -= um->Modifier.Variables[j].Value;
							unit.Variable[j].Increase -= um->Modifier.Variables[j].Increase;
						}

						unit.Variable[j].Max -= um->Modifier.Variables[j].Max;
						unit.Variable[j].Max = std::max(unit.Variable[j].Max, 0);

						clamp(&unit.Variable[j].Value, 0, unit.Variable[j].Max);
					}
					//Wyrmgus start
					//change variation if current one becomes forbidden
					VariationInfo *current_varinfo = UnitTypes[z]->VarInfo[unit.Variation];
					if (current_varinfo) {
						bool RequiredUpgrade = false;
						for (int u = 0; u < VariationMax; ++u) {
							if (!current_varinfo->UpgradesRequired[u].empty() && um->UpgradeId == CUpgrade::Get(current_varinfo->UpgradesRequired[u])->ID) {
								RequiredUpgrade = true;
								break;
							}
						}
						if (RequiredUpgrade == true) {
							int TypeVariationCount = 0;
							int LocalTypeVariations[VariationMax];
							for (int i = 0; i < VariationMax; ++i) {
								VariationInfo *varinfo = UnitTypes[z]->VarInfo[i];
								if (!varinfo) {
									continue;
								}
								if (!varinfo->Tileset.empty() && varinfo->Tileset != Map.Tileset->Name) {
									continue;
								}
								bool UpgradesCheck = true;
								for (int u = 0; u < VariationMax; ++u) {
									if (!varinfo->UpgradesRequired[u].empty() && UpgradeIdentAllowed(player, varinfo->UpgradesRequired[u].c_str()) != 'R' && unit.LearnedAbilities[CUpgrade::Get(varinfo->UpgradesRequired[u])->ID] == false) {
										UpgradesCheck = false;
										break;
									}
									if (!varinfo->UpgradesForbidden[u].empty() && (UpgradeIdentAllowed(player, varinfo->UpgradesForbidden[u].c_str()) == 'R' || unit.LearnedAbilities[CUpgrade::Get(varinfo->UpgradesForbidden[u])->ID] == true)) {
										UpgradesCheck = false;
										break;
									}
								}
								if (UpgradesCheck == false) {
									continue;
								}
								if (current_varinfo && varinfo->VariationId.find(current_varinfo->VariationId) != std::string::npos) { // if this variation includes the ident of the one incompatible with this upgrade, choose it automatically
									unit.Variation = i;
									TypeVariationCount = 0;
									break;
								}
								LocalTypeVariations[TypeVariationCount] = i;
								TypeVariationCount += 1;
							}
							if (TypeVariationCount > 0) {
								unit.Variation = LocalTypeVariations[SyncRand(TypeVariationCount)];
							}
						}
					}
					//Wyrmgus end
				}
			}
			if (um->ConvertTo) {
				ConvertUnitTypeTo(player, *um->ConvertTo, *UnitTypes[z]);
			}
		}
	}
}

//Wyrmgus start
/**
**  Apply the modifiers of an ability.
**
**  This function will mark upgrade done and do all required modifications
**  to unit types and will modify allow/forbid maps
**
**  @param player  Player that get all the upgrades.
**  @param um      Upgrade modifier that do the effects
*/
void ApplyIndividualUpgradeModifier(CUnit &unit, const CUpgradeModifier *um)
{
	Assert(um);

	if (um->Modifier.Variables[SIGHTRANGE_INDEX].Value) {
		if (!unit.Removed) {
			MapUnmarkUnitSight(unit);
			unit.CurrentSightRange = unit.Variable[SIGHTRANGE_INDEX].Value +
									 um->Modifier.Variables[SIGHTRANGE_INDEX].Value;
			//Wyrmgus start
			UpdateUnitSightRange(unit);
			//Wyrmgus end
			MapMarkUnitSight(unit);
		}
	}

	for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
		unit.Variable[j].Enable |= um->Modifier.Variables[j].Enable;
		if (um->ModifyPercent[j]) {
			unit.Variable[j].Value += unit.Variable[j].Value * um->ModifyPercent[j] / 100;
			unit.Variable[j].Max += unit.Variable[j].Max * um->ModifyPercent[j] / 100;
		} else {
			unit.Variable[j].Value += um->Modifier.Variables[j].Value;
			unit.Variable[j].Increase += um->Modifier.Variables[j].Increase;
		}
		unit.Variable[j].Max += um->Modifier.Variables[j].Max;
		unit.Variable[j].Max = std::max(unit.Variable[j].Max, 0);
		if (unit.Variable[j].Max > 0) {
			clamp(&unit.Variable[j].Value, 0, unit.Variable[j].Max);
		}

	}
	
	//change variation if current one becomes forbidden
	VariationInfo *current_varinfo = unit.Type->VarInfo[unit.Variation];
	if (current_varinfo) {
		bool ForbiddenUpgrade = false;
		for (int u = 0; u < VariationMax; ++u) {
			if (!current_varinfo->UpgradesForbidden[u].empty() && um->UpgradeId == CUpgrade::Get(current_varinfo->UpgradesForbidden[u])->ID) {
				ForbiddenUpgrade = true;
				break;
			}
		}
		if (ForbiddenUpgrade == true) {
			int TypeVariationCount = 0;
			int LocalTypeVariations[VariationMax];
			for (int i = 0; i < VariationMax; ++i) {
				VariationInfo *varinfo = unit.Type->VarInfo[i];
				if (!varinfo) {
					continue;
				}
				if (!varinfo->Tileset.empty() && varinfo->Tileset != Map.Tileset->Name) {
					continue;
				}
				bool UpgradesCheck = true;
				for (int u = 0; u < VariationMax; ++u) {
					if (!varinfo->UpgradesRequired[u].empty() && UpgradeIdentAllowed(*unit.Player, varinfo->UpgradesRequired[u].c_str()) != 'R' && unit.LearnedAbilities[CUpgrade::Get(varinfo->UpgradesRequired[u])->ID] == false) {
						UpgradesCheck = false;
						break;
					}
					if (!varinfo->UpgradesForbidden[u].empty() && (UpgradeIdentAllowed(*unit.Player, varinfo->UpgradesForbidden[u].c_str()) == 'R' || unit.LearnedAbilities[CUpgrade::Get(varinfo->UpgradesForbidden[u])->ID] == true)) {
						UpgradesCheck = false;
						break;
					}
				}
				if (UpgradesCheck == false) {
					continue;
				}
				if (current_varinfo && varinfo->VariationId.find(current_varinfo->VariationId) != std::string::npos) { // if this variation includes the ident of the one incompatible with this upgrade, choose it automatically
					unit.Variation = i;
					TypeVariationCount = 0;
					break;
				}
				LocalTypeVariations[TypeVariationCount] = i;
				TypeVariationCount += 1;
			}
			if (TypeVariationCount > 0) {
				unit.Variation = LocalTypeVariations[SyncRand(TypeVariationCount)];
			}
		}
	}
	
	if (um->ConvertTo) {
		CommandTransformIntoType(unit, *um->ConvertTo);
	}
}

static void RemoveIndividualUpgradeModifier(CUnit &unit, const CUpgradeModifier *um)
{
	Assert(um);

	if (um->Modifier.Variables[SIGHTRANGE_INDEX].Value) {
		if (!unit.Removed) {
			MapUnmarkUnitSight(unit);
			unit.CurrentSightRange = unit.Variable[SIGHTRANGE_INDEX].Value -
									 um->Modifier.Variables[SIGHTRANGE_INDEX].Value;
			//Wyrmgus start
			UpdateUnitSightRange(unit);
			//Wyrmgus end
			MapMarkUnitSight(unit);
		}
	}

	for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
		unit.Variable[j].Enable |= um->Modifier.Variables[j].Enable;
		if (um->ModifyPercent[j]) {
			unit.Variable[j].Value = unit.Variable[j].Value * 100 / (100 + um->ModifyPercent[j]);
			unit.Variable[j].Max = unit.Variable[j].Max * 100 / (100 + um->ModifyPercent[j]);
		} else {
			unit.Variable[j].Value -= um->Modifier.Variables[j].Value;
			unit.Variable[j].Increase -= um->Modifier.Variables[j].Increase;
		}
		unit.Variable[j].Max -= um->Modifier.Variables[j].Max;
		unit.Variable[j].Max = std::max(unit.Variable[j].Max, 0);
		if (unit.Variable[j].Max > 0) {
			clamp(&unit.Variable[j].Value, 0, unit.Variable[j].Max);
		}

	}
	
	//change variation if current one becomes forbidden
	VariationInfo *current_varinfo = unit.Type->VarInfo[unit.Variation];
	if (current_varinfo) {
		bool ForbiddenUpgrade = false;
		for (int u = 0; u < VariationMax; ++u) {
			if (!current_varinfo->UpgradesRequired[u].empty() && um->UpgradeId == CUpgrade::Get(current_varinfo->UpgradesRequired[u])->ID) {
				ForbiddenUpgrade = true;
				break;
			}
		}
		if (ForbiddenUpgrade == true) {
			int TypeVariationCount = 0;
			int LocalTypeVariations[VariationMax];
			for (int i = 0; i < VariationMax; ++i) {
				VariationInfo *varinfo = unit.Type->VarInfo[i];
				if (!varinfo) {
					continue;
				}
				if (!varinfo->Tileset.empty() && varinfo->Tileset != Map.Tileset->Name) {
					continue;
				}
				bool UpgradesCheck = true;
				for (int u = 0; u < VariationMax; ++u) {
					if (!varinfo->UpgradesRequired[u].empty() && UpgradeIdentAllowed(*unit.Player, varinfo->UpgradesRequired[u].c_str()) != 'R' && unit.LearnedAbilities[CUpgrade::Get(varinfo->UpgradesRequired[u])->ID] == false) {
						UpgradesCheck = false;
						break;
					}
					if (!varinfo->UpgradesForbidden[u].empty() && (UpgradeIdentAllowed(*unit.Player, varinfo->UpgradesForbidden[u].c_str()) == 'R' || unit.LearnedAbilities[CUpgrade::Get(varinfo->UpgradesForbidden[u])->ID] == true)) {
						UpgradesCheck = false;
						break;
					}
				}
				if (UpgradesCheck == false) {
					continue;
				}
				if (current_varinfo && varinfo->VariationId.find(current_varinfo->VariationId) != std::string::npos) { // if this variation includes the ident of the one incompatible with this upgrade, choose it automatically
					unit.Variation = i;
					TypeVariationCount = 0;
					break;
				}
				LocalTypeVariations[TypeVariationCount] = i;
				TypeVariationCount += 1;
			}
			if (TypeVariationCount > 0) {
				unit.Variation = LocalTypeVariations[SyncRand(TypeVariationCount)];
			}
		}
	}
}
//Wyrmgus end

/**
**  Handle that an upgrade was acquired.
**
**  @param player   Player researching the upgrade.
**  @param upgrade  Upgrade ready researched.
*/
void UpgradeAcquire(CPlayer &player, const CUpgrade *upgrade)
{
	int id = upgrade->ID;
	player.UpgradeTimers.Upgrades[id] = upgrade->Costs[TimeCost];
	AllowUpgradeId(player, id, 'R');  // research done

	for (int z = 0; z < NumUpgradeModifiers; ++z) {
		if (UpgradeModifiers[z]->UpgradeId == id) {
			ApplyUpgradeModifier(player, UpgradeModifiers[z]);
		}
	}

	//
	//  Upgrades could change the buttons displayed.
	//
	if (&player == ThisPlayer) {
		SelectedUnitChanged();
	}
}

/**
**  Upgrade will be lost
**
**  @param player   Player researching the upgrade.
**  @param id       Upgrade to be lost.
**  
*/
void UpgradeLost(CPlayer &player, int id)
{
	player.UpgradeTimers.Upgrades[id] = 0;
	//Wyrmgus start
	AllowUpgradeId(player, id, 'A'); // research is lost i.e. available
	//Wyrmgus end
	// FIXME: here we should reverse apply upgrade...
	
	for (int z = 0; z < NumUpgradeModifiers; ++z) {
		if (UpgradeModifiers[z]->UpgradeId == id) {
			RemoveUpgradeModifier(player, UpgradeModifiers[z]);
		}
	}

	//
	//  Upgrades could change the buttons displayed.
	//
	if (&player == ThisPlayer) {
		SelectedUnitChanged();
	}
}

/**
**  Apply researched upgrades when map is loading
**
**  @return:   void
*/
void ApplyUpgrades()
{
	for (std::vector<CUpgrade *>::size_type j = 0; j < AllUpgrades.size(); ++j) {
		CUpgrade *upgrade = AllUpgrades[j];
		if (upgrade) {
			for (int p = 0; p < PlayerMax; ++p) {
				if (Players[p].Allow.Upgrades[j] == 'R') {
					int id = upgrade->ID;
					Players[p].UpgradeTimers.Upgrades[id] = upgrade->Costs[TimeCost];
					AllowUpgradeId(Players[p], id, 'R');  // research done

					for (int z = 0; z < NumUpgradeModifiers; ++z) {
						if (UpgradeModifiers[z]->UpgradeId == id) {
							ApplyUpgradeModifier(Players[p], UpgradeModifiers[z]);
						}
					}
				}
			}
		}
	}
}

//Wyrmgus start
/**
**  Handle that an ability was acquired.
**
**  @param unit     Unit learning the upgrade.
**  @param upgrade  Upgrade ready researched.
*/
void AbilityAcquire(CUnit &unit, const CUpgrade *upgrade)
{
	int id = upgrade->ID;
	unit.Player->UpgradeTimers.Upgrades[id] = upgrade->Costs[TimeCost];
	unit.Variable[LEVELUP_INDEX].Value -= 1;
	unit.LearnedAbilities[id] = true;	// learning done

	for (int z = 0; z < NumUpgradeModifiers; ++z) {
		if (UpgradeModifiers[z]->UpgradeId == id) {
			ApplyIndividualUpgradeModifier(unit, UpgradeModifiers[z]);
		}
	}

	//
	//  Upgrades could change the buttons displayed.
	//
	if (unit.Player == ThisPlayer) {
		SelectedUnitChanged();
	}
}

void TraitAcquire(CUnit &unit, const CUpgrade *upgrade)
{
	if (!unit.Trait.empty()) { //remove previous trait, if any
		int old_id = CUpgrade::Get(unit.Trait)->ID;
		unit.LearnedAbilities[old_id] = false;
		if (!GameSettings.NoRandomness || unit.Type->BoolFlag[HERO_INDEX].value) { // if in no randomness setting, only change trait modifiers if the unit is a hero
			for (int z = 0; z < NumUpgradeModifiers; ++z) {
				if (UpgradeModifiers[z]->UpgradeId == old_id) {
					RemoveIndividualUpgradeModifier(unit, UpgradeModifiers[z]);
				}
			}
		}
	}

	int id = upgrade->ID;
	unit.Player->UpgradeTimers.Upgrades[id] = upgrade->Costs[TimeCost];
	unit.LearnedAbilities[id] = true;	// learning done

	if (!GameSettings.NoRandomness || unit.Type->BoolFlag[HERO_INDEX].value) { // if in no randomness setting, only apply trait modifiers if the unit is a hero
		for (int z = 0; z < NumUpgradeModifiers; ++z) {
			if (UpgradeModifiers[z]->UpgradeId == id) {
				ApplyIndividualUpgradeModifier(unit, UpgradeModifiers[z]);
			}
		}
	}

	//
	//  Upgrades could change the buttons displayed.
	//
	if (unit.Player == ThisPlayer) {
		SelectedUnitChanged();
	}
}

void IndividualUpgradeAcquire(CUnit &unit, const CUpgrade *upgrade)
{
	int id = upgrade->ID;
	unit.Player->UpgradeTimers.Upgrades[id] = upgrade->Costs[TimeCost];
	unit.LearnedAbilities[id] = true;	// learning done

	for (int z = 0; z < NumUpgradeModifiers; ++z) {
		if (UpgradeModifiers[z]->UpgradeId == id) {
			ApplyIndividualUpgradeModifier(unit, UpgradeModifiers[z]);
		}
	}

	//
	//  Upgrades could change the buttons displayed.
	//
	if (unit.Player == ThisPlayer) {
		SelectedUnitChanged();
	}
}

void IndividualUpgradeLost(CUnit &unit, const CUpgrade *upgrade)
{
	int id = upgrade->ID;
	unit.Player->UpgradeTimers.Upgrades[id] = 0;
	unit.LearnedAbilities[id] = false;	// learning undone

	for (int z = 0; z < NumUpgradeModifiers; ++z) {
		if (UpgradeModifiers[z]->UpgradeId == id) {
			RemoveIndividualUpgradeModifier(unit, UpgradeModifiers[z]);
		}
	}

	//
	//  Upgrades could change the buttons displayed.
	//
	if (unit.Player == ThisPlayer) {
		SelectedUnitChanged();
	}
}
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Allow(s)
----------------------------------------------------------------------------*/

// all the following functions are just map handlers, no specific notes

/**
**  Change allow for an unit-type.
**
**  @param player  Player to change
**  @param id      unit type id
**  @param units   maximum amount of units allowed
*/
//Wyrmgus start
//static void AllowUnitId(CPlayer &player, int id, int units)
void AllowUnitId(CPlayer &player, int id, int units)
//Wyrmgus end
{
	player.Allow.Units[id] = units;
}

/**
**  Change allow for an upgrade.
**
**  @param player  Player to change
**  @param id      upgrade id
**  @param af      'A'llow/'F'orbid/'R'eseached
*/
void AllowUpgradeId(CPlayer &player, int id, char af)
{
	Assert(af == 'A' || af == 'F' || af == 'R');
	player.Allow.Upgrades[id] = af;
	
	//Wyrmgus start
	//if the upgrade is a writing upgrade, and has been set to researched, set a new random faction for the player, if the current faction is a tribe (this happens only outside grand strategy mode)
	if (!GrandStrategy && af == 'R' && AllUpgrades[id]->Class == "writing" && (player.Faction == -1 || PlayerRaces.Factions[player.Race][player.Faction]->Type == "tribe")) {
		player.SetRandomFaction();
	}
	//Wyrmgus end
}

/**
**  Return the allow state of the unit.
**
**  @param player   Check state of this player.
**  @param id       Unit identifier.
**
**  @return the allow state of the unit.
*/
int UnitIdAllowed(const CPlayer &player, int id)
{
	Assert(id >= 0 && id < UnitTypeMax);
	return player.Allow.Units[id];
}

/**
**  Return the allow state of an upgrade.
**
**  @param player  Check state for this player.
**  @param id      Upgrade identifier.
**
**  @return the allow state of the upgrade.
*/
char UpgradeIdAllowed(const CPlayer &player, int id)
{
	Assert(id >= 0 && id < UpgradeMax);
	return player.Allow.Upgrades[id];
}

// ***************by string identifiers's

/**
**  Return the allow state of an upgrade.
**
**  @param player  Check state for this player.
**  @param ident   Upgrade identifier.
**
**  @note This function shouldn't be used during runtime, it is only for setup.
*/
char UpgradeIdentAllowed(const CPlayer &player, const std::string &ident)
{
	int id = UpgradeIdByIdent(ident);

	if (id != -1) {
		return UpgradeIdAllowed(player, id);
	}
	DebugPrint("Fix your code, wrong identifier `%s'\n" _C_ ident.c_str());
	return '-';
}

//@}
