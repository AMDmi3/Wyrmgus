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
//      (c) Copyright 2019-2020 by Andrettin
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

#include "database/data_type.h"
#include "database/detailed_data_entry.h"
#include "player.h" //for certain enums
#include "ui/icon.h"

class CAiBuildingTemplate;
class CCurrency;
class CDeity;
class CDynasty;
class CForceTemplate;
class CUnitType;
class CUpgrade;
class LuaCallback;

int CclDefineFaction(lua_State *l);

namespace stratagus {

class unit_class;

class faction : public detailed_data_entry, public data_type<faction>
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "faction";
	static constexpr const char *database_folder = "factions";

	static faction *add(const std::string &identifier, const stratagus::module *module)
	{
		faction *faction = data_type::add(identifier, module);
		faction->ID = faction::get_all().size() - 1;
		return faction;
	}

	faction(const std::string &identifier) : detailed_data_entry(identifier)
	{
	}

	~faction();

	virtual void check() const override;
	
	int GetUpgradePriority(const CUpgrade *upgrade) const;
	int GetForceTypeWeight(int force_type) const;
	CCurrency *GetCurrency() const;
	std::vector<CForceTemplate *> GetForceTemplates(int force_type) const;
	std::vector<CAiBuildingTemplate *> GetAiBuildingTemplates() const;
	const std::vector<std::string> &get_ship_names() const;

	CUnitType *get_class_unit_type(const unit_class *unit_class) const;

	void set_class_unit_type(const unit_class *unit_class, CUnitType *unit_type)
	{
		if (unit_type == nullptr) {
			this->class_unit_types.erase(unit_class);
			return;
		}

		this->class_unit_types[unit_class] = unit_type;
	}

	void remove_class_unit_type(CUnitType *unit_type)
	{
		for (std::map<const unit_class *, CUnitType *>::reverse_iterator iterator = this->class_unit_types.rbegin(); iterator != this->class_unit_types.rend(); ++iterator) {
			if (iterator->second == unit_type) {
				this->class_unit_types.erase(iterator->first);
			}
		}
	}

	std::string FactionUpgrade;											/// faction upgrade applied when the faction is set
	std::string Adjective;												/// adjective pertaining to the faction
	std::string DefaultAI = "land-attack";
	int ID = -1;														/// faction ID
	stratagus::civilization *civilization = nullptr; //faction civilization
	int Type = FactionTypeNoFactionType;								/// faction type (i.e. tribe or polity)
	int DefaultTier = FactionTierBarony;								/// default faction tier
	int DefaultGovernmentType = GovernmentTypeMonarchy;					/// default government type
	int ParentFaction = -1;												/// parent faction of this faction
	bool Playable = true;												/// faction playability
	bool DefiniteArticle = false;										/// whether the faction's name should be preceded by a definite article (e.g. "the Netherlands")
	IconConfig Icon;													/// Faction's icon
	CCurrency *Currency = nullptr;										/// The faction's currency
	CDeity *HolyOrderDeity = nullptr;									/// deity this faction belongs to, if it is a holy order
	LuaCallback *Conditions = nullptr;
	std::vector<int> Colors;											/// faction colors
	std::vector<faction *> DevelopsFrom;								/// from which factions can this faction develop
	std::vector<faction *> DevelopsTo;									/// to which factions this faction can develop
	std::vector<CDynasty *> Dynasties;									/// which dynasties are available to this faction
	std::string Titles[MaxGovernmentTypes][MaxFactionTiers];			/// this faction's title for each government type and faction tier
	std::string MinisterTitles[MaxCharacterTitles][MaxGenders][MaxGovernmentTypes][MaxFactionTiers]; /// this faction's minister title for each minister type and government type
	std::map<const CUpgrade *, int> UpgradePriorities;					/// Priority for each upgrade
	std::map<ButtonCmd, IconConfig> ButtonIcons;								/// icons for button actions
	std::map<const unit_class *, CUnitType *> class_unit_types; //the unit type slot of a particular class for a particular faction
	std::map<int, int> ClassUpgrades;									/// the upgrade slot of a particular class for a particular faction
	std::vector<std::string> ProvinceNames;								/// Province names for the faction
private:
	std::vector<std::string> ship_names;								/// Ship names for the faction
public:
	std::vector<site *> Cores; /// Core sites of this faction (required to found it)
	std::vector<site *> sites; /// Sites used for this faction if it needs a randomly-generated settlement
	std::map<int, std::vector<CForceTemplate *>> ForceTemplates;		/// Force templates, mapped to each force type
	std::map<int, int> ForceTypeWeights;								/// Weights for each force type
	std::vector<CAiBuildingTemplate *> AiBuildingTemplates;				/// AI building templates
	std::map<std::tuple<CDate, CDate, int>, CCharacter *> HistoricalMinisters;	/// historical ministers of the faction (as well as heads of state and government), mapped to the beginning and end of the rule, and the enum of the title in question
	std::map<std::string, std::map<CDate, bool>> HistoricalUpgrades;	/// historical upgrades of the faction, with the date of change
	std::map<int, int> HistoricalTiers;									/// dates in which this faction's tier changed; faction tier mapped to year
	std::map<int, int> HistoricalGovernmentTypes;						/// dates in which this faction's government type changed; government type mapped to year
	std::map<std::pair<CDate, faction *>, Diplomacy> HistoricalDiplomacyStates;	/// dates in which this faction's diplomacy state to another faction changed; diplomacy state mapped to year and faction
	std::map<std::pair<CDate, int>, int> HistoricalResources;	/// dates in which this faction's storage of a particular resource changed; resource quantities mapped to date and resource
	std::vector<std::pair<CDate, std::string>> HistoricalCapitals;		/// historical capitals of the faction; the values are: date and settlement ident
	std::vector<CFiller> UIFillers;
	
	std::string Mod;							/// To which mod (or map), if any, this faction belongs

	friend int ::CclDefineFaction(lua_State *l);
};

}
