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
//      (c) Copyright 1998-2020 by Lutz Sammer, Russell Smith, Jimmy Salmon
//		and Andrettin
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

#include "stratagus.h"

#include "action/action_spellcast.h"

#include "animation.h"

#include "animation/animation_attack.h"
#include "animation/animation_die.h"
#include "animation/animation_exactframe.h"
#include "animation/animation_frame.h"
#include "animation/animation_goto.h"
#include "animation/animation_ifvar.h"
#include "animation/animation_label.h"
#include "animation/animation_move.h"
#include "animation/animation_randomgoto.h"
#include "animation/animation_randomrotate.h"
#include "animation/animation_randomsound.h"
#include "animation/animation_randomwait.h"
#include "animation/animation_rotate.h"
#include "animation/animation_setvar.h"
#include "animation/animation_sound.h"
#include "animation/animation_spawnmissile.h"
#include "animation/animation_spawnunit.h"
#include "animation/animation_unbreakable.h"
#include "animation/animation_wait.h"

#include "actions.h"
#include "config.h"
#include "iolib.h"
#include "player.h"
#include "script.h"
#include "spell/spell.h"
#include "unit/unit.h"
#include "unit/unit_type.h"

struct LabelsStruct {
	CAnimation *Anim;
	std::string Name;
};
static std::vector<LabelsStruct> Labels;

struct LabelsLaterStruct {
	CAnimation **Anim;
	std::string Name;
};
static std::vector<LabelsLaterStruct> LabelsLater;

/*----------------------------------------------------------------------------
--  Animation
----------------------------------------------------------------------------*/

/**
**  Show unit animation.
**
**  @param unit  Unit of the animation.
**  @param anim  Animation script to handle.
**
**  @return      The flags of the current script step.
*/
int UnitShowAnimation(CUnit &unit, const CAnimation *anim)
{
	return UnitShowAnimationScaled(unit, anim, 8);
}

/**
**  Parse player number in animation frame
**
**  @param unit      Unit of the animation.
**  @param parseint  Integer to parse.
**
**  @return  The parsed value.
*/

static int ParseAnimPlayer(const CUnit &unit, char *parseint)
{
	if (!strcmp(parseint, "this")) {
		return unit.Player->Index;
	}
	return ParseAnimInt(unit, parseint);
}


/**
**  Parse integer in animation frame.
**
**  @param unit      Unit of the animation.
**  @param parseint  Integer to parse.
**
**  @return  The parsed value.
*/

int ParseAnimInt(const CUnit &unit, const char *parseint)
{
	char s[100];
	const CUnit *goal = &unit;

	if (!strlen(parseint)) {
		return 0;
	}

	strcpy(s, parseint);
	char *cur = &s[2];
	if (s[0] == 'v' || s[0] == 't') { //unit variable detected
		if (s[0] == 't') {
			if (unit.CurrentOrder()->has_goal()) {
				goal = unit.CurrentOrder()->get_goal();
			} else {
				return 0;
			}
		}
		char *next = strchr(cur, '.');
		if (next == nullptr) {
			throw std::runtime_error("Need also specify the variable \"" + std::string(cur) + "\" tag.");
		} else {
			*next = '\0';
		}
		const int index = UnitTypeVar.VariableNameLookup[cur];// User variables
		if (index == -1) {
			if (!strcmp(cur, "ResourcesHeld")) {
				return goal->ResourcesHeld;
			} else if (!strcmp(cur, "ResourceActive")) {
				return goal->Resource.Active;
			} else if (!strcmp(cur, "InsideCount")) {
				return goal->InsideCount;
			} else if (!strcmp(cur, "_Distance")) {
				return unit.MapDistanceTo(*goal);
			}
			throw std::runtime_error("Bad variable name \"" + std::string(cur) + "\".");
		}
		if (!strcmp(next + 1, "Value")) {
			//Wyrmgus start
//			return goal->Variable[index].Value;
			return goal->GetModifiedVariable(index, VariableAttribute::Value);
			//Wyrmgus end
		} else if (!strcmp(next + 1, "Max")) {
			//Wyrmgus start
//			return goal->Variable[index].Max;
			return goal->GetModifiedVariable(index, VariableAttribute::Max);
			//Wyrmgus end
		} else if (!strcmp(next + 1, "Increase")) {
			//Wyrmgus start
//			return goal->Variable[index].Increase;
			return goal->GetModifiedVariable(index, VariableAttribute::Increase);
			//Wyrmgus end
		} else if (!strcmp(next + 1, "Enable")) {
			return goal->Variable[index].Enable;
		} else if (!strcmp(next + 1, "Percent")) {
			//Wyrmgus start
//			return goal->Variable[index].Value * 100 / goal->Variable[index].Max;
			return goal->GetModifiedVariable(index, VariableAttribute::Value) * 100 / goal->GetModifiedVariable(index, VariableAttribute::Max);
			//Wyrmgus end
		}
		return 0;
	} else if (s[0] == 'b' || s[0] == 'g') { //unit bool flag detected
		if (s[0] == 'g') {
			if (unit.CurrentOrder()->has_goal()) {
				goal = unit.CurrentOrder()->get_goal();
			} else {
				return 0;
			}
		}
		const int index = UnitTypeVar.BoolFlagNameLookup[cur];// User bool flags
		if (index == -1) {
			throw std::runtime_error("Bad bool-flag name \"" + std::string(cur) + "\".");
		}
		return goal->Type->BoolFlag[index].value;
	} else if (s[0] == 's') { //spell type detected
		Assert(goal->CurrentAction() == UnitAction::SpellCast);
		const COrder_SpellCast &order = *static_cast<COrder_SpellCast *>(goal->CurrentOrder());
		const wyrmgus::spell &spell = order.GetSpell();
		if (!strcmp(spell.get_identifier().c_str(), cur)) {
			return 1;
		}
		return 0;
	} else if (s[0] == 'S') { // check if autocast for this spell available
		const wyrmgus::spell *spell = wyrmgus::spell::get(cur);
		if (unit.is_autocast_spell(spell)) {
			return 1;
		}
		return 0;
	} else if (s[0] == 'r') { //random value
		char *next = strchr(cur, '.');
		if (next == nullptr) {
			return SyncRand(atoi(cur) + 1);
		} else {
			*next = '\0';
			const int min = atoi(cur);
			return min + SyncRand(atoi(next + 1) - min + 1);
		}
	} else if (s[0] == 'l') { //player number
		return ParseAnimPlayer(unit, cur);

	}
	// Check if we trying to parse a number
	Assert(isdigit(s[0]) || s[0] == '-');
	return atoi(parseint);
}

/**
**  Parse flags list in animation frame.
**
**  @param unit       Unit of the animation.
**  @param parseflag  Flag list to parse.
**
**  @return The parsed value.
*/
int ParseAnimFlags(const CUnit &unit, const char *parseflag)
{
	char s[100];
	int flags = 0;

	strcpy(s, parseflag);
	char *cur = s;
	char *next = s;
	while (next && *next) {
		next = strchr(cur, '.');
		if (next) {
			*next = '\0';
			++next;
		}
		if (unit.Anim.Anim->Type == AnimationSpawnMissile) {
			if (!strcmp(cur, "none")) {
				flags = SM_None;
				return flags;
			} else if (!strcmp(cur, "damage")) {
				flags |= SM_Damage;
			} else if (!strcmp(cur, "totarget")) {
				flags |= SM_ToTarget;
			} else if (!strcmp(cur, "pixel")) {
				flags |= SM_Pixel;
			} else if (!strcmp(cur, "reltarget")) {
				flags |= SM_RelTarget;
			} else if (!strcmp(cur, "ranged")) {
				flags |= SM_Ranged;
			}  else if (!strcmp(cur, "setdirection")) {
				flags |= SM_SetDirection;
			} else {
				throw std::runtime_error("Unknown animation flag: \"" + std::string(cur) + "\".");
			}
		} else if (unit.Anim.Anim->Type == AnimationSpawnUnit) {
			if (!strcmp(cur, "none")) {
				flags = SU_None;
				return flags;
			} else if (!strcmp(cur, "summoned")) {
				flags |= SU_Summoned;
			} else if (!strcmp(cur, "jointoai")) {
				flags |= SU_JoinToAIForce;
			} else {
				throw std::runtime_error("Unknown animation flag: \"" + std::string(cur) + "\".");
			}
		}
		cur = next;
	}
	return flags;
}


/**
**  Show unit animation.
**
**  @param unit   Unit of the animation.
**  @param anim   Animation script to handle.
**  @param scale  Scaling factor of the wait times in animation (8 means no scaling).
**
**  @return       The flags of the current script step.
*/
int UnitShowAnimationScaled(CUnit &unit, const CAnimation *anim, int scale)
{
	// Changing animations
	if (anim && unit.Anim.CurrAnim != anim) {
		// Assert fails when transforming unit (upgrade-to).
		Assert(!unit.Anim.Unbreakable || unit.Waiting);
		unit.Anim.Anim = unit.Anim.CurrAnim = anim;
		unit.Anim.Wait = 0;
	}

	// Currently waiting
	if (unit.Anim.Wait) {
		--unit.Anim.Wait;
		if (!unit.Anim.Wait) {
			// Advance to next frame
			unit.Anim.Anim = unit.Anim.Anim->get_next();
		}
		return 0;
	}
	int move = 0;
	while (!unit.Anim.Wait) {
		unit.Anim.Anim->Action(unit, move, scale);
		if (!unit.Anim.Wait) {
			// Advance to next frame
			unit.Anim.Anim = unit.Anim.Anim->get_next();
		}
	}

	--unit.Anim.Wait;
	if (!unit.Anim.Wait) {
		// Advance to next frame
		unit.Anim.Anim = unit.Anim.Anim->get_next();
	}
	return move;
}

static int GetAdvanceIndex(const CAnimation *base, const CAnimation *anim)
{
	if (base == anim) {
		return 0;
	}
	int i = 1;
	for (const CAnimation *it = base->get_next(); it != base; it = it->get_next()) {
		if (it == anim) {
			return i;
		}
		++i;
	}
	return -1;
}

namespace wyrmgus {

void animation_set::SaveUnitAnim(CFile &file, const CUnit &unit)
{
	file.printf("\"anim-data\", {");
	file.printf("\"anim-wait\", %d,", unit.Anim.Wait);
	for (size_t i = 0; i < CAnimation::animation_list.size(); ++i) {
		if (CAnimation::animation_list[i] == unit.Anim.CurrAnim) {
			file.printf("\"curr-anim\", %d,", i);
			file.printf("\"anim\", %d,", GetAdvanceIndex(unit.Anim.CurrAnim, unit.Anim.Anim));
			break;
		}
	}
	if (unit.Anim.Unbreakable) {
		file.printf(" \"unbreakable\",");
	}
	file.printf("}, ");
	// Wait backup info
	file.printf("\"wait-anim-data\", {");
	file.printf("\"anim-wait\", %d,", unit.WaitBackup.Wait);
	for (size_t i = 0; i < CAnimation::animation_list.size(); ++i) {
		if (CAnimation::animation_list[i] == unit.WaitBackup.CurrAnim) {
			file.printf("\"curr-anim\", %d,", i);
			file.printf("\"anim\", %d,", GetAdvanceIndex(unit.WaitBackup.CurrAnim, unit.WaitBackup.Anim));
			break;
		}
	}
	if (unit.WaitBackup.Unbreakable) {
		file.printf(" \"unbreakable\",");
	}
	file.printf("}");
}

}

static const CAnimation *Advance(const CAnimation *anim, int n)
{
	for (int i = 0; i < n; ++i) {
		anim = anim->get_next();
	}
	return anim;
}

namespace wyrmgus {

void animation_set::LoadUnitAnim(lua_State *l, CUnit &unit, int luaIndex)
{
	if (!lua_istable(l, luaIndex)) {
		LuaError(l, "incorrect argument");
	}
	const int nargs = lua_rawlen(l, luaIndex);

	for (int j = 0; j != nargs; ++j) {
		const char *value = LuaToString(l, luaIndex, j + 1);
		++j;

		if (!strcmp(value, "anim-wait")) {
			unit.Anim.Wait = LuaToNumber(l, luaIndex, j + 1);
		} else if (!strcmp(value, "curr-anim")) {
			const int animIndex = LuaToNumber(l, luaIndex, j + 1);
			unit.Anim.CurrAnim = CAnimation::animation_list.at(animIndex);
		} else if (!strcmp(value, "anim")) {
			const int animIndex = LuaToNumber(l, luaIndex, j + 1);
			unit.Anim.Anim = Advance(unit.Anim.CurrAnim, animIndex);
		} else if (!strcmp(value, "unbreakable")) {
			unit.Anim.Unbreakable = 1;
			--j;
		} else {
			LuaError(l, "Unit anim-data: Unsupported tag: %s" _C_ value);
		}
	}
}

void animation_set::LoadWaitUnitAnim(lua_State *l, CUnit &unit, int luaIndex)
{
	if (!lua_istable(l, luaIndex)) {
		LuaError(l, "incorrect argument");
	}
	const int nargs = lua_rawlen(l, luaIndex);

	for (int j = 0; j != nargs; ++j) {
		const char *value = LuaToString(l, luaIndex, j + 1);
		++j;

		if (!strcmp(value, "anim-wait")) {
			unit.WaitBackup.Wait = LuaToNumber(l, luaIndex, j + 1);
		} else if (!strcmp(value, "curr-anim")) {
			const int animIndex = LuaToNumber(l, luaIndex, j + 1);
			unit.WaitBackup.CurrAnim = CAnimation::animation_list.at(animIndex);
		} else if (!strcmp(value, "anim")) {
			const int animIndex = LuaToNumber(l, luaIndex, j + 1);
			unit.WaitBackup.Anim = Advance(unit.WaitBackup.CurrAnim, animIndex);
		} else if (!strcmp(value, "unbreakable")) {
			unit.WaitBackup.Unbreakable = 1;
			--j;
		} else {
			LuaError(l, "Unit anim-data: Unsupported tag: %s" _C_ value);
		}
	}
}

void animation_set::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (
		tag == "start"
		|| tag == "still"
		|| tag == "death"
		|| tag == "attack"
		|| tag == "ranged_attack"
		|| tag == "spell_cast"
		|| tag == "move"
		|| tag == "repair"
		|| tag == "train"
		|| tag == "research"
		|| tag == "upgrade"
		|| tag == "build"
		|| tag == "harvest"
	) {
		const resource *resource = nullptr;
		std::string death_type;
		std::unique_ptr<CAnimation> first_anim;
		CAnimation *prev_anim = nullptr;

		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			std::unique_ptr<CAnimation> anim;

			if (tag == "death" && key == "death_type") {
				death_type = FindAndReplaceString(value, "_", "-");
			} else if (tag == "harvest" && key == "resource") {
				resource = resource::get(value);
			} else if (key == "frame") {
				anim = std::make_unique<CAnimation_Frame>();
			} else if (key == "exact-frame") {
				anim = std::make_unique<CAnimation_ExactFrame>();
			} else if (key == "wait") {
				anim = std::make_unique<CAnimation_Wait>();
			} else if (key == "random-wait") {
				anim = std::make_unique<CAnimation_RandomWait>();
			} else if (key == "sound") {
				anim = std::make_unique<CAnimation_Sound>();
			} else if (key == "random-sound") {
				anim = std::make_unique<CAnimation_RandomSound>();
			} else if (key == "attack") {
				anim = std::make_unique<CAnimation_Attack>();
			} else if (key == "spawn-missile") {
				anim = std::make_unique<CAnimation_SpawnMissile>();
			} else if (key == "spawn-unit") {
				anim = std::make_unique<CAnimation_SpawnUnit>();
			} else if (key == "if-var") {
				anim = std::make_unique<CAnimation_IfVar>();
			} else if (key == "set-var") {
				anim = std::make_unique<CAnimation_SetVar>();
			} else if (key == "die") {
				anim = std::make_unique<CAnimation_Die>();
			} else if (key == "rotate") {
				anim = std::make_unique<CAnimation_Rotate>();
			} else if (key == "random-rotate") {
				anim = std::make_unique<CAnimation_RandomRotate>();
			} else if (key == "move") {
				anim = std::make_unique<CAnimation_Move>();
			} else if (key == "unbreakable") {
				anim = std::make_unique<CAnimation_Unbreakable>();
			} else if (key == "goto") {
				anim = std::make_unique<CAnimation_Goto>();
			} else if (key == "random-goto") {
				anim = std::make_unique<CAnimation_RandomGoto>();
			} else {
				throw std::runtime_error("Invalid animation property: \"" + key + "\".");
			}

			if (anim) {
				anim->Init(value.c_str(), nullptr);

				CAnimation *temp_prev_anim = prev_anim;
				prev_anim = anim.get();

				if (!first_anim) {
					first_anim = std::move(anim);
				} else {
					temp_prev_anim->set_next(std::move(anim));
				}
			}
		});

		if (first_anim && prev_anim) {
			prev_anim->set_next(first_anim.get());
		}

		if (tag == "start") {
			this->Start = std::move(first_anim);
		} else if (tag == "still") {
			this->Still = std::move(first_anim);
		} else if (tag == "death") {
			if (!death_type.empty()) {
				const int death_index = ExtraDeathIndex(death_type.c_str());
				if (death_index == ANIMATIONS_DEATHTYPES) {
					this->Death[ANIMATIONS_DEATHTYPES] = std::move(first_anim);
				} else {
					this->Death[death_index] = std::move(first_anim);
				}
			} else {
				this->Death[ANIMATIONS_DEATHTYPES] = std::move(first_anim);
			}
		} else if (tag == "attack") {
			this->Attack = std::move(first_anim);
		} else if (tag == "ranged_attack") {
			this->RangedAttack = std::move(first_anim);
		} else if (tag == "spell_cast") {
			this->SpellCast = std::move(first_anim);
		} else if (tag == "move") {
			this->Move = std::move(first_anim);
		} else if (tag == "repair") {
			this->Repair = std::move(first_anim);
		} else if (tag == "train") {
			this->Train = std::move(first_anim);
		} else if (tag == "research") {
			this->Research = std::move(first_anim);
		} else if (tag == "upgrade") {
			this->Upgrade = std::move(first_anim);
		} else if (tag == "build") {
			this->Build = std::move(first_anim);
		} else if (tag == "harvest") {
			this->Harvest[resource->get_index()] = std::move(first_anim);
		}
	}
}

void animation_set::initialize()
{
	// Must add to array in a fixed order for save games
	animation_set::AddAnimationToArray(this->Start.get());
	animation_set::AddAnimationToArray(this->Still.get());
	for (int i = 0; i != ANIMATIONS_DEATHTYPES + 1; ++i) {
		animation_set::AddAnimationToArray(this->Death[i].get());
	}
	animation_set::AddAnimationToArray(this->Attack.get());
	animation_set::AddAnimationToArray(this->RangedAttack.get());
	animation_set::AddAnimationToArray(this->SpellCast.get());
	animation_set::AddAnimationToArray(this->Move.get());
	animation_set::AddAnimationToArray(this->Repair.get());
	animation_set::AddAnimationToArray(this->Train.get());
	for (int i = 0; i != MaxCosts; ++i) {
		animation_set::AddAnimationToArray(this->Harvest[i].get());
	}

	data_entry::initialize();
}

}

/**
**  Add a label
*/
static void AddLabel(CAnimation *anim, const std::string &name)
{
	LabelsStruct label;

	label.Anim = anim;
	label.Name = name;
	Labels.push_back(label);
}

/**
**  Find a label
*/
static CAnimation *FindLabel(lua_State *l, const std::string &name)
{
	for (size_t i = 0; i < Labels.size(); ++i) {
		if (Labels[i].Name == name) {
			return Labels[i].Anim;
		}
	}
	LuaError(l, "Label not found: %s" _C_ name.c_str());
	return nullptr;
}

/**
**  Find a label later
*/
void FindLabelLater(CAnimation **anim, const std::string &name)
{
	LabelsLaterStruct label;

	label.Anim = anim;
	label.Name = name;
	LabelsLater.push_back(label);
}

/**
**  Fix labels
*/
static void FixLabels(lua_State *l)
{
	for (size_t i = 0; i < LabelsLater.size(); ++i) {
		*LabelsLater[i].Anim = FindLabel(l, LabelsLater[i].Name);
	}
}


/**
**  Parse an animation frame
**
**  @param str  string formated as "animationType extraArgs"
*/
static std::unique_ptr<CAnimation> ParseAnimationFrame(lua_State *l, const char *str)
{
	const std::string all(str);
	const size_t len = all.size();
	size_t end = all.find(' ');
	const std::string op1(all, 0, end);
	size_t begin = std::min(len, all.find_first_not_of(' ', end));
	const std::string extraArg(all, begin);

	std::unique_ptr<CAnimation> anim;
	if (op1 == "frame") {
		anim = std::make_unique<CAnimation_Frame>();
	} else if (op1 == "exact-frame") {
		anim = std::make_unique<CAnimation_ExactFrame>();
	} else if (op1 == "wait") {
		anim = std::make_unique<CAnimation_Wait>();
	} else if (op1 == "random-wait") {
		anim = std::make_unique<CAnimation_RandomWait>();
	} else if (op1 == "sound") {
		anim = std::make_unique<CAnimation_Sound>();
	} else if (op1 == "random-sound") {
		anim = std::make_unique<CAnimation_RandomSound>();
	} else if (op1 == "attack") {
		anim = std::make_unique<CAnimation_Attack>();
	} else if (op1 == "spawn-missile") {
		anim = std::make_unique<CAnimation_SpawnMissile>();
	} else if (op1 == "spawn-unit") {
		anim = std::make_unique<CAnimation_SpawnUnit>();
	} else if (op1 == "if-var") {
		anim = std::make_unique<CAnimation_IfVar>();
	} else if (op1 == "set-var") {
		anim = std::make_unique<CAnimation_SetVar>();
	} else if (op1 == "die") {
		anim = std::make_unique<CAnimation_Die>();
	} else if (op1 == "rotate") {
		anim = std::make_unique<CAnimation_Rotate>();
	} else if (op1 == "random-rotate") {
		anim = std::make_unique<CAnimation_RandomRotate>();
	} else if (op1 == "move") {
		anim = std::make_unique<CAnimation_Move>();
	} else if (op1 == "unbreakable") {
		anim = std::make_unique<CAnimation_Unbreakable>();
	} else if (op1 == "label") {
		anim = std::make_unique<CAnimation_Label>();
		AddLabel(anim.get(), extraArg);
	} else if (op1 == "goto") {
		anim = std::make_unique<CAnimation_Goto>();
	} else if (op1 == "random-goto") {
		anim = std::make_unique<CAnimation_RandomGoto>();
	} else {
		LuaError(l, "Unknown animation: %s" _C_ op1.c_str());
	}
	anim->Init(extraArg.c_str(), l);
	return anim;
}

/**
**  Parse an animation
*/
static std::unique_ptr<CAnimation> ParseAnimation(lua_State *l, int idx)
{
	if (!lua_istable(l, idx)) {
		LuaError(l, "incorrect argument");
	}
	const int args = lua_rawlen(l, idx);

	if (args == 0) {
		return nullptr;
	}
	Labels.clear();
	LabelsLater.clear();

	const char *str = LuaToString(l, idx, 1);

	std::unique_ptr<CAnimation> firstAnim = ParseAnimationFrame(l, str);
	CAnimation *prev = firstAnim.get();
	for (int j = 1; j < args; ++j) {
		const char *secondary_str = LuaToString(l, idx, j + 1);
		std::unique_ptr<CAnimation> anim = ParseAnimationFrame(l, secondary_str);
		CAnimation *temp_anim = anim.get();
		prev->set_next(std::move(anim));
		prev = temp_anim;
	}
	prev->set_next(firstAnim.get());
	FixLabels(l);
	return firstAnim;
}

namespace wyrmgus {

void animation_set::AddAnimationToArray(CAnimation *anim)
{
	if (!anim) {
		return;
	}

	CAnimation::animation_list.push_back(anim);
}

}

/**
**  Define a unit-type animation set.
**
**  @param l  Lua state.
*/
static int CclDefineAnimations(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument");
	}

	const char *name = LuaToString(l, 1);
	wyrmgus::animation_set *anims = wyrmgus::animation_set::get_or_add(name, nullptr);

	lua_pushnil(l);
	while (lua_next(l, 2)) {
		const char *value = LuaToString(l, -2);

		if (!strcmp(value, "Start")) {
			anims->Start = ParseAnimation(l, -1);
		} else if (!strncmp(value, "Still", 5)) {
			anims->Still = ParseAnimation(l, -1);
		} else if (!strncmp(value, "Death", 5)) {
			if (strlen(value) > 5) {
				const int death = ExtraDeathIndex(value + 6);
				if (death == ANIMATIONS_DEATHTYPES) {
					anims->Death[ANIMATIONS_DEATHTYPES] = ParseAnimation(l, -1);
				} else {
					anims->Death[death] = ParseAnimation(l, -1);
				}
			} else {
				anims->Death[ANIMATIONS_DEATHTYPES] = ParseAnimation(l, -1);
			}
		} else if (!strcmp(value, "Attack")) {
			anims->Attack = ParseAnimation(l, -1);
		} else if (!strcmp(value, "RangedAttack")) {
			anims->RangedAttack = ParseAnimation(l, -1);
		} else if (!strcmp(value, "SpellCast")) {
			anims->SpellCast = ParseAnimation(l, -1);
		} else if (!strcmp(value, "Move")) {
			anims->Move = ParseAnimation(l, -1);
		} else if (!strcmp(value, "Repair")) {
			anims->Repair = ParseAnimation(l, -1);
		} else if (!strcmp(value, "Train")) {
			anims->Train = ParseAnimation(l, -1);
		} else if (!strcmp(value, "Research")) {
			anims->Research = ParseAnimation(l, -1);
		} else if (!strcmp(value, "Upgrade")) {
			anims->Upgrade = ParseAnimation(l, -1);
		} else if (!strcmp(value, "Build")) {
			anims->Build = ParseAnimation(l, -1);
		} else if (!strncmp(value, "Harvest_", 8)) {
			const int res = GetResourceIdByName(l, value + 8);
			anims->Harvest[res] = ParseAnimation(l, -1);
		} else {
			LuaError(l, "Unsupported animation: %s" _C_ value);
		}
		lua_pop(l, 1);
	}

	return 0;
}

void AnimationCclRegister()
{
	lua_register(Lua, "DefineAnimations", CclDefineAnimations);
}
