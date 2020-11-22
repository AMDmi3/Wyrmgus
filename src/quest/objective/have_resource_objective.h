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

#include "quest/objective/quest_objective.h"
#include "quest/objective_type.h"

namespace wyrmgus {

class have_resource_objective final : public quest_objective
{
public:
	explicit have_resource_objective(const wyrmgus::quest *quest) : quest_objective(quest)
	{
	}

	virtual objective_type get_objective_type() const override
	{
		return objective_type::have_resource;
	}

	virtual void update_counter(player_quest_objective *player_quest_objective) const override
	{
		const int resource_quantity = player_quest_objective->get_player()->get_resource(this->get_resource(), STORE_BOTH);
		player_quest_objective->set_counter(resource_quantity);
	}
};

}
