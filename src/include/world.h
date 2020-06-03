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
//      (c) Copyright 2016-2020 by Andrettin
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

#pragma once

#include "database/data_type.h"
#include "database/detailed_data_entry.h"
#include "data_type.h"
#include "map/terrain_geodata_map.h"

class CProvince;
class CSeasonSchedule;
class CTerrainFeature;
class CTimeOfDaySchedule;

namespace stratagus {

class plane;
class species;
class terrain_type;

class world final : public detailed_data_entry, public data_type<world>, public CDataType
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "world";
	static constexpr const char *database_folder = "worlds";
	static constexpr const char *terrain_map_folder = "terrain";

	static world *add(const std::string &identifier, const stratagus::module *module);

	explicit world(const std::string &identifier) : detailed_data_entry(identifier), CDataType(identifier)
	{
	}

	~world();

	virtual void ProcessConfigData(const CConfigData *config_data) override;

	plane *get_plane() const
	{
		return this->plane;
	}

	std::vector<QVariantList> parse_geojson_folder(const std::string_view &folder) const;
	terrain_geodata_map parse_terrain_geojson_folder() const;

	int ID = -1;
private:
	plane *plane = nullptr;
public:
	CTimeOfDaySchedule *TimeOfDaySchedule = nullptr;					/// this world's time of day schedule
	CSeasonSchedule *SeasonSchedule = nullptr;							/// this world's season schedule
	std::vector<CProvince *> Provinces;									/// Provinces in this world
	std::vector<CTerrainFeature *> TerrainFeatures;						/// Terrain features in this world
	std::vector<species *> Species;									/// Species in this world
};

}
