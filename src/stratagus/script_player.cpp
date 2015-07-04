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
//      (c) Copyright 2001-2007 by Lutz Sammer and Jimmy Salmon
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

#include "player.h"

#include "actions.h"
#include "ai.h"
#include "commands.h"
#include "map.h"
#include "script.h"
#include "unittype.h"
#include "unit.h"
#include "unit_find.h"
#include "video.h"
#include "upgrade.h"
//Wyrmgus start
#include "font.h"
#include "ui.h"
#include "util.h"
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

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
	return &Players[LuaToNumber(l, -1)];
}

/**
**  Parse the player configuration.
**
**  @param l  Lua state.
*/
static int CclPlayer(lua_State *l)
{
	int i = LuaToNumber(l, 1);

	CPlayer &player = Players[i];
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
			const char *raceName = LuaToString(l, j + 1);
			this->Race = PlayerRaces.GetRaceIndexByName(raceName);
			if (this->Race == -1) {
				LuaError(l, "Unsupported race: %s" _C_ raceName);
			}
		//Wyrmgus start
		} else if (!strcmp(value, "faction")) {
			this->SetFaction(PlayerRaces.FactionNames[this->Race][LuaToNumber(l, j + 1)]);
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
		} else if (!strcmp(value, "ai-enabled")) {
			this->AiEnabled = true;
			--j;
		} else if (!strcmp(value, "ai-disabled")) {
			this->AiEnabled = false;
			--j;
		} else if (!strcmp(value, "supply")) {
			this->Supply = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "demand")) {
			this->Demand = LuaToNumber(l, j + 1);
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
		} else if (!strcmp(value, "total-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			if (subargs != MaxCosts) {
				LuaError(l, "Wrong number of total-resources: %d" _C_ subargs);
			}
			for (int k = 0; k < subargs; ++k) {
				this->TotalResources[k] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "speed-resource-harvest")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			if (subargs != MaxCosts) {
				LuaError(l, "Wrong number of speed-resource-harvest: %d" _C_ subargs);
			}
			for (int k = 0; k < subargs; ++k) {
				this->SpeedResourcesHarvest[k] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "speed-resource-return")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			if (subargs != MaxCosts) {
				LuaError(l, "Wrong number of speed-resource-harvest: %d" _C_ subargs);
			}
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
		} else if (!strcmp(value, "color")) {
			if (!lua_istable(l, j + 1) || lua_rawlen(l, j + 1) != 3) {
				LuaError(l, "incorrect argument");
			}
			const int r = LuaToNumber(l, j + 1, 1);
			const int g = LuaToNumber(l, j + 1, 2);
			const int b = LuaToNumber(l, j + 1, 3);
			this->Color = Video.MapRGB(TheScreen->format, r, g, b);
		} else if (!strcmp(value, "timers")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			if (subargs != UpgradeMax) {
				LuaError(l, "Wrong upgrade timer length: %d" _C_ subargs);
			}
			for (int k = 0; k < subargs; ++k) {
				this->UpgradeTimers.Upgrades[k] = LuaToNumber(l, j + 1, k + 1);
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

	Select(pos1, pos2, table, HasSamePlayerAs(Players[oldp]));
	for (size_t i = 0; i != table.size(); ++i) {
		table[i]->ChangeOwner(Players[newp]);
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
	if (ThisPlayer) {
		lua_pushnumber(l, ThisPlayer - Players);
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
	ThisPlayer = &Players[plynr];

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
		Players[i].UnitLimit = LuaToNumber(l, 1);
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
		Players[i].BuildingLimit = LuaToNumber(l, 1);
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
		Players[i].TotalUnitLimit = LuaToNumber(l, 1);
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
		SendCommandDiplomacy(base, DiplomacyAllied, plynr);
	} else if (!strcmp(state, "neutral")) {
		SendCommandDiplomacy(base, DiplomacyNeutral, plynr);
	} else if (!strcmp(state, "crazy")) {
		SendCommandDiplomacy(base, DiplomacyCrazy, plynr);
	} else if (!strcmp(state, "enemy")) {
		SendCommandDiplomacy(base, DiplomacyEnemy, plynr);
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
	lua_pushnumber(l, ThisPlayer->Index);
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
	lua_pushnumber(l, ThisPlayer->Index);
	lua_insert(l, 1);
	return CclSetSharedVision(l);
}

/**
**  Define race names
**
**  @param l  Lua state.
*/
static int CclDefineRaceNames(lua_State *l)
{
	PlayerRaces.Clean();
	int args = lua_gettop(l);
	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		if (!strcmp(value, "race")) {
			++j;
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			int subargs = lua_rawlen(l, j + 1);
			int i = PlayerRaces.Count++;
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				if (!strcmp(value, "name")) {
					++k;
					PlayerRaces.Name[i] = LuaToString(l, j + 1, k + 1);
					//Wyrmgus start
					PlayerRaces.Playable[i] = true; //civilizations are playable by default
					//Wyrmgus end
				} else if (!strcmp(value, "display")) {
					++k;
					PlayerRaces.Display[i] = LuaToString(l, j + 1, k + 1);
				} else if (!strcmp(value, "visible")) {
					PlayerRaces.Visible[i] = 1;
				//Wyrmgus start
				} else if (!strcmp(value, "playable")) {
					++k;
					PlayerRaces.Playable[i] = LuaToBoolean(l, j + 1, k + 1);
				} else if (!strcmp(value, "species")) {
					++k;
					PlayerRaces.Species[i] = LuaToString(l, j + 1, k + 1);
				} else if (!strcmp(value, "parent-civilization")) {
					++k;
					PlayerRaces.ParentCivilization[i] = LuaToString(l, j + 1, k + 1);
				} else if (!strcmp(value, "personal-names")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.PersonalNames[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "personal-name-prefixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.PersonalNamePrefixes[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "personal-name-suffixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.PersonalNameSuffixes[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "province-names")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.ProvinceNames[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "province-name-prefixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.ProvinceNamePrefixes[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "province-name-suffixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.ProvinceNameSuffixes[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "settlement-names")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.SettlementNames[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "settlement-name-prefixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.SettlementNamePrefixes[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "settlement-name-suffixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.SettlementNameSuffixes[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "province-name-translations")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.ProvinceNameTranslations[i][n / 2][0] = LuaToString(l, -1, n + 1); //name to be translated
						++n;
						PlayerRaces.ProvinceNameTranslations[i][(n - 1) / 2][1] = LuaToString(l, -1, n + 1); //name translation
					}
				} else if (!strcmp(value, "province-name-prefix-translations")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.ProvinceNamePrefixTranslations[i][n / 2][0] = LuaToString(l, -1, n + 1); //prefix to be translated
						++n;
						PlayerRaces.ProvinceNamePrefixTranslations[i][(n - 1) / 2][1] = LuaToString(l, -1, n + 1); //prefix translation
					}
				} else if (!strcmp(value, "province-name-suffix-translations")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.ProvinceNameSuffixTranslations[i][n / 2][0] = LuaToString(l, -1, n + 1); //suffix to be translated
						++n;
						PlayerRaces.ProvinceNameSuffixTranslations[i][(n - 1) / 2][1] = LuaToString(l, -1, n + 1); //suffix translation
					}
				} else if (!strcmp(value, "settlement-name-translations")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.SettlementNameTranslations[i][n / 2][0] = LuaToString(l, -1, n + 1); //name to be translated
						++n;
						PlayerRaces.SettlementNameTranslations[i][(n - 1) / 2][1] = LuaToString(l, -1, n + 1); //name translation
					}
				} else if (!strcmp(value, "settlement-name-prefix-translations")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.SettlementNamePrefixTranslations[i][n / 2][0] = LuaToString(l, -1, n + 1); //prefix to be translated
						++n;
						PlayerRaces.SettlementNamePrefixTranslations[i][(n - 1) / 2][1] = LuaToString(l, -1, n + 1); //prefix translation
					}
				} else if (!strcmp(value, "settlement-name-suffix-translations")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.SettlementNameSuffixTranslations[i][n / 2][0] = LuaToString(l, -1, n + 1); //suffix to be translated
						++n;
						PlayerRaces.SettlementNameSuffixTranslations[i][(n - 1) / 2][1] = LuaToString(l, -1, n + 1); //suffix translation
					}
				//Wyrmgus end
				} else {
					LuaError(l, "Unsupported tag: %s" _C_ value);
				}
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}

	return 0;
}

//Wyrmgus start
/**
**  Define new race names
**
**  @param l  Lua state.
*/
static int CclDefineNewRaceNames(lua_State *l)
{
	int args = lua_gettop(l);
	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		if (!strcmp(value, "race")) {
			++j;
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			int subargs = lua_rawlen(l, j + 1);
			int i = PlayerRaces.Count++;
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				if (!strcmp(value, "name")) {
					++k;
					PlayerRaces.Name[i] = LuaToString(l, j + 1, k + 1);
					PlayerRaces.Playable[i] = true; //civilizations are playable by default
				} else if (!strcmp(value, "display")) {
					++k;
					PlayerRaces.Display[i] = LuaToString(l, j + 1, k + 1);
				} else if (!strcmp(value, "visible")) {
					PlayerRaces.Visible[i] = 1;
				} else if (!strcmp(value, "playable")) {
					++k;
					PlayerRaces.Playable[i] = LuaToBoolean(l, j + 1, k + 1);
				} else if (!strcmp(value, "species")) {
					++k;
					PlayerRaces.Species[i] = LuaToString(l, j + 1, k + 1);
				} else if (!strcmp(value, "parent-civilization")) {
					++k;
					PlayerRaces.ParentCivilization[i] = LuaToString(l, j + 1, k + 1);
				} else if (!strcmp(value, "personal-names")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.PersonalNames[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "personal-name-prefixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.PersonalNamePrefixes[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "personal-name-suffixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.PersonalNameSuffixes[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "province-names")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.ProvinceNames[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "province-name-prefixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.ProvinceNamePrefixes[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "province-name-suffixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.ProvinceNameSuffixes[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "settlement-names")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.SettlementNames[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "settlement-name-prefixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.SettlementNamePrefixes[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "settlement-name-suffixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.SettlementNameSuffixes[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "province-name-translations")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.ProvinceNameTranslations[i][n / 2][0] = LuaToString(l, -1, n + 1); //name to be translated
						++n;
						PlayerRaces.ProvinceNameTranslations[i][(n - 1) / 2][1] = LuaToString(l, -1, n + 1); //name translation
					}
				} else if (!strcmp(value, "province-name-prefix-translations")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.ProvinceNamePrefixTranslations[i][n / 2][0] = LuaToString(l, -1, n + 1); //prefix to be translated
						++n;
						PlayerRaces.ProvinceNamePrefixTranslations[i][(n - 1) / 2][1] = LuaToString(l, -1, n + 1); //prefix translation
					}
				} else if (!strcmp(value, "province-name-suffix-translations")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.ProvinceNameSuffixTranslations[i][n / 2][0] = LuaToString(l, -1, n + 1); //suffix to be translated
						++n;
						PlayerRaces.ProvinceNameSuffixTranslations[i][(n - 1) / 2][1] = LuaToString(l, -1, n + 1); //suffix translation
					}
				} else if (!strcmp(value, "settlement-name-translations")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.SettlementNameTranslations[i][n / 2][0] = LuaToString(l, -1, n + 1); //name to be translated
						++n;
						PlayerRaces.SettlementNameTranslations[i][(n - 1) / 2][1] = LuaToString(l, -1, n + 1); //name translation
					}
				} else if (!strcmp(value, "settlement-name-prefix-translations")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.SettlementNamePrefixTranslations[i][n / 2][0] = LuaToString(l, -1, n + 1); //prefix to be translated
						++n;
						PlayerRaces.SettlementNamePrefixTranslations[i][(n - 1) / 2][1] = LuaToString(l, -1, n + 1); //prefix translation
					}
				} else if (!strcmp(value, "settlement-name-suffix-translations")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.SettlementNameSuffixTranslations[i][n / 2][0] = LuaToString(l, -1, n + 1); //suffix to be translated
						++n;
						PlayerRaces.SettlementNameSuffixTranslations[i][(n - 1) / 2][1] = LuaToString(l, -1, n + 1); //suffix translation
					}
				} else {
					LuaError(l, "Unsupported tag: %s" _C_ value);
				}
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}

	return 0;
}


/**
**  Define a civilization's language
**
**  @param l  Lua state.
*/
static int CclDefineCivilizationLanguage(lua_State *l)
{
	int args = lua_gettop(l);
	int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 1));
	
	if (civilization == -1) { //if the civilization is invalid, don't define the language
		return 0;
	}
	
	for (int j = 1; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		if (!strcmp(value, "nouns")) {
			++j;
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				LanguageNoun *noun = new LanguageNoun;
				noun->Word = LuaToString(l, j + 1, k + 1);
				PlayerRaces.LanguageNouns[civilization][k / 2] = noun;
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table)");
				}
				int subsubargs = lua_rawlen(l, -1);
				for (int n = 0; n < subsubargs; ++n) {
					const char *value = LuaToString(l, -1, n + 1);
					if (!strcmp(value, "meaning")) {
						++n;
						noun->Meaning = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "verb")) {
						++n;
						noun->Verb = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "adjective")) {
						++n;
						noun->Adjective = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "singular-nominative")) {
						++n;
						noun->SingularNominative = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "singular-accusative")) {
						++n;
						noun->SingularAccusative = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "singular-dative")) {
						++n;
						noun->SingularDative = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "singular-genitive")) {
						++n;
						noun->SingularGenitive = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "plural-nominative")) {
						++n;
						noun->PluralNominative = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "plural-accusative")) {
						++n;
						noun->PluralAccusative = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "plural-dative")) {
						++n;
						noun->PluralDative = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "plural-genitive")) {
						++n;
						noun->PluralGenitive = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "gender")) {
						++n;
						noun->Gender = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "uncountable")) {
						++n;
						noun->Uncountable = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "prefix-singular")) {
						++n;
						noun->PrefixSingular = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "prefix-plural")) {
						++n;
						noun->PrefixPlural = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "prefix-personal-name")) {
						++n;
						noun->PrefixPersonalName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "prefix-settlement-name")) {
						++n;
						noun->PrefixSettlementName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "prefix-province-name")) {
						++n;
						noun->PrefixProvinceName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "suffix-singular")) {
						++n;
						noun->SuffixSingular = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "suffix-plural")) {
						++n;
						noun->SuffixPlural = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "suffix-personal-name")) {
						++n;
						noun->SuffixPersonalName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "suffix-settlement-name")) {
						++n;
						noun->SuffixSettlementName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "suffix-province-name")) {
						++n;
						noun->SuffixProvinceName = LuaToBoolean(l, -1, n + 1);
					} else {
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
			}
		} else if (!strcmp(value, "verbs")) {
			++j;
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				LanguageVerb *verb = new LanguageVerb;
				verb->Word = LuaToString(l, j + 1, k + 1);
				PlayerRaces.LanguageVerbs[civilization][k / 2] = verb;
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table)");
				}
				int subsubargs = lua_rawlen(l, -1);
				for (int n = 0; n < subsubargs; ++n) {
					const char *value = LuaToString(l, -1, n + 1);
					if (!strcmp(value, "meaning")) {
						++n;
						verb->Meaning = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "noun")) {
						++n;
						verb->Noun = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "adjective")) {
						++n;
						verb->Adjective = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "infinitive")) {
						++n;
						verb->Infinitive = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "singular-first-person-present")) {
						++n;
						verb->SingularFirstPersonPresent = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "singular-second-person-present")) {
						++n;
						verb->SingularSecondPersonPresent = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "singular-third-person-present")) {
						++n;
						verb->SingularThirdPersonPresent = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "plural-first-person-present")) {
						++n;
						verb->PluralFirstPersonPresent = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "plural-second-person-present")) {
						++n;
						verb->PluralSecondPersonPresent = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "plural-third-person-present")) {
						++n;
						verb->PluralThirdPersonPresent = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "singular-first-person-past")) {
						++n;
						verb->SingularFirstPersonPast = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "singular-second-person-past")) {
						++n;
						verb->SingularSecondPersonPast = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "singular-third-person-past")) {
						++n;
						verb->SingularThirdPersonPast = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "plural-first-person-past")) {
						++n;
						verb->PluralFirstPersonPast = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "plural-second-person-past")) {
						++n;
						verb->PluralSecondPersonPast = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "plural-third-person-past")) {
						++n;
						verb->PluralThirdPersonPast = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "singular-first-person-future")) {
						++n;
						verb->SingularFirstPersonFuture = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "singular-second-person-future")) {
						++n;
						verb->SingularSecondPersonFuture = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "singular-third-person-future")) {
						++n;
						verb->SingularThirdPersonFuture = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "plural-first-person-future")) {
						++n;
						verb->PluralFirstPersonFuture = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "plural-second-person-future")) {
						++n;
						verb->PluralSecondPersonFuture = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "plural-third-person-future")) {
						++n;
						verb->PluralThirdPersonFuture = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "participle-present")) {
						++n;
						verb->ParticiplePresent = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "participle-past")) {
						++n;
						verb->ParticiplePast = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "prefix-personal-name")) {
						++n;
						verb->PrefixPersonalName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "prefix-settlement-name")) {
						++n;
						verb->PrefixSettlementName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "prefix-province-name")) {
						++n;
						verb->PrefixProvinceName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "suffix-personal-name")) {
						++n;
						verb->SuffixPersonalName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "suffix-settlement-name")) {
						++n;
						verb->SuffixSettlementName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "suffix-province-name")) {
						++n;
						verb->SuffixProvinceName = LuaToBoolean(l, -1, n + 1);
					} else {
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
			}
		} else if (!strcmp(value, "adjectives")) {
			++j;
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				LanguageAdjective *adjective = new LanguageAdjective;
				adjective->Word = LuaToString(l, j + 1, k + 1);
				PlayerRaces.LanguageAdjectives[civilization][k / 2] = adjective;
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table)");
				}
				int subsubargs = lua_rawlen(l, -1);
				for (int n = 0; n < subsubargs; ++n) {
					const char *value = LuaToString(l, -1, n + 1);
					if (!strcmp(value, "meaning")) {
						++n;
						adjective->Meaning = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "noun")) {
						++n;
						adjective->Noun = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "verb")) {
						++n;
						adjective->Verb = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "comparative")) {
						++n;
						adjective->Comparative = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "prefix-personal-name")) {
						++n;
						adjective->PrefixPersonalName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "prefix-settlement-name")) {
						++n;
						adjective->PrefixSettlementName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "prefix-province-name")) {
						++n;
						adjective->PrefixProvinceName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "suffix-personal-name")) {
						++n;
						adjective->SuffixPersonalName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "suffix-settlement-name")) {
						++n;
						adjective->SuffixSettlementName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "suffix-province-name")) {
						++n;
						adjective->SuffixProvinceName = LuaToBoolean(l, -1, n + 1);
					} else {
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
			}
		} else if (!strcmp(value, "pronouns")) {
			++j;
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				LanguagePronoun *pronoun = new LanguagePronoun;
				pronoun->Word = LuaToString(l, j + 1, k + 1);
				PlayerRaces.LanguagePronouns[civilization][k / 2] = pronoun;
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table)");
				}
				int subsubargs = lua_rawlen(l, -1);
				for (int n = 0; n < subsubargs; ++n) {
					const char *value = LuaToString(l, -1, n + 1);
					if (!strcmp(value, "meaning")) {
						++n;
						pronoun->Meaning = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "nominative")) {
						++n;
						pronoun->Nominative = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "accusative")) {
						++n;
						pronoun->Accusative = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "dative")) {
						++n;
						pronoun->Dative = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "genitive")) {
						++n;
						pronoun->Genitive = LuaToString(l, -1, n + 1);
					} else {
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
			}
		} else if (!strcmp(value, "adverbs")) {
			++j;
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				LanguageAdverb *adverb = new LanguageAdverb;
				adverb->Word = LuaToString(l, j + 1, k + 1);
				PlayerRaces.LanguageAdverbs[civilization][k / 2] = adverb;
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table)");
				}
				int subsubargs = lua_rawlen(l, -1);
				for (int n = 0; n < subsubargs; ++n) {
					const char *value = LuaToString(l, -1, n + 1);
					if (!strcmp(value, "meaning")) {
						++n;
						adverb->Meaning = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "adjective")) {
						++n;
						adverb->Adjective = LuaToString(l, -1, n + 1);
					} else {
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
			}
		} else if (!strcmp(value, "conjunctions")) {
			++j;
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				LanguageConjunction *conjunction = new LanguageConjunction;
				conjunction->Word = LuaToString(l, j + 1, k + 1);
				PlayerRaces.LanguageConjunctions[civilization][k / 2] = conjunction;
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table)");
				}
				int subsubargs = lua_rawlen(l, -1);
				for (int n = 0; n < subsubargs; ++n) {
					const char *value = LuaToString(l, -1, n + 1);
					if (!strcmp(value, "meaning")) {
						++n;
						conjunction->Meaning = LuaToString(l, -1, n + 1);
					} else {
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
			}
		} else if (!strcmp(value, "numerals")) {
			++j;
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				LanguageNumeral *numeral = new LanguageNumeral;
				numeral->Word = LuaToString(l, j + 1, k + 1);
				PlayerRaces.LanguageNumerals[civilization][k / 2] = numeral;
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table)");
				}
				int subsubargs = lua_rawlen(l, -1);
				for (int n = 0; n < subsubargs; ++n) {
					const char *value = LuaToString(l, -1, n + 1);
					if (!strcmp(value, "number")) {
						++n;
						numeral->Number = LuaToNumber(l, -1, n + 1);
					} else if (!strcmp(value, "prefix-personal-name")) {
						++n;
						numeral->PrefixPersonalName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "prefix-settlement-name")) {
						++n;
						numeral->PrefixSettlementName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "prefix-province-name")) {
						++n;
						numeral->PrefixProvinceName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "suffix-personal-name")) {
						++n;
						numeral->SuffixPersonalName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "suffix-settlement-name")) {
						++n;
						numeral->SuffixSettlementName = LuaToBoolean(l, -1, n + 1);
					} else if (!strcmp(value, "suffix-province-name")) {
						++n;
						numeral->SuffixProvinceName = LuaToBoolean(l, -1, n + 1);
					} else {
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}

	return 0;
}

/**
**  Get whether a civilization is playable or not.
**
**  @param l  Lua state.
*/
static int CclIsCivilizationPlayable(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 1));
	lua_pop(l, 1);

	lua_pushboolean(l, PlayerRaces.Playable[civilization]);
	return 1;
}

/**
**  Get a civilization's species.
**
**  @param l  Lua state.
*/
static int CclGetCivilizationSpecies(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 1));
	lua_pop(l, 1);

	lua_pushstring(l, PlayerRaces.Species[civilization].c_str());
	return 1;
}

/**
**  Get a civilization's parent civilization.
**
**  @param l  Lua state.
*/
static int CclGetParentCivilization(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 1));
	lua_pop(l, 1);

	lua_pushstring(l, PlayerRaces.ParentCivilization[civilization].c_str());
	return 1;
}

/**
**  Generate a province name for the civilization.
**
**  @param l  Lua state.
*/
static int CclGenerateProvinceName(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 1));
	lua_pop(l, 1);

	std::string ProvinceName;
	
	if (
		!PlayerRaces.ProvinceNames[civilization][0].empty()
		|| !PlayerRaces.ProvinceNamePrefixes[civilization][0].empty()
		|| PlayerRaces.LanguageNouns[civilization][0]
		|| PlayerRaces.LanguageVerbs[civilization][0]
		|| PlayerRaces.LanguageAdjectives[civilization][0]
	) {
		int ProvinceNameCount = 0;
		std::string ProvinceNames[PersonalNameMax];
		int ProvinceNamePrefixCount = 0;
		std::string ProvinceNamePrefixes[PersonalNameMax];
		int ProvinceNameSuffixCount = 0;
		std::string ProvinceNameSuffixes[PersonalNameMax];
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.ProvinceNames[civilization][i].empty()) {
				break;
			}
			ProvinceNames[ProvinceNameCount] = PlayerRaces.ProvinceNames[civilization][i];
			ProvinceNameCount += 1;
		}
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.ProvinceNamePrefixes[civilization][i].empty()) {
				break;
			}
			ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.ProvinceNamePrefixes[civilization][i];
			ProvinceNamePrefixCount += 1;
		}
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.ProvinceNameSuffixes[civilization][i].empty()) {
				break;
			}
			ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.ProvinceNameSuffixes[civilization][i];
			ProvinceNameSuffixCount += 1;
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageNouns[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->PrefixProvinceName) {
				if (PlayerRaces.LanguageNouns[civilization][i]->Uncountable) { // if is uncountable, use the nominative instead of the genitive
					if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixSingular) {
						ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
						ProvinceNamePrefixCount += 1;
					}
					if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixPlural) {
						ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
						ProvinceNamePrefixCount += 1;
					}
				} else {
					if (!PlayerRaces.LanguageNouns[civilization][i]->SingularGenitive.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixSingular) {
						ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularGenitive;
						ProvinceNamePrefixCount += 1;
					}
					if (!PlayerRaces.LanguageNouns[civilization][i]->PluralGenitive.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixPlural) {
						ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralGenitive;
						ProvinceNamePrefixCount += 1;
					}
				}
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->SuffixProvinceName) {
				if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->SuffixSingular) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
					ProvinceNameSuffixCount += 1;
				}
				if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->SuffixPlural) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
					ProvinceNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageVerbs[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageVerbs[civilization][i]->PrefixProvinceName) { // only using verb participles for now; maybe should add more possibilities?
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent.empty()) {
					ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent;
					ProvinceNamePrefixCount += 1;
				}
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast.empty()) {
					ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast;
					ProvinceNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageVerbs[civilization][i]->SuffixProvinceName) {
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent.empty()) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent;
					ProvinceNameSuffixCount += 1;
				}
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast.empty()) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast;
					ProvinceNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageAdjectives[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->PrefixProvinceName) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					ProvinceNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->SuffixProvinceName) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					ProvinceNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageNumerals[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageNumerals[civilization][i]->PrefixProvinceName) {
				if (!PlayerRaces.LanguageNumerals[civilization][i]->Word.empty()) {
					ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageNumerals[civilization][i]->Word;
					ProvinceNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageNumerals[civilization][i]->SuffixProvinceName) {
				if (!PlayerRaces.LanguageNumerals[civilization][i]->Word.empty()) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageNumerals[civilization][i]->Word;
					ProvinceNameSuffixCount += 1;
				}
			}
		}
		
		if (ProvinceNameCount > 0 || ProvinceNamePrefixCount > 0 || ProvinceNameSuffixCount > 0) {
			int ProvinceNameProbability = ProvinceNameCount * 10000 / (ProvinceNameCount + (ProvinceNamePrefixCount * ProvinceNameSuffixCount));
			if (SyncRand(10000) < ProvinceNameProbability) {
				ProvinceName = ProvinceNames[SyncRand(ProvinceNameCount)];
			} else {
				std::string prefix = ProvinceNamePrefixes[SyncRand(ProvinceNamePrefixCount)];
				std::string suffix = ProvinceNameSuffixes[SyncRand(ProvinceNameSuffixCount)];
				
				if (PlayerRaces.RequiresPlural(prefix, civilization)) {
					suffix = PlayerRaces.GetPluralForm(suffix, civilization);
				}
				
				suffix[0] = tolower(suffix[0]);
				if (prefix.substr(prefix.size() - 2, 2) == "gs" && suffix.substr(0, 1) == "g") { //if the last two characters of the prefix are "gs", and the first character of the suffix is "g", then remove the final "s" from the prefix (as in "K�niggr�tz")
					prefix = FindAndReplaceStringEnding(prefix, "gs", "g");
				}
				if (prefix.substr(prefix.size() - 1, 1) == "s" && suffix.substr(0, 1) == "s") { //if the prefix ends in "s" and the suffix begins in "s" as well, then remove the final "s" from the prefix (as in "Josefstadt", "Kronstadt" and "Leopoldstadt")
					prefix = FindAndReplaceStringEnding(prefix, "s", "");
				}
				ProvinceName = prefix;
				ProvinceName += suffix;
			}
		}
	}
	
	ProvinceName = TransliterateText(ProvinceName);
	
	lua_pushstring(l, ProvinceName.c_str());
	return 1;
}

/**
**  Generate a settlement name for the civilization.
**
**  @param l  Lua state.
*/
static int CclGenerateSettlementName(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 1));
	lua_pop(l, 1);

	std::string SettlementName;
	
	if (
		!PlayerRaces.SettlementNames[civilization][0].empty()
		|| !PlayerRaces.SettlementNamePrefixes[civilization][0].empty()
		|| PlayerRaces.LanguageNouns[civilization][0]
		|| PlayerRaces.LanguageVerbs[civilization][0]
		|| PlayerRaces.LanguageAdjectives[civilization][0]
	) {
		int SettlementNameCount = 0;
		std::string SettlementNames[PersonalNameMax];
		int SettlementNamePrefixCount = 0;
		std::string SettlementNamePrefixes[PersonalNameMax];
		int SettlementNameSuffixCount = 0;
		std::string SettlementNameSuffixes[PersonalNameMax];
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.SettlementNames[civilization][i].empty()) {
				break;
			}
			SettlementNames[SettlementNameCount] = PlayerRaces.SettlementNames[civilization][i];
			SettlementNameCount += 1;
		}
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.SettlementNamePrefixes[civilization][i].empty()) {
				break;
			}
			SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.SettlementNamePrefixes[civilization][i];
			SettlementNamePrefixCount += 1;
		}
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.SettlementNameSuffixes[civilization][i].empty()) {
				break;
			}
			SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.SettlementNameSuffixes[civilization][i];
			SettlementNameSuffixCount += 1;
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageNouns[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->PrefixSettlementName) {
				if (PlayerRaces.LanguageNouns[civilization][i]->Uncountable) { // if is uncountable, use the nominative instead of the genitive
					if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixSingular) {
						SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
						SettlementNamePrefixCount += 1;
					}
					if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixPlural) {
						SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
						SettlementNamePrefixCount += 1;
					}
				} else {
					if (!PlayerRaces.LanguageNouns[civilization][i]->SingularGenitive.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixSingular) {
						SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularGenitive;
						SettlementNamePrefixCount += 1;
					}
					if (!PlayerRaces.LanguageNouns[civilization][i]->PluralGenitive.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixPlural) {
						SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralGenitive;
						SettlementNamePrefixCount += 1;
					}
				}
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->SuffixSettlementName) {
				if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->SuffixSingular) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
					SettlementNameSuffixCount += 1;
				}
				if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->SuffixPlural) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
					SettlementNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageVerbs[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageVerbs[civilization][i]->PrefixSettlementName) { // only using verb participles for now; maybe should add more possibilities?
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent.empty()) {
					SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent;
					SettlementNamePrefixCount += 1;
				}
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast.empty()) {
					SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast;
					SettlementNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageVerbs[civilization][i]->SuffixSettlementName) {
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent.empty()) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent;
					SettlementNameSuffixCount += 1;
				}
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast.empty()) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast;
					SettlementNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageAdjectives[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->PrefixSettlementName) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					SettlementNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->SuffixSettlementName) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					SettlementNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageNumerals[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageNumerals[civilization][i]->PrefixSettlementName) {
				if (!PlayerRaces.LanguageNumerals[civilization][i]->Word.empty()) {
					SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageNumerals[civilization][i]->Word;
					SettlementNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageNumerals[civilization][i]->SuffixSettlementName) {
				if (!PlayerRaces.LanguageNumerals[civilization][i]->Word.empty()) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageNumerals[civilization][i]->Word;
					SettlementNameSuffixCount += 1;
				}
			}
		}
		
		if (SettlementNameCount > 0 || SettlementNamePrefixCount > 0 || SettlementNameSuffixCount > 0) {
			int SettlementNameProbability = SettlementNameCount * 10000 / (SettlementNameCount + (SettlementNamePrefixCount * SettlementNameSuffixCount));
			if (SyncRand(10000) < SettlementNameProbability) {
				SettlementName = SettlementNames[SyncRand(SettlementNameCount)];
			} else {
				std::string prefix = SettlementNamePrefixes[SyncRand(SettlementNamePrefixCount)];
				std::string suffix = SettlementNameSuffixes[SyncRand(SettlementNameSuffixCount)];
				
				if (PlayerRaces.RequiresPlural(prefix, civilization)) {
					suffix = PlayerRaces.GetPluralForm(suffix, civilization);
				}
				
				suffix[0] = tolower(suffix[0]);
				if (prefix.substr(prefix.size() - 2, 2) == "gs" && suffix.substr(0, 1) == "g") { //if the last two characters of the prefix are "gs", and the first character of the suffix is "g", then remove the final "s" from the prefix (as in "K�niggr�tz")
					prefix = FindAndReplaceStringEnding(prefix, "gs", "g");
				}
				if (prefix.substr(prefix.size() - 1, 1) == "s" && suffix.substr(0, 1) == "s") { //if the prefix ends in "s" and the suffix begins in "s" as well, then remove the final "s" from the prefix (as in "Josefstadt", "Kronstadt" and "Leopoldstadt")
					prefix = FindAndReplaceStringEnding(prefix, "s", "");
				}
				SettlementName = prefix;
				SettlementName += suffix;
			}
		}
	}
	
	SettlementName = TransliterateText(SettlementName);
	
	lua_pushstring(l, SettlementName.c_str());
	return 1;
}

/**
**  "Translate" (that is, adapt) a province name from one culture (civilization) to another.
**
**  @param l  Lua state.
*/
static int CclTranslateProvinceName(lua_State *l)
{
	LuaCheckArgs(l, 2);
	std::string province_name = LuaToString(l, 1);
	int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 2));
	
	std::string new_province_name;

	// try to translate the entire name, as a particular translation for it may exist
	if (!PlayerRaces.ProvinceNameTranslations[civilization][0][0].empty()) {
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (!PlayerRaces.ProvinceNameTranslations[civilization][i][0].empty() && PlayerRaces.ProvinceNameTranslations[civilization][i][0] == province_name) {
				new_province_name = PlayerRaces.ProvinceNameTranslations[civilization][i][1];
				lua_pushstring(l, new_province_name.c_str());
				return 1;
			}
		}
	}
	
	//if adapting the entire name failed, try to match prefixes and suffixes
	if (!PlayerRaces.ProvinceNamePrefixTranslations[civilization][0][0].empty() && !PlayerRaces.ProvinceNameSuffixTranslations[civilization][0][0].empty()) {
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (!PlayerRaces.ProvinceNamePrefixTranslations[civilization][i][0].empty() && PlayerRaces.ProvinceNamePrefixTranslations[civilization][i][0] == province_name.substr(0, PlayerRaces.ProvinceNamePrefixTranslations[civilization][i][0].size())) {
				for (int j = 0; j < PersonalNameMax; ++j) {
					if (!PlayerRaces.ProvinceNameSuffixTranslations[civilization][j][0].empty() && PlayerRaces.ProvinceNameSuffixTranslations[civilization][j][0] == province_name.substr(PlayerRaces.ProvinceNamePrefixTranslations[civilization][i][0].size(), PlayerRaces.ProvinceNameSuffixTranslations[civilization][j][0].size())) {
						new_province_name = PlayerRaces.ProvinceNamePrefixTranslations[civilization][i][1];
						new_province_name += PlayerRaces.ProvinceNameSuffixTranslations[civilization][j][1];
						lua_pushstring(l, new_province_name.c_str());
						return 1;
					}
				}
			}
		}
	}
	
	lua_pushstring(l, new_province_name.c_str());
	return 1;
}

/**
**  "Translate" (that is, adapt) a settlement name from one culture (civilization) to another.
**
**  @param l  Lua state.
*/
static int CclTranslateSettlementName(lua_State *l)
{
	LuaCheckArgs(l, 2);
	std::string settlement_name = LuaToString(l, 1);
	int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 2));
	
	std::string new_settlement_name;

	// try to translate the entire name, as a particular translation for it may exist
	if (!PlayerRaces.SettlementNameTranslations[civilization][0][0].empty()) {
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (!PlayerRaces.SettlementNameTranslations[civilization][i][0].empty() && PlayerRaces.SettlementNameTranslations[civilization][i][0] == settlement_name) {
				new_settlement_name = PlayerRaces.SettlementNameTranslations[civilization][i][1];
				lua_pushstring(l, new_settlement_name.c_str());
				return 1;
			}
		}
	}
	
	//if adapting the entire name failed, try to match prefixes and suffixes
	if (!PlayerRaces.SettlementNamePrefixTranslations[civilization][0][0].empty() && !PlayerRaces.SettlementNameSuffixTranslations[civilization][0][0].empty()) {
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (!PlayerRaces.SettlementNamePrefixTranslations[civilization][i][0].empty() && PlayerRaces.SettlementNamePrefixTranslations[civilization][i][0] == settlement_name.substr(0, PlayerRaces.SettlementNamePrefixTranslations[civilization][i][0].size())) {
				for (int j = 0; j < PersonalNameMax; ++j) {
					if (!PlayerRaces.SettlementNameSuffixTranslations[civilization][j][0].empty() && PlayerRaces.SettlementNameSuffixTranslations[civilization][j][0] == settlement_name.substr(PlayerRaces.SettlementNamePrefixTranslations[civilization][i][0].size(), PlayerRaces.SettlementNameSuffixTranslations[civilization][j][0].size())) {
						new_settlement_name = PlayerRaces.SettlementNamePrefixTranslations[civilization][i][1];
						new_settlement_name += PlayerRaces.SettlementNameSuffixTranslations[civilization][j][1];
						lua_pushstring(l, new_settlement_name.c_str());
						return 1;
					}
				}
			}
		}
	}
	
	lua_pushstring(l, new_settlement_name.c_str());
	return 1;
}

/**
**  Define a civilization's factions
**
**  @param l  Lua state.
*/
static int CclDefineCivilizationFactions(lua_State *l)
{
	int args = lua_gettop(l);
	int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 1));
	for (int j = 1; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		if (!strcmp(value, "faction")) {
			++j;
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				if (!strcmp(value, "name")) {
					++k;
					PlayerRaces.FactionNames[civilization][(j - 1) / 2] = LuaToString(l, j + 1, k + 1);
					PlayerRaces.FactionPlayability[civilization][(j - 1) / 2] = true; //factions are playable by default
				} else if (!strcmp(value, "type")) {
					++k;
					PlayerRaces.FactionTypes[civilization][(j - 1) / 2] = LuaToString(l, j + 1, k + 1);
				} else if (!strcmp(value, "color")) {
					++k;
					PlayerRaces.FactionColors[civilization][(j - 1) / 2] = LuaToString(l, j + 1, k + 1);
				} else if (!strcmp(value, "secondary_color")) {
					++k;
					PlayerRaces.FactionSecondaryColors[civilization][(j - 1) / 2] = LuaToString(l, j + 1, k + 1);
				} else if (!strcmp(value, "playable")) {
					++k;
					PlayerRaces.FactionPlayability[civilization][(j - 1) / 2] = LuaToBoolean(l, j + 1, k + 1);
				} else {
					LuaError(l, "Unsupported tag: %s" _C_ value);
				}
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}

	return 0;
}

/**
**  Get a civilization's factions.
**
**  @param l  Lua state.
*/
static int CclGetCivilizationFactionNames(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 1));
	lua_pop(l, 1);

	int FactionCount = 0;
	for (int i = 0; i < FactionMax; ++i) {
		if (!PlayerRaces.FactionNames[civilization][i].empty()) {
			FactionCount += 1;
		}
	}

	lua_createtable(l, FactionCount, 0);
	for (int i = 1; i <= FactionCount; ++i)
	{
		lua_pushstring(l, PlayerRaces.FactionNames[civilization][i-1].c_str());
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
	LuaCheckArgs(l, 3);
	const int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 1));
	const char *faction_name = LuaToString(l, 2);
	int civilization_faction = 0;
	for (int i = 0; i < FactionMax; ++i) {
		if (PlayerRaces.FactionNames[civilization][i].compare(faction_name) == 0) {
			civilization_faction = i;
			break;
		}
	}
	const char *data = LuaToString(l, 3);

	if (!strcmp(data, "Type")) {
		lua_pushstring(l, PlayerRaces.FactionTypes[civilization][civilization_faction].c_str());
		return 1;
	} else if (!strcmp(data, "Color")) {
		lua_pushstring(l, PlayerRaces.FactionColors[civilization][civilization_faction].c_str());
		return 1;
	} else if (!strcmp(data, "SecondaryColor")) {
		lua_pushstring(l, PlayerRaces.FactionSecondaryColors[civilization][civilization_faction].c_str());
		return 1;
	} else if (!strcmp(data, "Playable")) {
		lua_pushboolean(l, PlayerRaces.FactionPlayability[civilization][civilization_faction]);
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}
//Wyrmgus end

/**
**  Define player colors
**
**  @param l  Lua state.
*/
static int CclDefinePlayerColors(lua_State *l)
{
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}

	const int args = lua_rawlen(l, 1);
	for (int i = 0; i < args; ++i) {
		PlayerColorNames[i / 2] = LuaToString(l, 1, i + 1);
		++i;
		lua_rawgeti(l, 1, i + 1);
		if (!lua_istable(l, -1)) {
			LuaError(l, "incorrect argument");
		}
		const int numcolors = lua_rawlen(l, -1);
		if (numcolors != PlayerColorIndexCount) {
			LuaError(l, "You should use %d colors (See DefinePlayerColorIndex())" _C_ PlayerColorIndexCount);
		}
		for (int j = 0; j < numcolors; ++j) {
			lua_rawgeti(l, -1, j + 1);
			PlayerColorsRGB[i / 2][j].Parse(l);
			lua_pop(l, 1);
		}
	}

	return 0;
}

/**
**  Make new player colors
**
**  @param l  Lua state.
*/
static int CclNewPlayerColors(lua_State *l)
{
	LuaCheckArgs(l, 0);
	SetPlayersPalette();

	return 0;
}

/**
**  Define player color indexes
**
**  @param l  Lua state.
*/
static int CclDefinePlayerColorIndex(lua_State *l)
{
	LuaCheckArgs(l, 2);
	PlayerColorIndexStart = LuaToNumber(l, 1);
	PlayerColorIndexCount = LuaToNumber(l, 2);

	//Wyrmgus start
//	for (int i = 0; i < PlayerMax; ++i) {
	for (int i = 0; i < PlayerColorMax; ++i) {
	//Wyrmgus end
		PlayerColorsRGB[i].clear();
		PlayerColorsRGB[i].resize(PlayerColorIndexCount);
		PlayerColors[i].clear();
		PlayerColors[i].resize(PlayerColorIndexCount, 0);
	}
	return 0;
}

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
		lua_pushstring(l, PlayerRaces.FactionNames[p->Race][p->Faction].c_str());
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "RaceName")) {
		lua_pushstring(l, PlayerRaces.Name[p->Race].c_str());
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Color")) {
		for (int i = 0; i < PlayerColorMax; ++i) {
			if (PlayerColors[i][0] == p->Color) {
				lua_pushstring(l, PlayerColorNames[i].c_str());
				break;
			}		
		}
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
	} else if (!strcmp(data, "UnitTypesCount")) {
		LuaCheckArgs(l, 3);
		CUnitType *type = CclGetUnitType(l);
		Assert(type);
		lua_pushnumber(l, p->UnitTypesCount[type->Slot]);
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "UnitTypesAiActiveCount")) {
		LuaCheckArgs(l, 3);
		CUnitType *type = CclGetUnitType(l);
		Assert(type);
		lua_pushnumber(l, p->UnitTypesAiActiveCount[type->Slot]);
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "AiEnabled")) {
		lua_pushboolean(l, p->AiEnabled);
		return 1;
	} else if (!strcmp(data, "TotalNumUnits")) {
		lua_pushnumber(l, p->GetUnitCount());
		return 1;
	} else if (!strcmp(data, "NumBuildings")) {
		lua_pushnumber(l, p->NumBuildings);
		return 1;
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
			if (UnitIdAllowed(Players[p->Index], id) > 0) {
				lua_pushstring(l, "A");
			} else if (UnitIdAllowed(Players[p->Index], id) == 0) {
				lua_pushstring(l, "F");
			}
		} else if (!strncmp(ident, "upgrade-", 8)) {
			if (UpgradeIdentAllowed(Players[p->Index], ident) == 'A') {
				lua_pushstring(l, "A");
			} else if (UpgradeIdentAllowed(Players[p->Index], ident) == 'R') {
				lua_pushstring(l, "R");
			} else if (UpgradeIdentAllowed(Players[p->Index], ident) == 'F') {
				lua_pushstring(l, "F");
			}
		} else {
			DebugPrint(" wrong ident %s\n" _C_ ident);
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

	if (!strcmp(data, "Name")) {
		p->SetName(LuaToString(l, 3));
	} else if (!strcmp(data, "RaceName")) {
		const char *racename = LuaToString(l, 3);
		p->Race = PlayerRaces.GetRaceIndexByName(racename);
		
		if (p->Race == -1) {
			LuaError(l, "invalid race name '%s'" _C_ racename);
		}
		
		//Wyrmgus start
		//if the civilization of the person player changed, update the UI
		if (ThisPlayer) {
			if (ThisPlayer->Index == p->Index) {
				LoadCursors(PlayerRaces.Name[p->Race]);
				UI.Load();
			}
		} else if (p->Index == 0) {
			LoadCursors(PlayerRaces.Name[p->Race]);
			UI.Load();
		}
		SetDefaultTextColors(UI.NormalFontColor, UI.ReverseFontColor);
		
		// set random one from the civilization's factions
		int FactionCount = 0;
		int LocalFactions[FactionMax];
		for (int i = 0; i < FactionMax; ++i) {
			if (!PlayerRaces.FactionNames[p->Race][i].empty()) {
				bool faction_used = false;
				for (int j = 0; j < PlayerMax; ++j) {
					if (p->Index != j && Players[j].Name == PlayerRaces.FactionNames[p->Race][i]) {
						faction_used = true;
					}		
				}
				if (!faction_used && PlayerRaces.FactionTypes[p->Race][i] == "tribe" && PlayerRaces.FactionPlayability[p->Race][i]) {
					LocalFactions[FactionCount] = i;
					FactionCount += 1;
				}
			}
		}
		if (FactionCount > 0) {
			int ChosenFaction = LocalFactions[SyncRand(FactionCount)];
			p->SetFaction(PlayerRaces.FactionNames[p->Race][ChosenFaction]);
		}
		//Wyrmgus end
	//Wyrmgus start
	} else if (!strcmp(data, "Faction")) {
		p->SetFaction(LuaToString(l, 3));
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

		if (!strncmp(ident, "upgrade-", 8)) {
			if (acquire == "R" && UpgradeIdentAllowed(*p, ident) != 'R') {
				UpgradeAcquire(*p, CUpgrade::Get(ident));
			} else if (acquire == "F" || acquire == "A") {
				if (UpgradeIdentAllowed(*p, ident) == 'R') {
					UpgradeLost(*p, CUpgrade::Get(ident)->ID);
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

	lua_register(Lua, "DefineRaceNames", CclDefineRaceNames);
	//Wyrmgus start
	lua_register(Lua, "DefineNewRaceNames", CclDefineNewRaceNames);
	lua_register(Lua, "DefineCivilizationLanguage", CclDefineCivilizationLanguage);
	lua_register(Lua, "IsCivilizationPlayable", CclIsCivilizationPlayable);
	lua_register(Lua, "GetCivilizationSpecies", CclGetCivilizationSpecies);
	lua_register(Lua, "GetParentCivilization", CclGetParentCivilization);
	lua_register(Lua, "GenerateProvinceName", CclGenerateProvinceName);
	lua_register(Lua, "GenerateSettlementName", CclGenerateSettlementName);
	lua_register(Lua, "TranslateProvinceName", CclTranslateProvinceName);
	lua_register(Lua, "TranslateSettlementName", CclTranslateSettlementName);
	lua_register(Lua, "DefineCivilizationFactions", CclDefineCivilizationFactions);
	lua_register(Lua, "GetCivilizationFactionNames", CclGetCivilizationFactionNames);
	lua_register(Lua, "GetFactionData", CclGetFactionData);
	//Wyrmgus end
	lua_register(Lua, "DefinePlayerColors", CclDefinePlayerColors);
	lua_register(Lua, "DefinePlayerColorIndex", CclDefinePlayerColorIndex);

	lua_register(Lua, "NewColors", CclNewPlayerColors);

	// player member access functions
	lua_register(Lua, "GetPlayerData", CclGetPlayerData);
	lua_register(Lua, "SetPlayerData", CclSetPlayerData);
	lua_register(Lua, "SetAiType", CclSetAiType);
}

//@}
