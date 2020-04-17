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
//      (c) Copyright 2016-2020 by Andrettin
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

#include "plane.h"

#include "config.h"
#include "religion/deity_domain.h"
#include "school_of_magic.h"
#include "time/season_schedule.h"
#include "time/time_of_day_schedule.h"
#include "ui/ui.h"

namespace stratagus {


plane *plane::add(const std::string &identifier, const stratagus::module *module)
{
	plane *plane = data_type::add(identifier, module);
	plane->ID = plane::get_all().size() - 1;
	UI.PlaneButtons.resize(plane::get_all().size());
	UI.PlaneButtons[plane->ID].X = -1;
	UI.PlaneButtons[plane->ID].Y = -1;
	return plane;
}

void plane::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->set_name(value);
		} else if (key == "description") {
			this->set_description(value);
		} else if (key == "background") {
			this->set_background(value);
		} else if (key == "quote") {
			this->set_quote(value);
		} else if (key == "time_of_day_schedule") {
			value = FindAndReplaceString(value, "_", "-");
			this->TimeOfDaySchedule = CTimeOfDaySchedule::GetTimeOfDaySchedule(value);
		} else if (key == "season_schedule") {
			value = FindAndReplaceString(value, "_", "-");
			this->SeasonSchedule = CSeasonSchedule::GetSeasonSchedule(value);
		} else if (key == "empowered_deity_domain") {
			value = FindAndReplaceString(value, "_", "-");
			CDeityDomain *deity_domain = CDeityDomain::GetDeityDomain(value);
			if (deity_domain) {
				this->EmpoweredDeityDomains.push_back(deity_domain);
			}
		} else if (key == "empowered_school_of_magic") {
			value = FindAndReplaceString(value, "_", "-");
			CSchoolOfMagic *school_of_magic = CSchoolOfMagic::GetSchoolOfMagic(value);
			if (school_of_magic) {
				this->EmpoweredSchoolsOfMagic.push_back(school_of_magic);
			}
		} else {
			fprintf(stderr, "Invalid plane property: \"%s\".\n", key.c_str());
		}
	}
}

}
