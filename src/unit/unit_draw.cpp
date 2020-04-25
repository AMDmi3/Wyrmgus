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
/**@name unit_draw.cpp - The draw routines for units. */
//
//      (c) Copyright 1998-2020 by Lutz Sammer, Jimmy Salmon, Nehal Mistry
//		and Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "actions.h"
#include "action/action_build.h"
#include "action/action_built.h"
#include "action/action_upgradeto.h"
//Wyrmgus start
#include "animation.h"
//Wyrmgus end
#include "construct.h"
#include "cursor.h"
#include "database/defines.h"
#include "editor.h"
#include "font.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "player.h"
#include "script.h"
#include "sound/sound.h"
#include "sound/unitsound.h"
#include "translate.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "unit/unit_type_type.h"
#include "unit/unit_type_variation.h"
#include "video.h"

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

/**
**  Decoration: health, mana.
*/
class Decoration
{
public:
	Decoration() : HotPos(0, 0), Width(0), Height(0), Sprite(nullptr) {}

	std::string File; /// File containing the graphics data
	PixelPos HotPos;  /// drawing position (relative)
	int Width;        /// width of the decoration
	int Height;       /// height of the decoration

	// --- FILLED UP ---
	CGraphic *Sprite;  /// loaded sprite images
};


/**
**  Structure grouping all Sprites for decoration.
*/
class DecoSpriteType
{
public:
	std::vector<std::string> Name;            /// Name of the sprite.
	std::vector<Decoration> SpriteArray;      /// Sprite to display variable.
};

static DecoSpriteType DecoSprite; /// All sprite's infos.

unsigned long ShowOrdersCount;    /// Show orders for some time

unsigned long ShowNameDelay;                 /// Delay to show unit's name
unsigned long ShowNameTime;                  /// Show unit's name for some time

// FIXME: not all variables of this file are here
// FIXME: perhaps split this file into two or three parts?

/**
**  Show that units are selected.
**
**  @param color    FIXME
**  @param x1,y1    Coordinates of the top left corner.
**  @param x2,y2    Coordinates of the bottom right corner.
*/
void (*DrawSelection)(IntColor color, int x1, int y1, int x2, int y2) = DrawSelectionNone;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

// FIXME: clean split screen support
// FIXME: integrate this with global versions of these functions in map.c

const CViewport *CurrentViewport;  /// FIXME: quick hack for split screen

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

/**
**  Show selection marker around a unit.
**
**  @param unit  Pointer to unit.
*/
void DrawUnitSelection(const CViewport &vp, const CUnit &unit)
{
	IntColor color;

	//Wyrmgus start
	const CUnitType &type = *unit.Type;
	const PixelPos screenPos = vp.scaled_map_to_screen_pixel_pos(unit.get_scaled_map_pixel_pos_center());
	const int scale_factor = stratagus::defines::get()->get_scale_factor();
	int frame_width = type.Width * scale_factor;
	int frame_height = type.Height * scale_factor;
	int sprite_width = (type.Sprite ? type.Sprite->Width : 0);
	int sprite_height = (type.Sprite ? type.Sprite->Height : 0);
	const CUnitTypeVariation *variation = unit.GetVariation();
	if (variation && variation->FrameWidth && variation->FrameHeight) {
		frame_width = variation->FrameWidth * scale_factor;
		frame_height = variation->FrameHeight * scale_factor;
		sprite_width = (variation->Sprite ? variation->Sprite->Width : 0);
		sprite_height = (variation->Sprite ? variation->Sprite->Height : 0);
	}
	int x = screenPos.x - type.BoxWidth * scale_factor / 2 - (frame_width - sprite_width) / 2;
	int y = screenPos.y - type.BoxHeight * scale_factor / 2 - (frame_height - sprite_height) / 2;
	
	// show player color circle below unit if that is activated
	if (Preference.PlayerColorCircle && unit.Player->Index != PlayerNumNeutral && unit.CurrentAction() != UnitAction::Die) {
		DrawSelectionCircleWithTrans(unit.Player->Color, x + type.BoxOffsetX * scale_factor + 1, y + type.BoxOffsetY * scale_factor + 1, x + type.BoxWidth * scale_factor + type.BoxOffsetX * scale_factor - 1, y + type.BoxHeight * scale_factor + type.BoxOffsetY * scale_factor - 1);
//		DrawSelectionRectangle(unit.Player->Color, x + type.BoxOffsetX, y + type.BoxOffsetY, x + type.BoxWidth + type.BoxOffsetX + 1, y + type.BoxHeight + type.BoxOffsetY + 1);
	}
	//Wyrmgus end
	
	// FIXME: make these colors customizable with scripts.

	if (Editor.Running && UnitUnderCursor == &unit && Editor.State == EditorSelecting) {
		color = ColorWhite;
	} else if (unit.Selected || unit.TeamSelected || (unit.Blink & 1)) {
		if (unit.Player->Index == PlayerNumNeutral) {
			color = ColorYellow;
		} else if ((unit.Selected || (unit.Blink & 1))
				   && (unit.Player == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->IsTeamed(unit))) {
			color = ColorGreen;
		} else if (CPlayer::GetThisPlayer()->IsEnemy(unit)) {
			color = ColorRed;
		} else {
			color = unit.Player->Color;

			for (int i = 0; i < PlayerMax; ++i) {
				if (unit.TeamSelected & (1 << i)) {
					color = CPlayer::Players[i]->Color;
				}
			}
		}
	} else if (CursorBuilding && unit.Type->BoolFlag[BUILDING_INDEX].value
			   && unit.CurrentAction() != UnitAction::Die
			   && (unit.Player == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->IsTeamed(unit))) {
		// If building mark all own buildings
		color = ColorGray;
	} else {
		return;
	}

	//Wyrmgus start
	/*
//	const CUnitType &type = *unit.Type;
//	const PixelPos screenPos = vp.scaled_map_to_screen_pixel_pos(unit.get_scaled_map_pixel_pos_center());
//	const int x = screenPos.x - type.BoxWidth / 2 - (type.Width - (type.Sprite ? type.Sprite->Width : 0)) / 2;
//	const int y = screenPos.y - type.BoxHeight / 2 - (type.Height - (type.Sprite ? type.Sprite->Height : 0)) / 2;
	*/
	//Wyrmgus end

	//Wyrmgus start
	int box_width = type.BoxWidth;
	int box_height = type.BoxHeight;
	if ((unit.MapLayer->Field(unit.tilePos)->Flags & MapFieldBridge) && !unit.Type->BoolFlag[BRIDGE_INDEX].value && unit.Type->UnitType == UnitTypeType::Land && !unit.Moving) { //if is on a raft, use the raft's box size instead
		std::vector<CUnit *> table;
		Select(unit.tilePos, unit.tilePos, table, unit.MapLayer->ID);
		for (size_t i = 0; i != table.size(); ++i) {
			if (!table[i]->Removed && table[i]->Type->BoolFlag[BRIDGE_INDEX].value && table[i]->CanMove() && table[i]->Type->BoxWidth > box_width && table[i]->Type->BoxHeight > box_height) {
				box_width = table[i]->Type->BoxWidth;
				box_height = table[i]->Type->BoxHeight;
			}
		}
	}

	box_width *= scale_factor;
	box_height *= scale_factor;
	x = screenPos.x - box_width / 2 - (frame_width - sprite_width) / 2;
	y = screenPos.y - box_height / 2 - (frame_height - sprite_height) / 2;
	
//	DrawSelection(color, x + type.BoxOffsetX * scale_factor, y + type.BoxOffsetY * scale_factor, x + type.BoxWidth * scale_factor + type.BoxOffsetX * scale_factor, y + type.BoxHeight * scale_factor + type.BoxOffsetY * scale_factor);
	DrawSelection(color, x + type.BoxOffsetX * scale_factor, y + type.BoxOffsetY * scale_factor, x + box_width + type.BoxOffsetX * scale_factor, y + box_height + type.BoxOffsetY * scale_factor);
	//Wyrmgus end
}

/**
**  Don't show selected units.
**
**  @param color  Color to draw, nothing in this case.
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionNone(IntColor, int, int, int, int)
{
}

/**
**  Show selected units with circle.
**
**  @param color  Color to draw circle
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionCircle(IntColor color, int x1, int y1, int x2, int y2)
{
	Video.DrawCircleClip(color, (x1 + x2) / 2, (y1 + y2) / 2,
						 std::min((x2 - x1) / 2, (y2 - y1) / 2) + 2);
}

/**
**  Show selected units with circle.
**
**  @param color  Color to draw and fill circle.
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionCircleWithTrans(IntColor color, int x1, int y1, int x2, int y2)
{
	Video.FillTransCircleClip(color, (x1 + x2) / 2, (y1 + y2) / 2,
							  std::min((x2 - x1) / 2, (y2 - y1) / 2), 95);
	//Wyrmgus start
	/*
	Video.DrawCircleClip(color, (x1 + x2) / 2, (y1 + y2) / 2,
						 std::min((x2 - x1) / 2, (y2 - y1) / 2));
	*/
	//Wyrmgus end
}

/**
**  Draw selected rectangle around the unit.
**
**  @param color  Color to draw rectangle.
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionRectangle(IntColor color, int x1, int y1, int x2, int y2)
{
	Video.DrawRectangleClip(color, x1, y1, x2 - x1, y2 - y1);
}

/**
**  Draw selected rectangle around the unit.
**
**  @param color  Color to draw and fill rectangle.
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionRectangleWithTrans(IntColor color, int x1, int y1, int x2, int y2)
{
	Video.DrawRectangleClip(color, x1, y1, x2 - x1, y2 - y1);
	Video.FillTransRectangleClip(color, x1 + 1, y1 + 1,
								 x2 - x1 - 2, y2 - y1 - 2, 75);
}

/**
**  Draw selected corners around the unit.
**
**  @param color  Color to draw corners.
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionCorners(IntColor color, int x1, int y1, int x2, int y2)
{
	const int CORNER_PIXELS = 6;

	Video.DrawVLineClip(color, x1, y1, CORNER_PIXELS);
	Video.DrawHLineClip(color, x1 + 1, y1, CORNER_PIXELS - 1);

	Video.DrawVLineClip(color, x2, y1, CORNER_PIXELS);
	Video.DrawHLineClip(color, x2 - CORNER_PIXELS + 1, y1, CORNER_PIXELS - 1);

	//Wyrmgus start
//	Video.DrawVLineClip(color, x1, y2 - CORNER_PIXELS + 1, CORNER_PIXELS);
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		Video.DrawVLineClip(color, x1, y2 - CORNER_PIXELS, CORNER_PIXELS);
	} else
#endif
	{
		Video.DrawVLineClip(color, x1, y2 - CORNER_PIXELS + 1, CORNER_PIXELS);
	}
	//Wyrmgus end
	Video.DrawHLineClip(color, x1, y2, CORNER_PIXELS - 1);

	//Wyrmgus start
//	Video.DrawVLineClip(color, x2, y2 - CORNER_PIXELS + 1, CORNER_PIXELS);
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		Video.DrawVLineClip(color, x2, y2 - CORNER_PIXELS, CORNER_PIXELS);
	} else
#endif
	{
		Video.DrawVLineClip(color, x2, y2 - CORNER_PIXELS + 1, CORNER_PIXELS);
	}
	//Wyrmgus end
	Video.DrawHLineClip(color, x2 - CORNER_PIXELS + 1, y2, CORNER_PIXELS - 1);
}


/**
**  Return the index of the sprite named SpriteName.
**
**  @param SpriteName    Name of the sprite.
**
**  @return              Index of the sprite. -1 if not found.
*/
int GetSpriteIndex(const char *SpriteName)
{
	Assert(SpriteName);
	for (unsigned int i = 0; i < DecoSprite.Name.size(); ++i) {
		if (!strcmp(SpriteName, DecoSprite.Name[i].c_str())) {
			return i;
		}
	}
	return -1;
}

/**
**  Define the sprite to show variables.
**
**  @param l    Lua_state
*/
static int CclDefineSprites(lua_State *l)
{
	const int args = lua_gettop(l);

	for (int i = 0; i < args; ++i) {
		Decoration deco;

		lua_pushnil(l);
		const char *name = nullptr;// name of the current sprite.
		while (lua_next(l, i + 1)) {
			const char *key = LuaToString(l, -2); // key name
			if (!strcmp(key, "Name")) {
				name = LuaToString(l, -1);
			} else if (!strcmp(key, "File")) {
				deco.File = LuaToString(l, -1);
			} else if (!strcmp(key, "Offset")) {
				CclGetPos(l, &deco.HotPos.x, &deco.HotPos.y);
			} else if (!strcmp(key, "Size")) {
				CclGetPos(l, &deco.Width, &deco.Height);
			} else { // Error.
				LuaError(l, "incorrect field '%s' for the DefineSprite." _C_ key);
			}
			lua_pop(l, 1); // pop the value;
		}
		if (name == nullptr) {
			LuaError(l, "CclDefineSprites requires the Name flag for sprite.");
		}
		int index = GetSpriteIndex(name); // Index of the Sprite.
		if (index == -1) { // new sprite.
			index = DecoSprite.SpriteArray.size();
			DecoSprite.Name.push_back(name);
			DecoSprite.SpriteArray.push_back(deco);
		} else {
			DecoSprite.SpriteArray[index].File.clear();
			DecoSprite.SpriteArray[index] = deco;
		}
		// Now verify validity.
		if (DecoSprite.SpriteArray[index].File.empty()) {
			LuaError(l, "CclDefineSprites requires the File flag for sprite.");
		}
		// FIXME check if file is valid with good size ?
	}
	return 0;
}

/**
**  Register CCL features for decorations.
*/
void DecorationCclRegister()
{
	DecoSprite.Name.clear();
	DecoSprite.SpriteArray.clear();

	lua_register(Lua, "DefineSprites", CclDefineSprites);
}

/**
**  Return the amount of decorations.
*/
int GetDecorationsCount()
{
	return DecoSprite.SpriteArray.size();
}

/**
**  Load decoration.
*/
void LoadDecorations()
{
	ShowLoadProgress("%s", _("Loading Decorations"));
		
	std::vector<Decoration>::iterator i;
	for (i = DecoSprite.SpriteArray.begin(); i != DecoSprite.SpriteArray.end(); ++i) {
		if ((*i).Sprite) {
			continue;
		}
		UpdateLoadProgress();
		(*i).Sprite = CGraphic::New((*i).File, (*i).Width, (*i).Height);
		(*i).Sprite->Load(false, stratagus::defines::get()->get_scale_factor());
	}
}

/**
**  Clean decorations.
*/
void CleanDecorations()
{
	for (unsigned int i = 0; i < DecoSprite.SpriteArray.size(); ++i) {
		CGraphic::Free(DecoSprite.SpriteArray[i].Sprite);
	}

	DecoSprite.Name.clear();
	DecoSprite.SpriteArray.clear();
}

/**
**  Draw bar for variables.
**
**  @param x       X screen pixel position
**  @param y       Y screen pixel position
**  @param unit    Unit pointer
**  @todo fix color configuration.
*/
void CDecoVarBar::Draw(int x, int y,
					   const CUnitType &type, const stratagus::unit_variable &var) const
{
	Assert(var.Max);

	int height = this->Height;
	if (height == 0) { // Default value
		height = type.BoxHeight; // Better size ? {,Box, Tile}
	}
	int width = this->Width;
	if (width == 0) { // Default value
		width = type.BoxWidth; // Better size ? {,Box, Tile}
	}
	int h;
	int w;
	if (this->IsVertical)  { // Vertical
		w = width;
		h = var.Value * height / var.Max;
	} else {
		w = var.Value * width / var.Max;
		h = height;
	}
	if (this->IsCenteredInX) {
		x -= w / 2;
	}
	if (this->IsCenteredInY) {
		y -= h / 2;
	}

	char b = this->BorderSize; // BorderSize.
	// Could depend of (value / max)
	int f = var.Value * 100 / var.Max;
	IntColor bcolor = ColorBlack; // Deco->Data.Bar.BColor  // Border color.
	IntColor color = f > 50 ? (f > 75 ? ColorGreen : ColorYellow) : (f > 25 ? ColorOrange : ColorRed);// inside color.
	// Deco->Data.Bar.Color
	if (b) {
		if (this->ShowFullBackground) {
			Video.FillRectangleClip(bcolor, x - b, y - b, 2 * b + width, 2 * b + height);
		} else {
			if (this->SEToNW) {
				Video.FillRectangleClip(bcolor, x - b - w + width, y - b - h + height,
										2 * b + w, 2 * b + h);
			} else {
				Video.FillRectangleClip(bcolor, x - b, y - b, 2 * b + w, 2 * b + h);
			}
		}
	}
	if (this->SEToNW) {
		Video.FillRectangleClip(color, x - w + width, y - h + height, w, h);
	} else {
		Video.FillRectangleClip(color, x, y, w, h);
	}
}

/**
**  Print variable values (and max....).
**
**  @param x       X screen pixel position
**  @param y       Y screen pixel position
**  @param unit    Unit pointer
**  @todo fix font/color configuration.
*/
void CDecoVarText::Draw(int x, int y, const CUnitType &/*type*/, const stratagus::unit_variable &var) const
{
	if (this->IsCenteredInX) {
		x -= 2; // GetGameFont()->Width(buf) / 2, with buf = str(Value)
	}
	if (this->IsCenteredInY) {
		y -= this->Font->Height() / 2;
	}
	CLabel(*this->Font).DrawClip(x, y, var.Value);
}

/**
**  Draw a sprite with is like a bar (several stages)
**
**  @param x       X screen pixel position
**  @param y       Y screen pixel position
**  @param unit    Unit pointer
**  @todo fix sprite configuration.
*/
void CDecoVarSpriteBar::Draw(int x, int y, const CUnitType &/*type*/, const stratagus::unit_variable &var) const
{
	Assert(var.Max);
	Assert(this->NSprite != -1);

	Decoration &decosprite = DecoSprite.SpriteArray[(int)this->NSprite];
	CGraphic &sprite = *decosprite.Sprite;
	x += decosprite.HotPos.x; // in addition of OffsetX... Useful ?
	y += decosprite.HotPos.y; // in addition of OffsetY... Useful ?

	//Wyrmgus start
	if (!decosprite.Sprite) {
		fprintf(stderr, "Tried to load non-existent DecoSprite\n");
	}
	//Wyrmgus end
	
	int n = sprite.NumFrames - 1; // frame of the sprite to show.
	n -= (n * var.Value) / var.Max;

	if (this->IsCenteredInX) {
		x -= sprite.Width / 2;
	}
	if (this->IsCenteredInY) {
		y -= sprite.Height / 2;
	}
	sprite.DrawFrameClip(n, x, y);
}

/**
**  Draw a static sprite.
**
**  @param x       X screen pixel position
**  @param y       Y screen pixel position
**  @param unit    Unit pointer
**
**  @todo fix sprite configuration configuration.
*/
void CDecoVarStaticSprite::Draw(int x, int y, const CUnitType &/*type*/, const stratagus::unit_variable &var) const
{
	Decoration &decosprite = DecoSprite.SpriteArray[(int)this->NSprite];
	CGraphic &sprite = *decosprite.Sprite;

	x += decosprite.HotPos.x; // in addition of OffsetX... Useful ?
	y += decosprite.HotPos.y; // in addition of OffsetY... Useful ?
	if (this->IsCenteredInX) {
		x -= sprite.Width / 2;
	}
	if (this->IsCenteredInY) {
		y -= sprite.Height / 2;
	}
	if (this->FadeValue && var.Value < this->FadeValue) {
		int alpha = var.Value * 255 / this->FadeValue;
		sprite.DrawFrameClipTrans(this->n, x, y, alpha);
	} else {
		sprite.DrawFrameClip(this->n, x, y);
	}
}

/**
**  Draw decoration (invis, for the unit.)
**
**  @param unit       Pointer to the unit.
**  @param type       Type of the unit.
**  @param screenPos  Screen position of the unit.
*/
static void DrawDecoration(const CUnit &unit, const CUnitType &type, const PixelPos &screenPos)
{
	int x = screenPos.x;
	int y = screenPos.y;
#ifdef DEBUG
	// Show the number of references.
	CLabel(GetGameFont()).DrawClip(x + 1, y + 1, unit.Refs);
#endif

	UpdateUnitVariables(const_cast<CUnit &>(unit));
	// Now show decoration for each variable.
	for (std::vector<CDecoVar *>::const_iterator i = UnitTypeVar.DecoVar.begin();
		 i < UnitTypeVar.DecoVar.end(); ++i) {
		const CDecoVar &var = *(*i);
		const int value = unit.Variable[var.Index].Value;
		//Wyrmgus start
//		const int max = unit.Variable[var.Index].Max;
		const int max = unit.GetModifiedVariable(var.Index, VariableMax);
		//Wyrmgus end
		Assert(value <= max);

		if (!((value == 0 && !var.ShowWhenNull) || (value == max && !var.ShowWhenMax)
			  || (var.HideHalf && value != 0 && value != max)
			  || (!var.ShowIfNotEnable && !unit.Variable[var.Index].Enable)
			  || (var.ShowOnlySelected && !unit.Selected)
			  || (unit.Player->Type == PlayerNeutral && var.HideNeutral)
			  //Wyrmgus start
			  || (unit.Player != CPlayer::GetThisPlayer() && !CPlayer::GetThisPlayer()->IsEnemy(unit) && !CPlayer::GetThisPlayer()->IsAllied(unit) && var.HideNeutral)
			  //Wyrmgus end
			  || (CPlayer::GetThisPlayer()->IsEnemy(unit) && !var.ShowOpponent)
			  || (CPlayer::GetThisPlayer()->IsAllied(unit) && (unit.Player != CPlayer::GetThisPlayer()) && var.HideAllied)
			  //Wyrmgus start
			  || (unit.Player == CPlayer::GetThisPlayer() && var.HideSelf)
			  || unit.Type->BoolFlag[DECORATION_INDEX].value // don't show decorations for decoration units
//			  || max == 0)) {
			  || (var.ShowIfCanCastAnySpell && !unit.CanCastAnySpell())
			  || max == 0 || max < var.MinValue)) {
			  //Wyrmgus end
			var.Draw(
				x + var.OffsetX + var.OffsetXPercent * unit.Type->TileSize.x * stratagus::defines::get()->get_scaled_tile_width() / 100,
				y + var.OffsetY + var.OffsetYPercent * unit.Type->TileSize.y * stratagus::defines::get()->get_scaled_tile_height() / 100,
				type, unit.Variable[var.Index]);
		}
	}

	// Draw group number
	if (unit.Selected && unit.GroupId != 0
#ifndef DEBUG
		&& unit.Player == CPlayer::GetThisPlayer()
#endif
	   ) {
		int groupId = 0;

		if (unit.Player->AiEnabled) {
			groupId = unit.GroupId - 1;
		} else {
			for (groupId = 0; !(unit.GroupId & (1 << groupId)); ++groupId) {
			}
		}
		const int width = GetGameFont().Width(groupId);
		x += (unit.Type->TileSize.x * stratagus::defines::get()->get_scaled_tile_width() + unit.Type->BoxWidth * stratagus::defines::get()->get_scale_factor()) / 2 - width;
		const int height = GetGameFont().Height();
		y += (unit.Type->TileSize.y * stratagus::defines::get()->get_scaled_tile_height() + unit.Type->BoxHeight * stratagus::defines::get()->get_scale_factor()) / 2 - height;
		CLabel(GetGameFont()).DrawClip(x, y, groupId);
	}
}

/**
**  Draw unit's shadow.
**
**  @param type   Pointer to the unit type.
**  @param frame  Frame number
**  @param screenPos  Screen position of the unit.
**
**  @todo FIXME: combine new shadow code with old shadow code.
*/
//Wyrmgus start
//void DrawShadow(const CUnitType &type, int frame, const PixelPos &screenPos)
void DrawShadow(const CUnitType &type, CGraphic *sprite, int frame, const PixelPos &screenPos)
//Wyrmgus end
{
	// Draw normal shadow sprite if available
	//Wyrmgus start
//	if (!type.ShadowSprite) {
	if (!sprite) {
	//Wyrmgus end
		return;
	}
	PixelPos pos = screenPos;
	pos.x -= (sprite->Width - type.TileSize.x * stratagus::defines::get()->get_scaled_tile_width()) / 2;
	pos.y -= (sprite->Height - type.TileSize.y * stratagus::defines::get()->get_scaled_tile_height()) / 2;
	pos.x += (type.OffsetX + type.ShadowOffsetX) * stratagus::defines::get()->get_scale_factor();
	pos.y += (type.OffsetY + type.ShadowOffsetY) * stratagus::defines::get()->get_scale_factor();

	if (type.Flip) {
		if (frame < 0) {
			//Wyrmgus start
//			type.ShadowSprite->DrawFrameClipX(-frame - 1, pos.x, pos.y);
			sprite->DrawFrameClipX(-frame - 1, pos.x, pos.y);
			//Wyrmgus end
		} else {
			//Wyrmgus start
//			type.ShadowSprite->DrawFrameClip(frame, pos.x, pos.y);
			sprite->DrawFrameClip(frame, pos.x, pos.y);
			//Wyrmgus end
		}
	} else {
		int row = type.NumDirections / 2 + 1;
		if (frame < 0) {
			frame = ((-frame - 1) / row) * type.NumDirections + type.NumDirections - (-frame - 1) % row;
		} else {
			frame = (frame / row) * type.NumDirections + frame % row;
		}
		//Wyrmgus start
//		type.ShadowSprite->DrawFrameClip(frame, pos.x, pos.y);
		sprite->DrawFrameClip(frame, pos.x, pos.y);
		//Wyrmgus end
	}
}

//Wyrmgus start
void DrawPlayerColorOverlay(const CUnitType &type, CPlayerColorGraphic *sprite, const int player, int frame, const PixelPos &screenPos, const stratagus::time_of_day *time_of_day)
{
	if (!sprite) {
		return;
	}
	PixelPos pos = screenPos;
	// FIXME: move this calculation to high level.
	pos.x -= (sprite->Width - type.TileSize.x * stratagus::defines::get()->get_scaled_tile_width()) / 2;
	pos.y -= (sprite->Height - type.TileSize.y * stratagus::defines::get()->get_scaled_tile_height()) / 2;
	pos.x += type.OffsetX * stratagus::defines::get()->get_scale_factor();
	pos.y += type.OffsetY * stratagus::defines::get()->get_scale_factor();

	if (type.Flip) {
		if (frame < 0) {
			if (type.Stats[player].Variables[TRANSPARENCY_INDEX].Value > 0) {
				sprite->DrawPlayerColorFrameClipTransX(player, -frame - 1, pos.x, pos.y, int(256 - 2.56 * type.Stats[player].Variables[TRANSPARENCY_INDEX].Value), time_of_day);
			} else {
				sprite->DrawPlayerColorFrameClipX(player, -frame - 1, pos.x, pos.y, time_of_day);
			}
		} else {
			if (type.Stats[player].Variables[TRANSPARENCY_INDEX].Value > 0) {
				sprite->DrawPlayerColorFrameClipTrans(player, frame, pos.x, pos.y, int(256 - 2.56 * type.Stats[player].Variables[TRANSPARENCY_INDEX].Value), time_of_day);
			} else {
				sprite->DrawPlayerColorFrameClip(player, frame, pos.x, pos.y, time_of_day);
			}
		}
	} else {
		const int row = type.NumDirections / 2 + 1;

		if (frame < 0) {
			frame = ((-frame - 1) / row) * type.NumDirections + type.NumDirections - (-frame - 1) % row;
		} else {
			frame = (frame / row) * type.NumDirections + frame % row;
		}
		if (type.Stats[player].Variables[TRANSPARENCY_INDEX].Value > 0) {
			sprite->DrawPlayerColorFrameClipTrans(player, frame, pos.x, pos.y, int(256 - 2.56 * type.Stats[player].Variables[TRANSPARENCY_INDEX].Value), time_of_day);
		} else {
			sprite->DrawPlayerColorFrameClip(player, frame, pos.x, pos.y, time_of_day);
		}
	}
}

void DrawOverlay(const CUnitType &type, CGraphic *sprite, int player, int frame, const PixelPos &screenPos, const stratagus::time_of_day *time_of_day)
{
	if (!sprite) {
		return;
	}
	PixelPos pos = screenPos;
	// FIXME: move this calculation to high level.
	pos.x -= (sprite->Width - type.TileSize.x * stratagus::defines::get()->get_scaled_tile_width()) / 2;
	pos.y -= (sprite->Height - type.TileSize.y * stratagus::defines::get()->get_scaled_tile_height()) / 2;
	pos.x += type.OffsetX * stratagus::defines::get()->get_scale_factor();
	pos.y += type.OffsetY * stratagus::defines::get()->get_scale_factor();

	if (type.Flip) {
		if (frame < 0) {
			if (type.Stats[player].Variables[TRANSPARENCY_INDEX].Value > 0) {
				sprite->DrawFrameClipTransX(-frame - 1, pos.x, pos.y, int(256 - 2.56 * type.Stats[player].Variables[TRANSPARENCY_INDEX].Value), time_of_day);
			} else {
				sprite->DrawFrameClipX(-frame - 1, pos.x, pos.y, time_of_day);
			}
		} else {
			if (type.Stats[player].Variables[TRANSPARENCY_INDEX].Value > 0) {
				sprite->DrawFrameClipTrans(frame, pos.x, pos.y, int(256 - 2.56 * type.Stats[player].Variables[TRANSPARENCY_INDEX].Value), time_of_day);
			} else {
				sprite->DrawFrameClip(frame, pos.x, pos.y, time_of_day);
			}
		}
	} else {
		const int row = type.NumDirections / 2 + 1;

		if (frame < 0) {
			frame = ((-frame - 1) / row) * type.NumDirections + type.NumDirections - (-frame - 1) % row;
		} else {
			frame = (frame / row) * type.NumDirections + frame % row;
		}
		if (type.Stats[player].Variables[TRANSPARENCY_INDEX].Value > 0) {
			sprite->DrawFrameClipTrans(frame, pos.x, pos.y, int(256 - 2.56 * type.Stats[player].Variables[TRANSPARENCY_INDEX].Value), time_of_day);
		} else {
			sprite->DrawFrameClip(frame, pos.x, pos.y, time_of_day);
		}
	}
}
//Wyrmgus end

/**
**  Show the current order of a unit.
**
**  @param unit  Pointer to the unit.
*/
void ShowOrder(const CUnit &unit)
{
	if (unit.Destroyed || unit.Removed) {
		return;
	}
#ifndef DEBUG
	if (!CPlayer::GetThisPlayer()->IsAllied(unit) && unit.Player != CPlayer::GetThisPlayer()) {
		return;
	}
#endif
	// Get current position
	const PixelPos mapPos = unit.get_scaled_map_pixel_pos_center();
	PixelPos screenStartPos = CurrentViewport->scaled_map_to_screen_pixel_pos(mapPos);
	const bool flushed = unit.Orders[0]->Finished;

	COrderPtr order;
	// If the current order is cancelled show the next one
	if (unit.Orders.size() > 1 && flushed) {
		order = unit.Orders[1];
	} else {
		order = unit.Orders[0];
	}
	PixelPos screenPos = order->Show(*CurrentViewport, screenStartPos);
	// Show the rest of the orders
	for (size_t i = 1 + (flushed ? 1 : 0); i < unit.Orders.size(); ++i) {
		screenPos = unit.Orders[i]->Show(*CurrentViewport, screenPos);
	}

	// Show order for new trained units
	if (unit.NewOrder) {
		unit.NewOrder->Show(*CurrentViewport, screenStartPos);
	}
	
	//Wyrmgus start
	//if unit has rally point, show it
	if (unit.RallyPointPos.x != -1 && unit.RallyPointPos.y != -1 && unit.RallyPointMapLayer && unit.RallyPointMapLayer == UI.CurrentMapLayer) {
		Video.FillCircleClip(ColorGreen, CurrentViewport->TilePosToScreen_Center(unit.RallyPointPos), 3);
	}
	//Wyrmgus end
}

/**
**  Draw additional informations of a unit.
**
**  @param unit  Unit pointer of drawn unit.
**  @param type  Unit-type pointer.
**  @param screenPos  screen pixel (top left) position of unit.
**
**  @todo FIXME: The different styles should become a function call.
*/
static void DrawInformations(const CUnit &unit, const CUnitType &type, const PixelPos &screenPos)
{
#if 0 && DEBUG // This is for showing vis counts and refs.
	char buf[10];
	sprintf(buf, "%d%c%c%d", unit.VisCount[ThisPlayer->Index],
			unit.Seen.ByPlayer & (1 << ThisPlayer->Index) ? 'Y' : 'N',
			unit.Seen.Destroyed & (1 << ThisPlayer->Index) ? 'Y' : 'N',
			unit.Refs);
	CLabel(GetSmallFont()).Draw(screenPos.x + 10, screenPos.y + 10, buf);
#endif

	const CUnitStats &stats = *unit.Stats;

	// For debug draw sight, react and attack range!
	if (IsOnlySelected(unit)) {
		const PixelPos center(screenPos + type.get_scaled_half_tile_pixel_size());

		if (Preference.ShowSightRange) {
			//Wyrmgus start
//			const int value = stats.Variables[SIGHTRANGE_INDEX].Max;
			const int value = unit.CurrentSightRange;
			//Wyrmgus end
			const int radius = value * stratagus::defines::get()->get_scaled_tile_width() + (type.TileSize.x - 1) * stratagus::defines::get()->get_scaled_tile_width() / 2;

			if (value) {
				// Radius -1 so you can see all ranges
				Video.DrawCircleClip(ColorGreen, center.x, center.y, radius - 1);
			}
		}
		//Wyrmgus start
//		if (type.CanAttack) {
		if (unit.CanAttack(true)) {
		//Wyrmgus end
			if (Preference.ShowReactionRange) {
				//Wyrmgus start
//				const int value = (unit.Player->Type == PlayerPerson) ? type.ReactRangePerson : type.ReactRangeComputer;
				const int value = unit.GetReactionRange();
				//Wyrmgus end
				const int radius = value * stratagus::defines::get()->get_scaled_tile_width() + (type.TileSize.x - 1) * stratagus::defines::get()->get_scaled_tile_width() / 2;

				if (value) {
					Video.DrawCircleClip(ColorBlue, center.x, center.y, radius);
				}
			}
			if (Preference.ShowAttackRange) {
				//Wyrmgus start
//				const int value = stats.Variables[ATTACKRANGE_INDEX].Max;
				const int value = unit.GetModifiedVariable(ATTACKRANGE_INDEX);
				
				//Wyrmgus end
				const int radius = value * stratagus::defines::get()->get_scaled_tile_width() + (type.TileSize.x - 1) * stratagus::defines::get()->get_scaled_tile_width() / 2;

				if (value) {
					// Radius +1 so you can see all ranges
					Video.DrawCircleClip(ColorGreen, center.x, center.y, radius - 1);
				}
			}
		}
		
		//Wyrmgus start
		if (unit.IsAlive() && unit.CurrentAction() != UnitAction::Built) {
			//show aura range if the unit has an aura
			if (unit.Variable[LEADERSHIPAURA_INDEX].Value > 0 || unit.Variable[REGENERATIONAURA_INDEX].Value > 0 || unit.Variable[HYDRATINGAURA_INDEX].Value > 0) {
				const int value = AuraRange - (unit.Type->TileSize.x - 1);
				const int radius = value * stratagus::defines::get()->get_scaled_tile_width() + (type.TileSize.x - 1) * stratagus::defines::get()->get_scaled_tile_width() / 2;

				if (value) {
					Video.DrawCircleClip(ColorBlue, center.x, center.y, radius);
				}
			}
		}
		//Wyrmgus end
	}

	// FIXME: johns: ugly check here, should be removed!
	if (unit.CurrentAction() != UnitAction::Die && (unit.IsVisible(*CPlayer::GetThisPlayer()) || ReplayRevealMap)) {
		DrawDecoration(unit, type, screenPos);
	}
}

/**
**  Draw construction shadow.
**
**  @param unit    Unit pointer.
**  @param cframe  Construction frame
**  @param frame   Frame number to draw.
**  @param screenPos  screen (top left) position of the unit.
*/
//Wyrmgus start
//static void DrawConstructionShadow(const CUnitType &type, const CConstructionFrame *cframe,
static void DrawConstructionShadow(const CUnit &unit, const CUnitType &type, const CConstructionFrame *cframe,
//Wyrmgus end
								   int frame, const PixelPos &screenPos)
{
	PixelPos pos = screenPos;
	const int scale_factor = stratagus::defines::get()->get_scale_factor();
	const CUnitTypeVariation *variation = unit.GetVariation();
	if (cframe->File == ConstructionFileConstruction) {
		if (variation && variation->Construction) {
			if (variation->Construction->ShadowSprite) {
				pos.x -= (variation->Construction->ShadowSprite->Width - type.TileSize.x * stratagus::defines::get()->get_scaled_tile_width()) / 2;
				pos.x += type.OffsetX * scale_factor;
				pos.y -= (variation->Construction->ShadowSprite->Height - type.TileSize.y * stratagus::defines::get()->get_scaled_tile_height()) / 2;
				pos.y += type.OffsetY * scale_factor;
				if (frame < 0) {
					variation->Construction->ShadowSprite->DrawFrameClipX(-frame - 1, pos.x, pos.y);
				} else {
					variation->Construction->ShadowSprite->DrawFrameClip(frame, pos.x, pos.y);
				}
			}
		} else {
			if (type.Construction->ShadowSprite) {
				pos.x -= (type.Construction->ShadowSprite->Width - type.TileSize.x * stratagus::defines::get()->get_scaled_tile_width()) / 2;
				pos.x += type.OffsetX * scale_factor;
				pos.y -= (type.Construction->ShadowSprite->Height - type.TileSize.y * stratagus::defines::get()->get_scaled_tile_height()) / 2;
				pos.y += type.OffsetY * scale_factor;
				if (frame < 0) {
					type.Construction->ShadowSprite->DrawFrameClipX(-frame - 1, pos.x, pos.y);
				} else {
					type.Construction->ShadowSprite->DrawFrameClip(frame, pos.x, pos.y);
				}
			}
		}
	} else {
		if (variation && variation->ShadowSprite) {
			pos.x -= (variation->ShadowSprite->Width - type.TileSize.x * stratagus::defines::get()->get_scaled_tile_width()) / 2;
			pos.x += (type.ShadowOffsetX + type.OffsetX) * scale_factor;
			pos.y -= (variation->ShadowSprite->Height - type.TileSize.y * stratagus::defines::get()->get_scaled_tile_height()) / 2;
			pos.y += (type.ShadowOffsetY + type.OffsetY) * scale_factor;
			if (frame < 0) {
				variation->ShadowSprite->DrawFrameClipX(-frame - 1, pos.x, pos.y);
			} else {
				variation->ShadowSprite->DrawFrameClip(frame, pos.x, pos.y);
			}
		} else if (type.ShadowSprite) {
			pos.x -= (type.ShadowSprite->Width - type.TileSize.x * stratagus::defines::get()->get_scaled_tile_width()) / 2;
			pos.x += (type.ShadowOffsetX + type.OffsetX) * scale_factor;
			pos.y -= (type.ShadowSprite->Height - type.TileSize.y * stratagus::defines::get()->get_scaled_tile_height()) / 2;
			pos.y += (type.ShadowOffsetY + type.OffsetY) * scale_factor;
			if (frame < 0) {
				type.ShadowSprite->DrawFrameClipX(-frame - 1, pos.x, pos.y);
			} else {
				type.ShadowSprite->DrawFrameClip(frame, pos.x, pos.y);
			}
		}
	}
}

/**
**  Draw construction.
**
**  @param unit    Unit pointer.
**  @param cframe  Construction frame to draw.
**  @param type    Unit type.
**  @param frame   Frame number.
**  @param screenPos  screen (top left) position of the unit.
*/
static void DrawConstruction(const int player, const CConstructionFrame *cframe,
							//Wyrmgus start
//							 const CUnitType &type, int frame, const PixelPos &screenPos)
							 const CUnit &unit, const CUnitType &type, int frame, const PixelPos &screenPos, const stratagus::time_of_day *time_of_day)
							//Wyrmgus end
{
	PixelPos pos = screenPos;
	const int scale_factor = stratagus::defines::get()->get_scale_factor();
	if (cframe->File == ConstructionFileConstruction) {
		const CUnitTypeVariation *variation = unit.GetVariation();
		if (variation && variation->Construction) {
			const CConstruction &construction = *variation->Construction;
			pos.x -= construction.Width * scale_factor / 2;
			pos.y -= construction.Height * scale_factor / 2;
			if (frame < 0) {
				construction.Sprite->DrawPlayerColorFrameClipX(player, -frame - 1, pos.x, pos.y, time_of_day);
			} else {
				construction.Sprite->DrawPlayerColorFrameClip(player, frame, pos.x, pos.y, time_of_day);
			}
		} else {
			const CConstruction &construction = *type.Construction;
			pos.x -= construction.Width * scale_factor / 2;
			pos.y -= construction.Height * scale_factor / 2;
			if (frame < 0) {
				construction.Sprite->DrawPlayerColorFrameClipX(player, -frame - 1, pos.x, pos.y, time_of_day);
			} else {
				construction.Sprite->DrawPlayerColorFrameClip(player, frame, pos.x, pos.y, time_of_day);
			}
		}
		//Wyrmgus end
	} else {
		//Wyrmgus start
//		pos.x += type.OffsetX - type.Width / 2;
//		pos.y += type.OffsetY - type.Height / 2;
		int frame_width = type.Width;
		int frame_height = type.Height;
		const CUnitTypeVariation *variation = unit.GetVariation();
		if (variation && variation->FrameWidth && variation->FrameHeight) {
			frame_width = variation->FrameWidth;
			frame_height = variation->FrameHeight;
		}
		frame_width *= scale_factor;
		frame_height *= scale_factor;
		pos.x += type.OffsetX * scale_factor - frame_width / 2;
		pos.y += type.OffsetY * scale_factor - frame_height / 2;
		//Wyrmgus end
		if (frame < 0) {
			frame = -frame - 1;
		}
		//Wyrmgus start
//		type.Sprite->DrawPlayerColorFrameClip(player, frame, pos.x, pos.y);
		if (variation && variation->Sprite) {
			variation->Sprite->DrawPlayerColorFrameClip(player, frame, pos.x, pos.y, time_of_day);
		} else {
			type.Sprite->DrawPlayerColorFrameClip(player, frame, pos.x, pos.y, time_of_day);
		}
		//Wyrmgus end
	}
}

/**
**  Units on map:
*/

/**
**  Draw unit on map.
*/
void CUnit::Draw(const CViewport &vp) const
{
	int frame;
	int state;
	int under_construction;
	const CConstructionFrame *cframe;
	const CUnitType *type;

	if (this->Destroyed || this->Container || this->Type->BoolFlag[REVEALER_INDEX].value) { // Revealers are not drawn
		return;
	}

	bool IsVisible = this->IsVisible(*CPlayer::GetThisPlayer());

	// Those should have been filtered. Check doesn't make sense with ReplayRevealMap
	Assert(ReplayRevealMap || this->Type->BoolFlag[VISIBLEUNDERFOG_INDEX].value || IsVisible);

	//Wyrmgus start
//	int player = this->RescuedFrom ? this->RescuedFrom->Index : this->Player->Index;
	int player = this->GetDisplayPlayer();
	//Wyrmgus end
	const UnitAction action = this->CurrentAction();
	PixelPos screenPos;
	if (ReplayRevealMap || IsVisible) {
		screenPos = vp.scaled_map_to_screen_pixel_pos(this->get_scaled_map_pixel_pos_top_left());
		type = this->Type;
		frame = this->Frame;
		state = (action == UnitAction::Built) | ((action == UnitAction::UpgradeTo) << 1);
		under_construction = this->UnderConstruction;
		// Reset Type to the type being upgraded to
		if (action == UnitAction::UpgradeTo) {
			const COrder_UpgradeTo &order = *static_cast<COrder_UpgradeTo *>(this->CurrentOrder());

			type = &order.GetUnitType();
		}

		if (this->CurrentAction() == UnitAction::Built) {
			COrder_Built &order = *static_cast<COrder_Built *>(this->CurrentOrder());

			cframe = &order.GetFrame();
		} else {
			cframe = nullptr;
		}
	} else {
		screenPos = vp.TilePosToScreen_TopLeft(this->Seen.tilePos);

		screenPos.x += this->Seen.pixel_offset.x() * stratagus::defines::get()->get_scale_factor();
		screenPos.y += this->Seen.pixel_offset.y() * stratagus::defines::get()->get_scale_factor();
		frame = this->Seen.Frame;
		type = this->Seen.Type;
		under_construction = this->Seen.UnderConstruction;
		state = this->Seen.State;
		cframe = this->Seen.CFrame;
	}

#ifdef DYNAMIC_LOAD
	if (!type->Sprite) {
		LoadUnitTypeSprite(type);
	}
#endif

	if (!IsVisible && frame == UnitNotSeen) {
		DebugPrint("FIXME: Something is wrong, unit %d not seen but drawn time %lu?.\n" _C_
				   UnitNumber(*this) _C_ GameCycle);
		return;
	}

	//Wyrmgus start
	//
	// Show that the unit is selected
	//
	// draw it under everything else
	DrawUnitSelection(vp, *this);
	//Wyrmgus end

	const CUnitTypeVariation *variation = this->GetVariation();

	if (state == 1 && under_construction && cframe) {
		//Wyrmgus start
//		DrawConstructionShadow(*type, cframe, frame, screenPos);
		DrawConstructionShadow(*this, *type, cframe, frame, screenPos);
		//Wyrmgus end
	} else {
		//Wyrmgus start
//		if (action != UnitAction::Die) {
		//Wyrmgus end
			//Wyrmgus start
//			DrawShadow(*type, frame, screenPos);
			if (variation && variation->ShadowSprite) {
				DrawShadow(*type, variation->ShadowSprite, frame, screenPos);
			} else if (type->ShadowSprite) {
				DrawShadow(*type, type->ShadowSprite, frame, screenPos);
			}
			//Wyrmgus end
		//Wyrmgus start
//		}
		//Wyrmgus end
	}

	//Wyrmgus start
	//
	// Show that the unit is selected
	//
//	DrawUnitSelection(vp, *this);
	//Wyrmgus end

	const stratagus::time_of_day *time_of_day = this->get_center_tile_time_of_day();
	
	//Wyrmgus start
	DrawPlayerColorOverlay(*type, this->GetLayerSprite(MountImageLayer), player, frame, screenPos, time_of_day); // draw the mount just before the body
	
	//draw the backpack before everything but the shadow if facing south (or the still frame, since that also faces south), southeast or southwest
	if (this->Direction == LookingS || frame == type->StillFrame || this->Direction == LookingSE || this->Direction == LookingSW) {
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(BackpackImageLayer), player, frame, screenPos, time_of_day);
	}
	
	//draw the left arm before the body if not facing south (or the still frame, since that also faces south); if the position of the arms in the southeast frame is inverted, don't draw the left arm yet either
	if (
		(this->Direction != LookingS || this->CurrentAction() == UnitAction::Die)
		&& frame != type->StillFrame
		&& !(
			type->BoolFlag[INVERTEDEASTARMS_INDEX].value
			&& (this->Direction == LookingE || this->Direction == LookingW)
			&& this->CurrentAction() != UnitAction::Die
		)
		&& !(
			type->BoolFlag[INVERTEDSOUTHEASTARMS_INDEX].value
			&& (this->Direction == LookingSE || this->Direction == LookingSW || (this->Direction == LookingS && this->CurrentAction() == UnitAction::Die))
		)
	) {
		//draw the shield before the left arm if not facing south
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(ShieldImageLayer), player, frame, screenPos, time_of_day);

		DrawPlayerColorOverlay(*type, this->GetLayerSprite(LeftArmImageLayer), player, frame, screenPos, time_of_day);
	}
	
	//draw the right arm before the body if facing north, or if facing southeast/southwest and the arms are inverted for that direction
	if (
		(this->Direction == LookingN && this->CurrentAction() != UnitAction::Die)
		|| (
			type->BoolFlag[INVERTEDEASTARMS_INDEX].value
			&& (this->Direction == LookingE || this->Direction == LookingW)
			&& this->CurrentAction() != UnitAction::Die
		)
		|| (
			type->BoolFlag[INVERTEDSOUTHEASTARMS_INDEX].value
			&& (this->Direction == LookingSE || this->Direction == LookingSW || (this->Direction == LookingS && this->CurrentAction() == UnitAction::Die))
		)
	) {
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(WeaponImageLayer), player, frame, screenPos, time_of_day);
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(RightArmImageLayer), player, frame, screenPos, time_of_day);
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(RightHandImageLayer), player, frame, screenPos, time_of_day);
	}
	//Wyrmgus end

	//
	// Adjust sprite for Harvesters.
	//
	CPlayerColorGraphic *sprite = type->Sprite;
	if (type->BoolFlag[HARVESTER_INDEX].value && this->CurrentResource) {
		ResourceInfo *resinfo = type->ResInfo[this->CurrentResource];
		if (this->ResourcesHeld) {
			if (resinfo->SpriteWhenLoaded) {
				sprite = resinfo->SpriteWhenLoaded;
			}
		} else {
			if (resinfo->SpriteWhenEmpty) {
				sprite = resinfo->SpriteWhenEmpty;
			}
		}
	}
	//Wyrmgus start
	// Adjust sprite for variations.
	if (variation) {
		if (variation->Sprite) {
			sprite = variation->Sprite;
		}
		if (type->BoolFlag[HARVESTER_INDEX].value && this->CurrentResource) {
			if (this->ResourcesHeld) {
				if (variation->SpriteWhenLoaded[this->CurrentResource]) {
					sprite = variation->SpriteWhenLoaded[this->CurrentResource];
				}
			} else {
				if (variation->SpriteWhenEmpty[this->CurrentResource]) {
					sprite = variation->SpriteWhenEmpty[this->CurrentResource];
				}
			}
		}
	}
	//Wyrmgus end

	//
	// Now draw!
	// Buildings under construction/upgrade/ready.
	//
	if (state == 1) {
		if (under_construction && cframe) {
			const PixelPos pos(screenPos + type->get_scaled_half_tile_pixel_size());
			DrawConstruction(player, cframe, *this, *type, frame, pos, time_of_day);
		} else {
			DrawUnitType(*type, sprite, player, frame, screenPos, time_of_day);
		}
		//
		// Draw the future unit type, if upgrading to it.
		//
	} else {
		DrawUnitType(*type, sprite, player, frame, screenPos, time_of_day);
	}
	
	//Wyrmgus start
	//draw the left arm and right arm clothing after the body, even if the arms were drawn before
	if ((this->Direction != LookingS || this->CurrentAction() == UnitAction::Die) && frame != type->StillFrame) {
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(ClothingLeftArmImageLayer), player, frame, screenPos, time_of_day);
	}
	if (
		(this->Direction == LookingN && this->CurrentAction() != UnitAction::Die)
		|| (
			type->BoolFlag[INVERTEDEASTARMS_INDEX].value
			&& (this->Direction == LookingE || this->Direction == LookingW)
			&& this->CurrentAction() != UnitAction::Die
		)
		|| (
			type->BoolFlag[INVERTEDSOUTHEASTARMS_INDEX].value
			&& (this->Direction == LookingSE || this->Direction == LookingSW || (this->Direction == LookingS && this->CurrentAction() == UnitAction::Die))
		)
	) {
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(ClothingRightArmImageLayer), player, frame, screenPos, time_of_day);
	}

	DrawPlayerColorOverlay(*type, this->GetLayerSprite(PantsImageLayer), player, frame, screenPos, time_of_day);
	DrawPlayerColorOverlay(*type, this->GetLayerSprite(ClothingImageLayer), player, frame, screenPos, time_of_day);
	
	//draw the backpack after the clothing if facing east or west, if isn't dying (dying animations for east and west use northeast frames)
	if ((this->Direction == LookingE || this->Direction == LookingW) && this->CurrentAction() != UnitAction::Die) {
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(BackpackImageLayer), player, frame, screenPos, time_of_day);
	}
	
	DrawPlayerColorOverlay(*type, this->GetLayerSprite(HairImageLayer), player, frame, screenPos, time_of_day);
	DrawPlayerColorOverlay(*type, this->GetLayerSprite(HelmetImageLayer), player, frame, screenPos, time_of_day);
	DrawPlayerColorOverlay(*type, this->GetLayerSprite(BootsImageLayer), player, frame, screenPos, time_of_day);
	
	//draw the left arm just after the body if facing south
	if (
		(this->Direction == LookingS && this->CurrentAction() != UnitAction::Die)
		|| frame == type->StillFrame
		|| (
			type->BoolFlag[INVERTEDEASTARMS_INDEX].value
			&& (this->Direction == LookingE || this->Direction == LookingW)
			&& this->CurrentAction() != UnitAction::Die
		)
		|| (
			type->BoolFlag[INVERTEDSOUTHEASTARMS_INDEX].value
			&& (this->Direction == LookingSE || this->Direction == LookingSW || (this->Direction == LookingS && this->CurrentAction() == UnitAction::Die))
		)
	) {
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(LeftArmImageLayer), player, frame, screenPos, time_of_day);
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(ClothingLeftArmImageLayer), player, frame, screenPos, time_of_day);
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(ShieldImageLayer), player, frame, screenPos, time_of_day);
	}

	//draw the right arm just after the body if not facing north
	if (
		(this->Direction != LookingN || this->CurrentAction() == UnitAction::Die)
		&& !(
			type->BoolFlag[INVERTEDEASTARMS_INDEX].value
			&& (this->Direction == LookingE || this->Direction == LookingW)
			&& this->CurrentAction() != UnitAction::Die
		)
		&& !(
			type->BoolFlag[INVERTEDSOUTHEASTARMS_INDEX].value
			&& (this->Direction == LookingSE || this->Direction == LookingSW || (this->Direction == LookingS && this->CurrentAction() == UnitAction::Die))
		)
	) {
		if ((this->Direction == LookingS || this->Direction == LookingSE || this->Direction == LookingSW) && this->CurrentAction() != UnitAction::Die && this->GetLayerSprite(RightHandImageLayer) != nullptr) { // if the unit has a right hand sprite, draw the weapon after the right arm, but before the hand
			DrawPlayerColorOverlay(*type, this->GetLayerSprite(RightArmImageLayer), player, frame, screenPos, time_of_day);
			DrawPlayerColorOverlay(*type, this->GetLayerSprite(ClothingRightArmImageLayer), player, frame, screenPos, time_of_day);
			DrawPlayerColorOverlay(*type, this->GetLayerSprite(WeaponImageLayer), player, frame, screenPos, time_of_day);
			DrawPlayerColorOverlay(*type, this->GetLayerSprite(RightHandImageLayer), player, frame, screenPos, time_of_day);
		} else {
			DrawPlayerColorOverlay(*type, this->GetLayerSprite(WeaponImageLayer), player, frame, screenPos, time_of_day);
			DrawPlayerColorOverlay(*type, this->GetLayerSprite(RightArmImageLayer), player, frame, screenPos, time_of_day);
			DrawPlayerColorOverlay(*type, this->GetLayerSprite(RightHandImageLayer), player, frame, screenPos, time_of_day);
			DrawPlayerColorOverlay(*type, this->GetLayerSprite(ClothingRightArmImageLayer), player, frame, screenPos, time_of_day);
		}
	}

	//draw the backpack after everything if facing north, northeast or northwest, or if facing east or west and is dying (dying animations for east and west use northeast frames)
	if (
		this->Direction == LookingN
		|| this->Direction == LookingNE
		|| this->Direction == LookingNW
		|| (
			(this->Direction == LookingE || this->Direction == LookingW) && this->CurrentAction() == UnitAction::Die
		)
	) {
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(BackpackImageLayer), player, frame, screenPos, time_of_day);
	}

	if (variation && variation->LightSprite) {
		DrawOverlay(*type, variation->LightSprite, player, frame, screenPos, time_of_day);
	} else if (type->LightSprite) {
		DrawOverlay(*type, type->LightSprite, player, frame, screenPos, time_of_day);
	}
	//Wyrmgus end
	
	// Unit's extras not fully supported.. need to be decorations themselves.
	DrawInformations(*this, *type, screenPos);
}

/**
**  Compare what order 2 units should be drawn on the map
**
**  @param c1  First Unit to compare (*Unit)
**  @param c2  Second Unit to compare (*Unit)
**
*/
static inline bool DrawLevelCompare(const CUnit *c1, const CUnit *c2)
{
	int drawlevel1 = c1->GetDrawLevel();
	int drawlevel2 = c2->GetDrawLevel();

	if (drawlevel1 == drawlevel2) {
		// diffpos compares unit's Y positions (bottom of sprite) on the map
		// and uses X position in case Y positions are equal.
		const int pos1 = (c1->tilePos.y + c1->Type->TileSize.y - 1) * stratagus::defines::get()->get_tile_height() + c1->get_pixel_offset().y();
		const int pos2 = (c2->tilePos.y + c2->Type->TileSize.y - 1) * stratagus::defines::get()->get_tile_height() + c2->get_pixel_offset().y();
		return pos1 == pos2 ?
			   (c1->tilePos.x != c2->tilePos.x ? c1->tilePos.x < c2->tilePos.x : UnitNumber(*c1) < UnitNumber(*c2)) : pos1 < pos2;
	} else {
		return drawlevel1 < drawlevel2;
	}
}

/**
**  Find all units to draw in viewport.
**
**  @param vp     Viewport to be drawn.
**  @param table  Table of units to return in sorted order
**
*/
int FindAndSortUnits(const CViewport &vp, std::vector<CUnit *> &table)
{
	//  Select all units touching the viewpoint.
	const Vec2i offset(1, 1);
	const Vec2i vpSize(vp.MapWidth, vp.MapHeight);
	const Vec2i minPos = vp.MapPos - offset;
	const Vec2i maxPos = vp.MapPos + vpSize + offset;

	//Wyrmgus start
//	Select(minPos, maxPos, table);
	Select(minPos, maxPos, table, UI.CurrentMapLayer->ID);
	//Wyrmgus end

	size_t n = table.size();
	for (size_t i = 0; i < table.size(); ++i) {
		if (!table[i]->IsVisibleInViewport(vp)) {
			table[i--] = table[--n];
			table.pop_back();
		}
	}
	Assert(n == table.size());
	std::sort(table.begin(), table.begin() + n, DrawLevelCompare);
	return n;
}
