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
#include <tuple>

#include "vec2i.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CUnitType;
class CUpgrade;
class CFaction;
class CProvince;
class CRiver;
class CCharacter;
class WorldMapTile;

/**
**  Indexes into pathway array.
*/
enum Pathways {
	PathwayTrail,
	PathwayRoad,
	PathwayRailroad,
	
	MaxPathways
};

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

class CPlane
{
public:
	CPlane() :
		ID(-1)
	{
	}
	
	int ID;																/// ID of this plane
	std::string Name;
	std::string Description;
	std::string Background;
	std::string Quote;
};

class CWorld
{
public:
	CWorld() :
		ID(-1), Plane(NULL)
	{
	}
	
	int ID;																/// ID of this world
	std::string Name;
	std::string Description;
	std::string Background;
	std::string Quote;
	CPlane *Plane;
	std::vector<CProvince *> Provinces;									/// Provinces in this world
	std::vector<CRiver *> Rivers;										/// Rivers in this world
	std::vector<WorldMapTile *> Tiles;									/// Tiles in the world
};

class CRegion
{
public:
	CRegion() :
		ID(-1)
	{
	}
	
	std::string Name;
	int ID;																/// ID of this province
	std::vector<CProvince *> Provinces;									/// Provinces which belong to this region
	std::map<int, int> HistoricalPopulation;							/// Historical population, mapped to the year
};

class CProvince
{
public:
	CProvince() :
		ID(-1), ReferenceProvince(-1),
		Water(false), Coastal(false), SettlementLocation(-1, -1),
		World(NULL)
	{
	}
	
	std::string Name;
	CWorld *World;
	std::string Map;
	std::string SettlementTerrain;
	int ID;																/// ID of this province
	int ReferenceProvince;										/// Used by water provinces to see based on which province should they use which cultural name
	bool Water;															/// Whether the province is a water province or not
	bool Coastal;														/// Whether the province is a coastal province or not
	Vec2i SettlementLocation;											/// In which tile the province's settlement is located
	std::map<int, std::string> CulturalNames;							/// Names for the province for each different culture/civilization
	std::map<CFaction *, std::string> FactionCulturalNames;				/// Names for the province for each different faction
	std::vector<CFaction *> FactionClaims;								/// Factions which have a claim to this province
	std::vector<Vec2i> Tiles;
	std::vector<CRegion *> Regions;										/// Regions to which this province belongs
	std::map<int, CFaction *> HistoricalOwners;							/// Historical owners of the province, mapped to the year
	std::map<int, CFaction *> HistoricalClaims;							/// Historical claims over the province, mapped to the year
	std::map<int, int> HistoricalCultures;								/// Historical cultures which were predominant in the province, mapped to the year
	std::map<int, int> HistoricalPopulation;							/// Historical population, mapped to the year
	std::map<int, std::map<int, bool>> HistoricalSettlementBuildings;	/// Historical settlement buildings, mapped to building unit type id and year
	std::map<CUpgrade *, std::map<int, bool>> HistoricalModifiers;		/// Historical province modifiers, mapped to the modifier's upgrade and year
	std::map<std::tuple<int, int>, CCharacter *> HistoricalGovernors;	/// Historical governors of the province, mapped to the beginning and end of the term
};

class WorldMapTile
{
public:
	WorldMapTile() :
		Terrain(-1), Resource(-1),
		Capital(false),
		Position(-1, -1),
		World(NULL), Province(NULL)
	{
	}

	int Terrain;								/// Tile terrain (i.e. plains)
	int Resource;								/// The tile's resource, if any
	bool Capital;								/// Whether the tile is its province's capital
	Vec2i Position;								/// Position of the tile
	CWorld *World;
	CProvince *Province;
	std::map<std::pair<int,int>, std::vector<std::string>> CulturalTerrainNames;			/// Names for the tile (if it has a certain terrain) for each culture/civilization
	std::map<std::pair<int,CFaction *>, std::vector<std::string>> FactionCulturalTerrainNames;	/// Names for the tile (if it has a certain terrain) for each faction
	std::map<std::pair<int,int>, std::vector<std::string>> CulturalResourceNames;		/// Names for the tile (if it has a certain resource) for each culture/civilization
	std::map<std::pair<int,CFaction *>, std::vector<std::string>> FactionCulturalResourceNames;	/// Names for the tile (if it has a certain resource) for each faction
	std::map<int, std::vector<std::string>> CulturalSettlementNames;	/// Names for the tile's settlement for each faction
	std::map<CFaction *, std::vector<std::string>> FactionCulturalSettlementNames;	/// Names for the tile's settlement for each faction
	std::vector<CFaction *> FactionClaims;								/// Factions which have a claim to this tile
	std::map<int, CFaction *> HistoricalOwners;							/// Historical owners of the tile, mapped to the year
	std::map<int, CFaction *> HistoricalClaims;							/// Historical claims over the tile, mapped to the year
};

class CRiver
{
public:
	CRiver() :
		ID(-1), World(NULL)
	{
	}
	
	int ID;
	std::string Name;
	CWorld *World;
	std::map<int, std::string> CulturalNames;							/// Names for the river for each different culture/civilization
	std::map<CFaction *, std::string> FactionCulturalNames;				/// Names for the river for each different faction
};

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

extern std::vector<CPlane *> Planes;
extern std::vector<CWorld *> Worlds;
extern std::vector<CRegion *> Regions;
extern std::vector<CProvince *> Provinces;
extern std::vector<CRiver *> Rivers;
extern std::vector<WorldMapTerrainType *>  WorldMapTerrainTypes;
extern std::map<std::string, int> WorldMapTerrainTypeStringToIndex;

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern void CleanWorlds();
extern CPlane *GetPlane(std::string plane_name);
extern CWorld *GetWorld(std::string world_name);
extern CRegion *GetRegion(std::string region_name);
extern CProvince *GetProvince(std::string province_name);
extern CRiver *GetRiver(std::string river_name);
extern int GetWorldMapTerrainTypeId(std::string terrain_type_name);
extern std::string GetPathwayNameById(int pathway);
extern int GetPathwayIdByName(std::string pathway);
extern int GetPathwayTransportLevel(int pathway);
extern int GetTransportLevelMaximumCapacity(int transport_level);
extern void ProvinceCclRegister();

//@}

#endif // !__PROVINCE_H__
