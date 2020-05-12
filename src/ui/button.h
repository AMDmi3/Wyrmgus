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
//      (c) Copyright 1998-2020 by Lutz Sammer, Jimmy Salmon and Andrettin
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
#include "sound/unitsound.h"
#include "ui/button_cmd.h"
#include "ui/icon.h"

class CButtonLevel;
class CUnit;
struct lua_State;

int CclDefineButton(lua_State *l);

namespace stratagus {

class button;
typedef bool (*button_check_func)(const CUnit &, const button &);

class button : public data_entry, public data_type<button>
{
	Q_OBJECT

	Q_PROPERTY(int pos MEMBER pos READ get_pos)

public:
	static constexpr const char *class_identifier = "button";
	static constexpr const char *database_folder = "buttons";

	static void ProcessConfigData(const CConfigData *config_data);

	button(const std::string &identifier = "") : data_entry(identifier)
	{
	}

	button &operator =(const button &other_button)
	{
		this->pos = other_button.pos;
		this->Level = other_button.Level;
		this->AlwaysShow = other_button.AlwaysShow;
		this->Action = other_button.Action;
		this->Value = other_button.Value;
		this->Payload = other_button.Payload;
		this->ValueStr = other_button.ValueStr;
		this->Allowed = other_button.Allowed;
		this->AllowStr = other_button.AllowStr;
		this->UnitMask = other_button.UnitMask;
		this->Icon = other_button.Icon;
		this->Key = other_button.Key;
		this->Hint = other_button.Hint;
		this->Description = other_button.Description;
		this->CommentSound = other_button.CommentSound;
		this->Popup = other_button.Popup;
		this->Mod = other_button.Mod;

		return *this;
	}

	virtual void initialize() override;

	int get_pos() const
	{
		return this->pos;
	}

	void SetTriggerData() const;
	void CleanTriggerData() const;
	int GetLevelID() const;
	int GetKey() const;
	std::string GetHint() const;

	int pos = 0; //button position in the grid
	CButtonLevel *Level = nullptr;		/// requires button level
	bool AlwaysShow = false;			/// button is always shown but drawn grayscale if not available
	ButtonCmd Action = ButtonCmd::Move;	/// command on button press
	int Value = 0;					/// extra value for command
	void *Payload = nullptr;
	std::string ValueStr;		/// keep original value string

	button_check_func Allowed = nullptr;    /// Check if this button is allowed
	std::string AllowStr;       /// argument for allowed
	std::string UnitMask;       /// for which units is it available
	IconConfig Icon;      		/// icon to display
	int Key = 0;                    /// alternative on keyboard
	std::string Hint;           /// tip texts
	std::string Description;    /// description shown on status bar (optional)
	SoundConfig CommentSound;   /// Sound comment used when you press the button
	std::string Popup;          /// Popup screen used for button
	std::string Mod;			/// Mod to which this button belongs to

	friend int ::CclDefineButton(lua_State *l);
};

}

//
// in botpanel.cpp
//
/// Generate all buttons
extern void InitButtons();
/// Free memory for buttons
extern void CleanButtons();
// Check if the button is allowed for the unit.
extern bool IsButtonAllowed(const CUnit &unit, const stratagus::button &buttonaction);

// Check if the button is usable for the unit.
extern bool IsButtonUsable(const CUnit &unit, const stratagus::button &buttonaction);

// Get the cooldown for the button for the unit.
extern int GetButtonCooldown(const CUnit &unit, const stratagus::button &buttonaction);

// Get the cooldown percent for the button for the unit.
extern int GetButtonCooldownPercent(const CUnit &unit, const stratagus::button &buttonaction);

extern std::string GetButtonActionNameById(const ButtonCmd button_action);
extern ButtonCmd GetButtonActionIdByName(const std::string &button_action);
extern bool IsNeutralUsableButtonAction(const ButtonCmd button_action);
