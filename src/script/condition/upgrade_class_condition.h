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

#include "script/condition/condition.h"
#include "unit/unit.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_class.h"
#include "upgrade/upgrade_structs.h"

namespace wyrmgus {

class upgrade_class_condition final : public condition
{
public:
	explicit upgrade_class_condition(const std::string &value)
	{
		this->upgrade_class = upgrade_class::get(value);
	}

	virtual bool check(const CPlayer *player, bool ignore_units = false) const override
	{
		const CUpgrade *upgrade = player->get_class_upgrade(this->upgrade_class);

		if (upgrade == nullptr) {
			return false;
		}

		return UpgradeIdAllowed(*player, upgrade->ID) == 'R';
	}

	virtual bool check(const CUnit *unit, bool ignore_units = false) const override
	{
		const CUpgrade *upgrade = unit->Player->get_class_upgrade(this->upgrade_class);

		if (upgrade == nullptr) {
			return false;
		}

		return this->check(unit->Player, ignore_units) || unit->GetIndividualUpgrade(upgrade);
	}

	virtual std::string get_string(const std::string &prefix = "") const override
	{
		std::string str = prefix + this->upgrade_class->get_name() + '\n';
		return str;
	}

private:
	const upgrade_class *upgrade_class = nullptr;
};

}