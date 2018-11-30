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
/**@name time_of_day.cpp - The time of day source file. */
//
//      (c) Copyright 2018 by Andrettin
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

#include "time_of_day.h"

#include "config.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CTimeOfDay *> CTimeOfDay::TimesOfDay;
std::map<std::string, CTimeOfDay *> CTimeOfDay::TimesOfDayByIdent;
	
/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a time of day
**
**	@param	ident		The time of day's string identifier
**	@param	should_find	Whether it is an error if the time of day could not be found; this is true by default
**
**	@return	The time of day if found, or null otherwise
*/
CTimeOfDay *CTimeOfDay::GetTimeOfDay(const std::string &ident, const bool should_find)
{
	std::map<std::string, CTimeOfDay *>::const_iterator find_iterator = TimesOfDayByIdent.find(ident);
	
	if (find_iterator != TimesOfDayByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid time of day: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a time of day
**
**	@param	ident	The time of day's string identifier
**
**	@return	The time of day if found, or a newly-created one otherwise
*/
CTimeOfDay *CTimeOfDay::GetOrAddTimeOfDay(const std::string &ident)
{
	CTimeOfDay *time_of_day = GetTimeOfDay(ident, false);
	
	if (!time_of_day) {
		time_of_day = new CTimeOfDay;
		time_of_day->Ident = ident;
		TimesOfDay.push_back(time_of_day);
		TimesOfDayByIdent[ident] = time_of_day;
	}
	
	return time_of_day;
}

/**
**	@brief	Remove the existing times of day
*/
void CTimeOfDay::ClearTimesOfDay()
{
	for (size_t i = 0; i < TimesOfDay.size(); ++i) {
		delete TimesOfDay[i];
	}
	TimesOfDay.clear();
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CTimeOfDay::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else {
			fprintf(stderr, "Invalid time of day property: \"%s\".\n", key.c_str());
		}
	}
}

//@}
