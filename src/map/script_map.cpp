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
/**@name script_map.cpp - The map ccl functions. */
//
//      (c) Copyright 1999-2005 by Lutz Sammer and Jimmy Salmon
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

#include "map.h"

//Wyrmgus start
#include "editor.h"
#include "game.h"
//Wyrmgus end
#include "iolib.h"
//Wyrmgus start
#include "province.h"
//Wyrmgus end
#include "script.h"
//Wyrmgus start
#include "settings.h"
//Wyrmgus end
#include "tileset.h"
#include "translate.h"
#include "ui.h"
#include "unit.h"
#include "version.h"
#include "video.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Parse a map.
**
**  @param l  Lua state.
*/
static int CclStratagusMap(lua_State *l)
{
	int args = lua_gettop(l);
	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		++j;

		if (!strcmp(value, "version")) {
			//Wyrmgus start
//			char buf[32];
			char buf[64];
			//Wyrmgus end

			const char *version = LuaToString(l, j + 1);
			strncpy(buf, VERSION, sizeof(buf));
			if (strcmp(buf, version)) {
				fprintf(stderr, "Warning not saved with this version.\n");
			}
		} else if (!strcmp(value, "uid")) {
			Map.Info.MapUID = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "description")) {
			Map.Info.Description = LuaToString(l, j + 1);
		} else if (!strcmp(value, "the-map")) {
			if (!lua_istable(l, j + 1)) {
				//Wyrmgus start
//				LuaError(l, "incorrect argument");
				LuaError(l, "incorrect argument for \"the-map\"");
				//Wyrmgus end
			}
			int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const char *value = LuaToString(l, j + 1, k + 1);
				++k;

				if (!strcmp(value, "size")) {
					lua_rawgeti(l, j + 1, k + 1);
					CclGetPos(l, &Map.Info.MapWidth, &Map.Info.MapHeight);
					lua_pop(l, 1);

					//Wyrmgus start
//					delete[] Map.Fields;
//					Map.Fields = new CMapField[Map.Info.MapWidth * Map.Info.MapHeight];
					for (size_t z = 0; z < Map.Fields.size(); ++z) {
						delete[] Map.Fields[z];
					}
					Map.Fields.clear();
					Map.Fields.push_back(new CMapField[Map.Info.MapWidth * Map.Info.MapHeight]);
					Map.Info.MapWidths.clear();
					Map.Info.MapWidths.push_back(Map.Info.MapWidth);
					Map.Info.MapHeights.clear();
					Map.Info.MapHeights.push_back(Map.Info.MapHeight);
					//Wyrmgus end
					// FIXME: this should be CreateMap or InitMap?
				} else if (!strcmp(value, "fog-of-war")) {
					Map.NoFogOfWar = false;
					--k;
				} else if (!strcmp(value, "no-fog-of-war")) {
					Map.NoFogOfWar = true;
					--k;
				} else if (!strcmp(value, "filename")) {
					Map.Info.Filename = LuaToString(l, j + 1, k + 1);
				//Wyrmgus start
				} else if (!strcmp(value, "extra-map-layers")) {
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument for \"extra-map-layers\"");
					}
					const int subsubargs = lua_rawlen(l, -1);
					for (int z = 0; z < subsubargs; ++z) {
						lua_rawgeti(l, -1, z + 1);
						if (!lua_istable(l, -1)) {
							LuaError(l, "incorrect argument");
						}
						int map_layer_width = LuaToNumber(l, -1, 1);
						int map_layer_height = LuaToNumber(l, -1, 2);
						Map.Info.MapWidths.push_back(map_layer_width);
						Map.Info.MapHeights.push_back(map_layer_height);
						Map.Fields.push_back(new CMapField[map_layer_width * map_layer_height]);
						lua_pop(l, 1);
					}
					lua_pop(l, 1);
				} else if (!strcmp(value, "time-of-day")) {
					Map.TimeOfDaySeconds.clear();
					Map.TimeOfDay.clear();
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument for \"time-of-day\"");
					}
					const int subsubargs = lua_rawlen(l, -1);
					for (int z = 0; z < subsubargs; ++z) {
						if (!lua_istable(l, -1)) {
							LuaError(l, "incorrect argument for \"time-of-day\"");
						}
						lua_rawgeti(l, -1, z + 1);
						int time_of_day_seconds = LuaToNumber(l, -1, 1);
						int time_of_day = LuaToNumber(l, -1, 2);
						Map.TimeOfDaySeconds.push_back(time_of_day_seconds);
						Map.TimeOfDay.push_back(time_of_day);
						lua_pop(l, 1);
					}
					lua_pop(l, 1);
				} else if (!strcmp(value, "layer-references")) {
					Map.Planes.clear();
					Map.Worlds.clear();
					Map.Layers.clear();
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument for \"layer-references\"");
					}
					const int subsubargs = lua_rawlen(l, -1);
					for (int z = 0; z < subsubargs; ++z) {
						if (!lua_istable(l, -1)) {
							LuaError(l, "incorrect argument for \"layer-references\"");
						}
						lua_rawgeti(l, -1, z + 1);
						Map.Planes.push_back(GetPlane(LuaToString(l, -1, 1)));
						Map.Worlds.push_back(GetWorld(LuaToString(l, -1, 2)));
						Map.Layers.push_back(LuaToNumber(l, -1, 3));
						Map.LayerConnectors.resize(z + 1);
						lua_pop(l, 1);
					}
					lua_pop(l, 1);
				//Wyrmgus end
				} else if (!strcmp(value, "map-fields")) {
					//Wyrmgus start
					/*
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument");
					}
					const int subsubargs = lua_rawlen(l, -1);
					if (subsubargs != Map.Info.MapWidth * Map.Info.MapHeight) {
						fprintf(stderr, "Wrong tile table length: %d\n", subsubargs);
					}
					for (int i = 0; i < subsubargs; ++i) {
						lua_rawgeti(l, -1, i + 1);
						if (!lua_istable(l, -1)) {
							LuaError(l, "incorrect argument");
						}
						Map.Fields[i].parse(l);
						lua_pop(l, 1);
					}
					lua_pop(l, 1);
					*/
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument");
					}
					const int subsubargs = lua_rawlen(l, -1);
					for (int z = 0; z < subsubargs; ++z) {
						lua_rawgeti(l, -1, z + 1);
						if (!lua_istable(l, -1)) {
							LuaError(l, "incorrect argument");
						}
						const int subsubsubargs = lua_rawlen(l, -1);
						if (subsubsubargs != Map.Info.MapWidths[z] * Map.Info.MapHeights[z]) {
							fprintf(stderr, "Wrong tile table length: %d\n", subsubsubargs);
						}
						for (int i = 0; i < subsubsubargs; ++i) {
							lua_rawgeti(l, -1, i + 1);
							if (!lua_istable(l, -1)) {
								LuaError(l, "incorrect argument");
							}
							//Wyrmgus start
	//						Map.Fields[i].parse(l);
							Map.Fields[z][i].parse(l);
							//Wyrmgus end
							lua_pop(l, 1);
						}
						lua_pop(l, 1);
					}
					lua_pop(l, 1);
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

/**
**  Reveal the complete map.
**
**  @param l  Lua state.
*/
static int CclRevealMap(lua_State *l)
{
	//Wyrmgus start
//	LuaCheckArgs(l, 0);
	//Wyrmgus end
	//Wyrmgus start
//	if (CclInConfigFile || !Map.Fields) {
	if (CclInConfigFile || Map.Fields.size() == 0) {
	//Wyrmgus end
		FlagRevealMap = 1;
	} else {
		//Wyrmgus start
//		Map.Reveal();
		bool only_person_players = false;
		const int nargs = lua_gettop(l);
		if (nargs == 1) {
			only_person_players = LuaToBoolean(l, 1);
		}
		Map.Reveal(only_person_players);
		//Wyrmgus end
	}
	return 0;
}

/**
**  Center the map.
**
**  @param l  Lua state.
*/
static int CclCenterMap(lua_State *l)
{
	LuaCheckArgs(l, 2);
	const Vec2i pos(LuaToNumber(l, 1), LuaToNumber(l, 2));

	UI.SelectedViewport->Center(Map.TilePosToMapPixelPos_Center(pos));
	return 0;
}

/**
**  Define the starting viewpoint for a given player.
**
**  @param l  Lua state.
*/
static int CclSetStartView(lua_State *l)
{
	LuaCheckArgs(l, 3);

	const int p = LuaToNumber(l, 1);
	Players[p].StartPos.x = LuaToNumber(l, 2);
	Players[p].StartPos.y = LuaToNumber(l, 3);

	return 0;
}

/**
**  Show Map Location
**
**  @param l  Lua state.
*/
static int CclShowMapLocation(lua_State *l)
{
	// Put a unit on map, use its properties, except for
	// what is listed below

	LuaCheckArgs(l, 4);
	const char *unitname = LuaToString(l, 5);
	CUnitType *unitType = UnitTypeByIdent(unitname);
	if (!unitType) {
		DebugPrint("Unable to find UnitType '%s'" _C_ unitname);
		return 0;
	}
	CUnit *target = MakeUnit(*unitType, ThisPlayer);
	if (target != NULL) {
		target->Variable[HP_INDEX].Value = 0;
		target->tilePos.x = LuaToNumber(l, 1);
		target->tilePos.y = LuaToNumber(l, 2);
		target->TTL = GameCycle + LuaToNumber(l, 4);
		target->CurrentSightRange = LuaToNumber(l, 3);
		//Wyrmgus start
		UpdateUnitSightRange(*target);
		//Wyrmgus end
		MapMarkUnitSight(*target);
	} else {
		DebugPrint("Unable to allocate Unit");
	}
	return 0;
}

/**
**  Set fog of war on/off.
**
**  @param l  Lua state.
*/
static int CclSetFogOfWar(lua_State *l)
{
	LuaCheckArgs(l, 1);
	Map.NoFogOfWar = !LuaToBoolean(l, 1);
	//Wyrmgus start
//	if (!CclInConfigFile && Map.Fields) {
	if (!CclInConfigFile && Map.Fields.size() > 0) {
	//Wyrmgus end
		UpdateFogOfWarChange();
		// FIXME: save setting in replay log
		//CommandLog("input", NoUnitP, FlushCommands, -1, -1, NoUnitP, "fow off", -1);
	}
	return 0;
}

static int CclGetFogOfWar(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushboolean(l, !Map.NoFogOfWar);
	return 1;
}

/**
**  Enable display of terrain in minimap.
**
**  @param l  Lua state.
*/
static int CclSetMinimapTerrain(lua_State *l)
{
	LuaCheckArgs(l, 1);
	UI.Minimap.WithTerrain = LuaToBoolean(l, 1);
	return 0;
}

/**
**  Fog of war opacity.
**
**  @param l  Lua state.
*/
static int CclSetFogOfWarOpacity(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int i = LuaToNumber(l, 1);
	if (i < 0 || i > 255) {
		PrintFunction();
		fprintf(stdout, "Opacity should be 0 - 256\n");
		i = 100;
	}
	FogOfWarOpacity = i;

	if (!CclInConfigFile) {
		Map.Init();
	}
	return 0;
}

/**
**  Set forest regeneration speed.
**
**  @param l  Lua state.
**
**  @return   Old speed
*/
static int CclSetForestRegeneration(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int i = LuaToNumber(l, 1);
	//Wyrmgus start
	/*
	if (i < 0 || i > 255) {
		PrintFunction();
		fprintf(stdout, "Regeneration speed should be 0 - 255\n");
		i = 100;
	}
	*/
	if (i < 0) {
		PrintFunction();
		fprintf(stdout, "Regeneration speed should be greater than 0\n");
		i = 100;
	}
	//Wyrmgus end
	const int old = ForestRegeneration;
	ForestRegeneration = i;

	lua_pushnumber(l, old);
	return 1;
}

/**
**  Set Fog color.
**
**  @param l  Lua state.
*/
static int CclSetFogOfWarColor(lua_State *l)
{
	LuaCheckArgs(l, 3);
	int r = LuaToNumber(l, 1);
	int g = LuaToNumber(l, 2);
	int b = LuaToNumber(l, 3);

	if ((r < 0 || r > 255) ||
		(g < 0 || g > 255) ||
		(b < 0 || b > 255)) {
		LuaError(l, "Arguments must be in the range 0-255");
	}
	FogOfWarColor.R = r;
	FogOfWarColor.G = g;
	FogOfWarColor.B = b;

	return 0;
}

/**
**  Define Fog graphics
**
**  @param l  Lua state.
*/
static int CclSetFogOfWarGraphics(lua_State *l)
{
	std::string FogGraphicFile;

	LuaCheckArgs(l, 1);
	FogGraphicFile = LuaToString(l, 1);
	if (CMap::FogGraphic) {
		CGraphic::Free(CMap::FogGraphic);
	}
	CMap::FogGraphic = CGraphic::New(FogGraphicFile, PixelTileSize.x, PixelTileSize.y);

	return 0;
}

/**
**  Set a tile
**
**  @param tileIndex   Tile number
**  @param pos    coordinate
**  @param value  Value of the tile
*/
//Wyrmgus start
//void SetTile(unsigned int tileIndex, const Vec2i &pos, int value)
void SetTile(unsigned int tileIndex, const Vec2i &pos, int value, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	if (!Map.Info.IsPointOnMap(pos)) {
	if (!Map.Info.IsPointOnMap(pos, z)) {
	//Wyrmgus end
		fprintf(stderr, "Invalid map coordonate : (%d, %d)\n", pos.x, pos.y);
		return;
	}
	if (Map.Tileset->getTileCount() <= tileIndex) {
		fprintf(stderr, "Invalid tile number: %d\n", tileIndex);
		return;
	}
	//Wyrmgus start
//	if (value < 0 || value >= 256) {
	if (value < 0) {
	//Wyrmgus end
		//Wyrmgus start
//		fprintf(stderr, "Invalid tile number: %d\n", tileIndex);
		fprintf(stderr, "Invalid tile value: %d\n", value);
		//Wyrmgus end
		return;
	}
	
	//Wyrmgus start
//	if (Map.Fields) {
	if (Map.Fields.size() > 0) {
	//Wyrmgus end
		//Wyrmgus start
//		CMapField &mf = *Map.Field(pos);
		CMapField &mf = *Map.Field(pos, z);
		//Wyrmgus end

		mf.setTileIndex(*Map.Tileset, tileIndex, value);
	}
}

//Wyrmgus start
/**
**  Set a tile
**
**  @param tileIndex   Tile number
**  @param pos    coordinate
**  @param value  Value of the tile
*/
void SetTileTerrain(std::string terrain_ident, const Vec2i &pos, int value, int z)
{
	if (!Map.Info.IsPointOnMap(pos, z)) {
		fprintf(stderr, "Invalid map coordinate : (%d, %d)\n", pos.x, pos.y);
		return;
	}
	
	CTerrainType *terrain = GetTerrainType(terrain_ident);
	
	if (!terrain) {
		fprintf(stderr, "Terrain \"%s\" doesn't exist.\n", terrain_ident.c_str());
		return;
	}
	if (value < 0) {
		fprintf(stderr, "Invalid tile value: %d\n", value);
		return;
	}
	
	//Wyrmgus start
//	if (Map.Fields) {
	if (Map.Fields.size() > 0) {
	//Wyrmgus end
		CMapField &mf = *Map.Field(pos, z);

		mf.Value = value;
		mf.SetTerrain(terrain);
	}
}

void SetMapTemplateTileTerrain(std::string map_ident, std::string terrain_ident, int x, int y, std::string tile_label)
{
	CMapTemplate *map = GetMapTemplate(map_ident);
	
	if (!map) {
		fprintf(stderr, "Map template \"%s\" doesn't exist.\n", map_ident.c_str());
		return;
	}
	
	Vec2i pos(x, y);
	
	if (pos.x < 0 || pos.x >= map->Width || pos.y < 0 || pos.y >= map->Height) {
		fprintf(stderr, "Invalid map coordinate : (%d, %d)\n", pos.x, pos.y);
		return;
	}
	
	CTerrainType *terrain = GetTerrainType(terrain_ident);
	
	if (!terrain) {
		fprintf(stderr, "Terrain \"%s\" doesn't exist.\n", terrain_ident.c_str());
		return;
	}
	
	map->SetTileTerrain(pos, terrain);
	
	if (!tile_label.empty()) {
		map->TileLabels[std::pair<int, int>(pos.x, pos.y)] = TransliterateText(tile_label);
	}
}

void SetMapTemplateTileTerrainByID(std::string map_ident, int terrain_id, int x, int y, std::string tile_label)
{
	CMapTemplate *map = GetMapTemplate(map_ident);
	
	if (!map) {
		fprintf(stderr, "Map template \"%s\" doesn't exist.\n", map_ident.c_str());
		return;
	}
	
	Vec2i pos(x, y);
	
	if (pos.x < 0 || pos.x >= map->Width || pos.y < 0 || pos.y >= map->Height) {
		fprintf(stderr, "Invalid map coordinate : (%d, %d)\n", pos.x, pos.y);
		return;
	}
	
	CTerrainType *terrain = TerrainTypes[terrain_id];
	
	if (!terrain) {
		fprintf(stderr, "Terrain doesn't exist.\n");
		return;
	}

	map->SetTileTerrain(pos, terrain);
	
	if (!tile_label.empty()) {
		map->TileLabels[std::pair<int, int>(pos.x, pos.y)] = TransliterateText(tile_label);
	}
}

static int CclSetMapTemplateTileLabel(lua_State *l)
{
	std::string map_template_ident = LuaToString(l, 1);
	CMapTemplate *map_template = GetMapTemplate(map_template_ident);
	if (!map_template) {
		LuaError(l, "Map template doesn't exist.\n");
	}

	std::string label_string = LuaToString(l, 2);
	
	Vec2i ipos;
	CclGetPos(l, &ipos.x, &ipos.y, 3);

	map_template->TileLabels[std::pair<int, int>(ipos.x, ipos.y)] = TransliterateText(label_string);
	
	return 1;
}

static int CclSetMapTemplateResource(lua_State *l)
{
	std::string map_template_ident = LuaToString(l, 1);
	CMapTemplate *map_template = GetMapTemplate(map_template_ident);
	if (!map_template) {
		LuaError(l, "Map template doesn't exist.\n");
	}

	lua_pushvalue(l, 2);
	CUnitType *unittype = CclGetUnitType(l);
	if (unittype == NULL) {
		LuaError(l, "Bad unittype");
	}
	lua_pop(l, 1);
	Vec2i ipos;
	CclGetPos(l, &ipos.x, &ipos.y, 3);

	int resources_held = 0;
	CUniqueItem *unique = NULL;
	
	const int nargs = lua_gettop(l);
	if (nargs >= 4) {
		resources_held = LuaToNumber(l, 4);
	}
	if (nargs >= 5) {
		unique = GetUniqueItem(LuaToString(l, 5));
		if (!unique) {
			LuaError(l, "Unique item doesn't exist.\n");
		}
	}
	
	map_template->Resources[std::pair<int, int>(ipos.x, ipos.y)] = std::tuple<CUnitType *, int, CUniqueItem *>(unittype, resources_held, unique);
	
	return 1;
}

static int CclSetMapTemplateUnit(lua_State *l)
{
	std::string map_template_ident = LuaToString(l, 1);
	CMapTemplate *map_template = GetMapTemplate(map_template_ident);
	if (!map_template) {
		LuaError(l, "Map template doesn't exist.\n");
	}

	lua_pushvalue(l, 2);
	CUnitType *unittype = CclGetUnitType(l);
	if (unittype == NULL) {
		LuaError(l, "Bad unittype");
	}
	lua_pop(l, 1);
	Vec2i ipos;
	CclGetPos(l, &ipos.x, &ipos.y, 4);

	std::string faction_name = LuaToString(l, 3);
	CFaction *faction = PlayerRaces.GetFaction(-1, faction_name);
	if (!faction) {
		LuaError(l, "Faction doesn't exist.\n");
	}

	int start_year = 0;
	int end_year = 0;
	const int nargs = lua_gettop(l);
	if (nargs >= 5) {
		start_year = LuaToNumber(l, 5);
	}
	if (nargs >= 6) {
		end_year = LuaToNumber(l, 6);
	}
	
	map_template->Units.push_back(std::tuple<Vec2i, CUnitType *, CFaction *, int, int>(ipos, unittype, faction, start_year, end_year));
	
	return 1;
}

static int CclSetMapTemplateLayerConnector(lua_State *l)
{
	std::string map_template_ident = LuaToString(l, 1);
	CMapTemplate *map_template = GetMapTemplate(map_template_ident);
	if (!map_template) {
		LuaError(l, "Map template doesn't exist.\n");
	}

	lua_pushvalue(l, 2);
	CUnitType *unittype = CclGetUnitType(l);
	if (unittype == NULL) {
		LuaError(l, "Bad unittype");
	}
	lua_pop(l, 1);
	Vec2i ipos;
	CclGetPos(l, &ipos.x, &ipos.y, 3);

	CUniqueItem *unique = NULL;
	
	const int nargs = lua_gettop(l);
	if (nargs >= 5) {
		unique = GetUniqueItem(LuaToString(l, 5));
		if (!unique) {
			LuaError(l, "Unique item doesn't exist.\n");
		}
	}
	
	if (lua_isstring(l, 4)) {
		std::string realm = LuaToString(l, 4);
		if (GetWorld(realm)) {
			map_template->WorldConnectors.push_back(std::tuple<Vec2i, CUnitType *, CWorld *, CUniqueItem *>(ipos, unittype, GetWorld(realm), unique));
		} else if (GetPlane(realm)) {
			map_template->PlaneConnectors.push_back(std::tuple<Vec2i, CUnitType *, CPlane *, CUniqueItem *>(ipos, unittype, GetPlane(realm), unique));
		} else {
			LuaError(l, "incorrect argument");
		}
	} else if (lua_isnumber(l, 4)) {
		int layer = LuaToNumber(l, 4);
		map_template->LayerConnectors.push_back(std::tuple<Vec2i, CUnitType *, int, CUniqueItem *>(ipos, unittype, layer, unique));
	} else {
		LuaError(l, "incorrect argument");
	}
	
	return 1;
}

void ApplyMapTemplate(std::string map_template_ident, int template_start_x, int template_start_y, int map_start_x, int map_start_y, int z)
{
	if (SaveGameLoading) {
		return;
	}
	
	CMapTemplate *map_template = GetMapTemplate(map_template_ident);
	
	if (!map_template) {
		fprintf(stderr, "Map template \"%s\" doesn't exist.\n", map_template_ident.c_str());
		return;
	}
	
	Vec2i template_start_pos(template_start_x, template_start_y);
	if (template_start_pos.x < 0 || template_start_pos.x >= map_template->Width || template_start_pos.y < 0 || template_start_pos.y >= map_template->Height) {
		fprintf(stderr, "Invalid map coordinate : (%d, %d)\n", template_start_pos.x, template_start_pos.y);
		return;
	}
	
	if (z >= (int) Map.Fields.size()) {
		Map.Info.MapWidths.push_back(map_template->Width);
		Map.Info.MapHeights.push_back(map_template->Height);
		Map.Fields.push_back(new CMapField[map_template->Width * map_template->Height]);
		Map.TimeOfDaySeconds.push_back(map_template->TimeOfDaySeconds);
		Map.TimeOfDay.push_back(NoTimeOfDay);
		Map.Planes.push_back(map_template->Plane);
		Map.Worlds.push_back(map_template->World);
		Map.Layers.push_back(map_template->Layer);
		Map.LayerConnectors.resize(z + 1);
	} else {
		if (!map_template->IsSubtemplateArea()) {
			Map.TimeOfDaySeconds[z] = map_template->TimeOfDaySeconds;
			Map.Planes[z] = map_template->Plane;
			Map.Worlds[z] = map_template->World;
			Map.Layers[z] = map_template->Layer;
		}
	}

	if (map_template->TimeOfDaySeconds && !GameSettings.Inside && !GameSettings.NoTimeOfDay && Editor.Running == EditorNotRunning && !map_template->IsSubtemplateArea()) {
		Map.TimeOfDay[z] = SyncRand(MaxTimesOfDay - 1) + 1; // begin at a random time of day
	}
	
	Vec2i map_start_pos(map_start_x, map_start_y);
	Vec2i map_end(std::min(Map.Info.MapWidths[z], map_start_x + map_template->Width), std::min(Map.Info.MapHeights[z], map_start_y + map_template->Height));
	if (!Map.Info.IsPointOnMap(map_start_pos, z)) {
		fprintf(stderr, "Invalid map coordinate : (%d, %d)\n", map_start_pos.x, map_start_pos.y);
		return;
	}
	
	for (int x = 0; x < Map.Info.MapWidths[z]; ++x) {
		if ((template_start_pos.x + x) >= map_template->Width) {
			break;
		}
		for (int y = 0; y < Map.Info.MapHeights[z]; ++y) {
			if ((template_start_pos.y + y) >= map_template->Height) {
				break;
			}
			Vec2i pos(template_start_pos.x + x, template_start_pos.y + y);
			Vec2i real_pos(map_start_pos.x + x, map_start_pos.y + y);
			if (map_template->GetTileTerrain(pos, false)) {
				SetTileTerrain(map_template->GetTileTerrain(pos, false)->Ident, real_pos, 0, z);
			}
			if (map_template->GetTileTerrain(pos, true)) {
				SetTileTerrain(map_template->GetTileTerrain(pos, true)->Ident, real_pos, 0, z);
			}
		}
	}
	
	if (map_template->IsSubtemplateArea() && map_template->SurroundingTerrain) {
		Vec2i surrounding_start_pos(map_start_pos - Vec2i(1, 1));
		Vec2i surrounding_end(map_end + Vec2i(1, 1));
		for (int x = surrounding_start_pos.x; x < surrounding_end.x; ++x) {
			for (int y = surrounding_start_pos.y; y < surrounding_end.y; y += (surrounding_end.y - surrounding_start_pos.y - 1)) {
				Vec2i surrounding_pos(x, y);
				if (!Map.Info.IsPointOnMap(surrounding_pos, z) || Map.IsPointInASubtemplateArea(surrounding_pos, z)) {
					continue;
				}
				SetTileTerrain(map_template->SurroundingTerrain->Ident, surrounding_pos, 0, z);
			}
		}
		for (int x = surrounding_start_pos.x; x < surrounding_end.x; x += (surrounding_end.x - surrounding_start_pos.x - 1)) {
			for (int y = surrounding_start_pos.y; y < surrounding_end.y; ++y) {
				Vec2i surrounding_pos(x, y);
				if (!Map.Info.IsPointOnMap(surrounding_pos, z) || Map.IsPointInASubtemplateArea(surrounding_pos, z)) {
					continue;
				}
				SetTileTerrain(map_template->SurroundingTerrain->Ident, surrounding_pos, 0, z);
			}
		}
	}
	
	for (std::map<std::pair<int, int>, std::string>::iterator iterator = map_template->TileLabels.begin(); iterator != map_template->TileLabels.end(); ++iterator) {
		Vec2i label_pos(map_start_pos.x + iterator->first.first - template_start_pos.x, map_start_pos.y + iterator->first.second - template_start_pos.y);
		if (!Map.Info.IsPointOnMap(label_pos, z)) {
			continue;
		}
		
		Map.Field(label_pos, z)->Label = iterator->second;
	}
	
	if (!PlayerFaction.empty() && !map_template->IsSubtemplateArea()) {
		CFaction *player_faction = PlayerRaces.GetFaction(-1, PlayerFaction);
		
		if (player_faction) {
			ThisPlayer->SetCivilization(player_faction->Civilization);
			ThisPlayer->SetFaction(player_faction->Name);
		}
	}
	
	for (size_t i = 0; i < map_template->Subtemplates.size(); ++i) {
		Vec2i random_pos(0, 0);
		Vec2i min_pos(map_start_pos);
		Vec2i max_pos(map_end.x - map_template->Subtemplates[i]->Width, map_end.y - map_template->Subtemplates[i]->Height);
		int while_count = 0;
		while (while_count < 1000) {
			random_pos.x = SyncRand(max_pos.x - min_pos.x + 1) + min_pos.x;
			random_pos.y = SyncRand(max_pos.y - min_pos.y + 1) + min_pos.y;
			
			bool on_map = Map.Info.IsPointOnMap(random_pos, z) && Map.Info.IsPointOnMap(Vec2i(random_pos.x + map_template->Subtemplates[i]->Width - 1, random_pos.y + map_template->Subtemplates[i]->Height - 1), z);
			
			bool on_subtemplate_area = false;
			for (int x = 0; x < map_template->Subtemplates[i]->Width; ++x) {
				for (int y = 0; y < map_template->Subtemplates[i]->Height; ++y) {
					if (Map.IsPointInASubtemplateArea(random_pos + Vec2i(x, y), z)) {
						on_subtemplate_area = true;
						break;
					}
				}
				if (on_subtemplate_area) {
					break;
				}
			}
			
			if (on_map && !on_subtemplate_area) {
				ApplyMapTemplate(map_template->Subtemplates[i]->Ident, 0, 0, random_pos.x, random_pos.y, z);
				
				Map.SubtemplateAreas[z].push_back(std::pair<Vec2i, Vec2i>(random_pos, Vec2i(random_pos.x + map_template->Subtemplates[i]->Width - 1, random_pos.y + map_template->Subtemplates[i]->Height - 1)));
				
				for (size_t j = 0; j < map_template->Subtemplates[i]->ExternalGeneratedTerrains.size(); ++j) {
					Vec2i external_start_pos(random_pos.x - (map_template->Subtemplates[i]->Width / 2), random_pos.y - (map_template->Subtemplates[i]->Height / 2));
					Vec2i external_end(random_pos.x + map_template->Subtemplates[i]->Width + (map_template->Subtemplates[i]->Width / 2), random_pos.y + map_template->Subtemplates[i]->Height + (map_template->Subtemplates[i]->Height / 2));
					int map_width = (external_end.x - external_start_pos.x);
					int map_height = (external_end.y - external_start_pos.y);
					int expansion_number = 0;
					
					int degree_level = map_template->Subtemplates[i]->ExternalGeneratedTerrains[j].second;
					
					if (degree_level == ExtremelyHighDegreeLevel) {
						expansion_number = map_width * map_height / 2;
					} else if (degree_level == VeryHighDegreeLevel) {
						expansion_number = map_width * map_height / 4;
					} else if (degree_level == HighDegreeLevel) {
						expansion_number = map_width * map_height / 8;
					} else if (degree_level == MediumDegreeLevel) {
						expansion_number = map_width * map_height / 16;
					} else if (degree_level == LowDegreeLevel) {
						expansion_number = map_width * map_height / 32;
					} else if (degree_level == VeryLowDegreeLevel) {
						expansion_number = map_width * map_height / 64;
					}
					
					Map.GenerateTerrain(map_template->Subtemplates[i]->ExternalGeneratedTerrains[j].first, 0, expansion_number, external_start_pos, external_end - Vec2i(1, 1), !map_template->Subtemplates[i]->TerrainFile.empty(), z);
				}
				break;
			}
			
			while_count += 1;
		}
	}
	
	if (!map_template->IsSubtemplateArea()) {
		Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
		Map.AdjustTileMapTransitions(map_start_pos, map_end, z);
		Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
	} else {
		Map.AdjustTileMapIrregularities(false, map_start_pos + Vec2i(1, 1), map_end - Vec2i(1, 1), z);
		Map.AdjustTileMapIrregularities(true, map_start_pos + Vec2i(1, 1), map_end - Vec2i(1, 1), z);
		Map.AdjustTileMapTransitions(map_start_pos + Vec2i(1, 1), map_end - Vec2i(1, 1), z);
		Map.AdjustTileMapIrregularities(false, map_start_pos + Vec2i(1, 1), map_end - Vec2i(1, 1), z);
		Map.AdjustTileMapIrregularities(true, map_start_pos + Vec2i(1, 1), map_end - Vec2i(1, 1), z);
	}
	
	for (std::map<std::pair<int, int>, std::tuple<CUnitType *, int, CUniqueItem *>>::iterator iterator = map_template->Resources.begin(); iterator != map_template->Resources.end(); ++iterator) {
		Vec2i unit_pos(map_start_pos.x + iterator->first.first - template_start_pos.x, map_start_pos.y + iterator->first.second - template_start_pos.y);
		if (!Map.Info.IsPointOnMap(unit_pos, z)) {
			continue;
		}
		
		Vec2i unit_offset((std::get<0>(iterator->second)->TileWidth - 1) / 2, (std::get<0>(iterator->second)->TileHeight - 1) / 2);
		CUnit *unit = CreateResourceUnit(unit_pos - unit_offset, *std::get<0>(iterator->second), z);
		
		if (std::get<1>(iterator->second)) {
			unit->SetResourcesHeld(std::get<1>(iterator->second));
			unit->Variable[GIVERESOURCE_INDEX].Value = std::get<1>(iterator->second);
			unit->Variable[GIVERESOURCE_INDEX].Max = std::get<1>(iterator->second);
			unit->Variable[GIVERESOURCE_INDEX].Enable = 1;
		}
		
		if (std::get<2>(iterator->second)) {
			unit->SetUnique(std::get<2>(iterator->second));
		}
	}

	for (size_t i = 0; i < map_template->PlaneConnectors.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(map_template->PlaneConnectors[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		if (unit_raw_pos.x == -1 && unit_raw_pos.y == -1) { // if the unit's coordinates were set to {-1, -1}, then randomly generate its location
			unit_pos = Map.GenerateUnitLocation(std::get<1>(map_template->PlaneConnectors[i]), NULL, map_start_pos, map_end - Vec2i(1, 1), z);
		}
		if (!Map.Info.IsPointOnMap(unit_pos, z)) {
			continue;
		}
		Vec2i unit_offset((std::get<1>(map_template->PlaneConnectors[i])->TileWidth - 1) / 2, (std::get<1>(map_template->PlaneConnectors[i])->TileHeight - 1) / 2);
		CUnit *unit = CreateUnit(unit_pos - unit_offset, *std::get<1>(map_template->PlaneConnectors[i]), &Players[PlayerNumNeutral], z);
		Map.LayerConnectors[z].push_back(unit);
		for (size_t second_z = 0; second_z < Map.LayerConnectors.size(); ++second_z) {
			bool found_other_connector = false;
			if (Map.Planes[second_z] == std::get<2>(map_template->PlaneConnectors[i])) {
				for (size_t j = 0; j < Map.LayerConnectors[second_z].size(); ++j) {
					if (Map.LayerConnectors[second_z][j]->Type == unit->Type && Map.LayerConnectors[second_z][j]->Unique == unit->Unique && Map.LayerConnectors[second_z][j]->ConnectingDestination == NULL) {
						Map.LayerConnectors[second_z][j]->ConnectingDestination = unit;
						unit->ConnectingDestination = Map.LayerConnectors[second_z][j];
						found_other_connector = true;
						break;
					}
				}
			}
			if (found_other_connector) {
				break;
			}
		}
	}
	
	for (size_t i = 0; i < map_template->WorldConnectors.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(map_template->WorldConnectors[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		if (unit_raw_pos.x == -1 && unit_raw_pos.y == -1) { // if the unit's coordinates were set to {-1, -1}, then randomly generate its location
			unit_pos = Map.GenerateUnitLocation(std::get<1>(map_template->WorldConnectors[i]), NULL, map_start_pos, map_end - Vec2i(1, 1), z);
		}
		if (!Map.Info.IsPointOnMap(unit_pos, z)) {
			continue;
		}
		Vec2i unit_offset((std::get<1>(map_template->WorldConnectors[i])->TileWidth - 1) / 2, (std::get<1>(map_template->WorldConnectors[i])->TileHeight - 1) / 2);
		CUnit *unit = CreateUnit(unit_pos - unit_offset, *std::get<1>(map_template->WorldConnectors[i]), &Players[PlayerNumNeutral], z);
		Map.LayerConnectors[z].push_back(unit);
		for (size_t second_z = 0; second_z < Map.LayerConnectors.size(); ++second_z) {
			bool found_other_connector = false;
			if (Map.Worlds[second_z] == std::get<2>(map_template->WorldConnectors[i])) {
				for (size_t j = 0; j < Map.LayerConnectors[second_z].size(); ++j) {
					if (Map.LayerConnectors[second_z][j]->Type == unit->Type && Map.LayerConnectors[second_z][j]->Unique == unit->Unique && Map.LayerConnectors[second_z][j]->ConnectingDestination == NULL) {
						Map.LayerConnectors[second_z][j]->ConnectingDestination = unit;
						unit->ConnectingDestination = Map.LayerConnectors[second_z][j];
						found_other_connector = true;
						break;
					}
				}
			}
			if (found_other_connector) {
				break;
			}
		}
	}
	
	for (size_t i = 0; i < map_template->LayerConnectors.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(map_template->LayerConnectors[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		if (unit_raw_pos.x == -1 && unit_raw_pos.y == -1) { // if the unit's coordinates were set to {-1, -1}, then randomly generate its location
			unit_pos = Map.GenerateUnitLocation(std::get<1>(map_template->LayerConnectors[i]), NULL, map_start_pos, map_end - Vec2i(1, 1), z);
		}
		if (!Map.Info.IsPointOnMap(unit_pos, z)) {
			continue;
		}
		Vec2i unit_offset((std::get<1>(map_template->LayerConnectors[i])->TileWidth - 1) / 2, (std::get<1>(map_template->LayerConnectors[i])->TileHeight - 1) / 2);
		CUnit *unit = CreateUnit(unit_pos - unit_offset, *std::get<1>(map_template->LayerConnectors[i]), &Players[PlayerNumNeutral], z);
		Map.LayerConnectors[z].push_back(unit);
		for (size_t second_z = 0; second_z < Map.LayerConnectors.size(); ++second_z) {
			bool found_other_connector = false;
			if (Map.Layers[second_z] == std::get<2>(map_template->LayerConnectors[i])) {
				for (size_t j = 0; j < Map.LayerConnectors[second_z].size(); ++j) {
					if (Map.LayerConnectors[second_z][j]->Type == unit->Type && Map.LayerConnectors[second_z][j]->Unique == unit->Unique && Map.LayerConnectors[second_z][j]->ConnectingDestination == NULL) {
						Map.LayerConnectors[second_z][j]->ConnectingDestination = unit;
						unit->ConnectingDestination = Map.LayerConnectors[second_z][j];
						found_other_connector = true;
						break;
					}
				}
			}
			if (found_other_connector) {
				break;
			}
		}
	}
	
	for (size_t i = 0; i < map_template->Units.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(map_template->Units[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		if (unit_raw_pos.x == -1 && unit_raw_pos.y == -1) { // if the unit's coordinates were set to {-1, -1}, then randomly generate its location
			unit_pos = Map.GenerateUnitLocation(std::get<1>(map_template->Units[i]), std::get<2>(map_template->Units[i]), map_start_pos, map_end - Vec2i(1, 1), z);
		}
		if (!Map.Info.IsPointOnMap(unit_pos, z)) {
			continue;
		}
		
//		if ((std::get<3>(map_template->Units[i]) == 0 || GrandStrategyYear >= std::get<3>(map_template->Units[i])) && (std::get<4>(map_template->Units[i]) == 0 || GrandStrategyYear < std::get<4>(map_template->Units[i]))) {
			CPlayer *player = NULL;
			if (std::get<2>(map_template->Units[i])) {
				player = GetOrAddFactionPlayer(std::get<2>(map_template->Units[i]));
				if (player->StartPos.x == 0 && player->StartPos.y == 0) {
					player->SetStartView(unit_pos, z);
				}
			} else {
				player = &Players[PlayerNumNeutral];
			}
			Vec2i unit_offset((std::get<1>(map_template->Units[i])->TileWidth - 1) / 2, (std::get<1>(map_template->Units[i])->TileHeight - 1) / 2);
			CUnit *unit = CreateUnit(unit_pos - unit_offset, *std::get<1>(map_template->Units[i]), player, z);
				
			if (unit->Type->CanStore[GoldCost]) { //if can store gold (i.e. is a town hall), create five worker units around it
				int civilization = PlayerRaces.GetRaceIndexByName(unit->Type->Civilization.c_str());
				int faction = PlayerRaces.GetFactionIndexByName(civilization, unit->Type->Faction.c_str());
				int worker_type_id = PlayerRaces.GetFactionClassUnitType(civilization, faction, GetUnitTypeClassIndexByName("worker"));
					
				if (worker_type_id != -1) {
					Vec2i worker_unit_offset((UnitTypes[worker_type_id]->TileWidth - 1) / 2, (UnitTypes[worker_type_id]->TileHeight - 1) / 2);
					for (int i = 0; i < 5; ++i) {
						CUnit *worker_unit = CreateUnit(unit_pos - worker_unit_offset, *UnitTypes[worker_type_id], player, z);
					}
				}
			} else if (unit->Type->CanStore[WoodCost] || unit->Type->CanStore[StoneCost]) { //if can store lumber or stone (i.e. is a lumber mill), create two worker units around it
				int civilization = PlayerRaces.GetRaceIndexByName(unit->Type->Civilization.c_str());
				int faction = PlayerRaces.GetFactionIndexByName(civilization, unit->Type->Faction.c_str());
				int worker_type_id = PlayerRaces.GetFactionClassUnitType(civilization, faction, GetUnitTypeClassIndexByName("worker"));
					
				if (worker_type_id != -1) {
					Vec2i worker_unit_offset((UnitTypes[worker_type_id]->TileWidth - 1) / 2, (UnitTypes[worker_type_id]->TileHeight - 1) / 2);
					for (int i = 0; i < 2; ++i) {
						CUnit *worker_unit = CreateUnit(unit_pos - worker_unit_offset, *UnitTypes[worker_type_id], player, z);
					}
				}
			}
//		}
	}
	
	for (int i = 0; i < PlayerMax; ++i) {
		if (Players[i].Type != PlayerPerson && Players[i].Type != PlayerComputer && Players[i].Type != PlayerRescueActive) {
			continue;
		}
		if (Map.IsPointInASubtemplateArea(Players[i].StartPos, z)) {
			continue;
		}
		if (Players[i].StartPos.x < map_start_pos.x || Players[i].StartPos.y < map_start_pos.y || Players[i].StartPos.x >= map_end.x || Players[i].StartPos.y >= map_end.y || Players[i].StartMapLayer != z) {
			continue;
		}
		for (size_t j = 0; j < map_template->PlayerLocationGeneratedNeutralUnits.size(); ++j) {
			Map.GenerateNeutralUnits(map_template->PlayerLocationGeneratedNeutralUnits[j].first, map_template->PlayerLocationGeneratedNeutralUnits[j].second, Players[i].StartPos - Vec2i(8, 8), Players[i].StartPos + Vec2i(8, 8), true, z);
		}
		for (size_t j = 0; j < map_template->PlayerLocationGeneratedTerrains.size(); ++j) {
			int map_width = 16;
			int map_height = 16;
			int seed_number = map_width * map_height / 1024;
			int expansion_number = 0;
			
			int degree_level = map_template->PlayerLocationGeneratedTerrains[j].second;
			
			if (degree_level == ExtremelyHighDegreeLevel) {
				expansion_number = map_width * map_height / 2;
				seed_number = map_width * map_height / 256;
			} else if (degree_level == VeryHighDegreeLevel) {
				expansion_number = map_width * map_height / 4;
				seed_number = map_width * map_height / 512;
			} else if (degree_level == HighDegreeLevel) {
				expansion_number = map_width * map_height / 8;
				seed_number = map_width * map_height / 1024;
			} else if (degree_level == MediumDegreeLevel) {
				expansion_number = map_width * map_height / 16;
				seed_number = map_width * map_height / 2048;
			} else if (degree_level == LowDegreeLevel) {
				expansion_number = map_width * map_height / 32;
				seed_number = map_width * map_height / 4096;
			} else if (degree_level == VeryLowDegreeLevel) {
				expansion_number = map_width * map_height / 64;
				seed_number = map_width * map_height / 8192;
			}
			
			seed_number = std::max(1, seed_number);
			
			Map.GenerateTerrain(map_template->PlayerLocationGeneratedTerrains[j].first, seed_number, expansion_number, Players[i].StartPos - Vec2i(8, 8), Players[i].StartPos + Vec2i(8, 8), !map_template->TerrainFile.empty(), z);
		}
	}
	
	for (size_t i = 0; i < map_template->GeneratedTerrains.size(); ++i) {
		int map_width = (map_end.x - map_start_pos.x);
		int map_height = (map_end.y - map_start_pos.y);
		int seed_number = map_width * map_height / 1024;
		int expansion_number = 0;
		
		int degree_level = map_template->GeneratedTerrains[i].second;
		
		if (degree_level == ExtremelyHighDegreeLevel) {
			expansion_number = map_width * map_height / 2;
			seed_number = map_width * map_height / 256;
		} else if (degree_level == VeryHighDegreeLevel) {
			expansion_number = map_width * map_height / 4;
			seed_number = map_width * map_height / 512;
		} else if (degree_level == HighDegreeLevel) {
			expansion_number = map_width * map_height / 8;
			seed_number = map_width * map_height / 1024;
		} else if (degree_level == MediumDegreeLevel) {
			expansion_number = map_width * map_height / 16;
			seed_number = map_width * map_height / 2048;
		} else if (degree_level == LowDegreeLevel) {
			expansion_number = map_width * map_height / 32;
			seed_number = map_width * map_height / 4096;
		} else if (degree_level == VeryLowDegreeLevel) {
			expansion_number = map_width * map_height / 64;
			seed_number = map_width * map_height / 8192;
		}
		
		seed_number = std::max(1, seed_number);
		
		Map.GenerateTerrain(map_template->GeneratedTerrains[i].first, seed_number, expansion_number, map_start_pos, map_end - Vec2i(1, 1), !map_template->TerrainFile.empty(), z);
	}
	
	for (size_t i = 0; i < map_template->GeneratedNeutralUnits.size(); ++i) {
		Map.GenerateNeutralUnits(map_template->GeneratedNeutralUnits[i].first, map_template->GeneratedNeutralUnits[i].second, map_start_pos, map_end - Vec2i(1, 1), false, z);
	}
	
	if (!map_template->IsSubtemplateArea()) {
		Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
		Map.AdjustTileMapTransitions(map_start_pos, map_end, z);
		Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
	} else {
		Map.AdjustTileMapIrregularities(false, map_start_pos + Vec2i(1, 1), map_end - Vec2i(1, 1), z);
		Map.AdjustTileMapIrregularities(true, map_start_pos + Vec2i(1, 1), map_end - Vec2i(1, 1), z);
		Map.AdjustTileMapTransitions(map_start_pos + Vec2i(1, 1), map_end - Vec2i(1, 1), z);
		Map.AdjustTileMapIrregularities(false, map_start_pos + Vec2i(1, 1), map_end - Vec2i(1, 1), z);
		Map.AdjustTileMapIrregularities(true, map_start_pos + Vec2i(1, 1), map_end - Vec2i(1, 1), z);
	}
}
//Wyrmgus end

/**
**  Define the type of each player available for the map
**
**  @param l  Lua state.
*/
static int CclDefinePlayerTypes(lua_State *l)
{
	int numplayers = lua_gettop(l); /* Number of players == number of arguments */
	if (numplayers < 2) {
		LuaError(l, "Not enough players");
	}

	for (int i = 0; i < numplayers && i < PlayerMax; ++i) {
		if (lua_isnil(l, i + 1)) {
			numplayers = i;
			break;
		}
		const char *type = LuaToString(l, i + 1);
		if (!strcmp(type, "neutral")) {
			Map.Info.PlayerType[i] = PlayerNeutral;
		} else if (!strcmp(type, "nobody")) {
			Map.Info.PlayerType[i] = PlayerNobody;
		} else if (!strcmp(type, "computer")) {
			Map.Info.PlayerType[i] = PlayerComputer;
		} else if (!strcmp(type, "person")) {
			Map.Info.PlayerType[i] = PlayerPerson;
		} else if (!strcmp(type, "rescue-passive")) {
			Map.Info.PlayerType[i] = PlayerRescuePassive;
		} else if (!strcmp(type, "rescue-active")) {
			Map.Info.PlayerType[i] = PlayerRescueActive;
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ type);
		}
	}
	for (int i = numplayers; i < PlayerMax - 1; ++i) {
		Map.Info.PlayerType[i] = PlayerNobody;
	}
	if (numplayers < PlayerMax) {
		Map.Info.PlayerType[PlayerMax - 1] = PlayerNeutral;
	}
	return 0;
}

/**
** Load the lua file which will define the tile models
**
**  @param l  Lua state.
*/
static int CclLoadTileModels(lua_State *l)
{
	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}
	Map.TileModelsFileName = LuaToString(l, 1);
	const std::string filename = LibraryFileName(Map.TileModelsFileName.c_str());
	if (LuaLoadFile(filename) == -1) {
		DebugPrint("Load failed: %s\n" _C_ filename.c_str());
	}
	return 0;
}

/**
**  Define tileset
**
**  @param l  Lua state.
*/
static int CclDefineTileset(lua_State *l)
{
	Map.Tileset->parse(l);

	//  Load and prepare the tileset
	PixelTileSize = Map.Tileset->getPixelTileSize();

	ShowLoadProgress(_("Loading Tileset \"%s\""), Map.Tileset->ImageFile.c_str());
	Map.TileGraphic = CGraphic::New(Map.Tileset->ImageFile, PixelTileSize.x, PixelTileSize.y);
	Map.TileGraphic->Load();
	//Wyrmgus start
	for (size_t i = 0; i != Map.Tileset->solidTerrainTypes.size(); ++i) {
		if (!Map.Tileset->solidTerrainTypes[i].ImageFile.empty()) {
			Map.SolidTileGraphics[i] = CGraphic::New(Map.Tileset->solidTerrainTypes[i].ImageFile, PixelTileSize.x, PixelTileSize.y);
			Map.SolidTileGraphics[i]->Load();
		}
	}
	//Wyrmgus end
	return 0;
}
/**
** Build tileset tables like humanWallTable or mixedLookupTable
**
** Called after DefineTileset and only for tilesets that have wall,
** trees and rocks. This function will be deleted when removing
** support of walls and alike in the tileset.
*/
static int CclBuildTilesetTables(lua_State *l)
{
	LuaCheckArgs(l, 0);

	Map.Tileset->buildTable(l);
	return 0;
}
/**
**  Set the flags like "water" for a tile of a tileset
**
**  @param l  Lua state.
*/
static int CclSetTileFlags(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "No flags defined");
	}
	const unsigned int tilenumber = LuaToNumber(l, 1);

	if (tilenumber >= Map.Tileset->tiles.size()) {
		LuaError(l, "Accessed a tile that's not defined");
	}
	int j = 0;
	int flags = 0;

	ParseTilesetTileFlags(l, &flags, &j);
	Map.Tileset->tiles[tilenumber].flag = flags;
	return 0;
}

//Wyrmgus start
/**
**  Get the ident of the current tileset.
**
**  @param l  Lua state.
**
**  @return   The name of the terrain of the tile.
*/
static int CclGetCurrentTileset(lua_State *l)
{
	const CTileset &tileset = *Map.Tileset;
	lua_pushstring(l, tileset.Ident.c_str());
	return 1;
}
//Wyrmgus end

/**
**  Get the name of the terrain of the tile.
**
**  @param l  Lua state.
**
**  @return   The name of the terrain of the tile.
*/
static int CclGetTileTerrainName(lua_State *l)
{
	//Wyrmgus start
//	LuaCheckArgs(l, 2);
	int z = 0;
	const int nargs = lua_gettop(l);
	if (nargs >= 3) {
		z = LuaToNumber(l, 3);
	}
	//Wyrmgus end

	const Vec2i pos(LuaToNumber(l, 1), LuaToNumber(l, 2));

	//Wyrmgus start
	/*
	const CMapField &mf = *Map.Field(pos);
	const CTileset &tileset = *Map.Tileset;
	const int index = tileset.findTileIndexByTile(mf.getGraphicTile());
	Assert(index != -1);
	const int baseTerrainIdx = tileset.tiles[index].tileinfo.BaseTerrain;

	lua_pushstring(l, tileset.getTerrainName(baseTerrainIdx).c_str());
	*/
	lua_pushstring(l, Map.GetTileTopTerrain(pos, false, z)->Ident.c_str());
	//Wyrmgus end
	return 1;
}

/**
**  Get the name of the mixed terrain of the tile.
**
**  @param l  Lua state.
**
**  @return   The name of the terrain of the tile.
*/
//Wyrmgus start
/*
static int CclGetTileTerrainMixedName(lua_State *l)
{
	LuaCheckArgs(l, 2);

	const Vec2i pos(LuaToNumber(l, 1), LuaToNumber(l, 2));

	const CMapField &mf = *Map.Field(pos);
	const CTileset &tileset = *Map.Tileset;
	//Wyrmgus start
//	const int index = tileset.findTileIndexByTile(mf.getGraphicTile());
	const int index = mf.getTileIndex();
	//Wyrmgus end
	Assert(index != -1);
	const int mixTerrainIdx = tileset.tiles[index].tileinfo.MixTerrain;

	lua_pushstring(l, mixTerrainIdx ? tileset.getTerrainName(mixTerrainIdx).c_str() : "");
	return 1;
}
*/
//Wyrmgus end

/**
**  Check if the tile's terrain has a particular flag.
**
**  @param l  Lua state.
**
**  @return   True if has the flag, false if not.
*/
static int CclGetTileTerrainHasFlag(lua_State *l)
{
	//Wyrmgus start
//	LuaCheckArgs(l, 3);
	int z = 0;
	const int nargs = lua_gettop(l);
	if (nargs >= 4) {
		z = LuaToNumber(l, 4);
	}
	//Wyrmgus end

	const Vec2i pos(LuaToNumber(l, 1), LuaToNumber(l, 2));

	//Wyrmgus start
	if (pos.x < 0 || pos.x >= Map.Info.MapWidths[z] || pos.y < 0 || pos.y >= Map.Info.MapHeights[z]) {
		lua_pushboolean(l, 0);
		return 1;
	}
	
//	unsigned short flag = 0;
	unsigned long flag = 0;
	//Wyrmgus end
	const char *flag_name = LuaToString(l, 3);
	if (!strcmp(flag_name, "water")) {
		flag = MapFieldWaterAllowed;
	} else if (!strcmp(flag_name, "land")) {
		flag = MapFieldLandAllowed;
	} else if (!strcmp(flag_name, "coast")) {
		flag = MapFieldCoastAllowed;
	} else if (!strcmp(flag_name, "no-building")) {
		flag = MapFieldNoBuilding;
	} else if (!strcmp(flag_name, "unpassable")) {
		flag = MapFieldUnpassable;
	//Wyrmgus start
	} else if (!strcmp(flag_name, "air-unpassable")) {
		flag = MapFieldAirUnpassable;
	} else if (!strcmp(flag_name, "dirt")) {
		flag = MapFieldDirt;
	} else if (!strcmp(flag_name, "grass")) {
		flag = MapFieldGrass;
	} else if (!strcmp(flag_name, "gravel")) {
		flag = MapFieldGravel;
	} else if (!strcmp(flag_name, "mud")) {
		flag = MapFieldMud;
	} else if (!strcmp(flag_name, "stone-floor")) {
		flag = MapFieldStoneFloor;
	} else if (!strcmp(flag_name, "stumps")) {
		flag = MapFieldStumps;
	//Wyrmgus end
	} else if (!strcmp(flag_name, "wall")) {
		flag = MapFieldWall;
	} else if (!strcmp(flag_name, "rock")) {
		flag = MapFieldRocks;
	} else if (!strcmp(flag_name, "forest")) {
		flag = MapFieldForest;
	}

	//Wyrmgus start
//	const CMapField &mf = *Map.Field(pos);
	const CMapField &mf = *Map.Field(pos, z);
	//Wyrmgus end

	if (mf.getFlag() & flag) {
		lua_pushboolean(l, 1);
	} else {
		lua_pushboolean(l, 0);
	}

	return 1;
}

//Wyrmgus start
/**
**  Define a terrain type.
**
**  @param l  Lua state.
*/
static int CclDefineTerrainType(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string terrain_ident = LuaToString(l, 1);
	CTerrainType *terrain = GetTerrainType(terrain_ident);
	if (terrain == NULL) {
		terrain = new CTerrainType;
		terrain->Ident = terrain_ident;
		terrain->ID = TerrainTypes.size();
		TerrainTypes.push_back(terrain);
		TerrainTypeStringToIndex[terrain_ident] = terrain->ID;
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			terrain->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Character")) {
			terrain->Character = LuaToString(l, -1);
			if (TerrainTypeCharacterToPointer.find(terrain->Character) != TerrainTypeCharacterToPointer.end()) {
				LuaError(l, "Character \"%s\" is already used by another terrain type." _C_ terrain->Character.c_str());
			}
			TerrainTypeCharacterToPointer[terrain->Character] = terrain;
		} else if (!strcmp(value, "Overlay")) {
			terrain->Overlay = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Buildable")) {
			terrain->Buildable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "AllowSingle")) {
			terrain->AllowSingle = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "SolidAnimationFrames")) {
			terrain->SolidAnimationFrames = LuaToNumber(l, -1);
		} else if (!strcmp(value, "BaseTerrains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CTerrainType *base_terrain = GetTerrainType(LuaToString(l, -1, j + 1));
				if (base_terrain == NULL) {
					LuaError(l, "Terrain doesn't exist.");
				}
				terrain->BaseTerrains.push_back(base_terrain);
			}
		} else if (!strcmp(value, "InnerBorderTerrains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CTerrainType *border_terrain = GetTerrainType(LuaToString(l, -1, j + 1));
				if (border_terrain == NULL) {
					LuaError(l, "Terrain doesn't exist.");
				}
				terrain->InnerBorderTerrains.push_back(border_terrain);
				terrain->BorderTerrains.push_back(border_terrain);
				border_terrain->OuterBorderTerrains.push_back(terrain);
				border_terrain->BorderTerrains.push_back(terrain);
			}
		} else if (!strcmp(value, "OuterBorderTerrains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CTerrainType *border_terrain = GetTerrainType(LuaToString(l, -1, j + 1));
				if (border_terrain == NULL) {
					LuaError(l, "Terrain doesn't exist.");
				}
				terrain->OuterBorderTerrains.push_back(border_terrain);
				terrain->BorderTerrains.push_back(border_terrain);
				border_terrain->InnerBorderTerrains.push_back(terrain);
				border_terrain->BorderTerrains.push_back(terrain);
			}
		} else if (!strcmp(value, "Flags")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			terrain->Flags = 0;
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string tile_flag = LuaToString(l, -1, j + 1);
				if (tile_flag == "land") {
					terrain->Flags |= MapFieldLandAllowed;
				} else if (tile_flag == "coast") {
					terrain->Flags |= MapFieldCoastAllowed;
				} else if (tile_flag == "water") {
					terrain->Flags |= MapFieldWaterAllowed;
				} else if (tile_flag == "no-building") {
					terrain->Flags |= MapFieldNoBuilding;
				} else if (tile_flag == "unpassable") {
					terrain->Flags |= MapFieldUnpassable;
				} else if (tile_flag == "wall") {
					terrain->Flags |= MapFieldWall;
				} else if (tile_flag == "rock") {
					terrain->Flags |= MapFieldRocks;
				} else if (tile_flag == "forest") {
					terrain->Flags |= MapFieldForest;
				} else if (tile_flag == "air-unpassable") {
					terrain->Flags |= MapFieldAirUnpassable;
				} else if (tile_flag == "dirt") {
					terrain->Flags |= MapFieldDirt;
				} else if (tile_flag == "grass") {
					terrain->Flags |= MapFieldGrass;
				} else if (tile_flag == "gravel") {
					terrain->Flags |= MapFieldGravel;
				} else if (tile_flag == "mud") {
					terrain->Flags |= MapFieldMud;
				} else if (tile_flag == "stone-floor") {
					terrain->Flags |= MapFieldStoneFloor;
				} else if (tile_flag == "stumps") {
					terrain->Flags |= MapFieldStumps;
				} else {
					LuaError(l, "Flag \"%s\" doesn't exist." _C_ tile_flag.c_str());
				}
			}
		} else if (!strcmp(value, "Graphics")) {
			std::string graphics_file = LuaToString(l, -1);
			if (!CanAccessFile(graphics_file.c_str())) {
				LuaError(l, "File \"%s\" doesn't exist." _C_ graphics_file.c_str());
			}
			if (CGraphic::Get(graphics_file) == NULL) {
				CGraphic *graphics = CGraphic::New(graphics_file, 32, 32);
			}
			terrain->Graphics = CGraphic::Get(graphics_file);
		} else if (!strcmp(value, "SolidTiles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				terrain->SolidTiles.push_back(LuaToNumber(l, -1, j + 1));
			}
		} else if (!strcmp(value, "DamagedTiles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				terrain->DamagedTiles.push_back(LuaToNumber(l, -1, j + 1));
			}
		} else if (!strcmp(value, "DestroyedTiles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				terrain->DestroyedTiles.push_back(LuaToNumber(l, -1, j + 1));
			}
		} else if (!strcmp(value, "TransitionTiles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string transition_terrain_name = LuaToString(l, -1, j + 1);
				CTerrainType *transition_terrain = GetTerrainType(transition_terrain_name);
				if (transition_terrain == NULL && transition_terrain_name != "any") {
					LuaError(l, "Terrain doesn't exist.");
				}
				int transition_terrain_id = transition_terrain_name == "any" ? -1 : transition_terrain->ID;
				++j;
				
				int transition_type = GetTransitionTypeIdByName(LuaToString(l, -1, j + 1));
				if (transition_type == -1) {
					LuaError(l, "Transition type doesn't exist.");
				}
				++j;
				
				terrain->TransitionTiles[std::tuple<int, int>(transition_terrain_id, transition_type)].push_back(LuaToNumber(l, -1, j + 1));
			}
		} else if (!strcmp(value, "AdjacentTransitionTiles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string transition_terrain_name = LuaToString(l, -1, j + 1);
				CTerrainType *transition_terrain = GetTerrainType(transition_terrain_name);
				if (transition_terrain == NULL && transition_terrain_name != "any") {
					LuaError(l, "Terrain doesn't exist.");
				}
				int transition_terrain_id = transition_terrain_name == "any" ? -1 : transition_terrain->ID;
				++j;
				
				int transition_type = GetTransitionTypeIdByName(LuaToString(l, -1, j + 1));
				if (transition_type == -1) {
					LuaError(l, "Transition type doesn't exist.");
				}
				++j;
				
				terrain->AdjacentTransitionTiles[std::tuple<int, int>(transition_terrain_id, transition_type)].push_back(LuaToNumber(l, -1, j + 1));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a map template.
**
**  @param l  Lua state.
*/
static int CclDefineMapTemplate(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string map_ident = LuaToString(l, 1);
	CMapTemplate *map = GetMapTemplate(map_ident);
	if (map == NULL) {
		map = new CMapTemplate;
		map->Ident = map_ident;
		MapTemplates.push_back(map);
		MapTemplateIdentToPointer[map_ident] = map;
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			map->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Plane")) {
			CPlane *plane = GetPlane(LuaToString(l, -1));
			if (!plane) {
				LuaError(l, "Plane doesn't exist.");
			}
			map->Plane = plane;
		} else if (!strcmp(value, "World")) {
			CWorld *world = GetWorld(LuaToString(l, -1));
			if (!world) {
				LuaError(l, "World doesn't exist.");
			}
			map->World = world;
		} else if (!strcmp(value, "TerrainFile")) {
			map->TerrainFile = LuaToString(l, -1);
		} else if (!strcmp(value, "OverlayTerrainFile")) {
			map->OverlayTerrainFile = LuaToString(l, -1);
		} else if (!strcmp(value, "Width")) {
			map->Width = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Height")) {
			map->Height = LuaToNumber(l, -1);
		} else if (!strcmp(value, "TimeOfDaySeconds")) {
			map->TimeOfDaySeconds = LuaToNumber(l, -1);
		} else if (!strcmp(value, "MainTemplate")) {
			CMapTemplate *main_template = GetMapTemplate(LuaToString(l, -1));
			if (!main_template) {
				LuaError(l, "Map template doesn't exist.");
			}
			map->MainTemplate = main_template;
			main_template->Subtemplates.push_back(map);
		} else if (!strcmp(value, "BaseTerrain")) {
			CTerrainType *terrain = GetTerrainType(LuaToString(l, -1));
			if (!terrain) {
				LuaError(l, "Terrain doesn't exist.");
			}
			map->BaseTerrain = terrain;
		} else if (!strcmp(value, "SurroundingTerrain")) {
			CTerrainType *terrain = GetTerrainType(LuaToString(l, -1));
			if (!terrain) {
				LuaError(l, "Terrain doesn't exist.");
			}
			map->SurroundingTerrain = terrain;
		} else if (!strcmp(value, "GeneratedTerrains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CTerrainType *terrain = GetTerrainType(LuaToString(l, -1, j + 1));
				if (!terrain) {
					LuaError(l, "Terrain doesn't exist.");
				}
				++j;
				
				int degree_level = GetDegreeLevelIdByName(LuaToString(l, -1, j + 1));
				if (degree_level == -1) {
					LuaError(l, "Degree level doesn't exist.");
				}
				
				map->GeneratedTerrains.push_back(std::pair<CTerrainType *, int>(terrain, degree_level));
			}
		} else if (!strcmp(value, "PlayerLocationGeneratedTerrains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CTerrainType *terrain = GetTerrainType(LuaToString(l, -1, j + 1));
				if (!terrain) {
					LuaError(l, "Terrain doesn't exist.");
				}
				++j;
				
				int degree_level = GetDegreeLevelIdByName(LuaToString(l, -1, j + 1));
				if (degree_level == -1) {
					LuaError(l, "Degree level doesn't exist.");
				}
				
				map->PlayerLocationGeneratedTerrains.push_back(std::pair<CTerrainType *, int>(terrain, degree_level));
			}
		} else if (!strcmp(value, "ExternalGeneratedTerrains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CTerrainType *terrain = GetTerrainType(LuaToString(l, -1, j + 1));
				if (!terrain) {
					LuaError(l, "Terrain doesn't exist.");
				}
				++j;
				
				int degree_level = GetDegreeLevelIdByName(LuaToString(l, -1, j + 1));
				if (degree_level == -1) {
					LuaError(l, "Degree level doesn't exist.");
				}
				
				map->ExternalGeneratedTerrains.push_back(std::pair<CTerrainType *, int>(terrain, degree_level));
			}
		} else if (!strcmp(value, "GeneratedNeutralUnits")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CUnitType *unit_type = UnitTypeByIdent(LuaToString(l, -1, j + 1));
				if (!unit_type) {
					LuaError(l, "Unit type doesn't exist.");
				}
				++j;
				
				int quantity = LuaToNumber(l, -1, j + 1);
				
				map->GeneratedNeutralUnits.push_back(std::pair<CUnitType *, int>(unit_type, quantity));
			}
		} else if (!strcmp(value, "PlayerLocationGeneratedNeutralUnits")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CUnitType *unit_type = UnitTypeByIdent(LuaToString(l, -1, j + 1));
				if (!unit_type) {
					LuaError(l, "Unit type doesn't exist.");
				}
				++j;
				
				int quantity = LuaToNumber(l, -1, j + 1);
				
				map->PlayerLocationGeneratedNeutralUnits.push_back(std::pair<CUnitType *, int>(unit_type, quantity));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	for (int i = 0; i < map->Width * map->Height; ++i) {
		map->TileTerrains.push_back(map->BaseTerrain);
		map->TileOverlayTerrains.push_back(NULL);
	}
	
	map->ParseTerrainFile(false);
	map->ParseTerrainFile(true);
	
	return 0;
}
//Wyrmgus end

/**
**  Register CCL features for map.
*/
void MapCclRegister()
{
	lua_register(Lua, "StratagusMap", CclStratagusMap);
	lua_register(Lua, "RevealMap", CclRevealMap);
	lua_register(Lua, "CenterMap", CclCenterMap);
	lua_register(Lua, "SetStartView", CclSetStartView);
	lua_register(Lua, "ShowMapLocation", CclShowMapLocation);

	lua_register(Lua, "SetFogOfWar", CclSetFogOfWar);
	lua_register(Lua, "GetFogOfWar", CclGetFogOfWar);
	lua_register(Lua, "SetMinimapTerrain", CclSetMinimapTerrain);

	lua_register(Lua, "SetFogOfWarGraphics", CclSetFogOfWarGraphics);
	lua_register(Lua, "SetFogOfWarOpacity", CclSetFogOfWarOpacity);
	lua_register(Lua, "SetFogOfWarColor", CclSetFogOfWarColor);

	lua_register(Lua, "SetForestRegeneration", CclSetForestRegeneration);

	lua_register(Lua, "LoadTileModels", CclLoadTileModels);
	lua_register(Lua, "DefinePlayerTypes", CclDefinePlayerTypes);

	lua_register(Lua, "DefineTileset", CclDefineTileset);
	lua_register(Lua, "SetTileFlags", CclSetTileFlags);
	lua_register(Lua, "BuildTilesetTables", CclBuildTilesetTables);

	//Wyrmgus start
	lua_register(Lua, "GetCurrentTileset", CclGetCurrentTileset);
	//Wyrmgus end
	lua_register(Lua, "GetTileTerrainName", CclGetTileTerrainName);
	//Wyrmgus start
//	lua_register(Lua, "GetTileTerrainMixedName", CclGetTileTerrainMixedName);
	//Wyrmgus end
	lua_register(Lua, "GetTileTerrainHasFlag", CclGetTileTerrainHasFlag);
	
	//Wyrmgus start
	lua_register(Lua, "DefineTerrainType", CclDefineTerrainType);
	lua_register(Lua, "DefineMapTemplate", CclDefineMapTemplate);
	lua_register(Lua, "SetMapTemplateTileLabel", CclSetMapTemplateTileLabel);
	lua_register(Lua, "SetMapTemplateResource", CclSetMapTemplateResource);
	lua_register(Lua, "SetMapTemplateUnit", CclSetMapTemplateUnit);
	lua_register(Lua, "SetMapTemplateLayerConnector", CclSetMapTemplateLayerConnector);
	//Wyrmgus end
}

//@}
