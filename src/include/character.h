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
//      (c) Copyright 2015-2020 by Andrettin
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
#include "data_type.h"
#include "item_slot.h"
#include "time/date.h"
#include "ui/icon.h"

class CFile;
class CLanguage;
class CPersistentItem;
class CProvince;
class CUnit;
class CUpgrade;
class LuaCallback;
class Spell_Polymorph;
struct lua_State;

void ChangeCustomHeroCivilization(const std::string &hero_full_name, const std::string &civilization_name, const std::string &new_hero_name, const std::string &new_hero_family_name);
int CclDefineCharacter(lua_State *l);
int CclDefineCustomHero(lua_State *l);

namespace stratagus {
	class calendar;
	class civilization;
	class condition;
	class deity;
	class faction;
	class historical_location;
	class quest;
	class religion;
	class site;
	class unit_type;
	enum class gender;
}

enum Attributes {
	StrengthAttribute,
	DexterityAttribute,
	IntelligenceAttribute,
	CharismaAttribute,

	MaxAttributes
};

namespace stratagus {

enum class character_title {
	none = -1,
	head_of_state, // also used for titulars to aristocratic titles which were formal only; for example: the French duke of Orléans did not rule over Orléans, but here we consider the "head of state" title to also encompass such cases
	head_of_government,
	education_minister,
	finance_minister,
	foreign_minister,
	intelligence_minister,
	interior_minister,
	justice_minister,
	war_minister,
	
	governor,
	mayor
};

class character : public detailed_data_entry, public data_type<character>, public CDataType
{
	Q_OBJECT

	Q_PROPERTY(QString surname READ get_surname_qstring)
	Q_PROPERTY(stratagus::unit_type* unit_type READ get_unit_type WRITE set_unit_type)
	Q_PROPERTY(stratagus::civilization* civilization MEMBER civilization READ get_civilization)
	Q_PROPERTY(stratagus::faction* default_faction MEMBER default_faction READ get_default_faction)
	Q_PROPERTY(stratagus::gender gender MEMBER gender READ get_gender)
	Q_PROPERTY(stratagus::site* home_settlement MEMBER home_settlement)
	Q_PROPERTY(QString variation READ get_variation_qstring)
	Q_PROPERTY(bool ai_active MEMBER ai_active READ is_ai_active)
	Q_PROPERTY(CUpgrade* trait MEMBER trait READ get_trait)
	Q_PROPERTY(bool active MEMBER active READ is_active)
	Q_PROPERTY(stratagus::faction *faction MEMBER faction READ get_faction)

public:
	static constexpr const char *class_identifier = "character";
	static constexpr const char *database_folder = "characters";

	static void clear();
	
	explicit character(const std::string &identifier);
	~character();
	
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void process_sml_dated_scope(const sml_data &scope, const QDateTime &date) override;
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void initialize() override;
	virtual void check() const override;
	virtual void reset_history() override;

	const std::string &get_surname() const
	{
		return this->surname;
	}

	Q_INVOKABLE void set_surname(const std::string &surname)
	{
		this->surname = surname;
	}

	QString get_surname_qstring() const
	{
		return QString::fromStdString(this->get_surname());
	}

	unit_type *get_unit_type() const
	{
		return this->unit_type;
	}

	void set_unit_type(unit_type *unit_type)
	{
		if (unit_type == this->get_unit_type()) {
			return;
		}

		this->unit_type = unit_type;
	}

	CUpgrade *get_trait() const
	{
		return this->trait;
	}

	civilization *get_civilization() const
	{
		return this->civilization;
	}

	faction *get_default_faction() const
	{
		return this->default_faction;
	}

	gender get_gender() const
	{
		return this->gender;
	}

	void GenerateMissingDates();
	int GetMartialAttribute() const;
	int GetAttributeModifier(int attribute) const;
	religion *get_religion() const;
	CLanguage *GetLanguage() const;
	calendar *get_calendar() const;
	bool IsParentOf(const std::string &child_full_name) const;
	bool IsChildOf(const std::string &parent_full_name) const;
	bool IsSiblingOf(const std::string &sibling_full_name) const;
	bool IsItemEquipped(const CPersistentItem *item) const;
	bool IsUsable() const;
	bool CanAppear(bool ignore_neutral = false) const;
	bool CanWorship() const;
	bool HasMajorDeity() const;
	std::string GetFullName() const;

	const std::string &get_variation() const
	{
		return this->variation;
	}

	Q_INVOKABLE void set_variation(const std::string &variation)
	{
		this->variation = FindAndReplaceString(variation, "_", "-");
	}

	QString get_variation_qstring() const
	{
		return QString::fromStdString(this->get_variation());
	}

	IconConfig GetIcon() const;
	CPersistentItem *GetItem(CUnit &item) const;
	void UpdateAttributes();

	bool is_ai_active() const
	{
		return this->ai_active;
	}

	bool is_active() const
	{
		return this->active;
	}

	faction *get_faction() const
	{
		return this->faction;
	}

	const std::unique_ptr<historical_location> &get_location() const
	{
		return this->location;
	}

	const std::unique_ptr<condition> &get_conditions() const
	{
		return this->conditions;
	}

	CDate BirthDate;			/// Date in which the character was born
	CDate StartDate;			/// Date in which the character historically starts being active
	CDate DeathDate;			/// Date in which the character historically died
private:
	civilization *civilization = nullptr;	/// Culture to which the character belongs
	faction *default_faction = nullptr;	//the default faction to which the character belongs
	gender gender;				/// Character's gender
public:
	int Level = 0;				/// Character's level
	int ExperiencePercent = 0;	/// Character's experience, as a percentage of the experience required to level up
	bool Custom = false;		/// Whether this character is a custom hero
	std::string ExtraName;		/// Extra given names of the character (used if necessary to differentiate from existing heroes)
private:
	std::string surname; //the character's surname
	std::string variation; //the identifier of the character variation
public:
	IconConfig Icon;					/// Character's icon
	IconConfig HeroicIcon;				/// Character's heroic icon (level 3 and upper)
private:
	stratagus::unit_type *unit_type = nullptr;
	CUpgrade *trait = nullptr;
public:
	deity *Deity = nullptr;			/// The deity which the character is (if it is a deity)
	character *Father = nullptr;		/// Character's father
	character *Mother = nullptr;		/// Character's mother
	LuaCallback *Conditions = nullptr;
private:
	std::unique_ptr<condition> conditions;
public:
	std::vector<CPersistentItem *> EquippedItems[static_cast<int>(stratagus::item_slot::count)]; //equipped items of the character, per slot
	std::vector<character *> Children;	/// Children of the character
	std::vector<character *> Siblings;	/// Siblings of the character
private:
	site *home_settlement = nullptr; //the home settlement of this character, where they can preferentially be recruited
	bool ai_active = true; //whether the character's AI is active
public:
	std::vector<deity *> Deities;		/// Deities chosen by this character to worship
	std::vector<const CUpgrade *> Abilities;
	std::vector<CUpgrade *> ReadWorks;
	std::vector<CUpgrade *> ConsumedElixirs;
	std::vector<CUpgrade *> AuthoredWorks;	/// Literary works of which this character is the author
	std::vector<CUpgrade *> LiteraryAppearances;	/// Literary works in which this character appears
	std::vector<CPersistentItem *> Items;
	int Attributes[MaxAttributes];
	std::vector<stratagus::unit_type *> ForbiddenUpgrades;	/// which unit types this character is forbidden to upgrade to
private:
	bool active = false; //whether the character is active, i.e. should be applied to the map; used for history
	faction *faction = nullptr; //the character's faction, used for history
	std::unique_ptr<historical_location> location; //the character's location, used for history
public:
	std::vector<std::pair<CDate, stratagus::faction *>> HistoricalFactions;	/// historical locations of the character; the values are: date, faction
	std::vector<std::unique_ptr<historical_location>> HistoricalLocations;	/// historical locations of the character
	std::vector<std::tuple<CDate, CDate, stratagus::faction *, character_title>> HistoricalTitles;	/// historical titles of the character, the first element is the beginning date of the term, the second one the end date, the third the faction it pertains to (if any, if not then it is null), and the fourth is the character title itself (from the character title enums)
	std::vector<std::tuple<int, int, CProvince *, int>> HistoricalProvinceTitles;

	friend ::Spell_Polymorph;
	friend void ::ChangeCustomHeroCivilization(const std::string &hero_full_name, const std::string &civilization_name, const std::string &new_hero_name, const std::string &new_hero_family_name);
	friend int ::CclDefineCharacter(lua_State *l);
	friend int ::CclDefineCustomHero(lua_State *l);
};

}

extern std::map<std::string, stratagus::character *> CustomHeroes;
extern stratagus::character *CurrentCustomHero;
extern bool LoadingPersistentHeroes;

extern int GetAttributeVariableIndex(int attribute);
extern stratagus::character *GetCustomHero(const std::string &hero_ident);
extern void SaveHero(stratagus::character *hero);
extern void SaveHeroes();
extern void SaveCustomHero(const std::string &hero_ident);
extern void DeleteCustomHero(const std::string &hero_ident);
extern void SetCurrentCustomHero(const std::string &hero_ident);
extern std::string GetCurrentCustomHero();
extern void ChangeCustomHeroCivilization(const std::string &hero_name, const std::string &civilization_ident, const std::string &new_hero_name, const std::string &new_hero_family_name);
extern bool IsNameValidForCustomHero(const std::string &hero_name, const std::string &hero_family_name);
extern std::string GetCharacterTitleNameById(const stratagus::character_title title);
extern stratagus::character_title GetCharacterTitleIdByName(const std::string &title);
extern bool IsMinisterialTitle(const stratagus::character_title title);
extern void CharacterCclRegister();
