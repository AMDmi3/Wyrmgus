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

#pragma once

#include "character.h" //for MaxCharacterTitles
#include "database/data_type.h"
#include "database/detailed_data_entry.h"
#include "gender.h"
#include "player.h" //for certain enums
#include "time/date.h"

class CAiBuildingTemplate;
class CCurrency;
class CDeity;
class CForceTemplate;
class CLanguage;
class CUpgrade;
enum class ForceType;
struct lua_State;

int CclDefineCivilization(lua_State *l);

namespace stratagus {

class calendar;
class quest;
class unit_class;
class upgrade_class;

class civilization final : public detailed_data_entry, public data_type<civilization>
{
	Q_OBJECT

	Q_PROPERTY(stratagus::civilization* parent_civilization MEMBER parent_civilization READ get_parent_civilization)
	Q_PROPERTY(bool visible MEMBER visible READ is_visible)
	Q_PROPERTY(bool playable MEMBER playable READ is_playable)
	Q_PROPERTY(QString interface READ get_interface_qstring)
	Q_PROPERTY(QString default_color READ get_default_color_qstring)
	Q_PROPERTY(CUpgrade* upgrade MEMBER upgrade READ get_upgrade)
	Q_PROPERTY(QStringList ship_names READ get_ship_names_qstring_list)

public:
	static constexpr const char *class_identifier = "civilization";
	static constexpr const char *database_folder = "civilizations";

	static civilization *add(const std::string &identifier, const stratagus::module *module)
	{
		civilization *civilization = data_type::add(identifier, module);
		civilization->ID = civilization::get_all().size() - 1;
		return civilization;
	}

	civilization(const std::string &identifier) : detailed_data_entry(identifier)
	{
	}

	~civilization();

	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize() override;

	civilization *get_parent_civilization() const
	{
		return this->parent_civilization;
	}
	
	int GetUpgradePriority(const CUpgrade *upgrade) const;
	int GetForceTypeWeight(const ForceType force_type) const;

	const std::string &get_interface() const
	{
		return this->interface;
	}

	QString get_interface_qstring() const
	{
		return QString::fromStdString(this->interface);
	}

	Q_INVOKABLE void set_interface(const std::string &interface)
	{
		this->interface = interface;
	}

	const std::string &get_default_color() const
	{
		return this->default_color;
	}

	QString get_default_color_qstring() const
	{
		return QString::fromStdString(this->default_color);
	}

	Q_INVOKABLE void set_default_color(const std::string &default_color)
	{
		this->default_color = default_color;
	}

	CUpgrade *get_upgrade() const
	{
		return this->upgrade;
	}

	calendar *get_calendar() const;
	CCurrency *GetCurrency() const;

	bool is_visible() const
	{
		return this->visible;
	}

	bool is_playable() const
	{
		return this->playable;
	}

	const std::vector<civilization *> &get_develops_from() const
	{
		return this->develops_from;
	}

	const std::vector<civilization *> &get_develops_to() const
	{
		return this->develops_to;
	}

	std::vector<CForceTemplate *> GetForceTemplates(const ForceType force_type) const;
	std::vector<CAiBuildingTemplate *> GetAiBuildingTemplates() const;

	const std::map<gender, std::vector<std::string>> &get_personal_names() const;
	const std::vector<std::string> &get_personal_names(const gender gender) const;

	void add_personal_name(const gender gender, const std::string &name)
	{
		this->personal_names[gender].push_back(name);
	}

	const std::vector<std::string> &get_surnames() const;

	void add_surname(const std::string &surname)
	{
		this->surnames.push_back(surname);
	}

	const std::vector<std::string> &get_unit_class_names(const unit_class *unit_class) const;
	const std::vector<std::string> &get_ship_names() const;
	QStringList get_ship_names_qstring_list() const;

	Q_INVOKABLE void add_ship_name(const std::string &ship_name)
	{
		this->ship_names.push_back(ship_name);
	}

	Q_INVOKABLE void remove_ship_name(const std::string &ship_name);

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

	CUpgrade *get_class_upgrade(const upgrade_class *upgrade_class) const;

	void set_class_upgrade(const upgrade_class *upgrade_class, CUpgrade *upgrade)
	{
		if (upgrade == nullptr) {
			this->class_upgrades.erase(upgrade_class);
			return;
		}

		this->class_upgrades[upgrade_class] = upgrade;
	}

	void remove_class_upgrade(CUpgrade *upgrade)
	{
		for (std::map<const upgrade_class *, CUpgrade *>::reverse_iterator iterator = this->class_upgrades.rbegin(); iterator != this->class_upgrades.rend(); ++iterator) {
			if (iterator->second == upgrade) {
				this->class_upgrades.erase(iterator->first);
			}
		}
	}

	const std::vector<character *> &get_characters() const
	{
		return this->characters;
	}

	void add_character(character *character)
	{
		this->characters.push_back(character);
	}

	int ID = -1;
private:
	civilization *parent_civilization = nullptr;
public:
	std::string Adjective;			/// adjective pertaining to the civilization
private:
	std::string interface; //the string identifier for the civilization's interface
	std::string default_color; //name of the civilization's default color (used for the encyclopedia, tech tree, etc.)
	CUpgrade *upgrade = nullptr;
public:
	CUnitSound UnitSounds;			/// sounds for unit events
	CLanguage *Language = nullptr;	/// the language used by the civilization
private:
	calendar *calendar = nullptr;	/// the calendar used by the civilization
public:
	CCurrency *Currency = nullptr;	/// the currency used by the civilization
private:
	bool visible = true; //whether the civilization is visible e.g. in the map editor
	bool playable = true; //civilizations are playable by default
	std::vector<civilization *> develops_from; //from which civilizations this civilization develops
	std::vector<civilization *> develops_to; //to which civilizations this civilization develops
public:
	std::vector<quest *> Quests;	/// quests belonging to this civilization
	std::map<const CUpgrade *, int> UpgradePriorities;		/// Priority for each upgrade
	std::map<ForceType, std::vector<CForceTemplate *>> ForceTemplates;	/// Force templates, mapped to each force type
	std::map<ForceType, int> ForceTypeWeights;	/// Weights for each force type
	std::vector<CAiBuildingTemplate *> AiBuildingTemplates;	/// AI building templates
private:
	std::map<gender, std::vector<std::string>> personal_names; //personal names for the civilization, mapped to the gender they pertain to (use gender::none for names which should be available for both genders)
	std::vector<std::string> surnames; //surnames for the civilization
	std::map<const unit_class *, std::vector<std::string>> unit_class_names;	/// Unit class names for the civilization, mapped to the unit class they pertain to, used for mechanical units, and buildings
public:
	std::vector<std::string> ProvinceNames;		/// Province names for the civilization
private:
	std::vector<std::string> ship_names;			/// Ship names for the civilization
	std::map<const unit_class *, CUnitType *> class_unit_types; //the unit type slot of a particular class for the civilization
	std::map<const upgrade_class *, CUpgrade *> class_upgrades; //the upgrade slot of a particular class for the civilization
	std::vector<character *> characters;
public:
	std::vector<CDeity *> Deities;
	std::vector<site *> sites; //sites used for this civilization if a randomly-generated one is required
	std::string MinisterTitles[MaxCharacterTitles][static_cast<int>(gender::count)][MaxGovernmentTypes][static_cast<int>(faction_tier::count)]; /// this civilization's minister title for each minister type and government type
	std::map<std::string, std::map<CDate, bool>> HistoricalUpgrades;	/// historical upgrades of the faction, with the date of change

	friend int ::CclDefineCivilization(lua_State *l);
};

}