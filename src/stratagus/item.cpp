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
/**@name item.cpp - The items. */
//
//      (c) Copyright 2015 by Andrettin
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "item.h"

#include <ctype.h>

#include <string>
#include <map>

#include "character.h"
#include "game.h"
#include "parameters.h"
#include "player.h"
#include "unit.h"
#include "unit_manager.h"
#include "unittype.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CUniqueItem *> UniqueItems;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

int GetItemClassIdByName(std::string item_class)
{
	if (item_class == "dagger") {
		return DaggerItemClass;
	} else if (item_class == "sword") {
		return SwordItemClass;
	} else if (item_class == "thrusting-sword") {
		return ThrustingSwordItemClass;
	} else if (item_class == "axe") {
		return AxeItemClass;
	} else if (item_class == "mace") {
		return MaceItemClass;
	} else if (item_class == "spear") {
		return SpearItemClass;
	} else if (item_class == "bow") {
		return BowItemClass;
	} else if (item_class == "throwing-axe") {
		return ThrowingAxeItemClass;
	} else if (item_class == "javelin") {
		return JavelinItemClass;
	} else if (item_class == "shield") {
		return ShieldItemClass;
	} else if (item_class == "helmet") {
		return HelmetItemClass;
	} else if (item_class == "armor") {
		return ArmorItemClass;
	} else if (item_class == "boots") {
		return BootsItemClass;
	} else if (item_class == "amulet") {
		return AmuletItemClass;
	} else if (item_class == "ring") {
		return RingItemClass;
	} else if (item_class == "arrows") {
		return ArrowsItemClass;
	} else if (item_class == "potion") {
		return PotionItemClass;
	} else if (item_class == "scroll") {
		return ScrollItemClass;
	}

	return -1;
}

std::string GetItemClassNameById(int item_class)
{
	if (item_class == DaggerItemClass) {
		return "dagger";
	} else if (item_class == SwordItemClass) {
		return "sword";
	} else if (item_class == ThrustingSwordItemClass) {
		return "thrusting-sword";
	} else if (item_class == AxeItemClass) {
		return "axe";
	} else if (item_class == MaceItemClass) {
		return "mace";
	} else if (item_class == SpearItemClass) {
		return "spear";
	} else if (item_class == BowItemClass) {
		return "bow";
	} else if (item_class == ThrowingAxeItemClass) {
		return "throwing-axe";
	} else if (item_class == JavelinItemClass) {
		return "javelin";
	} else if (item_class == ShieldItemClass) {
		return "shield";
	} else if (item_class == HelmetItemClass) {
		return "helmet";
	} else if (item_class == ArmorItemClass) {
		return "armor";
	} else if (item_class == BootsItemClass) {
		return "boots";
	} else if (item_class == AmuletItemClass) {
		return "amulet";
	} else if (item_class == RingItemClass) {
		return "ring";
	} else if (item_class == ArrowsItemClass) {
		return "arrows";
	} else if (item_class == PotionItemClass) {
		return "potion";
	} else if (item_class == ScrollItemClass) {
		return "scroll";
	}

	return "";
}

int GetItemClassSlot(int item_class)
{
	if (
		item_class == DaggerItemClass
		|| item_class == SwordItemClass
		|| item_class == ThrustingSwordItemClass
		|| item_class == AxeItemClass
		|| item_class == MaceItemClass
		|| item_class == SpearItemClass
		|| item_class == BowItemClass
		|| item_class == ThrowingAxeItemClass
		|| item_class == JavelinItemClass
	) {
		return WeaponItemSlot;
	} else if (item_class == ShieldItemClass) {
		return ShieldItemSlot;
	} else if (item_class == HelmetItemClass) {
		return HelmetItemSlot;
	} else if (item_class == ArmorItemClass) {
		return ArmorItemSlot;
	} else if (item_class == BootsItemClass) {
		return BootsItemSlot;
	} else if (item_class == AmuletItemClass) {
		return AmuletItemSlot;
	} else if (item_class == RingItemClass) {
		return RingItemSlot;
	} else if (item_class == ArrowsItemClass) {
		return ArrowsItemSlot;
	}

	return -1;
}

bool CUniqueItem::CanDrop()
{
	// unique items cannot drop if a persistent hero owns them already, or if there's already one of them in the current scenario; unless it's a character-specific bound item, in which case it can still drop
	for (size_t i = 0; i < Characters.size(); ++i) {
		for (size_t j = 0; j < Characters[i]->Items.size(); ++j) {
			if (Characters[i]->Items[j]->Unique && Characters[i]->Items[j]->Name == this->Name && !Characters[i]->Items[j]->Bound) {
				return false;
			}
		}
	}
	
	for (size_t i = 0; i < CustomHeroes.size(); ++i) {
		for (size_t j = 0; j < CustomHeroes[i]->Items.size(); ++j) {
			if (CustomHeroes[i]->Items[j]->Unique && CustomHeroes[i]->Items[j]->Name == this->Name && !CustomHeroes[i]->Items[j]->Bound) {
				return false;
			}
		}
	}
	
	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
		CUnit &unit = **it;
		if (unit.Unique && unit.Name == this->Name && !unit.Bound) {
			return false;
		}
	}

	return true;
}

void CleanUniqueItems()
{
	for (size_t i = 0; i < UniqueItems.size(); ++i) {
		delete UniqueItems[i];
	}
	UniqueItems.clear();
}

CUniqueItem *GetUniqueItem(std::string item_name)
{
	for (size_t i = 0; i < UniqueItems.size(); ++i) {
		if (item_name == UniqueItems[i]->Name) {
			return UniqueItems[i];
		}
	}
	return NULL;
}

//@}
