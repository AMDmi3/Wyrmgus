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
/**@name province.h - The province headerfile. */
//
//      (c) Copyright 2016 by Andrettin
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

#ifndef __PROVINCE_H__
#define __PROVINCE_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <vector>
#include <map>

#include "vec2i.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CUnitType;
class CUpgrade;
class CFaction;
class CProvince;
class WorldMapTile;

class WorldMapTerrainType
{
public:
	WorldMapTerrainType() :
		HasTransitions(false), Water(false), ID(-1), BaseTile(-1), Variations(0)
	{
	}

	std::string Name;
	std::string Tag;				/// used to locate graphic files
	bool HasTransitions;
	bool Water;
	int ID;
	int BaseTile;
	int Variations;					/// quantity of variations
};

class CWorld
{
public:
	CWorld() :
		ID(-1)
	{
	}
	
	std::string Name;
	std::string Description;
	std::string Background;
	int ID;																/// ID of this world
	std::vector<CProvince *> Provinces;									/// Provinces in this world
	std::map<std::pair<int,int>, WorldMapTile *> Tiles;								/// Tiles in the world
};

class CProvince
{
public:
	CProvince() :
		ID(-1),
		Water(false), Coastal(false), SettlementLocation(-1, -1),
		World(NULL)
	{
	}
	
	std::string Name;
	CWorld *World;
	std::string Map;
	std::string SettlementTerrain;
	int ID;																/// ID of this province
	bool Water;															/// Whether the province is a water province or not
	bool Coastal;														/// Whether the province is a coastal province or not
	Vec2i SettlementLocation;											/// In which tile the province's settlement is located
	std::string CulturalNames[MAX_RACES];								/// Names for the province for each different culture/civilization
	std::map<CFaction *, std::string> FactionCulturalNames;				/// Names for the province for each different faction
	std::vector<CFaction *> FactionClaims;								/// Factions which have a claim to this province
	std::vector<Vec2i> Tiles;
};

class WorldMapTile
{
public:
	WorldMapTile() :
		Position(-1, -1),
		World(NULL)
	{
	}

	Vec2i Position;								/// Position of the tile
	CWorld *World;
	std::map<std::pair<int,int>, std::string> CulturalTerrainNames;			/// Names for the tile (if it has a certain terrain) for each culture/civilization
	std::map<std::pair<int,CFaction *>, std::string> FactionCulturalTerrainNames;	/// Names for the tile (if it has a certain terrain) for each faction
	std::map<std::pair<int,int>, std::string> CulturalResourceNames;		/// Names for the tile (if it has a certain resource) for each culture/civilization
	std::map<std::pair<int,CFaction *>, std::string> FactionCulturalResourceNames;	/// Names for the tile (if it has a certain resource) for each faction
	std::map<int, std::string> CulturalSettlementNames;	/// Names for the tile's settlement for each faction
	std::map<CFaction *, std::string> FactionCulturalSettlementNames;	/// Names for the tile's settlement for each faction
};

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

extern std::vector<CWorld *> Worlds;
extern std::vector<CProvince *> Provinces;
extern std::vector<WorldMapTerrainType *>  WorldMapTerrainTypes;
extern std::map<std::string, int> WorldMapTerrainTypeStringToIndex;

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern void CleanWorlds();
extern CWorld *GetWorld(std::string world_name);
extern CProvince *GetProvince(std::string province_name);
extern int GetWorldMapTerrainTypeId(std::string terrain_type_name);
extern void ProvinceCclRegister();

//@}

#endif // !__PROVINCE_H__
