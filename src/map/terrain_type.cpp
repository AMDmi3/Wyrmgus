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
#include "util/container_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"
#include "video.h"

namespace stratagus {

void terrain_type::LoadTerrainTypeGraphics()
{
	for (terrain_type *terrain_type : terrain_type::get_all()) {
		if (terrain_type->graphics) {
			terrain_type->graphics->Load(false, defines::get()->get_scale_factor());
		}
		if (terrain_type->transition_graphics) {
			terrain_type->transition_graphics->Load(false, defines::get()->get_scale_factor());
		}
		for (const auto &kv_pair : terrain_type->season_graphics) {
			kv_pair.second->Load(false, defines::get()->get_scale_factor());
		}
		if (terrain_type->ElevationGraphics) {
			terrain_type->ElevationGraphics->Load(false, defines::get()->get_scale_factor());
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
		throw std::runtime_error("Flag \"" + flag_name + "\" doesn't exist.");
	}
}

/**
**	@brief	Destructor
*/
terrain_type::~terrain_type()
{
	if (this->graphics) {
		CGraphic::Free(this->graphics);
	}
	if (this->transition_graphics) {
		CGraphic::Free(this->transition_graphics);
	}
	for (const auto &kv_pair : this->season_graphics) {
		CGraphic::Free(kv_pair.second);
	}
	if (this->ElevationGraphics) {
		CGraphic::Free(this->ElevationGraphics);
	}
}

void terrain_type::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "character") {
		this->set_character(value.front());
	} else {
		data_entry::process_sml_property(property);
	}
}

void terrain_type::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "character_aliases") {
		for (const std::string &value : values) {
			this->map_to_character(value.front());
		}
	} else if (tag == "flags") {
		for (const std::string &value : values) {
			const unsigned long flag = terrain_type::GetTerrainFlagByName(value);
			this->Flags |= flag;
		}
	} else if (tag == "solid_tiles") {
		for (const std::string &value : values) {
			this->solid_tiles.push_back(std::stoi(value));
		}
	} else if (tag == "damaged_tiles") {
		for (const std::string &value : values) {
			this->damaged_tiles.push_back(std::stoi(value));
		}
	} else if (tag == "destroyed_tiles") {
		for (const std::string &value : values) {
			this->destroyed_tiles.push_back(std::stoi(value));
		}
	} else if (tag == "season_image_files") {
		scope.for_each_property([&](const sml_property &property) {
			const season *season = season::get(property.get_key());
			const std::filesystem::path filepath = property.get_value();

			this->season_image_files[season] = filepath;
		});
	} else if (tag == "transition_tiles" || tag == "adjacent_transition_tiles") {
		scope.for_each_child([&](const sml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const terrain_type *transition_terrain = nullptr;

			if (child_tag != "any") {
				transition_terrain = terrain_type::get(child_tag);
			}

			child_scope.for_each_child([&](const sml_data &grandchild_scope) {
				const std::string &grandchild_tag = grandchild_scope.get_tag();
				const tile_transition_type transition_type = GetTransitionTypeIdByName(FindAndReplaceString(grandchild_tag, "_", "-"));
				std::vector<int> tiles;

				for (const std::string &value : grandchild_scope.get_values()) {
					const int tile = std::stoi(value);

					if (tag == "transition_tiles") {
						this->transition_tiles[transition_terrain][transition_type].push_back(tile);
					} else if (tag == "adjacent_transition_tiles") {
						this->adjacent_transition_tiles[transition_terrain][transition_type].push_back(tile);
					}
				}
			});
		});
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void terrain_type::ProcessConfigData(const CConfigData *config_data)
{
	std::string graphics_file;
	std::string elevation_graphics_file;
	
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->set_name(value);
		} else if (key == "character") {
			this->set_character(value.front());
		} else if (key == "character_alias") {
			const char c = value.front();
			this->map_to_character(c);
		} else if (key == "color") {
			const CColor color = CColor::FromString(value);
			this->set_color(QColor(color.R, color.G, color.B));
		} else if (key == "overlay") {
			this->overlay = string::to_bool(value);
		} else if (key == "buildable") {
			this->Buildable = string::to_bool(value);
		} else if (key == "allow_single") {
			this->AllowSingle = string::to_bool(value);
		} else if (key == "hidden") {
			this->Hidden = string::to_bool(value);
		} else if (key == "resource") {
			this->resource = resource::get(value);
		} else if (key == "flag") {
			value = FindAndReplaceString(value, "_", "-");
			const unsigned long flag = terrain_type::GetTerrainFlagByName(value);
			this->Flags |= flag;
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
		} else if (key == "base_terrain_type") {
			terrain_type *base_terrain_type = terrain_type::get(value);
			this->add_base_terrain_type(base_terrain_type);
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
			this->solid_tiles.push_back(std::stoi(value));
		} else if (key == "damaged_tile") {
			this->damaged_tiles.push_back(std::stoi(value));
		} else if (key == "destroyed_tile") {
			this->destroyed_tiles.push_back(std::stoi(value));
		} else {
			fprintf(stderr, "Invalid terrain type property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "season_graphics") {
			std::string season_graphics_file;
			season *season = nullptr;
			
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "season") {
					season = season::get(value);
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
			
			this->season_graphics[season] = CPlayerColorGraphic::New(season_graphics_file, defines::get()->get_tile_size());
		} else if (child_config_data->Tag == "transition_tile" || child_config_data->Tag == "adjacent_transition_tile") {
			const terrain_type *transition_terrain = nullptr; //any terrain, by default
			tile_transition_type transition_type = tile_transition_type::none;
			std::vector<int> tiles;
			
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "terrain_type") {
					transition_terrain = terrain_type::get(value);
				} else if (key == "transition_type") {
					value = FindAndReplaceString(value, "_", "-");
					transition_type = GetTransitionTypeIdByName(value);
				} else if (key == "tile") {
					tiles.push_back(std::stoi(value));
				} else {
					fprintf(stderr, "Invalid transition tile property: \"%s\".\n", key.c_str());
				}
			}
			
			if (transition_type == tile_transition_type::none) {
				fprintf(stderr, "Transition tile has no transition type.\n");
				continue;
			}

			for (size_t j = 0; j < tiles.size(); ++j) {
				if (child_config_data->Tag == "transition_tile") {
					this->transition_tiles[transition_terrain][transition_type].push_back(tiles[j]);
				} else if (child_config_data->Tag == "adjacent_transition_tile") {
					this->adjacent_transition_tiles[transition_terrain][transition_type].push_back(tiles[j]);
				}
			}
		} else {
			fprintf(stderr, "Invalid terrain type property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
	
	if (!graphics_file.empty()) {
		this->graphics = CPlayerColorGraphic::New(graphics_file, defines::get()->get_tile_size());
	}
	if (!elevation_graphics_file.empty()) {
		this->ElevationGraphics = CGraphic::New(elevation_graphics_file, defines::get()->get_tile_size());
	}
}

void terrain_type::initialize()
{
	if (!this->get_image_file().empty() && this->graphics == nullptr) {
		this->graphics = CPlayerColorGraphic::New(this->get_image_file().string(), defines::get()->get_tile_size());
	}

	if (!this->get_transition_image_file().empty() && this->transition_graphics == nullptr) {
		this->transition_graphics = CPlayerColorGraphic::New(this->get_transition_image_file().string(), defines::get()->get_tile_size());
	}

	for (const auto &kv_pair : this->season_image_files) {
		const season *season = kv_pair.first;
		const std::filesystem::path &filepath = kv_pair.second;

		if (!this->season_graphics.contains(season)) {
			this->season_graphics[season] = CPlayerColorGraphic::New(filepath.string(), defines::get()->get_tile_size());
		}
	}
}

void terrain_type::set_character(const char character)
{
	if (character == this->get_character()) {
		return;
	}

	this->character = character;
	this->map_to_character(character);
}

void terrain_type::map_to_character(const char character)
{
	if (terrain_type::try_get_by_character(character) != nullptr) {
		throw std::runtime_error("Character \"" + std::string(character, 1) + "\" is already used by another terrain type.");
	}

	terrain_type::terrain_types_by_character[character] = this;
}

void terrain_type::set_color(const QColor &color)
{
	if (color == this->get_color()) {
		return;
	}

	if (terrain_type::try_get_by_color(color) != nullptr) {
		throw std::runtime_error("Color is already used by another terrain type.");
	} else if (TerrainFeatureColorToIndex.contains(std::tuple<int, int, int>(color.red(), color.green(), color.blue()))) {
		throw std::runtime_error("Color is already used by a terrain feature.");
	}

	this->color = color;
	terrain_type::terrain_types_by_color[color] = this;
}

void terrain_type::set_image_file(const std::filesystem::path &filepath)
{
	if (filepath == this->get_image_file()) {
		return;
	}

	this->image_file = database::get_graphics_path(this->get_module()) / filepath;
}

void terrain_type::set_transition_image_file(const std::filesystem::path &filepath)
{
	if (filepath == this->get_transition_image_file()) {
		return;
	}

	this->transition_image_file = database::get_graphics_path(this->get_module()) / filepath;
}

CPlayerColorGraphic *terrain_type::get_graphics(const season *season) const
{
	auto find_iterator = this->season_graphics.find(season);
	
	if (find_iterator != this->season_graphics.end()) {
		return find_iterator->second;
	} else {
		return this->graphics;
	}
}

QVariantList terrain_type::get_base_terrain_types_qvariant_list() const
{
	return container::to_qvariant_list(this->get_base_terrain_types());
}

void terrain_type::remove_base_terrain_type(terrain_type *terrain_type)
{
	vector::remove(this->base_terrain_types, terrain_type);
}

}