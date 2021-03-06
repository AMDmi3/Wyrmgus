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
//      Permission is hereby granted, free of charge, to any person obtaining a
//      copy of this software and associated documentation files (the
//      "Software"), to deal in the Software without restriction, including
//      without limitation the rights to use, copy, modify, merge, publish,
//      distribute, sublicense, and/or sell copies of the Software, and to
//      permit persons to whom the Software is furnished to do so, subject to
//      the following conditions:
//
//      The above copyright notice and this permission notice shall be included
//      in all copies or substantial portions of the Software.
//
//      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

class CMapLayer;
class CPlayer;
class CUnit;

namespace wyrmgus {

class resource;
class site;
class tile;

class site_game_data final
{
public:
	explicit site_game_data(const site *site) : site(site)
	{
	}

	const std::string &get_current_cultural_name() const;

	CUnit *get_site_unit() const
	{
		return this->site_unit;
	}

	void set_site_unit(CUnit *unit);

	CPlayer *get_owner() const
	{
		return this->owner;
	}

	void set_owner(CPlayer *player);

	CPlayer *get_realm_owner() const;

	const QPoint &get_map_pos() const
	{
		return this->map_pos;
	}

	void set_map_pos(const QPoint &pos)
	{
		this->map_pos = pos;
	}

	const CMapLayer *get_map_layer() const
	{
		return this->map_layer;
	}

	void set_map_layer(const CMapLayer *map_layer)
	{
		this->map_layer = map_layer;
	}

	void process_territory_tile(const tile *tile, const QPoint &tile_pos, const int z);

	void add_border_tile(const QPoint &tile_pos)
	{
		this->border_tiles.push_back(tile_pos);

		if (this->territory_rect.isNull()) {
			this->territory_rect = QRect(tile_pos, QSize(1, 1));
		} else {
			if (tile_pos.x() < this->territory_rect.x()) {
				this->territory_rect.setX(tile_pos.x());
			} else if (tile_pos.x() > this->territory_rect.right()) {
				this->territory_rect.setRight(tile_pos.x());
			}
			if (tile_pos.y() < this->territory_rect.y()) {
				this->territory_rect.setY(tile_pos.y());
			} else if (tile_pos.y() > this->territory_rect.bottom()) {
				this->territory_rect.setBottom(tile_pos.y());
			}
		}
	}

	void update_border_tiles();
	void update_minimap_territory();

	const std::vector<QPoint> &get_trade_route_tiles() const
	{
		return this->trade_route_tiles;
	}

	bool is_coastal() const
	{
		return this->coastal;
	}

	bool has_resource_source(const resource *resource) const;

	int get_resource_tile_count(const resource *resource) const
	{
		const auto find_iterator = this->resource_tile_counts.find(resource);
		if (find_iterator != this->resource_tile_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void increment_resource_tile_count(const resource *resource)
	{
		++this->resource_tile_counts[resource];
	}

	void decrement_resource_tile_count(const resource *resource)
	{
		const int quantity = --this->resource_tile_counts[resource];

		if (quantity <= 0) {
			this->resource_tile_counts.erase(resource);
		}
	}

	const std::vector<CUnit *> &get_resource_units(const resource *resource) const
	{
		static std::vector<CUnit *> empty_vector;

		const auto find_iterator = this->resource_units.find(resource);
		if (find_iterator != this->resource_units.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	void add_resource_unit(CUnit *unit, const resource *resource)
	{
		this->resource_units[resource].push_back(unit);
	}

	void add_resource_unit(CUnit *unit);
	void remove_resource_unit(CUnit *unit, const resource *resource);
	void remove_resource_unit(CUnit *unit);

	void clear_resource_units()
	{
		this->resource_units.clear();
	}

private:
	const wyrmgus::site *site = nullptr;
	CUnit *site_unit = nullptr; //unit which represents the site
	CPlayer *owner = nullptr;
	QPoint map_pos = QPoint(-1, -1);
	const CMapLayer *map_layer = nullptr;
	std::vector<QPoint> border_tiles; //the tiles for the settlement which border the territory of another settlement
	QRect territory_rect; //the territory rectangle of the site
	std::vector<QPoint> trade_route_tiles; //the tiles containing a trade route in the settlement's territory
	bool coastal = false;
	std::map<const resource *, int> resource_tile_counts; //resource tile counts in the settlement's territory
	std::map<const resource *, std::vector<CUnit *>> resource_units; //resource units in the settlement's territory
};

}
