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

namespace stratagus {

class not_condition final : public condition
{
public:
	not_condition() {}

	explicit not_condition(std::vector<std::unique_ptr<const condition>> &&conditions)
		: conditions(std::move(conditions))
	{
	}

	explicit not_condition(std::unique_ptr<const condition> &&condition)
	{
		this->conditions.push_back(std::move(condition));
	}

	virtual void process_sml_property(const sml_property &property) override
	{
		this->conditions.push_back(condition::from_sml_property(property));
	}

	virtual void process_sml_scope(const sml_data &scope) override
	{
		this->conditions.push_back(condition::from_sml_scope(scope));
	}

	virtual bool check(const CPlayer *player, bool ignore_units = false) const override
	{
		for (const auto &condition : this->conditions) {
			if (condition->check(player, ignore_units)) {
				return false;
			}
		}

		return true;
	}

	virtual bool check(const CUnit *unit, bool ignore_units = false) const override
	{
		for (const auto &condition : this->conditions) {
			if (condition->check(unit, ignore_units)) {
				return false;
			}
		}

		return true;
	}

	virtual std::string get_string(const std::string &prefix = "") const override
	{
		int element_count = 0;

		for (const auto &condition : this->conditions) {
			if (!condition->get_string(prefix + '\t').empty()) {
				element_count++;
			}
		}

		if (element_count >= 1) {
			std::string str = prefix + "NOT:\n";

			for (const auto &condition : this->conditions) {
				str += condition->get_string(prefix + '\t');
			}

			return str;
		} else {
			return std::string();
		}
	}

private:
	std::vector<std::unique_ptr<const condition>> conditions; //the conditions of which none should be true
};

}
