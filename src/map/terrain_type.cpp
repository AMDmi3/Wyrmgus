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
//      (c) Copyright 2018-2020 by Andrettin
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

#include "map/terrain_type.h"

#include "config.h"
#include "database/defines.h"
#include "iolib.h"
#include "map/map.h"
#include "map/tileset.h"
#include "time/season.h"
#include "upgrade/upgrade_structs.h"
#include "util/string_util.h"
#include "video.h"

namespace stratagus {

std::map<std::string, terrain_type *> terrain_type::TerrainTypesByCharacter;
std::map<std::tuple<int, int, int>, terrain_type *> terrain_type::TerrainTypesByColor;

/**
**	@brief	Load the graphics of the terrain types
*/
void terrain_type::LoadTerrainTypeGraphics()
{
	for (terrain_type *terrain_type : terrain_type::get_all()) {
		if (terrain_type->Graphics) {
			terrain_type->Graphics->Load();
		}
		for (std::map<const CSeason *, CGraphic *>::iterator sub_it = terrain_type->SeasonGraphics.begin(); sub_it != terrain_type->SeasonGraphics.end(); ++sub_it) {
			sub_it->second->Load();
		}
		if (terrain_type->ElevationGraphics) {
			terrain_type->ElevationGraphics->Load();
		}
		if (terrain_type->PlayerColorGraphics) {
			terrain_type->PlayerColorGraphics->Load();
		}
	}
}

/**
**	@brief	Get a terrain flag by name
**
**	@param	flag_name	The name of the terrain flag
**
**	@return	The terrain flag if it exists, or 0 otherwise
*/
unsigned long terrain_type::GetTerrainFlagByName(const std::string &flag_name)
{
	if (flag_name == "land") {
		return MapFieldLandAllowed;
	} else if (flag_name == "coast") {
		return MapFieldCoastAllowed;
	} else if (flag_name == "water") {
		return MapFieldWaterAllowed;
	} else if (flag_name == "no-building") {
		return MapFieldNoBuilding;
	} else if (flag_name == "unpassable") {
		return MapFieldUnpassable;
	} else if (flag_name == "wall") {
		return MapFieldWall;
	} else if (flag_name == "rock") {
		return MapFieldRocks;
	} else if (flag_name == "forest") {
		return MapFieldForest;
	} else if (flag_name == "air-unpassable") {
		return MapFieldAirUnpassable;
	} else if (flag_name == "desert") {
		return MapFieldDesert;
	} else if (flag_name == "dirt") {
		return MapFieldDirt;
	} else if (flag_name == "grass") {
		return MapFieldGrass;
	} else if (flag_name == "gravel") {
		return MapFieldGravel;
	} else if (flag_name == "ice") {
		return MapFieldIce;
	} else if (flag_name == "mud") {
		return MapFieldMud;
	} else if (flag_name == "railroad") {
		return MapFieldRailroad;
	} else if (flag_name == "road") {
		return MapFieldRoad;
	} else if (flag_name == "no-rail") {
		return MapFieldNoRail;
	} else if (flag_name == "snow") {
		return MapFieldSnow;
	} else if (flag_name == "stone-floor") {
		return MapFieldStoneFloor;
	} else if (flag_name == "stumps") {
		return MapFieldStumps;
	} else if (flag_name == "underground") {
		return MapFieldUnderground;
	} else {
		fprintf(stderr, "Flag \"%s\" doesn't exist.\n", flag_name.c_str());
		return 0;
	}
}

/**
**	@brief	Destructor
*/
terrain_type::~terrain_type()
{
	if (this->Graphics) {
		CGraphic::Free(this->Graphics);
	}
	for (std::map<const CSeason *, CGraphic *>::iterator iterator = this->SeasonGraphics.begin(); iterator != this->SeasonGraphics.end(); ++iterator) {
		CGraphic::Free(iterator->second);
	}
	if (this->ElevationGraphics) {
		CGraphic::Free(this->ElevationGraphics);
	}
	if (this->PlayerColorGraphics) {
		CPlayerColorGraphic::Free(this->PlayerColorGraphics);
	}
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void terrain_type::ProcessConfigData(const CConfigData *config_data)
{
	std::string graphics_file;
	std::string elevation_graphics_file;
	std::string player_color_graphics_file;
	
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->set_name(value);
		} else if (key == "character") {
			this->Character = value;
			
			if (terrain_type::TerrainTypesByCharacter.find(this->Character) != terrain_type::TerrainTypesByCharacter.end()) {
				fprintf(stderr, "Character \"%s\" is already used by another terrain type.\n", this->Character.c_str());
				continue;
			} else {
				terrain_type::TerrainTypesByCharacter[this->Character] = this;
			}
		} else if (key == "character_alias") {
			if (terrain_type::TerrainTypesByCharacter.find(value) != terrain_type::TerrainTypesByCharacter.end()) {
				fprintf(stderr, "Character \"%s\" is already used by another terrain type.\n", value.c_str());
				continue;
			} else {
				terrain_type::TerrainTypesByCharacter[value] = this;
			}
		} else if (key == "color") {
			this->Color = CColor::FromString(value);
			
			if (terrain_type::TerrainTypesByColor.find(std::tuple<int, int, int>(this->Color.R, this->Color.G, this->Color.B)) != terrain_type::TerrainTypesByColor.end()) {
				fprintf(stderr, "Color is already used by another terrain type.\n");
				continue;
			}
			if (TerrainFeatureColorToIndex.find(std::tuple<int, int, int>(this->Color.R, this->Color.G, this->Color.B)) != TerrainFeatureColorToIndex.end()) {
				fprintf(stderr, "Color is already used by a terrain feature.\n");
				continue;
			}
			terrain_type::TerrainTypesByColor[std::tuple<int, int, int>(this->Color.R, this->Color.G, this->Color.B)] = this;
		} else if (key == "overlay") {
			this->Overlay = string::to_bool(value);
		} else if (key == "buildable") {
			this->Buildable = string::to_bool(value);
		} else if (key == "allow_single") {
			this->AllowSingle = string::to_bool(value);
		} else if (key == "hidden") {
			this->Hidden = string::to_bool(value);
		} else if (key == "resource") {
			value = FindAndReplaceString(value, "_", "-");
			this->Resource = GetResourceIdByName(value.c_str());
		} else if (key == "flag") {
			value = FindAndReplaceString(value, "_", "-");
			unsigned long flag = terrain_type::GetTerrainFlagByName(value);
			if (flag) {
				this->Flags |= flag;
			}
		} else if (key == "graphics") {
			graphics_file = value;
			if (!CanAccessFile(graphics_file.c_str())) {
				fprintf(stderr, "File \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "elevation_graphics") {
			elevation_graphics_file = value;
			if (!CanAccessFile(elevation_graphics_file.c_str())) {
				fprintf(stderr, "File \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "player_color_graphics") {
			player_color_graphics_file = value;
			if (!CanAccessFile(player_color_graphics_file.c_str())) {
				fprintf(stderr, "File \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "base_terrain_type") {
			terrain_type *base_terrain_type = terrain_type::get(value);
			this->BaseTerrainTypes.push_back(base_terrain_type);
		} else if (key == "inner_border_terrain_type") {
			terrain_type *border_terrain_type = terrain_type::get(value);
			this->InnerBorderTerrains.push_back(border_terrain_type);
			this->BorderTerrains.push_back(border_terrain_type);
			border_terrain_type->OuterBorderTerrains.push_back(this);
			border_terrain_type->BorderTerrains.push_back(this);
		} else if (key == "outer_border_terrain_type") {
			terrain_type *border_terrain_type = terrain_type::get(value);
			this->OuterBorderTerrains.push_back(border_terrain_type);
			this->BorderTerrains.push_back(border_terrain_type);
			border_terrain_type->InnerBorderTerrains.push_back(this);
			border_terrain_type->BorderTerrains.push_back(this);
		} else if (key == "solid_tile") {
			this->SolidTiles.push_back(std::stoi(value));
		} else if (key == "damaged_tile") {
			this->DamagedTiles.push_back(std::stoi(value));
		} else if (key == "destroyed_tile") {
			this->DestroyedTiles.push_back(std::stoi(value));
		} else {
			fprintf(stderr, "Invalid terrain type property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "season_graphics") {
			std::string season_graphics_file;
			CSeason *season = nullptr;
			
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "season") {
					value = FindAndReplaceString(value, "_", "-");
					season = CSeason::GetSeason(value);
				} else if (key == "graphics") {
					season_graphics_file = value;
					if (!CanAccessFile(season_graphics_file.c_str())) {
						fprintf(stderr, "File \"%s\" doesn't exist.\n", value.c_str());
					}
				} else {
					fprintf(stderr, "Invalid season graphics property: \"%s\".\n", key.c_str());
				}
			}
			
			if (season_graphics_file.empty()) {
				fprintf(stderr, "Season graphics have no file.\n");
				continue;
			}
			
			if (!season) {
				fprintf(stderr, "Season graphics have no season.\n");
				continue;
			}
			
			if (CGraphic::Get(season_graphics_file) == nullptr) {
				CGraphic *graphics = CGraphic::New(season_graphics_file, defines::get()->get_tile_width(), defines::get()->get_tile_height());
			}
			this->SeasonGraphics[season] = CGraphic::Get(season_graphics_file);
		} else if (child_config_data->Tag == "transition_tile" || child_config_data->Tag == "adjacent_transition_tile") {
			int transition_terrain_id = -1; //any terrain, by default
			int transition_type = -1;
			std::vector<int> tiles;
			
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "terrain_type") {
					terrain_type *transition_terrain = terrain_type::get(value);
					transition_terrain_id = transition_terrain->ID;
				} else if (key == "transition_type") {
					value = FindAndReplaceString(value, "_", "-");
					transition_type = GetTransitionTypeIdByName(value);
				} else if (key == "tile") {
					tiles.push_back(std::stoi(value));
				} else {
					fprintf(stderr, "Invalid transition tile property: \"%s\".\n", key.c_str());
				}
			}
			
			if (transition_type == -1) {
				fprintf(stderr, "Transition tile has no transition type.\n");
				continue;
			}
			
			for (size_t j = 0; j < tiles.size(); ++j) {
				if (child_config_data->Tag == "transition_tile") {
					this->TransitionTiles[std::tuple<int, int>(transition_terrain_id, transition_type)].push_back(tiles[j]);
				} else if (child_config_data->Tag == "adjacent_transition_tile") {
					this->AdjacentTransitionTiles[std::tuple<int, int>(transition_terrain_id, transition_type)].push_back(tiles[j]);
				}
			}
		} else {
			fprintf(stderr, "Invalid terrain type property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
	
	if (!graphics_file.empty()) {
		if (CGraphic::Get(graphics_file) == nullptr) {
			CGraphic *graphics = CGraphic::New(graphics_file, defines::get()->get_tile_width(), defines::get()->get_tile_height());
		}
		this->Graphics = CGraphic::Get(graphics_file);
	}
	if (!elevation_graphics_file.empty()) {
		if (CGraphic::Get(elevation_graphics_file) == nullptr) {
			CGraphic *graphics = CGraphic::New(elevation_graphics_file, defines::get()->get_tile_width(), defines::get()->get_tile_height());
		}
		this->ElevationGraphics = CGraphic::Get(elevation_graphics_file);
	}
	if (!player_color_graphics_file.empty()) {
		if (CPlayerColorGraphic::Get(player_color_graphics_file) == nullptr) {
			CPlayerColorGraphic *graphics = CPlayerColorGraphic::New(player_color_graphics_file, defines::get()->get_tile_width(), defines::get()->get_tile_height());
		}
		this->PlayerColorGraphics = CPlayerColorGraphic::Get(player_color_graphics_file);
	}
}

/**
**	@brief	Get the graphics for the terrain type
**
**	@param	season	The season for the graphics, if any
**
**	@return	The graphics
*/
CGraphic *terrain_type::GetGraphics(const CSeason *season) const
{
	std::map<const CSeason *, CGraphic *>::const_iterator find_iterator = this->SeasonGraphics.find(season);
	
	if (find_iterator != this->SeasonGraphics.end()) {
		return find_iterator->second;
	} else {
		return this->Graphics;
	}
}

}