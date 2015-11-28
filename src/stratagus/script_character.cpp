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
/**@name script_character.cpp - The character ccl functions. */
//
//      (c) Copyright 2015 by Andrettin
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

#include "stratagus.h"

#include "character.h"

#include "grand_strategy.h"
#include "player.h"
#include "script.h"
#include "unittype.h"
#include "upgrade.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Define a character.
**
**  @param l  Lua state.
*/
static int CclDefineCharacter(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string character_full_name = TransliterateText(LuaToString(l, 1));
	CCharacter *character = GetCharacter(character_full_name);
	if (!character) {
		character = new CCharacter;
		Characters.push_back(character);
	} else {
		LuaError(l, "Character \"%s\" is being redefined." _C_ character_full_name.c_str());
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			character->Name = TransliterateText(LuaToString(l, -1));
		} else if (!strcmp(value, "ExtraName")) {
			character->ExtraName = TransliterateText(LuaToString(l, -1));
		} else if (!strcmp(value, "Dynasty")) {
			character->Dynasty = TransliterateText(LuaToString(l, -1));
		} else if (!strcmp(value, "Type")) {
			std::string unit_type_ident = LuaToString(l, -1);
			int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
			if (unit_type_id != -1) {
				character->Type = const_cast<CUnitType *>(&(*UnitTypes[unit_type_id]));
				if (character->Level < character->Type->DefaultStat.Variables[LEVEL_INDEX].Value) {
					character->Level = character->Type->DefaultStat.Variables[LEVEL_INDEX].Value;
				}
			} else {
				LuaError(l, "Unit type \"%s\" doesn't exist." _C_ unit_type_ident.c_str());
			}
		} else if (!strcmp(value, "Trait")) {
			std::string trait_ident = LuaToString(l, -1);
			int upgrade_id = UpgradeIdByIdent(trait_ident);
			if (upgrade_id != -1) {
				character->Trait = const_cast<CUpgrade *>(&(*AllUpgrades[upgrade_id]));
			} else {
				LuaError(l, "Trait upgrade \"%s\" doesn't exist." _C_ trait_ident.c_str());
			}
		} else if (!strcmp(value, "Year")) {
			character->Year = LuaToNumber(l, -1);
		} else if (!strcmp(value, "DeathYear")) {
			character->DeathYear = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Civilization")) {
			character->Civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1));
		} else if (!strcmp(value, "ProvinceOfOrigin")) {
			character->ProvinceOfOriginName = LuaToString(l, -1);
		} else if (!strcmp(value, "Father")) {
			std::string father_name = TransliterateText(LuaToString(l, -1));
			CCharacter *father = GetCharacter(father_name);
			if (father) {
				if (father->Gender == MaleGender) {
					character->Father = const_cast<CCharacter *>(&(*father));
					if (!father->IsParentOf(character_full_name)) { //check whether the character has already been set as a child of the father
						father->Children.push_back(character);
					}
					// see if the father's other children aren't already included in the character's siblings, and if they aren't, add them (and add the character to the siblings' sibling list, of course)
					for (size_t i = 0; i < father->Children.size(); ++i) {
						if (father->Children[i]->GetFullName() != character_full_name) {
							if (!character->IsSiblingOf(father->Children[i]->GetFullName())) {
								character->Siblings.push_back(father->Children[i]);
							}
							if (!father->Children[i]->IsSiblingOf(character_full_name)) {
								father->Children[i]->Siblings.push_back(character);
							}
						}
					}
				} else {
					LuaError(l, "Character \"%s\" set to be the biological father of \"%s\", but isn't male." _C_ father_name.c_str() _C_ character_full_name.c_str());
				}
			} else {
				LuaError(l, "Character \"%s\" doesn't exist." _C_ father_name.c_str());
			}
		} else if (!strcmp(value, "Mother")) {
			std::string mother_name = TransliterateText(LuaToString(l, -1));
			CCharacter *mother = GetCharacter(mother_name);
			if (mother) {
				if (mother->Gender == FemaleGender) {
					character->Mother = const_cast<CCharacter *>(&(*mother));
					if (!mother->IsParentOf(character_full_name)) { //check whether the character has already been set as a child of the mother
						mother->Children.push_back(character);
					}
					// see if the mother's other children aren't already included in the character's siblings, and if they aren't, add them (and add the character to the siblings' sibling list, of course)
					for (size_t i = 0; i < mother->Children.size(); ++i) {
						if (mother->Children[i]->GetFullName() != character_full_name) {
							if (!character->IsSiblingOf(mother->Children[i]->GetFullName())) {
								character->Siblings.push_back(mother->Children[i]);
							}
							if (!mother->Children[i]->IsSiblingOf(character_full_name)) {
								mother->Children[i]->Siblings.push_back(character);
							}
						}
					}
				} else {
					LuaError(l, "Character \"%s\" set to be the biological mother of \"%s\", but isn't female." _C_ mother_name.c_str() _C_ character_full_name.c_str());
				}
			} else {
				LuaError(l, "Character \"%s\" doesn't exist." _C_ mother_name.c_str());
			}
		} else if (!strcmp(value, "Gender")) {
			character->Gender = GetGenderIdByName(LuaToString(l, -1));
		} else if (!strcmp(value, "Icon")) {
			character->Icon.Name = LuaToString(l, -1);
			character->Icon.Icon = NULL;
			character->Icon.Load();
		} else if (!strcmp(value, "Persistent")) {
			character->Persistent = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Level")) {
			character->Level = LuaToNumber(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (character->Trait == NULL) { //if no trait was set, have the character be the same trait as the unit type (if the unit type has a single one predefined)
		if (character->Type != NULL && character->Type->Traits.size() == 1) {
			character->Trait = const_cast<CUpgrade *>(&(*character->Type->Traits[0]));
		}
	}
	
	if (character->Gender == NoGender) { //if no gender was set, have the character be the same gender as the unit type (if the unit type has it predefined)
		if (character->Type != NULL && character->Type->DefaultStat.Variables[GENDER_INDEX].Value != 0) {
			character->Gender = character->Type->DefaultStat.Variables[GENDER_INDEX].Value;
		}
	}
	
	if (character->GetFullName() != character_full_name) { // if the character's full name (built from its defined elements) is different from the name used to initialize the character, something went wrong
		LuaError(l, "Character name \"%s\" doesn't match the defined name \"%s\"." _C_ character->GetFullName().c_str() _C_ character_full_name.c_str());
	}
	
	return 0;
}

/**
**  Define a custom hero.
**
**  @param l  Lua state.
*/
static int CclDefineCustomHero(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string hero_full_name = LuaToString(l, 1);
	CCharacter *hero = GetCustomHero(hero_full_name);
	if (!hero) {
		hero = new CCharacter;
		CustomHeroes.push_back(hero);
	} else {
		LuaError(l, "Custom hero \"%s\" is being redefined." _C_ hero_full_name.c_str());
	}
	hero->Custom = true;
	hero->Persistent = true;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			hero->Name = TransliterateText(LuaToString(l, -1));
		} else if (!strcmp(value, "ExtraName")) {
			hero->ExtraName = TransliterateText(LuaToString(l, -1));
		} else if (!strcmp(value, "Dynasty")) {
			hero->Dynasty = TransliterateText(LuaToString(l, -1));
		} else if (!strcmp(value, "Type")) {
			std::string unit_type_ident = LuaToString(l, -1);
			int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
			if (unit_type_id != -1) {
				hero->Type = const_cast<CUnitType *>(&(*UnitTypes[unit_type_id]));
				if (hero->Level < hero->Type->DefaultStat.Variables[LEVEL_INDEX].Value) {
					hero->Level = hero->Type->DefaultStat.Variables[LEVEL_INDEX].Value;
				}
			} else {
				LuaError(l, "Unit type \"%s\" doesn't exist." _C_ unit_type_ident.c_str());
			}
		} else if (!strcmp(value, "Trait")) {
			std::string trait_ident = LuaToString(l, -1);
			int upgrade_id = UpgradeIdByIdent(trait_ident);
			if (upgrade_id != -1) {
				hero->Trait = const_cast<CUpgrade *>(&(*AllUpgrades[upgrade_id]));
			} else {
				LuaError(l, "Trait upgrade \"%s\" doesn't exist." _C_ trait_ident.c_str());
			}
		} else if (!strcmp(value, "Civilization")) {
			hero->Civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1));
		} else if (!strcmp(value, "Gender")) {
			hero->Gender = GetGenderIdByName(LuaToString(l, -1));
		} else if (!strcmp(value, "Level")) {
			hero->Level = LuaToNumber(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (hero->Gender == NoGender) { //if no gender was set, have the hero be the same gender as the unit type (if the unit type has it predefined)
		if (hero->Type != NULL && hero->Type->DefaultStat.Variables[GENDER_INDEX].Value != 0) {
			hero->Gender = hero->Type->DefaultStat.Variables[GENDER_INDEX].Value;
		}
	}
	
	if (hero->GetFullName() != hero_full_name) { // if the hero's full name (built from its defined elements) is different from the name used to initialize the hero, something went wrong
		LuaError(l, "Custom hero name \"%s\" doesn't match the defined name \"%s\"." _C_ hero->GetFullName().c_str() _C_ hero_full_name.c_str());
	}
	
	return 0;
}

/**
**  Define a grand strategy hero.
**
**  @param l  Lua state.
*/
static int CclDefineGrandStrategyHero(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string hero_full_name = TransliterateText(LuaToString(l, 1));
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (!hero) {
		hero = new CGrandStrategyHero;
		GrandStrategyGame.Heroes.push_back(hero);
		GrandStrategyHeroStringToIndex[hero_full_name] = GrandStrategyGame.Heroes.size() - 1;
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			hero->Name = TransliterateText(LuaToString(l, -1));
		} else if (!strcmp(value, "ExtraName")) {
			hero->ExtraName = TransliterateText(LuaToString(l, -1));
		} else if (!strcmp(value, "Dynasty")) {
			hero->Dynasty = TransliterateText(LuaToString(l, -1));
		} else if (!strcmp(value, "Type")) {
			std::string unit_type_ident = LuaToString(l, -1);
			int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
			if (unit_type_id != -1) {
				hero->Type = const_cast<CUnitType *>(&(*UnitTypes[unit_type_id]));
			} else {
				LuaError(l, "Unit type \"%s\" doesn't exist." _C_ unit_type_ident.c_str());
			}
		} else if (!strcmp(value, "Trait")) {
			std::string trait_ident = LuaToString(l, -1);
			int upgrade_id = UpgradeIdByIdent(trait_ident);
			if (upgrade_id != -1) {
				hero->Trait = const_cast<CUpgrade *>(&(*AllUpgrades[upgrade_id]));
			} else {
				LuaError(l, "Trait upgrade \"%s\" doesn't exist." _C_ trait_ident.c_str());
			}
		} else if (!strcmp(value, "Year")) {
			hero->Year = LuaToNumber(l, -1);
		} else if (!strcmp(value, "DeathYear")) {
			hero->DeathYear = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Civilization")) {
			hero->Civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1));
		} else if (!strcmp(value, "ProvinceOfOrigin")) {
			hero->ProvinceOfOriginName = LuaToString(l, -1);
		} else if (!strcmp(value, "Father")) {
			std::string father_name = TransliterateText(LuaToString(l, -1));
			CGrandStrategyHero *father = GrandStrategyGame.GetHero(father_name);
			if (father) {
				if (father->Gender == MaleGender) {
					hero->Father = const_cast<CGrandStrategyHero *>(&(*father));
					if (!father->IsParentOf(hero_full_name)) { //check whether the hero has already been set as a child of the father
						father->Children.push_back(hero);
					}
					// see if the father's other children aren't already included in the hero's siblings, and if they aren't, add them (and add the hero to the siblings' sibling list, of course)
					for (size_t i = 0; i < father->Children.size(); ++i) {
						if (father->Children[i]->GetFullName() != hero_full_name) {
							if (!hero->IsSiblingOf(father->Children[i]->GetFullName())) {
								hero->Siblings.push_back(father->Children[i]);
							}
							if (!father->Children[i]->IsSiblingOf(hero_full_name)) {
								father->Children[i]->Siblings.push_back(hero);
							}
						}
					}
				} else {
					LuaError(l, "Hero \"%s\" set to be the biological father of \"%s\", but isn't male." _C_ father_name.c_str() _C_ hero_full_name.c_str());
				}
			} else {
				LuaError(l, "Hero \"%s\" doesn't exist." _C_ father_name.c_str());
			}
		} else if (!strcmp(value, "Mother")) {
			std::string mother_name = TransliterateText(LuaToString(l, -1));
			CGrandStrategyHero *mother = GrandStrategyGame.GetHero(mother_name);
			if (mother) {
				if (mother->Gender == FemaleGender) {
					hero->Mother = const_cast<CGrandStrategyHero *>(&(*mother));
					if (!mother->IsParentOf(hero_full_name)) { //check whether the hero has already been set as a child of the mother
						mother->Children.push_back(hero);
					}
					// see if the mother's other children aren't already included in the hero's siblings, and if they aren't, add them (and add the hero to the siblings' sibling list, of course)
					for (size_t i = 0; i < mother->Children.size(); ++i) {
						if (mother->Children[i]->GetFullName() != hero_full_name) {
							if (!hero->IsSiblingOf(mother->Children[i]->GetFullName())) {
								hero->Siblings.push_back(mother->Children[i]);
							}
							if (!mother->Children[i]->IsSiblingOf(hero_full_name)) {
								mother->Children[i]->Siblings.push_back(hero);
							}
						}
					}
				} else {
					LuaError(l, "Hero \"%s\" set to be the biological mother of \"%s\", but isn't female." _C_ mother_name.c_str() _C_ hero_full_name.c_str());
				}
			} else {
				LuaError(l, "Hero \"%s\" doesn't exist." _C_ mother_name.c_str());
			}
		} else if (!strcmp(value, "Gender")) {
			hero->Gender = GetGenderIdByName(LuaToString(l, -1));
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	hero->Initialize();
	
	return 0;
}

// ----------------------------------------------------------------------------

/**
**  Register CCL features for quests.
*/
void CharacterCclRegister()
{
	lua_register(Lua, "DefineCharacter", CclDefineCharacter);
	lua_register(Lua, "DefineCustomHero", CclDefineCustomHero);
	lua_register(Lua, "DefineGrandStrategyHero", CclDefineGrandStrategyHero);
}

//@}
