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
//      (c) Copyright 1998-2020 by Lutz Sammer, Jimmy Salmon,
//      Russell Smith and Andrettin
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

#include "action/action_built.h"

#include "ai.h"
//Wyrmgus start
#include "ai/ai_local.h"
//Wyrmgus end
#include "character.h"
#include "commands.h"
#include "iolib.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tileset.h"
#include "player.h"
#include "quest/objective_type.h"
#include "quest/player_quest_objective.h"
#include "script.h"
#include "sound/sound.h"
#include "sound/unit_sound_type.h"
#include "translate.h"
#include "unit/construction.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_ref.h"
#include "unit/unit_type.h"
#include "util/vector_util.h"

/// How many resources the player gets back if canceling building
static constexpr int CancelBuildingCostsFactor = 75;

extern void AiReduceMadeInBuilt(PlayerAi &pai, const wyrmgus::unit_type &type, int landmass, const wyrmgus::site *settlement);

std::unique_ptr<COrder> COrder::NewActionBuilt(CUnit &builder, CUnit &unit)
{
	auto order = std::make_unique<COrder_Built>();

	// Make sure the bulding doesn't cancel itself out right away.

	unit.Variable[HP_INDEX].Value = 1;
	if (unit.Variable[SHIELD_INDEX].Max) {
		unit.Variable[SHIELD_INDEX].Value = 1;
	}
	order->UpdateConstructionFrame(unit);

	if (unit.Type->BoolFlag[BUILDEROUTSIDE_INDEX].value == false) {
		order->Worker = builder.acquire_ref();
	}
	return order;
}

COrder_Built::COrder_Built() : COrder(UnitAction::Built)
{
}

COrder_Built::~COrder_Built()
{
}

void COrder_Built::Save(CFile &file, const CUnit &unit) const
{
	file.printf("{\"action-built\", ");
	if (this->Finished) {
		file.printf(" \"finished\", ");
	}

	const wyrmgus::construction_frame *cframe = unit.get_construction()->get_initial_frame();

	int frame = 0;
	while (cframe != this->get_frame()) {
		cframe = cframe->get_next();
		++frame;
	}
	if (this->get_worker() != nullptr) {
		file.printf("\"worker\", \"%s\", ", UnitReference(this->get_worker()).c_str());
	}
	file.printf("\"progress\", %d, \"frame\", %d", this->ProgressCounter, frame);
	if (this->IsCancelled) {
		file.printf(", \"cancel\"");
	}
	file.printf("}");
}

bool COrder_Built::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	if (!strcmp(value, "worker")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		this->Worker = CclGetUnitFromRef(l)->acquire_ref();
		lua_pop(l, 1);
	} else if (!strcmp(value, "progress")) {
		++j;
		this->ProgressCounter = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "cancel")) {
		this->IsCancelled = true;
	} else if (!strcmp(value, "frame")) {
		++j;
		int frame = LuaToNumber(l, -1, j + 1);

		const wyrmgus::construction_frame *cframe = unit.get_construction()->get_initial_frame();
		
		while (frame-- && cframe->get_next() != nullptr) {
			cframe = cframe->get_next();
		}
		this->frame = cframe;
	} else {
		return false;
	}
	return true;
}

bool COrder_Built::IsValid() const
{
	return true;
}

PixelPos COrder_Built::Show(const CViewport &, const PixelPos &lastScreenPos) const
{
	return lastScreenPos;
}


static void CancelBuilt(COrder_Built &order, CUnit &unit)
{
	Assert(unit.CurrentOrder() == &order);
	CUnit *worker = order.get_worker();

	// Drop out unit
	if (worker != nullptr) {
		worker->ClearAction();

		DropOutOnSide(*worker, LookingW, &unit);
	}
	// Player gets back 75% of the original cost for a building.
	int type_costs[MaxCosts];
	unit.Player->GetUnitTypeCosts(unit.Type, type_costs, false, true);
	unit.Player->AddCostsFactor(type_costs, CancelBuildingCostsFactor);
	// Cancel building
	LetUnitDie(unit);
}

static void Finish(COrder_Built &order, CUnit &unit)
{
	const wyrmgus::unit_type &type = *unit.Type;
	CPlayer &player = *unit.Player;

	DebugPrint("%d: Building %s(%s) ready.\n" _C_ player.Index _C_ type.Ident.c_str() _C_ type.GetDefaultName(&player).c_str());

	// HACK: the building is ready now
	//Wyrmgus start
	if (!type.TerrainType) {
		player.NumBuildingsUnderConstruction--;
		player.ChangeUnitTypeUnderConstructionCount(&type, -1);
	}
	//Wyrmgus end

	player.IncreaseCountsForUnit(&unit);
	
	for (const auto &objective : player.get_quest_objectives()) {
		objective->on_unit_built(&unit);
	}

	if (unit.site != nullptr) {
		wyrmgus::site_game_data *site_game_data = unit.site->get_game_data();

		if (site_game_data->get_site_unit() == &unit) {
			if (player.Index != PlayerNumNeutral) {
				site_game_data->set_owner(&player);
			} else {
				site_game_data->set_owner(nullptr);
			}
		}
	}

	unit.UnderConstruction = 0;
	if (unit.Frame < 0) {
		unit.Frame = -1;
	} else {
		unit.Frame = 0;
	}
	CUnit *worker = order.get_worker();
	
	//Wyrmgus start
	int worker_count = 0;
	//count workers that are helping build the building, and make them harvest/return goods to it, if applicable
	std::vector<CUnit *> table;
	SelectAroundUnit(unit, 2, table);
	for (size_t i = 0; i != table.size(); ++i) {
		if (table[i]->CurrentAction() == UnitAction::Repair && table[i]->CurrentOrder()->get_goal() == &unit) {
			// If we can harvest from the new building, do it.
			if (type.get_given_resource() != nullptr && table[i]->Type->ResInfo[type.get_given_resource()->get_index()] != nullptr) {
				CommandResource(*table[i], unit, 0);
			}
			// If we can reurn goods to a new depot, do it.
			//Wyrmgus start
//			if (table[i]->CurrentResource && table[i]->ResourcesHeld > 0 && type.CanStore[table[i]->CurrentResource]) {
			if (table[i]->CanReturnGoodsTo(&unit) && table[i]->ResourcesHeld > 0) {
			//Wyrmgus end
				CommandReturnGoods(*table[i], &unit, 0);
			}
			worker_count += 1;
		}
	}
	
	//give builders experience for the construction of the structure
	int xp_gained = type.Stats[unit.Player->Index].Costs[TimeCost] / 10;
	//Wyrmgus end

	if (worker != nullptr) {
		//Wyrmgus start
		worker_count += 1;
		//Wyrmgus end
		
		if (type.BoolFlag[BUILDERLOST_INDEX].value) {
			// Bye bye worker.
			LetUnitDie(*worker);
			worker = nullptr;
		} else { // Drop out the worker.
			worker->ClearAction();

			DropOutOnSide(*worker, LookingW, &unit);

			// If we can harvest from the new building, do it.
			if (type.get_given_resource() != nullptr && worker->Type->ResInfo[type.get_given_resource()->get_index()] != nullptr) {
				CommandResource(*worker, unit, 0);
			}
			// If we can reurn goods to a new depot, do it.
			//Wyrmgus start
//			if (worker->CurrentResource && worker->ResourcesHeld > 0 && type.CanStore[worker->CurrentResource]) {
			if (worker->CanReturnGoodsTo(&unit) && worker->ResourcesHeld > 0) {
			//Wyrmgus end
				CommandReturnGoods(*worker, &unit, 0);
			}
			
			//Wyrmgus start
			// give experience to the builder
			worker->ChangeExperience(xp_gained / worker_count);
			//Wyrmgus end
		}
	}

	//Wyrmgus start
	for (size_t i = 0; i != table.size(); ++i) { // also give experience to all other workers who helped build the structure
		if (table[i]->CurrentAction() == UnitAction::Repair && table[i]->CurrentOrder()->get_goal() == &unit) {
			table[i]->ChangeExperience(xp_gained / worker_count);
		}
	}
			
//	if (type.GivesResource && type.StartingResources != 0) {
	if (type.get_given_resource() != nullptr) {
	//Wyrmgus end
		// Has StartingResources, Use those
		//Wyrmgus start
//		unit.ResourcesHeld = type.StartingResources[SyncRand(type.StartingResources.size())];
		if (type.StartingResources.size() > 0) {
			unit.SetResourcesHeld(type.StartingResources[SyncRand(type.StartingResources.size())]);
		}
		unit.GivesResource = type.get_given_resource()->get_index();
		//Wyrmgus end
	}

	//Wyrmgus start
	//we don't need to notify the player for every building constructed
//	player.Notify(NotifyGreen, unit.tilePos, _("New %s done"), type.Name.c_str());
	//Wyrmgus end
	if (&player == CPlayer::GetThisPlayer()) {
		if (type.MapSound->Ready.Sound) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::ready);
		}
		if (worker) {
			if (!type.TerrainType || worker->Orders.size() == 1 || worker->Orders[1]->Action != UnitAction::Build) {
				PlayUnitSound(*worker, wyrmgus::unit_sound_type::work_completed);
			}
		} else {
			//Wyrmgus start
			// why play the under-construction sound if the building has just been completed?
//			PlayUnitSound(unit, wyrmgus::unit_sound_type::construction);
			for (size_t i = 0; i != table.size(); ++i) { // see if there is a builder/repairer available to give the work completed voice, if the "worker" pointer is null
				if (table[i]->CurrentAction() == UnitAction::Repair && table[i]->CurrentOrder()->get_goal() == &unit) {
					if (!type.TerrainType || table[i]->Orders.size() == 1 || table[i]->Orders[1]->Action != UnitAction::Build) { //don't play the work complete sound if building a tile unit and the worker has further build orders, to prevent the voice from repetitively being played after each tile in a series is constructed
						PlayUnitSound(*table[i], wyrmgus::unit_sound_type::work_completed);
						break;
					}
				}
			}
			//Wyrmgus end
		}
	}

	if (player.AiEnabled) {
		/* Worker can be null */
		AiWorkComplete(worker, unit);
	}

	// FIXME: Vladi: this is just a hack to test wall fixing,
	// FIXME:  also not sure if the right place...
	// FIXME: Johns: hardcoded unit-type wall / more races!
	//Wyrmgus start
//	if (&type == UnitTypeOrcWall || &type == UnitTypeHumanWall) {
	if (type.TerrainType) {
	//Wyrmgus end
		//Wyrmgus start
//		CMap::Map.SetWall(unit.tilePos, &type == UnitTypeHumanWall);
		if (type.TerrainType->is_overlay() && CMap::Map.GetTileTerrain(unit.tilePos, type.TerrainType->is_overlay(), unit.MapLayer->ID) != nullptr) {
			//remove an existent overlay terrain if present, e.g. so that if a destroyed wall of the same type is present here, the new wall can be properly placed without still being destroyed
			CMap::Map.RemoveTileOverlayTerrain(unit.tilePos, unit.MapLayer->ID);
		}
		CMap::Map.SetTileTerrain(unit.tilePos, type.TerrainType, unit.MapLayer->ID);
		//Wyrmgus end
		unit.Remove(nullptr);
		UnitLost(unit);
		unit.clear_orders();
		unit.Release();
		return;
	}

	UpdateForNewUnit(unit, 0);

	// Set the direction of the building if it supports them
	if (type.NumDirections > 1 && type.BoolFlag[NORANDOMPLACING_INDEX].value == false) {
		if (type.BoolFlag[WALL_INDEX].value) { // Special logic for walls
			CorrectWallDirections(unit);
			CorrectWallNeighBours(unit);
		} else {
			unit.Direction = SyncRand(256); // random heading
		}
		UnitUpdateHeading(unit);
	}

	if (IsOnlySelected(unit) || &player == CPlayer::GetThisPlayer()) {
		SelectedUnitChanged();
	}
	MapUnmarkUnitSight(unit);
	unit.CurrentSightRange = unit.Stats->Variables[SIGHTRANGE_INDEX].Max;
	//Wyrmgus start
	UpdateUnitSightRange(unit);
	//Wyrmgus end
	MapMarkUnitSight(unit);
	//Wyrmgus start
	unit.UpdatePersonalName();
	unit.UpdateSoldUnits();
	//Wyrmgus end
	order.Finished = true;
}


/* virtual */ void COrder_Built::Execute(CUnit &unit)
{
	const wyrmgus::unit_type &type = *unit.Type;

	int amount;
	if (type.BoolFlag[BUILDEROUTSIDE_INDEX].value) {
		amount = type.AutoBuildRate;
	} else {
		// FIXME: implement this below:
		// this->Data.Worker->Type->BuilderSpeedFactor;
		amount = 100;
	}
	//Wyrmgus start
	if (!amount && unit.Player->has_neutral_faction_type()) { //trading companies and etc. get their buildings constructed automatically, since they aren't supposed to have workers
		amount = 100;
	}
	//Wyrmgus end
	this->Progress(unit, amount);

	// Check if construction should be canceled...
	if (this->IsCancelled || this->ProgressCounter < 0) {
		//Wyrmgus start
//		DebugPrint("%d: %s canceled.\n" _C_ unit.Player->Index _C_ unit.Type->Name.c_str());
		DebugPrint("%d: %s canceled.\n" _C_ unit.Player->Index _C_ unit.get_type_name().c_str());
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

void COrder_Built::Cancel(CUnit &unit)
{
	Q_UNUSED(unit)

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
	unit.Seen.cframe = this->get_frame();
}

/** Called when unit is killed.
**  warn the AI module.
*/
void COrder_Built::AiUnitKilled(CUnit &unit)
{
	DebugPrint("%d: %d(%s) killed, under construction!\n" _C_
			   unit.Player->Index _C_ UnitNumber(unit) _C_ unit.Type->Ident.c_str());
	//Wyrmgus start
//	AiReduceMadeInBuilt(*unit.Player->Ai, *unit.Type);
	AiReduceMadeInBuilt(*unit.Player->Ai, *unit.Type, CMap::Map.GetTileLandmass(unit.tilePos, unit.MapLayer->ID), unit.settlement);
	//Wyrmgus end
}

static const wyrmgus::construction_frame *FindCFramePercent(const wyrmgus::construction_frame *cframe, int percent)
{
	const wyrmgus::construction_frame *prev = cframe;

	for (const wyrmgus::construction_frame *it = cframe->get_next(); it; it = it->get_next()) {
		if (percent < it->get_percent()) {
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
	const wyrmgus::unit_type &type = *unit.Type;
	const int percent = this->ProgressCounter / (type.Stats[unit.Player->Index].Costs[TimeCost] * 6);

	const wyrmgus::construction_frame *cframe = FindCFramePercent(unit.get_construction()->get_initial_frame(), percent);

	Assert(cframe != nullptr);

	if (cframe != this->get_frame()) {
		this->frame = cframe;
		if (unit.Frame < 0) {
			unit.Frame = -cframe->get_frame() - 1;
		} else {
			unit.Frame = cframe->get_frame();
		}
	}
}


void COrder_Built::Progress(CUnit &unit, int amount)
{
	Boost(unit, amount, HP_INDEX);
	Boost(unit, amount, SHIELD_INDEX);

	//Wyrmgus start
//	this->ProgressCounter += std::max(1, amount * unit.Player->SpeedBuild / SPEEDUP_FACTOR);
	this->ProgressCounter += std::max(0, amount * unit.Player->SpeedBuild / SPEEDUP_FACTOR);
	//Wyrmgus end
	UpdateConstructionFrame(unit);
}

void COrder_Built::ProgressHp(CUnit &unit, int amount)
{
	Boost(unit, amount, HP_INDEX);

	//Wyrmgus start
//	this->ProgressCounter += std::max(1, amount * unit.Player->SpeedBuild / SPEEDUP_FACTOR);
	this->ProgressCounter += std::max(0, amount * unit.Player->SpeedBuild / SPEEDUP_FACTOR);
	//Wyrmgus end
	UpdateConstructionFrame(unit);
}

CUnit *COrder_Built::get_worker() const
{
	if (this->Worker == nullptr) {
		return nullptr;
	}

	return this->Worker->get();
}

void COrder_Built::Boost(CUnit &building, int amount, int varIndex) const
{
	Assert(building.CurrentOrder() == this);

	const int costs = building.Stats->Costs[TimeCost] * 600;
	const int progress = this->ProgressCounter;
	//Wyrmgus start
//	const int newProgress = progress + std::max(1, amount * building.Player->SpeedBuild / SPEEDUP_FACTOR);
	const int newProgress = progress + std::max(0, amount * building.Player->SpeedBuild / SPEEDUP_FACTOR);
	//Wyrmgus end
	const int maxValue = building.Variable[varIndex].Max;

	int &currentValue = building.Variable[varIndex].Value;

	// damageValue is the current damage taken by the unit.
	const int damageValue = (progress * maxValue) / costs - currentValue;

	// Keep the same level of damage while increasing Value.
	currentValue = (newProgress * maxValue) / costs - damageValue;
	currentValue = std::min(currentValue, maxValue);
}
