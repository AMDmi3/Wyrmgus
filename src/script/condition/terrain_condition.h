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

#include "map/terrain_type.h"
#include "map/tile.h"
#include "script/condition/condition.h"
#include "util/string_util.h"

namespace wyrmgus {

class terrain_condition final : public condition
{
public:
	explicit terrain_condition(const std::string &value)
	{
		this->terrain = terrain_type::get(value);
	}

	virtual bool check(const CPlayer *player, const bool ignore_units) const override
	{
		Q_UNUSED(player)
		Q_UNUSED(ignore_units)

		return true;
	}

	virtual bool check(const CUnit *unit, const bool ignore_units) const override
	{
		Q_UNUSED(ignore_units)

		if (unit->MapLayer == nullptr) {
			return false;
		}

		const tile *center_tile = unit->get_center_tile();
		if (center_tile == nullptr) {
			return false;
		}

		if (this->terrain->is_overlay()) {
			return this->terrain == center_tile->get_overlay_terrain();
		} else {
			return this->terrain == center_tile->get_terrain();
		}
	}

	virtual std::string get_string(const size_t indent) const override
	{
		Q_UNUSED(indent)

		return "Is on " + string::highlight(this->terrain->get_name()) + " terrain";
	}

private:
	const terrain_type *terrain = nullptr;
};

}
