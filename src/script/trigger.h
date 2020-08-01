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
//      (c) Copyright 2002-2020 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "database/data_entry.h"
#include "database/data_type.h"

class CFile;
class CPlayer;
class CUnit;
class CUpgrade;
class LuaCallback;
struct lua_State;

namespace stratagus {
	class condition;
	class effect;
	class effect_list;
	class faction;
	class unit_type;
}

class CTimer
{
public:
	void Reset()
	{
		Init = false;
		Running = false;
		Increasing = false;
		Cycles = 0;
		LastUpdate = 0;
	}

	bool Init = false;				/// timer is initialized
	bool Running = false;			/// timer is running
	bool Increasing = false;		/// increasing or decreasing
	long Cycles = 0;				/// current value in game cycles
	unsigned long LastUpdate = 0;	/// GameCycle of last update
};

namespace stratagus {

class trigger final : public data_entry, public data_type<trigger>
{
	Q_OBJECT

	Q_PROPERTY(bool only_once MEMBER only_once READ fires_only_once)
	Q_PROPERTY(bool campaign_only MEMBER campaign_only READ is_campaign_only)

public:
	enum class TriggerType
	{
		GlobalTrigger = 0, //checked once
		PlayerTrigger //checked for each player
	};

	static constexpr const char *class_identifier = "trigger";
	static constexpr const char *database_folder = "triggers";

	static void clear();
	static void InitActiveTriggers();	/// Setup triggers
	static void ClearActiveTriggers();

	static std::vector<trigger *> ActiveTriggers; //triggers that are active for the current game
	static std::vector<std::string> DeactivatedTriggers;
	static unsigned int CurrentTriggerId;

	explicit trigger(const std::string &identifier);
	~trigger();
	
	virtual void process_sml_property(const sml_property &property) override;
	virtual void process_sml_scope(const sml_data &scope) override;

	bool fires_only_once() const
	{
		return this->only_once;
	}

	bool is_campaign_only() const
	{
		return this->campaign_only;
	}

	condition *get_preconditions() const
	{
		return this->preconditions;
	}

	condition *get_conditions() const
	{
		return this->conditions;
	}

	TriggerType Type = TriggerType::GlobalTrigger;
	bool Local = false;
private:
	bool only_once = false;				/// Whether the trigger should occur only once in a game
	bool campaign_only = false;			/// Whether the trigger should only occur in the campaign mode
public:
	LuaCallback *Conditions = nullptr;
	LuaCallback *Effects = nullptr;
private:
	condition *preconditions = nullptr;
	condition *conditions = nullptr;
public:
	std::unique_ptr<effect_list> effects;
};

}

#define ANY_UNIT ((const stratagus::unit_type *)0)
#define ALL_FOODUNITS ((const stratagus::unit_type *)-1)
#define ALL_BUILDINGS ((const stratagus::unit_type *)-2)

/**
**  Data to referer game info when game running.
*/
struct TriggerDataType {
	CUnit *Attacker;  /// Unit which send the missile.
	CUnit *Defender;  /// Unit which is hit by missile.
	CUnit *Active;    /// Unit which is selected or else under cursor unit.
	//Wyrmgus start
	CUnit *Unit;	  /// Unit used in trigger
	//Wyrmgus end
	const stratagus::unit_type *Type;  /// Type used in trigger;
	//Wyrmgus start
	const CUpgrade *Upgrade; /// Upgrade used in trigger
	int *Resource;		/// Resource used in trigger
	stratagus::faction *Faction; /// Faction used in trigger
	//Wyrmgus end
	const CPlayer *player = nullptr;
};

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern CTimer GameTimer; /// the game timer

/// Some data accessible for script during the game.
extern TriggerDataType TriggerData;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern int TriggerGetPlayer(lua_State *l);/// get player number.
extern const stratagus::unit_type *TriggerGetUnitType(lua_State *l); /// get the unit-type
extern void TriggersEachCycle();    /// test triggers

extern void TriggerCclRegister();   /// Register ccl features
extern void SaveTriggers(CFile &file); /// Save the trigger module
