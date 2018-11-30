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
/**@name season_schedule.cpp - The season schedule source file. */
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

#include "season_schedule.h"

#include "config.h"
#include "season.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CSeasonSchedule *> CSeasonSchedule::SeasonSchedules;
std::map<std::string, CSeasonSchedule *> CSeasonSchedule::SeasonSchedulesByIdent;
	
/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a season schedule
**
**	@param	ident		The season schedule's string identifier
**	@param	should_find	Whether it is an error if the season schedule could not be found; this is true by default
**
**	@return	The season schedule if found, or null otherwise
*/
CSeasonSchedule *CSeasonSchedule::GetSeasonSchedule(const std::string &ident, const bool should_find)
{
	std::map<std::string, CSeasonSchedule *>::const_iterator find_iterator = SeasonSchedulesByIdent.find(ident);
	
	if (find_iterator != SeasonSchedulesByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid season schedule: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a season schedule
**
**	@param	ident	The season schedule's string identifier
**
**	@return	The season schedule if found, or a newly-created one otherwise
*/
CSeasonSchedule *CSeasonSchedule::GetOrAddSeasonSchedule(const std::string &ident)
{
	CSeasonSchedule *season_schedule = GetSeasonSchedule(ident, false);
	
	if (!season_schedule) {
		season_schedule = new CSeasonSchedule;
		season_schedule->Ident = ident;
		SeasonSchedules.push_back(season_schedule);
		SeasonSchedulesByIdent[ident] = season_schedule;
	}
	
	return season_schedule;
}

/**
**	@brief	Remove the existing season schedules
*/
void CSeasonSchedule::ClearSeasonSchedules()
{
	for (size_t i = 0; i < SeasonSchedules.size(); ++i) {
		delete SeasonSchedules[i];
	}
	SeasonSchedules.clear();
}

/**
**	@brief	Destructor
*/
CSeasonSchedule::~CSeasonSchedule()
{
	for (size_t i = 0; i < this->ScheduledSeasons.size(); ++i) {
		delete this->ScheduledSeasons[i];
	}
}


/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CSeasonSchedule::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else {
			fprintf(stderr, "Invalid season schedule property: \"%s\".\n", key.c_str());
		}
	}
	
	
	for (size_t i = 0; i < config_data->Children.size(); ++i) {
		CConfigData *child_config_data = config_data->Children[i];
		
		if (child_config_data->Tag == "scheduled_season") {
			CSeason *season = nullptr;
			int days = 0;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "scheduled_season") {
					value = FindAndReplaceString(value, "_", "-");
					season = CSeason::GetSeason(value);
				} else if (key == "days") {
					days = std::stoi(value);
				} else {
					fprintf(stderr, "Invalid scheduled season property: \"%s\".\n", key.c_str());
				}
			}
			
			if (!season) {
				fprintf(stderr, "Scheduled season has no season.\n");
				continue;
			}
			
			if (days <= 0) {
				fprintf(stderr, "Scheduled season has no days defined.\n");
				continue;
			}
			
			CScheduledSeason *scheduled_season = new CScheduledSeason;
			scheduled_season->Season = season;
			scheduled_season->Days = days;
			
			this->ScheduledSeasons.push_back(scheduled_season);
		} else {
			fprintf(stderr, "Invalid season schedule property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}

//@}
