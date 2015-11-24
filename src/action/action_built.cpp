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
/**@name action_build.cpp - The build building action. */
//
//      (c) Copyright 1998-2012 by Lutz Sammer, Jimmy Salmon, and
//                                 Russell Smith
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

#include "stratagus.h"
#include "action/action_built.h"

#include "ai.h"
#include "commands.h"
#include "construct.h"
#include "iolib.h"
#include "map.h"
#include "player.h"
#include "script.h"
#include "sound.h"
#include "translate.h"
#include "unit.h"
//Wyrmgus start
#include "unit_find.h"
//Wyrmgus end
#include "unittype.h"

/// How many resources the player gets back if canceling building
#define CancelBuildingCostsFactor  75


extern void AiReduceMadeInBuilt(PlayerAi &pai, const CUnitType &type);

/* static */ COrder *COrder::NewActionBuilt(CUnit &builder, CUnit &unit)
{
	COrder_Built *order = new COrder_Built();

	// Make sure the bulding doesn't cancel itself out right away.

	unit.Variable[HP_INDEX].Value = 1;
	if (unit.Variable[SHIELD_INDEX].Max) {
		unit.Variable[SHIELD_INDEX].Value = 1;
	}
	order->UpdateConstructionFrame(unit);

	//Wyrmgus start
	//workers building from outside shouldn't be treated differently in this instance
//	if (unit.Type->BuilderOutside == false) {
		order->Worker = &builder;
//	}
	//Wyrmgus end
	return order;
}


/* virtual */ void COrder_Built::Save(CFile &file, const CUnit &unit) const
{
	file.printf("{\"action-built\", ");
	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	//Wyrmgus start
//	CConstructionFrame *cframe = unit.Type->Construction->Frames;
	CConstructionFrame *cframe = unit.GetConstruction()->Frames;
	//Wyrmgus end
	int frame = 0;
	while (cframe != this->Frame) {
		cframe = cframe->Next;
		++frame;
	}
	if (this->Worker != NULL) {
		file.printf("\"worker\", \"%s\", ", UnitReference(this->Worker).c_str());
	}
	file.printf("\"progress\", %d, \"frame\", %d", this->ProgressCounter, frame);
	if (this->IsCancelled) {
		file.printf(", \"cancel\"");
	}
	file.printf("}");
}

/* virtual */ bool COrder_Built::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	if (!strcmp(value, "worker")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		this->Worker = CclGetUnitFromRef(l);
		lua_pop(l, 1);
	} else if (!strcmp(value, "progress")) {
		++j;
		this->ProgressCounter = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "cancel")) {
		this->IsCancelled = true;
	} else if (!strcmp(value, "frame")) {
		++j;
		int frame = LuaToNumber(l, -1, j + 1);
		//Wyrmgus start
//		CConstructionFrame *cframe = unit.Type->Construction->Frames;
		CConstructionFrame *cframe = unit.GetConstruction()->Frames;
		//Wyrmgus end
		while (frame-- && cframe->Next != NULL) {
			cframe = cframe->Next;
		}
		this->Frame = cframe;
	} else {
		return false;
	}
	return true;
}

/* virtual */ bool COrder_Built::IsValid() const
{
	return true;
}

/* virtual */ PixelPos COrder_Built::Show(const CViewport &, const PixelPos &lastScreenPos) const
{
	return lastScreenPos;
}


static void CancelBuilt(COrder_Built &order, CUnit &unit)
{
	Assert(unit.CurrentOrder() == &order);
	CUnit *worker = order.GetWorkerPtr();

	// Drop out unit
	if (worker != NULL) {
		worker->ClearAction();

		//Wyrmgus start
//		DropOutOnSide(*worker, LookingW, &unit);
		if (unit.Type->BoolFlag[BUILDEROUTSIDE_INDEX].value == false) {
			DropOutOnSide(*worker, LookingW, &unit);
		}
		//Wyrmgus end
	}
	// Player gets back 75% of the original cost for a building.
	unit.Player->AddCostsFactor(unit.Stats->Costs, CancelBuildingCostsFactor);
	// Cancel building
	LetUnitDie(unit);
}

static void Finish(COrder_Built &order, CUnit &unit)
{
	const CUnitType &type = *unit.Type;
	CPlayer &player = *unit.Player;

	//Wyrmgus start
//	DebugPrint("%d: Building %s(%s) ready.\n" _C_ player.Index _C_ type.Ident.c_str() _C_ type.Name.c_str());
	VariationInfo *varinfo = type.GetDefaultVariation(player);
	if (varinfo && !varinfo->TypeName.empty()) {
		DebugPrint("%d: Building %s(%s) ready.\n" _C_ player.Index _C_ type.Ident.c_str() _C_ varinfo->TypeName.c_str());
	} else {
		DebugPrint("%d: Building %s(%s) ready.\n" _C_ player.Index _C_ type.Ident.c_str() _C_ type.Name.c_str());
	}
	//Wyrmgus end

	// HACK: the building is ready now
	player.UnitTypesCount[type.Slot]++;
	//Wyrmgus start
	if (unit.Active) {
		player.UnitTypesAiActiveCount[type.Slot]++;
	}
	//Wyrmgus end
	unit.Constructed = 0;
	if (unit.Frame < 0) {
		unit.Frame = -1;
	} else {
		unit.Frame = 0;
	}
	CUnit *worker = order.GetWorkerPtr();

	if (worker != NULL) {
		//Wyrmgus start
//		if (type.BuilderLost) {
		if (type.BoolFlag[BUILDERLOST_INDEX].value) {
		//Wyrmgus end
			// Bye bye worker.
			LetUnitDie(*worker);
			worker = NULL;
		} else { // Drop out the worker.
			worker->ClearAction();

			//Wyrmgus start
//			DropOutOnSide(*worker, LookingW, &unit);
			if (unit.Type->BoolFlag[BUILDEROUTSIDE_INDEX].value == false) {
				DropOutOnSide(*worker, LookingW, &unit);
			}
			//Wyrmgus end

			// If we can harvest from the new building, do it.
			if (worker->Type->ResInfo[type.GivesResource]) {
				CommandResource(*worker, unit, 0);
			}
			// If we can reurn goods to a new depot, do it.
			if (worker->CurrentResource && worker->ResourcesHeld > 0 && type.CanStore[worker->CurrentResource]) {
				CommandReturnGoods(*worker, &unit, 0);
			}
			
			//Wyrmgus start
			//make workers that are helping build the building also harvest/return goods to it, if applicable
			std::vector<CUnit *> table;
			SelectAroundUnit(unit, 2, table);
			for (size_t i = 0; i != table.size(); ++i) {
				if (table[i]->CurrentAction() == UnitActionRepair && table[i]->CurrentOrder()->GetGoal() == &unit) {
					// If we can harvest from the new building, do it.
					if (table[i]->Type->ResInfo[type.GivesResource]) {
						CommandResource(*table[i], unit, 0);
					}
					// If we can reurn goods to a new depot, do it.
					if (table[i]->CurrentResource && table[i]->ResourcesHeld > 0 && type.CanStore[table[i]->CurrentResource]) {
						CommandReturnGoods(*table[i], &unit, 0);
					}
				}
			}
			//Wyrmgus end
		}
	}

	if (type.GivesResource && type.StartingResources != 0) {
		// Has StartingResources, Use those
		unit.ResourcesHeld = type.StartingResources;
	}

	//Wyrmgus start
	//we don't need to notify the player for every building constructed
//	player.Notify(NotifyGreen, unit.tilePos, _("New %s done"), type.Name.c_str());
	//Wyrmgus end
	if (&player == ThisPlayer) {
		//Wyrmgus start
		/*
		if (type.Sound.Ready.Sound) {
			PlayUnitSound(unit, VoiceReady);
		} else if (worker) {
			PlayUnitSound(*worker, VoiceWorkCompleted);
		*/
		if (type.MapSound.Ready.Sound) {
			PlayUnitSound(unit, VoiceReady);
		}
		if (worker) {
			PlayUnitSound(*worker, VoiceWorkCompleted);
		//Wyrmgus end
		//Wyrmgus start
		// why play the under-construction sound if the building has just been completed?
//		} else {
//			PlayUnitSound(unit, VoiceBuilding);
		//Wyrmgus end
		}
	}

	if (player.AiEnabled) {
		/* Worker can be NULL */
		AiWorkComplete(worker, unit);
	}

	// FIXME: Vladi: this is just a hack to test wall fixing,
	// FIXME:  also not sure if the right place...
	// FIXME: Johns: hardcoded unit-type wall / more races!
	if (&type == UnitTypeOrcWall || &type == UnitTypeHumanWall) {
		Map.SetWall(unit.tilePos, &type == UnitTypeHumanWall);
		unit.Remove(NULL);
		UnitLost(unit);
		UnitClearOrders(unit);
		unit.Release();
		return ;
	}

	UpdateForNewUnit(unit, 0);

	// Set the direction of the building if it supports them
	//Wyrmgus start
//	if (type.NumDirections > 1 && type.NoRandomPlacing == false) {
	if (type.NumDirections > 1 && type.BoolFlag[NORANDOMPLACING_INDEX].value == false) {
	//Wyrmgus end
		//Wyrmgus start
//		if (type.Wall) { // Special logic for walls
		if (type.BoolFlag[WALL_INDEX].value) { // Special logic for walls
		//Wyrmgus end
			CorrectWallDirections(unit);
			CorrectWallNeighBours(unit);
		} else {
			unit.Direction = (MyRand() >> 8) & 0xFF; // random heading
		}
		UnitUpdateHeading(unit);
	}

	if (IsOnlySelected(unit) || &player == ThisPlayer) {
		SelectedUnitChanged();
	}
	MapUnmarkUnitSight(unit);
	unit.CurrentSightRange = unit.Stats->Variables[SIGHTRANGE_INDEX].Max;
	//Wyrmgus start
	UpdateUnitSightRange(unit);
	//Wyrmgus end
	MapMarkUnitSight(unit);
	order.Finished = true;
}


/* virtual */ void COrder_Built::Execute(CUnit &unit)
{
	const CUnitType &type = *unit.Type;

	int amount;
	//Wyrmgus start
//	if (type.BuilderOutside) {
	if (type.BoolFlag[BUILDEROUTSIDE_INDEX].value) {
	//Wyrmgus end
		amount = type.AutoBuildRate;
	} else {
		// FIXME: implement this below:
		// this->Data.Worker->Type->BuilderSpeedFactor;
		amount = 100;
	}
	this->Progress(unit, amount);

	// Check if construction should be canceled...
	if (this->IsCancelled || this->ProgressCounter < 0) {
		//Wyrmgus start
//		DebugPrint("%d: %s canceled.\n" _C_ unit.Player->Index _C_ unit.Type->Name.c_str());
		VariationInfo *varinfo = unit.Type->VarInfo[unit.Variation];
		if (varinfo && !varinfo->TypeName.empty()) {
			DebugPrint("%d: %s canceled.\n" _C_ unit.Player->Index _C_ varinfo->TypeName.c_str());
		} else {
			DebugPrint("%d: %s canceled.\n" _C_ unit.Player->Index _C_ unit.Type->Name.c_str());
		}
		//Wyrmgus end

		CancelBuilt(*this, unit);
		return ;
	}

	const int maxProgress = type.Stats[unit.Player->Index].Costs[TimeCost] * 600;

	// Check if building ready. Note we can both build and repair.
	if (!unit.Anim.Unbreakable && this->ProgressCounter >= maxProgress) {
		Finish(*this, unit);
	}
}

/* virtual */ void COrder_Built::Cancel(CUnit &unit)
{
	this->IsCancelled = true;
}

/* virtual */ void COrder_Built::UpdateUnitVariables(CUnit &unit) const
{
	Assert(unit.CurrentOrder() == this);

	unit.Variable[BUILD_INDEX].Value = this->ProgressCounter;
	unit.Variable[BUILD_INDEX].Max = unit.Type->Stats[unit.Player->Index].Costs[TimeCost] * 600;

	// This should happen when building unit with several peons
	// Maybe also with only one.
	// FIXME : Should be better to fix it in action_{build,repair}.c ?
	unit.Variable[BUILD_INDEX].Value = std::min(unit.Variable[BUILD_INDEX].Max, unit.Variable[BUILD_INDEX].Value);
}

/* virtual */ void COrder_Built::FillSeenValues(CUnit &unit) const
{
	unit.Seen.State = 1;
	unit.Seen.CFrame = this->Frame;
}

/** Called when unit is killed.
**  warn the AI module.
*/
void COrder_Built::AiUnitKilled(CUnit &unit)
{
	DebugPrint("%d: %d(%s) killed, under construction!\n" _C_
			   unit.Player->Index _C_ UnitNumber(unit) _C_ unit.Type->Ident.c_str());
	AiReduceMadeInBuilt(*unit.Player->Ai, *unit.Type);
}


static const CConstructionFrame *FindCFramePercent(const CConstructionFrame &cframe, int percent)
{
	const CConstructionFrame *prev = &cframe;

	for (const CConstructionFrame *it = cframe.Next; it; it = it->Next) {
		if (percent < it->Percent) {
			return prev;
		}
		prev = it;
	}
	return prev;
}

/**
**  Update construction frame
**
**  @param unit  The building under construction.
*/
void COrder_Built::UpdateConstructionFrame(CUnit &unit)
{
	const CUnitType &type = *unit.Type;
	const int percent = this->ProgressCounter / (type.Stats[unit.Player->Index].Costs[TimeCost] * 6);
	//Wyrmgus start
//	const CConstructionFrame *cframe = FindCFramePercent(*type.Construction->Frames, percent);
	const CConstructionFrame *cframe = FindCFramePercent(*unit.GetConstruction()->Frames, percent);
	//Wyrmgus end

	Assert(cframe != NULL);

	if (cframe != this->Frame) {
		this->Frame = cframe;
		if (unit.Frame < 0) {
			unit.Frame = -cframe->Frame - 1;
		} else {
			unit.Frame = cframe->Frame;
		}
	}
}


void COrder_Built::Progress(CUnit &unit, int amount)
{
	Boost(unit, amount, HP_INDEX);
	Boost(unit, amount, SHIELD_INDEX);

	this->ProgressCounter += std::max(1, amount * unit.Player->SpeedBuild / SPEEDUP_FACTOR);
	UpdateConstructionFrame(unit);
}

void COrder_Built::ProgressHp(CUnit &unit, int amount)
{
	Boost(unit, amount, HP_INDEX);

	this->ProgressCounter += std::max(1, amount * unit.Player->SpeedBuild / SPEEDUP_FACTOR);
	UpdateConstructionFrame(unit);
}


void COrder_Built::Boost(CUnit &building, int amount, int varIndex) const
{
	Assert(building.CurrentOrder() == this);

	const int costs = building.Stats->Costs[TimeCost] * 600;
	const int progress = this->ProgressCounter;
	const int newProgress = progress + std::max(1, amount * building.Player->SpeedBuild / SPEEDUP_FACTOR);
	const int maxValue = building.Variable[varIndex].Max;

	int &currentValue = building.Variable[varIndex].Value;

	// damageValue is the current damage taken by the unit.
	const int damageValue = (progress * maxValue) / costs - currentValue;

	// Keep the same level of damage while increasing Value.
	currentValue = (newProgress * maxValue) / costs - damageValue;
	currentValue = std::min(currentValue, maxValue);
}

//@}
