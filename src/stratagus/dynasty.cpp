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

#include "stratagus.h"

#include "dynasty.h"

#include "faction.h"
#include "script/condition/and_condition.h"
#include "util/container_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

dynasty::dynasty(const std::string &identifier) : detailed_data_entry(identifier)
{
}

dynasty::~dynasty()
{
}

void dynasty::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "preconditions") {
		this->preconditions = std::make_unique<and_condition>();
		database::process_sml_data(this->preconditions, scope);
	} else if (tag == "conditions") {
		this->conditions = std::make_unique<and_condition>();
		database::process_sml_data(this->conditions, scope);
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void dynasty::check() const
{
	if (this->get_preconditions() != nullptr) {
		this->get_preconditions()->check_validity();
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}
}

void dynasty::set_upgrade(CUpgrade *upgrade)
{
	if (upgrade == this->get_upgrade()) {
		return;
	}

	this->upgrade = upgrade;
	upgrade->set_dynasty(this);
}

QVariantList dynasty::get_factions_qvariant_list() const
{
	return container::to_qvariant_list(this->get_factions());
}

void dynasty::add_faction(faction *faction)
{
	this->factions.push_back(faction);
	faction->add_dynasty(this);
}

void dynasty::remove_faction(faction *faction)
{
	vector::remove(this->factions, faction);
}

}
