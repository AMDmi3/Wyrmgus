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
/**@name script_player.cpp - The player ccl functions. */
//
//      (c) Copyright 2001-2020 by Lutz Sammer, Jimmy Salmon and Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "player.h"

#include "actions.h"
#include "age.h"
#include "ai.h"
#include "character.h"
#include "civilization.h"
#include "commands.h"
#include "currency.h"
//Wyrmgus start
#include "editor.h"
#include "faction.h"
#include "font.h"
#include "grand_strategy.h"
#include "item.h"
#include "luacallback.h"
//Wyrmgus end
#include "map/map.h"
#include "map/site.h"
#include "plane.h"
#include "player_color.h"
#include "province.h"
#include "quest.h"
#include "religion/deity.h"
#include "religion/deity_domain.h"
#include "religion/pantheon.h"
#include "religion/religion.h"
#include "script.h"
#include "time/calendar.h"
#include "ui/button_action.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "video.h"
#include "upgrade/upgrade.h"
//Wyrmgus start
#include "ui/ui.h"
#include "util/util.h"
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern CUnit *CclGetUnitFromRef(lua_State *l);

/**
**  Get a player pointer
**
**  @param l  Lua state.
**
**  @return   The player pointer
*/
static CPlayer *CclGetPlayer(lua_State *l)
{
	return CPlayer::Players[LuaToNumber(l, -1)];
}

/**
**  Parse the player configuration.
**
**  @param l  Lua state.
*/
static int CclPlayer(lua_State *l)
{
	int i = LuaToNumber(l, 1);

	CPlayer &player = *CPlayer::Players[i];
	player.Index = i;

	if (NumPlayers <= i) {
		NumPlayers = i + 1;
	}

	player.Load(l);
	return 0;
}

void CPlayer::Load(lua_State *l)
{
	const int args = lua_gettop(l);

	this->Units.resize(0);
	this->FreeWorkers.resize(0);
	//Wyrmgus start
	this->LevelUpUnits.resize(0);
	//Wyrmgus end

	// j = 0 represent player Index.
	for (int j = 1; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		++j;

		if (!strcmp(value, "name")) {
			this->SetName(LuaToString(l, j + 1));
		} else if (!strcmp(value, "type")) {
			value = LuaToString(l, j + 1);
			if (!strcmp(value, "neutral")) {
				this->Type = PlayerNeutral;
			} else if (!strcmp(value, "nobody")) {
				this->Type = PlayerNobody;
			} else if (!strcmp(value, "computer")) {
				this->Type = PlayerComputer;
			} else if (!strcmp(value, "person")) {
				this->Type = PlayerPerson;
			} else if (!strcmp(value, "rescue-passive")) {
				this->Type = PlayerRescuePassive;
			} else if (!strcmp(value, "rescue-active")) {
				this->Type = PlayerRescueActive;
			} else {
				LuaError(l, "Unsupported tag: %s" _C_ value);
			}
		} else if (!strcmp(value, "race")) {
			const char *civilization_ident = LuaToString(l, j + 1);
			stratagus::civilization *civilization = stratagus::civilization::get(civilization_ident);
			this->Race = civilization->ID;
		//Wyrmgus start
		} else if (!strcmp(value, "faction")) {
			this->Faction = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "dynasty")) {
			this->Dynasty = PlayerRaces.GetDynasty(LuaToString(l, j + 1));
		} else if (!strcmp(value, "age")) {
			this->age = stratagus::age::get(LuaToString(l, j + 1));
		} else if (!strcmp(value, "player-color")) {
			this->player_color = stratagus::player_color::get(LuaToString(l, j + 1));
			this->minimap_color = player_color->get_colors()[0];
		//Wyrmgus end
		} else if (!strcmp(value, "ai-name")) {
			this->AiName = LuaToString(l, j + 1);
		} else if (!strcmp(value, "team")) {
			this->Team = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "enemy")) {
			value = LuaToString(l, j + 1);
			for (int i = 0; i < PlayerMax && *value; ++i, ++value) {
				if (*value == '-' || *value == '_' || *value == ' ') {
					this->Enemy &= ~(1 << i);
				} else {
					this->Enemy |= (1 << i);
				}
			}
		} else if (!strcmp(value, "allied")) {
			value = LuaToString(l, j + 1);
			for (int i = 0; i < PlayerMax && *value; ++i, ++value) {
				if (*value == '-' || *value == '_' || *value == ' ') {
					this->Allied &= ~(1 << i);
				} else {
					this->Allied |= (1 << i);
				}
			}
		} else if (!strcmp(value, "shared-vision")) {
			value = LuaToString(l, j + 1);
			for (int i = 0; i < PlayerMax && *value; ++i, ++value) {
				if (*value == '-' || *value == '_' || *value == ' ') {
					this->SharedVision &= ~(1 << i);
				} else {
					this->SharedVision |= (1 << i);
				}
			}
		} else if (!strcmp(value, "start")) {
			CclGetPos(l, &this->StartPos.x, &this->StartPos.y, j + 1);
		//Wyrmgus start
		} else if (!strcmp(value, "start-map-layer")) {
			this->StartMapLayer = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "overlord")) {
			int overlord_id = LuaToNumber(l, j + 1);
			this->SetOverlord(CPlayer::Players[overlord_id]);
		//Wyrmgus end
		} else if (!strcmp(value, "resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;
				const int resId = GetResourceIdByName(l, value);
				this->Resources[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "stored-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;

				const int resId = GetResourceIdByName(l, value);
				this->StoredResources[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "max-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;
				const int resId = GetResourceIdByName(l, value);
				this->MaxResources[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "last-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;
				const int resId = GetResourceIdByName(l, value);
				this->LastResources[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "incomes")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;

				const int resId = GetResourceIdByName(l, value);
				this->Incomes[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "revenue")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;

				const int resId = GetResourceIdByName(l, value);
				this->Revenue[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		//Wyrmgus start
		} else if (!strcmp(value, "prices")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;

				const int resId = GetResourceIdByName(l, value);
				this->Prices[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		//Wyrmgus end
		} else if (!strcmp(value, "ai-enabled")) {
			this->AiEnabled = true;
			--j;
		} else if (!strcmp(value, "ai-disabled")) {
			this->AiEnabled = false;
			--j;
		//Wyrmgus start
		} else if (!strcmp(value, "revealed")) {
			this->Revealed = true;
			--j;
		//Wyrmgus end
		} else if (!strcmp(value, "supply")) {
			this->Supply = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "demand")) {
			this->Demand = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "trade-cost")) {
			this->TradeCost = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "unit-limit")) {
			this->UnitLimit = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "building-limit")) {
			this->BuildingLimit = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "total-unit-limit")) {
			this->TotalUnitLimit = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "score")) {
			this->Score = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "total-units")) {
			this->TotalUnits = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "total-buildings")) {
			this->TotalBuildings = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "total-razings")) {
			this->TotalRazings = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "total-kills")) {
			this->TotalKills = LuaToNumber(l, j + 1);
		//Wyrmgus start
		} else if (!strcmp(value, "unit-type-kills")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				CUnitType *unit_type = CUnitType::get(LuaToString(l, j + 1, k + 1));
				++k;
				this->UnitTypeKills[unit_type->Slot] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "lost-town-hall-timer")) {
			this->LostTownHallTimer = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "hero-cooldown-timer")) {
			this->HeroCooldownTimer = LuaToNumber(l, j + 1);
		//Wyrmgus end
		} else if (!strcmp(value, "total-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			//Wyrmgus start
//			if (subargs != MaxCosts) {
//				LuaError(l, "Wrong number of total-resources: %d" _C_ subargs);
//			}
			if (subargs != MaxCosts) {
				fprintf(stderr, "Wrong number of total-resources: %d.\n", subargs);
			}
			//Wyrmgus end
			for (int k = 0; k < subargs; ++k) {
				this->TotalResources[k] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "speed-resource-harvest")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			//Wyrmgus start
//			if (subargs != MaxCosts) {
//				LuaError(l, "Wrong number of speed-resource-harvest: %d" _C_ subargs);
//			}
			if (subargs != MaxCosts) {
				fprintf(stderr, "Wrong number of speed-resource-harvest: %d.\n", subargs);
			}
			//Wyrmgus end
			for (int k = 0; k < subargs; ++k) {
				this->SpeedResourcesHarvest[k] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "speed-resource-return")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			//Wyrmgus start
//			if (subargs != MaxCosts) {
//				LuaError(l, "Wrong number of speed-resource-harvest: %d" _C_ subargs);
//			}
			if (subargs != MaxCosts) {
				fprintf(stderr, "Wrong number of speed-resource-return: %d.\n", subargs);
			}
			//Wyrmgus end
			for (int k = 0; k < subargs; ++k) {
				this->SpeedResourcesReturn[k] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "speed-build")) {
			this->SpeedBuild = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "speed-train")) {
			this->SpeedTrain = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "speed-upgrade")) {
			this->SpeedUpgrade = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "speed-research")) {
			this->SpeedResearch = LuaToNumber(l, j + 1);
		//Wyrmgus start
		/*
		} else if (!strcmp(value, "color")) {
			if (!lua_istable(l, j + 1) || lua_rawlen(l, j + 1) != 3) {
				LuaError(l, "incorrect argument");
			}
			const int r = LuaToNumber(l, j + 1, 1);
			const int g = LuaToNumber(l, j + 1, 2);
			const int b = LuaToNumber(l, j + 1, 3);
			this->Color = Video.MapRGB(TheScreen->format, r, g, b);
		*/
		//Wyrmgus end
		//Wyrmgus start
		} else if (!strcmp(value, "current-quests")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				stratagus::quest *quest = stratagus::quest::get(LuaToString(l, j + 1, k + 1));
				this->CurrentQuests.push_back(quest);
			}
		} else if (!strcmp(value, "completed-quests")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				stratagus::quest *quest = stratagus::quest::get(LuaToString(l, j + 1, k + 1));
				this->CompletedQuests.push_back(quest);
				if (quest->Competitive) {
					quest->CurrentCompleted = true;
				}
			}
		} else if (!strcmp(value, "quest-objectives")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, j + 1, k + 1);
				const stratagus::quest *quest = nullptr;
				CPlayerQuestObjective *objective = nullptr;
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for quest objectives)");
				}
				const int subsubargs = lua_rawlen(l, -1);
				for (int n = 0; n < subsubargs; ++n) {
					value = LuaToString(l, -1, n + 1);
					++n;
					if (!strcmp(value, "quest")) {
						quest = stratagus::quest::get(LuaToString(l, -1, n + 1));
					} else if (!strcmp(value, "objective-index")) {
						const int objective_index = LuaToNumber(l, -1, n + 1);
						objective = new CPlayerQuestObjective(quest->Objectives[objective_index]);
						this->QuestObjectives.push_back(objective);
					} else if (!strcmp(value, "counter")) {
						objective->Counter = LuaToNumber(l, -1, n + 1);
					} else {
						LuaError(l, "Invalid quest objective property.");
					}
				}
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "modifiers")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				CUpgrade *modifier_upgrade = CUpgrade::get(LuaToString(l, j + 1, k + 1));
				++k;
				int end_cycle = LuaToNumber(l, j + 1, k + 1);
				this->Modifiers.push_back(std::pair<CUpgrade *, int>(modifier_upgrade, end_cycle));
			}
		//Wyrmgus end
		} else if (!strcmp(value, "autosell-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const int res = GetResourceIdByName(LuaToString(l, j + 1, k + 1));
				if (res != -1) {
					this->AutosellResources.push_back(res);
				}
			}
		} else if (!strcmp(value, "timers")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			//Wyrmgus start
//			if (subargs != UpgradeMax) {
//				LuaError(l, "Wrong upgrade timer length: %d" _C_ subargs);
//			}
			//Wyrmgus end
			for (int k = 0; k < subargs; ++k) {
				//Wyrmgus start
//				this->UpgradeTimers.Upgrades[k] = LuaToNumber(l, j + 1, k + 1);
				CUpgrade *timer_upgrade = CUpgrade::get(LuaToString(l, j + 1, k + 1));
				++k;
				this->UpgradeTimers.Upgrades[timer_upgrade->ID] = LuaToNumber(l, j + 1, k + 1);
				//Wyrmgus end
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	// Manage max
	for (int i = 0; i < MaxCosts; ++i) {
		if (this->MaxResources[i] != -1) {
			this->SetResource(i, this->Resources[i] + this->StoredResources[i], STORE_BOTH);
		}
	}
}

/**
**  Change unit owner
**
**  @param l  Lua state.
*/
static int CclChangeUnitsOwner(lua_State *l)
{
	LuaCheckArgs(l, 4);

	Vec2i pos1;
	Vec2i pos2;
	CclGetPos(l, &pos1.x, &pos1.y, 1);
	CclGetPos(l, &pos2.x, &pos2.y, 2);
	const int oldp = LuaToNumber(l, 3);
	const int newp = LuaToNumber(l, 4);
	std::vector<CUnit *> table;

	//Wyrmgus start
//	Select(pos1, pos2, table, HasSamePlayerAs(*CPlayer::Players[oldp]));
	Select(pos1, pos2, table, 0, HasSamePlayerAs(*CPlayer::Players[oldp]));
	//Wyrmgus end
	for (size_t i = 0; i != table.size(); ++i) {
		table[i]->ChangeOwner(*CPlayer::Players[newp]);
	}
	return 0;
}

/**
**  Get ThisPlayer.
**
**  @param l  Lua state.
*/
static int CclGetThisPlayer(lua_State *l)
{
	LuaCheckArgs(l, 0);
	if (CPlayer::GetThisPlayer()) {
		lua_pushnumber(l, CPlayer::GetThisPlayer()->Index);
	} else {
		lua_pushnumber(l, 0);
	}
	return 1;
}

/**
**  Set ThisPlayer.
**
**  @param l  Lua state.
*/
static int CclSetThisPlayer(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int plynr = LuaToNumber(l, 1);

	CPlayer::SetThisPlayer(CPlayer::Players[plynr]);
	
	//Wyrmgus start
	UI.Load();
	//Wyrmgus end

	lua_pushnumber(l, plynr);
	return 1;
}

/**
**  Set MaxSelectable
**
**  @param l  Lua state.
*/
static int CclSetMaxSelectable(lua_State *l)
{
	LuaCheckArgs(l, 1);
	MaxSelectable = LuaToNumber(l, 1);

	lua_pushnumber(l, MaxSelectable);
	return 1;
}

/**
**  Set player unit limit.
**
**  @param l  Lua state.
*/
static int CclSetAllPlayersUnitLimit(lua_State *l)
{
	LuaCheckArgs(l, 1);
	for (int i = 0; i < PlayerMax; ++i) {
		CPlayer::Players[i]->UnitLimit = LuaToNumber(l, 1);
	}

	lua_pushnumber(l, lua_tonumber(l, 1));
	return 1;
}

/**
**  Set player unit limit.
**
**  @param l  Lua state.
*/
static int CclSetAllPlayersBuildingLimit(lua_State *l)
{
	LuaCheckArgs(l, 1);
	for (int i = 0; i < PlayerMax; ++i) {
		CPlayer::Players[i]->BuildingLimit = LuaToNumber(l, 1);
	}

	lua_pushnumber(l, lua_tonumber(l, 1));
	return 1;
}

/**
**  Set player unit limit.
**
**  @param l  Lua state.
*/
static int CclSetAllPlayersTotalUnitLimit(lua_State *l)
{
	LuaCheckArgs(l, 1);
	for (int i = 0; i < PlayerMax; ++i) {
		CPlayer::Players[i]->TotalUnitLimit = LuaToNumber(l, 1);
	}

	lua_pushnumber(l, lua_tonumber(l, 1));
	return 1;
}

/**
**  Change the diplomacy from player to another player.
**
**  @param l  Lua state.
**
**  @return          FIXME: should return old state.
*/
static int CclSetDiplomacy(lua_State *l)
{
	LuaCheckArgs(l, 3);
	const int base = LuaToNumber(l, 1);
	const int plynr = LuaToNumber(l, 3);
	const char *state = LuaToString(l, 2);

	if (!strcmp(state, "allied")) {
		SendCommandDiplomacy(base, diplomacy_state::allied, plynr);
	} else if (!strcmp(state, "neutral")) {
		SendCommandDiplomacy(base, diplomacy_state::neutral, plynr);
	} else if (!strcmp(state, "crazy")) {
		SendCommandDiplomacy(base, diplomacy_state::crazy, plynr);
	} else if (!strcmp(state, "enemy")) {
		SendCommandDiplomacy(base, diplomacy_state::enemy, plynr);
	//Wyrmgus start
	} else if (!strcmp(state, "overlord")) {
		SendCommandDiplomacy(base, diplomacy_state::overlord, plynr);
	} else if (!strcmp(state, "vassal")) {
		SendCommandDiplomacy(base, diplomacy_state::vassal, plynr);
	//Wyrmgus end
	}
	return 0;
}

/**
**  Change the diplomacy from ThisPlayer to another player.
**
**  @param l  Lua state.
*/
static int CclDiplomacy(lua_State *l)
{
	lua_pushnumber(l, CPlayer::GetThisPlayer()->Index);
	lua_insert(l, 1);
	return CclSetDiplomacy(l);
}

/**
**  Change the shared vision from player to another player.
**
**  @param l  Lua state.
**
**  @return   FIXME: should return old state.
*/
static int CclSetSharedVision(lua_State *l)
{
	LuaCheckArgs(l, 3);

	const int base = LuaToNumber(l, 1);
	const bool shared = LuaToBoolean(l, 2);
	const int plynr = LuaToNumber(l, 3);

	SendCommandSharedVision(base, shared, plynr);

	return 0;
}

/**
**  Change the shared vision from ThisPlayer to another player.
**
**  @param l  Lua state.
*/
static int CclSharedVision(lua_State *l)
{
	lua_pushnumber(l, CPlayer::GetThisPlayer()->Index);
	lua_insert(l, 1);
	return CclSetSharedVision(l);
}

//Wyrmgus start
/**
**  Define a civilization.
**
**  @param l  Lua state.
*/
static int CclDefineCivilization(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string civilization_name = LuaToString(l, 1);
	stratagus::civilization *civilization = stratagus::civilization::get_or_add(civilization_name, nullptr);
	int civilization_id = civilization->ID;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		if (!strcmp(value, "Display")) {
			civilization->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Description")) {
			civilization->set_description(LuaToString(l, -1));
		} else if (!strcmp(value, "Quote")) {
			civilization->set_quote(LuaToString(l, -1));
		} else if (!strcmp(value, "Background")) {
			civilization->set_background(LuaToString(l, -1));
		} else if (!strcmp(value, "Adjective")) {
			civilization->Adjective = LuaToString(l, -1);
		} else if (!strcmp(value, "Interface")) {
			civilization->interface = LuaToString(l, -1);
		} else if (!strcmp(value, "Visible")) {
			civilization->visible = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Playable")) {
			civilization->playable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Species")) {
			PlayerRaces.Species[civilization_id] = LuaToString(l, -1);
		} else if (!strcmp(value, "ParentCivilization")) {
			civilization->parent_civilization = stratagus::civilization::get(LuaToString(l, -1));
		} else if (!strcmp(value, "Language")) {
			CLanguage *language = PlayerRaces.GetLanguage(LuaToString(l, -1));
			if (language) {
				civilization->Language = language;
				language->used_by_civilization_or_faction = true;
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "Calendar")) {
			stratagus::calendar *calendar = stratagus::calendar::get(LuaToString(l, -1));
			civilization->calendar = calendar;
		} else if (!strcmp(value, "Currency")) {
			CCurrency *currency = CCurrency::GetCurrency(LuaToString(l, -1));
			civilization->Currency = currency;
		} else if (!strcmp(value, "DefaultColor")) {
			civilization->default_color = LuaToString(l, -1);
		} else if (!strcmp(value, "CivilizationUpgrade")) {
			PlayerRaces.civilization_upgrades[civilization_id] = LuaToString(l, -1);
		} else if (!strcmp(value, "DevelopsFrom")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string originary_civilization_name = LuaToString(l, -1, j + 1);
				stratagus::civilization *originary_civilization = stratagus::civilization::get(originary_civilization_name);
				PlayerRaces.DevelopsFrom[civilization_id].push_back(originary_civilization->ID);
				PlayerRaces.DevelopsTo[originary_civilization->ID].push_back(civilization_id);
			}
		} else if (!strcmp(value, "ButtonIcons")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string button_action_name = LuaToString(l, -1, j + 1);
				const ButtonCmd button_action = GetButtonActionIdByName(button_action_name);
				if (button_action != ButtonCmd::None) {
					++j;
					PlayerRaces.ButtonIcons[civilization_id][button_action].Name = LuaToString(l, -1, j + 1);
					PlayerRaces.ButtonIcons[civilization_id][button_action].Icon = nullptr;
					PlayerRaces.ButtonIcons[civilization_id][button_action].Load();
				} else {
					LuaError(l, "Button action \"%s\" doesn't exist." _C_ button_action_name.c_str());
				}
			}
		} else if (!strcmp(value, "ForceTypeWeights")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			civilization->ForceTypeWeights.clear();
			
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int force_type = GetForceTypeIdByName(LuaToString(l, -1, j + 1));
				++j;
				civilization->ForceTypeWeights[force_type] = LuaToNumber(l, -1, j + 1);
			}
		} else if (!strcmp(value, "ForceTemplates")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				CForceTemplate *force = new CForceTemplate;
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for force templates)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "force-type")) {
						force->ForceType = GetForceTypeIdByName(LuaToString(l, -1, k + 1));
						if (force->ForceType == -1) {
							LuaError(l, "Force type doesn't exist.");
						}
						civilization->ForceTemplates[force->ForceType].push_back(force);
					} else if (!strcmp(value, "priority")) {
						force->Priority = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "weight")) {
						force->Weight = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "unit-class")) {
						const stratagus::unit_class *unit_class = stratagus::unit_class::get(LuaToString(l, -1, k + 1));
						++k;
						int unit_quantity = LuaToNumber(l, -1, k + 1);
						force->add_unit(unit_class, unit_quantity);
					} else {
						printf("\n%s\n", civilization->get_identifier().c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				lua_pop(l, 1);
			}
			for (std::map<int, std::vector<CForceTemplate *>>::iterator iterator = civilization->ForceTemplates.begin(); iterator != civilization->ForceTemplates.end(); ++iterator) {
				std::sort(iterator->second.begin(), iterator->second.end(), [](CForceTemplate *a, CForceTemplate *b) {
					return a->Priority > b->Priority;
				});
			}
		} else if (!strcmp(value, "AiBuildingTemplates")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				CAiBuildingTemplate *building_template = new CAiBuildingTemplate;
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for force templates)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "unit-class")) {
						const stratagus::unit_class *unit_class = stratagus::unit_class::get(LuaToString(l, -1, k + 1));
						building_template->set_unit_class(unit_class);
						civilization->AiBuildingTemplates.push_back(building_template);
					} else if (!strcmp(value, "priority")) {
						building_template->set_priority(LuaToNumber(l, -1, k + 1));
					} else if (!strcmp(value, "per-settlement")) {
						building_template->set_per_settlement(LuaToBoolean(l, -1, k + 1));
					} else {
						printf("\n%s\n", civilization->get_identifier().c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				lua_pop(l, 1);
			}
			std::sort(civilization->AiBuildingTemplates.begin(), civilization->AiBuildingTemplates.end(), [](CAiBuildingTemplate *a, CAiBuildingTemplate *b) {
				return a->get_priority() > b->get_priority();
			});
		} else if (!strcmp(value, "UIFillers")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			PlayerRaces.civilization_ui_fillers[civilization_id].clear();
			
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CFiller filler = CFiller();
				std::string filler_file = LuaToString(l, -1, j + 1);
				if (filler_file.empty()) {
					LuaError(l, "Filler graphic file is empty.");
				}				
				filler.G = CGraphic::New(filler_file);
				++j;
				filler.X = LuaToNumber(l, -1, j + 1);
				++j;
				filler.Y = LuaToNumber(l, -1, j + 1);
				PlayerRaces.civilization_ui_fillers[civilization_id].push_back(filler);
			}
		} else if (!strcmp(value, "UnitSounds")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, -1, k + 1);
				++k;

				if (!strcmp(value, "selected")) {
					civilization->UnitSounds.Selected.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "acknowledge")) {
					civilization->UnitSounds.Acknowledgement.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "attack")) {
					civilization->UnitSounds.Attack.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "idle")) {
					civilization->UnitSounds.Idle.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "build")) {
					civilization->UnitSounds.Build.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "ready")) {
					civilization->UnitSounds.Ready.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "repair")) {
					civilization->UnitSounds.Repair.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "harvest")) {
					const std::string name = LuaToString(l, -1, k + 1);
					++k;
					const int resId = GetResourceIdByName(l, name.c_str());
					civilization->UnitSounds.Harvest[resId].Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "help")) {
					civilization->UnitSounds.Help.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "help-town")) {
					civilization->UnitSounds.HelpTown.Name = LuaToString(l, -1, k + 1);
				} else {
					LuaError(l, "Unsupported sound tag: %s" _C_ value);
				}
			}
		} else if (!strcmp(value, "PersonalNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				int gender_id = GetGenderIdByName(LuaToString(l, -1, j + 1));
				if (gender_id == -1) {
					gender_id = NoGender;
				} else {
					++j;
				}
				
				civilization->PersonalNames[gender_id].push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "UnitClassNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string class_name = LuaToString(l, -1, j + 1);
				if (class_name.empty()) {
					LuaError(l, "Class is given as a blank string.");
				}
				const stratagus::unit_class *unit_class = stratagus::unit_class::get(class_name);
				++j;
				
				civilization->unit_class_names[unit_class].push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "FamilyNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				civilization->FamilyNames.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "ProvinceNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				civilization->ProvinceNames.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "ShipNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				civilization->ship_names.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "MinisterTitles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				int title = GetCharacterTitleIdByName(LuaToString(l, -1, k + 1));
				++k;
				int gender = GetGenderIdByName(LuaToString(l, -1, k + 1));
				++k;
				int government_type = GetGovernmentTypeIdByName(LuaToString(l, -1, k + 1));
				++k;
				const faction_tier tier = GetFactionTierIdByName(LuaToString(l, -1, k + 1));
				++k;
				civilization->MinisterTitles[title][gender][government_type][static_cast<int>(tier)] = LuaToString(l, -1, k + 1);
			}
		} else if (!strcmp(value, "HistoricalUpgrades")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CDate date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &date);
				lua_pop(l, 1);
				++j;

				std::string technology_ident = LuaToString(l, -1, j + 1);
				++j;
				
				bool has_upgrade = LuaToBoolean(l, -1, j + 1);

				civilization->HistoricalUpgrades[technology_ident][date] = has_upgrade;
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}

	civilization->initialize();	
	
	return 0;
}

/**
**  Define a word for a particular language.
**
**  @param l  Lua state.
*/
static int CclDefineLanguageWord(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	LanguageWord *word = new LanguageWord;
	word->Word = LuaToString(l, 1);
	
	LanguageWord *replaces = nullptr;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Language")) {
			CLanguage *language = PlayerRaces.GetLanguage(LuaToString(l, -1));
			
			if (language) {
				language->LanguageWords.push_back(word);
				word->Language = language;
				
				for (size_t i = 0; i < language->Dialects.size(); ++i) { //copy the word over for dialects
					language->Dialects[i]->LanguageWords.push_back(word);
				}
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "Meanings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				word->Meanings.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "Type")) {
			std::string word_type_name = LuaToString(l, -1);
			int word_type = GetWordTypeIdByName(word_type_name);
			if (word_type != -1) {
				word->Type = word_type;
			} else {
				LuaError(l, "Word type \"%s\" doesn't exist." _C_ word_type_name.c_str());
			}
		} else if (!strcmp(value, "DerivesFrom")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			int j = 0;
			CLanguage *derives_from_language = PlayerRaces.GetLanguage(LuaToString(l, -1, j + 1));
			++j;
			int derives_from_word_type = GetWordTypeIdByName(LuaToString(l, -1, j + 1));
			++j;
			
			std::vector<std::string> word_meanings;
			lua_rawgeti(l, -1, j + 1);
			if (lua_istable(l, -1)) {
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					word_meanings.push_back(LuaToString(l, -1, k + 1));
				}
				
				++j;
			}
			lua_pop(l, 1);
			
			if (derives_from_language && derives_from_word_type != -1) {
				std::string derives_from_word = LuaToString(l, -1, j + 1);
				word->DerivesFrom = derives_from_language->GetWord(derives_from_word, derives_from_word_type, word_meanings);
				
				if (word->DerivesFrom != nullptr) {
					word->DerivesFrom->DerivesTo.push_back(word);
				} else {
					LuaError(l, "Word \"%s\" is set to derive from \"%s\" (%s, %s), but the latter doesn't exist" _C_ word->Word.c_str() _C_ derives_from_word.c_str() _C_ derives_from_language->Ident.c_str() _C_ GetWordTypeNameById(derives_from_word_type).c_str());
				}
			} else {
				LuaError(l, "Word \"%s\"'s derives from is incorrectly set, as either the language or the word type set for the original word given is incorrect" _C_ word->Word.c_str());
			}
		} else if (!strcmp(value, "Replaces")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			int j = 0;
			CLanguage *replaces_language = PlayerRaces.GetLanguage(LuaToString(l, -1, j + 1));
			++j;
			int replaces_word_type = GetWordTypeIdByName(LuaToString(l, -1, j + 1));
			++j;
			
			std::vector<std::string> word_meanings;
			lua_rawgeti(l, -1, j + 1);
			if (lua_istable(l, -1)) {
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					word_meanings.push_back(LuaToString(l, -1, k + 1));
				}
				
				++j;
			}
			lua_pop(l, 1);
			
			if (replaces_language && replaces_word_type != -1) {
				std::string replaces_word = LuaToString(l, -1, j + 1);
				replaces = replaces_language->GetWord(replaces_word, replaces_word_type, word_meanings);
				
				if (replaces == nullptr) {
					LuaError(l, "Word \"%s\" is set to replace \"%s\" (%s, %s), but the latter doesn't exist" _C_ word->Word.c_str() _C_ replaces_word.c_str() _C_ replaces_language->Ident.c_str() _C_ GetWordTypeNameById(replaces_word_type).c_str());
				}
			} else {
				LuaError(l, "Word \"%s\"'s replace is incorrectly set, as either the language or the word type set for the original word given is incorrect" _C_ word->Word.c_str());
			}
		} else if (!strcmp(value, "CompoundElements")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string affix_type_name = LuaToString(l, -1, j + 1);
				int affix_type = GetAffixTypeIdByName(affix_type_name);
				if (affix_type == -1) {
					LuaError(l, "Affix type \"%s\" doesn't exist." _C_ affix_type_name.c_str());
				}
				++j;
				
				CLanguage *affix_language = PlayerRaces.GetLanguage(LuaToString(l, -1, j + 1)); // should be the same language as that of the word, but needs to be specified since the word's language may not have been set yet
				++j;
				int affix_word_type = GetWordTypeIdByName(LuaToString(l, -1, j + 1));
				++j;
				
				std::vector<std::string> word_meanings;
				lua_rawgeti(l, -1, j + 1);
				if (lua_istable(l, -1)) {
					const int subargs = lua_rawlen(l, -1);
					for (int k = 0; k < subargs; ++k) {
						word_meanings.push_back(LuaToString(l, -1, k + 1));
					}
					
					++j;
				}
				lua_pop(l, 1);

				if (affix_language && affix_word_type != -1) {
					std::string affix_word = LuaToString(l, -1, j + 1);
					word->CompoundElements[affix_type] = affix_language->GetWord(affix_word, affix_word_type, word_meanings);
					
					if (word->CompoundElements[affix_type] != nullptr) {
						word->CompoundElements[affix_type]->CompoundElementOf[affix_type].push_back(word);
					} else {
						LuaError(l, "Word \"%s\" is set to be a compound formed by \"%s\" (%s, %s), but the latter doesn't exist" _C_ word->Word.c_str() _C_ affix_word.c_str() _C_ affix_language->Ident.c_str() _C_ GetWordTypeNameById(affix_word_type).c_str());
					}
				} else {
					LuaError(l, "Word \"%s\"'s compound elements are incorrectly set, as either the language or the word type set for one of the element words given is incorrect" _C_ word->Word.c_str());
				}
			}
		} else if (!strcmp(value, "Gender")) {
			std::string grammatical_gender_name = LuaToString(l, -1);
			int grammatical_gender = GetGrammaticalGenderIdByName(grammatical_gender_name);
			if (grammatical_gender != -1) {
				word->Gender = grammatical_gender;
			} else {
				LuaError(l, "Grammatical gender \"%s\" doesn't exist." _C_ grammatical_gender_name.c_str());
			}
		} else if (!strcmp(value, "GrammaticalNumber")) {
			std::string grammatical_number_name = LuaToString(l, -1);
			int grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
			if (grammatical_number != -1) {
				word->GrammaticalNumber = grammatical_number;
			} else {
				LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
			}
		} else if (!strcmp(value, "Archaic")) {
			word->Archaic = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "NumberCaseInflections")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string grammatical_number_name = LuaToString(l, -1, j + 1);
				int grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
				if (grammatical_number == -1) {
					LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
				}
				++j;
				
				std::string grammatical_case_name = LuaToString(l, -1, j + 1);
				int grammatical_case = GetGrammaticalCaseIdByName(grammatical_case_name);
				if (grammatical_case == -1) {
					LuaError(l, "Grammatical case \"%s\" doesn't exist." _C_ grammatical_case_name.c_str());
				}
				++j;

				word->NumberCaseInflections[std::tuple<int, int>(grammatical_number, grammatical_case)] = LuaToString(l, -1, j + 1);
			}
		} else if (!strcmp(value, "NumberPersonTenseMoodInflections")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string grammatical_number_name = LuaToString(l, -1, j + 1);
				int grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
				if (grammatical_number == -1) {
					LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
				}
				++j;
				
				std::string grammatical_person_name = LuaToString(l, -1, j + 1);
				int grammatical_person = GetGrammaticalPersonIdByName(grammatical_person_name);
				if (grammatical_person == -1) {
					LuaError(l, "Grammatical person \"%s\" doesn't exist." _C_ grammatical_person_name.c_str());
				}
				++j;
				
				std::string grammatical_tense_name = LuaToString(l, -1, j + 1);
				int grammatical_tense = GetGrammaticalTenseIdByName(grammatical_tense_name);
				if (grammatical_tense == -1) {
					LuaError(l, "Grammatical tense \"%s\" doesn't exist." _C_ grammatical_tense_name.c_str());
				}
				++j;
				
				std::string grammatical_mood_name = LuaToString(l, -1, j + 1);
				int grammatical_mood = GetGrammaticalMoodIdByName(grammatical_mood_name);
				if (grammatical_mood == -1) {
					LuaError(l, "Grammatical mood \"%s\" doesn't exist." _C_ grammatical_mood_name.c_str());
				}
				++j;

				word->NumberPersonTenseMoodInflections[std::tuple<int, int, int, int>(grammatical_number, grammatical_person, grammatical_tense, grammatical_mood)] = LuaToString(l, -1, j + 1);
			}
		} else if (!strcmp(value, "ComparisonDegreeCaseInflections")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string comparison_degree_name = LuaToString(l, -1, j + 1);
				int comparison_degree = GetComparisonDegreeIdByName(comparison_degree_name);
				if (comparison_degree == -1) {
					LuaError(l, "Comparison degree \"%s\" doesn't exist." _C_ comparison_degree_name.c_str());
				}
				++j;
				
				int grammatical_case = GrammaticalCaseNoCase;
				if (GetGrammaticalCaseIdByName(LuaToString(l, -1, j + 1)) != -1) {
					std::string grammatical_case_name = LuaToString(l, -1, j + 1);
					grammatical_case = GetGrammaticalCaseIdByName(grammatical_case_name);
					if (grammatical_case == -1) {
						LuaError(l, "Grammatical case \"%s\" doesn't exist." _C_ grammatical_case_name.c_str());
					}
					++j;
				}
				
				word->ComparisonDegreeCaseInflections[comparison_degree][grammatical_case] = LuaToString(l, -1, j + 1);
			}
		} else if (!strcmp(value, "Participles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string grammatical_tense_name = LuaToString(l, -1, j + 1);
				int grammatical_tense = GetGrammaticalTenseIdByName(grammatical_tense_name);
				if (grammatical_tense == -1) {
					LuaError(l, "Grammatical tense \"%s\" doesn't exist." _C_ grammatical_tense_name.c_str());
				}
				++j;
				
				word->Participles[grammatical_tense] = LuaToString(l, -1, j + 1);
			}
		//noun-specific variables
		} else if (!strcmp(value, "Uncountable")) {
			word->Uncountable = LuaToBoolean(l, -1);
		//pronoun and article-specific variables
		} else if (!strcmp(value, "Nominative")) {
			word->Nominative = LuaToString(l, -1);
		} else if (!strcmp(value, "Accusative")) {
			word->Accusative = LuaToString(l, -1);
		} else if (!strcmp(value, "Dative")) {
			word->Dative = LuaToString(l, -1);
		} else if (!strcmp(value, "Genitive")) {
			word->Genitive = LuaToString(l, -1);
		//article-specific variables
		} else if (!strcmp(value, "ArticleType")) {
			std::string article_type_name = LuaToString(l, -1);
			int article_type = GetArticleTypeIdByName(article_type_name);
			if (article_type != -1) {
				word->ArticleType = article_type;
			} else {
				LuaError(l, "Article type \"%s\" doesn't exist." _C_ article_type_name.c_str());
			}
		//numeral-specific variables
		} else if (!strcmp(value, "Number")) {
			word->Number = LuaToNumber(l, -1);
		//type name variables
		} else if (!strcmp(value, "Mod")) {
			word->Mod = LuaToString(l, -1);
		} else if (!strcmp(value, "MapWord")) { //to keep backwards compatibility
			word->Mod = CMap::Map.Info.Filename;
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (!word->Language) {
		LuaError(l, "Word \"%s\" has not been assigned to any language" _C_ word->Word.c_str());
	}
	
	if (word->Type == -1) {
		LuaError(l, "Word \"%s\" has no type" _C_ word->Word.c_str());
	}
	
	if (replaces != nullptr) {
		word->Language->RemoveWord(replaces);
	}
	
	return 0;
}

/**
**  Get a civilization's data.
**
**  @param l  Lua state.
*/
static int CclGetCivilizationData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string civilization_name = LuaToString(l, 1);
	stratagus::civilization *civilization = stratagus::civilization::get(civilization_name);
	if (!civilization) {
		return 0;
	}
	int civilization_id = civilization->ID;
	
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Display")) {
		lua_pushstring(l, civilization->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, civilization->get_description().c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, civilization->get_quote().c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, civilization->get_background().c_str());
		return 1;
	} else if (!strcmp(data, "Adjective")) {
		if (!civilization->Adjective.empty()) {
			lua_pushstring(l, civilization->Adjective.c_str());
		} else {
			lua_pushstring(l, civilization->get_name().c_str());
		}
		return 1;
	} else if (!strcmp(data, "Interface")) {
		lua_pushstring(l, civilization->get_interface().c_str());
		return 1;
	} else if (!strcmp(data, "Playable")) {
		lua_pushboolean(l, civilization->is_playable());
		return 1;
	} else if (!strcmp(data, "Species")) {
		lua_pushstring(l, PlayerRaces.Species[civilization_id].c_str());
		return 1;
	} else if (!strcmp(data, "ParentCivilization")) {
		if (civilization->parent_civilization) {
			lua_pushstring(l, civilization->parent_civilization->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Language")) {
		CLanguage *language = PlayerRaces.get_civilization_language(civilization_id);
		if (language) {
			lua_pushstring(l, language->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "DefaultColor")) {
		lua_pushstring(l, civilization->get_default_color().c_str());
		return 1;
	} else if (!strcmp(data, "CivilizationUpgrade")) {
		lua_pushstring(l, PlayerRaces.civilization_upgrades[civilization_id].c_str());
		return 1;
	} else if (!strcmp(data, "DevelopsFrom")) {
		lua_createtable(l, PlayerRaces.DevelopsFrom[civilization_id].size(), 0);
		for (size_t i = 1; i <= PlayerRaces.DevelopsFrom[civilization_id].size(); ++i)
		{
			lua_pushstring(l, stratagus::civilization::get_all()[PlayerRaces.DevelopsFrom[civilization_id][i-1]]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "DevelopsTo")) {
		lua_createtable(l, PlayerRaces.DevelopsTo[civilization_id].size(), 0);
		for (size_t i = 1; i <= PlayerRaces.DevelopsTo[civilization_id].size(); ++i)
		{
			lua_pushstring(l, stratagus::civilization::get_all()[PlayerRaces.DevelopsTo[civilization_id][i-1]]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Factions")) {
		bool is_mod = false;
		if (lua_gettop(l) >= 3) {
			is_mod = true;
		}
		
		std::string mod_file;

		if (is_mod) {
			mod_file = LuaToString(l, 3);
		}
		
		std::vector<std::string> factions;
		for (stratagus::faction *faction : stratagus::faction::get_all())
		{
			if (faction->get_civilization() != civilization) {
				continue;
			}
			
			if (!is_mod || faction->Mod == mod_file) {
				factions.push_back(faction->get_identifier());
			}
		}
		
		lua_createtable(l, factions.size(), 0);
		for (size_t i = 1; i <= factions.size(); ++i)
		{
			lua_pushstring(l, factions[i-1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Quests")) {
		lua_createtable(l, civilization->Quests.size(), 0);
		for (size_t i = 1; i <= civilization->Quests.size(); ++i)
		{
			lua_pushstring(l, civilization->Quests[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Get a civilization's unit type/upgrade of a certain class.
**
**  @param l  Lua state.
*/
static int CclGetCivilizationClassUnitType(lua_State *l)
{
	LuaCheckArgs(l, 2);
	std::string class_name = LuaToString(l, 1);
	const stratagus::unit_class *unit_class = stratagus::unit_class::try_get(class_name);
	stratagus::civilization *civilization = stratagus::civilization::get(LuaToString(l, 2));
	std::string unit_type_ident;
	if (civilization && unit_class != nullptr) {
		const CUnitType *unit_type = civilization->get_class_unit_type(unit_class);
		if (unit_type != nullptr) {
			unit_type_ident = unit_type->get_identifier();
		}
	}
		
	if (unit_type_ident.empty()) { //if wasn't found, see if it is an upgrade class instead
		const int class_id = GetUpgradeClassIndexByName(class_name);
		if (civilization && class_id != -1) {
			int upgrade_id = PlayerRaces.get_civilization_class_upgrade(civilization->ID, class_id);
			if (upgrade_id != -1) {
				unit_type_ident = CUpgrade::get_all()[upgrade_id]->Ident;
			}
		}
	}
	
	if (!unit_type_ident.empty()) {
		lua_pushstring(l, unit_type_ident.c_str());
	} else {
		lua_pushnil(l);
	}

	return 1;
}


/**
**  Get a faction's unit type/upgrade of a certain class.
**
**  @param l  Lua state.
*/
static int CclGetFactionClassUnitType(lua_State *l)
{
	std::string class_name = LuaToString(l, 1);
	const stratagus::unit_class *unit_class = stratagus::unit_class::try_get(class_name);
	int faction_id = -1;
	stratagus::faction *faction = nullptr;
	const int nargs = lua_gettop(l);
	if (nargs == 2) {
		faction = stratagus::faction::get(LuaToString(l, 2));
		faction_id = faction->ID;
	} else if (nargs == 3) {
		//the civilization was the second argument, but it isn't needed anymore
		faction = stratagus::faction::get(LuaToString(l, 3));
		faction_id = faction->ID;
	}
	std::string unit_type_ident;
	if (unit_class != nullptr) {
		const CUnitType *unit_type = faction->get_class_unit_type(unit_class);
		if (unit_type != nullptr) {
			unit_type_ident = unit_type->get_identifier();
		}
	}
		
	if (unit_type_ident.empty()) { //if wasn't found, see if it is an upgrade class instead
		const int class_id = GetUpgradeClassIndexByName(class_name);
		if (class_id != -1) {
			int upgrade_id = PlayerRaces.GetFactionClassUpgrade(faction_id, class_id);
			if (upgrade_id != -1) {
				unit_type_ident = CUpgrade::get_all()[upgrade_id]->Ident;
			}
		}
	}
	
	if (!unit_type_ident.empty()) {
		lua_pushstring(l, unit_type_ident.c_str());
	} else {
		lua_pushnil(l);
	}

	return 1;
}

/**
**  Define a faction.
**
**  @param l  Lua state.
*/
static int CclDefineFaction(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string faction_name = LuaToString(l, 1);
	std::string parent_faction;
	
	stratagus::faction *faction = stratagus::faction::get_or_add(faction_name, nullptr);
	if (faction) { // redefinition
		if (faction->ParentFaction != -1) {
			parent_faction = stratagus::faction::get_all()[faction->ParentFaction]->get_identifier();
		}
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Civilization")) {
			stratagus::civilization *civilization = stratagus::civilization::get(LuaToString(l, -1));
			faction->civilization = civilization;
		} else if (!strcmp(value, "Name")) {
			faction->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Description")) {
			faction->set_description(LuaToString(l, -1));
		} else if (!strcmp(value, "Quote")) {
			faction->set_quote(LuaToString(l, -1));
		} else if (!strcmp(value, "Background")) {
			faction->set_background(LuaToString(l, -1));
		} else if (!strcmp(value, "Adjective")) {
			faction->Adjective = LuaToString(l, -1);
		} else if (!strcmp(value, "Type")) {
			std::string faction_type_name = LuaToString(l, -1);
			int faction_type = GetFactionTypeIdByName(faction_type_name);
			if (faction_type != -1) {
				faction->Type = faction_type;
			} else {
				LuaError(l, "Faction type \"%s\" doesn't exist." _C_ faction_type_name.c_str());
			}
		} else if (!strcmp(value, "Colors")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			faction->player_colors.clear(); //remove previously defined colors
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				const std::string color_name = LuaToString(l, -1, k + 1);
				faction->player_colors.push_back(stratagus::player_color::get(color_name));
			}
		} else if (!strcmp(value, "DefaultTier")) {
			std::string faction_tier_name = LuaToString(l, -1);
			const faction_tier tier = GetFactionTierIdByName(faction_tier_name);
			if (tier != faction_tier::none) {
				faction->default_tier = tier;
			} else {
				LuaError(l, "Faction tier \"%s\" doesn't exist." _C_ faction_tier_name.c_str());
			}
		} else if (!strcmp(value, "DefaultGovernmentType")) {
			std::string government_type_name = LuaToString(l, -1);
			int government_type = GetGovernmentTypeIdByName(government_type_name);
			if (government_type != -1) {
				faction->DefaultGovernmentType = government_type;
			} else {
				LuaError(l, "Government type \"%s\" doesn't exist." _C_ government_type_name.c_str());
			}
		} else if (!strcmp(value, "DefaultAI")) {
			faction->DefaultAI = LuaToString(l, -1);
		} else if (!strcmp(value, "ParentFaction")) {
			parent_faction = LuaToString(l, -1);
		} else if (!strcmp(value, "Playable")) {
			faction->Playable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "DefiniteArticle")) {
			faction->DefiniteArticle = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Icon")) {
			faction->icon = CIcon::get(LuaToString(l, -1));
		} else if (!strcmp(value, "Currency")) {
			CCurrency *currency = CCurrency::GetCurrency(LuaToString(l, -1));
			faction->Currency = currency;
		} else if (!strcmp(value, "DevelopsFrom")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				stratagus::faction *second_faction = stratagus::faction::get(LuaToString(l, -1, k + 1));
				faction->DevelopsFrom.push_back(second_faction);
				second_faction->DevelopsTo.push_back(faction);
			}
		} else if (!strcmp(value, "DevelopsTo")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				stratagus::faction *second_faction = stratagus::faction::get(LuaToString(l, -1, k + 1));
				faction->DevelopsTo.push_back(second_faction);
				second_faction->DevelopsFrom.push_back(faction);
			}
		} else if (!strcmp(value, "Titles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				int government_type = GetGovernmentTypeIdByName(LuaToString(l, -1, k + 1));
				++k;
				const faction_tier tier = GetFactionTierIdByName(LuaToString(l, -1, k + 1));
				++k;
				faction->Titles[government_type][static_cast<int>(tier)] = LuaToString(l, -1, k + 1);
			}
		} else if (!strcmp(value, "MinisterTitles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				int title = GetCharacterTitleIdByName(LuaToString(l, -1, k + 1));
				++k;
				int gender = GetGenderIdByName(LuaToString(l, -1, k + 1));
				++k;
				int government_type = GetGovernmentTypeIdByName(LuaToString(l, -1, k + 1));
				++k;
				const faction_tier tier = GetFactionTierIdByName(LuaToString(l, -1, k + 1));
				++k;
				faction->MinisterTitles[title][gender][government_type][static_cast<int>(tier)] = LuaToString(l, -1, k + 1);
			}
		} else if (!strcmp(value, "FactionUpgrade")) {
			faction->FactionUpgrade = LuaToString(l, -1);
		} else if (!strcmp(value, "ButtonIcons")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string button_action_name = LuaToString(l, -1, j + 1);
				const ButtonCmd button_action = GetButtonActionIdByName(button_action_name);
				if (button_action != ButtonCmd::None) {
					++j;
					faction->ButtonIcons[button_action].Name = LuaToString(l, -1, j + 1);
					faction->ButtonIcons[button_action].Icon = nullptr;
					faction->ButtonIcons[button_action].Load();
				} else {
					LuaError(l, "Button action \"%s\" doesn't exist." _C_ button_action_name.c_str());
				}
			}
		} else if (!strcmp(value, "ForceTypeWeights")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			faction->ForceTypeWeights.clear();
			
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int force_type = GetForceTypeIdByName(LuaToString(l, -1, j + 1));
				++j;
				faction->ForceTypeWeights[force_type] = LuaToNumber(l, -1, j + 1);
			}
		} else if (!strcmp(value, "ForceTemplates")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				CForceTemplate *force = new CForceTemplate;
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for force templates)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "force-type")) {
						force->ForceType = GetForceTypeIdByName(LuaToString(l, -1, k + 1));
						if (force->ForceType == -1) {
							LuaError(l, "Force type doesn't exist.");
						}
						faction->ForceTemplates[force->ForceType].push_back(force);
					} else if (!strcmp(value, "priority")) {
						force->Priority = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "weight")) {
						force->Weight = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "unit-class")) {
						const stratagus::unit_class *unit_class = stratagus::unit_class::get(LuaToString(l, -1, k + 1));
						++k;
						const int unit_quantity = LuaToNumber(l, -1, k + 1);
						force->add_unit(unit_class, unit_quantity);
					} else {
						printf("\n%s\n", faction->get_identifier().c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				lua_pop(l, 1);
			}
			for (std::map<int, std::vector<CForceTemplate *>>::iterator iterator = faction->ForceTemplates.begin(); iterator != faction->ForceTemplates.end(); ++iterator) {
				std::sort(iterator->second.begin(), iterator->second.end(), [](CForceTemplate *a, CForceTemplate *b) {
					return a->Priority > b->Priority;
				});
			}
		} else if (!strcmp(value, "AiBuildingTemplates")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				CAiBuildingTemplate *building_template = new CAiBuildingTemplate;
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for force templates)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "unit-class")) {
						const stratagus::unit_class *unit_class = stratagus::unit_class::get(LuaToString(l, -1, k + 1));
						building_template->set_unit_class(unit_class);
						faction->AiBuildingTemplates.push_back(building_template);
					} else if (!strcmp(value, "priority")) {
						building_template->set_priority(LuaToNumber(l, -1, k + 1));
					} else if (!strcmp(value, "per-settlement")) {
						building_template->set_per_settlement(LuaToBoolean(l, -1, k + 1));
					} else {
						printf("\n%s\n", faction->get_identifier().c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				lua_pop(l, 1);
			}
			std::sort(faction->AiBuildingTemplates.begin(), faction->AiBuildingTemplates.end(), [](CAiBuildingTemplate *a, CAiBuildingTemplate *b) {
				return a->get_priority() > b->get_priority();
			});
		} else if (!strcmp(value, "UIFillers")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			faction->UIFillers.clear();
			
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CFiller filler = CFiller();
				std::string filler_file = LuaToString(l, -1, j + 1);
				if (filler_file.empty()) {
					LuaError(l, "Filler graphic file is empty.");
				}
				filler.G = CGraphic::New(filler_file);
				++j;
				filler.X = LuaToNumber(l, -1, j + 1);
				++j;
				filler.Y = LuaToNumber(l, -1, j + 1);
				faction->UIFillers.push_back(filler);
			}
		} else if (!strcmp(value, "Conditions")) {
			faction->Conditions = new LuaCallback(l, -1);
		} else if (!strcmp(value, "ProvinceNames")) {
			faction->ProvinceNames.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				faction->ProvinceNames.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "ShipNames")) {
			faction->ship_names.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				faction->ship_names.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "HistoricalUpgrades")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CDate date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &date);
				lua_pop(l, 1);
				++j;

				std::string technology_ident = LuaToString(l, -1, j + 1);
				++j;
				
				bool has_upgrade = LuaToBoolean(l, -1, j + 1);

				faction->HistoricalUpgrades[technology_ident][date] = has_upgrade;
			}
		} else if (!strcmp(value, "HistoricalTiers")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				std::string faction_tier_name = LuaToString(l, -1, j + 1);
				const faction_tier tier = GetFactionTierIdByName(faction_tier_name);
				if (tier == faction_tier::none) {
					LuaError(l, "Faction tier \"%s\" doesn't exist." _C_ faction_tier_name.c_str());
				}
				faction->HistoricalTiers[year] = tier;
			}
		} else if (!strcmp(value, "HistoricalGovernmentTypes")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				std::string government_type_name = LuaToString(l, -1, j + 1);
				int government_type = GetGovernmentTypeIdByName(government_type_name);
				if (government_type == -1) {
					LuaError(l, "Government type \"%s\" doesn't exist." _C_ government_type_name.c_str());
				}
				faction->HistoricalGovernmentTypes[year] = government_type;
			}
		} else if (!strcmp(value, "HistoricalDiplomacyStates")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CDate date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &date);
				lua_pop(l, 1);
				++j;
				
				std::string diplomacy_state_faction_ident = LuaToString(l, -1, j + 1);
				stratagus::faction *diplomacy_state_faction = stratagus::faction::get(diplomacy_state_faction_ident);
				++j;

				std::string diplomacy_state_name = LuaToString(l, -1, j + 1);
				const diplomacy_state diplomacy_state = GetDiplomacyStateIdByName(diplomacy_state_name);
				faction->HistoricalDiplomacyStates[std::pair<CDate, stratagus::faction *>(date, diplomacy_state_faction)] = diplomacy_state;
			}
		} else if (!strcmp(value, "HistoricalResources")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CDate date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &date);
				lua_pop(l, 1);
				++j;
				
				std::string resource_ident = LuaToString(l, -1, j + 1);
				int resource = GetResourceIdByName(l, resource_ident.c_str());
				if (resource == -1) {
					LuaError(l, "Resource \"%s\" doesn't exist." _C_ resource_ident.c_str());
				}
				++j;

				faction->HistoricalResources[std::pair<CDate, int>(date, resource)] = LuaToNumber(l, -1, j + 1);
			}
		} else if (!strcmp(value, "HistoricalCapitals")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CDate date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &date);
				lua_pop(l, 1);
				++j;
				
				std::string site_ident = LuaToString(l, -1, j + 1);

				faction->HistoricalCapitals.push_back(std::pair<CDate, std::string>(date, site_ident));
			}
		} else if (!strcmp(value, "Mod")) {
			faction->Mod = LuaToString(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
		
	if (!parent_faction.empty()) { //process this here
		faction->ParentFaction = stratagus::faction::get(parent_faction)->ID;
	} else if (parent_faction.empty()) {
		faction->ParentFaction = -1; // to allow redefinitions to remove the parent faction setting
	}
	
	return 0;
}

/**
**  Define a dynasty.
**
**  @param l  Lua state.
*/
static int CclDefineDynasty(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string dynasty_ident = LuaToString(l, 1);
	
	CDynasty *dynasty = PlayerRaces.GetDynasty(dynasty_ident);
	if (!dynasty) { // new definition
		dynasty = new CDynasty;
		dynasty->Ident = dynasty_ident;
		dynasty->ID = PlayerRaces.Dynasties.size();
		PlayerRaces.Dynasties.push_back(dynasty);
		DynastyStringToIndex[dynasty->Ident] = dynasty->ID;
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Civilization")) {
			stratagus::civilization *civilization = stratagus::civilization::get(LuaToString(l, -1));
			dynasty->civilization = civilization->ID;
		} else if (!strcmp(value, "Name")) {
			dynasty->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Description")) {
			dynasty->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Quote")) {
			dynasty->Quote = LuaToString(l, -1);
		} else if (!strcmp(value, "Background")) {
			dynasty->Background = LuaToString(l, -1);
		} else if (!strcmp(value, "Icon")) {
			dynasty->Icon.Name = LuaToString(l, -1);
			dynasty->Icon.Icon = nullptr;
			dynasty->Icon.Load();
		} else if (!strcmp(value, "Factions")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				stratagus::faction *faction = stratagus::faction::get(LuaToString(l, -1, k + 1));
				dynasty->Factions.push_back(faction);
				faction->Dynasties.push_back(dynasty);
			}
		} else if (!strcmp(value, "DynastyUpgrade")) {
			dynasty->DynastyUpgrade = CUpgrade::get(LuaToString(l, -1));
		} else if (!strcmp(value, "Conditions")) {
			dynasty->Conditions = new LuaCallback(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a religion.
**
**  @param l  Lua state.
*/
static int CclDefineReligion(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string religion_ident = LuaToString(l, 1);
	CReligion *religion = CReligion::GetOrAddReligion(religion_ident);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			religion->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Description")) {
			religion->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Background")) {
			religion->Background = LuaToString(l, -1);
		} else if (!strcmp(value, "Quote")) {
			religion->Quote = LuaToString(l, -1);
		} else if (!strcmp(value, "CulturalDeities")) {
			religion->CulturalDeities = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Domains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CDeityDomain *deity_domain = CDeityDomain::GetDeityDomain(LuaToString(l, -1, j + 1));
				if (deity_domain) {
					religion->Domains.push_back(deity_domain);
				}
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a deity.
**
**  @param l  Lua state.
*/
static int CclDefineDeity(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string deity_ident = LuaToString(l, 1);
	CDeity *deity = CDeity::GetOrAddDeity(deity_ident);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			deity->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Pantheon")) {
			deity->Pantheon = CPantheon::GetPantheon(LuaToString(l, -1));
		} else if (!strcmp(value, "Gender")) {
			deity->Gender = GetGenderIdByName(LuaToString(l, -1));
		} else if (!strcmp(value, "Major")) {
			deity->Major = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Description")) {
			deity->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Background")) {
			deity->Background = LuaToString(l, -1);
		} else if (!strcmp(value, "Quote")) {
			deity->Quote = LuaToString(l, -1);
		} else if (!strcmp(value, "HomePlane")) {
			stratagus::plane *plane = stratagus::plane::get(LuaToString(l, -1));
			deity->home_plane = plane;
		} else if (!strcmp(value, "DeityUpgrade")) {
			CUpgrade *upgrade = CUpgrade::get(LuaToString(l, -1));
			deity->DeityUpgrade = upgrade;
			CDeity::DeitiesByUpgrade[upgrade] = deity;
		} else if (!strcmp(value, "CharacterUpgrade")) {
			CUpgrade *upgrade = CUpgrade::get(LuaToString(l, -1));
			deity->CharacterUpgrade = upgrade;
		} else if (!strcmp(value, "Icon")) {
			deity->Icon.Name = LuaToString(l, -1);
			deity->Icon.Icon = nullptr;
			deity->Icon.Load();
		} else if (!strcmp(value, "Civilizations")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				stratagus::civilization *civilization = stratagus::civilization::get(LuaToString(l, -1, j + 1));
				deity->civilizations.push_back(civilization);
				civilization->Deities.push_back(deity);
			}
		} else if (!strcmp(value, "Religions")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CReligion *religion = CReligion::GetReligion(LuaToString(l, -1, j + 1));
				if (religion) {
					deity->Religions.push_back(religion);
				}
			}
		} else if (!strcmp(value, "Domains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CDeityDomain *deity_domain = CDeityDomain::GetDeityDomain(LuaToString(l, -1, j + 1));
				if (deity_domain) {
					deity->Domains.push_back(deity_domain);
				}
			}
		} else if (!strcmp(value, "HolyOrders")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				stratagus::faction *holy_order = stratagus::faction::get(LuaToString(l, -1, j + 1));
				deity->HolyOrders.push_back(holy_order);
				holy_order->HolyOrderDeity = deity;
			}
		} else if (!strcmp(value, "Abilities")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CUpgrade *ability = CUpgrade::get(LuaToString(l, -1, j + 1));
				if (!ability->is_ability()) {
					LuaError(l, "Ability upgrade is not actually an ability.");
				}

				if (std::find(deity->Abilities.begin(), deity->Abilities.end(), ability) == deity->Abilities.end()) {
					deity->Abilities.push_back(ability);
				}
			}
		} else if (!strcmp(value, "Feasts")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string feast = LuaToString(l, -1, j + 1);

				deity->Feasts.push_back(feast);
			}
		} else if (!strcmp(value, "CulturalNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const stratagus::civilization *civilization = stratagus::civilization::get(LuaToString(l, -1, j + 1));
				++j;

				std::string cultural_name = LuaToString(l, -1, j + 1);
				deity->CulturalNames[civilization] = cultural_name;
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (deity->Major && deity->Domains.size() > MAJOR_DEITY_DOMAIN_MAX) {
		deity->Domains.resize(MAJOR_DEITY_DOMAIN_MAX);
	} else if (!deity->Major && deity->Domains.size() > MINOR_DEITY_DOMAIN_MAX) {
		deity->Domains.resize(MINOR_DEITY_DOMAIN_MAX);
	}
	
	for (CDeityDomain *domain : deity->Domains) {
		for (CUpgrade *ability : domain->Abilities) {
			if (std::find(deity->Abilities.begin(), deity->Abilities.end(), ability) == deity->Abilities.end()) {
				deity->Abilities.push_back(ability);
			}
		}
	}
	
	return 0;
}

/**
**  Define a language.
**
**  @param l  Lua state.
*/
static int CclDefineLanguage(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string language_ident = LuaToString(l, 1);
	CLanguage *language = PlayerRaces.GetLanguage(language_ident);
	if (!language) {
		language = new CLanguage;
		PlayerRaces.Languages.push_back(language);
		LanguageIdentToPointer[language_ident] = language;
	}
	
	language->Ident = language_ident;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			language->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Family")) {
			language->Family = LuaToString(l, -1);
		} else if (!strcmp(value, "DialectOf")) {
			CLanguage *parent_language = PlayerRaces.GetLanguage(LuaToString(l, -1));
			if (parent_language) {
				language->DialectOf = parent_language;
				parent_language->Dialects.push_back(language);
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "NounEndings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				std::string grammatical_number_name = LuaToString(l, -1, k + 1);
				int grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
				if (grammatical_number == -1) {
					LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
				}
				++k;
				
				std::string grammatical_case_name = LuaToString(l, -1, k + 1);
				int grammatical_case = GetGrammaticalCaseIdByName(grammatical_case_name);
				if (grammatical_case == -1) {
					LuaError(l, "Grammatical case \"%s\" doesn't exist." _C_ grammatical_case_name.c_str());
				}
				++k;
				
				int word_junction_type = WordJunctionTypeNoWordJunction;
				if (GetWordJunctionTypeIdByName(LuaToString(l, -1, k + 1)) != -1) {
					std::string word_junction_type_name = LuaToString(l, -1, k + 1);
					int word_junction_type = GetWordJunctionTypeIdByName(word_junction_type_name);
					if (word_junction_type == -1) {
						LuaError(l, "Word junction type \"%s\" doesn't exist." _C_ word_junction_type_name.c_str());
					}
					++k;
				}

				language->NounEndings[grammatical_number][grammatical_case][word_junction_type] = LuaToString(l, -1, k + 1);
			}
		} else if (!strcmp(value, "AdjectiveEndings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				std::string article_type_name = LuaToString(l, -1, k + 1);
				int article_type = GetArticleTypeIdByName(article_type_name);
				if (article_type == -1) {
					LuaError(l, "Article type \"%s\" doesn't exist." _C_ article_type_name.c_str());
				}
				++k;

				std::string grammatical_case_name = LuaToString(l, -1, k + 1);
				int grammatical_case = GetGrammaticalCaseIdByName(grammatical_case_name);
				if (grammatical_case == -1) {
					LuaError(l, "Grammatical case \"%s\" doesn't exist." _C_ grammatical_case_name.c_str());
				}
				++k;

				std::string grammatical_number_name = LuaToString(l, -1, k + 1);
				int grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
				if (grammatical_number == -1) {
					LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
				}
				++k;
				
				std::string grammatical_gender_name = LuaToString(l, -1, k + 1);
				int grammatical_gender = GetGrammaticalGenderIdByName(grammatical_gender_name);
				if (grammatical_gender == -1) {
					LuaError(l, "Grammatical gender \"%s\" doesn't exist." _C_ grammatical_gender_name.c_str());
				}
				++k;
				
				language->AdjectiveEndings[article_type][grammatical_case][grammatical_number][grammatical_gender] = LuaToString(l, -1, k + 1);
			}
		} else if (!strcmp(value, "NameTranslations")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				std::string translation_from = LuaToString(l, -1, k + 1); //name to be translated
				++k;
				std::string translation_to = LuaToString(l, -1, k + 1); //name translation
				language->NameTranslations[translation_from].push_back(translation_to);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}
//Wyrmgus end

/**
**  Get the civilizations.
**
**  @param l  Lua state.
*/
static int CclGetCivilizations(lua_State *l)
{
	const int nargs = lua_gettop(l);
	bool only_visible = false;
	if (nargs >= 1) {
		only_visible = LuaToBoolean(l, 1);
	}

	std::vector<std::string> civilization_idents;
	for (const stratagus::civilization *civilization : stratagus::civilization::get_all()) {
		if (!only_visible || civilization->is_visible()) {
			civilization_idents.push_back(civilization->get_identifier());
		}
	}

	lua_createtable(l, civilization_idents.size(), 0);
	for (unsigned int i = 1; i <= civilization_idents.size(); ++i)
	{
		lua_pushstring(l, civilization_idents[i - 1].c_str());
		lua_rawseti(l, -2, i);
	}
	
	return 1;
}

/**
**  Get the factions.
**
**  @param l  Lua state.
*/
static int CclGetFactions(lua_State *l)
{
	stratagus::civilization *civilization = nullptr;
	if (lua_gettop(l) >= 1) {
		civilization = stratagus::civilization::try_get(LuaToString(l, 1));
	}
	
	int faction_type = -1;
	if (lua_gettop(l) >= 2) {
		faction_type = GetFactionTypeIdByName(LuaToString(l, 2));
	}
	
	std::vector<std::string> factions;
	if (civilization != nullptr) {
		for (stratagus::faction *faction : stratagus::faction::get_all()) {
			if (faction_type != -1 && faction->Type != faction_type) {
				continue;
			}
			if (faction->get_civilization() == civilization) {
				factions.push_back(faction->get_identifier());
			}
		}
	} else {
		for (stratagus::faction *faction : stratagus::faction::get_all()) {
			if (faction_type != -1 && faction->Type != faction_type) {
				continue;
			}
			factions.push_back(faction->get_identifier());
		}
	}
		
	lua_createtable(l, factions.size(), 0);
	for (size_t i = 1; i <= factions.size(); ++i)
	{
		lua_pushstring(l, factions[i-1].c_str());
		lua_rawseti(l, -2, i);
	}
	
	return 1;
}

/**
**  Get the player colors.
**
**  @param l  Lua state.
*/
static int CclGetPlayerColors(lua_State *l)
{
	lua_createtable(l, stratagus::player_color::get_all().size(), 0);
	for (size_t i = 1; i <= stratagus::player_color::get_all().size(); ++i)
	{
		lua_pushstring(l, stratagus::player_color::get_all()[i-1]->get_identifier().c_str());
		lua_rawseti(l, -2, i);
	}
	
	return 1;
}

/**
**  Get faction data.
**
**  @param l  Lua state.
*/
static int CclGetFactionData(lua_State *l)
{
	LuaCheckArgs(l, 2);
	std::string faction_name = LuaToString(l, 1);
	stratagus::faction *faction = stratagus::faction::get(faction_name);
	
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, faction->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, faction->get_description().c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, faction->get_quote().c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, faction->get_background().c_str());
		return 1;
	} else if (!strcmp(data, "Adjective")) {
		if (!faction->Adjective.empty()) {
			lua_pushstring(l, faction->Adjective.c_str());
		} else {
			lua_pushstring(l, faction->get_name().c_str());
		}
		return 1;
	} else if (!strcmp(data, "Type")) {
		lua_pushstring(l, GetFactionTypeNameById(faction->Type).c_str());
		return 1;
	} else if (!strcmp(data, "Civilization")) {
		if (faction->get_civilization() != nullptr) {
			lua_pushstring(l, faction->get_civilization()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Color")) {
		if (faction->get_player_color() != nullptr) {
			lua_pushstring(l, faction->get_player_color()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Playable")) {
		lua_pushboolean(l, faction->Playable);
		return 1;
	} else if (!strcmp(data, "FactionUpgrade")) {
		lua_pushstring(l, faction->FactionUpgrade.c_str());
		return 1;
	} else if (!strcmp(data, "ParentFaction")) {
		if (faction->ParentFaction != -1) {
			lua_pushstring(l, stratagus::faction::get_all()[faction->ParentFaction]->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "DefaultAI")) {
		lua_pushstring(l, faction->DefaultAI.c_str());
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Get dynasty data.
**
**  @param l  Lua state.
*/
static int CclGetDynastyData(lua_State *l)
{
	LuaCheckArgs(l, 2);
	std::string dynasty_ident = LuaToString(l, 1);
	CDynasty *dynasty = PlayerRaces.GetDynasty(dynasty_ident);
	if (dynasty == nullptr) {
		LuaError(l, "Dynasty \"%s\" doesn't exist." _C_ dynasty_ident.c_str());
	}
	
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, dynasty->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, dynasty->Description.c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, dynasty->Quote.c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, dynasty->Background.c_str());
		return 1;
	} else if (!strcmp(data, "Civilization")) {
		if (dynasty->civilization != -1) {
			lua_pushstring(l, stratagus::civilization::get_all()[dynasty->civilization]->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "DynastyUpgrade")) {
		if (dynasty->DynastyUpgrade) {
			lua_pushstring(l, dynasty->DynastyUpgrade->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Factions")) {
		lua_createtable(l, dynasty->Factions.size(), 0);
		for (size_t i = 1; i <= dynasty->Factions.size(); ++i)
		{
			lua_pushstring(l, dynasty->Factions[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}
//Wyrmgus end

// ----------------------------------------------------------------------------

/**
**  Get player data.
**
**  @param l  Lua state.
*/
static int CclGetPlayerData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	lua_pushvalue(l, 1);
	const CPlayer *p = CclGetPlayer(l);
	lua_pop(l, 1);
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, p->Name.c_str());
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Faction")) {
		if (p->Race != -1 && p->Faction != -1) {
			lua_pushstring(l, stratagus::faction::get_all()[p->Faction]->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Dynasty")) {
		if (p->Dynasty) {
			lua_pushstring(l, p->Dynasty->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "RaceName")) {
		lua_pushstring(l, stratagus::civilization::get_all()[p->Race]->get_identifier().c_str());
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Color")) {
		if (p->get_player_color() == nullptr) {
			LuaError(l, "Player %d has no color." _C_ p->Index);
		}
		lua_pushstring(l, p->get_player_color()->get_identifier().c_str());
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "Resources")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->Resources[resId] + p->StoredResources[resId]);
		return 1;
	} else if (!strcmp(data, "StoredResources")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->StoredResources[resId]);
		return 1;
	} else if (!strcmp(data, "MaxResources")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->MaxResources[resId]);
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Prices")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->GetResourcePrice(resId));
		return 1;
	} else if (!strcmp(data, "ResourceDemand")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->ResourceDemand[resId]);
		return 1;
	} else if (!strcmp(data, "StoredResourceDemand")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->StoredResourceDemand[resId]);
		return 1;
	} else if (!strcmp(data, "EffectiveResourceDemand")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->GetEffectiveResourceDemand(resId));
		return 1;
	} else if (!strcmp(data, "EffectiveResourceSellPrice")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->GetEffectiveResourceSellPrice(resId));
		return 1;
	} else if (!strcmp(data, "EffectiveResourceBuyPrice")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->GetEffectiveResourceBuyPrice(resId));
		return 1;
	} else if (!strcmp(data, "TotalPriceDifferenceWith")) {
		LuaCheckArgs(l, 3);
		
		int other_player = LuaToNumber(l, 3);;

		lua_pushnumber(l, p->GetTotalPriceDifferenceWith(*CPlayer::Players[other_player]));
		return 1;
	} else if (!strcmp(data, "TradePotentialWith")) {
		LuaCheckArgs(l, 3);
		
		int other_player = LuaToNumber(l, 3);;

		lua_pushnumber(l, p->GetTradePotentialWith(*CPlayer::Players[other_player]));
		return 1;
	} else if (!strcmp(data, "HasHero")) {
		LuaCheckArgs(l, 3);
		
		CCharacter *hero = CCharacter::get(LuaToString(l, 3));

		lua_pushboolean(l, p->HasHero(hero));
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "UnitTypesCount")) {
		LuaCheckArgs(l, 3);
		CUnitType *type = CclGetUnitType(l);
		Assert(type);
		lua_pushnumber(l, p->GetUnitTypeCount(type));
		return 1;
	} else if (!strcmp(data, "UnitTypesUnderConstructionCount")) {
		LuaCheckArgs(l, 3);
		CUnitType *type = CclGetUnitType(l);
		Assert(type);
		lua_pushnumber(l, p->GetUnitTypeUnderConstructionCount(type));
		return 1;
	} else if (!strcmp(data, "UnitTypesAiActiveCount")) {
		LuaCheckArgs(l, 3);
		CUnitType *type = CclGetUnitType(l);
		Assert(type);
		lua_pushnumber(l, p->GetUnitTypeAiActiveCount(type));
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Heroes")) {
		lua_createtable(l, p->Heroes.size(), 0);
		for (size_t i = 1; i <= p->Heroes.size(); ++i)
		{
			lua_pushstring(l, p->Heroes[i-1]->Character->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "AiEnabled")) {
		lua_pushboolean(l, p->AiEnabled);
		return 1;
	} else if (!strcmp(data, "TotalNumUnits")) {
		lua_pushnumber(l, p->GetUnitCount());
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "TotalNumUnitsConstructed")) {
		lua_pushnumber(l, p->GetUnitCount() - p->NumBuildingsUnderConstruction);
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "NumBuildings")) {
		lua_pushnumber(l, p->NumBuildings);
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "NumBuildingsUnderConstruction")) {
		lua_pushnumber(l, p->NumBuildingsUnderConstruction);
		return 1;
	} else if (!strcmp(data, "NumTownHalls")) {
		lua_pushnumber(l, p->NumTownHalls);
		return 1;
	} else if (!strcmp(data, "NumHeroes")) {
		lua_pushnumber(l, p->Heroes.size());
		return 1;
	} else if (!strcmp(data, "TradeCost")) {
		lua_pushnumber(l, p->TradeCost);
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "Supply")) {
		lua_pushnumber(l, p->Supply);
		return 1;
	} else if (!strcmp(data, "Demand")) {
		lua_pushnumber(l, p->Demand);
		return 1;
	} else if (!strcmp(data, "UnitLimit")) {
		lua_pushnumber(l, p->UnitLimit);
		return 1;
	} else if (!strcmp(data, "BuildingLimit")) {
		lua_pushnumber(l, p->BuildingLimit);
		return 1;
	} else if (!strcmp(data, "TotalUnitLimit")) {
		lua_pushnumber(l, p->TotalUnitLimit);
		return 1;
	} else if (!strcmp(data, "Score")) {
		lua_pushnumber(l, p->Score);
		return 1;
	} else if (!strcmp(data, "TotalUnits")) {
		lua_pushnumber(l, p->TotalUnits);
		return 1;
	} else if (!strcmp(data, "TotalBuildings")) {
		lua_pushnumber(l, p->TotalBuildings);
		return 1;
	} else if (!strcmp(data, "TotalResources")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->TotalResources[resId]);
		return 1;
	} else if (!strcmp(data, "TotalRazings")) {
		lua_pushnumber(l, p->TotalRazings);
		return 1;
	} else if (!strcmp(data, "TotalKills")) {
		lua_pushnumber(l, p->TotalKills);
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "UnitTypeKills")) {
		LuaCheckArgs(l, 3);
		CUnitType *type = CclGetUnitType(l);
		Assert(type);
		lua_pushnumber(l, p->UnitTypeKills[type->Slot]);
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "SpeedResourcesHarvest")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->SpeedResourcesHarvest[resId]);
		return 1;
	} else if (!strcmp(data, "SpeedResourcesReturn")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->SpeedResourcesReturn[resId]);
		return 1;
	} else if (!strcmp(data, "SpeedBuild")) {
		lua_pushnumber(l, p->SpeedBuild);
		return 1;
	} else if (!strcmp(data, "SpeedTrain")) {
		lua_pushnumber(l, p->SpeedTrain);
		return 1;
	} else if (!strcmp(data, "SpeedUpgrade")) {
		lua_pushnumber(l, p->SpeedUpgrade);
		return 1;
	} else if (!strcmp(data, "SpeedResearch")) {
		lua_pushnumber(l, p->SpeedResearch);
		return 1;
	} else if (!strcmp(data, "Allow")) {
		LuaCheckArgs(l, 3);
		const char *ident = LuaToString(l, 3);
		if (!strncmp(ident, "unit-", 5)) {
			int id = UnitTypeIdByIdent(ident);
			if (UnitIdAllowed(*CPlayer::Players[p->Index], id) > 0) {
				lua_pushstring(l, "A");
			} else if (UnitIdAllowed(*CPlayer::Players[p->Index], id) == 0) {
				lua_pushstring(l, "F");
			}
		} else if (!strncmp(ident, "upgrade", 7)) {
			if (UpgradeIdentAllowed(*CPlayer::Players[p->Index], ident) == 'A') {
				lua_pushstring(l, "A");
			} else if (UpgradeIdentAllowed(*CPlayer::Players[p->Index], ident) == 'R') {
				lua_pushstring(l, "R");
			} else if (UpgradeIdentAllowed(*CPlayer::Players[p->Index], ident) == 'F') {
				lua_pushstring(l, "F");
			}
		} else {
			DebugPrint(" wrong ident %s\n" _C_ ident);
		}
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "HasContactWith")) {
		LuaCheckArgs(l, 3);
		int second_player = LuaToNumber(l, 3);
		lua_pushboolean(l, p->HasContactWith(*CPlayer::Players[second_player]));
		return 1;
	} else if (!strcmp(data, "HasQuest")) {
		LuaCheckArgs(l, 3);
		stratagus::quest *quest = stratagus::quest::get(LuaToString(l, 3));
		if (std::find(p->CurrentQuests.begin(), p->CurrentQuests.end(), quest) != p->CurrentQuests.end()) {
			lua_pushboolean(l, true);
		} else {
			lua_pushboolean(l, false);
		}
		return 1;
	} else if (!strcmp(data, "CompletedQuest")) {
		LuaCheckArgs(l, 3);
		stratagus::quest *quest = stratagus::quest::get(LuaToString(l, 3));
		if (std::find(p->CompletedQuests.begin(), p->CompletedQuests.end(), quest) != p->CompletedQuests.end()) {
			lua_pushboolean(l, true);
		} else {
			lua_pushboolean(l, false);
		}
		return 1;
	} else if (!strcmp(data, "FactionTitle")) {
		lua_pushstring(l, p->GetFactionTitleName().c_str());
		return 1;
	} else if (!strcmp(data, "CharacterTitle")) {
		LuaCheckArgs(l, 4);
		std::string title_type_ident = LuaToString(l, 3);
		std::string gender_ident = LuaToString(l, 4);
		int title_type_id = GetCharacterTitleIdByName(title_type_ident);
		int gender_id = GetGenderIdByName(gender_ident);
		
		lua_pushstring(l, p->GetCharacterTitleName(title_type_id, gender_id).c_str());
		return 1;
	} else if (!strcmp(data, "HasSettlement")) {
		LuaCheckArgs(l, 3);
		std::string site_ident = LuaToString(l, 3);
		const stratagus::site *site = stratagus::site::get(site_ident);
		lua_pushboolean(l, p->HasSettlement(site));
		return 1;
	} else if (!strcmp(data, "SettlementName")) {
		LuaCheckArgs(l, 3);
		std::string site_ident = LuaToString(l, 3);
		const stratagus::site *site = stratagus::site::get(site_ident);
		lua_pushstring(l, site->GetCulturalName(p->Race != -1 ? stratagus::civilization::get_all()[p->Race] : nullptr).c_str());
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "Currency")) {
		const CCurrency *currency = p->GetCurrency();
		if (currency) {
			lua_pushstring(l, currency->Name.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Set player data.
**
**  @param l  Lua state.
*/
static int CclSetPlayerData(lua_State *l)
{
	if (lua_gettop(l) < 3) {
		LuaError(l, "incorrect argument");
	}
	lua_pushvalue(l, 1);
	CPlayer *p = CclGetPlayer(l);
	lua_pop(l, 1);
	const char *data = LuaToString(l, 2);
	
	//Wyrmgus start
	//if player is unused, return
	if (p->Type == PlayerNobody && Editor.Running == EditorNotRunning) {
		return 0;
	}
	//Wyrmgus end

	if (!strcmp(data, "Name")) {
		p->SetName(LuaToString(l, 3));
	} else if (!strcmp(data, "RaceName")) {
		if (GameRunning) {
			p->SetFaction(nullptr);
		}

		const char *civilization_ident = LuaToString(l, 3);
		stratagus::civilization *civilization = stratagus::civilization::get(civilization_ident);
		p->set_civilization(civilization->ID);
	//Wyrmgus start
	} else if (!strcmp(data, "Faction")) {
		std::string faction_name = LuaToString(l, 3);
		if (faction_name == "random") {
			p->SetRandomFaction();
		} else {
			p->SetFaction(stratagus::faction::try_get(faction_name));
		}
	} else if (!strcmp(data, "Dynasty")) {
		std::string dynasty_ident = LuaToString(l, 3);
		p->SetDynasty(PlayerRaces.GetDynasty(dynasty_ident));
	//Wyrmgus end
	} else if (!strcmp(data, "Resources")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		p->SetResource(resId, LuaToNumber(l, 4));
	} else if (!strcmp(data, "StoredResources")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		p->SetResource(resId, LuaToNumber(l, 4), STORE_BUILDING);
		// } else if (!strcmp(data, "UnitTypesCount")) {
		// } else if (!strcmp(data, "AiEnabled")) {
		// } else if (!strcmp(data, "TotalNumUnits")) {
		// } else if (!strcmp(data, "NumBuildings")) {
		// } else if (!strcmp(data, "Supply")) {
		// } else if (!strcmp(data, "Demand")) {
	} else if (!strcmp(data, "UnitLimit")) {
		p->UnitLimit = LuaToNumber(l, 3);
	} else if (!strcmp(data, "BuildingLimit")) {
		p->BuildingLimit = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalUnitLimit")) {
		p->TotalUnitLimit = LuaToNumber(l, 3);
	} else if (!strcmp(data, "Score")) {
		p->Score = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalUnits")) {
		p->TotalUnits = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalBuildings")) {
		p->TotalBuildings = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalResources")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		p->TotalResources[resId] = LuaToNumber(l, 4);
	} else if (!strcmp(data, "TotalRazings")) {
		p->TotalRazings = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalKills")) {
		p->TotalKills = LuaToNumber(l, 3);
	} else if (!strcmp(data, "SpeedResourcesHarvest")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		p->SpeedResourcesHarvest[resId] = LuaToNumber(l, 4);
	} else if (!strcmp(data, "SpeedResourcesReturn")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		p->SpeedResourcesReturn[resId] = LuaToNumber(l, 4);
	} else if (!strcmp(data, "SpeedBuild")) {
		p->SpeedBuild = LuaToNumber(l, 3);
	} else if (!strcmp(data, "SpeedTrain")) {
		p->SpeedTrain = LuaToNumber(l, 3);
	} else if (!strcmp(data, "SpeedUpgrade")) {
		p->SpeedUpgrade = LuaToNumber(l, 3);
	} else if (!strcmp(data, "SpeedResearch")) {
		p->SpeedResearch = LuaToNumber(l, 3);
	} else if (!strcmp(data, "Allow")) {
		LuaCheckArgs(l, 4);
		const char *ident = LuaToString(l, 3);
		const std::string acquire = LuaToString(l, 4);

		if (!strncmp(ident, "upgrade", 7)) {
			if (acquire == "R" && UpgradeIdentAllowed(*p, ident) != 'R') {
				UpgradeAcquire(*p, CUpgrade::get(ident));
			} else if (acquire == "F" || acquire == "A") {
				if (UpgradeIdentAllowed(*p, ident) == 'R') {
					UpgradeLost(*p, CUpgrade::get(ident)->ID);
				}
				AllowUpgradeId(*p, UpgradeIdByIdent(ident), acquire[0]);
			}
		//Wyrmgus start
		} else if (!strncmp(ident, "unit-", 5)) {
			const int UnitMax = 65536; /// How many units supported
			int id = UnitTypeIdByIdent(ident);
			if (acquire == "A" || acquire == "R") {
				AllowUnitId(*p, id, UnitMax);
			} else if (acquire == "F") {
				AllowUnitId(*p, id, 0);
			}
		//Wyrmgus end
		} else {
			LuaError(l, " wrong ident %s\n" _C_ ident);
		}
	//Wyrmgus start
	} else if (!strcmp(data, "AiEnabled")) {
		p->AiEnabled = LuaToBoolean(l, 3);
	} else if (!strcmp(data, "Team")) {
		p->Team = LuaToNumber(l, 3);
	} else if (!strcmp(data, "AcceptQuest")) {
		stratagus::quest *quest = stratagus::quest::get(LuaToString(l, 3));
		p->accept_quest(quest);
	} else if (!strcmp(data, "CompleteQuest")) {
		stratagus::quest *quest = stratagus::quest::get(LuaToString(l, 3));
		p->complete_quest(quest);
	} else if (!strcmp(data, "FailQuest")) {
		stratagus::quest *quest = stratagus::quest::get(LuaToString(l, 3));
		p->fail_quest(quest);
	} else if (!strcmp(data, "AddModifier")) {
		LuaCheckArgs(l, 4);
		CUpgrade *modifier_upgrade = CUpgrade::get(LuaToString(l, 3));
		int cycles = LuaToNumber(l, 4);
		p->AddModifier(modifier_upgrade, cycles);
	//Wyrmgus end
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Set ai player algo.
**
**  @param l  Lua state.
*/
static int CclSetAiType(lua_State *l)
{
	CPlayer *p;

	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	lua_pushvalue(l, 1);
	p = CclGetPlayer(l);
	lua_pop(l, 1);

	p->AiName = LuaToString(l, 2);

	return 0;
}

//Wyrmgus start
/**
**  Init ai for player.
**
**  @param l  Lua state.
*/
static int CclInitAi(lua_State *l)
{
	CPlayer *p;

	if (lua_gettop(l) < 1) {
		LuaError(l, "incorrect argument");
	}
	lua_pushvalue(l, 1);
	p = CclGetPlayer(l);
	lua_pop(l, 1);

	AiInit(*p);

	return 0;
}

static int CclGetLanguages(lua_State *l)
{
	bool only_used = false;
	if (lua_gettop(l) >= 1) {
		only_used = LuaToBoolean(l, 1);
	}
	
	std::vector<std::string> languages;
	for (size_t i = 0; i != PlayerRaces.Languages.size(); ++i) {
		if (!only_used || PlayerRaces.Languages[i]->used_by_civilization_or_faction) {
			languages.push_back(PlayerRaces.Languages[i]->Ident);
		}
	}
		
	lua_createtable(l, languages.size(), 0);
	for (size_t i = 1; i <= languages.size(); ++i)
	{
		lua_pushstring(l, languages[i-1].c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

/**
**  Get language data.
**
**  @param l  Lua state.
*/
static int CclGetLanguageData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string language_name = LuaToString(l, 1);
	const CLanguage *language = PlayerRaces.GetLanguage(language_name);
	if (!language) {
		LuaError(l, "Language \"%s\" doesn't exist." _C_ language_name.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, language->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Family")) {
		lua_pushstring(l, language->Family.c_str());
		return 1;
	} else if (!strcmp(data, "Words")) {
		lua_createtable(l, language->LanguageWords.size(), 0);
		for (size_t i = 1; i <= language->LanguageWords.size(); ++i)
		{
			lua_pushstring(l, language->LanguageWords[i-1]->Word.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Get language word data.
**
**  @param l  Lua state.
*/
static int CclGetLanguageWordData(lua_State *l)
{
	if (lua_gettop(l) < 3) {
		LuaError(l, "incorrect argument");
	}
	std::string language_name = LuaToString(l, 1);
	const CLanguage *language = PlayerRaces.GetLanguage(language_name);
	if (!language) {
		LuaError(l, "Language \"%s\" doesn't exist." _C_ language_name.c_str());
	}
	
	std::string word_name = LuaToString(l, 2);
	std::vector<std::string> word_meanings;
	const LanguageWord *word = language->GetWord(word_name, -1, word_meanings);
	if (word == nullptr) {
		LuaError(l, "Word \"%s\" doesn't exist for the \"%s\" language." _C_ word_name.c_str() _C_ language_name.c_str());
	}
	
	const char *data = LuaToString(l, 3);

	if (!strcmp(data, "Type")) {
		if (word->Type != -1) {
			lua_pushstring(l, GetWordTypeNameById(word->Type).c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Meaning")) {
		for (size_t i = 0; i < word->Meanings.size(); ++i) {
			lua_pushstring(l, word->Meanings[i].c_str());
			return 1;
		}
		lua_pushstring(l, "");
		return 1;
	} else if (!strcmp(data, "Gender")) {
		if (word->Gender != -1) {
			lua_pushstring(l, GetGrammaticalGenderNameById(word->Gender).c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

static int CclGetReligions(lua_State *l)
{
	lua_createtable(l, CReligion::Religions.size(), 0);
	for (size_t i = 1; i <= CReligion::Religions.size(); ++i)
	{
		lua_pushstring(l, CReligion::Religions[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetDeityDomains(lua_State *l)
{
	lua_createtable(l, CDeityDomain::DeityDomains.size(), 0);
	for (size_t i = 1; i <= CDeityDomain::DeityDomains.size(); ++i)
	{
		lua_pushstring(l, CDeityDomain::DeityDomains[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetDeities(lua_State *l)
{
	lua_createtable(l, CDeity::Deities.size(), 0);
	for (size_t i = 1; i <= CDeity::Deities.size(); ++i)
	{
		lua_pushstring(l, CDeity::Deities[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

/**
**  Get religion data.
**
**  @param l  Lua state.
*/
static int CclGetReligionData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string religion_ident = LuaToString(l, 1);
	const CReligion *religion = CReligion::GetReligion(religion_ident);
	if (!religion) {
		return 0;
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, religion->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, religion->Description.c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, religion->Background.c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, religion->Quote.c_str());
		return 1;
	} else if (!strcmp(data, "CulturalDeities")) {
		lua_pushboolean(l, religion->CulturalDeities);
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Get deity domain data.
**
**  @param l  Lua state.
*/
static int CclGetDeityDomainData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string deity_domain_ident = LuaToString(l, 1);
	const CDeityDomain *deity_domain = CDeityDomain::GetDeityDomain(deity_domain_ident);
	if (!deity_domain) {
		return 0;
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, deity_domain->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Abilities")) {
		lua_createtable(l, deity_domain->Abilities.size(), 0);
		for (size_t i = 1; i <= deity_domain->Abilities.size(); ++i)
		{
			lua_pushstring(l, deity_domain->Abilities[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Get deity data.
**
**  @param l  Lua state.
*/
static int CclGetDeityData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string deity_ident = LuaToString(l, 1);
	const CDeity *deity = CDeity::GetDeity(deity_ident);
	if (!deity) {
		return 0;
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, deity->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Pantheon")) {
		if (deity->Pantheon) {
			lua_pushstring(l, deity->Pantheon->Name.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, deity->Description.c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, deity->Background.c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, deity->Quote.c_str());
		return 1;
	} else if (!strcmp(data, "Major")) {
		lua_pushboolean(l, deity->Major);
		return 1;
	} else if (!strcmp(data, "HomePlane")) {
		if (deity->get_home_plane()) {
			lua_pushstring(l, deity->get_home_plane()->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Icon")) {
		lua_pushstring(l, deity->Icon.Name.c_str());
		return 1;
	} else if (!strcmp(data, "Civilizations")) {
		lua_createtable(l, deity->civilizations.size(), 0);
		for (size_t i = 1; i <= deity->civilizations.size(); ++i)
		{
			lua_pushstring(l, deity->civilizations[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Religions")) {
		lua_createtable(l, deity->Religions.size(), 0);
		for (size_t i = 1; i <= deity->Religions.size(); ++i)
		{
			lua_pushstring(l, deity->Religions[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Domains")) {
		lua_createtable(l, deity->Domains.size(), 0);
		for (size_t i = 1; i <= deity->Domains.size(); ++i)
		{
			lua_pushstring(l, deity->Domains[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Abilities")) {
		lua_createtable(l, deity->Abilities.size(), 0);
		for (size_t i = 1; i <= deity->Abilities.size(); ++i)
		{
			lua_pushstring(l, deity->Abilities[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "CulturalName")) {
		if (lua_gettop(l) < 3) {
			LuaError(l, "incorrect argument");
		}
		
		const stratagus::civilization *civilization = stratagus::civilization::get(LuaToString(l, 3));
		lua_pushstring(l, deity->GetCulturalName(civilization).c_str());
		
		return 1;
	} else if (!strcmp(data, "Gender")) {
		lua_pushstring(l, GetGenderNameById(deity->Gender).c_str());
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}
//Wyrmgus end

// ----------------------------------------------------------------------------

/**
**  Register CCL features for players.
*/
void PlayerCclRegister()
{
	lua_register(Lua, "Player", CclPlayer);
	lua_register(Lua, "ChangeUnitsOwner", CclChangeUnitsOwner);
	lua_register(Lua, "GetThisPlayer", CclGetThisPlayer);
	lua_register(Lua, "SetThisPlayer", CclSetThisPlayer);

	lua_register(Lua, "SetMaxSelectable", CclSetMaxSelectable);

	lua_register(Lua, "SetAllPlayersUnitLimit", CclSetAllPlayersUnitLimit);
	lua_register(Lua, "SetAllPlayersBuildingLimit", CclSetAllPlayersBuildingLimit);
	lua_register(Lua, "SetAllPlayersTotalUnitLimit", CclSetAllPlayersTotalUnitLimit);

	lua_register(Lua, "SetDiplomacy", CclSetDiplomacy);
	lua_register(Lua, "Diplomacy", CclDiplomacy);
	lua_register(Lua, "SetSharedVision", CclSetSharedVision);
	lua_register(Lua, "SharedVision", CclSharedVision);

	//Wyrmgus start
	lua_register(Lua, "DefineCivilization", CclDefineCivilization);
	lua_register(Lua, "DefineLanguageWord", CclDefineLanguageWord);
	lua_register(Lua, "GetCivilizationData", CclGetCivilizationData);
	lua_register(Lua, "GetCivilizationClassUnitType", CclGetCivilizationClassUnitType);
	lua_register(Lua, "GetFactionClassUnitType", CclGetFactionClassUnitType);
	lua_register(Lua, "DefineFaction", CclDefineFaction);
	lua_register(Lua, "DefineDynasty", CclDefineDynasty);
	lua_register(Lua, "DefineReligion", CclDefineReligion);
	lua_register(Lua, "DefineDeity", CclDefineDeity);
	lua_register(Lua, "DefineLanguage", CclDefineLanguage);
	lua_register(Lua, "GetCivilizations", CclGetCivilizations);
	lua_register(Lua, "GetFactions", CclGetFactions);
	lua_register(Lua, "GetPlayerColors", CclGetPlayerColors);
	lua_register(Lua, "GetFactionData", CclGetFactionData);
	lua_register(Lua, "GetDynastyData", CclGetDynastyData);
	//Wyrmgus end

	// player member access functions
	lua_register(Lua, "GetPlayerData", CclGetPlayerData);
	lua_register(Lua, "SetPlayerData", CclSetPlayerData);
	lua_register(Lua, "SetAiType", CclSetAiType);
	//Wyrmgus start
	lua_register(Lua, "InitAi", CclInitAi);
	lua_register(Lua, "GetLanguages", CclGetLanguages);
	lua_register(Lua, "GetLanguageData", CclGetLanguageData);
	lua_register(Lua, "GetLanguageWordData", CclGetLanguageWordData);
	
	lua_register(Lua, "GetReligions", CclGetReligions);
	lua_register(Lua, "GetDeityDomains", CclGetDeityDomains);
	lua_register(Lua, "GetDeities", CclGetDeities);
	lua_register(Lua, "GetReligionData", CclGetReligionData);
	lua_register(Lua, "GetDeityDomainData", CclGetDeityDomainData);
	lua_register(Lua, "GetDeityData", CclGetDeityData);
	//Wyrmgus end
}
