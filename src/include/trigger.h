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
/**@name trigger.h - The game trigger headerfile. */
//
//      (c) Copyright 2002-2005 by Lutz Sammer and Jimmy Salmon
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

#ifndef __TRIGGER_H__
#define __TRIGGER_H__

//@{

//Wyrmgus start
/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <vector>
#include <map>
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

//Wyrmgus start
class CFaction;
//Wyrmgus end
class CUnit;
class CUnitType;
//Wyrmgus start
class CUpgrade;
//Wyrmgus end
struct lua_State;
class CFile;
//Wyrmgus start
class LuaCallback;
//Wyrmgus end

/**
**  Timer structure
*/
class CTimer
{
public:
	CTimer() : Init(false), Running(false), Increasing(false), Cycles(0),
		LastUpdate(0) {}

	void Reset()
	{
		Init = false;
		Running = false;
		Increasing = false;
		Cycles = 0;
		LastUpdate = 0;
	}

	bool Init;                  /// timer is initialized
	bool Running;               /// timer is running
	bool Increasing;            /// increasing or decreasing
	long Cycles;                /// current value in game cycles
	unsigned long LastUpdate;   /// GameCycle of last update
};

//Wyrmgus start
class CTrigger
{
public:
	CTrigger() :
		Conditions(NULL), Effects(NULL)
	{
	}
	~CTrigger();
	
	std::string Ident;
	LuaCallback *Conditions;
	LuaCallback *Effects;
};
//Wyrmgus end

#define ANY_UNIT ((const CUnitType *)0)
#define ALL_FOODUNITS ((const CUnitType *)-1)
#define ALL_BUILDINGS ((const CUnitType *)-2)


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
	CUnitType *Type;  /// Type used in trigger;
	//Wyrmgus start
	CUpgrade *Upgrade; /// Upgrade used in trigger
	int *Resource;		/// Resource used in trigger
	CFaction *Faction; /// Faction used in trigger
	//Wyrmgus end
};

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern CTimer GameTimer; /// the game timer

/// Some data accessible for script during the game.
extern TriggerDataType TriggerData;

//Wyrmgus start
extern std::vector<CTrigger *> Triggers;
extern std::vector<std::string> DeactivatedTriggers;
extern std::map<std::string, CTrigger *> TriggerIdentToPointer;
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern int TriggerGetPlayer(lua_State *l);/// get player number.
extern const CUnitType *TriggerGetUnitType(lua_State *l); /// get the unit-type
extern void TriggersEachCycle();    /// test triggers

//Wyrmgus start
extern CTrigger *GetTrigger(std::string trigger_ident);
//Wyrmgus end

extern void TriggerCclRegister();   /// Register ccl features
extern void SaveTriggers(CFile &file); /// Save the trigger module
extern void InitTriggers();         /// Setup triggers
extern void CleanTriggers();        /// Cleanup the trigger module

//@}

#endif // !__TRIGGER_H__
