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
//      (c) Copyright 2018-2020 by Andrettin
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

#include "database/defines.h"
#include "resource.h"
#include "video/video.h"

namespace wyrmgus {

resource::~resource()
{
	CGraphic::Free(this->icon_graphics);
}

void resource::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "icon_file") {
		this->icon_file = database::get_graphics_path(this->get_module()) / value;
	} else if (key == "action_name") {
		this->action_name = value;
	} else {
		data_entry::process_sml_property(property);
	}
}

void resource::initialize()
{
	if (this->final_resource != nullptr) {
		this->final_resource->ChildResources.push_back(this);
	}

	if (!this->icon_file.empty()) {
		this->icon_graphics = CGraphic::New(this->icon_file, defines::get()->get_resource_icon_size());
		this->icon_graphics->Load(false, wyrmgus::defines::get()->get_scale_factor());
	}

	data_entry::initialize();
}

bool resource::IsMineResource() const
{
	switch (this->get_index()) {
		case CopperCost:
		case SilverCost:
		case GoldCost:
		case IronCost:
		case MithrilCost:
		case CoalCost:
		case DiamondsCost:
		case EmeraldsCost:
			return true;
		default:
			return false;
	}
}

}