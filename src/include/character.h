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
/**@name character.h - The character headerfile. */
//
//      (c) Copyright 2015-2018 by Andrettin
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

#ifndef __CHARACTER_H__
#define __CHARACTER_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <vector>
#include <tuple>

#ifndef __ICONS_H__
#include "icons.h"
#endif

#ifndef __ITEM_H__
#include "item.h"
#endif

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CConfigData;
class CDeity;
class CFaction;
class CFile;
class CMapTemplate;
class CLanguage;
class CPersistentItem;
class CProvince;
class CQuest;
class CUnitType;
class CUnit;
class CUpgrade;
class LuaCallback;

/**
**  Indexes into gender array.
*/
enum Genders {
	NoGender,
	MaleGender,
	FemaleGender,
	AsexualGender, //i.e. slimes reproduce asexually

	MaxGenders
};

enum Attributes {
	StrengthAttribute,
	DexterityAttribute,
	IntelligenceAttribute,
	CharismaAttribute,

	MaxAttributes
};

/**
**  Indexes into character title array.
*/
enum CharacterTitles {
	CharacterTitleHeadOfState, // also used for titulars to aristocratic titles which were formal only; for example: the French duke of Orléans did not rule over Orléans, but here we consider the "head of state" title to also encompass such cases
	CharacterTitleHeadOfGovernment,
	CharacterTitleEducationMinister,
	CharacterTitleFinanceMinister,
	CharacterTitleForeignMinister,
	CharacterTitleIntelligenceMinister,
	CharacterTitleInteriorMinister,
	CharacterTitleJusticeMinister,
	CharacterTitleWarMinister,
	
	CharacterTitleGovernor,
	CharacterTitleMayor,

	MaxCharacterTitles
};

class CCharacter
{
public:
	CCharacter() :
		Civilization(-1), Faction(NULL), Gender(0), Level(0), ExperiencePercent(0),
		ViolentDeath(false), Custom(false),
		Type(NULL), Trait(NULL), Deity(NULL),
		Father(NULL), Mother(NULL),
		Conditions(NULL)
	{
		memset(Attributes, 0, sizeof(Attributes));
		memset(ForbiddenUpgrades, 0, sizeof(ForbiddenUpgrades));
	}
	~CCharacter();
	
	static void GenerateCharacterHistory();		/// Generates history for characters
	static void ResetCharacterHistory();		/// Removes generated history data from characters
	
	void ProcessConfigData(CConfigData *config_data);
	void GenerateHistory();
	void ResetHistory();
	void SaveHistory();
	int GetMartialAttribute() const;
	int GetAttributeModifier(int attribute) const;
	CLanguage *GetLanguage() const;
	bool IsParentOf(std::string child_full_name) const;
	bool IsChildOf(std::string parent_full_name) const;
	bool IsSiblingOf(std::string sibling_full_name) const;
	bool IsItemEquipped(const CPersistentItem *item) const;
	bool IsUsable() const;
	bool CanAppear(bool ignore_neutral = false) const;
	std::string GetFullName() const;
	IconConfig GetIcon() const;
	CPersistentItem *GetItem(CUnit &item) const;
	void UpdateAttributes();
	void SaveHistory(CFile &file);		/// Save generated history data for the character

	CDate Date;					/// Date in which the character historically starts being active
	CDate DeathDate;			/// Date in which the character historically died
	int Civilization;			/// Culture to which the character belongs
	CFaction *Faction;			/// Faction to which the character belongs
	int Gender;					/// Character's gender
	int Level;					/// Character's level
	int ExperiencePercent;		/// Character's experience, as a percentage of the experience required to level up
	bool ViolentDeath;			/// If historical death was violent
	bool Custom;				/// Whether this character is a custom hero
	std::string Ident;			/// Ident of the character
	std::string Name;			/// Given name of the character
	std::string ExtraName;		/// Extra given names of the character (used if necessary to differentiate from existing heroes)
	std::string FamilyName;		/// Name of the character's family
	std::string Description;	/// Description of the character from an in-game universe perspective
	std::string Background;		/// Description of the character from a perspective outside of the game's universe
	std::string Quote;			/// A quote relating to the character
	std::string HairVariation;	/// Name of the character's hair variation
	IconConfig Icon;					/// Character's icon
	IconConfig HeroicIcon;				/// Character's heroic icon (level 3 and upper)
	CUnitType *Type;
	CUpgrade *Trait;
	CDeity *Deity;						/// The deity which the character is (if it is a deity)
	CCharacter *Father;					/// Character's father
	CCharacter *Mother;					/// Character's mother
	LuaCallback *Conditions;
	std::vector<CPersistentItem *> EquippedItems[MaxItemSlots];	/// Equipped items of the character, per slot
	std::vector<CCharacter *> Children;	/// Children of the character
	std::vector<CCharacter *> Siblings;	/// Siblings of the character
	std::vector<CDeity *> Deities;		/// Deities chosen by this character to worship
	std::vector<CDeity *> DeityProfiles;
	std::vector<CUpgrade *> Abilities;
	std::vector<CUpgrade *> ReadWorks;
	std::vector<CUpgrade *> ConsumedElixirs;
	std::vector<CUpgrade *> AuthoredWorks;	/// Literary works of which this character is the author
	std::vector<CUpgrade *> LiteraryAppearances;	/// Literary works in which this character appears
	std::vector<CQuest *> QuestsInProgress;	/// Quests in progress, only for playable, custom characters
	std::vector<CQuest *> QuestsCompleted;	/// Quests completed, only for playable, custom characters
	std::vector<CPersistentItem *> Items;
	int Attributes[MaxAttributes];
	bool ForbiddenUpgrades[UnitTypeMax];	/// which unit types this character is forbidden to upgrade to
	std::vector<std::pair<CDate, CFaction *>> HistoricalFactions;	/// historical locations of the character; the values are: date, faction
	std::vector<std::tuple<CDate, CMapTemplate *, Vec2i>> HistoricalLocations;	/// historical locations of the character; the values are: date, map template, position
	std::vector<std::tuple<CDate, CDate, CFaction *, int>> HistoricalTitles;	/// historical titles of the character, the first element is the beginning date of the term, the second one the end date, the third the faction it pertains to (if any, if not then it is NULL), and the fourth is the character title itself (from the character title enums)
	std::vector<std::tuple<int, int, CProvince *, int>> HistoricalProvinceTitles;
};

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

extern std::map<std::string, CCharacter *> Characters;
extern std::map<std::string, CCharacter *> CustomHeroes;
extern CCharacter *CurrentCustomHero;
extern bool LoadingPersistentHeroes;

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern int GetAttributeVariableIndex(int attribute);
extern void CleanCharacters();
extern CCharacter *GetCharacter(std::string character_full_name);
extern CCharacter *GetCustomHero(std::string hero_full_name);
extern void SaveHero(CCharacter *hero);
extern void SaveHeroes();
extern void HeroAddQuest(std::string hero_full_name, std::string quest_name);
extern void HeroCompleteQuest(std::string hero_full_name, std::string quest_name);
extern void SaveCustomHero(std::string hero_full_name);
extern void DeleteCustomHero(std::string hero_full_name);
extern void SetCurrentCustomHero(std::string hero_full_name);
extern std::string GetCurrentCustomHero();
extern void ChangeCustomHeroCivilization(std::string hero_full_name, std::string civilization_name, std::string new_hero_name, std::string new_hero_family_name);
extern bool IsNameValidForCustomHero(std::string hero_name, std::string hero_family_name);
extern std::string GetGenderNameById(int gender);
extern int GetGenderIdByName(std::string gender);
extern std::string GetCharacterTitleNameById(int title);
extern int GetCharacterTitleIdByName(std::string title);
extern bool IsMinisterialTitle(int title);
extern void CharacterCclRegister();

//@}

#endif // !__CHARACTER_H__
