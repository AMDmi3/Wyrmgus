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

#include "iolib.h"
//Wyrmgus start
#include <fstream> //for the 0 AD map conversion
//Wyrmgus end
#include "player.h"
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

CMap Map;                   /// The current map
int FlagRevealMap;          /// Flag must reveal the map
int ReplayRevealMap;        /// Reveal Map is replay
int ForestRegeneration;     /// Forest regeneration
char CurrentMapPath[1024];  /// Path of the current map

/*----------------------------------------------------------------------------
--  Visible and explored handling
----------------------------------------------------------------------------*/

/**
**  Marks seen tile -- used mainly for the Fog Of War
**
**  @param mf  MapField-position.
*/
void CMap::MarkSeenTile(CMapField &mf)
{
	const unsigned int tile = mf.getGraphicTile();
	const unsigned int seentile = mf.playerInfo.SeenTile;

	//  Nothing changed? Seeing already the correct tile.
	if (tile == seentile) {
		return;
	}
	mf.playerInfo.SeenTile = tile;

#ifdef MINIMAP_UPDATE
	//rb - GRRRRRRRRRRRR
	const unsigned int index = &mf - Map.Fields;
	const int y = index / Info.MapWidth;
	const int x = index - (y * Info.MapWidth);
	const Vec2i pos = {x, y}
#endif

	if (this->Tileset->TileTypeTable.empty() == false) {
#ifndef MINIMAP_UPDATE
		//rb - GRRRRRRRRRRRR
		const unsigned int index = &mf - Map.Fields;
		const int y = index / Info.MapWidth;
		const int x = index - (y * Info.MapWidth);
		const Vec2i pos(x, y);
#endif

		//  Handle wood changes. FIXME: check if for growing wood correct?
		//Wyrmgus start
//		if (tile == this->Tileset->getRemovedTreeTile()) {
		if (tile == this->Tileset->getRemovedTreeTile() && mf.getFlag() & MapFieldStumps) {
		//Wyrmgus end
			FixNeighbors(MapFieldForest, 1, pos);
		//Wyrmgus start
//		} else if (seentile == this->Tileset->getRemovedTreeTile()) {
		} else if (seentile == this->Tileset->getRemovedTreeTile() && ((mf.getFlag() & MapFieldStumps) || (mf.getFlag() & MapFieldForest))) {
		//Wyrmgus end
			FixTile(MapFieldForest, 1, pos);
		} else if (mf.ForestOnMap()) {
			FixTile(MapFieldForest, 1, pos);
			FixNeighbors(MapFieldForest, 1, pos);

			// Handle rock changes.
		//Wyrmgus start
//		} else if (tile == Tileset->getRemovedRockTile()) {
		} else if (tile == Tileset->getRemovedRockTile() && mf.getFlag() & MapFieldGravel) {
		//Wyrmgus end
			FixNeighbors(MapFieldRocks, 1, pos);
		//Wyrmgus start
//		} else if (seentile == Tileset->getRemovedRockTile()) {
		} else if (seentile == Tileset->getRemovedRockTile() && ((mf.getFlag() & MapFieldGravel) || (mf.getFlag() & MapFieldRocks))) {
		//Wyrmgus end
			FixTile(MapFieldRocks, 1, pos);
		} else if (mf.RockOnMap()) {
			FixTile(MapFieldRocks, 1, pos);
			FixNeighbors(MapFieldRocks, 1, pos);

			//  Handle Walls changes.
		} else if (this->Tileset->isAWallTile(tile)
				   || this->Tileset->isAWallTile(seentile)) {
			MapFixSeenWallTile(pos);
			MapFixSeenWallNeighbors(pos);
		}
	}

#ifdef MINIMAP_UPDATE
	UI.Minimap.UpdateXY(pos);
#endif
}

/**
**  Reveal the entire map.
*/
void CMap::Reveal()
{
	//  Mark every explored tile as visible. 1 turns into 2.
	for (int i = 0; i != this->Info.MapWidth * this->Info.MapHeight; ++i) {
		CMapField &mf = *this->Field(i);
		CMapFieldPlayerInfo &playerInfo = mf.playerInfo;
		for (int p = 0; p < PlayerMax; ++p) {
			playerInfo.Visible[p] = std::max<unsigned short>(1, playerInfo.Visible[p]);
		}
		MarkSeenTile(mf);
	}
	//  Global seen recount. Simple and effective.
	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
		CUnit &unit = **it;
		//  Reveal neutral buildings. Gold mines:)
		if (unit.Player->Type == PlayerNeutral) {
			for (int p = 0; p < PlayerMax; ++p) {
				if (Players[p].Type != PlayerNobody && (!(unit.Seen.ByPlayer & (1 << p)))) {
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

/**
**  Wall on map tile.
**
**  @param pos  map tile position.
**
**  @return    True if wall, false otherwise.
*/
bool CMap::WallOnMap(const Vec2i &pos) const
{
	Assert(Map.Info.IsPointOnMap(pos));
	return Field(pos)->isAWall();
}

/**
**  Human wall on map tile.
**
**  @param pos  map tile position.
**
**  @return    True if human wall, false otherwise.
*/
bool CMap::HumanWallOnMap(const Vec2i &pos) const
{
	Assert(Map.Info.IsPointOnMap(pos));
	return Field(pos)->isAHumanWall();
}

/**
**  Orc wall on map tile.
**
**  @param pos  map tile position.
**
**  @return    True if orcish wall, false otherwise.
*/
bool CMap::OrcWallOnMap(const Vec2i &pos) const
{
	Assert(Map.Info.IsPointOnMap(pos));
	return Field(pos)->isAOrcWall();
}

/**
**  Can move to this point, applying mask.
**
**  @param pos   map tile position.
**  @param mask  Mask for movement to apply.
**
**  @return      True if could be entered, false otherwise.
*/
bool CheckedCanMoveToMask(const Vec2i &pos, int mask)
{
	return Map.Info.IsPointOnMap(pos) && CanMoveToMask(pos, mask);
}

/**
**  Can a unit of unit-type be placed at this point.
**
**  @param type  unit-type to be checked.
**  @param pos   map tile position.
**
**  @return      True if could be entered, false otherwise.
*/
bool UnitTypeCanBeAt(const CUnitType &type, const Vec2i &pos)
{
	const int mask = type.MovementMask;
	unsigned int index = pos.y * Map.Info.MapWidth;

	for (int addy = 0; addy < type.TileHeight; ++addy) {
		for (int addx = 0; addx < type.TileWidth; ++addx) {
			if (Map.Info.IsPointOnMap(pos.x + addx, pos.y + addy) == false
				|| Map.Field(pos.x + addx + index)->CheckMask(mask) == true) {
				return false;
			}
		}
		index += Map.Info.MapWidth;
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
bool UnitCanBeAt(const CUnit &unit, const Vec2i &pos)
{
	Assert(unit.Type);
	if (unit.Type->BoolFlag[NONSOLID_INDEX].value) {
		return true;
	}
	return UnitTypeCanBeAt(*unit.Type, pos);
}

/**
**  Fixes initially the wood and seen tiles.
*/
void PreprocessMap()
{
	for (int ix = 0; ix < Map.Info.MapWidth; ++ix) {
		for (int iy = 0; iy < Map.Info.MapHeight; ++iy) {
			CMapField &mf = *Map.Field(ix, iy);
			mf.playerInfo.SeenTile = mf.getGraphicTile();
		}
	}
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
}

/**
**  Clear CMapInfo.
*/
void CMapInfo::Clear()
{
	this->Description.clear();
	this->Filename.clear();
	this->MapWidth = this->MapHeight = 0;
	memset(this->PlayerSide, 0, sizeof(this->PlayerSide));
	memset(this->PlayerType, 0, sizeof(this->PlayerType));
	this->MapUID = 0;
}

CMap::CMap() : Fields(NULL), NoFogOfWar(false), TileGraphic(NULL)
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
	Assert(!this->Fields);

	this->Fields = new CMapField[this->Info.MapWidth * this->Info.MapHeight];
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
	delete[] this->Fields;

	// Tileset freed by Tileset?

	this->Info.Clear();
	this->Fields = NULL;
	this->NoFogOfWar = false;
	//Wyrmgus start
	for (size_t i = 0; i != Map.Tileset->solidTerrainTypes.size(); ++i) {
		if (!Map.Tileset->solidTerrainTypes[i].ImageFile.empty()) {
			CGraphic::Free(this->SolidTileGraphics[i]);
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
	file.printf("  \"map-fields\", {\n");
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
void CMap::FixTile(unsigned short type, int seen, const Vec2i &pos)
{
	Assert(type == MapFieldForest || type == MapFieldRocks);

	//  Outside of map or no wood.
	if (!Info.IsPointOnMap(pos)) {
		return;
	}
	unsigned int index = getIndex(pos);
	CMapField &mf = *this->Field(index);

	//Wyrmgus start
	/*
	if (!((type == MapFieldForest && Tileset->isAWoodTile(mf.playerInfo.SeenTile))
		  || (type == MapFieldRocks && Tileset->isARockTile(mf.playerInfo.SeenTile)))) {
		if (seen) {
			return;
		}
	}
	*/
	if (!((type == MapFieldForest && Tileset->isAWoodTile(mf.playerInfo.SeenTile) && (mf.getFlag() & type))
		  || (type == MapFieldRocks && Tileset->isARockTile(mf.playerInfo.SeenTile) && (mf.getFlag() & type)))) {
		if (seen) {
			return;
		}
	}
	//Wyrmgus end
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
			//Wyrmgus start
			if (type == MapFieldForest) {
				mf.Flags |= MapFieldStumps;
			} else if (type == MapFieldRocks) {
				mf.Flags |= MapFieldGravel;
				mf.Flags |= MapFieldNoBuilding;
			}
			//Wyrmgus end
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

/**
**  Correct the surrounding fields.
**
**  @param type  Tiletype of tile to adjust
**  @param seen  1 if updating seen value, 0 for real
**  @param pos   Map tile-position.
*/
void CMap::FixNeighbors(unsigned short type, int seen, const Vec2i &pos)
{
	const Vec2i offset[] = {Vec2i(1, 0), Vec2i(-1, 0), Vec2i(0, 1), Vec2i(0, -1),
							Vec2i(-1, -1), Vec2i(-1, 1), Vec2i(1, -1), Vec2i(1, 1)
						   };

	for (unsigned int i = 0; i < sizeof(offset) / sizeof(*offset); ++i) {
		FixTile(type, seen, pos + offset[i]);
	}
}

/// Remove wood from the map.
void CMap::ClearWoodTile(const Vec2i &pos)
{
	CMapField &mf = *this->Field(pos);

	mf.setGraphicTile(this->Tileset->getRemovedTreeTile());
	mf.Flags &= ~(MapFieldForest | MapFieldUnpassable);
	//Wyrmgus start
	mf.Flags |= MapFieldStumps;
	//Wyrmgus end
	mf.Value = 0;

	UI.Minimap.UpdateXY(pos);
	FixNeighbors(MapFieldForest, 0, pos);

	//maybe isExplored
	if (mf.playerInfo.IsExplored(*ThisPlayer)) {
		UI.Minimap.UpdateSeenXY(pos);
		MarkSeenTile(mf);
	}
	
	//Wyrmgus start
	//remove decorations if a wall, tree or rock was removed from the tile
	std::vector<CUnit *> table;
	Select(pos, pos, table);
	for (size_t i = 0; i != table.size(); ++i) {
		if (table[i]->Type->UnitType == UnitTypeLand && table[i]->Type->BoolFlag[DECORATION_INDEX].value) {
			LetUnitDie(*table[i]);			
		}
	}
	//Wyrmgus end
}
/// Remove rock from the map.
void CMap::ClearRockTile(const Vec2i &pos)
{
	CMapField &mf = *this->Field(pos);

	mf.setGraphicTile(this->Tileset->getRemovedRockTile());
	mf.Flags &= ~(MapFieldRocks | MapFieldUnpassable);
	//Wyrmgus start
	mf.Flags |= MapFieldGravel;
	mf.Flags |= MapFieldNoBuilding;
	//Wyrmgus end
	mf.Value = 0;

	UI.Minimap.UpdateXY(pos);
	FixNeighbors(MapFieldRocks, 0, pos);

	//maybe isExplored
	if (mf.playerInfo.IsExplored(*ThisPlayer)) {
		UI.Minimap.UpdateSeenXY(pos);
		MarkSeenTile(mf);
	}
	
	//Wyrmgus start
	//remove decorations if a wall, tree or rock was removed from the tile
	std::vector<CUnit *> table;
	Select(pos, pos, table);
	for (size_t i = 0; i != table.size(); ++i) {
		if (table[i]->Type->UnitType == UnitTypeLand && table[i]->Type->BoolFlag[DECORATION_INDEX].value) {
			LetUnitDie(*table[i]);			
		}
	}
	//Wyrmgus end
}



/**
**  Regenerate forest.
**
**  @param pos  Map tile pos
*/
void CMap::RegenerateForestTile(const Vec2i &pos)
{
	Assert(Map.Info.IsPointOnMap(pos));
	CMapField &mf = *this->Field(pos);

	//Wyrmgus start
//	if (mf.getGraphicTile() != this->Tileset->getRemovedTreeTile()) {
	if (mf.getGraphicTile() != this->Tileset->getRemovedTreeTile() || !(mf.getFlag() & MapFieldStumps)) {
	//Wyrmgus end
		return;
	}

	//  Increment each value of no wood.
	//  If grown up, place new wood.
	//  FIXME: a better looking result would be fine
	//    Allow general updates to any tiletype that regrows

	//Wyrmgus start
//	const unsigned int occupedFlag = (MapFieldWall | MapFieldUnpassable | MapFieldLandUnit | MapFieldBuilding);
	const unsigned int occupedFlag = (MapFieldWall | MapFieldUnpassable | MapFieldLandUnit | MapFieldBuilding | MapFieldItem); // don't regrow forests under items
	//Wyrmgus end
	++mf.Value;
	if (mf.Value < ForestRegeneration) {
		return;
	}
	mf.Value = ForestRegeneration;
	if ((mf.Flags & occupedFlag) || pos.y == 0) {
		return;
	}
	const Vec2i offset(0, -1);
	CMapField &topMf = *(&mf - this->Info.MapWidth);
	//Wyrmgus start
	const Vec2i topleftOffset(-1, -1);
	CMapField &topleftMf = *this->Field(pos + topleftOffset);
	const Vec2i leftOffset(-1, 0);
	CMapField &leftMf = *this->Field(pos + leftOffset);
	const Vec2i bottomleftOffset(-1, 1);
	CMapField &bottomleftMf = *this->Field(pos + bottomleftOffset);
	const Vec2i toprightOffset(1, -1);
	CMapField &toprightMf = *this->Field(pos + toprightOffset);
	const Vec2i rightOffset(1, 0);
	CMapField &rightMf = *this->Field(pos + rightOffset);
	const Vec2i bottomrightOffset(1, 1);
	CMapField &bottomrightMf = *this->Field(pos + bottomrightOffset);
	const Vec2i bottomOffset(0, 1);
	CMapField &bottomMf = *this->Field(pos + bottomOffset);
//	if (topMf.getGraphicTile() == this->Tileset->getRemovedTreeTile()
//		&& topMf.Value >= ForestRegeneration
//		&& !(topMf.Flags & occupedFlag)) {
	if (((topMf.getGraphicTile() == this->Tileset->getRemovedTreeTile() && (topMf.getFlag() & MapFieldStumps) && topMf.Value >= ForestRegeneration && !(topMf.Flags & occupedFlag)) || (topMf.getFlag() & MapFieldForest))
		&& this->Info.IsPointOnMap(pos + topleftOffset) && ((topleftMf.getGraphicTile() == this->Tileset->getRemovedTreeTile() && (topleftMf.getFlag() & MapFieldStumps) && topleftMf.Value >= ForestRegeneration && !(topleftMf.Flags & occupedFlag)) || (topleftMf.getFlag() & MapFieldForest))
		&& this->Info.IsPointOnMap(pos + leftOffset) && ((leftMf.getGraphicTile() == this->Tileset->getRemovedTreeTile() && (leftMf.getFlag() & MapFieldStumps) && leftMf.Value >= ForestRegeneration && !(leftMf.Flags & occupedFlag)) || (leftMf.getFlag() & MapFieldForest))) {
		//Wyrmgus end
		DebugPrint("Real place wood\n");
		//Wyrmgus start
//		topMf.setTileIndex(*Map.Tileset, Map.Tileset->getTopOneTreeTile(), 0);
//		topMf.setGraphicTile(Map.Tileset->getTopOneTreeTile());
		topMf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), 100);
		topMf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
		//Wyrmgus end
		topMf.playerInfo.SeenTile = topMf.getGraphicTile();
		//Wyrmgus start
//		topMf.Value = 0;
		topMf.Value = 100;
//		topMf.Flags |= MapFieldForest | MapFieldUnpassable;
		//Wyrmgus end
		UI.Minimap.UpdateSeenXY(pos + offset);
		UI.Minimap.UpdateXY(pos + offset);
		
		//Wyrmgus start
		topleftMf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), 100);
		topleftMf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
		topleftMf.playerInfo.SeenTile = topleftMf.getGraphicTile();
		topleftMf.Value = 100;
		UI.Minimap.UpdateSeenXY(pos + topleftOffset);
		UI.Minimap.UpdateXY(pos + topleftOffset);
		
		leftMf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), 100);
		leftMf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
		leftMf.playerInfo.SeenTile = leftMf.getGraphicTile();
		leftMf.Value = 100;
		UI.Minimap.UpdateSeenXY(pos + leftOffset);
		UI.Minimap.UpdateXY(pos + leftOffset);
		
//		mf.setTileIndex(*Map.Tileset, Map.Tileset->getBottomOneTreeTile(), 0);
//		mf.setGraphicTile(Map.Tileset->getBottomOneTreeTile());
		mf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), 100);
		mf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
		//Wyrmgus end
		mf.playerInfo.SeenTile = mf.getGraphicTile();
		//Wyrmgus start
//		mf.Value = 0;
		mf.Value = 100;
//		mf.Flags |= MapFieldForest | MapFieldUnpassable;
		//Wyrmgus end
		UI.Minimap.UpdateSeenXY(pos);
		UI.Minimap.UpdateXY(pos);
		
		if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(mf);
		}
		if (Map.Field(pos + offset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(topMf);
		}
		//Wyrmgus start
		if (Map.Field(pos + topleftOffset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(topleftMf);
		}
		if (Map.Field(pos + leftOffset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(leftMf);
		}

		FixNeighbors(MapFieldForest, 0, pos + topleftOffset);
		FixNeighbors(MapFieldForest, 0, pos + leftOffset);
		//Wyrmgus end
		FixNeighbors(MapFieldForest, 0, pos + offset);
		FixNeighbors(MapFieldForest, 0, pos);
	//Wyrmgus start
	} else if (((topMf.getGraphicTile() == this->Tileset->getRemovedTreeTile() && (topMf.getFlag() & MapFieldStumps) && topMf.Value >= ForestRegeneration && !(topMf.Flags & occupedFlag)) || (topMf.getFlag() & MapFieldForest))
		&& this->Info.IsPointOnMap(pos + toprightOffset) && ((toprightMf.getGraphicTile() == this->Tileset->getRemovedTreeTile() && (toprightMf.getFlag() & MapFieldStumps) && toprightMf.Value >= ForestRegeneration && !(toprightMf.Flags & occupedFlag)) || (toprightMf.getFlag() & MapFieldForest))
		&& this->Info.IsPointOnMap(pos + rightOffset) && ((rightMf.getGraphicTile() == this->Tileset->getRemovedTreeTile() && (rightMf.getFlag() & MapFieldStumps) && rightMf.Value >= ForestRegeneration && !(rightMf.Flags & occupedFlag)) || (rightMf.getFlag() & MapFieldForest))) {
		DebugPrint("Real place wood\n");
		topMf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), 100);
		topMf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
		topMf.playerInfo.SeenTile = topMf.getGraphicTile();
		topMf.Value = 100;
		UI.Minimap.UpdateSeenXY(pos + offset);
		UI.Minimap.UpdateXY(pos + offset);
		
		toprightMf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), 100);
		toprightMf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
		toprightMf.playerInfo.SeenTile = toprightMf.getGraphicTile();
		toprightMf.Value = 100;
		UI.Minimap.UpdateSeenXY(pos + toprightOffset);
		UI.Minimap.UpdateXY(pos + toprightOffset);
		
		rightMf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), 100);
		rightMf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
		rightMf.playerInfo.SeenTile = rightMf.getGraphicTile();
		rightMf.Value = 100;
		UI.Minimap.UpdateSeenXY(pos + rightOffset);
		UI.Minimap.UpdateXY(pos + rightOffset);
		
		mf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), 100);
		mf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
		mf.playerInfo.SeenTile = mf.getGraphicTile();
		mf.Value = 100;
		UI.Minimap.UpdateSeenXY(pos);
		UI.Minimap.UpdateXY(pos);
		
		if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(mf);
		}
		if (Map.Field(pos + offset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(topMf);
		}
		if (Map.Field(pos + toprightOffset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(toprightMf);
		}
		if (Map.Field(pos + rightOffset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(rightMf);
		}

		FixNeighbors(MapFieldForest, 0, pos + toprightOffset);
		FixNeighbors(MapFieldForest, 0, pos + rightOffset);
		FixNeighbors(MapFieldForest, 0, pos + offset);
		FixNeighbors(MapFieldForest, 0, pos);
	} else if (this->Info.IsPointOnMap(pos + bottomOffset) && ((bottomMf.getGraphicTile() == this->Tileset->getRemovedTreeTile() && (bottomMf.getFlag() & MapFieldStumps) && bottomMf.Value >= ForestRegeneration && !(bottomMf.Flags & occupedFlag)) || (bottomMf.getFlag() & MapFieldForest))
		&& this->Info.IsPointOnMap(pos + bottomleftOffset) && ((bottomleftMf.getGraphicTile() == this->Tileset->getRemovedTreeTile() && (bottomleftMf.getFlag() & MapFieldStumps) && bottomleftMf.Value >= ForestRegeneration && !(bottomleftMf.Flags & occupedFlag)) || (bottomleftMf.getFlag() & MapFieldForest))
		&& this->Info.IsPointOnMap(pos + leftOffset) && ((leftMf.getGraphicTile() == this->Tileset->getRemovedTreeTile() && (leftMf.getFlag() & MapFieldStumps) && leftMf.Value >= ForestRegeneration && !(leftMf.Flags & occupedFlag)) || (leftMf.getFlag() & MapFieldForest))) {
		DebugPrint("Real place wood\n");
		bottomMf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), 100);
		bottomMf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
		bottomMf.playerInfo.SeenTile = bottomMf.getGraphicTile();
		bottomMf.Value = 100;
		UI.Minimap.UpdateSeenXY(pos + offset);
		UI.Minimap.UpdateXY(pos + offset);
		
		bottomleftMf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), 100);
		bottomleftMf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
		bottomleftMf.playerInfo.SeenTile = bottomleftMf.getGraphicTile();
		bottomleftMf.Value = 100;
		UI.Minimap.UpdateSeenXY(pos + bottomleftOffset);
		UI.Minimap.UpdateXY(pos + bottomleftOffset);
		
		leftMf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), 100);
		leftMf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
		leftMf.playerInfo.SeenTile = leftMf.getGraphicTile();
		leftMf.Value = 100;
		UI.Minimap.UpdateSeenXY(pos + leftOffset);
		UI.Minimap.UpdateXY(pos + leftOffset);
		
		mf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), 100);
		mf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
		mf.playerInfo.SeenTile = mf.getGraphicTile();
		mf.Value = 100;
		UI.Minimap.UpdateSeenXY(pos);
		UI.Minimap.UpdateXY(pos);
		
		if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(mf);
		}
		if (Map.Field(pos + offset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(bottomMf);
		}
		if (Map.Field(pos + bottomleftOffset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(bottomleftMf);
		}
		if (Map.Field(pos + leftOffset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(leftMf);
		}

		FixNeighbors(MapFieldForest, 0, pos + bottomleftOffset);
		FixNeighbors(MapFieldForest, 0, pos + leftOffset);
		FixNeighbors(MapFieldForest, 0, pos + offset);
		FixNeighbors(MapFieldForest, 0, pos);
	} else if (this->Info.IsPointOnMap(pos + bottomOffset) && ((bottomMf.getGraphicTile() == this->Tileset->getRemovedTreeTile() && (bottomMf.getFlag() & MapFieldStumps) && bottomMf.Value >= ForestRegeneration && !(bottomMf.Flags & occupedFlag)) || (bottomMf.getFlag() & MapFieldForest))
		&& this->Info.IsPointOnMap(pos + bottomrightOffset) && ((bottomrightMf.getGraphicTile() == this->Tileset->getRemovedTreeTile() && (bottomrightMf.getFlag() & MapFieldStumps) && bottomrightMf.Value >= ForestRegeneration && !(bottomrightMf.Flags & occupedFlag)) || (bottomrightMf.getFlag() & MapFieldForest))
		&& this->Info.IsPointOnMap(pos + rightOffset) && ((rightMf.getGraphicTile() == this->Tileset->getRemovedTreeTile() && (rightMf.getFlag() & MapFieldStumps) && rightMf.Value >= ForestRegeneration && !(rightMf.Flags & occupedFlag)) || (rightMf.getFlag() & MapFieldForest))) {
		DebugPrint("Real place wood\n");
		bottomMf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), 100);
		bottomMf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
		bottomMf.playerInfo.SeenTile = bottomMf.getGraphicTile();
		bottomMf.Value = 100;
		UI.Minimap.UpdateSeenXY(pos + offset);
		UI.Minimap.UpdateXY(pos + offset);
		
		bottomrightMf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), 100);
		bottomrightMf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
		bottomrightMf.playerInfo.SeenTile = bottomrightMf.getGraphicTile();
		bottomrightMf.Value = 100;
		UI.Minimap.UpdateSeenXY(pos + bottomrightOffset);
		UI.Minimap.UpdateXY(pos + bottomrightOffset);
		
		rightMf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), 100);
		rightMf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
		rightMf.playerInfo.SeenTile = rightMf.getGraphicTile();
		rightMf.Value = 100;
		UI.Minimap.UpdateSeenXY(pos + rightOffset);
		UI.Minimap.UpdateXY(pos + rightOffset);
		
		mf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), 100);
		mf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
		mf.playerInfo.SeenTile = mf.getGraphicTile();
		mf.Value = 100;
		UI.Minimap.UpdateSeenXY(pos);
		UI.Minimap.UpdateXY(pos);
		
		if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(mf);
		}
		if (Map.Field(pos + offset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(bottomMf);
		}
		if (Map.Field(pos + bottomrightOffset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(bottomrightMf);
		}
		if (Map.Field(pos + rightOffset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(rightMf);
		}

		FixNeighbors(MapFieldForest, 0, pos + bottomrightOffset);
		FixNeighbors(MapFieldForest, 0, pos + rightOffset);
		FixNeighbors(MapFieldForest, 0, pos + offset);
		FixNeighbors(MapFieldForest, 0, pos);
	//Wyrmgus end
	}
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
	for (pos.y = 0; pos.y < Info.MapHeight; ++pos.y) {
		for (pos.x = 0; pos.x < Info.MapWidth; ++pos.x) {
			RegenerateForestTile(pos);
		}
	}
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

//Wyrmgus start
std::string RawTiles[MaxMapWidth][MaxMapHeight];
/**
**  Convert 0 AD map
**
**  @param mapname  map filename
*/
void Convert0ADMap(const std::string &mapname)
{
	const std::string filename = LibraryFileName(mapname.c_str());
	
	for (int x = 0; x < MaxMapWidth; ++x) {
		for (int y = 0; y < MaxMapHeight; ++y) {
			RawTiles[x][y].clear();
		}
	}
	int map_size = 0;
	int tile_heights_xy[MaxMapWidth][MaxMapHeight];
		
	std::ifstream is_pmp(filename, std::ifstream::binary);
	if (is_pmp) {
		is_pmp.seekg (0, is_pmp.end);
		int length = is_pmp.tellg();
		is_pmp.seekg (0, is_pmp.beg);

		char * buffer = new char [length];

		is_pmp.read(buffer,length);

		if (!is_pmp) {
			fprintf(stderr, ("File " + filename + " loading error.").c_str());
		}
		
		int current_byte = 0;
		current_byte += 4; //char magic[4]; // == "PSMP"
		current_byte += 4; //u32 version; // == 6
		current_byte += 4; //u32 data_size; // == filesize-12
		
		map_size = (map_size << 8) + buffer[current_byte + 3];
		map_size = (map_size << 8) + buffer[current_byte + 2];
		map_size = (map_size << 8) + buffer[current_byte + 1];
		map_size = (map_size << 8) + buffer[current_byte + 0];
		current_byte += 4; //u32 map_size; // number of patches (16x16 tiles) per side
		
		int tile_heights[MaxMapWidth * MaxMapHeight];
		for (int i = 0; i < ((map_size * 16 + 1) * (map_size * 16 + 1)); ++i) { //u16 heightmap[(mapsize*16 + 1)*(mapsize*16 + 1)]; // vertex heights
			int height = 0;
			height = (height << 8) + buffer[current_byte + 1];
			height = (height << 8) + buffer[current_byte + 0];
			current_byte += 2;
			
			tile_heights[i] = height;
		}
		
		int num_terrain_textures = 0;
		num_terrain_textures = (num_terrain_textures << 8) + buffer[current_byte + 3];
		num_terrain_textures = (num_terrain_textures << 8) + buffer[current_byte + 2];
		num_terrain_textures = (num_terrain_textures << 8) + buffer[current_byte + 1];
		num_terrain_textures = (num_terrain_textures << 8) + buffer[current_byte + 0];
		current_byte += 4; //u32 num_terrain_textures;
		
		std::string terrain_texture_names[1024];
		for (int i = 0; i < num_terrain_textures; ++i) { //String terrain_textures[num_terrain_textures]; // filenames (no path), e.g. "cliff1.dds"
			int string_length = 0;
			string_length = (string_length << 8) + buffer[current_byte + 3];
			string_length = (string_length << 8) + buffer[current_byte + 2];
			string_length = (string_length << 8) + buffer[current_byte + 1];
			string_length = (string_length << 8) + buffer[current_byte + 0];
			current_byte += 4; //u32 length;
			
			char string_data[256];
			for (int j = 0; j < string_length; ++j) {
				string_data[j] = buffer[current_byte + j];
			}
			string_data[string_length] = '\0';
			current_byte += string_length; //char data[length]; // not NUL-terminated
			
			terrain_texture_names[i] = std::string(string_data);
		}
		
		int patch_quantity = map_size * map_size;
		for (int i = 0; i < patch_quantity; ++i) { //Patch patches[mapsize^2];
			int tile_quantity = 16 * 16;
			for (int j = 0; j < tile_quantity; ++j) { //Tile tiles[16*16];
				int texture1 = 0;
				texture1 = (texture1 << 8) + buffer[current_byte + 1];
				texture1 = (texture1 << 8) + buffer[current_byte + 0];
				current_byte += 2; //u16 texture1; // index into terrain_textures[]
					
				int texture2 = 0;
				texture2 = (texture2 << 8) + buffer[current_byte + 1];
				texture2 = (texture2 << 8) + buffer[current_byte + 0];
				current_byte += 2; //u16 texture2; // index, or 0xFFFF for 'none'
					
				current_byte += 4; //u32 priority; // Used for blending between edges of tiles with different textures. A higher priority is blended on top of a lower priority.
					
				int patch_x = i % map_size;
				int x = (patch_x * 16) + (j % 16);
					
				int patch_y = i / map_size;
				int y = (map_size * 16) - 1 - ((patch_y * 16) + (j / 16));
				
				int height = abs(tile_heights[(((map_size * 16) - 1 - y) * (map_size * 16 + 1)) + x + 1]);
				tile_heights_xy[x][y] = height;
				
				std::string tile_type;
					
				if (texture1 >= 0) {
					std::string texture1_string = terrain_texture_names[texture1];
					tile_type = Convert0ADTextureToTileType(texture1_string);
				}
						
				if (texture2 >= 0) {
					std::string texture2_string = terrain_texture_names[texture2];
				}

				RawTiles[x][y] = tile_type;
			}
		}
		
		is_pmp.close();

		delete[] buffer;
	}
	
	float water_height;
	std::string map_title;
	std::vector<std::string> units_unit_types;
	std::vector<int> units_players;
	std::vector<Vec2i> units_positions;

	int player_quantity = 0;
	Vec2i player_start_points[PlayerMax];
	std::string player_civilizations[PlayerMax];
	
	std::string xml_filename = LibraryFileName(FindAndReplaceStringEnding(filename, ".pmp", ".xml").c_str());
	std::ifstream is_xml(xml_filename);
	
	std::string processing_state;
	std::string line_str;
	while (std::getline(is_xml, line_str))
	{
		std::string line_contents = FindAndReplaceString(line_str, "	", "");
		
		if (processing_state == "<WaterBody>") {
			if (line_contents.find("<Height>", 0) != std::string::npos) {
				line_contents = FindAndReplaceString(line_contents, "<Height>", "");
				line_contents = FindAndReplaceString(line_contents, "</Height>", "");
				water_height = std::stof(line_contents);
			} else if (line_contents == "</WaterBody>") {
				processing_state = "";
			}
		} else if (processing_state == "<ScriptSettings>") {
			if (line_contents.find("\"Name\": ", 0) != std::string::npos) {
				line_contents = FindAndReplaceStringBeginning(line_contents, "  \"Name\": \"", "");
				line_contents = FindAndReplaceStringEnding(line_contents, "\",", "");
				map_title = line_contents;
			} else if (line_contents.find("\"PlayerData\": [", 0) != std::string::npos) {
				processing_state = "PlayerData";
			} else if (line_contents.find("</ScriptSettings>", 0) != std::string::npos) {
				processing_state = "";
			}
		} else if (processing_state == "PlayerData") {
			if (line_contents.find("\"Civ\": ", 0) != std::string::npos) {
				line_contents = FindAndReplaceString(line_contents, " ", "");
				line_contents = FindAndReplaceString(line_contents, "\"Civ\":", "");
				line_contents = FindAndReplaceString(line_contents, "\"", "");
				line_contents = FindAndReplaceString(line_contents, ",", "");
				player_civilizations[player_quantity] = Convert0ADCivilizationToCivilization(line_contents);
				player_quantity += 1;
			} else if (line_contents.find("],", 0) != std::string::npos) {
				processing_state = "<ScriptSettings>";
			}
		} else if (processing_state == "<Entity>") {
			if (line_contents.find("<Template>", 0) != std::string::npos) {
				line_contents = FindAndReplaceString(line_contents, "<Template>", "");
				line_contents = FindAndReplaceString(line_contents, "</Template>", "");
				units_unit_types.push_back(Convert0ADTemplateToUnitTypeIdent(line_contents));
				if (line_contents.find("actor|", 0) != std::string::npos) { //these don't have a player defined, so we need to push one for them
					units_players.push_back(15);
				}
			} else if (line_contents.find("<Player>", 0) != std::string::npos) {
				line_contents = FindAndReplaceString(line_contents, "<Player>", "");
				line_contents = FindAndReplaceString(line_contents, "</Player>", "");
				int unit_player = std::stoi(line_contents) - 1;
				if (unit_player == -1) { //gaia units use player 0 in 0 AD
					unit_player = 15;
				}
				units_players.push_back(unit_player);
			} else if (line_contents.find("<Position", 0) != std::string::npos) {
				std::string x_position_string = FindAndReplaceString(line_contents, "<Position x=\"", "");
				int x_position_string_end = x_position_string.find("\"", 0);
				x_position_string.replace(x_position_string_end, x_position_string.length() - x_position_string_end, "");
				float x_position_float = std::stof(x_position_string) / 4;
				int x_position = x_position_float;
				
				std::string y_position_string = FindAndReplaceStringEnding(line_contents, "\"/>", "");
				int y_position_string_beginning = y_position_string.find("z=\"", 0);
				y_position_string.replace(0, y_position_string_beginning, "");
				y_position_string = FindAndReplaceString(y_position_string, "z=\"", "");
				float y_position_float = ((map_size * 16 * 4) - std::stof(y_position_string)) / 4;
				int y_position = y_position_float;
				
				units_positions.push_back(Vec2i(x_position, y_position));
				
				//if we've reached the position and the unit still has no player defined, define the neutral player for it
				if (units_players.size() < units_unit_types.size()) {
					units_players.push_back(15);
				}
				
				if (!units_unit_types[units_unit_types.size() - 1].empty() && units_unit_types[units_unit_types.size() - 1] != "Tree" && units_unit_types[units_unit_types.size() - 1] != "Rock" && UnitTypes[UnitTypeIdByIdent(units_unit_types[units_unit_types.size() - 1])]->Class == "town-hall" && units_players[units_players.size() - 1] != 15) { //if this is a town hall building not belonging to the neutral player, set its owner's starting position to it
					int start_point_player = units_players[units_players.size() - 1];
					player_start_points[start_point_player].x = x_position;
					player_start_points[start_point_player].y = y_position;
				}
			} else if (line_contents == "</Entity>") {
				processing_state = "";
			}
		} else {
			if (line_contents == "<WaterBody>") {
				processing_state = "<WaterBody>";
			} else if (line_contents.find("<ScriptSettings>", 0) != std::string::npos) {
				processing_state = "<ScriptSettings>";
			} else if (line_contents.find("<Entity uid", 0) != std::string::npos) {
				processing_state = "<Entity>";
			}
		}
	}  
	
	water_height *= 65536.f;
	float height_scale = 1.f / 92;
	water_height *= height_scale;
		
	for (int x = 0; x < map_size * 16; ++x) {
		for (int y = 0; y < map_size * 16; ++y) {
			if (tile_heights_xy[x][y] < water_height) {
				RawTiles[x][y] = "Water";
			}
		}
	}
	
	for (size_t i = 0; i < units_unit_types.size(); ++i) {
		if (units_unit_types[i] == "Tree") {
			RawTiles[units_positions[i].x][units_positions[i].y] = "Tree";
			units_unit_types.erase(units_unit_types.begin() + i);
			units_players.erase(units_players.begin() + i);
			units_positions.erase(units_positions.begin() + i);
		}
	}
	
	AdjustRawTileMapIrregularities(map_size * 16, map_size * 16);
	AdjustRawTileMapTransitions(map_size * 16, map_size * 16);
	AdjustRawTileMapIrregularities(map_size * 16, map_size * 16);
		
	//write the map presentation
	std::string smp_filename = FindAndReplaceStringEnding(filename, ".pmp", ".smp");
	FileWriter *f_smp = NULL;

	try {
		f_smp = CreateFileWriter(smp_filename);
		f_smp->printf("-- Stratagus Map Presentation\n");
		f_smp->printf("-- Map converted from 0 AD's map format by the Stratagus built-in map converter.\n");
		f_smp->printf("-- File licensed under the GNU GPL version 2.\n\n");

		f_smp->printf("DefinePlayerTypes(");
		for (int i = 0; i < player_quantity; ++i) {
			f_smp->printf("%s\"%s\"", (i ? ", " : ""), "person");
		}
		f_smp->printf(")\n");

		f_smp->printf("PresentMap(\"%s\", %d, %d, %d, %d)\n",
				  map_title.c_str(), player_quantity, map_size * 16, map_size * 16, 1);
	} catch (const FileException &) {
		fprintf(stderr, "ERROR: cannot write the map presentation\n");
		delete f_smp;
		return;
	}

	delete f_smp;
	//end of map presentation writing

	//begin writing the map setup
	FileWriter *f = NULL;
	std::string sms_filename = FindAndReplaceStringEnding(filename, ".pmp", ".sms");

	try {
		f = CreateFileWriter(sms_filename);

		f->printf("-- Stratagus Map Setup\n");
		f->printf("-- Map converted from 0 AD's map format by the Stratagus built-in map converter.\n");
		f->printf("-- File licensed under the GNU GPL version 2.\n\n");

		f->printf("-- load tilesets\n");
		f->printf("LoadTileModels(\"%s\")\n\n", "scripts/tilesets/conifer_forest_summer.lua");
			
		for (int x = 0; x < map_size * 16; ++x) {
			for (int y = 0; y < map_size * 16; ++y) {
				f->printf("SetRawTile(%d, %d, \"%s\")\n", x, y, RawTiles[x][y].c_str());
			}
		}
			
		f->printf("ApplyRawTiles()\n");

		f->printf("-- player configuration\n");
		for (int i = 0; i < player_quantity; ++i) {
			f->printf("SetStartView(%d, %d, %d)\n", i, Players[i].StartPos.x, Players[i].StartPos.y);
			f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
					  i, DefaultResourceNames[GoldCost].c_str(),
					  5000);
			f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
					  i, DefaultResourceNames[WoodCost].c_str(),
					  2000);
			f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
					  i, DefaultResourceNames[StoneCost].c_str(),
					  2000);
			f->printf("SetStartView(%d, %d, %d)\n", i, player_start_points[i].x, player_start_points[i].y);
			f->printf("SetPlayerData(%d, \"RaceName\", \"%s\")\n",
					  i, player_civilizations[i].c_str());
			f->printf("SetAiType(%d, \"%s\")\n",
					  i, "land-attack");
		}
					  
		for (size_t i = 0; i < units_unit_types.size(); ++i) {
			if (!units_unit_types[i].empty() && units_unit_types[i] != "Tree" && units_unit_types[i] != "Rock") {
				int unit_x_offset = - ((UnitTypes[UnitTypeIdByIdent(units_unit_types[i])]->TileWidth - 1) / 2); //0 AD units' position is mapped to their center
				int unit_y_offset = - ((UnitTypes[UnitTypeIdByIdent(units_unit_types[i])]->TileHeight - 1) / 2);
				f->printf("unit = CreateUnit(\"%s\", %d, {%d, %d})\n",
						  units_unit_types[i].c_str(),
						  units_players[i],
						  units_positions[i].x + unit_x_offset, units_positions[i].y + unit_y_offset);
			}
		}

		f->printf("\n");
	} catch (const FileException &) {
		fprintf(stderr, "Can't save map setup : '%s' \n", sms_filename);
		delete f;
		return;
	}
		
	delete f; // end of the writing of the map setup
}

std::string Convert0ADTextureToTileType(const std::string zero_ad_texture)
{
	//Mediterranean biome
	if (zero_ad_texture == "medit_city_pavement") {
		return "Land";
	} else if (zero_ad_texture == "medit_cliff_aegean") {
		return "Rock";
	} else if (zero_ad_texture == "medit_cliff_aegean_shrubs") {
		return "Rock";
	} else if (zero_ad_texture == "medit_cliff_grass") {
		return "Rock";
	} else if (zero_ad_texture == "medit_cliff_greek") {
		return "Rock";
	} else if (zero_ad_texture == "medit_cliff_greek_2") {
		return "Rock";
	} else if (zero_ad_texture == "medit_cliff_italia_grass") {
		return "Rock";
	} else if (zero_ad_texture == "medit_dirt") {
		return "Rough";
	} else if (zero_ad_texture == "medit_dirt_b") {
		return "Rough";
	} else if (zero_ad_texture == "medit_dirt_c") {
		return "Rough";
	} else if (zero_ad_texture == "medit_dirt_d") {
		return "Rough";
	} else if (zero_ad_texture == "medit_dirt_e") {
		return "Rough";
	} else if (zero_ad_texture == "medit_forestfloor_a") {
		return "Tree";
	} else if (zero_ad_texture == "medit_grass_field") {
		return "Land";
	} else if (zero_ad_texture == "medit_grass_field_a") {
		return "Land";
	} else if (zero_ad_texture == "medit_grass_field_b") {
		return "Land";
	} else if (zero_ad_texture == "medit_grass_field_brown") {
		return "Land";
	} else if (zero_ad_texture == "medit_grass_field_dry") {
		return "Land";
	} else if (zero_ad_texture == "medit_grass_flowers") {
		return "Land";
	} else if (zero_ad_texture == "medit_grass_shrubs") {
		return "Land";
	} else if (zero_ad_texture == "medit_grass_wild") {
		return "Land";
	} else if (zero_ad_texture == "medit_riparian_mud") {
		return "Rough";
	} else if (zero_ad_texture == "medit_rocks") {
		return "Rough";
	} else if (zero_ad_texture == "medit_rocks_grass") {
		return "Rough";
	} else if (zero_ad_texture == "medit_rocks_grass_shrubs") {
		return "Rough";
	} else if (zero_ad_texture == "medit_rocks_shrubs") {
		return "Rough";
	} else if (zero_ad_texture == "medit_rocks_wet") {
		return "Rough";
	} else if (zero_ad_texture == "medit_sand_messy") {
		return "Rough";
	} else if (zero_ad_texture == "medit_sea_coral_deep") {
		return "Water";
	} else if (zero_ad_texture == "medit_sea_coral_plants") {
		return "Water";
	} else if (zero_ad_texture == "medit_sea_depths") {
		return "Water";
	} else if (zero_ad_texture == "medit_shrubs") {
		return "Land";
	} else if (zero_ad_texture == "medit_shrubs_dry") {
		return "Land";
	} else if (zero_ad_texture == "medit_shrubs_golden") {
		return "Land";
	//City
	} else if (zero_ad_texture == "medit_city_tile") {
		return "Land";
	} else if (zero_ad_texture == "medit_road_broken") {
		return "Land";
	} else if (zero_ad_texture == "savanna_tile_a") {
		return "Land";
	} else if (zero_ad_texture == "savanna_tile_a_red") {
		return "Land";
	} else if (zero_ad_texture == "temp_road_muddy") {
		return "Land";
	} else if (zero_ad_texture == "tropic_citytile_a") {
		return "Land";
	//Grass
	} else if (zero_ad_texture == "grass1_spring") {
		return "Land";
	//Special
	} else if (zero_ad_texture == "farmland_a") {
		return "Land";
	} else {
		return "";
	}
}

std::string Convert0ADTemplateToUnitTypeIdent(const std::string zero_ad_template)
{
	//Fauna
	if (zero_ad_template == "gaia/fauna_deer") {
		return "unit-rat";
	} else if (zero_ad_template == "gaia/fauna_fish") {
		return "";
	} else if (zero_ad_template == "gaia/fauna_goat") {
		return "unit-rat";
	//Flora
	} else if (zero_ad_template == "gaia/flora_tree_apple") {
		return "";
	} else if (zero_ad_template == "gaia/flora_bush_grapes") {
		return "";
	} else if (zero_ad_template == "gaia/flora_tree_carob") {
		return "Tree";
	} else if (zero_ad_template == "gaia/flora_tree_cypress") {
		return "Tree";
	} else if (zero_ad_template == "gaia/flora_tree_euro_beech") {
		return "Tree";
	} else if (zero_ad_template == "gaia/flora_tree_medit_fan_palm") {
		return "Tree";
	} else if (zero_ad_template == "gaia/flora_tree_oak") {
		return "Tree";
	} else if (zero_ad_template == "gaia/flora_tree_pine") {
		return "Tree";
	} else if (zero_ad_template == "gaia/flora_tree_poplar_lombardy") {
		return "Tree";
	//Geology
	} else if (zero_ad_template == "gaia/geology_metal_mediterranean_slabs") {
		return "unit-gold-deposit";
	} else if (zero_ad_template == "gaia/geology_stone_mediterranean") {
		return "Rock";
	//Special neutral units
	} else if (zero_ad_template == "gaia/special_treasure_pegasus") {
		return "unit-gold-chest";
	} else if (zero_ad_template == "gaia/special_treasure_golden_fleece") {
		return "unit-gold-chest";
	//Hellene units
	} else if (zero_ad_template == "units/hele_support_female_citizen") {
		return "unit-teuton-worker";
	} else if (zero_ad_template == "units/hele_infantry_spearman_b") {
		return "unit-teuton-swordsman";
	} else if (zero_ad_template == "units/hele_infantry_spearman_a") { //advanced greek spearman
		return "unit-teuton-swordsman";
	} else if (zero_ad_template == "units/hele_infantry_spearman_e") { //elite greek spearman
		return "unit-teuton-swordsman";
	} else if (zero_ad_template == "units/hele_infantry_archer_b") {
		return "unit-teuton-archer";
	} else if (zero_ad_template == "units/hele_infantry_javelinist_b") {
		return "unit-teuton-archer";
	} else if (zero_ad_template == "units/hele_cavalry_swordsman_e") { //cavalry swordsman
		return "unit-teuton-swordsman";
	} else if (zero_ad_template == "units/hele_cavalry_swordsman_e") { //advanced cavalry swordsman
		return "unit-teuton-swordsman";
	} else if (zero_ad_template == "units/hele_cavalry_swordsman_e") { //elite cavalry swordsman
		return "unit-teuton-swordsman";
	} else if (zero_ad_template == "units/hele_cavalry_javelinist_b") { //javelinist cavalry
		return "unit-teuton-archer";
	} else if (zero_ad_template == "units/hele_cavalry_javelinist_a") { //advanced javelinist cavalry
		return "unit-teuton-archer";
	} else if (zero_ad_template == "units/hele_cavalry_javelinist_e") { //elite javelinist cavalry
		return "unit-teuton-archer";
	//Hellene structures
	} else if (zero_ad_template == "structures/hele_civil_centre") {
		return "unit-teuton-town-hall";
	} else if (zero_ad_template == "structures/hele_field") {
		return "";
	} else if (zero_ad_template == "structures/hele_defense_tower") {
		return "unit-teuton-guard-tower";
	} else if (zero_ad_template == "other/hellenic_stoa") {
		return "unit-teuton-farm"; //maybe a market would be a better fitting building, but its effect is similar to that of a farm
	} else if (zero_ad_template == "other/hellenic_royal_stoa") {
		return "unit-teuton-barracks"; //maybe a market would be a better fitting building for its description, but it allows recruiting military units; otherwise its effect is similar to that of a farm
	} else if (zero_ad_template == "other/unfinished_greek_temple") {
		return ""; //should be a temple
	//Hellene heroes
	} else if (zero_ad_template == "units/hele_hero_themistocles") {
		return "unit-teuton-swordsman";
	//Theban units
	} else if (zero_ad_template == "units/thebes_sacred_band_hoplitai") { //champion spearman
		return "unit-teuton-swordsman";
	//Celt units
	} else if (zero_ad_template == "units/celt_fanatic") {
		return "unit-germanic-warrior";
	//Celt structures
	} else if (zero_ad_template == "structures/celt_field") {
		return "";
	} else if (zero_ad_template == "other/celt_hut") {
		return "unit-celt-farm";
	} else if (zero_ad_template == "other/celt_longhouse") {
		return "unit-celt-farm";
	//Gaul units
	} else if (zero_ad_template == "units/gaul_support_female_citizen") {
		return "unit-germanic-worker";
	} else if (zero_ad_template == "units/gaul_infantry_spearman_b") {
		return "unit-germanic-warrior";
	} else if (zero_ad_template == "units/gaul_infantry_spearman_a") {
		return "unit-germanic-warrior";
	} else if (zero_ad_template == "units/gaul_infantry_spearman_e") {
		return "unit-germanic-warrior";
	} else if (zero_ad_template == "units/gaul_infantry_javelinist_b") {
		return "unit-germanic-archer";
	} else if (zero_ad_template == "units/gaul_infantry_javelinist_a") {
		return "unit-germanic-archer";
	} else if (zero_ad_template == "units/gaul_infantry_javelinist_e") {
		return "unit-germanic-archer";
	} else if (zero_ad_template == "units/gaul_cavalry_swordsman_b") {
		return "unit-germanic-warrior";
	} else if (zero_ad_template == "units/gaul_cavalry_swordsman_a") {
		return "unit-germanic-warrior";
	} else if (zero_ad_template == "units/gaul_cavalry_swordsman_e") {
		return "unit-germanic-warrior";
	} else if (zero_ad_template == "units/gaul_cavalry_javelinist_b") {
		return "unit-germanic-archer";
	} else if (zero_ad_template == "units/gaul_cavalry_javelinist_a") {
		return "unit-germanic-archer";
	} else if (zero_ad_template == "units/gaul_cavalry_javelinist_e") {
		return "unit-germanic-archer";
	//Gaul structures
	} else if (zero_ad_template == "structures/gaul_civil_centre") {
		return "unit-germanic-town-hall";
	} else if (zero_ad_template == "structures/gaul_defense_tower") {
		return "unit-teuton-guard-tower";
	} else if (zero_ad_template == "structures/gaul_tavern") {
		return "unit-germanic-farm";
	//Gaul heroes
	} else if (zero_ad_template == "units/gaul_hero_brennus") {
		return "unit-germanic-warrior";
	//Miscellaneous structures
	} else if (zero_ad_template == "other/column_doric") {
		return "unit-small-rocks";
	} else if (zero_ad_template == "other/column_doric_fallen") {
		return "unit-small-rocks";
	} else if (zero_ad_template == "other/column_doric_fallen_b") {
		return "unit-small-rocks";
	} else if (zero_ad_template == "other/fence_long") {
		return ""; //should be a palisade?
	} else if (zero_ad_template == "other/fence_short") {
		return ""; //should be a palisade?
	//Props
	} else if (zero_ad_template == "actor|geology/stone_medit_med.xml") {
		return "unit-small-rocks";
	} else if (zero_ad_template == "actor|props/flora/bush_medit_sm.xml") {
		return "";
	} else if (zero_ad_template == "actor|props/flora/bush_medit_sm_dry.xml") {
		return "";
	} else if (zero_ad_template == "actor|props/flora/bush_medit_sm_lush.xml") {
		return "";
	} else if (zero_ad_template == "actor|props/flora/bush_medit_underbrush.xml") {
		return "";
	} else if (zero_ad_template == "actor|props/flora/grass_medit_field.xml") {
		return "";
	} else if (zero_ad_template == "actor|props/flora/grass_soft_dry_small.xml") {
		return "";
	} else if (zero_ad_template == "actor|props/flora/grass_soft_dry_small_tall.xml") {
		return "";
	} else if (zero_ad_template == "actor|props/flora/grass_soft_large.xml") {
		return "";
	} else if (zero_ad_template == "actor|props/flora/grass_soft_large_tall.xml") {
		return "";
	} else if (zero_ad_template == "actor|props/flora/grass_soft_small.xml") {
		return "";
	} else if (zero_ad_template == "actor|props/flora/grass_soft_small_tall.xml") {
		return "";
	} else if (zero_ad_template == "actor|props/flora/grass_soft_tuft_a.xml") {
		return "";
	} else if (zero_ad_template == "actor|props/flora/pond_lillies_large.xml") {
		return "";
	} else if (zero_ad_template == "actor|props/flora/water_lillies.xml") {
		return "";
	} else {
		return "";
	}
}

std::string Convert0ADCivilizationToCivilization(const std::string zero_ad_civilization)
{
	if (zero_ad_civilization == "gaul") {
		return "celt";
	} else if (zero_ad_civilization == "hele") {
		return "greek";
	} else {
		return "";
	}
}

void AdjustRawTileMapIrregularities(int map_width, int map_height)
{
	bool no_irregularities_found = false;
	while (!no_irregularities_found) {
		no_irregularities_found = true;
		for (int x = 0; x < map_width; ++x) {
			for (int y = 0; y < map_height; ++y) {
				if (RawTiles[x][y] == "Rock") {
					int horizontal_adjacent_tiles = 0;
					if ((x - 1) >= 0 && RawTiles[x - 1][y] != "Rock") {
						horizontal_adjacent_tiles += 1;
					}
					if ((x + 1) < map_width && RawTiles[x + 1][y] != "Rock") {
						horizontal_adjacent_tiles += 1;
					}
					int vertical_adjacent_tiles = 0;
					if ((y - 1) >= 0 && RawTiles[x][y - 1] != "Rock") {
						vertical_adjacent_tiles += 1;
					}
					if ((y + 1) < map_height && RawTiles[x][y + 1] != "Rock") {
						vertical_adjacent_tiles += 1;
					}
					if (horizontal_adjacent_tiles >= 2 || vertical_adjacent_tiles >= 2) {
						RawTiles[x][y] = "Rough";
						no_irregularities_found = false;
					}
				} else if (RawTiles[x][y] == "Water") {
					int horizontal_adjacent_tiles = 0;
					if ((x - 1) >= 0 && RawTiles[x - 1][y] != "Water") {
						horizontal_adjacent_tiles += 1;
					}
					if ((x + 1) < map_width && RawTiles[x + 1][y] != "Water") {
						horizontal_adjacent_tiles += 1;
					}
					int vertical_adjacent_tiles = 0;
					if ((y - 1) >= 0 && RawTiles[x][y - 1] != "Water") {
						vertical_adjacent_tiles += 1;
					}
					if ((y + 1) < map_height && RawTiles[x][y + 1] != "Water") {
						vertical_adjacent_tiles += 1;
					}
					if (horizontal_adjacent_tiles >= 2 || vertical_adjacent_tiles >= 2) {
						RawTiles[x][y] = "Rough";
						no_irregularities_found = false;
					}
				} else if (RawTiles[x][y] == "Rough") {
					int horizontal_adjacent_tiles = 0;
					if ((x - 1) >= 0 && RawTiles[x - 1][y] != "Rough" && RawTiles[x - 1][y] != "Rock" && RawTiles[x - 1][y] != "Water") {
						horizontal_adjacent_tiles += 1;
					}
					if ((x + 1) < map_width && RawTiles[x + 1][y] != "Rough" && RawTiles[x + 1][y] != "Rock" && RawTiles[x + 1][y] != "Water") {
						horizontal_adjacent_tiles += 1;
					}
					int vertical_adjacent_tiles = 0;
					if ((y - 1) >= 0 && RawTiles[x][y - 1] != "Rough" && RawTiles[x][y - 1] != "Rock" && RawTiles[x][y - 1] != "Water") {
						vertical_adjacent_tiles += 1;
					}
					if ((y + 1) < map_height && RawTiles[x][y + 1] != "Rough" && RawTiles[x][y + 1] != "Rock" && RawTiles[x][y + 1] != "Water") {
						vertical_adjacent_tiles += 1;
					}
					if (horizontal_adjacent_tiles >= 2 || vertical_adjacent_tiles >= 2) {
						RawTiles[x][y] = "Land";
						no_irregularities_found = false;
					}
				} else if (RawTiles[x][y] == "Tree") {
					int horizontal_adjacent_tiles = 0;
					if ((x - 1) >= 0 && RawTiles[x - 1][y] != "Tree") {
						horizontal_adjacent_tiles += 1;
					}
					if ((x + 1) < map_width && RawTiles[x + 1][y] != "Tree") {
						horizontal_adjacent_tiles += 1;
					}
					int vertical_adjacent_tiles = 0;
					if ((y - 1) >= 0 && RawTiles[x][y - 1] != "Tree") {
						vertical_adjacent_tiles += 1;
					}
					if ((y + 1) < map_height && RawTiles[x][y + 1] != "Tree") {
						vertical_adjacent_tiles += 1;
					}
					if (horizontal_adjacent_tiles >= 2 || vertical_adjacent_tiles >= 2) {
						RawTiles[x][y] = "Land";
						no_irregularities_found = false;
					}
				}
			}
		}
	}
}

void AdjustRawTileMapTransitions(int map_width, int map_height)
{
	for (int x = 0; x < map_width; ++x) {
		for (int y = 0; y < map_height; ++y) {
			if (RawTiles[x][y] != "Rock" && RawTiles[x][y] != "Water" && RawTiles[x][y] != "Rough" && RawTiles[x][y] != "Dark-Rough") {
				for (int sub_x = -1; sub_x <= 1; ++sub_x) {
					for (int sub_y = -1; sub_y <= 1; ++sub_y) {
						if ((x + sub_x) < 0 || (x + sub_x) >= map_width || (y + sub_y) < 0 || (y + sub_y) >= map_height) {
							continue;
						}
						if (RawTiles[x + sub_x][y + sub_y] == "Rock" || RawTiles[x + sub_x][y + sub_y] == "Water" || RawTiles[x + sub_x][y + sub_y] == "Dark-Rough") {
							RawTiles[x][y] = "Rough";
						}
					}
				}
			}
		}
	}
}
//Wyrmgus end

//@}
