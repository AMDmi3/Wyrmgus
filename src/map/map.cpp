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
/**@name map.cpp - The map. */
//
//      (c) Copyright 1998-2006 by Lutz Sammer, Vladi Shabanski and
//                                 Francois Beerten
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
#include "game.h" // for the SaveGameLoading variable
//Wyrmgus end
#include "iolib.h"
//Wyrmgus start
#include <fstream> //for the 0 AD map conversion
//Wyrmgus end
#include "player.h"
//Wyrmgus start
#include "province.h"
#include "quest.h"
#include "settings.h"
//Wyrmgus end
#include "tileset.h"
#include "unit.h"
//Wyrmgus start
#include "unit_find.h"
//Wyrmgus end
#include "unit_manager.h"
#include "ui.h"
//Wyrmgus start
#include "upgrade.h"
//Wyrmgus end
#include "version.h"
#include "video.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

//Wyrmgus start
std::vector<CMapTemplate *> MapTemplates;
std::map<std::string, CMapTemplate *> MapTemplateIdentToPointer;
//Wyrmgus end
CMap Map;                   /// The current map
//Wyrmgus start
int CurrentMapLayer = 0;
//Wyrmgus end
int FlagRevealMap;          /// Flag must reveal the map
int ReplayRevealMap;        /// Reveal Map is replay
int ForestRegeneration;     /// Forest regeneration
char CurrentMapPath[1024];  /// Path of the current map

//Wyrmgus start
/**
**  Get a map template
*/
CMapTemplate *GetMapTemplate(std::string map_ident)
{
	if (map_ident.empty()) {
		return NULL;
	}
	
	if (MapTemplateIdentToPointer.find(map_ident) != MapTemplateIdentToPointer.end()) {
		return MapTemplateIdentToPointer[map_ident];
	}
	
	return NULL;
}

//Wyrmgus start
std::string GetDegreeLevelNameById(int degree_level)
{
	if (degree_level == ExtremelyHighDegreeLevel) {
		return "extremely-high";
	} else if (degree_level == VeryHighDegreeLevel) {
		return "very-high";
	} else if (degree_level == HighDegreeLevel) {
		return "high";
	} else if (degree_level == MediumDegreeLevel) {
		return "medium";
	} else if (degree_level == LowDegreeLevel) {
		return "low";
	} else if (degree_level == VeryLowDegreeLevel) {
		return "very-low";
	}
	return "";
}

int GetDegreeLevelIdByName(std::string degree_level)
{
	if (degree_level == "extremely-high") {
		return ExtremelyHighDegreeLevel;
	} else if (degree_level == "very-high") {
		return VeryHighDegreeLevel;
	} else if (degree_level == "high") {
		return HighDegreeLevel;
	} else if (degree_level == "medium") {
		return MediumDegreeLevel;
	} else if (degree_level == "low") {
		return LowDegreeLevel;
	} else if (degree_level == "very-low") {
		return VeryLowDegreeLevel;
	} else {
		return -1;
	}
}

void CMapTemplate::SetTileTerrain(const Vec2i &pos, CTerrainType *terrain)
{
	unsigned int index = pos.x + pos.y * this->Width;
	
	if (terrain->Overlay && !(terrain->Flags & MapFieldWaterAllowed)) {
		if (index >= this->TileOverlayTerrains.size()) {
			this->TileOverlayTerrains.resize(index + 1);
			std::fill(this->TileOverlayTerrains.begin(), this->TileOverlayTerrains.end(), -1);
		}
		this->TileOverlayTerrains[index] = terrain->ID;
	} else {
		if (index >= this->TileTerrains.size()) {
			this->TileTerrains.resize(index + 1);
			std::fill(this->TileTerrains.begin(), this->TileTerrains.end(), -1);
		}
		this->TileTerrains[index] = terrain->ID;
	}
}

void CMapTemplate::ParseTerrainFile(bool overlay)
{
	std::string terrain_file;
	if (overlay) {
		terrain_file = this->OverlayTerrainFile;
	} else {
		terrain_file = this->TerrainFile;
	}
	
	if (terrain_file.empty()) {
		return;
	}
	
	const std::string terrain_filename = LibraryFileName(terrain_file.c_str());
		
	if (!CanAccessFile(terrain_filename.c_str())) {
		fprintf(stderr, "File \"%s\" not found.\n", terrain_filename.c_str());
	}
	
	std::ifstream is_map(terrain_filename);
	
	std::string line_str;
	int y = 0;
	while (std::getline(is_map, line_str))
	{
		int x = 0;
		
		size_t pos = 0;
		size_t previous_pos = pos;
		while ((pos = line_str.find(",", pos)) != std::string::npos) {
			int terrain_id_length = pos - previous_pos - 1;
			char terrain_id = atoi(line_str.substr(pos - terrain_id_length, terrain_id_length).c_str());
			if (terrain_id != -1) {
				if (overlay) {
					this->TileOverlayTerrains[x + y * this->Width] = terrain_id;
				} else {
					this->TileTerrains[x + y * this->Width] = terrain_id;
				}
			}
			previous_pos = pos;
			pos += 1;
			x += 1;
		}
		
		y += 1;
	}  
}

void CMapTemplate::Apply(Vec2i template_start_pos, Vec2i map_start_pos, int z)
{
	if (SaveGameLoading) {
		return;
	}
	
	if (template_start_pos.x < 0 || template_start_pos.x >= this->Width || template_start_pos.y < 0 || template_start_pos.y >= this->Height) {
		fprintf(stderr, "Invalid map coordinate : (%d, %d)\n", template_start_pos.x, template_start_pos.y);
		return;
	}
	
	if (z >= (int) Map.Fields.size()) {
		Map.Info.MapWidths.push_back(this->Width);
		Map.Info.MapHeights.push_back(this->Height);
		Map.Fields.push_back(new CMapField[this->Width * this->Height]);
		Map.TimeOfDaySeconds.push_back(this->TimeOfDaySeconds);
		Map.TimeOfDay.push_back(NoTimeOfDay);
		Map.Planes.push_back(this->Plane);
		Map.Worlds.push_back(this->World);
		Map.Layers.push_back(this->Layer);
		Map.LayerConnectors.resize(z + 1);
	} else {
		if (!this->IsSubtemplateArea()) {
			Map.TimeOfDaySeconds[z] = this->TimeOfDaySeconds;
			Map.Planes[z] = this->Plane;
			Map.Worlds[z] = this->World;
			Map.Layers[z] = this->Layer;
		}
	}

	if (this->TimeOfDaySeconds && !GameSettings.Inside && !GameSettings.NoTimeOfDay && Editor.Running == EditorNotRunning && !this->IsSubtemplateArea()) {
		Map.TimeOfDay[z] = SyncRand(MaxTimesOfDay - 1) + 1; // begin at a random time of day
	}
	
	Vec2i map_end(std::min(Map.Info.MapWidths[z], map_start_pos.x + this->Width), std::min(Map.Info.MapHeights[z], map_start_pos.y + this->Height));
	if (!Map.Info.IsPointOnMap(map_start_pos, z)) {
		fprintf(stderr, "Invalid map coordinate : (%d, %d)\n", map_start_pos.x, map_start_pos.y);
		return;
	}
	
	for (int x = 0; x < Map.Info.MapWidths[z]; ++x) {
		if ((template_start_pos.x + x) >= this->Width) {
			break;
		}
		for (int y = 0; y < Map.Info.MapHeights[z]; ++y) {
			if ((template_start_pos.y + y) >= this->Height) {
				break;
			}
			Vec2i pos(template_start_pos.x + x, template_start_pos.y + y);
			Vec2i real_pos(map_start_pos.x + x, map_start_pos.y + y);
			if (this->GetTileTerrain(pos, false)) {
				Map.Field(real_pos, z)->SetTerrain(this->GetTileTerrain(pos, false));
			}
			if (this->GetTileTerrain(pos, true)) {
				Map.Field(real_pos, z)->SetTerrain(this->GetTileTerrain(pos, true));
			}
		}
	}
	
	if (this->IsSubtemplateArea() && this->SurroundingTerrain) {
		Vec2i surrounding_start_pos(map_start_pos - Vec2i(1, 1));
		Vec2i surrounding_end(map_end + Vec2i(1, 1));
		for (int x = surrounding_start_pos.x; x < surrounding_end.x; ++x) {
			for (int y = surrounding_start_pos.y; y < surrounding_end.y; y += (surrounding_end.y - surrounding_start_pos.y - 1)) {
				Vec2i surrounding_pos(x, y);
				if (!Map.Info.IsPointOnMap(surrounding_pos, z) || Map.IsPointInASubtemplateArea(surrounding_pos, z)) {
					continue;
				}
				Map.Field(surrounding_pos, z)->SetTerrain(this->SurroundingTerrain);
			}
		}
		for (int x = surrounding_start_pos.x; x < surrounding_end.x; x += (surrounding_end.x - surrounding_start_pos.x - 1)) {
			for (int y = surrounding_start_pos.y; y < surrounding_end.y; ++y) {
				Vec2i surrounding_pos(x, y);
				if (!Map.Info.IsPointOnMap(surrounding_pos, z) || Map.IsPointInASubtemplateArea(surrounding_pos, z)) {
					continue;
				}
				Map.Field(surrounding_pos, z)->SetTerrain(this->SurroundingTerrain);
			}
		}
	}
	
	if (CurrentCampaign && CurrentCampaign->Faction && !this->IsSubtemplateArea() && ThisPlayer->Faction != CurrentCampaign->Faction->ID) {
		ThisPlayer->SetCivilization(CurrentCampaign->Faction->Civilization);
		ThisPlayer->SetFaction(CurrentCampaign->Faction);
		ThisPlayer->Resources[CopperCost] = 2500; // give the player enough resources to start up
		ThisPlayer->Resources[WoodCost] = 2500;
		ThisPlayer->Resources[StoneCost] = 2500;
	}
	
	for (size_t i = 0; i < this->Subtemplates.size(); ++i) {
		Vec2i random_pos(0, 0);
		Vec2i min_pos(map_start_pos);
		Vec2i max_pos(map_end.x - this->Subtemplates[i]->Width, map_end.y - this->Subtemplates[i]->Height);
		int while_count = 0;
		while (while_count < 1000) {
			random_pos.x = SyncRand(max_pos.x - min_pos.x + 1) + min_pos.x;
			random_pos.y = SyncRand(max_pos.y - min_pos.y + 1) + min_pos.y;
			
			bool on_map = Map.Info.IsPointOnMap(random_pos, z) && Map.Info.IsPointOnMap(Vec2i(random_pos.x + this->Subtemplates[i]->Width - 1, random_pos.y + this->Subtemplates[i]->Height - 1), z);
			
			bool on_subtemplate_area = false;
			for (int x = 0; x < this->Subtemplates[i]->Width; ++x) {
				for (int y = 0; y < this->Subtemplates[i]->Height; ++y) {
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
				this->Subtemplates[i]->Apply(Vec2i(0, 0), random_pos, z);
				
				Map.SubtemplateAreas[z].push_back(std::pair<Vec2i, Vec2i>(random_pos, Vec2i(random_pos.x + this->Subtemplates[i]->Width - 1, random_pos.y + this->Subtemplates[i]->Height - 1)));
				
				for (size_t j = 0; j < this->Subtemplates[i]->ExternalGeneratedTerrains.size(); ++j) {
					Vec2i external_start_pos(random_pos.x - (this->Subtemplates[i]->Width / 2), random_pos.y - (this->Subtemplates[i]->Height / 2));
					Vec2i external_end(random_pos.x + this->Subtemplates[i]->Width + (this->Subtemplates[i]->Width / 2), random_pos.y + this->Subtemplates[i]->Height + (this->Subtemplates[i]->Height / 2));
					int map_width = (external_end.x - external_start_pos.x);
					int map_height = (external_end.y - external_start_pos.y);
					int expansion_number = 0;
					
					int degree_level = this->Subtemplates[i]->ExternalGeneratedTerrains[j].second;
					
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
					
					Map.GenerateTerrain(this->Subtemplates[i]->ExternalGeneratedTerrains[j].first, 0, expansion_number, external_start_pos, external_end - Vec2i(1, 1), !this->Subtemplates[i]->TerrainFile.empty(), z);
				}
				break;
			}
			
			while_count += 1;
		}
	}
	
	if (!this->IsSubtemplateArea()) {
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
	
	for (std::map<std::pair<int, int>, std::tuple<CUnitType *, int, CUniqueItem *>>::iterator iterator = this->Resources.begin(); iterator != this->Resources.end(); ++iterator) {
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

	this->ApplyUnits(template_start_pos, map_start_pos, z);
	
	for (size_t i = 0; i < this->GeneratedTerrains.size(); ++i) {
		int map_width = (map_end.x - map_start_pos.x);
		int map_height = (map_end.y - map_start_pos.y);
		int seed_number = map_width * map_height / 1024;
		int expansion_number = 0;
		
		int degree_level = this->GeneratedTerrains[i].second;
		
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
		
		Map.GenerateTerrain(this->GeneratedTerrains[i].first, seed_number, expansion_number, map_start_pos, map_end - Vec2i(1, 1), !this->TerrainFile.empty(), z);
	}
	
	if (!this->IsSubtemplateArea()) {
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

	// now, generate the units and heroes that were set to be generated at a random position (by having their position set to {-1, -1})
	this->ApplyUnits(template_start_pos, map_start_pos, z, true);

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
		if (Players[i].StartPos.x == 0 && Players[i].StartPos.y == 0) {
			continue;
		}
		// add five workers at the player's starting location
		if (Players[i].NumTownHalls > 0) {
			int worker_type_id = PlayerRaces.GetFactionClassUnitType(Players[i].Race, Players[i].Faction, GetUnitTypeClassIndexByName("worker"));			
			if (worker_type_id != -1) {
				Vec2i worker_unit_offset((UnitTypes[worker_type_id]->TileWidth - 1) / 2, (UnitTypes[worker_type_id]->TileHeight - 1) / 2);
				for (int j = 0; j < 5; ++j) {
					CUnit *worker_unit = CreateUnit(Players[i].StartPos, *UnitTypes[worker_type_id], &Players[i], Players[i].StartMapLayer);
				}
			}
		}
		
		if (Players[i].NumTownHalls > 0 || Players[i].Index == ThisPlayer->Index) {
			for (size_t j = 0; j < this->PlayerLocationGeneratedNeutralUnits.size(); ++j) {
				Map.GenerateNeutralUnits(this->PlayerLocationGeneratedNeutralUnits[j].first, this->PlayerLocationGeneratedNeutralUnits[j].second, Players[i].StartPos - Vec2i(8, 8), Players[i].StartPos + Vec2i(8, 8), true, z);
			}
		}
	}
	
	for (size_t i = 0; i < this->GeneratedNeutralUnits.size(); ++i) {
		bool grouped = this->GeneratedNeutralUnits[i].first->GivesResource && this->GeneratedNeutralUnits[i].first->TileWidth == 1 && this->GeneratedNeutralUnits[i].first->TileHeight == 1; // group small resources
		Map.GenerateNeutralUnits(this->GeneratedNeutralUnits[i].first, this->GeneratedNeutralUnits[i].second, map_start_pos, map_end - Vec2i(1, 1), grouped, z);
	}
}

void CMapTemplate::ApplyUnits(Vec2i template_start_pos, Vec2i map_start_pos, int z, bool random)
{
	Vec2i map_end(std::min(Map.Info.MapWidths[z], map_start_pos.x + this->Width), std::min(Map.Info.MapHeights[z], map_start_pos.y + this->Height));

	for (size_t i = 0; i < this->PlaneConnectors.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(this->PlaneConnectors[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = Map.GenerateUnitLocation(std::get<1>(this->PlaneConnectors[i]), NULL, map_start_pos, map_end - Vec2i(1, 1), z);
		}
		if (!Map.Info.IsPointOnMap(unit_pos, z)) {
			continue;
		}
		Vec2i unit_offset((std::get<1>(this->PlaneConnectors[i])->TileWidth - 1) / 2, (std::get<1>(this->PlaneConnectors[i])->TileHeight - 1) / 2);
		CUnit *unit = CreateUnit(unit_pos - unit_offset, *std::get<1>(this->PlaneConnectors[i]), &Players[PlayerNumNeutral], z);
		Map.LayerConnectors[z].push_back(unit);
		for (size_t second_z = 0; second_z < Map.LayerConnectors.size(); ++second_z) {
			bool found_other_connector = false;
			if (Map.Planes[second_z] == std::get<2>(this->PlaneConnectors[i])) {
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
	
	for (size_t i = 0; i < this->WorldConnectors.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(this->WorldConnectors[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = Map.GenerateUnitLocation(std::get<1>(this->WorldConnectors[i]), NULL, map_start_pos, map_end - Vec2i(1, 1), z);
		}
		if (!Map.Info.IsPointOnMap(unit_pos, z)) {
			continue;
		}
		Vec2i unit_offset((std::get<1>(this->WorldConnectors[i])->TileWidth - 1) / 2, (std::get<1>(this->WorldConnectors[i])->TileHeight - 1) / 2);
		CUnit *unit = CreateUnit(unit_pos - unit_offset, *std::get<1>(this->WorldConnectors[i]), &Players[PlayerNumNeutral], z);
		Map.LayerConnectors[z].push_back(unit);
		for (size_t second_z = 0; second_z < Map.LayerConnectors.size(); ++second_z) {
			bool found_other_connector = false;
			if (Map.Worlds[second_z] == std::get<2>(this->WorldConnectors[i])) {
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
	
	for (size_t i = 0; i < this->LayerConnectors.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(this->LayerConnectors[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = Map.GenerateUnitLocation(std::get<1>(this->LayerConnectors[i]), NULL, map_start_pos, map_end - Vec2i(1, 1), z);
		}
		if (!Map.Info.IsPointOnMap(unit_pos, z)) {
			continue;
		}
		Vec2i unit_offset((std::get<1>(this->LayerConnectors[i])->TileWidth - 1) / 2, (std::get<1>(this->LayerConnectors[i])->TileHeight - 1) / 2);
		CUnit *unit = CreateUnit(unit_pos - unit_offset, *std::get<1>(this->LayerConnectors[i]), &Players[PlayerNumNeutral], z);
		Map.LayerConnectors[z].push_back(unit);
		for (size_t second_z = 0; second_z < Map.LayerConnectors.size(); ++second_z) {
			bool found_other_connector = false;
			if (Map.Layers[second_z] == std::get<2>(this->LayerConnectors[i]) && Map.Worlds[second_z] == this->World && Map.Planes[second_z] == this->Plane) {
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
	
	for (size_t i = 0; i < this->Units.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(this->Units[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		const CUnitType *type = std::get<1>(this->Units[i]);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = Map.GenerateUnitLocation(type, std::get<2>(this->Units[i]), map_start_pos, map_end - Vec2i(1, 1), z);
		}
		if (!Map.Info.IsPointOnMap(unit_pos, z)) {
			continue;
		}
		
		if ((!CurrentCampaign || std::get<3>(this->Units[i]) == 0 || CurrentCampaign->Year >= std::get<3>(this->Units[i])) && (std::get<4>(this->Units[i]) == 0 || CurrentCampaign->Year < std::get<4>(this->Units[i]))) {
			CPlayer *player = NULL;
			if (std::get<2>(this->Units[i])) {
				player = GetOrAddFactionPlayer(std::get<2>(this->Units[i]));
				if (player->StartPos.x == 0 && player->StartPos.y == 0) {
					Vec2i default_pos(map_start_pos + std::get<2>(this->Units[i])->DefaultStartPos - template_start_pos);
					if (std::get<2>(this->Units[i])->DefaultStartPos.x != -1 && std::get<2>(this->Units[i])->DefaultStartPos.y != -1 && Map.Info.IsPointOnMap(default_pos, z)) {
						player->SetStartView(default_pos, z);
					} else {
						player->SetStartView(unit_pos, z);
					}
				}
			} else {
				player = &Players[PlayerNumNeutral];
			}
			Vec2i unit_offset((type->TileWidth - 1) / 2, (type->TileHeight - 1) / 2);
			CUnit *unit = CreateUnit(unit_pos - unit_offset, *std::get<1>(this->Units[i]), player, z);
			if (!type->BoolFlag[BUILDING_INDEX].value) { // make non-building units not have an active AI
				unit->Active = 0;
				player->UnitTypesAiActiveCount[type->Slot]--;
			}
		}
	}
	
	for (size_t i = 0; i < this->Heroes.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(this->Heroes[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		CCharacter *hero = std::get<1>(this->Heroes[i]);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = Map.GenerateUnitLocation(hero->Type, std::get<2>(this->Heroes[i]), map_start_pos, map_end - Vec2i(1, 1), z);
		}
		if (!Map.Info.IsPointOnMap(unit_pos, z)) {
			continue;
		}
		
		if ((!CurrentCampaign || std::get<3>(this->Heroes[i]) == 0 || CurrentCampaign->Year >= std::get<3>(this->Heroes[i])) && (std::get<4>(this->Heroes[i]) == 0 || CurrentCampaign->Year < std::get<4>(this->Heroes[i]))) {
			CPlayer *player = NULL;
			if (std::get<2>(this->Heroes[i])) {
				player = GetOrAddFactionPlayer(std::get<2>(this->Heroes[i]));
				if (player->StartPos.x == 0 && player->StartPos.y == 0) {
					player->SetStartView(unit_pos, z);
				}
			} else {
				player = &Players[PlayerNumNeutral];
			}
			Vec2i unit_offset((hero->Type->TileWidth - 1) / 2, (hero->Type->TileHeight - 1) / 2);
			CUnit *unit = CreateUnit(unit_pos - unit_offset, *hero->Type, player, z);
			unit->SetCharacter(hero->GetFullName());
			unit->Active = 0;
			player->UnitTypesAiActiveCount[hero->Type->Slot]--;
		}
	}
}

bool CMapTemplate::IsSubtemplateArea()
{
	return this->MainTemplate != NULL;
}

CTerrainType *CMapTemplate::GetTileTerrain(const Vec2i &pos, bool overlay)
{
	unsigned int index = pos.x + pos.y * this->Width;
	
	if (overlay) {
		if (index < this->TileOverlayTerrains.size() && this->TileOverlayTerrains[index] != -1) {
			return TerrainTypes[this->TileOverlayTerrains[index]];
		}
	} else {
		if (index < this->TileTerrains.size() && this->TileTerrains[index] != -1) {
			return TerrainTypes[this->TileTerrains[index]];
		} else if (this->BorderTerrain && (pos.x == 0 || pos.x == (this->Width - 1) || pos.y == 0 || pos.y == (this->Height - 1))) {
			return this->BorderTerrain;
		} else if (this->BaseTerrain) {
			return this->BaseTerrain;
		}
	}
	
	return NULL;
}
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Visible and explored handling
----------------------------------------------------------------------------*/

/**
**  Marks seen tile -- used mainly for the Fog Of War
**
**  @param mf  MapField-position.
*/
//Wyrmgus start
//void CMap::MarkSeenTile(CMapField &mf)
void CMap::MarkSeenTile(CMapField &mf, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	const unsigned int tile = mf.getGraphicTile();
//	const unsigned int seentile = mf.playerInfo.SeenTile;
	//Wyrmgus end

	//  Nothing changed? Seeing already the correct tile.
	//Wyrmgus start
//	if (tile == seentile) {
	if (mf.IsSeenTileCorrect()) {
	//Wyrmgus end
		return;
	}
	//Wyrmgus start
//	mf.playerInfo.SeenTile = tile;
	mf.UpdateSeenTile();
	//Wyrmgus end

#ifdef MINIMAP_UPDATE
	//rb - GRRRRRRRRRRRR
	//Wyrmgus start
//	const unsigned int index = &mf - Map.Fields;
//	const int y = index / Info.MapWidth;
//	const int x = index - (y * Info.MapWidth);
	const unsigned int index = &mf - Map.Fields[z];
	const int y = index / Info.MapWidths[z];
	const int x = index - (y * Info.MapWidths[z]);
	//Wyrmgus end
	const Vec2i pos = {x, y}
#endif

	//Wyrmgus start
	/*
	if (this->Tileset->TileTypeTable.empty() == false) {
#ifndef MINIMAP_UPDATE
		//rb - GRRRRRRRRRRRR
		const unsigned int index = &mf - Map.Fields;
		const int y = index / Info.MapWidth;
		const int x = index - (y * Info.MapWidth);
		const Vec2i pos(x, y);
#endif

		//  Handle wood changes. FIXME: check if for growing wood correct?
		if (tile == this->Tileset->getRemovedTreeTile()) {
			FixNeighbors(MapFieldForest, 1, pos);
		} else if (seentile == this->Tileset->getRemovedTreeTile()) {
			FixTile(MapFieldForest, 1, pos);
		} else if (mf.ForestOnMap()) {
			FixTile(MapFieldForest, 1, pos);
			FixNeighbors(MapFieldForest, 1, pos);

			// Handle rock changes.
		} else if (tile == Tileset->getRemovedRockTile()) {
			FixNeighbors(MapFieldRocks, 1, pos);
		} else if (seentile == Tileset->getRemovedRockTile()) {
			FixTile(MapFieldRocks, 1, pos);
		} else if (mf.RockOnMap()) {
			FixTile(MapFieldRocks, 1, pos);
			FixNeighbors(MapFieldRocks, 1, pos);

			//  Handle Walls changes.
		} else if (this->Tileset->isAWallTile(tile)
				   || this->Tileset->isAWallTile(seentile)) {
		//Wyrmgus end
			MapFixSeenWallTile(pos);
			MapFixSeenWallNeighbors(pos);
		}
	}
	*/
	//Wyrmgus end

#ifdef MINIMAP_UPDATE
	//Wyrmgus start
//	UI.Minimap.UpdateXY(pos);
	UI.Minimap.UpdateXY(pos, z);
	//Wyrmgus end
#endif
}

/**
**  Reveal the entire map.
*/
//Wyrmgus start
//void CMap::Reveal()
void CMap::Reveal(bool only_person_players)
//Wyrmgus end
{
	//  Mark every explored tile as visible. 1 turns into 2.
	//Wyrmgus start
	/*
	for (int i = 0; i != this->Info.MapWidth * this->Info.MapHeight; ++i) {
		CMapField &mf = *this->Field(i);
		CMapFieldPlayerInfo &playerInfo = mf.playerInfo;
		for (int p = 0; p < PlayerMax; ++p) {
			//Wyrmgus start
//			playerInfo.Visible[p] = std::max<unsigned short>(1, playerInfo.Visible[p]);
			if (Players[p].Type == PlayerPerson || !only_person_players) {
				playerInfo.Visible[p] = std::max<unsigned short>(1, playerInfo.Visible[p]);
			}
			//Wyrmgus end
		}
		MarkSeenTile(mf);
	}
	*/
	for (size_t z = 0; z < this->Fields.size(); ++z) {
		for (int i = 0; i != this->Info.MapWidths[z] * this->Info.MapHeights[z]; ++i) {
			CMapField &mf = *this->Field(i, z);
			CMapFieldPlayerInfo &playerInfo = mf.playerInfo;
			for (int p = 0; p < PlayerMax; ++p) {
				if (Players[p].Type == PlayerPerson || !only_person_players) {
					playerInfo.Visible[p] = std::max<unsigned short>(1, playerInfo.Visible[p]);
				}
			}
			MarkSeenTile(mf, z);
		}
	}
	//Wyrmgus end
	//  Global seen recount. Simple and effective.
	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
		CUnit &unit = **it;
		//  Reveal neutral buildings. Gold mines:)
		if (unit.Player->Type == PlayerNeutral) {
			for (int p = 0; p < PlayerMax; ++p) {
				//Wyrmgus start
//				if (Players[p].Type != PlayerNobody && (!(unit.Seen.ByPlayer & (1 << p)))) {
				if (Players[p].Type != PlayerNobody && (Players[p].Type == PlayerPerson || !only_person_players) && (!(unit.Seen.ByPlayer & (1 << p)))) {
				//Wyrmgus end
					UnitGoesOutOfFog(unit, Players[p]);
					UnitGoesUnderFog(unit, Players[p]);
				}
			}
		}
		UnitCountSeen(unit);
	}
}

/*----------------------------------------------------------------------------
--  Map queries
----------------------------------------------------------------------------*/

Vec2i CMap::MapPixelPosToTilePos(const PixelPos &mapPos) const
{
	const Vec2i tilePos(mapPos.x / PixelTileSize.x, mapPos.y / PixelTileSize.y);

	return tilePos;
}

PixelPos CMap::TilePosToMapPixelPos_TopLeft(const Vec2i &tilePos) const
{
	PixelPos mapPixelPos(tilePos.x * PixelTileSize.x, tilePos.y * PixelTileSize.y);

	return mapPixelPos;
}

PixelPos CMap::TilePosToMapPixelPos_Center(const Vec2i &tilePos) const
{
	return TilePosToMapPixelPos_TopLeft(tilePos) + PixelTileSize / 2;
}

//Wyrmgus start
CTerrainType *CMap::GetTileTerrain(const Vec2i &pos, bool overlay, int z) const
{
	if (!Map.Info.IsPointOnMap(pos, z)) {
		return NULL;
	}
	
	CMapField &mf = *this->Field(pos, z);
	
	if (overlay) {
		return mf.OverlayTerrain;
	} else {
		return mf.Terrain;
	}
}

CTerrainType *CMap::GetTileTopTerrain(const Vec2i &pos, bool seen, int z) const
{
	if (!Map.Info.IsPointOnMap(pos, z)) {
		return NULL;
	}
	
	CMapField &mf = *this->Field(pos, z);
	
	if (!seen) {
		if (mf.OverlayTerrain) {
			return mf.OverlayTerrain;
		} else {
			return mf.Terrain;
		}
	} else {
		if (mf.playerInfo.SeenOverlayTerrain) {
			return mf.playerInfo.SeenOverlayTerrain;
		} else {
			return mf.playerInfo.SeenTerrain;
		}
	}
}

Vec2i CMap::GenerateUnitLocation(const CUnitType *unit_type, CFaction *faction, Vec2i min_pos, Vec2i max_pos, int z) const
{
	if (SaveGameLoading) {
		return Vec2i(-1, -1);
	}
	
	CPlayer *player = GetFactionPlayer(faction);
	if (!unit_type->BoolFlag[TOWNHALL_INDEX].value && player != NULL && player->StartMapLayer == z && player->StartPos.x >= min_pos.x && player->StartPos.x <= max_pos.x && player->StartPos.y >= min_pos.y && player->StartPos.y <= max_pos.y) {
		min_pos = player->StartPos - Vec2i(8, 8);
		max_pos = player->StartPos + Vec2i(8, 8);
	}
	
	Vec2i random_pos(-1, -1);

	int while_count = 0;
	
	while (while_count < 100) {
		random_pos.x = SyncRand(max_pos.x - (unit_type->TileWidth - 1) - min_pos.x + 1) + min_pos.x;
		random_pos.y = SyncRand(max_pos.y - (unit_type->TileHeight - 1) - min_pos.y + 1) + min_pos.y;
		
		if (!this->Info.IsPointOnMap(random_pos, z) || (this->IsPointInASubtemplateArea(random_pos, z) && GameCycle > 0)) {
			continue;
		}
		
		std::vector<CUnit *> table;
		if (player != NULL) {
			Select(random_pos - Vec2i(16, 16), random_pos + Vec2i(unit_type->TileWidth - 1, unit_type->TileHeight - 1) + Vec2i(16, 16), table, z, MakeAndPredicate(HasNotSamePlayerAs(*player), HasNotSamePlayerAs(Players[PlayerNumNeutral])));
		} else if (!unit_type->GivesResource) {
			Select(random_pos - Vec2i(16, 16), random_pos + Vec2i(unit_type->TileWidth - 1, unit_type->TileHeight - 1) + Vec2i(16, 16), table, z, HasNotSamePlayerAs(Players[PlayerNumNeutral]));
		}
		
		if (table.size() == 0 && UnitTypeCanBeAt(*unit_type, random_pos, z) && (!unit_type->BoolFlag[BUILDING_INDEX].value || CanBuildUnitType(NULL, *unit_type, random_pos, 0, true, z))) {
			return random_pos;
		}
		
		while_count += 1;
	}
	
	return Vec2i(-1, -1);
}
//Wyrmgus end

/**
**  Wall on map tile.
**
**  @param pos  map tile position.
**
**  @return    True if wall, false otherwise.
*/
//Wyrmgus start
//bool CMap::WallOnMap(const Vec2i &pos) const
bool CMap::WallOnMap(const Vec2i &pos, int z) const
//Wyrmgus end
{
	//Wyrmgus start
//	Assert(Map.Info.IsPointOnMap(pos));
//	return Field(pos)->isAWall();
	Assert(Map.Info.IsPointOnMap(pos, z));
	return Field(pos, z)->isAWall();
	//Wyrmgus end
}

/**
**  Human wall on map tile.
**
**  @param pos  map tile position.
**
**  @return    True if human wall, false otherwise.
*/
//Wyrmgus start
/*
bool CMap::HumanWallOnMap(const Vec2i &pos) const
{
	Assert(Map.Info.IsPointOnMap(pos));
	return Field(pos)->isAHumanWall();
}
*/
//Wyrmgus end

/**
**  Orc wall on map tile.
**
**  @param pos  map tile position.
**
**  @return    True if orcish wall, false otherwise.
*/
//Wyrmgus start
/*
bool CMap::OrcWallOnMap(const Vec2i &pos) const
{
	Assert(Map.Info.IsPointOnMap(pos));
	return Field(pos)->isAOrcWall();
}
*/
//Wyrmgus end

//Wyrmgus start
bool CMap::CurrentTerrainCanBeAt(const Vec2i &pos, bool overlay, int z)
{
	CMapField &mf = *this->Field(pos, z);
	CTerrainType *terrain = NULL;
	
	if (overlay) {
		terrain = mf.OverlayTerrain;
	} else {
		terrain = mf.Terrain;
	}
	
	if (!terrain) {
		return true;
	}
	
	if (terrain->AllowSingle) {
		return true;
	}

	std::vector<int> transition_directions;
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
						
					CTerrainType *adjacent_terrain = this->GetTileTerrain(adjacent_pos, overlay, z);
					if (overlay && adjacent_terrain && this->Field(adjacent_pos, z)->OverlayTerrainDestroyed) {
						adjacent_terrain = NULL;
					}
					if (terrain != adjacent_terrain) { // also happens if terrain is NULL, so that i.e. tree transitions display correctly when adjacent to tiles without overlays
						transition_directions.push_back(GetDirectionFromOffset(x_offset, y_offset));
					}
				}
			}
		}
	}
	
	if (std::find(transition_directions.begin(), transition_directions.end(), North) != transition_directions.end() && std::find(transition_directions.begin(), transition_directions.end(), South) != transition_directions.end()) {
		return false;
	} else if (std::find(transition_directions.begin(), transition_directions.end(), West) != transition_directions.end() && std::find(transition_directions.begin(), transition_directions.end(), East) != transition_directions.end()) {
		return false;
	}

	return true;
}

bool CMap::TileBordersOnlySameTerrain(const Vec2i &pos, CTerrainType *new_terrain, int z)
{
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			if (!this->Info.IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			if (this->IsPointInASubtemplateArea(pos, z) && !this->IsPointInASubtemplateArea(adjacent_pos, z)) {
				continue;
			}
			CTerrainType *top_terrain = GetTileTopTerrain(pos, false, z);
			CTerrainType *adjacent_top_terrain = GetTileTopTerrain(adjacent_pos, false, z);
			if (!new_terrain->Overlay) {
				if (
					adjacent_top_terrain
					&& adjacent_top_terrain != top_terrain
					&& (std::find(top_terrain->InnerBorderTerrains.begin(), top_terrain->InnerBorderTerrains.end(), adjacent_top_terrain) == top_terrain->InnerBorderTerrains.end() || std::find(new_terrain->InnerBorderTerrains.begin(), new_terrain->InnerBorderTerrains.end(), adjacent_top_terrain) == new_terrain->InnerBorderTerrains.end())
					&& adjacent_top_terrain != new_terrain
				) {
					return false;
				}
			} else {
				if (
					adjacent_top_terrain
					&& adjacent_top_terrain != top_terrain
					&& std::find(top_terrain->BaseTerrains.begin(), top_terrain->BaseTerrains.end(), adjacent_top_terrain) == top_terrain->BaseTerrains.end() && std::find(adjacent_top_terrain->BaseTerrains.begin(), adjacent_top_terrain->BaseTerrains.end(), top_terrain) == adjacent_top_terrain->BaseTerrains.end()
					&& adjacent_top_terrain != new_terrain
				) {
					return false;
				}
			}
		}
	}
		
	return true;
}

bool CMap::TileBordersFlag(const Vec2i &pos, int z, int flag, bool reverse)
{
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			if (!this->Info.IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			CMapField &mf = *Map.Field(adjacent_pos, z);
			
			if ((!reverse && mf.CheckMask(flag)) || (reverse && !mf.CheckMask(flag))) {
				return true;
			}
		}
	}
		
	return false;
}

bool CMap::TileBordersBuilding(const Vec2i &pos, int z)
{
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			if (!this->Info.IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			CMapField &mf = *Map.Field(adjacent_pos, z);
			
			const CUnitCache &cache = mf.UnitCache;
			for (size_t i = 0; i != cache.size(); ++i) {
				CUnit &unit = *cache[i];
				if (unit.IsAliveOnMap()) {
					if (unit.Type->BoolFlag[BUILDING_INDEX].value || unit.Type->GivesResource) { // also return true for resource-giving units, so that unpassable terrain isn't generated near them
						return true;
					}
				}
			}
		}
	}
		
	return false;
}

bool CMap::TileHasUnitsIncompatibleWithTerrain(const Vec2i &pos, CTerrainType *terrain, int z)
{
	CMapField &mf = *Map.Field(pos, z);
	
	const CUnitCache &cache = mf.UnitCache;
	for (size_t i = 0; i != cache.size(); ++i) {
		CUnit &unit = *cache[i];
		if (unit.IsAliveOnMap() && (terrain->Flags & unit.Type->MovementMask) != 0) {
			return true;
		}
	}

	return false;
}

bool CMap::IsPointInASubtemplateArea(const Vec2i &pos, int z) const
{
	if (this->SubtemplateAreas.find(z) == this->SubtemplateAreas.end()) {
		return false;
	}
	
	for (size_t i = 0; i < this->SubtemplateAreas.find(z)->second.size(); ++i) {
		Vec2i min_pos = this->SubtemplateAreas.find(z)->second[i].first;
		Vec2i max_pos = this->SubtemplateAreas.find(z)->second[i].second;
		if (pos.x >= min_pos.x && pos.y >= min_pos.y && pos.x <= max_pos.x && pos.y <= max_pos.y) {
			return true;
		}
	}

	return false;
}
//Wyrmgus end

/**
**  Can move to this point, applying mask.
**
**  @param pos   map tile position.
**  @param mask  Mask for movement to apply.
**
**  @return      True if could be entered, false otherwise.
*/
//Wyrmgus start
//bool CheckedCanMoveToMask(const Vec2i &pos, int mask)
bool CheckedCanMoveToMask(const Vec2i &pos, int mask, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	return Map.Info.IsPointOnMap(pos) && CanMoveToMask(pos, mask);
	return Map.Info.IsPointOnMap(pos, z) && CanMoveToMask(pos, mask, z);
	//Wyrmgus end
}

/**
**  Can a unit of unit-type be placed at this point.
**
**  @param type  unit-type to be checked.
**  @param pos   map tile position.
**
**  @return      True if could be entered, false otherwise.
*/
//Wyrmgus start
//bool UnitTypeCanBeAt(const CUnitType &type, const Vec2i &pos)
bool UnitTypeCanBeAt(const CUnitType &type, const Vec2i &pos, int z)
//Wyrmgus end
{
	const int mask = type.MovementMask;
	//Wyrmgus start
//	unsigned int index = pos.y * Map.Info.MapWidth;
	unsigned int index = pos.y * Map.Info.MapWidths[z];
	//Wyrmgus end

	for (int addy = 0; addy < type.TileHeight; ++addy) {
		for (int addx = 0; addx < type.TileWidth; ++addx) {
			//Wyrmgus start
			/*
			if (Map.Info.IsPointOnMap(pos.x + addx, pos.y + addy) == false
				|| Map.Field(pos.x + addx + index)->CheckMask(mask) == true) {
			*/
			if (Map.Info.IsPointOnMap(pos.x + addx, pos.y + addy, z) == false
				|| Map.Field(pos.x + addx + index, z)->CheckMask(mask) == true) {
			//Wyrmgus end
				return false;
			}
		}
		//Wyrmgus start
//		index += Map.Info.MapWidth;
		index += Map.Info.MapWidths[z];
		//Wyrmgus end
	}
	return true;
}

/**
**  Can a unit be placed to this point.
**
**  @param unit  unit to be checked.
**  @param pos   map tile position.
**
**  @return      True if could be placeded, false otherwise.
*/
//Wyrmgus start
//bool UnitCanBeAt(const CUnit &unit, const Vec2i &pos)
bool UnitCanBeAt(const CUnit &unit, const Vec2i &pos, int z)
//Wyrmgus end
{
	Assert(unit.Type);
	if (unit.Type->BoolFlag[NONSOLID_INDEX].value) {
		return true;
	}
	//Wyrmgus start
//	return UnitTypeCanBeAt(*unit.Type, pos);
	return UnitTypeCanBeAt(*unit.Type, pos, z);
	//Wyrmgus end
}

/**
**  Fixes initially the wood and seen tiles.
*/
void PreprocessMap()
{
	//Wyrmgus start
	/*
	for (int ix = 0; ix < Map.Info.MapWidth; ++ix) {
		for (int iy = 0; iy < Map.Info.MapHeight; ++iy) {
			CMapField &mf = *Map.Field(ix, iy);
			mf.playerInfo.SeenTile = mf.getGraphicTile();
		}
	}
	*/
	for (size_t z = 0; z < Map.Fields.size(); ++z) {
		for (int ix = 0; ix < Map.Info.MapWidths[z]; ++ix) {
			for (int iy = 0; iy < Map.Info.MapHeights[z]; ++iy) {
				CMapField &mf = *Map.Field(ix, iy, z);
				Map.CalculateTileTransitions(Vec2i(ix, iy), false, z);
				Map.CalculateTileTransitions(Vec2i(ix, iy), true, z);
				Map.CalculateTileVisibility(Vec2i(ix, iy), z);
				mf.UpdateSeenTile();
			}
		}
	}
	//Wyrmgus end
	//Wyrmgus start
	/*
	// it is required for fixing the wood that all tiles are marked as seen!
	if (Map.Tileset->TileTypeTable.empty() == false) {
		Vec2i pos;
		for (pos.x = 0; pos.x < Map.Info.MapWidth; ++pos.x) {
			for (pos.y = 0; pos.y < Map.Info.MapHeight; ++pos.y) {
				MapFixWallTile(pos);
				MapFixSeenWallTile(pos);
			}
		}
	}
	*/
	//Wyrmgus end
}

//Wyrmgus start
void ChangeCurrentMapLayer(int z)
{
	if (z < 0 || z >= (int) Map.Fields.size() || CurrentMapLayer == z) {
		return;
	}
	
	Vec2i new_viewport_map_pos(UI.SelectedViewport->MapPos.x * Map.Info.MapWidths[z] / Map.Info.MapWidths[CurrentMapLayer], UI.SelectedViewport->MapPos.y * Map.Info.MapHeights[z] / Map.Info.MapHeights[CurrentMapLayer]);
	
	CurrentMapLayer = z;
	UI.Minimap.UpdateCache = true;
	UI.SelectedViewport->Set(new_viewport_map_pos, PixelTileSize / 2);
}

void SetTimeOfDay(int time_of_day, int z)
{
	Map.TimeOfDay[z] = time_of_day;
}
//Wyrmgus end

/**
**  Clear CMapInfo.
*/
void CMapInfo::Clear()
{
	this->Description.clear();
	this->Filename.clear();
	this->MapWidth = this->MapHeight = 0;
	//Wyrmgus start
	this->MapWidths.clear();
	this->MapHeights.clear();
	//Wyrmgus end
	memset(this->PlayerSide, 0, sizeof(this->PlayerSide));
	memset(this->PlayerType, 0, sizeof(this->PlayerType));
	this->MapUID = 0;
}

//Wyrmgus start
//CMap::CMap() : Fields(NULL), NoFogOfWar(false), TileGraphic(NULL)
CMap::CMap() : NoFogOfWar(false), TileGraphic(NULL)
//Wyrmgus end
{
	Tileset = new CTileset;
}

CMap::~CMap()
{
	delete Tileset;
}

/**
**  Alocate and initialise map table
*/
void CMap::Create()
{
	//Wyrmgus start
//	Assert(!this->Fields);
	Assert(this->Fields.size() == 0);
	//Wyrmgus end

	//Wyrmgus start
//	this->Fields = new CMapField[this->Info.MapWidth * this->Info.MapHeight];
	this->Fields.push_back(new CMapField[this->Info.MapWidth * this->Info.MapHeight]);
	this->Info.MapWidths.push_back(this->Info.MapWidth);
	this->Info.MapHeights.push_back(this->Info.MapHeight);
	this->TimeOfDaySeconds.push_back(DefaultTimeOfDaySeconds);
	if (!GameSettings.Inside && !GameSettings.NoTimeOfDay && Editor.Running == EditorNotRunning) {
		this->TimeOfDay.push_back(SyncRand(MaxTimesOfDay - 1) + 1); // begin at a random time of day
	} else {
		this->TimeOfDay.push_back(NoTimeOfDay); // make indoors have no time of day setting until it is possible to make light sources change their surrounding "time of day" // indoors it is always dark (maybe would be better to allow a special setting to have bright indoor places?
	}
	this->Planes.push_back(NULL);
	this->Worlds.push_back(NULL);
	this->Layers.push_back(0);
	this->LayerConnectors.resize(1);
	//Wyrmgus end
}

/**
**  Initialize the fog of war.
**  Build tables, setup functions.
*/
void CMap::Init()
{
	InitFogOfWar();
}

/**
**  Cleanup the map module.
*/
void CMap::Clean()
{
	//Wyrmgus start
	CurrentMapLayer = 0;
	//Wyrmgus end

	//Wyrmgus start
//	delete[] this->Fields;
	for (size_t z = 0; z < this->Fields.size(); ++z) {
		delete[] this->Fields[z];
	}
	this->Fields.clear();
	this->TimeOfDaySeconds.clear();
	this->TimeOfDay.clear();
	this->Planes.clear();
	this->Worlds.clear();
	this->Layers.clear();
	this->LayerConnectors.clear();
	//Wyrmgus end

	// Tileset freed by Tileset?

	this->Info.Clear();
	//Wyrmgus start
//	this->Fields = NULL;
	//Wyrmgus end
	this->NoFogOfWar = false;
	//Wyrmgus start
	for (size_t i = 0; i != Map.Tileset->solidTerrainTypes.size(); ++i) {
		if (!Map.Tileset->solidTerrainTypes[i].ImageFile.empty()) {
			if (CGraphic::Get(Map.Tileset->solidTerrainTypes[i].ImageFile) != NULL) {
				CGraphic::Free(this->SolidTileGraphics[i]);
			}
			this->SolidTileGraphics[i] = NULL;
		}
	}
	memset(SolidTileGraphics, 0, sizeof(SolidTileGraphics));
	//Wyrmgus end
	this->Tileset->clear();
	this->TileModelsFileName.clear();
	CGraphic::Free(this->TileGraphic);
	this->TileGraphic = NULL;

	FlagRevealMap = 0;
	ReplayRevealMap = 0;

	UI.Minimap.Destroy();
	
	//Wyrmgus start
	SubtemplateAreas.clear();
	//Wyrmgus end
}

/**
** Save the complete map.
**
** @param file Output file.
*/
void CMap::Save(CFile &file) const
{
	file.printf("\n--- -----------------------------------------\n");
	file.printf("--- MODULE: map\n");
	file.printf("LoadTileModels(\"%s\")\n\n", this->TileModelsFileName.c_str());
	file.printf("StratagusMap(\n");
	file.printf("  \"version\", \"%s\",\n", VERSION);
	file.printf("  \"description\", \"%s\",\n", this->Info.Description.c_str());
	file.printf("  \"the-map\", {\n");
	file.printf("  \"size\", {%d, %d},\n", this->Info.MapWidth, this->Info.MapHeight);
	file.printf("  \"%s\",\n", this->NoFogOfWar ? "no-fog-of-war" : "fog-of-war");
	file.printf("  \"filename\", \"%s\",\n", this->Info.Filename.c_str());
	//Wyrmgus start
	file.printf("  \"extra-map-layers\", {\n");
	for (size_t z = 1; z < this->Fields.size(); ++z) {
		file.printf("  {%d, %d},\n", this->Info.MapWidths[z], this->Info.MapHeights[z]);
	}
	file.printf("  },\n");
	file.printf("  \"time-of-day\", {\n");
	for (size_t z = 0; z < this->TimeOfDaySeconds.size(); ++z) {
		file.printf("  {%d, %d},\n", this->TimeOfDaySeconds[z], this->TimeOfDay[z]);
	}
	file.printf("  },\n");
	file.printf("  \"layer-references\", {\n");
	for (size_t z = 0; z < this->Fields.size(); ++z) {
		file.printf("  {\"%s\", \"%s\", %d},\n", this->Planes[z] ? this->Planes[z]->Name.c_str() : "", this->Worlds[z] ? this->Worlds[z]->Name.c_str() : "", this->Layers[z]);
	}
	file.printf("  },\n");
	//Wyrmgus end
	file.printf("  \"map-fields\", {\n");
	//Wyrmgus start
	/*
	for (int h = 0; h < this->Info.MapHeight; ++h) {
		file.printf("  -- %d\n", h);
		for (int w = 0; w < this->Info.MapWidth; ++w) {
			const CMapField &mf = *this->Field(w, h);

			mf.Save(file);
			if (w & 1) {
				file.printf(",\n");
			} else {
				file.printf(", ");
			}
		}
	}
	*/
	for (size_t z = 0; z < this->Fields.size(); ++z) {
		file.printf("  {\n");
		for (int h = 0; h < this->Info.MapHeights[z]; ++h) {
			file.printf("  -- %d\n", h);
			for (int w = 0; w < this->Info.MapWidths[z]; ++w) {
				const CMapField &mf = *this->Field(w, h, z);

				mf.Save(file);
				if (w & 1) {
					file.printf(",\n");
				} else {
					file.printf(", ");
				}
			}
		}
		file.printf("  },\n");
	}
	//Wyrmgus end
	file.printf("}})\n");
}

/*----------------------------------------------------------------------------
-- Map Tile Update Functions
----------------------------------------------------------------------------*/

/**
**  Correct the seen wood field, depending on the surrounding.
**
**  @param type  type of tile to update
**  @param seen  1 if updating seen value, 0 for real
**  @param pos   Map tile-position.
*/
//Wyrmgus start
/*
void CMap::FixTile(unsigned short type, int seen, const Vec2i &pos)
{
	Assert(type == MapFieldForest || type == MapFieldRocks);

	//  Outside of map or no wood.
	if (!Info.IsPointOnMap(pos)) {
		return;
	}
	unsigned int index = getIndex(pos);
	CMapField &mf = *this->Field(index);

	if (!((type == MapFieldForest && Tileset->isAWoodTile(mf.playerInfo.SeenTile))
		  || (type == MapFieldRocks && Tileset->isARockTile(mf.playerInfo.SeenTile)))) {
		if (seen) {
			return;
		}
	}

	if (!seen && !(mf.getFlag() & type)) {
		return;
	}

	// Select Table to lookup
	int removedtile;
	int flags;
	if (type == MapFieldForest) {
		removedtile = this->Tileset->getRemovedTreeTile();
		flags = (MapFieldForest | MapFieldUnpassable);
	} else { // (type == MapFieldRocks)
		removedtile = this->Tileset->getRemovedRockTile();
		flags = (MapFieldRocks | MapFieldUnpassable);
	}
	//  Find out what each tile has with respect to wood, or grass.
	int ttup;
	int ttdown;
	int ttleft;
	int ttright;

	if (pos.y - 1 < 0) {
		ttup = -1; //Assign trees in all directions
	} else {
		const CMapField &new_mf = *(&mf - this->Info.MapWidth);
		ttup = seen ? new_mf.playerInfo.SeenTile : new_mf.getGraphicTile();
	}
	if (pos.x + 1 >= this->Info.MapWidth) {
		ttright = -1; //Assign trees in all directions
	} else {
		const CMapField &new_mf = *(&mf + 1);
		ttright = seen ? new_mf.playerInfo.SeenTile : new_mf.getGraphicTile();
	}
	if (pos.y + 1 >= this->Info.MapHeight) {
		ttdown = -1; //Assign trees in all directions
	} else {
		const CMapField &new_mf = *(&mf + this->Info.MapWidth);
		ttdown = seen ? new_mf.playerInfo.SeenTile : new_mf.getGraphicTile();
	}
	if (pos.x - 1 < 0) {
		ttleft = -1; //Assign trees in all directions
	} else {
		const CMapField &new_mf = *(&mf - 1);
		ttleft = seen ? new_mf.playerInfo.SeenTile : new_mf.getGraphicTile();
	}
	int tile = this->Tileset->getTileBySurrounding(type, ttup, ttright, ttdown, ttleft);

	//Update seen tile.
	if (tile == -1) { // No valid wood remove it.
		if (seen) {
			mf.playerInfo.SeenTile = removedtile;
			this->FixNeighbors(type, seen, pos);
		} else {
			mf.setGraphicTile(removedtile);
			mf.Flags &= ~flags;
			mf.Value = 0;
			UI.Minimap.UpdateXY(pos);
		}
	} else if (seen && this->Tileset->isEquivalentTile(tile, mf.playerInfo.SeenTile)) { //Same Type
		return;
	} else {
		if (seen) {
			mf.playerInfo.SeenTile = tile;
		} else {
			mf.setGraphicTile(tile);
		}
	}

	//maybe isExplored
	if (mf.playerInfo.IsExplored(*ThisPlayer)) {
		UI.Minimap.UpdateSeenXY(pos);
		if (!seen) {
			MarkSeenTile(mf);
		}
	}
}
*/
//Wyrmgus end

/**
**  Correct the surrounding fields.
**
**  @param type  Tiletype of tile to adjust
**  @param seen  1 if updating seen value, 0 for real
**  @param pos   Map tile-position.
*/
//Wyrmgus start
/*
void CMap::FixNeighbors(unsigned short type, int seen, const Vec2i &pos)
{
	const Vec2i offset[] = {Vec2i(1, 0), Vec2i(-1, 0), Vec2i(0, 1), Vec2i(0, -1),
							Vec2i(-1, -1), Vec2i(-1, 1), Vec2i(1, -1), Vec2i(1, 1)
						   };

	for (unsigned int i = 0; i < sizeof(offset) / sizeof(*offset); ++i) {
		FixTile(type, seen, pos + offset[i]);
	}
}
*/
//Wyrmgus end

//Wyrmgus start
void CMap::SetTileTerrain(const Vec2i &pos, CTerrainType *terrain, int z)
{
	if (!terrain) {
		return;
	}
	
	CMapField &mf = *this->Field(pos, z);
	
	if (terrain->Overlay)  {
		if (mf.OverlayTerrain == terrain) {
			return;
		}
	} else {
		if (mf.Terrain == terrain) {
			return;
		}
	}
	
	mf.SetTerrain(terrain);
	
	this->CalculateTileTransitions(pos, false, z); //recalculate both, since one may have changed the other
	this->CalculateTileTransitions(pos, true, z);
	this->CalculateTileVisibility(pos, z);
	
	UI.Minimap.UpdateXY(pos, z);
	if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
		MarkSeenTile(mf, z);
	}
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
						
					this->CalculateTileTransitions(adjacent_pos, false, z);
					this->CalculateTileTransitions(adjacent_pos, true, z);
					this->CalculateTileVisibility(adjacent_pos, z);
					
					UI.Minimap.UpdateXY(adjacent_pos, z);
					if (adjacent_mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
						MarkSeenTile(adjacent_mf, z);
					}
				}
			}
		}
	}
}

//Wyrmgus start
//void CMap::RemoveTileOverlayTerrain(const Vec2i &pos)
void CMap::RemoveTileOverlayTerrain(const Vec2i &pos, int z)
//Wyrmgus end
{
	CMapField &mf = *this->Field(pos, z);
	
	if (!mf.OverlayTerrain) {
		return;
	}
	
	mf.RemoveOverlayTerrain();
	
	this->CalculateTileTransitions(pos, true, z);
	this->CalculateTileVisibility(pos, z);
	
	UI.Minimap.UpdateXY(pos, z);
	if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
		MarkSeenTile(mf, z);
	}
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
						
					this->CalculateTileTransitions(adjacent_pos, true, z);
					this->CalculateTileVisibility(adjacent_pos, z);
					
					UI.Minimap.UpdateXY(adjacent_pos, z);
					if (adjacent_mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
						MarkSeenTile(adjacent_mf, z);
					}
				}
			}
		}
	}
}

void CMap::SetOverlayTerrainDestroyed(const Vec2i &pos, bool destroyed, int z)
{
	CMapField &mf = *this->Field(pos, z);
	
	if (!mf.OverlayTerrain || mf.OverlayTerrainDestroyed == destroyed) {
		return;
	}
	
	mf.SetOverlayTerrainDestroyed(destroyed);
	
	this->CalculateTileTransitions(pos, true, z);
	this->CalculateTileVisibility(pos, z);
	
	UI.Minimap.UpdateXY(pos, z);
	if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
		MarkSeenTile(mf, z);
	}
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
						
					this->CalculateTileTransitions(adjacent_pos, true, z);
					this->CalculateTileVisibility(adjacent_pos, z);
					
					UI.Minimap.UpdateXY(adjacent_pos, z);
					if (adjacent_mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
						MarkSeenTile(adjacent_mf, z);
					}
				}
			}
		}
	}
}

//Wyrmgus start
//void CMap::SetOverlayTerrainDamaged(const Vec2i &pos, bool damaged)
void CMap::SetOverlayTerrainDamaged(const Vec2i &pos, bool damaged, int z)
//Wyrmgus end
{
	CMapField &mf = *this->Field(pos, z);
	
	if (!mf.OverlayTerrain || mf.OverlayTerrainDestroyed == damaged) {
		return;
	}
	
	mf.SetOverlayTerrainDamaged(damaged);
	
	this->CalculateTileTransitions(pos, true, z);
	
	UI.Minimap.UpdateXY(pos, z);
	if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
		MarkSeenTile(mf, z);
	}
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
						
					this->CalculateTileTransitions(adjacent_pos, true, z);
					
					UI.Minimap.UpdateXY(adjacent_pos, z);
					if (adjacent_mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
						MarkSeenTile(adjacent_mf, z);
					}
				}
			}
		}
	}
}

void CMap::CalculateTileTransitions(const Vec2i &pos, bool overlay, int z)
{
	CMapField &mf = *this->Field(pos, z);
	CTerrainType *terrain = NULL;
	if (overlay) {
		terrain = mf.OverlayTerrain;
		mf.OverlayTransitionTiles.clear();
	} else {
		terrain = mf.Terrain;
		mf.TransitionTiles.clear();
	}
	
	if (!terrain || (overlay && mf.OverlayTerrainDestroyed)) {
		return;
	}
	
	int terrain_id = terrain->ID;
	
	std::map<int, std::vector<int>> adjacent_terrain_directions;
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CTerrainType *adjacent_terrain = this->GetTileTerrain(adjacent_pos, overlay, z);
					if (overlay && adjacent_terrain && this->Field(adjacent_pos, z)->OverlayTerrainDestroyed) {
						adjacent_terrain = NULL;
					}
					if (adjacent_terrain && terrain != adjacent_terrain) {
						if (std::find(terrain->InnerBorderTerrains.begin(), terrain->InnerBorderTerrains.end(), adjacent_terrain) != terrain->InnerBorderTerrains.end()) {
							adjacent_terrain_directions[adjacent_terrain->ID].push_back(GetDirectionFromOffset(x_offset, y_offset));
						} else if (std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), adjacent_terrain) == terrain->BorderTerrains.end()) { //if the two terrain types can't border, look for a third terrain type which can border both, and which treats both as outer border terrains, and then use for transitions between both tiles
							for (size_t i = 0; i < terrain->BorderTerrains.size(); ++i) {
								CTerrainType *border_terrain = terrain->BorderTerrains[i];
								if (std::find(terrain->InnerBorderTerrains.begin(), terrain->InnerBorderTerrains.end(), border_terrain) != terrain->InnerBorderTerrains.end() && std::find(adjacent_terrain->InnerBorderTerrains.begin(), adjacent_terrain->InnerBorderTerrains.end(), border_terrain) != adjacent_terrain->InnerBorderTerrains.end()) {
									adjacent_terrain_directions[border_terrain->ID].push_back(GetDirectionFromOffset(x_offset, y_offset));
									break;
								}
							}
						}
					}
					if (terrain != adjacent_terrain) { // also happens if terrain is NULL, so that i.e. tree transitions display correctly when adjacent to tiles without overlays
						adjacent_terrain_directions[TerrainTypes.size()].push_back(GetDirectionFromOffset(x_offset, y_offset));
					}
				}
			}
		}
	}
	
	for (std::map<int, std::vector<int>>::iterator iterator = adjacent_terrain_directions.begin(); iterator != adjacent_terrain_directions.end(); ++iterator) {
		int adjacent_terrain_id = iterator->first;
		CTerrainType *adjacent_terrain = adjacent_terrain_id < (int) TerrainTypes.size() ? TerrainTypes[adjacent_terrain_id] : NULL;
		int transition_type = -1;
		
		if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), North) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end()) {
			transition_type = SingleTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), North) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end()) {
			transition_type = NorthSingleTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end()) {
			transition_type = SouthSingleTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), North) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end()) {
			transition_type = WestSingleTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), North) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end()) {
			transition_type = EastSingleTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), North) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end()) {
			transition_type = NorthSouthTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end()) {
			transition_type = WestEastTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southeast) != iterator->second.end()) {
			transition_type = WestSoutheastInnerTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southwest) != iterator->second.end()) {
			transition_type = EastSouthwestInnerTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northeast) != iterator->second.end()) {
			transition_type = SouthwestOuterNortheastInnerTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northwest) != iterator->second.end()) {
			transition_type = SoutheastOuterNorthwestInnerTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), North) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southwest) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southeast) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end()) {
			transition_type = NorthTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northwest) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northeast) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end()) {
			transition_type = SouthTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northeast) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southeast) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end()) {
			transition_type = WestTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northwest) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southwest) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end()) {
			transition_type = EastTransitionType;
		} else if ((std::find(iterator->second.begin(), iterator->second.end(), North) != iterator->second.end() || std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end()) && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southeast) == iterator->second.end()) {
			transition_type = NorthwestOuterTransitionType;
		} else if ((std::find(iterator->second.begin(), iterator->second.end(), North) != iterator->second.end() || std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end()) && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southwest) == iterator->second.end()) {
			transition_type = NortheastOuterTransitionType;
		} else if ((std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() || std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end()) && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northeast) == iterator->second.end()) {
			transition_type = SouthwestOuterTransitionType;
		} else if ((std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() || std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end()) && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northwest) == iterator->second.end()) {
			transition_type = SoutheastOuterTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), Northwest) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southeast) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northeast) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southwest) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end()) {
			transition_type = NorthwestSoutheastInnerTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), Northeast) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southwest) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northwest) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southeast) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end()) {
			transition_type = NortheastSouthwestInnerTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), Northwest) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end()) {
			transition_type = NorthwestInnerTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), Northeast) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end()) {
			transition_type = NortheastInnerTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), Southwest) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end()) {
			transition_type = SouthwestInnerTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), Southeast) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end()) {
			transition_type = SoutheastInnerTransitionType;
		}
		
		if (transition_type != -1) {
			bool found_transition = false;
			
			if (!overlay) {
				if (adjacent_terrain) {
					if (terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)].size() > 0) {
						mf.TransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)][SyncRand(terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)].size())]));
						found_transition = true;
					} else if (adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)].size() > 0) {
						mf.TransitionTiles.push_back(std::pair<CTerrainType *, int>(adjacent_terrain, adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)][SyncRand(adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)].size())]));
						found_transition = true;
					} else if (adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)].size() > 0) {
						mf.TransitionTiles.push_back(std::pair<CTerrainType *, int>(adjacent_terrain, adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)][SyncRand(adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)].size())]));
						found_transition = true;
					}
				} else {
					if (terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)].size() > 0) {
						mf.TransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)][SyncRand(terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)].size())]));
					}
				}
			} else {
				if (adjacent_terrain) {
					if (adjacent_terrain && terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)].size() > 0) {
						mf.OverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)][SyncRand(terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)].size())]));
						found_transition = true;
					} else if (adjacent_terrain && adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)].size() > 0) {
						mf.OverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(adjacent_terrain, adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)][SyncRand(adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)].size())]));
						found_transition = true;
					} else if (adjacent_terrain && adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)].size() > 0) {
						mf.OverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(adjacent_terrain, adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)][SyncRand(adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)].size())]));
						found_transition = true;
					}
				} else {
					if (terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)].size() > 0) {
						mf.OverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)][SyncRand(terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)].size())]));
					}
				}
				
				if ((mf.Flags & MapFieldWaterAllowed) && (!adjacent_terrain || !(adjacent_terrain->Flags & MapFieldWaterAllowed))) { //if this is a water tile adjacent to a non-water tile, replace the water flag with a coast one
					mf.Flags &= ~(MapFieldWaterAllowed);
					mf.Flags |= MapFieldCoastAllowed;
				}
			}
			
			if (adjacent_terrain && found_transition) {
				for (size_t i = 0; i != iterator->second.size(); ++i) {
					adjacent_terrain_directions[TerrainTypes.size()].erase(std::remove(adjacent_terrain_directions[TerrainTypes.size()].begin(), adjacent_terrain_directions[TerrainTypes.size()].end(), iterator->second[i]), adjacent_terrain_directions[TerrainTypes.size()].end());
				}
			}
		}
	}
	
	//sort the transitions so that they will be displayed in the correct order
	if (overlay) {
		bool swapped = true;
		for (int passes = 0; passes < (int) mf.OverlayTransitionTiles.size() && swapped; ++passes) {
			swapped = false;
			for (int i = 0; i < ((int) mf.OverlayTransitionTiles.size()) - 1; ++i) {
				bool change_order = false;
				if (std::find(mf.OverlayTransitionTiles[i + 1].first->InnerBorderTerrains.begin(), mf.OverlayTransitionTiles[i + 1].first->InnerBorderTerrains.end(), mf.OverlayTransitionTiles[i].first) != mf.OverlayTransitionTiles[i + 1].first->InnerBorderTerrains.end()) {
					std::pair<CTerrainType *, int> temp_transition = mf.OverlayTransitionTiles[i];
					mf.OverlayTransitionTiles[i] = mf.OverlayTransitionTiles[i + 1];
					mf.OverlayTransitionTiles[i + 1] = temp_transition;
					swapped = true;
				}
			}
		}
	} else {
		bool swapped = true;
		for (int passes = 0; passes < (int) mf.TransitionTiles.size() && swapped; ++passes) {
			swapped = false;
			for (int i = 0; i < ((int) mf.TransitionTiles.size()) - 1; ++i) {
				bool change_order = false;
				if (std::find(mf.TransitionTiles[i + 1].first->InnerBorderTerrains.begin(), mf.TransitionTiles[i + 1].first->InnerBorderTerrains.end(), mf.TransitionTiles[i].first) != mf.TransitionTiles[i + 1].first->InnerBorderTerrains.end()) {
					std::pair<CTerrainType *, int> temp_transition = mf.TransitionTiles[i];
					mf.TransitionTiles[i] = mf.TransitionTiles[i + 1];
					mf.TransitionTiles[i + 1] = temp_transition;
					swapped = true;
				}
			}
		}
	}
}

void CMap::CalculateTileVisibility(const Vec2i &pos, int z)
{
	if (!this->Info.IsPointOnMap(pos, z)) {
		return;
	}
	
	CMapField &mf = *this->Field(pos, z);
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z) && (this->Field(adjacent_pos, z)->Flags & MapFieldAirUnpassable)) {
					mf.Visible[GetDirectionFromOffset(x_offset, y_offset)] = false;
				} else {
					mf.Visible[GetDirectionFromOffset(x_offset, y_offset)] = true;
				}
			}
		}
	}
}

void CMap::AdjustTileMapIrregularities(bool overlay, const Vec2i &min_pos, const Vec2i &max_pos, int z)
{
	bool no_irregularities_found = false;
	while (!no_irregularities_found) {
		no_irregularities_found = true;
		for (int x = min_pos.x; x < max_pos.x; ++x) {
			for (int y = min_pos.y; y < max_pos.y; ++y) {
				CMapField &mf = *this->Field(x, y, z);
				CTerrainType *terrain = overlay ? mf.OverlayTerrain : mf.Terrain;
				if (!terrain) {
					continue;
				}
				std::vector<CTerrainType *> acceptable_adjacent_tile_types;
				acceptable_adjacent_tile_types.push_back(terrain);
				for (size_t i = 0; i < terrain->OuterBorderTerrains.size(); ++i) {
					acceptable_adjacent_tile_types.push_back(terrain->OuterBorderTerrains[i]);
				}
				
				int horizontal_adjacent_tiles = 0;
				int vertical_adjacent_tiles = 0;
				int nw_quadrant_adjacent_tiles = 0; //should be 4 if the wrong tile types are present in X-1,Y; X-1,Y-1; X,Y-1; and X+1,Y+1
				int ne_quadrant_adjacent_tiles = 0;
				int sw_quadrant_adjacent_tiles = 0;
				int se_quadrant_adjacent_tiles = 0;
				
				if ((x - 1) >= 0 && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), this->GetTileTerrain(Vec2i(x - 1, y), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					horizontal_adjacent_tiles += 1;
					nw_quadrant_adjacent_tiles += 1;
					sw_quadrant_adjacent_tiles += 1;
				}
				if ((x + 1) < this->Info.MapWidths[z] && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), this->GetTileTerrain(Vec2i(x + 1, y), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					horizontal_adjacent_tiles += 1;
					ne_quadrant_adjacent_tiles += 1;
					se_quadrant_adjacent_tiles += 1;
				}
				
				if ((y - 1) >= 0 && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), this->GetTileTerrain(Vec2i(x, y - 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					vertical_adjacent_tiles += 1;
					nw_quadrant_adjacent_tiles += 1;
					ne_quadrant_adjacent_tiles += 1;
				}
				if ((y + 1) < this->Info.MapHeights[z] && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), this->GetTileTerrain(Vec2i(x, y + 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					vertical_adjacent_tiles += 1;
					sw_quadrant_adjacent_tiles += 1;
					se_quadrant_adjacent_tiles += 1;
				}

				if ((x - 1) >= 0 && (y - 1) >= 0 && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), this->GetTileTerrain(Vec2i(x - 1, y - 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					nw_quadrant_adjacent_tiles += 1;
					se_quadrant_adjacent_tiles += 1;
				}
				if ((x - 1) >= 0 && (y + 1) < this->Info.MapHeights[z] && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), GetTileTerrain(Vec2i(x - 1, y + 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					sw_quadrant_adjacent_tiles += 1;
					ne_quadrant_adjacent_tiles += 1;
				}
				if ((x + 1) < this->Info.MapWidths[z] && (y - 1) >= 0 && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), GetTileTerrain(Vec2i(x + 1, y - 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					ne_quadrant_adjacent_tiles += 1;
					sw_quadrant_adjacent_tiles += 1;
				}
				if ((x + 1) < this->Info.MapWidths[z] && (y + 1) < this->Info.MapHeights[z] && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), GetTileTerrain(Vec2i(x + 1, y + 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					se_quadrant_adjacent_tiles += 1;
					nw_quadrant_adjacent_tiles += 1;
				}
				
				
				if (horizontal_adjacent_tiles >= 2 || vertical_adjacent_tiles >= 2 || nw_quadrant_adjacent_tiles >= 4 || ne_quadrant_adjacent_tiles >= 4 || sw_quadrant_adjacent_tiles >= 4 || se_quadrant_adjacent_tiles >= 4) {
					if (overlay) {
						mf.RemoveOverlayTerrain();
					} else {
						if (terrain->InnerBorderTerrains.size() > 0) {
							mf.SetTerrain(terrain->InnerBorderTerrains[0]);
						}
					}
					no_irregularities_found = false;
				}
			}
		}
	}
}

void CMap::AdjustTileMapTransitions(const Vec2i &min_pos, const Vec2i &max_pos, int z)
{
	for (int x = min_pos.x; x < max_pos.x; ++x) {
		for (int y = min_pos.y; y < max_pos.y; ++y) {
			CMapField &mf = *this->Field(x, y, z);

			for (int sub_x = -1; sub_x <= 1; ++sub_x) {
				for (int sub_y = -1; sub_y <= 1; ++sub_y) {
					if ((x + sub_x) < min_pos.x || (x + sub_x) >= max_pos.x || (y + sub_y) < min_pos.y || (y + sub_y) >= max_pos.y || (sub_x == 0 && sub_y == 0)) {
						continue;
					}
					CTerrainType *tile_terrain = GetTileTerrain(Vec2i(x + sub_x, y + sub_y), false, z);
					CTerrainType *tile_top_terrain = GetTileTopTerrain(Vec2i(x + sub_x, y + sub_y), false, z);
					if (mf.Terrain != tile_terrain && tile_top_terrain->Overlay && std::find(tile_terrain->OuterBorderTerrains.begin(), tile_terrain->OuterBorderTerrains.end(), mf.Terrain) == tile_terrain->OuterBorderTerrains.end() && std::find(tile_top_terrain->BaseTerrains.begin(), tile_top_terrain->BaseTerrains.end(), mf.Terrain) == tile_top_terrain->BaseTerrains.end()) {
						mf.SetTerrain(tile_terrain);
					}
				}
			}
		}
	}

	for (int x = min_pos.x; x < max_pos.x; ++x) {
		for (int y = min_pos.y; y < max_pos.y; ++y) {
			CMapField &mf = *this->Field(x, y, z);

			for (int sub_x = -1; sub_x <= 1; ++sub_x) {
				for (int sub_y = -1; sub_y <= 1; ++sub_y) {
					if ((x + sub_x) < min_pos.x || (x + sub_x) >= max_pos.x || (y + sub_y) < min_pos.y || (y + sub_y) >= max_pos.y || (sub_x == 0 && sub_y == 0)) {
						continue;
					}
					CTerrainType *tile_terrain = GetTileTerrain(Vec2i(x + sub_x, y + sub_y), false, z);
					if (mf.Terrain != tile_terrain && std::find(mf.Terrain->BorderTerrains.begin(), mf.Terrain->BorderTerrains.end(), tile_terrain) == mf.Terrain->BorderTerrains.end()) {
						for (size_t i = 0; i < mf.Terrain->BorderTerrains.size(); ++i) {
							CTerrainType *border_terrain = mf.Terrain->BorderTerrains[i];
							if (std::find(border_terrain->BorderTerrains.begin(), border_terrain->BorderTerrains.end(), mf.Terrain) != border_terrain->BorderTerrains.end() && std::find(border_terrain->BorderTerrains.begin(), border_terrain->BorderTerrains.end(), tile_terrain) != border_terrain->BorderTerrains.end()) {
								mf.SetTerrain(border_terrain);
								break;
							}
						}
					}
				}
			}
		}
	}
}

void CMap::GenerateTerrain(CTerrainType *terrain, int seed_number, int expansion_number, const Vec2i &min_pos, const Vec2i &max_pos, bool preserve_coastline, int z)
{
	if (SaveGameLoading) {
		return;
	}
	
	Vec2i random_pos(0, 0);
	int count = seed_number;
	int while_count = 0;
	
	// create initial seeds
	while (count > 0 && while_count < seed_number * 100) {
		random_pos.x = SyncRand(max_pos.x - min_pos.x + 1) + min_pos.x;
		random_pos.y = SyncRand(max_pos.y - min_pos.y + 1) + min_pos.y;
		
		if (!this->Info.IsPointOnMap(random_pos, z) || this->IsPointInASubtemplateArea(random_pos, z)) {
			continue;
		}
		
		CTerrainType *tile_terrain = GetTileTerrain(random_pos, false, z);
		
		if (
			(
				(
					!terrain->Overlay
					&& std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), tile_terrain) != terrain->BorderTerrains.end() && this->TileBordersOnlySameTerrain(random_pos, terrain, z)
				)
				|| (terrain->Overlay && std::find(terrain->BaseTerrains.begin(), terrain->BaseTerrains.end(), tile_terrain) != terrain->BaseTerrains.end() && this->TileBordersOnlySameTerrain(random_pos, terrain, z))
			)
			&& (!GetTileTopTerrain(random_pos, false, z)->Overlay || GetTileTopTerrain(random_pos, false, z) == terrain)
			&& (!preserve_coastline || (terrain->Flags & MapFieldWaterAllowed) == (tile_terrain->Flags & MapFieldWaterAllowed))
			&& !this->TileHasUnitsIncompatibleWithTerrain(random_pos, terrain, z)
			&& (!(terrain->Flags & MapFieldUnpassable) || !this->TileBordersBuilding(random_pos, z)) // if the terrain is unpassable, don't expand to spots adjacent to buildings
		) {
			std::vector<Vec2i> adjacent_positions;
			for (int sub_x = -1; sub_x <= 1; sub_x += 2) { // +2 so that only diagonals are used
				for (int sub_y = -1; sub_y <= 1; sub_y += 2) {
					Vec2i diagonal_pos(random_pos.x + sub_x, random_pos.y + sub_y);
					Vec2i vertical_pos(random_pos.x, random_pos.y + sub_y);
					Vec2i horizontal_pos(random_pos.x + sub_x, random_pos.y);
					if (!this->Info.IsPointOnMap(diagonal_pos, z)) {
						continue;
					}
					
					CTerrainType *diagonal_tile_terrain = GetTileTerrain(diagonal_pos, false, z);
					CTerrainType *vertical_tile_terrain = GetTileTerrain(vertical_pos, false, z);
					CTerrainType *horizontal_tile_terrain = GetTileTerrain(horizontal_pos, false, z);
					
					if (
						(
							(
								!terrain->Overlay
								&& std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), diagonal_tile_terrain) != terrain->BorderTerrains.end() && this->TileBordersOnlySameTerrain(diagonal_pos, terrain, z)
								&& std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), vertical_tile_terrain) != terrain->BorderTerrains.end() && this->TileBordersOnlySameTerrain(vertical_pos, terrain, z)
								&& std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), horizontal_tile_terrain) != terrain->BorderTerrains.end() && this->TileBordersOnlySameTerrain(horizontal_pos, terrain, z)
							)
							|| (
								terrain->Overlay
								&& std::find(terrain->BaseTerrains.begin(), terrain->BaseTerrains.end(), diagonal_tile_terrain) != terrain->BaseTerrains.end() && this->TileBordersOnlySameTerrain(diagonal_pos, terrain, z)
								&& std::find(terrain->BaseTerrains.begin(), terrain->BaseTerrains.end(), vertical_tile_terrain) != terrain->BaseTerrains.end() && this->TileBordersOnlySameTerrain(vertical_pos, terrain, z)
								&& std::find(terrain->BaseTerrains.begin(), terrain->BaseTerrains.end(), horizontal_tile_terrain) != terrain->BaseTerrains.end() && this->TileBordersOnlySameTerrain(horizontal_pos, terrain, z)
							)
						)
						&& (!GetTileTopTerrain(diagonal_pos, false, z)->Overlay || GetTileTopTerrain(diagonal_pos, false, z) == terrain) && (!GetTileTopTerrain(vertical_pos, false, z)->Overlay || GetTileTopTerrain(vertical_pos, false, z) == terrain) && (!GetTileTopTerrain(horizontal_pos, false, z)->Overlay || GetTileTopTerrain(horizontal_pos, false, z) == terrain)
						&& (!preserve_coastline || ((terrain->Flags & MapFieldWaterAllowed) == (diagonal_tile_terrain->Flags & MapFieldWaterAllowed) && (terrain->Flags & MapFieldWaterAllowed) == (vertical_tile_terrain->Flags & MapFieldWaterAllowed) && (terrain->Flags & MapFieldWaterAllowed) == (horizontal_tile_terrain->Flags & MapFieldWaterAllowed)))
						&& !this->TileHasUnitsIncompatibleWithTerrain(diagonal_pos, terrain, z) && !this->TileHasUnitsIncompatibleWithTerrain(vertical_pos, terrain, z) && !this->TileHasUnitsIncompatibleWithTerrain(horizontal_pos, terrain, z)
						&& (!(terrain->Flags & MapFieldUnpassable) || (!this->TileBordersBuilding(diagonal_pos, z) && !this->TileBordersBuilding(vertical_pos, z) && !this->TileBordersBuilding(horizontal_pos, z))) // if the terrain is unpassable, don't expand to spots adjacent to buildings
						&& !this->IsPointInASubtemplateArea(diagonal_pos, z) && !this->IsPointInASubtemplateArea(vertical_pos, z) && !this->IsPointInASubtemplateArea(horizontal_pos, z)
					) {
						adjacent_positions.push_back(diagonal_pos);
					}
				}
			}
			
			if (adjacent_positions.size() > 0) {
				Vec2i adjacent_pos = adjacent_positions[SyncRand(adjacent_positions.size())];
				this->Field(random_pos, z)->SetTerrain(terrain);
				this->Field(adjacent_pos, z)->SetTerrain(terrain);
				this->Field(Vec2i(random_pos.x, adjacent_pos.y), z)->SetTerrain(terrain);
				this->Field(Vec2i(adjacent_pos.x, random_pos.y), z)->SetTerrain(terrain);
				count -= 1;
			}
		}
		
		while_count += 1;
	}
	
	// expand seeds
	count = expansion_number;
	while_count = 0;
	
	while (count > 0 && while_count < expansion_number * 100) {
		random_pos.x = SyncRand(max_pos.x - min_pos.x + 1) + min_pos.x;
		random_pos.y = SyncRand(max_pos.y - min_pos.y + 1) + min_pos.y;
		
		if (
			this->Info.IsPointOnMap(random_pos, z)
			&& GetTileTerrain(random_pos, terrain->Overlay, z) == terrain
			&& (!terrain->Overlay || this->TileBordersOnlySameTerrain(random_pos, terrain, z))
		) {
			std::vector<Vec2i> adjacent_positions;
			for (int sub_x = -1; sub_x <= 1; sub_x += 2) { // +2 so that only diagonals are used
				for (int sub_y = -1; sub_y <= 1; sub_y += 2) {
					Vec2i diagonal_pos(random_pos.x + sub_x, random_pos.y + sub_y);
					Vec2i vertical_pos(random_pos.x, random_pos.y + sub_y);
					Vec2i horizontal_pos(random_pos.x + sub_x, random_pos.y);
					if (!this->Info.IsPointOnMap(diagonal_pos, z)) {
						continue;
					}
					
					CTerrainType *diagonal_tile_terrain = GetTileTerrain(diagonal_pos, false, z);
					CTerrainType *vertical_tile_terrain = GetTileTerrain(vertical_pos, false, z);
					CTerrainType *horizontal_tile_terrain = GetTileTerrain(horizontal_pos, false, z);
					
					if (
						(
							(
								!terrain->Overlay
								&& (diagonal_tile_terrain == terrain || (std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), diagonal_tile_terrain) != terrain->BorderTerrains.end() && this->TileBordersOnlySameTerrain(diagonal_pos, terrain, z)))
								&& (vertical_tile_terrain == terrain || (std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), vertical_tile_terrain) != terrain->BorderTerrains.end() && this->TileBordersOnlySameTerrain(vertical_pos, terrain, z)))
								&& (horizontal_tile_terrain == terrain || (std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), horizontal_tile_terrain) != terrain->BorderTerrains.end() && this->TileBordersOnlySameTerrain(horizontal_pos, terrain, z)))
								&& (diagonal_tile_terrain != terrain || vertical_tile_terrain != terrain || horizontal_tile_terrain != terrain)
							)
							|| (
								terrain->Overlay
								&& ((std::find(terrain->BaseTerrains.begin(), terrain->BaseTerrains.end(), diagonal_tile_terrain) != terrain->BaseTerrains.end() && this->TileBordersOnlySameTerrain(diagonal_pos, terrain, z)) || GetTileTerrain(diagonal_pos, terrain->Overlay, z) == terrain)
								&& ((std::find(terrain->BaseTerrains.begin(), terrain->BaseTerrains.end(), vertical_tile_terrain) != terrain->BaseTerrains.end() && this->TileBordersOnlySameTerrain(vertical_pos, terrain, z)) || GetTileTerrain(vertical_pos, terrain->Overlay, z) == terrain)
								&& ((std::find(terrain->BaseTerrains.begin(), terrain->BaseTerrains.end(), horizontal_tile_terrain) != terrain->BaseTerrains.end() && this->TileBordersOnlySameTerrain(horizontal_pos, terrain, z)) || GetTileTerrain(horizontal_pos, terrain->Overlay, z) == terrain)
								&& (GetTileTerrain(diagonal_pos, terrain->Overlay, z) != terrain || GetTileTerrain(vertical_pos, terrain->Overlay, z) != terrain || GetTileTerrain(horizontal_pos, terrain->Overlay, z) != terrain)
							)
						)
						&& (!GetTileTopTerrain(diagonal_pos, false, z)->Overlay || GetTileTopTerrain(diagonal_pos, false, z) == terrain) && (!GetTileTopTerrain(vertical_pos, false, z)->Overlay || GetTileTopTerrain(vertical_pos, false, z) == terrain) && (!GetTileTopTerrain(horizontal_pos, false, z)->Overlay || GetTileTopTerrain(horizontal_pos, false, z) == terrain) // don't expand into tiles with overlays
						&& (!preserve_coastline || ((terrain->Flags & MapFieldWaterAllowed) == (diagonal_tile_terrain->Flags & MapFieldWaterAllowed) && (terrain->Flags & MapFieldWaterAllowed) == (vertical_tile_terrain->Flags & MapFieldWaterAllowed) && (terrain->Flags & MapFieldWaterAllowed) == (horizontal_tile_terrain->Flags & MapFieldWaterAllowed)))
						&& !this->TileHasUnitsIncompatibleWithTerrain(diagonal_pos, terrain, z) && !this->TileHasUnitsIncompatibleWithTerrain(vertical_pos, terrain, z) && !this->TileHasUnitsIncompatibleWithTerrain(horizontal_pos, terrain, z)
						&& (!(terrain->Flags & MapFieldUnpassable) || (!this->TileBordersBuilding(diagonal_pos, z) && !this->TileBordersBuilding(vertical_pos, z) && !this->TileBordersBuilding(horizontal_pos, z))) // if the terrain is unpassable, don't expand to spots adjacent to buildings
						&& (!this->IsPointInASubtemplateArea(diagonal_pos, z) || GetTileTerrain(diagonal_pos, terrain->Overlay, z) == terrain) && (!this->IsPointInASubtemplateArea(vertical_pos, z) || GetTileTerrain(vertical_pos, terrain->Overlay, z) == terrain) && (!this->IsPointInASubtemplateArea(horizontal_pos, z) || GetTileTerrain(horizontal_pos, terrain->Overlay, z) == terrain)
					) {
						adjacent_positions.push_back(diagonal_pos);
					}
				}
			}
			
			if (adjacent_positions.size() > 0) {
				Vec2i adjacent_pos = adjacent_positions[SyncRand(adjacent_positions.size())];
				this->Field(adjacent_pos, z)->SetTerrain(terrain);
				this->Field(Vec2i(random_pos.x, adjacent_pos.y), z)->SetTerrain(terrain);
				this->Field(Vec2i(adjacent_pos.x, random_pos.y), z)->SetTerrain(terrain);
				count -= 1;
			}
		}
		
		while_count += 1;
	}
}

void CMap::GenerateNeutralUnits(CUnitType *unit_type, int quantity, const Vec2i &min_pos, const Vec2i &max_pos, bool grouped, int z)
{
	if (SaveGameLoading) {
		return;
	}
	
	Vec2i unit_pos(-1, -1);
	
	for (int i = 0; i < quantity; ++i) {
		if (i == 0 || !grouped) {
			unit_pos = GenerateUnitLocation(unit_type, NULL, min_pos, max_pos, z);
		}
		if (!this->Info.IsPointOnMap(unit_pos, z)) {
			continue;
		}
		if (unit_type->GivesResource) {
			CUnit *unit = CreateResourceUnit(unit_pos, *unit_type, z);
		} else {
			CUnit *unit = CreateUnit(unit_pos, *unit_type, &Players[PlayerNumNeutral], z);
		}
	}
}
//Wyrmgus end

//Wyrmgus start
void CMap::ClearOverlayTile(const Vec2i &pos, int z)
{
	CMapField &mf = *this->Field(pos, z);

	if (!mf.OverlayTerrain) {
		return;
	}
	
	this->SetOverlayTerrainDestroyed(pos, true, z);
	if (mf.OverlayTerrain->Flags & MapFieldForest) {
		mf.Flags &= ~(MapFieldForest | MapFieldUnpassable);
		mf.Flags |= MapFieldStumps;
	} else if (mf.OverlayTerrain->Flags & MapFieldRocks) {
		mf.Flags &= ~(MapFieldRocks | MapFieldUnpassable);
		mf.Flags |= MapFieldGravel;
		mf.Flags |= MapFieldNoBuilding;
	} else if (mf.OverlayTerrain->Flags & MapFieldWall) {
		mf.Flags &= ~(MapFieldHuman | MapFieldWall | MapFieldUnpassable);
		if (GameSettings.Inside) {
			mf.Flags &= ~(MapFieldAirUnpassable);
		}
		mf.Flags |= MapFieldGravel;
	}
	mf.Value = 0;

	//remove decorations if a wall, tree or rock was removed from the tile
	std::vector<CUnit *> table;
	Select(pos, pos, table, z);
	for (size_t i = 0; i != table.size(); ++i) {
		if (table[i]->Type->UnitType == UnitTypeLand && table[i]->Type->BoolFlag[DECORATION_INDEX].value) {
			LetUnitDie(*table[i]);			
		}
	}

	//check if any further tile should be removed with the clearing of this one
	if (!mf.OverlayTerrain->AllowSingle) {
		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
					if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
						CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
						
						if (adjacent_mf.OverlayTerrain == mf.OverlayTerrain && !adjacent_mf.OverlayTerrainDestroyed && !this->CurrentTerrainCanBeAt(adjacent_pos, true, z)) {
							this->ClearOverlayTile(adjacent_pos, z);
						}
					}
				}
			}
		}
	}
}
//Wyrmgus end

//Wyrmgus start
/*
/// Remove wood from the map.
void CMap::ClearWoodTile(const Vec2i &pos)
{
	CMapField &mf = *this->Field(pos);

	mf.setGraphicTile(this->Tileset->getRemovedTreeTile());
	mf.Flags &= ~(MapFieldForest | MapFieldUnpassable);
	mf.Value = 0;

	UI.Minimap.UpdateXY(pos);
	FixNeighbors(MapFieldForest, 0, pos);

	//maybe isExplored
	if (mf.playerInfo.IsExplored(*ThisPlayer)) {
		UI.Minimap.UpdateSeenXY(pos);
		MarkSeenTile(mf);
	}
}

/// Remove rock from the map.
void CMap::ClearRockTile(const Vec2i &pos)
{
	CMapField &mf = *this->Field(pos);

	mf.setGraphicTile(this->Tileset->getRemovedRockTile());
	mf.Flags &= ~(MapFieldRocks | MapFieldUnpassable);
	mf.Value = 0;
	
	UI.Minimap.UpdateXY(pos);
	FixNeighbors(MapFieldRocks, 0, pos);

	//maybe isExplored
	if (mf.playerInfo.IsExplored(*ThisPlayer)) {
		UI.Minimap.UpdateSeenXY(pos);
		MarkSeenTile(mf);
	}
}
*/
//Wyrmgus end

/**
**  Regenerate forest.
**
**  @param pos  Map tile pos
*/
//Wyrmgus start
//void CMap::RegenerateForestTile(const Vec2i &pos)
void CMap::RegenerateForestTile(const Vec2i &pos, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	Assert(Map.Info.IsPointOnMap(pos));
//	CMapField &mf = *this->Field(pos);
	Assert(Map.Info.IsPointOnMap(pos, z));
	CMapField &mf = *this->Field(pos, z);
	//Wyrmgus end

	//Wyrmgus start
//	if (mf.getGraphicTile() != this->Tileset->getRemovedTreeTile()) {
	if ((mf.OverlayTerrain && std::find(mf.OverlayTerrain->DestroyedTiles.begin(), mf.OverlayTerrain->DestroyedTiles.end(), mf.SolidTile) == mf.OverlayTerrain->DestroyedTiles.end()) || !(mf.getFlag() & MapFieldStumps)) {
	//Wyrmgus end
		return;
	}

	//  Increment each value of no wood.
	//  If grown up, place new wood.
	//  FIXME: a better looking result would be fine
	//    Allow general updates to any tiletype that regrows

	//Wyrmgus start
	/*
	const unsigned int occupedFlag = (MapFieldWall | MapFieldUnpassable | MapFieldLandUnit | MapFieldBuilding);
	++mf.Value;
	if (mf.Value < ForestRegeneration) {
		return;
	}
	mf.Value = ForestRegeneration;
	if ((mf.Flags & occupedFlag) || pos.y == 0) {
		return;
	}
	*/
	
	const unsigned int permanent_occupied_flag = (MapFieldWall | MapFieldUnpassable | MapFieldBuilding);
	const unsigned int occupedFlag = (MapFieldWall | MapFieldUnpassable | MapFieldLandUnit | MapFieldBuilding | MapFieldItem); // don't regrow forests under items
	
	if ((mf.Flags & permanent_occupied_flag)) { //if the tree tile is occupied by buildings and the like, reset the regeneration process
		mf.Value = 0;
		return;
	}

	if ((mf.Flags & occupedFlag) || pos.y == 0) {
		return;
	}
	
	++mf.Value;
	if (mf.Value < ForestRegeneration) {
		return;
	}
	mf.Value = ForestRegeneration;
	//Wyrmgus end
	
	//Wyrmgus start
//	const Vec2i offset(0, -1);
//	CMapField &topMf = *(&mf - this->Info.MapWidth);

	for (int x_offset = -1; x_offset <= 1; x_offset+=2) { //increment by 2 to avoid instances where it is 0
		for (int y_offset = -1; y_offset <= 1; y_offset+=2) {
			const Vec2i verticalOffset(0, y_offset);
			CMapField &verticalMf = *this->Field(pos + verticalOffset, z);
			const Vec2i horizontalOffset(x_offset, 0);
			CMapField &horizontalMf = *this->Field(pos + horizontalOffset, z);
			const Vec2i diagonalOffset(x_offset, y_offset);
			CMapField &diagonalMf = *this->Field(pos + diagonalOffset, z);
			
			if (
				this->Info.IsPointOnMap(pos + verticalOffset, z)
				&& ((verticalMf.OverlayTerrain && std::find(verticalMf.OverlayTerrain->DestroyedTiles.begin(), verticalMf.OverlayTerrain->DestroyedTiles.end(), verticalMf.SolidTile) == verticalMf.OverlayTerrain->DestroyedTiles.end() && (verticalMf.getFlag() & MapFieldStumps) && verticalMf.Value >= ForestRegeneration && !(verticalMf.Flags & occupedFlag)) || (verticalMf.getFlag() & MapFieldForest))
				&& this->Info.IsPointOnMap(pos + diagonalOffset, z)
				&& ((diagonalMf.OverlayTerrain && std::find(diagonalMf.OverlayTerrain->DestroyedTiles.begin(), diagonalMf.OverlayTerrain->DestroyedTiles.end(), diagonalMf.SolidTile) == diagonalMf.OverlayTerrain->DestroyedTiles.end() && (diagonalMf.getFlag() & MapFieldStumps) && diagonalMf.Value >= ForestRegeneration && !(diagonalMf.Flags & occupedFlag)) || (diagonalMf.getFlag() & MapFieldForest))
				&& this->Info.IsPointOnMap(pos + horizontalOffset, z)
				&& ((horizontalMf.OverlayTerrain && std::find(horizontalMf.OverlayTerrain->DestroyedTiles.begin(), horizontalMf.OverlayTerrain->DestroyedTiles.end(), horizontalMf.SolidTile) == horizontalMf.OverlayTerrain->DestroyedTiles.end() && (horizontalMf.getFlag() & MapFieldStumps) && horizontalMf.Value >= ForestRegeneration && !(horizontalMf.Flags & occupedFlag)) || (horizontalMf.getFlag() & MapFieldForest))
			) {
				DebugPrint("Real place wood\n");
				this->SetOverlayTerrainDestroyed(pos + verticalOffset, false, z);
				verticalMf.Value = DefaultResourceAmounts[WoodCost];
				
				this->SetOverlayTerrainDestroyed(pos + diagonalOffset, false, z);
				diagonalMf.Value = DefaultResourceAmounts[WoodCost];
				
				this->SetOverlayTerrainDestroyed(pos + horizontalOffset, false, z);
				horizontalMf.Value = DefaultResourceAmounts[WoodCost];
				
				this->SetOverlayTerrainDestroyed(pos, false, z);
				mf.Value = DefaultResourceAmounts[WoodCost];
				
				return;
			}
		}
	}

	/*
	if (topMf.getGraphicTile() == this->Tileset->getRemovedTreeTile()
		&& topMf.Value >= ForestRegeneration
		&& !(topMf.Flags & occupedFlag)) {
		DebugPrint("Real place wood\n");
		topMf.setTileIndex(*Map.Tileset, Map.Tileset->getTopOneTreeTile(), 0);
		topMf.setGraphicTile(Map.Tileset->getTopOneTreeTile());
		topMf.playerInfo.SeenTile = topMf.getGraphicTile();
		topMf.Value = 0;
		topMf.Flags |= MapFieldForest | MapFieldUnpassable;
		UI.Minimap.UpdateSeenXY(pos + offset);
		UI.Minimap.UpdateXY(pos + offset);
		
		mf.setTileIndex(*Map.Tileset, Map.Tileset->getBottomOneTreeTile(), 0);
		mf.setGraphicTile(Map.Tileset->getBottomOneTreeTile());
		mf.playerInfo.SeenTile = mf.getGraphicTile();
		mf.Value = 0;
		mf.Flags |= MapFieldForest | MapFieldUnpassable;
		UI.Minimap.UpdateSeenXY(pos);
		UI.Minimap.UpdateXY(pos);
		
		if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(mf);
		}
		if (Map.Field(pos + offset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(topMf);
		}
		FixNeighbors(MapFieldForest, 0, pos + offset);
		FixNeighbors(MapFieldForest, 0, pos);
	}
	*/
}

/**
**  Regenerate forest.
*/
void CMap::RegenerateForest()
{
	if (!ForestRegeneration) {
		return;
	}
	Vec2i pos;
	//Wyrmgus start
	/*
	for (pos.y = 0; pos.y < Info.MapHeight; ++pos.y) {
		for (pos.x = 0; pos.x < Info.MapWidth; ++pos.x) {
			RegenerateForestTile(pos);
		}
	}
	*/
	for (size_t z = 0; z < this->Fields.size(); ++z) {
		for (pos.y = 0; pos.y < Info.MapHeights[z]; ++pos.y) {
			for (pos.x = 0; pos.x < Info.MapWidths[z]; ++pos.x) {
				RegenerateForestTile(pos, z);
			}
		}
	}
	//Wyrmgus end
}


/**
**  Load the map presentation
**
**  @param mapname  map filename
*/
void LoadStratagusMapInfo(const std::string &mapname)
{
	// Set the default map setup by replacing .smp with .sms
	size_t loc = mapname.find(".smp");
	if (loc != std::string::npos) {
		Map.Info.Filename = mapname;
		Map.Info.Filename.replace(loc, 4, ".sms");
	}

	const std::string filename = LibraryFileName(mapname.c_str());
	LuaLoadFile(filename);
}

//@}
