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

#pragma once

#include "color.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "data_type.h"
#include "util/color_container.h"

class CGraphic;
class CPlayerColorGraphic;
class CUnitType;
struct lua_State;

int CclDefineTerrainType(lua_State *l);

namespace stratagus {

class resource;
class season;

class terrain_type : public named_data_entry, public data_type<terrain_type>, public CDataType
{
	Q_OBJECT

	Q_PROPERTY(QColor color READ get_color WRITE set_color)
	Q_PROPERTY(QString image_file READ get_image_file_qstring)
	Q_PROPERTY(bool overlay MEMBER overlay READ is_overlay)
	Q_PROPERTY(stratagus::resource* resource MEMBER resource READ get_resource)
	Q_PROPERTY(QVariantList base_terrain_types READ get_base_terrain_types_qvariant_list)

public:
	static constexpr const char *class_identifier = "terrain_type";
	static constexpr const char *database_folder = "terrain_types";

	static terrain_type *get_by_character(const char character)
	{
		terrain_type *terrain_type = terrain_type::try_get_by_character(character);

		if (terrain_type == nullptr) {
			throw std::runtime_error("No terrain type found for character: " + std::string(character, 1) + ".");
		}

		return terrain_type;
	}

	static terrain_type *try_get_by_character(const char character)
	{
		auto find_iterator = terrain_type::terrain_types_by_character.find(character);
		if (find_iterator != terrain_type::terrain_types_by_character.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	static terrain_type *get_by_color(const QColor &color)
	{
		terrain_type *terrain_type = terrain_type::try_get_by_color(color);

		if (terrain_type == nullptr) {
			throw std::runtime_error("No terrain type found for color: (" + std::to_string(color.red()) + ", " + std::to_string(color.green()) + ", " + std::to_string(color.blue()) + ").");
		}

		return terrain_type;
	}

	static terrain_type *try_get_by_color(const QColor &color)
	{
		auto find_iterator = terrain_type::terrain_types_by_color.find(color);
		if (find_iterator != terrain_type::terrain_types_by_color.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	static terrain_type *add(const std::string &identifier, const stratagus::module *module)
	{
		terrain_type *terrain_type = data_type::add(identifier, module);
		terrain_type->ID = terrain_type::get_all().size() - 1;
		return terrain_type;
	}

	static void clear()
	{
		data_type::clear();

		terrain_type::terrain_types_by_character.clear();
		terrain_type::terrain_types_by_color.clear();
	}

	terrain_type(const std::string &identifier) : named_data_entry(identifier), CDataType(identifier)
	{
	}
	
	~terrain_type();
	
	static void LoadTerrainTypeGraphics();
	static unsigned long GetTerrainFlagByName(const std::string &flag_name);
	
private:
	static inline std::map<char, terrain_type *> terrain_types_by_character;
	static inline color_map<terrain_type *> terrain_types_by_color;

public:
	virtual void process_sml_property(const sml_property &property) override;
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void initialize() override;

	char get_character() const
	{
		return this->character;
	}

	void set_character(const char character);
	void map_to_character(const char character);

	const QColor &get_color() const
	{
		return this->color;
	}

	void set_color(const QColor &color);

	const std::filesystem::path &get_image_file() const
	{
		return this->image_file;
	}

	void set_image_file(const std::filesystem::path &filepath);

	QString get_image_file_qstring() const
	{
		return QString::fromStdString(this->get_image_file().string());
	}

	Q_INVOKABLE void set_image_file(const std::string &filepath)
	{
		this->set_image_file(std::filesystem::path(filepath));
	}

	CPlayerColorGraphic *GetGraphics(const season *season = nullptr) const;

	bool is_overlay() const
	{
		return this->overlay;
	}

	resource *get_resource() const
	{
		return this->resource;
	}

	const std::vector<terrain_type *> &get_base_terrain_types() const
	{
		return this->base_terrain_types;
	}

	QVariantList get_base_terrain_types_qvariant_list() const;

	Q_INVOKABLE void add_base_terrain_type(terrain_type *terrain_type)
	{
		this->base_terrain_types.push_back(terrain_type);
	}

	Q_INVOKABLE void remove_base_terrain_type(terrain_type *terrain_type);

	const std::vector<int> &get_solid_tiles() const
	{
		return this->solid_tiles;
	}

	const std::vector<int> &get_damaged_tiles() const
	{
		return this->damaged_tiles;
	}

	const std::vector<int> &get_destroyed_tiles() const
	{
		return this->destroyed_tiles;
	}

private:
	char character = 0;
	QColor color;
public:
	int ID = -1;
	int SolidAnimationFrames = 0;
private:
	resource *resource = nullptr;
public:
	unsigned long Flags = 0;
private:
	bool overlay = false;										/// Whether this terrain type belongs to the overlay layer
public:
	bool Buildable = false;										/// Whether structures can be built upon this terrain type
	bool AllowSingle = false;									/// Whether this terrain type has transitions for single tiles
	bool Hidden = false;
	CUnitType *UnitType = nullptr;
	std::filesystem::path image_file;
	CPlayerColorGraphic *Graphics = nullptr;
	std::map<const season *, std::filesystem::path> season_image_files;
	std::map<const season *, CPlayerColorGraphic *> SeasonGraphics;		/// Graphics to be displayed instead of the normal ones during particular seasons
public:
	CGraphic *ElevationGraphics = nullptr;						/// Semi-transparent elevation graphics, separated so that borders look better
private:
	std::vector<terrain_type *> base_terrain_types; //possible base terrain types for this terrain type (if it is an overlay terrain)
public:
	std::vector<terrain_type *> BorderTerrains;					/// Terrain types which this one can border
	std::vector<terrain_type *> InnerBorderTerrains;			/// Terrain types which this one can border, and which "enter" this tile type in transitions
	std::vector<terrain_type *> OuterBorderTerrains;			/// Terrain types which this one can border, and which are "entered" by this tile type in transitions
private:
	std::vector<int> solid_tiles;
	std::vector<int> damaged_tiles;
	std::vector<int> destroyed_tiles;
public:
	std::map<std::tuple<int, int>, std::vector<int>> TransitionTiles;	/// Transition graphics, mapped to the tile type (-1 means any tile) and the transition type (i.e. northeast outer)
	std::map<std::tuple<int, int>, std::vector<int>> AdjacentTransitionTiles;	/// Transition graphics for the tiles adjacent to this terrain type, mapped to the tile type (-1 means any tile) and the transition type (i.e. northeast outer)

	friend int ::CclDefineTerrainType(lua_State *l);
};

}
