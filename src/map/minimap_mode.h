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

namespace stratagus {

enum class minimap_mode
{
	terrain, //terrain and units
	units, //only units
	territories, //territories (no units)
	territories_with_non_land, //territories (with non-land tiles, no units)

	count
};

inline const char *get_minimap_mode_name(const minimap_mode mode)
{
	switch (mode) {
		case minimap_mode::terrain:
			return "Terrain Minimap Mode";
		case minimap_mode::units:
			return "Units Minimap Mode";
		case minimap_mode::territories:
			return "Territories Minimap Mode";
		case minimap_mode::territories_with_non_land:
			return "Territories (with Non-Land) Minimap Mode";
		default:
			break;
	}

	throw std::runtime_error("Invalid minimap mode: \"" + std::to_string(static_cast<int>(mode)) + "\".");
}

}
