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

#pragma once

class CUnit;

namespace wyrmgus {

/**
**  Class to ease the ref counting of each CUnit instance.
*/
class unit_ref final
{
public:
	unit_ref()
	{
	}

	unit_ref(CUnit *u)
	{
		this->set_unit(u);
	}

	unit_ref(const unit_ref &u)
	{
		this->set_unit(u.unit);
	}

	~unit_ref()
	{
		this->reset();
	}

	void reset()
	{
		this->set_unit(nullptr);
	}

	operator CUnit *() { return unit; }
	operator CUnit *() const { return unit; }

	CUnit &operator*() { return *unit; }
	CUnit *operator->() const { return unit; }

	unit_ref &operator= (CUnit *u)
	{
		this->set_unit(u);
		return *this;
	}

	bool operator== (CUnit *u) const { return this->unit == u; }
	bool operator!= (CUnit *u) const { return this->unit != u; }

private:
	void set_unit(CUnit *unit);

	CUnit *unit = nullptr;
};

}