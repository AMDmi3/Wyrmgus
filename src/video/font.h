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

#include "color.h"
#include "database/data_entry.h"
#include "database/data_type.h"
#include "guichan/font.h"

class CGraphic;

namespace wyrmgus {

class font_color;

class font final : public data_entry, public gcn::Font, public data_type<font>
{
	Q_OBJECT

	Q_PROPERTY(QSize size MEMBER size)

public:
	static constexpr const char *class_identifier = "font";
	static constexpr const char *database_folder = "fonts";

	explicit font(const std::string &ident);
	virtual ~font();

	virtual void process_sml_property(const sml_property &property) override;
	virtual void initialize() override;

	static font *Get(const std::string &identifier)
	{
		return font::get(identifier);
	}

	int Height() const;
	int Width(const std::string &text) const;
	int Width(const int number) const;

	virtual int getHeight() const override { return Height(); }
	virtual int getWidth(const std::string &text) const override { return Width(text); }
	//Wyrmgus start
//	virtual void drawString(gcn::Graphics *graphics, const std::string &text, int x, int y);
	virtual void drawString(gcn::Graphics *graphics, const std::string &text, int x, int y, bool is_normal = true) override;
	//Wyrmgus end

	void Reload();
#if defined(USE_OPENGL) || defined(USE_GLES)
	void FreeOpenGL();
#endif

	CGraphic *GetFontColorGraphic(const wyrmgus::font_color &fontColor) const;

	template<bool CLIP>
	unsigned int DrawChar(CGraphic &g, int utf8, int x, int y, const wyrmgus::font_color &fc) const;

private:
#if defined(USE_OPENGL) || defined(USE_GLES)
	void make_font_color_textures();
#endif
	void MeasureWidths();

private:
	std::filesystem::path filepath;
	QSize size;
	char *CharWidth = nullptr; /// Real font width (starting with ' ')
	CGraphic *G = nullptr; /// Graphic object used to draw
	std::map<const wyrmgus::font_color *, std::unique_ptr<CGraphic>> font_color_graphics;
};

}

///  Return the 'line' line of the string 's'.
extern std::string GetLineFont(unsigned int line, const std::string &s, unsigned int maxlen, const wyrmgus::font *font);

/// Get the hot key from a string
extern int GetHotKey(const std::string &text);

#if defined(USE_OPENGL) || defined(USE_GLES)
/// Free OpenGL fonts
extern void FreeOpenGLFonts();
/// Reload OpenGL fonts
extern void ReloadFonts();
#endif

class CLabel
{
public:
	explicit CLabel(const wyrmgus::font *f, const wyrmgus::font_color *nc, const wyrmgus::font_color *rc);
	explicit CLabel(const wyrmgus::font *f);

	int Height() const { return font->Height(); }

	void SetFont(const wyrmgus::font *f) { font = f; }

	void SetNormalColor(const wyrmgus::font_color *nc);

	/// Draw text/number unclipped
	int Draw(int x, int y, const char *const text) const;
	int Draw(int x, int y, const std::string &text) const;
	int Draw(int x, int y, int number) const;
	/// Draw text/number clipped
	int DrawClip(int x, int y, const char *const text) const;
	//Wyrmgus start
//	int DrawClip(int x, int y, const std::string &text) const;
	int DrawClip(int x, int y, const std::string &text, bool is_normal = true) const;
	//Wyrmgus end
	int DrawClip(int x, int y, int number) const;
	/// Draw reverse text/number unclipped
	int DrawReverse(int x, int y, const char *const text) const;
	int DrawReverse(int x, int y, const std::string &text) const;
	int DrawReverse(int x, int y, int number) const ;
	/// Draw reverse text/number clipped
	int DrawReverseClip(int x, int y, const char *const text) const;
	int DrawReverseClip(int x, int y, const std::string &text) const;
	int DrawReverseClip(int x, int y, int number) const;

	int DrawCentered(int x, int y, const std::string &text) const;
	int DrawReverseCentered(int x, int y, const std::string &text) const;
private:
	template <const bool CLIP>
	int DoDrawText(int x, int y, const char *const text,
				   const size_t len, const wyrmgus::font_color *fc) const;
private:
	const wyrmgus::font_color *normal;
	const wyrmgus::font_color *reverse;
	const wyrmgus::font *font;
};
