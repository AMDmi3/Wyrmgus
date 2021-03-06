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
/**@name linedraw.cpp - The general linedraw functions. */
//
//      (c) Copyright 2000-2011 by Lutz Sammer, Stephan Rasenberg,
//                                 Jimmy Salmon, Nehal Mistry and Pali Rohár
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
#include "video/video.h"

#include "intern_video.h"

#ifdef USE_OPENGL
#ifdef __APPLE__
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <SDL_opengl.h>
#endif

/**
** Bitmask, denoting a position left/right/above/below clip rectangle
** (mainly used by VideoDrawLineClip)
*/
static constexpr int ClipCodeInside = 0; /// Clipping inside rectangle
static constexpr int ClipCodeAbove = 1; /// Clipping above rectangle
static constexpr int ClipCodeBelow = 2; /// Clipping below rectangle
static constexpr int ClipCodeLeft = 4; /// Clipping left rectangle
static constexpr int ClipCodeRight = 8; /// Clipping right rectangle

#if defined(USE_OPENGL) || defined(USE_GLES)

namespace linedraw_gl
{

/**
**  Draw pixel unclipped.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
*/
void DrawPixel(uint32_t color, int x, int y)
{
	GLubyte r, g, b, a;

	CVideo::GetRGBA(color, &r, &g, &b, &a);
	glDisable(GL_TEXTURE_2D);
	glColor4ub(r, g, b, a);
#ifdef USE_GLES
	float vertex[] = {
		2.0f / (GLfloat)Video.Width *x - 1.0f, -2.0f / (GLfloat)Video.Height *y + 1.0f
	};

	glEnableClientState(GL_VERTEX_ARRAY);

	glVertexPointer(2, GL_FLOAT, 0, vertex);
	glDrawArrays(GL_POINTS, 0, 1);

	glDisableClientState(GL_VERTEX_ARRAY);
#endif
#ifdef USE_OPENGL
	glBegin(GL_POINTS);
	glVertex2i(x, y);
	glEnd();
#endif
	glEnable(GL_TEXTURE_2D);
}

/**
**  Draw translucent pixel unclipped.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param alpha  alpha value of pixel.
*/
void DrawTransPixel(uint32_t color, int x, int y, unsigned char alpha)
{
	GLubyte r, g, b;

	CVideo::GetRGB(color, &r, &g, &b);
	color = CVideo::MapRGBA(r, g, b, alpha);
	DrawPixel(color, x, y);
}

/**
**  Draw pixel clipped to current clip setting.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
*/
void DrawPixelClip(uint32_t color, int x, int y)
{
	if (x < ClipX1 || x > ClipX2 || y < ClipY1 || y > ClipY2) {
		return;
	}
	DrawPixel(color, x, y);
}

/**
**  Draw translucent pixel clipped to current clip setting.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param alpha  alpha value of pixel.
*/
void DrawTransPixelClip(uint32_t color, int x, int y, unsigned char alpha)
{
	GLubyte r, g, b;

	CVideo::GetRGB(color, &r, &g, &b);
	color = CVideo::MapRGBA(r, g, b, alpha);
	DrawPixelClip(color, x, y);
}

/**
**  Draw horizontal line unclipped.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param width  width of line (0=don't draw).
*/
void DrawHLine(uint32_t color, int x, int y, int width)
{
	GLubyte r, g, b, a;

	CVideo::GetRGBA(color, &r, &g, &b, &a);
	glDisable(GL_TEXTURE_2D);
	glColor4ub(r, g, b, a);
#ifdef USE_GLES
	float vertex[] = {
		2.0f / (GLfloat)Video.Width *x - 1.0f, -2.0f / (GLfloat)Video.Height *y + 1.0f,
		2.0f / (GLfloat)Video.Width *(x + width) - 1.0f, -2.0f / (GLfloat)Video.Height *y + 1.0f
	};

	glEnableClientState(GL_VERTEX_ARRAY);

	glVertexPointer(2, GL_FLOAT, 0, vertex);
	glDrawArrays(GL_LINES, 0, 2);

	glDisableClientState(GL_VERTEX_ARRAY);
#endif
#ifdef USE_OPENGL
	glBegin(GL_LINES);
	glVertex2i(x, y);
	glVertex2i(x + width, y);
	glEnd();
#endif
	glEnable(GL_TEXTURE_2D);
}

/**
**  Draw translucent horizontal line unclipped.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param width  width of line (0=don't draw).
**  @param alpha  alpha value of pixels.
*/
void DrawTransHLine(uint32_t color, int x, int y, int width, unsigned char alpha)
{
	GLubyte r, g, b;

	CVideo::GetRGB(color, &r, &g, &b);
	color = CVideo::MapRGBA(r, g, b, alpha);
	DrawHLine(color, x, y, width);
}

/**
**  Draw horizontal line clipped.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param width  width of line (0=don't draw).
*/
void DrawHLineClip(uint32_t color, int x, int y, int width)
{
	if (y < ClipY1 || y > ClipY2) {
		return;
	}
	if (x < ClipX1) {
		int f = ClipX1 - x;
		if (width <= f) {
			return;
		}
		width -= f;
		x = ClipX1;
	}
	if ((x + width) > ClipX2 + 1) {
		if (x > ClipX2) {
			return;
		}
		width = ClipX2 - x + 1;
	}
	DrawHLine(color, x, y, width);
}

/**
**  Draw translucent horizontal line clipped.
**
**  @param color  Color index
**  @param x      X pixel coordinate on the screen
**  @param y      Y c pixeloordinate on the screen
**  @param width  Width of line (0=don't draw)
**  @param alpha  Alpha value of pixels
*/
void DrawTransHLineClip(uint32_t color, int x, int y, int width, unsigned char alpha)
{
	GLubyte r, g, b;

	CVideo::GetRGB(color, &r, &g, &b);
	color = CVideo::MapRGBA(r, g, b, alpha);
	DrawHLineClip(color, x, y, width);
}

/**
**  Draw vertical line unclipped.
**
**  @param color   color
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
**  @param height  height of line (0=don't draw).
*/
void DrawVLine(uint32_t color, int x, int y, int height)
{
	GLubyte r, g, b, a;

	CVideo::GetRGBA(color, &r, &g, &b, &a);
	glDisable(GL_TEXTURE_2D);
	glColor4ub(r, g, b, a);
#ifdef USE_GLES
	float vertex[] = {
		2.0f / (GLfloat)Video.Width *x - 1.0f, -2.0f / (GLfloat)Video.Height *y + 1.0f,
		2.0f / (GLfloat)Video.Width *x - 1.0f, -2.0f / (GLfloat)Video.Height *(y + height) + 1.0f
	};

	glEnableClientState(GL_VERTEX_ARRAY);

	glVertexPointer(2, GL_FLOAT, 0, vertex);
	glDrawArrays(GL_LINES, 0, 2);

	glDisableClientState(GL_VERTEX_ARRAY);
#endif
#ifdef USE_OPENGL
	glBegin(GL_LINES);
	glVertex2i(x, y);
	glVertex2i(x, y + height);
	glEnd();
#endif
	glEnable(GL_TEXTURE_2D);
}

/**
**  Draw translucent vertical line unclipped.
**
**  @param color   color
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
**  @param height  height of line (0=don't draw).
**  @param alpha   alpha value of pixels.
*/
void DrawTransVLine(uint32_t color, int x, int y, int height, unsigned char alpha)
{
	GLubyte r, g, b;

	CVideo::GetRGB(color, &r, &g, &b);
	color = CVideo::MapRGBA(r, g, b, alpha);
	DrawVLine(color, x, y, height);
}

/**
**  Draw vertical line clipped.
**
**  @param color   color
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
**  @param height  height of line (0=don't draw).
*/
void DrawVLineClip(uint32_t color, int x, int y, int height)
{
	if (x < ClipX1 || x > ClipX2) {
		return;
	}
	if (y < ClipY1) {
		int f = ClipY1 - y;
		if (height <= f) {
			return;
		}
		height -= f;
		y = ClipY1;
	}
	if ((y + height) > ClipY2 + 1) {
		if (y > ClipY2) {
			return;
		}
		height = ClipY2 - y + 1;
	}
	DrawVLine(color, x, y, height);
}

/**
**  Draw translucent vertical line clipped.
**
**  @param color   color
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
**  @param height  height of line (0=don't draw).
**  @param alpha   alpha value of pixels.
*/
void DrawTransVLineClip(uint32_t color, int x, int y, int height, unsigned char alpha)
{
	GLubyte r, g, b;

	CVideo::GetRGB(color, &r, &g, &b);
	color = CVideo::MapRGBA(r, g, b, alpha);
	DrawVLineClip(color, x, y, height);
}

/**
**  Draw line unclipped into 32bit framebuffer.
**
**  @param color  color
**  @param x1     Source x coordinate on the screen
**  @param y1     Source y coordinate on the screen
**  @param x2     Destination x coordinate on the screen
**  @param y2     Destination y coordinate on the screen
*/
void DrawLine(uint32_t color, int x1, int y1, int x2, int y2)
{
	GLubyte r, g, b, a;

	float xx1 = (float)x1;
	float xx2 = (float)x2;
	float yy1 = (float)y1;
	float yy2 = (float)y2;

	if (xx1 <= xx2) {
		xx2 += 0.5f;
	} else {
		xx1 += 0.5f;
	}
	if (yy1 <= yy2) {
		yy2 += 0.5f;
	} else {
		yy1 += 0.5f;
	}

	CVideo::GetRGBA(color, &r, &g, &b, &a);
	glDisable(GL_TEXTURE_2D);
	glColor4ub(r, g, b, a);
#ifdef USE_GLES
	float vertex[] = {
		2.0f / (GLfloat)Video.Width *xx1 - 1.0f, -2.0f / (GLfloat)Video.Height *yy1 + 1.0f,
		2.0f / (GLfloat)Video.Width *xx2 - 1.0f, -2.0f / (GLfloat)Video.Height *yy2 + 1.0f
	};

	glEnableClientState(GL_VERTEX_ARRAY);

	glVertexPointer(2, GL_FLOAT, 0, vertex);
	glDrawArrays(GL_LINES, 0, 2);

	glDisableClientState(GL_VERTEX_ARRAY);
#endif
#ifdef USE_OPENGL
	glBegin(GL_LINES);
	glVertex2f(xx1, yy1);
	glVertex2f(xx2, yy2);
	glEnd();
#endif
	glEnable(GL_TEXTURE_2D);
}

/**
**  Delivers bitmask denoting given point is left/right/above/below
**      clip rectangle, used for faster determinination of clipped position.
**
**  @param x  pixel's x position (not restricted to screen width)
**  @param y  pixel's y position (not restricted to screen height)
*/
static int ClipCodeLine(int x, int y)
{
	int result;

	if (y < ClipY1) {
		result = ClipCodeAbove;
	} else if (y > ClipY2) {
		result = ClipCodeBelow;
	} else {
		result = ClipCodeInside;
	}

	if (x < ClipX1) {
		result |= ClipCodeLeft;
	} else if (x > ClipX2) {
		result |= ClipCodeRight;
	}

	return result;
}

/**
**  Denotes entire line located at the same side outside clip rectangle
**      (point 1 and 2 are both as left/right/above/below the clip rectangle)
**
**  @param code1  ClipCode of one point of line
**  @param code2  ClipCode of second point of line
*/
static int LineIsUnclippedOnSameSide(int code1, int code2)
{
	return code1 & code2;
}

/**
**  Denotes part of (or entire) line located outside clip rectangle
**      (point 1 and/or 2 is outside clip rectangle)
**
**  @param code1  ClipCode of one point of line
**  @param code2  ClipCode of second point of line
*/
static int LineIsUnclipped(int code1, int code2)
{
	return code1 | code2;
}

/**
**  Draw line clipped.
**      Based on Sutherland-Cohen clipping technique
**      (Replaces Liang/Barksy clipping algorithm in CVS version 1.18, which
**       might be faster, but that one contained some BUGs)
**
**  @param color  color
**  @param x1     Source x coordinate on the screen
**  @param y1     Source y coordinate on the screen
**  @param x2     Destination x coordinate on the screen
**  @param y2     Destination y coordinate on the screen
*/
void DrawLineClip(uint32_t color, int x1, int y1, int x2, int y2)
{
	int code1;
	int code2;

	// Make sure coordinates or on/in clipped rectangle
	while (code1 = ClipCodeLine(x1, y1), code2 = ClipCodeLine(x2, y2),
		   LineIsUnclipped(code1, code2)) {
		if (LineIsUnclippedOnSameSide(code1, code2)) {
			return;
		}

		if (!code1) {
			std::swap(x1, x2);
			std::swap(y1, y2);
			code1 = code2;
		}

		if (code1 & ClipCodeAbove) {
			int temp = ClipY1;
			x1 += (int)(((long)(temp - y1) * (x2 - x1)) / (y2 - y1));
			y1 = temp;
		} else if (code1 & ClipCodeBelow) {
			int temp = ClipY2;
			x1 += (int)(((long)(temp - y1) * (x2 - x1)) / (y2 - y1));
			y1 = temp;
		} else if (code1 & ClipCodeLeft) {
			int temp = ClipX1;
			y1 += (int)(((long)(temp - x1) * (y2 - y1)) / (x2 - x1));
			x1 = temp;
		} else {  /* code1 & ClipCodeRight */
			int temp = ClipX2;
			y1 += (int)(((long)(temp - x1) * (y2 - y1)) / (x2 - x1));
			x1 = temp;
		}
	}

	// Draw line based on clipped coordinates
	// FIXME: As the clipped coordinates are rounded to integers, the line's
	//        direction vector might be slightly off. Somehow, the sub-pixel
	//        position(s) on the clipped retangle should be denoted to the line
	//        drawing routine..
	Assert(x1 >= ClipX1 && x2 >= ClipX1 && x1 <= ClipX2 && x2 <= ClipX2 &&
		   y1 >= ClipY1 && y2 >= ClipY1 && y1 <= ClipY2 && y2 <= ClipY2);
	DrawLine(color, x1, y1, x2, y2);
}

/**
**  Draw a transparent line
*/
void DrawTransLine(uint32_t color, int sx, int sy,
				   int dx, int dy, unsigned char)
{
	// FIXME: trans
	DrawLine(color, sx, sy, dx, dy);
}

/**
**  Draw a transparent line clipped
*/
void DrawTransLineClip(uint32_t color, int sx, int sy,
					   int dx, int dy, unsigned char)
{
	// FIXME: trans
	DrawLineClip(color, sx, sy, dx, dy);
}

/**
**  Draw rectangle.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param h      height of rectangle (0=don't draw).
**  @param w      width of rectangle (0=don't draw).
*/
void DrawRectangle(uint32_t color, int x, int y, int w, int h)
{
	GLubyte r, g, b, a;

	CVideo::GetRGBA(color, &r, &g, &b, &a);
	glDisable(GL_TEXTURE_2D);
	glColor4ub(r, g, b, a);
#ifdef USE_GLES
	float vertex[] = {
		2.0f / (GLfloat)Video.Width *x - 1.0f, -2.0f / (GLfloat)Video.Height *y + 1.0f,
		2.0f / (GLfloat)Video.Width *(x + w) - 1.0f, -2.0f / (GLfloat)Video.Height *y + 1.0f,
		2.0f / (GLfloat)Video.Width *(x + w - 1) - 1.0f, -2.0f / (GLfloat)Video.Height *(y + 1) + 1.0f,
		2.0f / (GLfloat)Video.Width *(x + w - 1) - 1.0f, -2.0f / (GLfloat)Video.Height *(y + h) + 1.0f,
		2.0f / (GLfloat)Video.Width *(x + w - 1) - 1.0f, -2.0f / (GLfloat)Video.Height *(y + h - 1) + 1.0f,
		2.0f / (GLfloat)Video.Width *x - 1.0f, -2.0f / (GLfloat)Video.Height *(y + h - 1) + 1.0f,
		2.0f / (GLfloat)Video.Width *x - 1.0f, -2.0f / (GLfloat)Video.Height *(y + h - 1) + 1.0f,
		2.0f / (GLfloat)Video.Width *x - 1.0f, -2.0f / (GLfloat)Video.Height *(y + 1) + 1.0f
	};

	glEnableClientState(GL_VERTEX_ARRAY);

	glVertexPointer(2, GL_FLOAT, 0, vertex);
	glDrawArrays(GL_LINES, 0, 8);

	glDisableClientState(GL_VERTEX_ARRAY);
#endif
#ifdef USE_OPENGL
	glBegin(GL_LINES);
	glVertex2i(x, y);
	//Wyrmgus start
//	glVertex2i(x + w, y);
	glVertex2i(x + w - 1, y);
	//Wyrmgus end

	glVertex2i(x + w - 1, y + 1);
	//Wyrmgus start
//	glVertex2i(x + w - 1, y + h);
	glVertex2i(x + w - 1, y + h - 1);
	//Wyrmgus end

	glVertex2i(x + w - 1, y + h - 1);
	glVertex2i(x, y + h - 1);

	glVertex2i(x, y + h - 1);
	glVertex2i(x, y + 1);
	glEnd();
#endif
	glEnable(GL_TEXTURE_2D);
}

/**
**  Draw translucent rectangle.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param h      height of rectangle (0=don't draw).
**  @param w      width of rectangle (0=don't draw).
**  @param alpha  alpha value of pixel.
*/
void DrawTransRectangle(uint32_t color, int x, int y,
						int w, int h, unsigned char alpha)
{
	GLubyte r, g, b;

	CVideo::GetRGB(color, &r, &g, &b);
	color = CVideo::MapRGBA(r, g, b, alpha);
	DrawRectangle(color, x, y, w, h);
}

/**
**  Draw rectangle clipped.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param h      height of rectangle (0=don't draw).
**  @param w      width of rectangle (0=don't draw).
*/
void DrawRectangleClip(uint32_t color, int x, int y,
					   int w, int h)
{
	// Ensure non-empty rectangle
	if (!w || !h) {
		// rectangle is `void'
		return;
	}

	// Clip rectangle boundary
	int left = 1;
	int right = 1;
	int top = 1;
	int bottom = 1;

	if (x < ClipX1) {            // no left side
		int f = ClipX1 - x;
		if (w <= f) {
			return;                    // entire rectangle left --> not visible
		}
		w -= f;
		x = ClipX1;
		left = 0;
	}
	if ((x + w) > ClipX2 + 1) {     // no right side
		if (x > ClipX2) {
			return;                    // entire rectangle right --> not visible
		}
		w = ClipX2 - x + 1;
		right = 0;
	}
	if (y < ClipY1) {               // no top
		int f = ClipY1 - y;
		if (h <= f) {
			return;                    // entire rectangle above --> not visible
		}
		h -= f;
		y = ClipY1;
		top = 0;
	}
	if ((y + h) > ClipY2 + 1) {    // no bottom
		if (y > ClipY2) {
			return;                  // entire rectangle below --> not visible
		}
		h = ClipY2 - y + 1;
		bottom = 0;
	}

	// Draw (part of) rectangle sides
	// Note: _hline and _vline should be able to handle zero width/height
	if (top) {
		//Wyrmgus start
//		DrawHLine(color, x, y, w);
		DrawHLine(color, x, y, w - 1);
		//Wyrmgus end
		if (!--h) {
			return;                    // rectangle as horizontal line
		}
		++y;
	}
	if (bottom) {
		//Wyrmgus start
//		DrawHLine(color, x, y + h - 1, w);
		DrawHLine(color, x, y + h - 1, w - 1);
		//Wyrmgus end
		--h;
	}
	if (left) {
		DrawVLine(color, x, y, h);
		if (!--w) {
			return;                    // rectangle as vertical line
		}
		++x;
	}
	if (right) {
		DrawVLine(color, x + w - 1, y, h);
	}
}

/**
**  Draw translucent rectangle clipped.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param h      height of rectangle (0=don't draw).
**  @param w      width of rectangle (0=don't draw).
**  @param alpha  alpha value of pixels.
*/
void DrawTransRectangleClip(uint32_t color, int x, int y,
							int w, int h, unsigned char alpha)
{
	GLubyte r, g, b;

	CVideo::GetRGB(color, &r, &g, &b);
	color = CVideo::MapRGBA(r, g, b, alpha);
	DrawRectangleClip(color, x, y, w, h);
}

/**
**  Fill rectangle.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param h      height of rectangle (0=don't draw).
**  @param w      width of rectangle (0=don't draw).
*/
void FillRectangle(uint32_t color, int x, int y,
				   int w, int h)
{
	GLubyte r, g, b, a;

	CVideo::GetRGBA(color, &r, &g, &b, &a);
	glDisable(GL_TEXTURE_2D);
	glColor4ub(r, g, b, a);
#ifdef USE_GLES
	float vertex[] = {
		2.0f / (GLfloat)Video.Width *x - 1.0f, -2.0f / (GLfloat)Video.Height *y + 1.0f,
		2.0f / (GLfloat)Video.Width *(x + w) - 1.0f, -2.0f / (GLfloat)Video.Height *y + 1.0f,
		2.0f / (GLfloat)Video.Width *x - 1.0f, -2.0f / (GLfloat)Video.Height *(y + h) + 1.0f,
		2.0f / (GLfloat)Video.Width *(x + w) - 1.0f, -2.0f / (GLfloat)Video.Height *(y + h) + 1.0f
	};

	glEnableClientState(GL_VERTEX_ARRAY);

	glVertexPointer(2, GL_FLOAT, 0, vertex);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableClientState(GL_VERTEX_ARRAY);
#endif
#ifdef USE_OPENGL
	glBegin(GL_TRIANGLE_STRIP);
	glVertex2i(x, y);
	glVertex2i(x + w, y);
	glVertex2i(x, y + h);
	glVertex2i(x + w, y + h);
	glEnd();
#endif
	glEnable(GL_TEXTURE_2D);
}

/**
**  Draw translucent rectangle.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param h      height of rectangle (0=don't draw).
**  @param w      width of rectangle (0=don't draw).
**  @param alpha  alpha value of pixel.
*/
void FillTransRectangle(uint32_t color, int x, int y,
						int w, int h, unsigned char alpha)
{
	GLubyte r, g, b;

	CVideo::GetRGB(color, &r, &g, &b);
	color = CVideo::MapRGBA(r, g, b, alpha);
	FillRectangle(color, x, y, w, h);
}

/**
**  Fill rectangle clipped.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param h      height of rectangle (0=don't draw).
**  @param w      width of rectangle (0=don't draw).
*/
void FillRectangleClip(uint32_t color, int x, int y,
					   int w, int h)
{
	CLIP_RECTANGLE(x, y, w, h);
	FillRectangle(color, x, y, w, h);
}

/**
**  Fill rectangle translucent clipped.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param h      height of rectangle (0=don't draw).
**  @param w      width of rectangle (0=don't draw).
**  @param alpha  alpha value of pixels.
*/
void FillTransRectangleClip(uint32_t color, int x, int y,
							int w, int h, unsigned char alpha)
{
	GLubyte r, g, b;

	CVideo::GetRGB(color, &r, &g, &b);
	color = CVideo::MapRGBA(r, g, b, alpha);
	FillRectangleClip(color, x, y, w, h);
}

/**
**  Draw circle.
**
**  @param color   color
**  @param x       Center x coordinate on the screen
**  @param y       Center y coordinate on the screen
**  @param radius  radius of circle
*/
void DrawCircle(uint32_t color, int x, int y, int radius)
{
	int cx = 0;
	int cy = radius;
	int df = 1 - radius;
	int d_e = 3;
	int d_se = -2 * radius + 5;

	// FIXME: could be much improved :)
	do {
		if (cx == 0) {
			DrawPixel(color, x, y + cy);
			DrawPixel(color, x, y - cy);
			DrawPixel(color, x + cy, y);
			DrawPixel(color, x - cy, y);
		} else if (cx == cy) {
			Assert(cx != 0 && cy != 0);
			DrawPixel(color, x + cx, y + cy);
			DrawPixel(color, x - cx, y + cy);
			DrawPixel(color, x + cx, y - cy);
			DrawPixel(color, x - cx, y - cy);
		} else if (cx < cy) {
			Assert(cx != 0 && cy != 0);
			DrawPixel(color, x + cx, y + cy);
			DrawPixel(color, x + cx, y - cy);
			DrawPixel(color, x + cy, y + cx);
			DrawPixel(color, x + cy, y - cx);
			DrawPixel(color, x - cx, y + cy);
			DrawPixel(color, x - cx, y - cy);
			DrawPixel(color, x - cy, y + cx);
			DrawPixel(color, x - cy, y - cx);
		}
		if (df < 0) {
			df += d_e;
			d_se += 2;
		} else {
			df += d_se;
			d_se += 4;
			--cy;
		}
		d_e += 2;
		++cx;
	} while (cx <= cy);
}

/**
**  Draw circle clipped.
**
**  @param color   color
**  @param x       Center x coordinate on the screen
**  @param y       Center y coordinate on the screen
**  @param radius  radius of circle
*/
void DrawCircleClip(uint32_t color, int x, int y, int radius)
{
	int cx = 0;
	int cy = radius;
	int df = 1 - radius;
	int d_e = 3;
	int d_se = -2 * radius + 5;

	// FIXME: could be much improved :)
	do {
		if (cx == 0) {
			DrawPixelClip(color, x, y + cy);
			DrawPixelClip(color, x, y - cy);
			DrawPixelClip(color, x + cy, y);
			DrawPixelClip(color, x - cy, y);
		} else if (cx == cy) {
			Assert(cx != 0 && cy != 0);
			DrawPixelClip(color, x + cx, y + cy);
			DrawPixelClip(color, x - cx, y + cy);
			DrawPixelClip(color, x + cx, y - cy);
			DrawPixelClip(color, x - cx, y - cy);
		} else if (cx < cy) {
			Assert(cx != 0 && cy != 0);
			DrawPixelClip(color, x + cx, y + cy);
			DrawPixelClip(color, x + cx, y - cy);
			DrawPixelClip(color, x + cy, y + cx);
			DrawPixelClip(color, x + cy, y - cx);
			DrawPixelClip(color, x - cx, y + cy);
			DrawPixelClip(color, x - cx, y - cy);
			DrawPixelClip(color, x - cy, y + cx);
			DrawPixelClip(color, x - cy, y - cx);
		}
		if (df < 0) {
			df += d_e;
			d_se += 2;
		} else {
			df += d_se;
			d_se += 4;
			--cy;
		}
		d_e += 2;
		++cx;
	} while (cx <= cy);
}

/**
**  Draw translucent circle.
**
**  @param color   color
**  @param x       Center x coordinate on the screen
**  @param y       Center y coordinate on the screen
**  @param radius  radius of circle
**  @param alpha   alpha value of pixels.
*/
void DrawTransCircle(uint32_t color, int x, int y, int radius,
					 unsigned char alpha)
{
	GLubyte r, g, b;

	CVideo::GetRGB(color, &r, &g, &b);
	color = CVideo::MapRGBA(r, g, b, alpha);
	DrawCircle(color, x, y, radius);
}

/**
**  Draw translucent circle clipped.
**
**  @param color   color
**  @param x       Center x coordinate on the screen
**  @param y       Center y coordinate on the screen
**  @param radius  radius of circle
**  @param alpha   alpha value of pixels.
*/
void DrawTransCircleClip(uint32_t color, int x, int y, int radius,
						 unsigned char alpha)
{
	GLubyte r, g, b;

	CVideo::GetRGB(color, &r, &g, &b);
	color = CVideo::MapRGBA(r, g, b, alpha);
	DrawCircleClip(color, x, y, radius);
}

/**
**  Fill circle.
**
**  @param color   color
**  @param x       Center x coordinate on the screen
**  @param y       Center y coordinate on the screen
**  @param radius  radius of circle
*/
void FillCircle(uint32_t color, int x, int y, int radius)
{
	int p = 1 - radius;
	int py = radius;

	for (int px = 0; px <= py; ++px) {
		// Fill up the middle half of the circle
		DrawVLine(color, x + px, y, py + 1);
		DrawVLine(color, x + px, y - py, py);
		if (px) {
			DrawVLine(color, x - px, y, py + 1);
			DrawVLine(color, x - px, y - py, py);
		}

		if (p < 0) {
			p += 2 * px + 3;
		} else {
			p += 2 * (px - py) + 5;
			py -= 1;
			// Fill up the left/right half of the circle
			if (py >= px) {
				DrawVLine(color, x + py + 1, y, px + 1);
				DrawVLine(color, x + py + 1, y - px, px);
				DrawVLine(color, x - py - 1, y, px + 1);
				DrawVLine(color, x - py - 1, y - px,  px);
			}
		}
	}
}

/**
**  Fill translucent circle clipped.
**
**  @param color   color
**  @param x       Center x coordinate on the screen
**  @param y       Center y coordinate on the screen
**  @param radius  radius of circle
**  @param alpha   alpha value of pixels.
*/
void FillTransCircle(uint32_t color, int x, int y,
					 int radius, unsigned char alpha)
{
	GLubyte r, g, b;

	CVideo::GetRGB(color, &r, &g, &b);
	color = CVideo::MapRGBA(r, g, b, alpha);
	FillCircle(color, x, y, radius);
}

/**
**  Fill circle clipped.
**
**  @param color   color
**  @param x       Center x coordinate on the screen
**  @param y       Center y coordinate on the screen
**  @param radius  radius of circle
*/
void FillCircleClip(uint32_t color, int x, int y, int radius)
{
	int cx = 0;
	int cy = radius;
	int df = 1 - radius;
	int d_e = 3;
	int d_se = -2 * radius + 5;

	// FIXME: could be much improved :)
	do {
		DrawHLineClip(color, x - cy, y - cx, 1 + cy * 2);
		if (cx) {
			DrawHLineClip(color, x - cy, y + cx, 1 + cy * 2);
		}
		if (df < 0) {
			df += d_e;
			d_se += 2;
		} else {
			if (cx != cy) {
				DrawHLineClip(color, x - cx, y - cy, 1 + cx * 2);
				DrawHLineClip(color, x - cx, y + cy, 1 + cx * 2);
			}
			df += d_se;
			d_se += 4;
			--cy;
		}
		d_e += 2;
		++cx;
	} while (cx <= cy);
}

/**
**  Fill translucent circle clipped.
**
**  @param color   color
**  @param x       Center x coordinate on the screen
**  @param y       Center y coordinate on the screen
**  @param radius  radius of circle
**  @param alpha   alpha value of pixels.
*/
void FillTransCircleClip(uint32_t color, int x, int y,
						 int radius, unsigned char alpha)
{
	GLubyte r, g, b;

	CVideo::GetRGB(color, &r, &g, &b);
	color = CVideo::MapRGBA(r, g, b, alpha);
	FillCircleClip(color, x, y, radius);
}

/**
**  Initialize line draw
*/
void InitLineDraw()
{
}

}

#endif

void CVideo::DrawPixelClip(uint32_t color, int x, int y)
{
	linedraw_gl::DrawPixelClip(color, x, y);
}

void CVideo::DrawTransPixelClip(uint32_t color, int x, int y, unsigned char alpha)
{
	linedraw_gl::DrawTransPixelClip(color, x, y, alpha);
}

void CVideo::DrawVLine(uint32_t color, int x, int y, int height)
{
	linedraw_gl::DrawVLine(color, x, y, height);
}

void CVideo::DrawTransVLine(uint32_t color, int x, int y, int height, unsigned char alpha)
{
	linedraw_gl::DrawTransVLine(color, x, y, height, alpha);
}

void CVideo::DrawVLineClip(uint32_t color, int x, int y, int height)
{
	linedraw_gl::DrawVLineClip(color, x, y, height);
}

void CVideo::DrawTransVLineClip(uint32_t color, int x, int y, int height, unsigned char alpha)
{
	linedraw_gl::DrawTransVLineClip(color, x, y, height, alpha);
}

void CVideo::DrawHLine(uint32_t color, int x, int y, int width)
{
	linedraw_gl::DrawHLine(color, x, y, width);
}

void CVideo::DrawTransHLine(uint32_t color, int x, int y, int width, unsigned char alpha)
{
	linedraw_gl::DrawTransHLine(color, x, y, width, alpha);
}

void CVideo::DrawHLineClip(uint32_t color, int x, int y, int width)
{
	linedraw_gl::DrawHLineClip(color, x, y, width);
}

void CVideo::DrawTransHLineClip(uint32_t color, int x, int y, int width, unsigned char alpha)
{
	linedraw_gl::DrawTransHLineClip(color, x, y, width, alpha);
}

void CVideo::DrawLine(uint32_t color, int sx, int sy, int dx, int dy)
{
	linedraw_gl::DrawLine(color, sx, sy, dx, dy);
}

void CVideo::DrawTransLine(uint32_t color, int sx, int sy, int dx, int dy, unsigned char alpha)
{
	linedraw_gl::DrawTransLine(color, sx, sy, dx, dy, alpha);
}

void CVideo::DrawLineClip(uint32_t color, const PixelPos &pos1, const PixelPos &pos2)
{
	linedraw_gl::DrawLineClip(color, pos1.x, pos1.y, pos2.x, pos2.y);
}

void CVideo::DrawTransLineClip(uint32_t color, int sx, int sy, int dx, int dy, unsigned char alpha)
{
	linedraw_gl::DrawTransLineClip(color, sx, sy, dx, dy, alpha);
}

void CVideo::DrawRectangle(uint32_t color, int x, int y, int w, int h)
{
	linedraw_gl::DrawRectangle(color, x, y, w, h);
}

void CVideo::DrawTransRectangle(uint32_t color, int x, int y, int w, int h, unsigned char alpha)
{
	linedraw_gl::DrawTransRectangle(color, x, y, w, h, alpha);
}

void CVideo::DrawRectangleClip(uint32_t color, int x, int y, int w, int h)
{
	linedraw_gl::DrawRectangleClip(color, x, y, w, h);
}

void CVideo::DrawTransRectangleClip(uint32_t color, int x, int y, int w, int h, unsigned char alpha)
{
	linedraw_gl::DrawTransRectangleClip(color, x, y, w, h, alpha);
}

void CVideo::FillRectangle(uint32_t color, int x, int y, int w, int h)
{
	linedraw_gl::FillRectangle(color, x, y, w, h);
}

void CVideo::FillTransRectangle(uint32_t color, int x, int y, int w, int h, unsigned char alpha)
{
	linedraw_gl::FillTransRectangle(color, x, y, w, h, alpha);
}

void CVideo::FillRectangleClip(uint32_t color, int x, int y, int w, int h)
{
	linedraw_gl::FillRectangleClip(color, x, y, w, h);
}

void CVideo::FillTransRectangleClip(uint32_t color, int x, int y, int w, int h, unsigned char alpha)
{
	linedraw_gl::FillTransRectangleClip(color, x, y, w, h, alpha);
}

void CVideo::DrawCircle(uint32_t color, int x, int y, int r)
{
	linedraw_gl::DrawCircle(color, x, y, r);
}

void CVideo::DrawTransCircle(uint32_t color, int x, int y, int r, unsigned char alpha)
{
	linedraw_gl::DrawTransCircle(color, x, y, r, alpha);
}

void CVideo::DrawCircleClip(uint32_t color, int x, int y, int r)
{
	linedraw_gl::DrawCircleClip(color, x, y, r);
}

void CVideo::DrawTransCircleClip(uint32_t color, int x, int y, int r, unsigned char alpha)
{
	linedraw_gl::DrawTransCircleClip(color, x, y, r, alpha);
}

void CVideo::FillCircle(uint32_t color, int x, int y, int r)
{
	linedraw_gl::FillCircle(color, x, y, r);
}

void CVideo::FillTransCircle(uint32_t color, int x, int y, int r, unsigned char alpha)
{
	linedraw_gl::FillTransCircle(color, x, y, r, alpha);
}

void CVideo::FillCircleClip(uint32_t color, const PixelPos &screenPos, int r)
{
	linedraw_gl::FillCircleClip(color, screenPos.x, screenPos.y, r);
}

void CVideo::FillTransCircleClip(uint32_t color, int x, int y, int r, unsigned char alpha)
{
	linedraw_gl::FillTransCircleClip(color, x, y, r, alpha);
}

void InitLineDraw()
{
	linedraw_gl::InitLineDraw();
}
