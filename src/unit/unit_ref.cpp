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
//      (c) Copyright 2012-2020 by Joris Dauphin and Andrettin
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

#include "unit/unit_ref.h"

#include "unit/unit.h"

namespace wyrmgus {

unit_ref::unit_ref(CUnit *u) : unit(u)
{
	if (this->unit != nullptr) {
		this->unit->RefsIncrease();
	}
}

unit_ref::unit_ref(const unit_ref &u) : unit(u.unit)
{
	if (this->unit != nullptr) {
		unit->RefsIncrease();
	}
}

void unit_ref::Reset()
{
	if (this->unit != nullptr) {
		unit->RefsDecrease();
	}

	this->unit = nullptr;
}

unit_ref &unit_ref::operator= (CUnit *u)
{
	if (this->unit != u) {
		if (u) {
			u->RefsIncrease();
		}
		if (this->unit != nullptr) {
			this->unit->RefsDecrease();
		}
		unit = u;
	}
	return *this;
}

}
