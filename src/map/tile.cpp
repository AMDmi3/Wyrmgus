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
//      (c) Copyright 2013-2020 by Joris Dauphin and Andrettin
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

#include "map/tile.h"

//Wyrmgus start
#include "editor.h"
//Wyrmgus end
#include "iolib.h"
#include "map/map.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_feature.h"
#include "map/terrain_type.h"
#include "map/tileset.h"
#include "player.h"
#include "script.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "util/vector_util.h"

namespace wyrmgus {

tile::tile()
{
	this->player_info = std::make_unique<tile_player_info>();
}

const terrain_type *tile::get_top_terrain(const bool seen, const bool ignore_destroyed) const
{
	if (!seen) {
		if (this->get_overlay_terrain() != nullptr && (!ignore_destroyed || !this->OverlayTerrainDestroyed)) {
			return this->get_overlay_terrain();
		} else {
			return this->get_terrain();
		}
	} else {
		if (this->player_info->SeenOverlayTerrain) {
			return this->player_info->SeenOverlayTerrain;
		} else {
			return this->player_info->SeenTerrain;
		}
	}
}

bool tile::IsSeenTileCorrect() const
{
	return this->get_terrain() == this->player_info->SeenTerrain && this->get_overlay_terrain() == this->player_info->SeenOverlayTerrain && this->SolidTile == this->player_info->SeenSolidTile && this->OverlaySolidTile == this->player_info->SeenOverlaySolidTile && this->TransitionTiles == this->player_info->SeenTransitionTiles && this->OverlayTransitionTiles == this->player_info->SeenOverlayTransitionTiles;
}

const resource *tile::get_resource() const
{
	if (this->get_overlay_terrain() != nullptr && !this->OverlayTerrainDestroyed) {
		return this->get_overlay_terrain()->get_resource();
	}

	return nullptr;
}

bool tile::is_destroyed_tree_tile() const
{
	return this->get_overlay_terrain() != nullptr && this->OverlayTerrainDestroyed && (this->get_flags() & MapFieldStumps);
}

//Wyrmgus start
/**
**	@brief	Set the tile's terrain type
**
**	@param	terrain_type	The new terrain type for the tile
*/
void tile::SetTerrain(const terrain_type *terrain_type)
{
	if (!terrain_type) {
		return;
	}

	//remove the flags of the old terrain type
	if (terrain_type->is_overlay()) {
		if (this->get_overlay_terrain() == terrain_type) {
			return;
		}
		if (this->get_overlay_terrain() != nullptr) {
			this->Flags &= ~(this->get_overlay_terrain()->Flags);

			if (this->OverlayTerrainDestroyed) {
				if (this->get_overlay_terrain()->Flags & MapFieldForest) {
					this->Flags &= ~(MapFieldStumps);
				}

				if (((this->get_overlay_terrain()->Flags & MapFieldRocks) || (this->get_overlay_terrain()->Flags & MapFieldWall)) && !(this->get_terrain()->Flags & MapFieldGravel)) {
					this->Flags &= ~(MapFieldGravel);
				}
			}
			this->Flags &= ~(MapFieldGravel);
		}
	} else {
		if (this->get_terrain() == terrain_type) {
			return;
		}
		if (this->get_terrain() != nullptr) {
			this->Flags &= ~(this->get_terrain()->Flags);
		}
	}

	if (terrain_type->is_overlay()) {
		if (this->get_terrain() == nullptr || !vector::contains(terrain_type->get_base_terrain_types(), this->get_terrain())) {
			this->SetTerrain(terrain_type->get_base_terrain_types().front());
		}
		if ((terrain_type->Flags & MapFieldWaterAllowed) || terrain_type->Flags & MapFieldSpace) {
			this->Flags &= ~(this->get_terrain()->Flags); // if the overlay is water or space, remove all flags from the base terrain
			if (terrain_type->Flags & MapFieldWaterAllowed) {
				this->Flags &= ~(MapFieldCoastAllowed); // need to do this manually, since MapFieldCoast is added dynamically
			}
		}
		this->overlay_terrain = terrain_type;
		this->OverlayTerrainDestroyed = false;
		this->OverlayTerrainDamaged = false;
	} else {
		this->terrain = terrain_type;
		if (this->get_overlay_terrain() != nullptr && !vector::contains(this->get_overlay_terrain()->get_base_terrain_types(), terrain_type)) { //if the overlay terrain is incompatible with the new base terrain, remove the overlay
			this->Flags &= ~(this->get_overlay_terrain()->Flags);
			this->Flags &= ~(MapFieldCoastAllowed); // need to do this manually, since MapFieldCoast is added dynamically
			this->overlay_terrain = nullptr;
			this->OverlayTransitionTiles.clear();
		}
	}

	if (Editor.Running == EditorNotRunning && terrain_type->SolidAnimationFrames > 0) {
		if (terrain_type->is_overlay()) {
			this->OverlayAnimationFrame = SyncRand(terrain_type->SolidAnimationFrames);
		} else {
			this->AnimationFrame = SyncRand(terrain_type->SolidAnimationFrames);
		}
	} else {
		if (terrain_type->is_overlay()) {
			this->OverlayAnimationFrame = 0;
		} else {
			this->AnimationFrame = 0;
		}
	}

	//apply the flags from the new terrain type
	this->Flags |= terrain_type->Flags;
	const CUnitCache &cache = this->UnitCache;
	for (size_t i = 0; i != cache.size(); ++i) {
		CUnit &unit = *cache[i];
		if (unit.IsAliveOnMap()) {
			if (unit.Type->BoolFlag[AIRUNPASSABLE_INDEX].value) { // restore MapFieldAirUnpassable related to units (i.e. doors)
				this->Flags |= MapFieldUnpassable;
				this->Flags |= MapFieldAirUnpassable;
			}
			const unit_type_variation *variation = unit.GetVariation();
			if (variation != nullptr && !unit.can_have_variation(variation)) { // if a unit that is on the tile has a terrain-dependent variation that is not compatible with the current variation, repick the unit's variation
				unit.ChooseVariation();
			}
		}
	}

	this->update_movement_cost();

	if (this->Flags & MapFieldRailroad) {
		this->Flags &= ~(MapFieldNoRail);
	} else {
		this->Flags |= MapFieldNoRail;
	}

	if (terrain_type->is_overlay() && (this->Flags & MapFieldUnderground) && (this->Flags & MapFieldWall)) {
		//underground walls are not passable by air units
		this->Flags |= MapFieldAirUnpassable;
	}

	//wood and rock tiles must always begin with the default value for their respective resource types
	if (terrain_type->is_overlay() && terrain_type->get_resource() != nullptr) {
		this->value = terrain_type->get_resource()->get_default_amount();
	} else if ((terrain_type->Flags & MapFieldWall) && terrain_type->UnitType) {
		this->value = terrain_type->UnitType->MapDefaultStat.Variables[HP_INDEX].Max;
	}

	//remove the terrain feature, unless it is a trade route and a pathway is being built over it
	if (this->get_terrain_feature() != nullptr && (!this->get_terrain_feature()->is_trade_route() || !terrain_type->is_pathway())) {
		//only remove the terrain feature if we are changing the top terrain type
		if (terrain_type->is_overlay() || !this->get_terrain_feature()->get_terrain_type()->is_overlay()) {
			this->terrain_feature = nullptr;
		}
	}
}

void tile::RemoveOverlayTerrain()
{
	if (this->get_overlay_terrain() == nullptr) {
		return;
	}

	if (this->get_resource() != nullptr && this->get_settlement() != nullptr) {
		//decrement the resource tile count for the tile's settlement
		//forest tiles aren't decremented on overlay destruction, since they can regrow, so we need to decrement them now even if the overlay terrain has already been destroyed
		const bool already_decremented = this->OverlayTerrainDestroyed && !(this->get_overlay_terrain()->Flags & MapFieldForest);
		if (!already_decremented) {
			this->get_settlement()->get_game_data()->decrement_resource_tile_count(this->get_resource());
		}
	}

	this->value = 0;
	this->Flags &= ~(this->get_overlay_terrain()->Flags);

	this->Flags &= ~(MapFieldCoastAllowed); // need to do this manually, since MapFieldCoast is added dynamically
	this->overlay_terrain = nullptr;
	this->OverlayTerrainDestroyed = false;
	this->OverlayTerrainDamaged = false;
	this->OverlayTransitionTiles.clear();

	this->Flags |= this->get_terrain()->Flags;
	// restore MapFieldAirUnpassable related to units (i.e. doors)
	const CUnitCache &cache = this->UnitCache;
	for (size_t i = 0; i != cache.size(); ++i) {
		CUnit &unit = *cache[i];
		if (unit.IsAliveOnMap() && unit.Type->BoolFlag[AIRUNPASSABLE_INDEX].value) {
			this->Flags |= MapFieldUnpassable;
			this->Flags |= MapFieldAirUnpassable;
		}
	}

	this->update_movement_cost();

	if (this->Flags & MapFieldRailroad) {
		this->Flags &= ~(MapFieldNoRail);
	} else {
		this->Flags |= MapFieldNoRail;
	}

	if (this->get_terrain_feature() != nullptr) {
		this->terrain_feature = nullptr;
	}
}

void tile::SetOverlayTerrainDestroyed(bool destroyed)
{
	if (this->get_overlay_terrain() == nullptr || this->OverlayTerrainDestroyed == destroyed) {
		return;
	}

	if (this->get_resource() != nullptr && this->get_settlement() != nullptr) {
		//decrement the resource tile count for the tile's settlement
		//forest tiles aren't decremented on overlay destruction, since they can regrow
		if (!(this->get_overlay_terrain()->Flags & MapFieldForest)) {
			this->get_settlement()->get_game_data()->decrement_resource_tile_count(this->get_resource());
		}
	}

	this->OverlayTerrainDestroyed = destroyed;
	this->update_movement_cost();
}

void tile::SetOverlayTerrainDamaged(bool damaged)
{
	if (this->get_overlay_terrain() == nullptr || this->OverlayTerrainDamaged == damaged) {
		return;
	}

	this->OverlayTerrainDamaged = damaged;
}
//Wyrmgus end

void tile::setTileIndex(const CTileset &tileset, unsigned int tileIndex, int value)
{
	const CTile &tile = tileset.tiles[tileIndex];
	//Wyrmgus start
//	this->tile = tile.tile;
	//Wyrmgus end
	this->value = value;
	//Wyrmgus start
	/*
#if 0
	this->Flags = tile.flag;
#else
	this->Flags &= ~(MapFieldLandAllowed | MapFieldCoastAllowed |
					 MapFieldWaterAllowed | MapFieldNoBuilding | MapFieldUnpassable |
					 //Wyrmgus start
//					 MapFieldWall | MapFieldRocks | MapFieldForest);
					 MapFieldWall | MapFieldRocks | MapFieldForest |
					 MapFieldAirUnpassable | MapFieldDirt | MapFieldGrass |
					 MapFieldGravel | MapFieldMud | MapFieldStoneFloor | MapFieldStumps);
					 //Wyrmgus end
	this->Flags |= tile.flag;
#endif
	this->movement_cost = 1 << (tile.flag & MapFieldSpeedMask);
#ifdef DEBUG
	this->tilesetTile = tileIndex;
#endif
	*/
	//Wyrmgus end

	//Wyrmgus start
	terrain_type *terrain = terrain_type::get(tileset.getTerrainName(tile.tileinfo.BaseTerrain));
	if (terrain->is_overlay()) {
		if (terrain->Flags & MapFieldForest) {
			this->SetTerrain(terrain_type::get(tileset.solidTerrainTypes[3].TerrainName));
		} else if (terrain->Flags & MapFieldRocks || terrain->Flags & MapFieldWaterAllowed) {
			this->SetTerrain(terrain_type::get(tileset.solidTerrainTypes[2].TerrainName));
		}
	}
	this->SetTerrain(terrain);
	if (!terrain) {
		fprintf(stderr, "Terrain type \"%s\" doesn't exist.\n", tileset.getTerrainName(tile.tileinfo.BaseTerrain).c_str());
	}
	//Wyrmgus end
}

//Wyrmgus start
void tile::UpdateSeenTile()
{
	this->player_info->SeenTerrain = this->get_terrain();
	this->player_info->SeenOverlayTerrain = this->get_overlay_terrain();
	this->player_info->SeenSolidTile = this->SolidTile;
	this->player_info->SeenOverlaySolidTile = this->OverlaySolidTile;
	this->player_info->SeenTransitionTiles.clear();
	this->player_info->SeenTransitionTiles = this->TransitionTiles;
	this->player_info->SeenOverlayTransitionTiles.clear();
	this->player_info->SeenOverlayTransitionTiles = this->OverlayTransitionTiles;
}
//Wyrmgus end

void tile::Save(CFile &file) const
{
	const wyrmgus::terrain_feature *terrain_feature = this->get_terrain_feature();

	file.printf("  {\"%s\", \"%s\", \"%s\", %s, %s, \"%s\", \"%s\", %d, %d, %d, %d, %2d, %2d, %2d, \"%s\"", (this->get_terrain() != nullptr ? this->get_terrain()->get_identifier().c_str() : ""), (this->get_overlay_terrain() != nullptr ? this->get_overlay_terrain()->get_identifier().c_str() : ""), (terrain_feature != nullptr ? terrain_feature->get_identifier().c_str() : ""), OverlayTerrainDamaged ? "true" : "false", OverlayTerrainDestroyed ? "true" : "false", player_info->SeenTerrain ? player_info->SeenTerrain->Ident.c_str() : "", player_info->SeenOverlayTerrain ? player_info->SeenOverlayTerrain->Ident.c_str() : "", SolidTile, OverlaySolidTile, player_info->SeenSolidTile, player_info->SeenOverlaySolidTile, this->get_value(), this->get_movement_cost(), Landmass, this->get_settlement() != nullptr ? this->get_settlement()->get_identifier().c_str() : "");

	for (size_t i = 0; i != TransitionTiles.size(); ++i) {
		file.printf(", \"transition-tile\", \"%s\", %d", TransitionTiles[i].first->Ident.c_str(), TransitionTiles[i].second);
	}

	for (size_t i = 0; i != OverlayTransitionTiles.size(); ++i) {
		file.printf(", \"overlay-transition-tile\", \"%s\", %d", OverlayTransitionTiles[i].first->Ident.c_str(), OverlayTransitionTiles[i].second);
	}

	for (size_t i = 0; i != player_info->SeenTransitionTiles.size(); ++i) {
		file.printf(", \"seen-transition-tile\", \"%s\", %d", player_info->SeenTransitionTiles[i].first->Ident.c_str(), player_info->SeenTransitionTiles[i].second);
	}

	for (size_t i = 0; i != player_info->SeenOverlayTransitionTiles.size(); ++i) {
		file.printf(", \"seen-overlay-transition-tile\", \"%s\", %d", player_info->SeenOverlayTransitionTiles[i].first->Ident.c_str(), player_info->SeenOverlayTransitionTiles[i].second);
	}
	//Wyrmgus end
	for (int i = 0; i != PlayerMax; ++i) {
		if (player_info->Visible[i] == 1) {
			file.printf(", \"explored\", %d", i);
		}
	}
	if (Flags & MapFieldLandAllowed) {
		file.printf(", \"land\"");
	}
	if (Flags & MapFieldCoastAllowed) {
		file.printf(", \"coast\"");
	}
	if (Flags & MapFieldWaterAllowed) {
		file.printf(", \"water\"");
	}
	if (Flags & MapFieldSpace) {
		file.printf(", \"space\"");
	}
	if (Flags & MapFieldUnderground) {
		file.printf(", \"underground\"");
	}
	if (Flags & MapFieldNoBuilding) {
		//Wyrmgus start
//		file.printf(", \"mud\"");
		file.printf(", \"no-building\"");
		//Wyrmgus end
	}
	if (Flags & MapFieldUnpassable) {
		file.printf(", \"block\"");
	}
	if (Flags & MapFieldWall) {
		file.printf(", \"wall\"");
	}
	if (Flags & MapFieldRocks) {
		file.printf(", \"rock\"");
	}
	if (Flags & MapFieldForest) {
		file.printf(", \"wood\"");
	}
	//Wyrmgus start
	if (Flags & MapFieldAirUnpassable) {
		file.printf(", \"air-unpassable\"");
	}
	if (Flags & MapFieldDesert) {
		file.printf(", \"desert\"");
	}
	if (Flags & MapFieldDirt) {
		file.printf(", \"dirt\"");
	}
	if (Flags & MapFieldIce) {
		file.printf(", \"ice\"");
	}
	if (Flags & MapFieldGrass) {
		file.printf(", \"grass\"");
	}
	if (Flags & MapFieldGravel) {
		file.printf(", \"gravel\"");
	}
	if (Flags & MapFieldMud) {
		file.printf(", \"mud\"");
	}
	if (Flags & MapFieldRailroad) {
		file.printf(", \"railroad\"");
	}
	if (Flags & MapFieldRoad) {
		file.printf(", \"road\"");
	}
	if (Flags & MapFieldNoRail) {
		file.printf(", \"no-rail\"");
	}
	if (Flags & MapFieldSnow) {
		file.printf(", \"snow\"");
	}
	if (Flags & MapFieldStoneFloor) {
		file.printf(", \"stone_floor\"");
	}
	if (Flags & MapFieldStumps) {
		file.printf(", \"stumps\"");
	}

#if 1
	// Not Required for save
	// These are required for now, UnitType::FieldFlags is 0 until
	// UpdateStats is called which is after the game is loaded
	if (Flags & MapFieldLandUnit) {
		file.printf(", \"ground\"");
	}
	if (Flags & MapFieldAirUnit) {
		file.printf(", \"air\"");
	}
	if (Flags & MapFieldSeaUnit) {
		file.printf(", \"sea\"");
	}
	if (Flags & MapFieldBuilding) {
		file.printf(", \"building\"");
	}
	//Wyrmgus start
	if (Flags & MapFieldItem) {
		file.printf(", \"item\"");
	}
	if (Flags & MapFieldBridge) {
		file.printf(", \"bridge\"");
	}
	//Wyrmgus end
#endif
	file.printf("}");
}


void tile::parse(lua_State *l)
{
	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	const int len = lua_rawlen(l, -1);
	//Wyrmgus start
//	if (len < 4) {
	if (len < 14) {
		//Wyrmgus end
		LuaError(l, "incorrect argument");
	}

	//Wyrmgus start
	/*
	this->tile = LuaToNumber(l, -1, 1);
	this->player_info->SeenTile = LuaToNumber(l, -1, 2);
	this->Value = LuaToNumber(l, -1, 3);
	this->movement_cost = LuaToNumber(l, -1, 4);
	*/
	const std::string terrain_ident = LuaToString(l, -1, 1);
	if (!terrain_ident.empty()) {
		this->terrain = terrain_type::get(terrain_ident);
	}

	const std::string overlay_terrain_ident = LuaToString(l, -1, 2);
	if (!overlay_terrain_ident.empty()) {
		this->overlay_terrain = terrain_type::get(overlay_terrain_ident);
	}

	const std::string terrain_feature_ident = LuaToString(l, -1, 3);
	if (!terrain_feature_ident.empty()) {
		this->terrain_feature = terrain_feature::get(terrain_feature_ident);
	}

	this->SetOverlayTerrainDamaged(LuaToBoolean(l, -1, 4));
	this->SetOverlayTerrainDestroyed(LuaToBoolean(l, -1, 5));

	const std::string seen_terrain_ident = LuaToString(l, -1, 6);
	if (!seen_terrain_ident.empty()) {
		this->player_info->SeenTerrain = terrain_type::get(seen_terrain_ident);
	}

	const std::string seen_overlay_terrain_ident = LuaToString(l, -1, 7);
	if (!seen_overlay_terrain_ident.empty()) {
		this->player_info->SeenOverlayTerrain = terrain_type::get(seen_overlay_terrain_ident);
	}

	this->SolidTile = LuaToNumber(l, -1, 8);
	this->OverlaySolidTile = LuaToNumber(l, -1, 9);
	this->player_info->SeenSolidTile = LuaToNumber(l, -1, 10);
	this->player_info->SeenOverlaySolidTile = LuaToNumber(l, -1, 11);
	this->value = LuaToNumber(l, -1, 12);
	this->movement_cost = LuaToNumber(l, -1, 13);
	this->Landmass = LuaToNumber(l, -1, 14);
	const std::string settlement_identifier = LuaToString(l, -1, 15);
	if (!settlement_identifier.empty()) {
		this->settlement = site::get(settlement_identifier);
	}
	//Wyrmgus end

	for (int j = 15; j < len; ++j) {
		const char *value = LuaToString(l, -1, j + 1);

		//Wyrmgus start
//		if (!strcmp(value, "explored")) {
		if (!strcmp(value, "transition-tile")) {
			++j;
			terrain_type *terrain = terrain_type::get(LuaToString(l, -1, j + 1));
			++j;
			int tile_number = LuaToNumber(l, -1, j + 1);
			this->TransitionTiles.push_back(std::pair<terrain_type *, int>(terrain, tile_number));
		} else if (!strcmp(value, "overlay-transition-tile")) {
			++j;
			terrain_type *terrain = terrain_type::get(LuaToString(l, -1, j + 1));
			++j;
			int tile_number = LuaToNumber(l, -1, j + 1);
			this->OverlayTransitionTiles.push_back(std::pair<terrain_type *, int>(terrain, tile_number));
		} else if (!strcmp(value, "seen-transition-tile")) {
			++j;
			terrain_type *terrain = terrain_type::get(LuaToString(l, -1, j + 1));
			++j;
			int tile_number = LuaToNumber(l, -1, j + 1);
			this->player_info->SeenTransitionTiles.push_back(std::pair<terrain_type *, int>(terrain, tile_number));
		} else if (!strcmp(value, "seen-overlay-transition-tile")) {
			++j;
			terrain_type *terrain = terrain_type::get(LuaToString(l, -1, j + 1));
			++j;
			int tile_number = LuaToNumber(l, -1, j + 1);
			this->player_info->SeenOverlayTransitionTiles.push_back(std::pair<terrain_type *, int>(terrain, tile_number));
		} else if (!strcmp(value, "explored")) {
			//Wyrmgus end
			++j;
			this->player_info->Visible[LuaToNumber(l, -1, j + 1)] = 1;
		} else if (!strcmp(value, "land")) {
			this->Flags |= MapFieldLandAllowed;
		} else if (!strcmp(value, "coast")) {
			this->Flags |= MapFieldCoastAllowed;
		} else if (!strcmp(value, "water")) {
			this->Flags |= MapFieldWaterAllowed;
			//Wyrmgus start
	//		} else if (!strcmp(value, "mud")) {
		} else if (!strcmp(value, "no-building")) {
			//Wyrmgus end
			this->Flags |= MapFieldNoBuilding;
		} else if (!strcmp(value, "block")) {
			this->Flags |= MapFieldUnpassable;
		} else if (!strcmp(value, "wall")) {
			this->Flags |= MapFieldWall;
		} else if (!strcmp(value, "rock")) {
			this->Flags |= MapFieldRocks;
		} else if (!strcmp(value, "wood")) {
			this->Flags |= MapFieldForest;
		} else if (!strcmp(value, "ground")) {
			this->Flags |= MapFieldLandUnit;
		} else if (!strcmp(value, "air")) {
			this->Flags |= MapFieldAirUnit;
		} else if (!strcmp(value, "sea")) {
			this->Flags |= MapFieldSeaUnit;
		} else if (!strcmp(value, "building")) {
			this->Flags |= MapFieldBuilding;
		} else if (!strcmp(value, "item")) {
			this->Flags |= MapFieldItem;
		} else if (!strcmp(value, "bridge")) {
			this->Flags |= MapFieldBridge;
		} else if (!strcmp(value, "air-unpassable")) {
			this->Flags |= MapFieldAirUnpassable;
		} else if (!strcmp(value, "desert")) {
			this->Flags |= MapFieldDesert;
		} else if (!strcmp(value, "dirt")) {
			this->Flags |= MapFieldDirt;
		} else if (!strcmp(value, "grass")) {
			this->Flags |= MapFieldGrass;
		} else if (!strcmp(value, "gravel")) {
			this->Flags |= MapFieldGravel;
		} else if (!strcmp(value, "ice")) {
			this->Flags |= MapFieldIce;
		} else if (!strcmp(value, "mud")) {
			this->Flags |= MapFieldMud;
		} else if (!strcmp(value, "railroad")) {
			this->Flags |= MapFieldRailroad;
		} else if (!strcmp(value, "road")) {
			this->Flags |= MapFieldRoad;
		} else if (!strcmp(value, "no-rail")) {
			this->Flags |= MapFieldNoRail;
		} else if (!strcmp(value, "snow")) {
			this->Flags |= MapFieldSnow;
		} else if (!strcmp(value, "stone_floor")) {
			this->Flags |= MapFieldStoneFloor;
		} else if (!strcmp(value, "stumps")) {
			this->Flags |= MapFieldStumps;
		} else if (!strcmp(value, "underground")) {
			this->Flags |= MapFieldUnderground;
		} else if (!strcmp(value, "space")) {
			this->Flags |= MapFieldSpace;
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
}

/// Check if a field flags.
bool tile::CheckMask(const int mask) const
{
	//Wyrmgus start
	//for purposes of this check, don't count MapFieldWaterAllowed and MapFieldCoastAllowed if there is a bridge present
	int check_flags = this->Flags;
	if (check_flags & MapFieldBridge) {
		check_flags &= ~(MapFieldWaterAllowed | MapFieldCoastAllowed);
	}
	//	return (this->Flags & mask) != 0;
	return (check_flags & mask) != 0;
	//Wyrmgus end
}

bool tile::is_water() const
{
	return this->Flags & (MapFieldWaterAllowed | MapFieldCoastAllowed);
}

bool tile::is_non_coastal_water() const
{
	return this->Flags & MapFieldWaterAllowed;
}

bool tile::is_coastal_water() const
{
	return this->Flags & MapFieldCoastAllowed;
}

bool tile::is_river() const
{
	return this->get_terrain_feature() != nullptr && this->get_terrain_feature()->is_river();
}

bool tile::is_space() const
{
	return this->Flags & MapFieldSpace;
}

/// Returns true, if water on the map tile field
bool tile::WaterOnMap() const
{
	return CheckMask(MapFieldWaterAllowed);
}

/// Returns true, if coast on the map tile field
bool tile::CoastOnMap() const
{
	return CheckMask(MapFieldCoastAllowed);
}

/// Returns true, if water on the map tile field
bool tile::ForestOnMap() const
{
	return CheckMask(MapFieldForest);
}

/// Returns true, if coast on the map tile field
bool tile::RockOnMap() const
{
	return CheckMask(MapFieldRocks);
}

bool tile::isAWall() const
{
	return CheckMask(MapFieldWall);
}

void tile::update_movement_cost()
{
	this->movement_cost = DefaultTileMovementCost; // default speed
	this->movement_cost -= this->get_top_terrain(false, true)->get_movement_bonus();
}

CPlayer *tile::get_owner() const
{
	if (this->get_settlement() == nullptr) {
		return nullptr;
	}

	return this->get_settlement()->get_game_data()->get_owner();
}

CPlayer *tile::get_realm_owner() const
{
	if (this->get_settlement() == nullptr) {
		return nullptr;
	}

	return this->get_settlement()->get_game_data()->get_realm_owner();
}

bool tile::is_on_trade_route() const
{
	return this->get_terrain_feature() != nullptr && this->get_terrain_feature()->is_trade_route();
}

//
//  tile_player_info
//

unsigned char tile_player_info::TeamVisibilityState(const CPlayer &player) const
{
	if (this->IsVisible(player)) {
		return 2;
	}

	unsigned char maxVision = 0;

	if (this->IsExplored(player)) {
		maxVision = 1;
	}

	const int player_index = player.Index;
	for (const int i : player.get_shared_vision()) {
		if (CPlayer::Players[i]->has_shared_vision_with(player_index)) { //if the shared vision is mutual
			maxVision = std::max<unsigned char>(maxVision, this->Visible[i]);
			if (maxVision >= 2) {
				return 2;
			}
		}
	}

	for (const CPlayer *other_player : CPlayer::get_revealed_players()) {
		const int other_player_index = other_player->Index;
		if (this->Visible[other_player_index] < 2) { //don't show a revealed player's explored tiles, only the currently visible ones
			continue;
		}

		maxVision = std::max<unsigned char>(maxVision, Visible[other_player_index]);
		if (maxVision >= 2) {
			return 2;
		}
	}

	if (maxVision == 1 && CMap::Map.NoFogOfWar) {
		return 2;
	}

	return maxVision;
}

bool tile_player_info::IsExplored(const CPlayer &player) const
{
	return Visible[player.Index] != 0;
}

//Wyrmgus start
bool tile_player_info::IsTeamExplored(const CPlayer &player) const
{
	return Visible[player.Index] != 0 || TeamVisibilityState(player) != 0;
}
//Wyrmgus end

bool tile_player_info::IsVisible(const CPlayer &player) const
{
	const bool fogOfWar = !CMap::Map.NoFogOfWar;
	return Visible[player.Index] >= 2 || (!fogOfWar && IsExplored(player));
}

bool tile_player_info::IsTeamVisible(const CPlayer &player) const
{
	return TeamVisibilityState(player) == 2;
}

}
