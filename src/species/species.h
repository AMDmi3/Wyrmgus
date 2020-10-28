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
//      (c) Copyright 2020 by Andrettin
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

struct lua_State;

static int CclDefineSpecies(lua_State *l);

namespace wyrmgus {

class plane;
class taxon;
class terrain_type;
class unit_type;
class world;
enum class geological_era;
enum class taxonomic_rank;

class species final : public detailed_data_entry, public data_type<species>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::taxon* supertaxon MEMBER supertaxon READ get_supertaxon)
	Q_PROPERTY(QString specific_name READ get_specific_name_qstring)
	Q_PROPERTY(wyrmgus::geological_era era MEMBER era READ get_era)
	Q_PROPERTY(wyrmgus::plane* home_plane MEMBER home_plane READ get_home_plane)
	Q_PROPERTY(wyrmgus::world* homeworld MEMBER homeworld READ get_homeworld)
	Q_PROPERTY(bool sapient MEMBER sapient READ is_sapient)
	Q_PROPERTY(bool asexual MEMBER asexual READ is_asexual)

public:
	static constexpr const char *class_identifier = "species";
	static constexpr const char *database_folder = "species";

	static std::map<const taxon *, int> get_supertaxon_counts(const std::vector<const species *> &source_species_list, const std::vector<const taxon *> &taxons);
	static std::vector<std::string> get_name_list(const std::vector<const species *> &species_list);

	explicit species(const std::string &identifier);

	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize();
	virtual void check() const;

	taxon *get_supertaxon() const
	{
		return this->supertaxon;
	}

	const taxon *get_supertaxon_of_rank(const taxonomic_rank rank) const;
	bool is_subtaxon_of(const taxon *taxon) const;

	const std::string &get_specific_name() const
	{
		return this->specific_name;
	}

	QString get_specific_name_qstring() const
	{
		return QString::fromStdString(this->specific_name);
	}

	Q_INVOKABLE void set_specific_name(const std::string &name)
	{
		this->specific_name = name;
	}

	std::string get_scientific_name() const;

	geological_era get_era() const
	{
		return this->era;
	}

	plane *get_home_plane() const
	{
		return this->home_plane;
	}

	world *get_homeworld() const
	{
		return this->homeworld;
	}

	bool is_sapient() const
	{
		return this->sapient;
	}

	bool is_prehistoric() const;

	bool is_asexual() const
	{
		return this->asexual;
	}

	const std::vector<const terrain_type *> &get_native_terrain_types() const
	{
		return this->native_terrain_types;
	}

	const std::vector<const species *> &get_pre_evolutions() const
	{
		return this->pre_evolutions;
	}

	const std::vector<const species *> &get_evolutions() const
	{
		return this->evolutions;
	}

	bool has_evolution(const terrain_type *terrain = nullptr, const bool sapient_only = false) const;
	const species *get_random_evolution(const terrain_type *terrain) const;
	
private:
	taxon *supertaxon = nullptr;
	std::string specific_name;
	geological_era era;
	plane *home_plane = nullptr;
	world *homeworld = nullptr;
	bool sapient = false;
	bool asexual = false;
public:
	unit_type *Type = nullptr;
private:
	std::vector<const terrain_type *> native_terrain_types; //in which terrains does this species live
	std::vector<const species *> pre_evolutions; //species from which this one can evolve
	std::vector<const species *> evolutions; //species to which this one can evolve

	friend static int ::CclDefineSpecies(lua_State *l);
};

}