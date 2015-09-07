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
/**@name grand_strategy.cpp - The grand strategy mode. */
//
//      (c) Copyright 2015 by Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "grand_strategy.h"

#include "font.h"	// for grand strategy mode tooltip drawing
#include "interface.h"
#include "iolib.h"
#include "player.h"
#include "results.h"
#include "ui.h"
#include "unit.h"
#include "unittype.h"
#include "upgrade.h"
#include "util.h"
#include "video.h"

#include <ctype.h>

#include <string>
#include <map>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

bool GrandStrategy = false;				///if the game is in grand strategy mode
bool GrandStrategyGamePaused = false;
std::string GrandStrategyWorld;
int WorldMapOffsetX;
int WorldMapOffsetY;
int GrandStrategyMapWidthIndent;
int GrandStrategyMapHeightIndent;
int BattalionMultiplier;
int PopulationGrowthThreshold = 1000;
std::string GrandStrategyInterfaceState;
CGrandStrategyGame GrandStrategyGame;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Clean up the GrandStrategy elements.
*/
void CGrandStrategyGame::Clean()
{
	for (int x = 0; x < WorldMapWidthMax; ++x) {
		for (int y = 0; y < WorldMapHeightMax; ++y) {
			if (this->WorldMapTiles[x][y]) {
				delete this->WorldMapTiles[x][y];
			}
		}
	}
	this->WorldMapWidth = 0;
	this->WorldMapHeight = 0;
	
	for (int i = 0; i < WorldMapTerrainTypeMax; ++i) {
		if (this->TerrainTypes[i]) {
			delete this->TerrainTypes[i];
		}
	}
	
	for (int i = 0; i < ProvinceMax; ++i) {
		if (this->Provinces[i]) {
			delete this->Provinces[i];
		}
	}
	this->ProvinceCount = 0;
	
	for (int i = 0; i < MaxCosts; ++i) {
		for (int j = 0; j < WorldMapResourceMax; ++j) {
			this->WorldMapResources[i][j].x = -1;
			this->WorldMapResources[i][j].y = -1;
		}
	}
	
	//destroy minimap surface
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		if (this->MinimapSurfaceGL) {
			glDeleteTextures(1, &this->MinimapTexture);
			delete[] this->MinimapSurfaceGL;
			this->MinimapSurfaceGL = NULL;
		}
	} else
#endif
	{
		if (this->MinimapSurface) {
			VideoPaletteListRemove(this->MinimapSurface);
			SDL_FreeSurface(this->MinimapSurface);
			this->MinimapSurface = NULL;
		}
	}
}

/**
**  Draw the grand strategy map.
*/
void CGrandStrategyGame::DrawMap()
{
	int grand_strategy_map_width = UI.MapArea.EndX - UI.MapArea.X;
	int grand_strategy_map_height = UI.MapArea.EndY - UI.MapArea.Y;
	
	int width_indent = GrandStrategyMapWidthIndent;
	int height_indent = GrandStrategyMapHeightIndent;
	
	for (int x = WorldMapOffsetX; x <= (WorldMapOffsetX + (grand_strategy_map_width / 64) + 1) && x < GetWorldMapWidth(); ++x) {
		for (int y = WorldMapOffsetY; y <= (WorldMapOffsetY + (grand_strategy_map_height / 64) + 1) && y < GetWorldMapHeight(); ++y) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->GraphicTile) {
				if (GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->BaseTile != -1 && GrandStrategyGame.WorldMapTiles[x][y]->BaseTile) {
					GrandStrategyGame.WorldMapTiles[x][y]->BaseTile->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
				}
				
				GrandStrategyGame.WorldMapTiles[x][y]->GraphicTile->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
			}
		}
	}
	
	// draw rivers (has to be drawn separately so that they appear over the terrain of the adjacent tiles they go over)
	for (int x = WorldMapOffsetX; x <= (WorldMapOffsetX + (grand_strategy_map_width / 64) + 1) && x < GetWorldMapWidth(); ++x) {
		for (int y = WorldMapOffsetY; y <= (WorldMapOffsetY + (grand_strategy_map_height / 64) + 1) && y < GetWorldMapHeight(); ++y) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->Terrain != -1) {
				for (int i = 0; i < MaxDirections; ++i) {
					if (GrandStrategyGame.WorldMapTiles[x][y]->River[i] != -1) {
						//only draw diagonal directions if inner
						if (i == Northeast && (GrandStrategyGame.WorldMapTiles[x][y]->River[North] != -1 || GrandStrategyGame.WorldMapTiles[x][y]->River[East] != -1)) {
							continue;
						} else if (i == Southeast && (GrandStrategyGame.WorldMapTiles[x][y]->River[South] != -1 || GrandStrategyGame.WorldMapTiles[x][y]->River[East] != -1)) {
							continue;
						} else if (i == Southwest && (GrandStrategyGame.WorldMapTiles[x][y]->River[South] != -1 || GrandStrategyGame.WorldMapTiles[x][y]->River[West] != -1)) {
							continue;
						} else if (i == Northwest && (GrandStrategyGame.WorldMapTiles[x][y]->River[North] != -1 || GrandStrategyGame.WorldMapTiles[x][y]->River[West] != -1)) {
							continue;
						}
						
						if (GrandStrategyGame.WorldMapTiles[x][y]->Province != -1 && GrandStrategyGame.Provinces[GrandStrategyGame.WorldMapTiles[x][y]->Province]->Water) { //water tiles always use rivermouth graphics if they have a river
							//see from which direction the rivermouth comes
							bool flipped = false;
							if (i == North) {
								if (GrandStrategyGame.WorldMapTiles[x - 1][y] && GrandStrategyGame.WorldMapTiles[x - 1][y]->River[North] != -1) {
									flipped = true;
								}
							} else if (i == East) {
								if (GrandStrategyGame.WorldMapTiles[x][y - 1] && GrandStrategyGame.WorldMapTiles[x][y - 1]->River[East] != -1) {
									flipped = true;
								}
							} else if (i == South) {
								if (GrandStrategyGame.WorldMapTiles[x - 1][y] && GrandStrategyGame.WorldMapTiles[x - 1][y]->River[South] != -1) {
									flipped = true;
								}
							} else if (i == West) {
								if (GrandStrategyGame.WorldMapTiles[x][y - 1] && GrandStrategyGame.WorldMapTiles[x][y - 1]->River[West] != -1) {
									flipped = true;
								}
							}
							
							if (flipped) {
								GrandStrategyGame.RivermouthGraphics[i][1]->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent - 10, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 10, true);
							} else {
								GrandStrategyGame.RivermouthGraphics[i][0]->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent - 10, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 10, true);
							}
						} else if (GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[i] != -1) {
							//see to which direction the riverhead runs
							bool flipped = false;
							if (i == North) {
								if (GrandStrategyGame.WorldMapTiles[x][y]->River[Northwest] != -1) {
									flipped = true;
								}
							} else if (i == East) {
								if (GrandStrategyGame.WorldMapTiles[x][y]->River[Northeast] != -1) {
									flipped = true;
								}
							} else if (i == South) {
								if (GrandStrategyGame.WorldMapTiles[x][y]->River[Southwest] != -1) {
									flipped = true;
								}
							} else if (i == West) {
								if (GrandStrategyGame.WorldMapTiles[x][y]->River[Northwest] != -1) {
									flipped = true;
								}
							}
							
							if (flipped) {
								GrandStrategyGame.RiverheadGraphics[i][1]->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent - 10, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 10, true);
							} else {
								GrandStrategyGame.RiverheadGraphics[i][0]->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent - 10, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 10, true);
							}
						} else {
							GrandStrategyGame.RiverGraphics[i]->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent - 10, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 10, true);
						}
					}
				}
			}
		}
	}
	
	// draw pathways (has to be drawn separately so that they appear over the terrain of the adjacent tiles they go over)
	for (int x = WorldMapOffsetX; x <= (WorldMapOffsetX + (grand_strategy_map_width / 64) + 1) && x < GetWorldMapWidth(); ++x) {
		for (int y = WorldMapOffsetY; y <= (WorldMapOffsetY + (grand_strategy_map_height / 64) + 1) && y < GetWorldMapHeight(); ++y) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->Terrain != -1) {
				for (int i = 0; i < MaxDirections; ++i) {
					if (GrandStrategyGame.WorldMapTiles[x][y]->Pathway[i] != -1) {
						GrandStrategyGame.PathwayGraphics[GrandStrategyGame.WorldMapTiles[x][y]->Pathway[i]][i]->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
					}
				}
			}
		}
	}
	
	//draw settlement and resource graphics after rivers and pathways so that they appear over them
	for (int x = WorldMapOffsetX; x <= (WorldMapOffsetX + (grand_strategy_map_width / 64) + 1) && x < GetWorldMapWidth(); ++x) {
		for (int y = WorldMapOffsetY; y <= (WorldMapOffsetY + (grand_strategy_map_height / 64) + 1) && y < GetWorldMapHeight(); ++y) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->Terrain != -1) {
				if (GrandStrategyGame.WorldMapTiles[x][y]->Resource != -1 && GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected) {
					GrandStrategyGame.ResourceBuildingGraphics[GrandStrategyGame.WorldMapTiles[x][y]->Resource]->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
				}
				
				int province_id = GrandStrategyGame.WorldMapTiles[x][y]->Province;
				if (province_id != -1) {
					int civilization = GrandStrategyGame.Provinces[province_id]->Civilization;
					if (civilization != -1 && GrandStrategyGame.Provinces[province_id]->Owner != NULL) {
						//draw the province's settlement
						if (GrandStrategyGame.Provinces[province_id]->SettlementLocation.x == x && GrandStrategyGame.Provinces[province_id]->SettlementLocation.y == y && GrandStrategyGame.Provinces[province_id]->HasBuildingClass("town-hall")) {
							int player_color = PlayerRaces.Factions[GrandStrategyGame.Provinces[province_id]->Owner->Civilization][GrandStrategyGame.Provinces[province_id]->Owner->Faction]->Color;
							
							GrandStrategyGame.SettlementGraphics[civilization]->DrawPlayerColorFrameClip(player_color, 0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
							
							if (GrandStrategyGame.BarracksGraphics[civilization] && GrandStrategyGame.Provinces[province_id]->HasBuildingClass("barracks")) {
								GrandStrategyGame.BarracksGraphics[civilization]->DrawPlayerColorFrameClip(player_color, 0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
							}
						}
					}
					
					//draw symbol that the province is being attacked by the human player, if that is the case
					if (GrandStrategyGame.Provinces[province_id]->AttackedBy != NULL && GrandStrategyGame.Provinces[province_id]->AttackedBy == GrandStrategyGame.PlayerFaction && GrandStrategyGame.Provinces[province_id]->SettlementLocation.x == x && GrandStrategyGame.Provinces[province_id]->SettlementLocation.y == y) {
						GrandStrategyGame.SymbolAttack->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
					} else if (GrandStrategyGame.Provinces[province_id]->Movement && GrandStrategyGame.Provinces[province_id]->Owner != NULL && GrandStrategyGame.Provinces[province_id]->Owner == GrandStrategyGame.PlayerFaction && GrandStrategyGame.Provinces[province_id]->SettlementLocation.x == x && GrandStrategyGame.Provinces[province_id]->SettlementLocation.y == y) {
						GrandStrategyGame.SymbolMove->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
					}
					
					if (GrandStrategyGame.Provinces[province_id]->Heroes.size() > 0 && GrandStrategyGame.Provinces[province_id]->Owner != NULL && GrandStrategyGame.Provinces[province_id]->Owner == GrandStrategyGame.PlayerFaction && GrandStrategyGame.Provinces[province_id]->SettlementLocation.x == x && GrandStrategyGame.Provinces[province_id]->SettlementLocation.y == y) {
						GrandStrategyGame.SymbolHero->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
					}
				}
			}
		}
	}
	
	//draw the tile borders (they need to be drawn here, so that they appear over all tiles, as they go beyond their own tile)
	for (int x = WorldMapOffsetX; x <= (WorldMapOffsetX + (grand_strategy_map_width / 64) + 1) && x < GetWorldMapWidth(); ++x) {
		for (int y = WorldMapOffsetY; y <= (WorldMapOffsetY + (grand_strategy_map_height / 64) + 1) && y < GetWorldMapHeight(); ++y) {
			int province_id = GrandStrategyGame.WorldMapTiles[x][y]->Province;
			if (province_id != -1) {
				for (int i = 0; i < MaxDirections; ++i) {
					if (GrandStrategyGame.WorldMapTiles[x][y]->Borders[i]) {
						//only draw diagonal directions if inner
						if (i == Northeast && (GrandStrategyGame.WorldMapTiles[x][y]->Borders[North] || GrandStrategyGame.WorldMapTiles[x][y]->Borders[East])) {
							continue;
						} else if (i == Southeast && (GrandStrategyGame.WorldMapTiles[x][y]->Borders[South] || GrandStrategyGame.WorldMapTiles[x][y]->Borders[East])) {
							continue;
						} else if (i == Southwest && (GrandStrategyGame.WorldMapTiles[x][y]->Borders[South] || GrandStrategyGame.WorldMapTiles[x][y]->Borders[West])) {
							continue;
						} else if (i == Northwest && (GrandStrategyGame.WorldMapTiles[x][y]->Borders[North] || GrandStrategyGame.WorldMapTiles[x][y]->Borders[West])) {
							continue;
						}
						int sub_x = 0;
						int sub_y = 0;
						if (i == North) {
							sub_y = -1;
						} else if (i == Northeast) {
							sub_x = 1;
							sub_y = -1;
						} else if (i == East) {
							sub_x = 1;
						} else if (i == Southeast) {
							sub_x = 1;
							sub_y = 1;
						} else if (i == South) {
							sub_y = 1;
						} else if (i == Southwest) {
							sub_x = -1;
							sub_y = 1;
						} else if (i == West) {
							sub_x = -1;
						} else if (i == Northwest) {
							sub_x = -1;
							sub_y = -1;
						}
						
						int second_province_id = -1;
						if (GrandStrategyGame.WorldMapTiles[x + sub_x][y + sub_y]) {
							second_province_id = GrandStrategyGame.WorldMapTiles[x + sub_x][y + sub_y]->Province;
						}
						
						if (second_province_id == -1 || (GrandStrategyGame.Provinces[province_id]->Owner == GrandStrategyGame.Provinces[second_province_id]->Owner)) { // is not a national border
							GrandStrategyGame.BorderGraphics[i]->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent - 10, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 10, true);
						} else {
							int player_color;
							if (GrandStrategyGame.Provinces[province_id]->Owner != NULL) {
								player_color = PlayerRaces.Factions[GrandStrategyGame.Provinces[province_id]->Owner->Civilization][GrandStrategyGame.Provinces[province_id]->Owner->Faction]->Color;
							} else {
								player_color = 15;
							}
										
							GrandStrategyGame.NationalBorderGraphics[i]->DrawPlayerColorFrameClip(player_color, 0, 64 * (x - WorldMapOffsetX) + width_indent - 10, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 10, true);
						}
					}
				}
			}
		}
	}
	
	//if is clicking on a tile, draw a square on its borders
	if (!GrandStrategyGamePaused && UI.MapArea.Contains(CursorScreenPos) && GrandStrategyGame.WorldMapTiles[GrandStrategyGame.GetTileUnderCursor().x][GrandStrategyGame.GetTileUnderCursor().y]->Terrain != -1 && (MouseButtons & LeftButton)) {
		int tile_screen_x = ((GrandStrategyGame.GetTileUnderCursor().x - WorldMapOffsetX) * 64) + UI.MapArea.X + width_indent;
		int tile_screen_y = ((GrandStrategyGame.GetTileUnderCursor().y - WorldMapOffsetY) * 64) + UI.MapArea.Y + height_indent;
			
//		clamp(&tile_screen_x, 0, Video.Width);
//		clamp(&tile_screen_y, 0, Video.Height);
			
		Video.DrawRectangle(ColorWhite, tile_screen_x, tile_screen_y, 64, 64);
	}
	
	//draw fog over terra incognita
	for (int x = WorldMapOffsetX; x <= (WorldMapOffsetX + (grand_strategy_map_width / 64) + 1) && x < GetWorldMapWidth(); ++x) {
		for (int y = WorldMapOffsetY; y <= (WorldMapOffsetY + (grand_strategy_map_height / 64) + 1) && y < GetWorldMapHeight(); ++y) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->Terrain == -1) {
				GrandStrategyGame.FogTile->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent - 16, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 16, true);
			}
		}
	}
}

/**
**  Draw the grand strategy map.
*/
void CGrandStrategyGame::DrawMinimap()
{
	#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		glBindTexture(GL_TEXTURE_2D, this->MinimapTexture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->MinimapTextureWidth, this->MinimapTextureHeight,
						GL_RGBA, GL_UNSIGNED_BYTE, this->MinimapSurfaceGL);

	#ifdef USE_GLES
		float texCoord[] = {
			0.0f, 0.0f,
			(float)this->MinimapTextureWidth / this->MinimapTextureWidth, 0.0f,
			0.0f, (float)this->MinimapTextureHeight / this->MinimapTextureHeight,
			(float)this->MinimapTextureWidth / this->MinimapTextureWidth, (float)this->MinimapTextureHeight / this->MinimapTextureHeight
		};

		float vertex[] = {
			2.0f / (GLfloat)Video.Width *(UI.Minimap.X + this->MinimapOffsetX) - 1.0f, -2.0f / (GLfloat)Video.Height *(UI.Minimap.Y + this->MinimapOffsetY) + 1.0f,
			2.0f / (GLfloat)Video.Width *(UI.Minimap.X + this->MinimapOffsetX + this->MinimapTextureWidth) - 1.0f, -2.0f / (GLfloat)Video.Height *(UI.Minimap.Y + this->MinimapOffsetY) + 1.0f,
			2.0f / (GLfloat)Video.Width *(UI.Minimap.X + this->MinimapOffsetX) - 1.0f, -2.0f / (GLfloat)Video.Height *(UI.Minimap.Y + this->MinimapOffsetY + this->MinimapTextureHeight) + 1.0f,
			2.0f / (GLfloat)Video.Width *(UI.Minimap.X + this->MinimapOffsetX + this->MinimapTextureWidth) - 1.0f, -2.0f / (GLfloat)Video.Height *(UI.Minimap.Y + this->MinimapOffsetY + this->MinimapTextureHeight) + 1.0f
		};

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);

		glTexCoordPointer(2, GL_FLOAT, 0, texCoord);
		glVertexPointer(2, GL_FLOAT, 0, vertex);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
#endif
#ifdef USE_OPENGL
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2i(UI.Minimap.X + this->MinimapOffsetX, UI.Minimap.Y + this->MinimapOffsetY);
		glTexCoord2f(0.0f, (float)this->MinimapTextureHeight / this->MinimapTextureHeight);
		glVertex2i(UI.Minimap.X + this->MinimapOffsetX, UI.Minimap.Y + this->MinimapOffsetY + this->MinimapTextureHeight);
		glTexCoord2f((float)this->MinimapTextureWidth / this->MinimapTextureWidth, (float)this->MinimapTextureHeight / this->MinimapTextureHeight);
		glVertex2i(UI.Minimap.X + this->MinimapOffsetX + this->MinimapTextureWidth, UI.Minimap.Y + this->MinimapOffsetY + this->MinimapTextureHeight);
		glTexCoord2f((float)this->MinimapTextureWidth / this->MinimapTextureWidth, 0.0f);
		glVertex2i(UI.Minimap.X + this->MinimapOffsetX + this->MinimapTextureWidth, UI.Minimap.Y + this->MinimapOffsetY);
		glEnd();
#endif
	} else
#endif
	{
		SDL_Rect drect = {Sint16(UI.Minimap.X + this->MinimapOffsetX), Sint16(UI.Minimap.Y + this->MinimapOffsetY), 0, 0};
		SDL_BlitSurface(this->MinimapSurface, NULL, TheScreen, &drect);
	}

	int start_x = UI.Minimap.X + GrandStrategyGame.MinimapOffsetX + (WorldMapOffsetX * this->MinimapTileWidth / 1000);
	int start_y = UI.Minimap.Y + GrandStrategyGame.MinimapOffsetY + (WorldMapOffsetY * this->MinimapTileHeight / 1000);
	int rectangle_width = (((UI.MapArea.EndX - UI.MapArea.X) / 64) + 1) * this->MinimapTileWidth / 1000;
	int rectangle_height = (((UI.MapArea.EndY - UI.MapArea.Y) / 64) + 1) * this->MinimapTileHeight / 1000;

	Video.DrawRectangle(ColorGray, start_x, start_y, rectangle_width, rectangle_height);
}

void CGrandStrategyGame::DrawInterface()
{
	int item_y = 0;
	
	if (this->SelectedProvince != -1) {
		if (GrandStrategyInterfaceState == "Province") {
//			UI.Resources[FoodCost].G->DrawFrameClip(0, 16, Video.Height - 14 - 5, true);
//			CLabel(GetGameFont()).Draw(16 + 14 + 2, Video.Height - 14 - 5, "1000");
		} else if (GrandStrategyInterfaceState == "town-hall" || GrandStrategyInterfaceState == "stronghold") {
			if (GrandStrategyGame.Provinces[this->SelectedProvince]->Civilization != -1) {
				std::string province_culture_string = "Province Culture: " + CapitalizeString(PlayerRaces.Name[GrandStrategyGame.Provinces[this->SelectedProvince]->Civilization]);
				CLabel(GetGameFont()).Draw(UI.InfoPanel.X + ((218 - 6) / 2) - (GetGameFont().Width(province_culture_string) / 2), UI.InfoPanel.Y + 180 - 94 + (item_y * 23), province_culture_string);
				item_y += 1;
			}
			
			std::string administrative_efficiency_string = "Administrative Efficiency: " + std::to_string((long long) 100 + GrandStrategyGame.Provinces[this->SelectedProvince]->GetAdministrativeEfficiencyModifier()) + "%";
			CLabel(GetGameFont()).Draw(UI.InfoPanel.X + ((218 - 6) / 2) - (GetGameFont().Width(administrative_efficiency_string) / 2), UI.InfoPanel.Y + 180 - 94 + (item_y * 23), administrative_efficiency_string);
			item_y += 1;
			
			std::string population_string = "Population: " + std::to_string((long long) GrandStrategyGame.Provinces[this->SelectedProvince]->GetPopulation());
			CLabel(GetGameFont()).Draw(UI.InfoPanel.X + ((218 - 6) / 2) - (GetGameFont().Width(population_string) / 2), UI.InfoPanel.Y + 180 - 94 + (item_y * 23), population_string);
			item_y += 1;
			
			std::string food_string = std::to_string((long long) GrandStrategyGame.Provinces[this->SelectedProvince]->PopulationGrowthProgress) + "/" + std::to_string((long long) PopulationGrowthThreshold);
			int food_change = GrandStrategyGame.Provinces[this->SelectedProvince]->Income[GrainCost] + GrandStrategyGame.Provinces[this->SelectedProvince]->Income[MushroomCost] + GrandStrategyGame.Provinces[this->SelectedProvince]->Income[FishCost] - GrandStrategyGame.Provinces[this->SelectedProvince]->FoodConsumption;

			if (food_change > 0) {
				food_string += "+" + std::to_string((long long) food_change);
			} else if (food_change < 0) {
				food_string += "-" + std::to_string((long long) food_change * -1);
			}
			
			UI.Resources[FoodCost].G->DrawFrameClip(0, UI.InfoPanel.X + ((218 - 6) / 2) - ((GetGameFont().Width(food_string) + 18) / 2), UI.InfoPanel.Y + 180 - 94 + (item_y * 23), true);
			CLabel(GetGameFont()).Draw(UI.InfoPanel.X + ((218 - 6) / 2) - ((GetGameFont().Width(food_string) + 18) / 2) + 18, UI.InfoPanel.Y + 180 - 94 + (item_y * 23), food_string);
		} else if (GrandStrategyInterfaceState == "barracks") {
			std::string revolt_risk_string = "Revolt Risk: " + std::to_string((long long) GrandStrategyGame.Provinces[this->SelectedProvince]->GetRevoltRisk()) + "%";
			CLabel(GetGameFont()).Draw(UI.InfoPanel.X + ((218 - 6) / 2) - (GetGameFont().Width(revolt_risk_string) / 2), UI.InfoPanel.Y + 180 - 94 + (item_y * 23), revolt_risk_string);
			item_y += 1;
		} else if (GrandStrategyInterfaceState == "lumber-mill" || GrandStrategyInterfaceState == "smithy") {
			std::string labor_string = std::to_string((long long) GrandStrategyGame.Provinces[this->SelectedProvince]->Labor);
			UI.Resources[LaborCost].G->DrawFrameClip(0, UI.InfoPanel.X + ((218 - 6) / 2) - ((GetGameFont().Width(labor_string) + 18) / 2), UI.InfoPanel.Y + 180 - 94 + (item_y * 23), true);
			CLabel(GetGameFont()).Draw(UI.InfoPanel.X + ((218 - 6) / 2) - ((GetGameFont().Width(labor_string) + 18) / 2) + 18, UI.InfoPanel.Y + 180 - 94 + (item_y * 23), labor_string);
		}
	}
}

/**
**  Draw the grand strategy tile tooltip.
*/
void CGrandStrategyGame::DrawTileTooltip(int x, int y)
{
	std::string tile_tooltip;
	
	int province_id = GrandStrategyGame.WorldMapTiles[x][y]->Province;
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]->Owner != NULL && GrandStrategyGame.Provinces[province_id]->SettlementLocation.x == x && GrandStrategyGame.Provinces[province_id]->SettlementLocation.y == y && GrandStrategyGame.Provinces[province_id]->HasBuildingClass("town-hall")) {
		tile_tooltip += "Settlement";
		if (!GrandStrategyGame.Provinces[province_id]->GetCulturalSettlementName().empty()) {
			tile_tooltip += " of ";
			tile_tooltip += GrandStrategyGame.Provinces[province_id]->GetCulturalSettlementName();
		}
		tile_tooltip += " (";
		tile_tooltip += GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name;
		tile_tooltip += ")";
	} else if (GrandStrategyGame.WorldMapTiles[x][y]->Resource != -1 && GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected) {
		if (GrandStrategyGame.WorldMapTiles[x][y]->Resource == GoldCost) {
			tile_tooltip += "Gold Mine";
		} else if (GrandStrategyGame.WorldMapTiles[x][y]->Resource == WoodCost) {
			tile_tooltip += "Timber Lodge";
		} else if (GrandStrategyGame.WorldMapTiles[x][y]->Resource == StoneCost) {
			tile_tooltip += "Quarry";
		} else if (GrandStrategyGame.WorldMapTiles[x][y]->Resource == GrainCost) {
			tile_tooltip += "Grain Farm";
		} else if (GrandStrategyGame.WorldMapTiles[x][y]->Resource == MushroomCost) {
			tile_tooltip += "Mushroom Farm";
		}
		tile_tooltip += " (";
		tile_tooltip += GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name;
		tile_tooltip += ")";
	} else if (GrandStrategyGame.WorldMapTiles[x][y]->Terrain != -1) {
		if (!GrandStrategyGame.WorldMapTiles[x][y]->GetCulturalName().empty()) { //if the terrain feature has a particular name, use it
			tile_tooltip += GrandStrategyGame.WorldMapTiles[x][y]->GetCulturalName();
			tile_tooltip += " (";
			tile_tooltip += GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name;
			tile_tooltip += ")";
		} else {
			tile_tooltip += GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name;
		}
	} else {
		tile_tooltip += "Unexplored";
	}
	
	if (province_id != -1 && !GrandStrategyGame.Provinces[province_id]->Water) {
		int tooltip_rivers[MaxDirections];
		memset(tooltip_rivers, -1, sizeof(tooltip_rivers));
		int tooltip_river_count = 0;
		for (int i = 0; i < MaxDirections; ++i) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->River[i] != -1) {
				bool already_in_tooltip = false;
				for (int j = 0; j < MaxDirections; ++j) {
					if (tooltip_rivers[j] == -1) { //reached blank spot, no need to continue the loop
						break;
					}

					if (tooltip_rivers[j] == GrandStrategyGame.WorldMapTiles[x][y]->River[i]) {
						already_in_tooltip = true;
						break;
					}
				}
				if (!already_in_tooltip) {
					tooltip_rivers[tooltip_river_count] = GrandStrategyGame.WorldMapTiles[x][y]->River[i];
					tooltip_river_count += 1;
					tile_tooltip += " (";
					if (!GrandStrategyGame.Rivers[GrandStrategyGame.WorldMapTiles[x][y]->River[i]]->GetCulturalName(GrandStrategyGame.Provinces[province_id]->Civilization).empty()) {
						tile_tooltip += GrandStrategyGame.Rivers[GrandStrategyGame.WorldMapTiles[x][y]->River[i]]->GetCulturalName(GrandStrategyGame.Provinces[province_id]->Civilization) + " ";
					}
					tile_tooltip += "River";
					tile_tooltip += ")";
				}
			}
		}
	}
				
	if (province_id != -1) {
		tile_tooltip += ",\n";
		tile_tooltip += GrandStrategyGame.Provinces[province_id]->GetCulturalName();
		
		if (GrandStrategyGame.Provinces[province_id]->Owner != NULL) {
			tile_tooltip += ",\n";
			tile_tooltip += PlayerRaces.Factions[GrandStrategyGame.Provinces[province_id]->Owner->Civilization][GrandStrategyGame.Provinces[province_id]->Owner->Faction]->Name;
		}
	}
	tile_tooltip += "\n(";
	tile_tooltip += std::to_string((long long) x);
	tile_tooltip += ", ";
	tile_tooltip += std::to_string((long long) y);
	tile_tooltip += ")";

	if (!Preference.NoStatusLineTooltips) {
		CLabel(GetGameFont()).Draw(UI.StatusLine.TextX, UI.StatusLine.TextY, tile_tooltip);
	}
	
	//draw tile tooltip as a popup
	DrawGenericPopup(tile_tooltip, 0, UI.InfoPanel.Y);
}

void CGrandStrategyGame::DoTurn()
{
	//allocate labor
	for (int i = 0; i < this->ProvinceCount; ++i) {
		if (this->Provinces[i] && !this->Provinces[i]->Name.empty()) { //if this is a valid province
			if (this->Provinces[i]->Civilization != -1 && this->Provinces[i]->Owner != NULL && this->Provinces[i]->Labor > 0) { // if this province has a culture and an owner, and has surplus labor
				this->Provinces[i]->AllocateLabor();
			}
		} else { //if a somehow invalid province is reached
			break;
		}
	}
	
	//faction income
	for (int i = 0; i < MAX_RACES; ++i) {
		for (int j = 0; j < FactionMax; ++j) {
			if (this->Factions[i][j]) {
				if (this->Factions[i][j]->ProvinceCount > 0) {
					for (int k = 0; k < MaxCosts; ++k) {
						if (k == GrainCost || k == MushroomCost || k == FishCost) { //food resources are not added to the faction's storage, being stored at the province level instead
							continue;
						} else if (k == ResearchCost) {
							this->Factions[i][j]->Resources[k] += this->Factions[i][j]->Income[k] / this->Factions[i][j]->ProvinceCount;
						} else {
							this->Factions[i][j]->Resources[k] += this->Factions[i][j]->Income[k];
						}
					}
				}
			} else { //end of valid factions
				break;
			}
		}
	}
	
	//this function takes care only of some things for now, move the rest from Lua later
	this->DoTrade();
	
	for (int i = 0; i < this->ProvinceCount; ++i) {
		if (this->Provinces[i] && !this->Provinces[i]->Name.empty()) { //if this is a valid province
			if (this->Provinces[i]->Civilization != -1) { // if this province has a culture
				// construct buildings
				if (this->Provinces[i]->CurrentConstruction != -1) {
					this->Provinces[i]->SetSettlementBuilding(this->Provinces[i]->CurrentConstruction, true);
					this->Provinces[i]->CurrentConstruction = -1;
				}
					
				// if the province has a town hall, a barracks and a smithy, give it a mercenary camp; not for Earth for now, since there are no recruitable mercenaries for Earth yet
				int mercenary_camp_id = UnitTypeIdByIdent("unit-mercenary-camp");
				if (mercenary_camp_id != -1 && this->Provinces[i]->SettlementBuildings[mercenary_camp_id] == false && GrandStrategyWorld != "Earth") {
					if (this->Provinces[i]->HasBuildingClass("town-hall") && this->Provinces[i]->HasBuildingClass("barracks") && this->Provinces[i]->HasBuildingClass("smithy")) {
						this->Provinces[i]->SetSettlementBuilding(mercenary_camp_id, true);
					}
				}
				
				if (this->Provinces[i]->Owner != NULL) {
					//check revolt risk and potentially trigger a revolt
					if (
						this->Provinces[i]->GetRevoltRisk() > 0
						&& SyncRand(100) < this->Provinces[i]->GetRevoltRisk()
						&& this->Provinces[i]->AttackedBy == NULL
						&& this->Provinces[i]->TotalWorkers > 0
					) { //if a revolt is triggered this turn (a revolt can only happen if the province is not being attacked that turn, and the quantity of revolting units is based on the quantity of workers in the province)
						int possible_revolters[FactionMax];
						int possible_revolter_count = 0;
						for (int j = 0; j < this->Provinces[i]->ClaimCount; ++j) {
							if (
								this->Provinces[i]->Claims[j][0] == this->Provinces[i]->Civilization
								&& PlayerRaces.Factions[this->Provinces[i]->Civilization][this->Provinces[i]->Claims[j][1]]->Type == PlayerRaces.Factions[this->Provinces[i]->Owner->Civilization][this->Provinces[i]->Owner->Faction]->Type
								&& !(this->Provinces[i]->Claims[j][0] == this->Provinces[i]->Owner->Civilization && this->Provinces[i]->Claims[j][1] == this->Provinces[i]->Owner->Faction)
								&& PlayerRaces.Factions[GrandStrategyGame.Provinces[i]->Claims[j][0]][GrandStrategyGame.Provinces[i]->Claims[j][1]]->Name != PlayerRaces.Factions[GrandStrategyGame.Provinces[i]->Owner->Civilization][GrandStrategyGame.Provinces[i]->Owner->Faction]->Name // they can't have the same name (this is needed because some of the Lua code identifies factions by name)
							) { //if faction which has a claim on this province has the same civilization as the province, and if it is of the same faction type as the province's owner
								possible_revolters[possible_revolter_count] = this->Provinces[i]->Claims[j][1];
								possible_revolter_count += 1;
							}
						}
						if (possible_revolter_count > 0) {
							int revolter_faction = possible_revolters[SyncRand(possible_revolter_count)];
							this->Provinces[i]->AttackedBy = const_cast<CGrandStrategyFaction *>(&(*GrandStrategyGame.Factions[this->Provinces[i]->Civilization][revolter_faction]));
							
							int militia_id = PlayerRaces.GetCivilizationClassUnitType(this->Provinces[i]->Civilization, GetUnitTypeClassIndexByName("militia"));
							int infantry_id = PlayerRaces.GetCivilizationClassUnitType(this->Provinces[i]->Civilization, GetUnitTypeClassIndexByName("infantry"));
							
							if (militia_id != -1) {
								this->Provinces[i]->SetAttackingUnitQuantity(militia_id, SyncRand(this->Provinces[i]->TotalWorkers) + 1);
							} else if (infantry_id != -1 && this->Provinces[i]->TotalWorkers >= 2) { //if the province's civilization doesn't have militia units, use infantry instead (but with half the quantity)
								this->Provinces[i]->SetAttackingUnitQuantity(infantry_id, (SyncRand(this->Provinces[i]->TotalWorkers / 2) + 1));
							}
						}
					}
					
					if (!this->Provinces[i]->HasFactionClaim(this->Provinces[i]->Owner->Civilization, this->Provinces[i]->Owner->Faction) && SyncRand(100) < 1) { // 1% chance the owner of this province will get a claim on it
						this->Provinces[i]->AddFactionClaim(this->Provinces[i]->Owner->Civilization, this->Provinces[i]->Owner->Faction);
					}
					
					
					//population growth
	//				this->Provinces[i]->PopulationGrowthProgress += (this->Provinces[i]->GetPopulation() / 2) * BasePopulationGrowthPermyriad / 10000;
					int province_food_income = this->Provinces[i]->Income[GrainCost] + this->Provinces[i]->Income[MushroomCost] + this->Provinces[i]->Income[FishCost] - this->Provinces[i]->FoodConsumption;
					this->Provinces[i]->PopulationGrowthProgress += province_food_income;
					if (this->Provinces[i]->PopulationGrowthProgress >= PopulationGrowthThreshold) { //if population growth progress is greater than or equal to the population growth threshold, create a new worker unit
						if (province_food_income >= 100) { //if province food income is enough to support a new unit
							int worker_unit_type = PlayerRaces.GetCivilizationClassUnitType(this->Provinces[i]->Civilization, GetUnitTypeClassIndexByName("worker"));
							int new_units = this->Provinces[i]->PopulationGrowthProgress / PopulationGrowthThreshold;
							this->Provinces[i]->PopulationGrowthProgress -= PopulationGrowthThreshold * new_units;
							
							this->Provinces[i]->ChangeUnitQuantity(worker_unit_type, new_units);
						} else { //if the province's food income is positive, but not enough to sustain a new unit, keep it at the population growth threshold
							this->Provinces[i]->PopulationGrowthProgress = PopulationGrowthThreshold;
						}
					} else if (province_food_income < 0) { // if the province's food income is negative, then try to reallocate labor
						this->Provinces[i]->ReallocateLabor();
						province_food_income = this->Provinces[i]->Income[GrainCost] + this->Provinces[i]->Income[MushroomCost] + this->Provinces[i]->Income[FishCost] - this->Provinces[i]->FoodConsumption;
						//if the food income is still negative after reallocating labor (this shouldn't happen most of the time!) then decrease the population by 1 due to starvation; only do this if the population is above 1 (to prevent provinces from being entirely depopulated and unable to grow a population afterwards)
						if (province_food_income < 0) {
							int worker_unit_type = PlayerRaces.GetCivilizationClassUnitType(this->Provinces[i]->Civilization, GetUnitTypeClassIndexByName("worker"));
							this->Provinces[i]->ChangeUnitQuantity(worker_unit_type, -1);
						}
					}
					this->Provinces[i]->PopulationGrowthProgress = std::max(0, this->Provinces[i]->PopulationGrowthProgress);
				}
			}
			this->Provinces[i]->Movement = false; //after processing the turn, always set the movement to false
		} else { //if a somehow invalid province is reached
			break;
		}
	}
	
	//research technologies
	for (int i = 0; i < MAX_RACES; ++i) {
		for (int j = 0; j < FactionMax; ++j) {
			if (this->Factions[i][j]) {
				if (this->Factions[i][j]->CurrentResearch != -1) {
					this->Factions[i][j]->SetTechnology(this->Factions[i][j]->CurrentResearch, true);
					this->Factions[i][j]->CurrentResearch = -1;
				}
			} else { //end of valid factions
				break;
			}
		}
	}
}

void CGrandStrategyGame::DoTrade()
{
	//store the human player's trade settings
	int player_trade_preferences[MaxCosts];
	if (this->PlayerFaction != NULL) {
		for (int i = 0; i < MaxCosts; ++i) {
			player_trade_preferences[i] = this->PlayerFaction->Trade[i];
		}
	}
	
	bool province_consumed_commodity[MaxCosts][ProvinceMax];
	for (int i = 0; i < MaxCosts; ++i) {
		for (int j = 0; j < this->ProvinceCount; ++j) {
			province_consumed_commodity[i][j] = false;
		}
	}
	
	// first sell to domestic provinces, then to other factions, and only then to foreign provinces
	for (int i = 0; i < MAX_RACES; ++i) {
		for (int j = 0; j < FactionMax; ++j) {
			if (this->Factions[i][j]) {
				if (this->Factions[i][j]->ProvinceCount > 0) {
					for (int k = 0; k < this->Factions[i][j]->ProvinceCount; ++k) {
						int province_id = this->Factions[i][j]->OwnedProvinces[k];
						for (int res = 0; res < MaxCosts; ++res) {
							if (res == GoldCost || res == ResearchCost || res == PrestigeCost || res == LaborCost || res == GrainCost || res == MushroomCost || res == FishCost) {
								continue;
							}
							
							if (province_consumed_commodity[res][province_id] == false && this->Factions[i][j]->Trade[res] >= this->Provinces[province_id]->GetResourceDemand(res) && this->Provinces[province_id]->HasBuildingClass("town-hall")) {
								this->Factions[i][j]->Resources[res] -= this->Provinces[province_id]->GetResourceDemand(res);
								this->Factions[i][j]->Resources[GoldCost] += this->Provinces[province_id]->GetResourceDemand(res) * this->CommodityPrices[res] / 100;
								this->Factions[i][j]->Trade[res] -= this->Provinces[province_id]->GetResourceDemand(res);
								province_consumed_commodity[res][province_id] = true;
							}
						}
					}
				}
			} else { //end of valid factions
				break;
			}
		}
	}
	
	CGrandStrategyFaction *factions_by_prestige[MAX_RACES * FactionMax];
	int factions_by_prestige_count = 0;
	for (int i = 0; i < MAX_RACES; ++i) {
		for (int j = 0; j < FactionMax; ++j) {
			if (this->Factions[i][j]) {
				if (this->Factions[i][j]->ProvinceCount > 0) {
					factions_by_prestige[factions_by_prestige_count] = const_cast<CGrandStrategyFaction *>(&(*this->Factions[i][j]));
					factions_by_prestige_count += 1;
				}
			} else { //end of valid factions
				break;
			}
		}
	}
	
	//sort factions by prestige
	bool swapped = true;
	for (int passes = 0; passes < factions_by_prestige_count && swapped; ++passes) {
		swapped = false;
		for (int i = 0; i < factions_by_prestige_count - 1; ++i) {
			if (factions_by_prestige[i] && factions_by_prestige[i + 1]) {
				if (!TradePriority(*factions_by_prestige[i], *factions_by_prestige[i + 1])) {
					CGrandStrategyFaction *temp_faction = const_cast<CGrandStrategyFaction *>(&(*factions_by_prestige[i]));
					factions_by_prestige[i] = const_cast<CGrandStrategyFaction *>(&(*factions_by_prestige[i + 1]));
					factions_by_prestige[i + 1] = const_cast<CGrandStrategyFaction *>(&(*temp_faction));
					swapped = true;
				}
			} else {
				break;
			}
		}
	}

	for (int i = 0; i < factions_by_prestige_count; ++i) {
		if (factions_by_prestige[i]) {
			for (int res = 0; res < MaxCosts; ++res) {
				if (res == GoldCost || res == ResearchCost || res == PrestigeCost || res == LaborCost || res == GrainCost || res == MushroomCost || res == FishCost) {
					continue;
				}
				
				if (factions_by_prestige[i]->Trade[res] < 0) { // if wants to import lumber
					for (int j = 0; j < factions_by_prestige_count; ++j) {
						if (j != i && factions_by_prestige[j]) {
							if (factions_by_prestige[j]->Trade[res] > 0) { // if second faction wants to export lumber
								this->PerformTrade(*factions_by_prestige[i], *factions_by_prestige[j], res);
							}
						} else {
							break;
						}
					}
				} else if (factions_by_prestige[i]->Trade[res] > 0) { // if wants to export lumber
					for (int j = 0; j < factions_by_prestige_count; ++j) {
						if (j != i && factions_by_prestige[j]) {
							if (factions_by_prestige[j]->Trade[res] < 0) { // if second faction wants to import lumber
								this->PerformTrade(*factions_by_prestige[j], *factions_by_prestige[i], res);
							}
						} else {
							break;
						}
					}
				}
			}
		} else {
			break;
		}
	}
	
	//sell to foreign provinces
	for (int i = 0; i < factions_by_prestige_count; ++i) {
		if (factions_by_prestige[i]) {
			for (int j = 0; j < factions_by_prestige_count; ++j) {
				if (j != i && factions_by_prestige[j]) {
					for (int k = 0; k < factions_by_prestige[j]->ProvinceCount; ++k) {
						int province_id = factions_by_prestige[j]->OwnedProvinces[k];
						
						for (int res = 0; res < MaxCosts; ++res) {
							if (res == GoldCost || res == ResearchCost || res == PrestigeCost || res == LaborCost || res == GrainCost || res == MushroomCost || res == FishCost) {
								continue;
							}
							
							if (province_consumed_commodity[res][province_id] == false && factions_by_prestige[i]->Trade[res] >= this->Provinces[province_id]->GetResourceDemand(res) && this->Provinces[province_id]->HasBuildingClass("town-hall")) {
								factions_by_prestige[i]->Resources[res] -= this->Provinces[province_id]->GetResourceDemand(res);
								factions_by_prestige[i]->Resources[GoldCost] += this->Provinces[province_id]->GetResourceDemand(res) * this->CommodityPrices[res] / 100;
								factions_by_prestige[i]->Trade[res] -= this->Provinces[province_id]->GetResourceDemand(res);
								province_consumed_commodity[res][province_id] = true;
							}
						}
					}
				} else {
					break;
				}
			}
		} else {
			break;
		}
	}

	// check whether offers or bids have been greater, and change the commodity's price accordingly
	int remaining_wanted_trade[MaxCosts];
	memset(remaining_wanted_trade, 0, sizeof(remaining_wanted_trade));
	for (int res = 0; res < MaxCosts; ++res) {
		if (res == GoldCost || res == ResearchCost || res == PrestigeCost || res == LaborCost || res == GrainCost || res == MushroomCost || res == FishCost) {
			continue;
		}
		
		for (int i = 0; i < factions_by_prestige_count; ++i) {
			if (factions_by_prestige[i]) {
				remaining_wanted_trade[res] += factions_by_prestige[i]->Trade[res];
			} else {
				break;
			}
		}
		
		for (int i = 0; i < this->ProvinceCount; ++i) {
			if (this->Provinces[i] && !this->Provinces[i]->Name.empty()) { //if this is a valid province
				if (this->Provinces[i]->HasBuildingClass("town-hall") && this->Provinces[i]->Owner != NULL) {
					if (province_consumed_commodity[res][i] == false) {
						remaining_wanted_trade[res] -= this->Provinces[i]->GetResourceDemand(res);
					}
				}
			} else { //if a somehow invalid province is reached
				break;
			}
		}
	
		if (remaining_wanted_trade[res] > 0 && this->CommodityPrices[res] > 1) { // more offers than bids
			this->CommodityPrices[res] -= 1;
		} else if (remaining_wanted_trade[res] < 0) { // more bids than offers
			this->CommodityPrices[res] += 1;
		}
	}
	
	//now restore the human player's trade settings
	if (this->PlayerFaction != NULL) {
		for (int i = 0; i < MaxCosts; ++i) {
			if (i == GoldCost || i == ResearchCost || i == PrestigeCost || i == LaborCost || i == GrainCost || i == MushroomCost || i == FishCost) {
				continue;
			}
		
			if (player_trade_preferences[i] > 0 && this->PlayerFaction->Resources[i] < player_trade_preferences[i]) {
				player_trade_preferences[i] = this->PlayerFaction->Resources[i];
			} else if (player_trade_preferences[i] < 0 && this->PlayerFaction->Resources[GoldCost] < 0) {
				player_trade_preferences[i] = 0;
			} else if (player_trade_preferences[i] < 0 && this->PlayerFaction->Resources[GoldCost] < (player_trade_preferences[i] * -1 * this->CommodityPrices[i] / 100)) {
				player_trade_preferences[i] = (this->PlayerFaction->Resources[GoldCost] / this->CommodityPrices[i] * 100) * -1;
			}
			this->PlayerFaction->Trade[i] = player_trade_preferences[i];
		}
	}
}

void CGrandStrategyGame::DoProspection()
{
	for (int i = 0; i < MaxCosts; ++i) {
		for (int j = 0; j < WorldMapResourceMax; ++j) {
			if (GrandStrategyGame.WorldMapResources[i][j].x != -1 && GrandStrategyGame.WorldMapResources[i][j].y != -1) {
				int x = GrandStrategyGame.WorldMapResources[i][j].x;
				int y = GrandStrategyGame.WorldMapResources[i][j].y;
				
				if (GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected) { //already discovered
					continue;
				}

				int province_id = GrandStrategyGame.WorldMapTiles[x][y]->Province;
						
				if (province_id != -1 && GrandStrategyGame.Provinces[province_id]->Owner != NULL && GrandStrategyGame.Provinces[province_id]->HasBuildingClass("town-hall")) {
					if (SyncRand(100) < 1) { // 1% chance of discovery per turn
						GrandStrategyGame.WorldMapTiles[x][y]->SetResourceProspected(i, true);
						if (GrandStrategyGame.PlayerFaction == GrandStrategyGame.Provinces[province_id]->Owner) {
							char buf[256];
							snprintf(
								buf, sizeof(buf), "if (GrandStrategyDialog ~= nil) then GrandStrategyDialog(\"%s\", \"%s\") end;",
								(CapitalizeString(DefaultResourceNames[i]) + " found in " + GrandStrategyGame.Provinces[province_id]->GetCulturalName()).c_str(),
								("My lord, " + CapitalizeString(DefaultResourceNames[i]) + " has been found in the " + DecapitalizeString(GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name) + " of " + GrandStrategyGame.Provinces[province_id]->GetCulturalName() + "!").c_str()
							);
							CclCommand(buf);
							
							/*
							if (wyr.preferences.ShowTips) {
								Tip("Gold Discovery in Province", "Congratulations! You have found gold in one of your provinces. Each gold mine provides you with 200 gold per turn, if a town hall is built in its province.")
							}
							*/
						}
					}
				}
			} else {
				break;
			}
		}
	}
}

void CGrandStrategyGame::PerformTrade(CGrandStrategyFaction &importer_faction, CGrandStrategyFaction &exporter_faction, int resource)
{
	if (abs(importer_faction.Trade[resource]) > exporter_faction.Trade[resource]) {
		importer_faction.Resources[resource] += exporter_faction.Trade[resource];
		exporter_faction.Resources[resource] -= exporter_faction.Trade[resource];

		importer_faction.Resources[GoldCost] -= exporter_faction.Trade[resource] * this->CommodityPrices[resource] / 100;
		exporter_faction.Resources[GoldCost] += exporter_faction.Trade[resource] * this->CommodityPrices[resource] / 100;
		
		importer_faction.Trade[resource] += exporter_faction.Trade[resource];
		exporter_faction.Trade[resource] = 0;
	} else {
		importer_faction.Resources[resource] += abs(importer_faction.Trade[resource]);
		exporter_faction.Resources[resource] -= abs(importer_faction.Trade[resource]);

		importer_faction.Resources[GoldCost] -= abs(importer_faction.Trade[resource]) * this->CommodityPrices[resource] / 100;
		exporter_faction.Resources[GoldCost] += abs(importer_faction.Trade[resource]) * this->CommodityPrices[resource] / 100;
		
		exporter_faction.Trade[resource] += importer_faction.Trade[resource];
		importer_faction.Trade[resource] = 0;
	}
}

bool CGrandStrategyGame::TradePriority(CGrandStrategyFaction &faction_a, CGrandStrategyFaction &faction_b)
{
	return faction_a.Resources[PrestigeCost] > faction_b.Resources[PrestigeCost];
}

Vec2i CGrandStrategyGame::GetTileUnderCursor()
{
	Vec2i tile_under_cursor(0, 0);

	if (UI.MapArea.Contains(CursorScreenPos)) {
		int width_indent = GrandStrategyMapWidthIndent;
		int height_indent = GrandStrategyMapHeightIndent;

		tile_under_cursor.x = WorldMapOffsetX + ((CursorScreenPos.x - UI.MapArea.X - width_indent) / 64);
		tile_under_cursor.y = WorldMapOffsetY + ((CursorScreenPos.y - UI.MapArea.Y - height_indent) / 64);
	} else if (
		CursorScreenPos.x >= UI.Minimap.X + GrandStrategyGame.MinimapOffsetX
		&& CursorScreenPos.x < UI.Minimap.X + GrandStrategyGame.MinimapOffsetX + GrandStrategyGame.MinimapTextureWidth
		&& CursorScreenPos.y >= UI.Minimap.Y + GrandStrategyGame.MinimapOffsetY
		&& CursorScreenPos.y < UI.Minimap.Y + GrandStrategyGame.MinimapOffsetY + GrandStrategyGame.MinimapTextureHeight
	) {
		tile_under_cursor.x = (CursorScreenPos.x - UI.Minimap.X - GrandStrategyGame.MinimapOffsetX) * 1000 / this->MinimapTileWidth;
		tile_under_cursor.y = (CursorScreenPos.y - UI.Minimap.Y - GrandStrategyGame.MinimapOffsetY) * 1000 / this->MinimapTileHeight;
	}
	
	return tile_under_cursor;
}

#if defined(USE_OPENGL) || defined(USE_GLES)
/**
**  Create the minimap texture
*/
void CGrandStrategyGame::CreateMinimapTexture()
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &this->MinimapTexture);
	glBindTexture(GL_TEXTURE_2D, this->MinimapTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->MinimapTextureWidth,
				 this->MinimapTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
				 this->MinimapSurfaceGL);
}
#endif

void CGrandStrategyGame::UpdateMinimap()
{
	// Clear Minimap background if not transparent
	#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		memset(this->MinimapSurfaceGL, 0, this->MinimapTextureWidth * this->MinimapTextureHeight * 4);
	} else
	#endif
	{
		SDL_FillRect(this->MinimapSurface, NULL, SDL_MapRGB(this->MinimapSurface->format, 0, 0, 0));
	}

	int bpp;
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		bpp = 0;
	} else
#endif
	{
		bpp = this->MinimapSurface->format->BytesPerPixel;
	}

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		SDL_LockSurface(this->MinimapSurface);
	}

	for (int my = 0; my < this->MinimapTextureHeight; ++my) {
		for (int mx = 0; mx < this->MinimapTextureWidth; ++mx) {
			int tile_x = mx * 1000 / this->MinimapTileWidth;
			int tile_y = my * 1000 / this->MinimapTileHeight;
#if defined(USE_OPENGL) || defined(USE_GLES)
			if (UseOpenGL) {
				if (GrandStrategyGame.WorldMapTiles[tile_x][tile_y] && GrandStrategyGame.WorldMapTiles[tile_x][tile_y]->Province != -1) {
					int province_id = GrandStrategyGame.WorldMapTiles[tile_x][tile_y]->Province;
					if (GrandStrategyGame.Provinces[province_id]->Owner != NULL) {
						int player_color = PlayerRaces.Factions[GrandStrategyGame.Provinces[province_id]->Owner->Civilization][GrandStrategyGame.Provinces[province_id]->Owner->Faction]->Color;
						*(Uint32 *)&(this->MinimapSurfaceGL[(mx + my * this->MinimapTextureWidth) * 4]) = Video.MapRGB(TheScreen->format, PlayerColorsRGB[player_color][0]);
					} else if (GrandStrategyGame.Provinces[province_id]->Water) {
						*(Uint32 *)&(this->MinimapSurfaceGL[(mx + my * this->MinimapTextureWidth) * 4]) = Video.MapRGB(TheScreen->format, 171, 198, 217);
					} else {
						*(Uint32 *)&(this->MinimapSurfaceGL[(mx + my * this->MinimapTextureWidth) * 4]) = Video.MapRGB(TheScreen->format, 255, 255, 255);
					}
				} else {
					*(Uint32 *)&(this->MinimapSurfaceGL[(mx + my * this->MinimapTextureWidth) * 4]) = Video.MapRGB(0, 0, 0, 0);
				}
			} else
#endif
			{
				const int index = mx * bpp + my * this->MinimapSurface->pitch;
				if (GrandStrategyGame.WorldMapTiles[tile_x][tile_y] && GrandStrategyGame.WorldMapTiles[tile_x][tile_y]->Province != -1) {
					int province_id = GrandStrategyGame.WorldMapTiles[tile_x][tile_y]->Province;
					if (GrandStrategyGame.Provinces[province_id]->Owner != NULL) {
						int player_color = PlayerRaces.Factions[GrandStrategyGame.Provinces[province_id]->Owner->Civilization][GrandStrategyGame.Provinces[province_id]->Owner->Faction]->Color;
						if (bpp == 2) {
							*(Uint16 *)&((Uint8 *)this->MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, PlayerColorsRGB[player_color][0]);
						} else {
							*(Uint32 *)&((Uint8 *)this->MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, PlayerColorsRGB[player_color][0]);
						}
					} else if (GrandStrategyGame.Provinces[province_id]->Water) {
						if (bpp == 2) {
							*(Uint16 *)&((Uint8 *)this->MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, 171, 198, 217);
						} else {
							*(Uint32 *)&((Uint8 *)this->MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, 171, 198, 217);
						}
					} else {
						if (bpp == 2) {
							*(Uint16 *)&((Uint8 *)this->MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, 255, 255, 255);
						} else {
							*(Uint32 *)&((Uint8 *)this->MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, 255, 255, 255);
						}
					}
				} else {
					if (bpp == 2) {
						*(Uint16 *)&((Uint8 *)this->MinimapSurface->pixels)[index] = ColorBlack;
					} else {
						*(Uint32 *)&((Uint8 *)this->MinimapSurface->pixels)[index] = ColorBlack;
					}
				}
			}
		}
	}

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		SDL_UnlockSurface(this->MinimapSurface);
	}
}

void WorldMapTile::UpdateMinimap()
{
	if (!(
		(GetWorldMapWidth() <= UI.Minimap.X && GetWorldMapHeight() <= UI.Minimap.Y)
		|| (
			(this->Position.x % std::max(1000 / GrandStrategyGame.MinimapTileWidth, 1)) == 0
			&& (this->Position.y % std::max(1000 / GrandStrategyGame.MinimapTileHeight, 1)) == 0
		)
	)) {
		return;
	}
	
	int bpp;
	#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		bpp = 0;
	} else
	#endif
	{
		bpp = GrandStrategyGame.MinimapSurface->format->BytesPerPixel;
	}

	#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
	#endif
	{
		SDL_LockSurface(GrandStrategyGame.MinimapSurface);
	}

	int minimap_tile_width = std::max(GrandStrategyGame.MinimapTileWidth / 1000, 1);
	int minimap_tile_height = std::max(GrandStrategyGame.MinimapTileHeight / 1000, 1);
	for (int sub_x = 0; sub_x < minimap_tile_width; ++sub_x) {
		for (int sub_y = 0; sub_y < minimap_tile_height; ++sub_y) {
			int mx = (this->Position.x * GrandStrategyGame.MinimapTileWidth / 1000) + sub_x;
			int my = (this->Position.y * GrandStrategyGame.MinimapTileHeight / 1000) + sub_y;
#if defined(USE_OPENGL) || defined(USE_GLES)
			if (UseOpenGL) {
				if (this->Province != -1) {
					int province_id = this->Province;
					if (GrandStrategyGame.Provinces[province_id]->Owner != NULL) {
						int player_color = PlayerRaces.Factions[GrandStrategyGame.Provinces[province_id]->Owner->Civilization][GrandStrategyGame.Provinces[province_id]->Owner->Faction]->Color;
						*(Uint32 *)&(GrandStrategyGame.MinimapSurfaceGL[(mx + my * GrandStrategyGame.MinimapTextureWidth) * 4]) = Video.MapRGB(TheScreen->format, PlayerColorsRGB[player_color][0]);
					} else if (GrandStrategyGame.Provinces[province_id]->Water) {
						*(Uint32 *)&(GrandStrategyGame.MinimapSurfaceGL[(mx + my * GrandStrategyGame.MinimapTextureWidth) * 4]) = Video.MapRGB(TheScreen->format, 171, 198, 217);
					} else {
						*(Uint32 *)&(GrandStrategyGame.MinimapSurfaceGL[(mx + my * GrandStrategyGame.MinimapTextureWidth) * 4]) = Video.MapRGB(TheScreen->format, 255, 255, 255);
					}
				} else {
					*(Uint32 *)&(GrandStrategyGame.MinimapSurfaceGL[(mx + my * GrandStrategyGame.MinimapTextureWidth) * 4]) = Video.MapRGB(0, 0, 0, 0);
				}
			} else
#endif
			{
				const int index = mx * bpp + my * GrandStrategyGame.MinimapSurface->pitch;
				if (this->Province != -1) {
					int province_id = this->Province;
					if (GrandStrategyGame.Provinces[province_id]->Owner != NULL) {
						int player_color = PlayerRaces.Factions[GrandStrategyGame.Provinces[province_id]->Owner->Civilization][GrandStrategyGame.Provinces[province_id]->Owner->Faction]->Color;
						if (bpp == 2) {
							*(Uint16 *)&((Uint8 *)GrandStrategyGame.MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, PlayerColorsRGB[player_color][0]);
						} else {
							*(Uint32 *)&((Uint8 *)GrandStrategyGame.MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, PlayerColorsRGB[player_color][0]);
						}
					} else if (GrandStrategyGame.Provinces[province_id]->Water) {
						if (bpp == 2) {
							*(Uint16 *)&((Uint8 *)GrandStrategyGame.MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, 171, 198, 217);
						} else {
							*(Uint32 *)&((Uint8 *)GrandStrategyGame.MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, 171, 198, 217);
						}
					} else {
						if (bpp == 2) {
							*(Uint16 *)&((Uint8 *)GrandStrategyGame.MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, 255, 255, 255);
						} else {
							*(Uint32 *)&((Uint8 *)GrandStrategyGame.MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, 255, 255, 255);
						}
					}
				} else {
					if (bpp == 2) {
						*(Uint16 *)&((Uint8 *)GrandStrategyGame.MinimapSurface->pixels)[index] = ColorBlack;
					} else {
						*(Uint32 *)&((Uint8 *)GrandStrategyGame.MinimapSurface->pixels)[index] = ColorBlack;
					}
				}
			}
		}
	}

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		SDL_UnlockSurface(GrandStrategyGame.MinimapSurface);
	}
}

void WorldMapTile::SetResourceProspected(int resource_id, bool discovered)
{
	if (this->ResourceProspected == discovered) { //no change, return
		return;
	}
	
	if (resource_id != -1 && this->Resource == resource_id) {
		this->ResourceProspected = discovered;
		
		if (this->Province != -1) {
			if (this->ResourceProspected) {
				GrandStrategyGame.Provinces[this->Province]->ProductionCapacity[resource_id] += 1;
				GrandStrategyGame.Provinces[this->Province]->AllocateLabor(); //allocate labor so that the 
			} else {
				GrandStrategyGame.Provinces[this->Province]->ProductionCapacity[resource_id] -= 1;
				GrandStrategyGame.Provinces[this->Province]->ReallocateLabor();
			}
		}
	}
}

/**
**  Get whether the tile has a resource
*/
bool WorldMapTile::HasResource(int resource, bool ignore_prospection)
{
	if (resource == this->Resource && (this->ResourceProspected || ignore_prospection)) {
		return true;
	}
	return false;
}

/**
**  Get the tile's cultural name.
*/
std::string WorldMapTile::GetCulturalName()
{
	if (this->Province != -1 && !GrandStrategyGame.Provinces[this->Province]->Water && GrandStrategyGame.Provinces[this->Province]->Civilization != -1 && !this->CulturalNames[GrandStrategyGame.Provinces[this->Province]->Civilization].empty()) {
		return this->CulturalNames[GrandStrategyGame.Provinces[this->Province]->Civilization];
	} else if (
		this->Province != -1
		&& GrandStrategyGame.Provinces[this->Province]->Water && GrandStrategyGame.Provinces[this->Province]->ReferenceProvince != -1
		&& GrandStrategyGame.Provinces[GrandStrategyGame.Provinces[this->Province]->ReferenceProvince]->Civilization != -1
		&& !this->CulturalNames[GrandStrategyGame.Provinces[GrandStrategyGame.Provinces[this->Province]->ReferenceProvince]->Civilization].empty()
	) {
		return this->CulturalNames[GrandStrategyGame.Provinces[GrandStrategyGame.Provinces[this->Province]->ReferenceProvince]->Civilization];
	} else {
		return this->Name;
	}
}

/**
**  Get a river's cultural name.
*/
std::string CRiver::GetCulturalName(int civilization)
{
	if (civilization != -1 && !this->CulturalNames[civilization].empty()) {
		return this->CulturalNames[civilization];
	} else {
		return this->Name;
	}
}

void CProvince::UpdateMinimap()
{
	for (int i = 0; i < ProvinceTileMax; ++i) {
		int x = this->Tiles[i].x;
		int y = this->Tiles[i].y;
		if (x == -1 || y == -1) {
			break;
		}
		if (GrandStrategyGame.WorldMapTiles[x][y]) {
			GrandStrategyGame.WorldMapTiles[x][y]->UpdateMinimap();
		}
	}
}

void CProvince::SetOwner(int civilization_id, int faction_id)
{
	if (this->Owner != NULL) { //if province has a previous owner, remove it from the owner's province list
		for (int i = 0; i < this->Owner->ProvinceCount; ++i) {
			if (this->Owner->OwnedProvinces[i] == this->ID) {
				//if this owned province is the one we are changing the owner of, push every element of the array after it back one step
				for (int j = i; j < this->Owner->ProvinceCount; ++j) {
					this->Owner->OwnedProvinces[j] = this->Owner->OwnedProvinces[j + 1];
				}
				break;
			}
		}
		this->Owner->ProvinceCount -= 1;
		
		//also remove its resource incomes from the owner's incomes, and reset the province's income so it won't be deduced from the new owner's income when recalculating it
		for (int i = 0; i < MaxCosts; ++i) {
			if (this->Income[i] != 0) {
				this->Owner->Income[i] -= this->Income[i];
				this->Income[i] = 0;
			}
		}
	}
	
	for (size_t i = 0; i < UnitTypes.size(); ++i) { //change the province's military score to be appropriate for the new faction's technologies
		if (IsMilitaryUnit(*UnitTypes[i])) {
			int old_owner_military_score_bonus = (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[i] : 0);
			int new_owner_military_score_bonus = (faction_id != -1 ? GrandStrategyGame.Factions[civilization_id][faction_id]->MilitaryScoreBonus[i] : 0);
			if (old_owner_military_score_bonus != new_owner_military_score_bonus) {
				this->MilitaryScore += this->Units[i] * (new_owner_military_score_bonus - old_owner_military_score_bonus);
				this->OffensiveMilitaryScore += this->Units[i] * new_owner_military_score_bonus - old_owner_military_score_bonus;
			}
		} else if (UnitTypes[i]->Class == "worker") {
			int militia_unit_type = PlayerRaces.GetCivilizationClassUnitType(PlayerRaces.GetRaceIndexByName(UnitTypes[i]->Civilization.c_str()), GetUnitTypeClassIndexByName("militia"));
			if (militia_unit_type != -1) {
				int old_owner_military_score_bonus = (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[militia_unit_type] : 0);
				int new_owner_military_score_bonus = (faction_id != -1 ? GrandStrategyGame.Factions[civilization_id][faction_id]->MilitaryScoreBonus[militia_unit_type] : 0);
				if (old_owner_military_score_bonus != new_owner_military_score_bonus) {
					this->MilitaryScore += this->Units[i] * ((new_owner_military_score_bonus - old_owner_military_score_bonus) / 2);
				}
			}
		}
	}

	if (civilization_id != -1 && faction_id != -1) {
		this->Owner = const_cast<CGrandStrategyFaction *>(&(*GrandStrategyGame.Factions[civilization_id][faction_id]));
		this->Owner->OwnedProvinces[this->Owner->ProvinceCount] = this->ID;
		this->Owner->ProvinceCount += 1;
		
		if (this->Civilization == -1) { // if province has no civilization/culture defined, then make its culture that of its owner
			this->SetCivilization(this->Owner->Civilization);
		}
	} else {
		this->Owner = NULL;
		this->SetCivilization(-1); //if there is no owner, change the civilization to none
	}
	
	if (this->Owner != NULL && this->Labor > 0 && this->HasBuildingClass("town-hall")) {
		this->AllocateLabor();
	}
	
	this->CalculateIncomes();
}

void CProvince::SetCivilization(int civilization)
{
	int old_civilization = this->Civilization;
	
	this->Civilization = civilization;
	
	if (civilization != -1) {
		// create new cultural names for the province's terrain features, if there aren't any
		for (int i = 0; i < ProvinceTileMax; ++i) {
			int x = this->Tiles[i].x;
			int y = this->Tiles[i].y;
			if (x == -1 || y == -1) {
				break;
			}
			if (GrandStrategyGame.WorldMapTiles[x][y]->CulturalNames[civilization].empty()) {
				std::string new_tile_name = "";
				// first see if can translate the cultural name of the old civilization
				if (old_civilization != -1 && !GrandStrategyGame.WorldMapTiles[x][y]->CulturalNames[old_civilization].empty()) {
					new_tile_name = PlayerRaces.TranslateName(GrandStrategyGame.WorldMapTiles[x][y]->CulturalNames[old_civilization], civilization);
				}
				if (new_tile_name == "") { // try to translate any cultural name
					for (int i = 0; i < MAX_RACES; ++i) {
						if (!GrandStrategyGame.WorldMapTiles[x][y]->CulturalNames[i].empty()) {
							new_tile_name = PlayerRaces.TranslateName(GrandStrategyGame.WorldMapTiles[x][y]->CulturalNames[i], civilization);
							if (!new_tile_name.empty()) {
								break;
							}
						}
					}
				}
				if (new_tile_name == "") { // if trying to translate all cultural names failed, generate a new name
					new_tile_name = this->GenerateTileName(civilization, GrandStrategyGame.WorldMapTiles[x][y]->Terrain);
				}
				if (new_tile_name != "") {
					GrandStrategyGame.WorldMapTiles[x][y]->CulturalNames[civilization] = new_tile_name;
				}
			}
		}
			
		// create a new cultural name for the province's settlement, if there isn't any
		if (this->CulturalSettlementNames[civilization].empty()) {
			std::string new_settlement_name = "";
			// first see if can translate the cultural name of the old civilization
			if (old_civilization != -1 && !this->CulturalSettlementNames[old_civilization].empty()) {
				new_settlement_name = PlayerRaces.TranslateName(this->CulturalSettlementNames[old_civilization], civilization);
			}
			if (new_settlement_name == "") { // try to translate any cultural name
				for (int i = 0; i < MAX_RACES; ++i) {
					if (!this->CulturalSettlementNames[i].empty()) {
						new_settlement_name = PlayerRaces.TranslateName(this->CulturalSettlementNames[i], civilization);
						if (!new_settlement_name.empty()) {
							break;
						}
					}
				}
			}
			if (new_settlement_name == "") { // if trying to translate all cultural names failed, generate a new name
				new_settlement_name = this->GenerateSettlementName(civilization);
			}
			if (new_settlement_name != "") {
				this->CulturalSettlementNames[civilization] = new_settlement_name;
			}
		}
			
		// create a new cultural name for the province, if there isn't any
		if (this->CulturalNames[civilization].empty()) {
			std::string new_province_name = "";
			// first see if can translate the cultural name of the old civilization
			if (old_civilization != -1 && !this->CulturalNames[old_civilization].empty()) {
				new_province_name = PlayerRaces.TranslateName(this->CulturalNames[old_civilization], civilization);
			}
			if (new_province_name == "") { // try to translate any cultural name
				for (int i = 0; i < MAX_RACES; ++i) {
					if (!this->CulturalNames[i].empty()) {
						new_province_name = PlayerRaces.TranslateName(this->CulturalNames[i], civilization);
						if (!new_province_name.empty()) {
							break;
						}
					}
				}
			}
			if (new_province_name == "") { // if trying to translate all cultural names failed, generate a new name
				new_province_name = this->GenerateProvinceName(civilization);
			}
			if (new_province_name != "") {
				this->CulturalNames[civilization] = new_province_name;
			}
		}
		
		for (size_t i = 0; i < UnitTypes.size(); ++i) {
			// replace existent buildings from other civilizations with buildings of the new civilization
			if (IsGrandStrategyBuilding(*UnitTypes[i]) && !UnitTypes[i]->Civilization.empty()) {
				if (this->SettlementBuildings[i] && PlayerRaces.GetCivilizationClassUnitType(civilization, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) != i) {
					this->SetSettlementBuilding(i, false); // remove building from other civilization
					if (PlayerRaces.GetCivilizationClassUnitType(civilization, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) != -1) {
						this->SetSettlementBuilding(PlayerRaces.GetCivilizationClassUnitType(civilization, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)), true);
					}
				}
			// replace existent units from the previous civilization with units of the new civilization
			} else if (
				IsGrandStrategyUnit(*UnitTypes[i])
				&& !UnitTypes[i]->Class.empty()
				&& !UnitTypes[i]->Civilization.empty()
				&& PlayerRaces.GetCivilizationClassUnitType(old_civilization, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) == i
				&& PlayerRaces.GetCivilizationClassUnitType(civilization, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) != -1
				&& PlayerRaces.GetCivilizationClassUnitType(civilization, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) != PlayerRaces.GetCivilizationClassUnitType(old_civilization, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) // don't replace if both civilizations use the same unit type
			) {
				this->ChangeUnitQuantity(PlayerRaces.GetCivilizationClassUnitType(civilization, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)), this->Units[i]);
				this->UnderConstructionUnits[PlayerRaces.GetCivilizationClassUnitType(civilization, GetUnitTypeClassIndexByName(UnitTypes[i]->Class))] += this->UnderConstructionUnits[i];
				this->SetUnitQuantity(i, 0);
				this->UnderConstructionUnits[i] = 0;
			}
		}
		
		if (old_civilization == -1 && this->TotalWorkers == 0) {
			//if the province had no culture set and thus has no worker, give it one worker
			this->SetUnitQuantity(PlayerRaces.GetCivilizationClassUnitType(civilization, GetUnitTypeClassIndexByName("worker")), 1);
		}
	} else {
		//if province is being set to no culture, remove all workers
		if (old_civilization != -1) {
			this->SetUnitQuantity(PlayerRaces.GetCivilizationClassUnitType(old_civilization, GetUnitTypeClassIndexByName("worker")), 0);
		}
	}
	
	this->CurrentConstruction = -1; // under construction buildings get canceled
	
	this->CalculateIncomes();
}

void CProvince::SetSettlementBuilding(int building_id, bool has_settlement_building)
{
	if (this->SettlementBuildings[building_id] == has_settlement_building) {
		return;
	}
	
	this->SettlementBuildings[building_id] = has_settlement_building;
	
	int change = has_settlement_building ? 1 : -1;
	for (int i = 0; i < MaxCosts; ++i) {
		if (UnitTypes[building_id]->GrandStrategyProductionEfficiencyModifier[i] != 0) {
			this->ProductionEfficiencyModifier[i] += UnitTypes[building_id]->GrandStrategyProductionEfficiencyModifier[i] * change;
			if (this->Owner != NULL) {
				this->CalculateIncome(i);
			}
		}
	}
	
	//recalculate the faction incomes if a town hall or a building that provides research was constructed
	if (this->Owner != NULL) {
		if (UnitTypes[building_id]->Class == "town-hall") {
			this->CalculateIncomes();
		} else if (UnitTypes[building_id]->Class == "lumber-mill") {
			this->CalculateIncome(ResearchCost);
		} else if (UnitTypes[building_id]->Class == "smithy") {
			this->CalculateIncome(ResearchCost);
		}
	}
	
	if (UnitTypes[building_id]->Class == "stronghold") { //increase the military score of the province, if this building is a stronghold
		this->MilitaryScore += (100 * 2) * change; // two guard towers if has a stronghold
	}

	// allocate labor (in case building a town hall or another building may have allowed a new sort of production)
	if (this->Owner != NULL && this->Labor > 0 && this->HasBuildingClass("town-hall")) {
		this->AllocateLabor();
	}
}

void CProvince::SetUnitQuantity(int unit_type_id, int quantity)
{
	quantity = std::max(0, quantity);
	
	int change = quantity - this->Units[unit_type_id];
	
	this->TotalUnits += change;
	
	if (IsMilitaryUnit(*UnitTypes[unit_type_id])) {
		this->MilitaryScore += change * (UnitTypes[unit_type_id]->DefaultStat.Variables[POINTS_INDEX].Value + (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[unit_type_id] : 0));
		this->OffensiveMilitaryScore += change * (UnitTypes[unit_type_id]->DefaultStat.Variables[POINTS_INDEX].Value + (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[unit_type_id] : 0));
	}
	
	if (UnitTypes[unit_type_id]->Class == "worker") {
		this->TotalWorkers += change;
		
		//if this unit's civilization can change workers into militia, add half of the militia's points to the military score (one in every two workers becomes a militia when the province is attacked)
		int militia_unit_type = PlayerRaces.GetCivilizationClassUnitType(PlayerRaces.GetRaceIndexByName(UnitTypes[unit_type_id]->Civilization.c_str()), GetUnitTypeClassIndexByName("militia"));
		if (militia_unit_type != -1) {
			this->MilitaryScore += change * ((UnitTypes[militia_unit_type]->DefaultStat.Variables[POINTS_INDEX].Value + (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[militia_unit_type] : 0)) / 2);
		}
		
		int labor_change = change * 100;
		if (labor_change >= 0) {
			this->Labor += labor_change;
			this->AllocateLabor();
		} else { //if workers are being removed from the province, reallocate labor
			this->ReallocateLabor();
		}		
	}
	
	this->Units[unit_type_id] = quantity;
}

void CProvince::ChangeUnitQuantity(int unit_type_id, int quantity)
{
	this->SetUnitQuantity(unit_type_id, this->Units[unit_type_id] + quantity);
}

void CProvince::SetAttackingUnitQuantity(int unit_type_id, int quantity)
{
	quantity = std::max(0, quantity);
	
	int change = quantity - this->AttackingUnits[unit_type_id];
	
	if (IsMilitaryUnit(*UnitTypes[unit_type_id])) {
		this->AttackingMilitaryScore += change * (UnitTypes[unit_type_id]->DefaultStat.Variables[POINTS_INDEX].Value + (this->AttackedBy != NULL ? this->AttackedBy->MilitaryScoreBonus[unit_type_id] : 0));
	}
		
	this->AttackingUnits[unit_type_id] = quantity;
}

void CProvince::ChangeAttackingUnitQuantity(int unit_type_id, int quantity)
{
	this->SetAttackingUnitQuantity(unit_type_id, this->AttackingUnits[unit_type_id] + quantity);
}

void CProvince::SetHero(int unit_type_id, int value)
{
	if (value == 1) {
		this->Movement = true;
	}
	bool found_hero = false;
	//update the hero
	for (size_t i = 0; i < GrandStrategyGame.Heroes.size(); ++i) {
		if (GrandStrategyGame.Heroes[i]->Type->Slot == UnitTypes[unit_type_id]->Slot) {
			if (GrandStrategyGame.Heroes[i]->Province != NULL) {
				if (GrandStrategyGame.Heroes[i]->State == 2) {
					GrandStrategyGame.Heroes[i]->Province->MilitaryScore -= (UnitTypes[unit_type_id]->DefaultStat.Variables[POINTS_INDEX].Value + (GrandStrategyGame.Heroes[i]->Province->Owner != NULL ? GrandStrategyGame.Heroes[i]->Province->Owner->MilitaryScoreBonus[unit_type_id] : 0));
				} else if (GrandStrategyGame.Heroes[i]->State == 3) {
					GrandStrategyGame.Heroes[i]->Province->AttackingMilitaryScore -= (UnitTypes[unit_type_id]->DefaultStat.Variables[POINTS_INDEX].Value + (GrandStrategyGame.Heroes[i]->Province->AttackedBy != NULL ? GrandStrategyGame.Heroes[i]->Province->AttackedBy->MilitaryScoreBonus[unit_type_id] : 0));
				}
			}
			GrandStrategyGame.Heroes[i]->State = value;
			
			if (this != GrandStrategyGame.Heroes[i]->Province || value == 0) { //if the new province is different from the hero's current province
				if (GrandStrategyGame.Heroes[i]->Province != NULL) {
					GrandStrategyGame.Heroes[i]->Province->Heroes.erase(std::remove(GrandStrategyGame.Heroes[i]->Province->Heroes.begin(), GrandStrategyGame.Heroes[i]->Province->Heroes.end(), GrandStrategyGame.Heroes[i]), GrandStrategyGame.Heroes[i]->Province->Heroes.end());  //remove the hero from the last province
				}
				GrandStrategyGame.Heroes[i]->Province = value != 0 ? const_cast<CProvince *>(&(*this)) : NULL;
				if (GrandStrategyGame.Heroes[i]->Province != NULL) {
					GrandStrategyGame.Heroes[i]->Province->Heroes.push_back(GrandStrategyGame.Heroes[i]); //add the hero to the new province
				}
			}
			
			found_hero = true;
			break;
		}
	}
	
	//if the hero hasn't been defined yet, do so now
	if (found_hero == false) {
		CGrandStrategyHero *hero = new CGrandStrategyHero;
		hero->State = value;
		hero->Province = value != 0 ? const_cast<CProvince *>(&(*this)) : NULL;
		hero->Type = const_cast<CUnitType *>(&(*UnitTypes[unit_type_id]));
		GrandStrategyGame.Heroes.push_back(hero);
		if (hero->Province != NULL) {
			this->Heroes.push_back(hero);
		}
	}
	
	if (value == 2) {
		this->MilitaryScore += (UnitTypes[unit_type_id]->DefaultStat.Variables[POINTS_INDEX].Value + (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[unit_type_id] : 0));
	} else if (value == 3) {
		this->AttackingMilitaryScore += (UnitTypes[unit_type_id]->DefaultStat.Variables[POINTS_INDEX].Value + (this->AttackedBy != NULL ? this->AttackedBy->MilitaryScoreBonus[unit_type_id] : 0));
	}
}
		
void CProvince::AllocateLabor()
{
	if (this->Owner == NULL || !this->HasBuildingClass("town-hall")) { //no production if no town hall is in place, or if the province has no owner
		return;
	}
	
	if (this->Labor == 0) { //if there's no labor, nothing to allocate
		return;
	}
	
	//first, try to allocate as many workers as possible in food production, to increase the population, then allocate workers to gold, and then to other goods
//	std::vector<int> resources_by_priority = {GrainCost, MushroomCost, FishCost, GoldCost, WoodCost, StoneCost};
	std::vector<int> resources_by_priority;
	resources_by_priority.push_back(GrainCost);
	resources_by_priority.push_back(MushroomCost);
	resources_by_priority.push_back(FishCost);
	resources_by_priority.push_back(GoldCost);
	resources_by_priority.push_back(WoodCost);
	resources_by_priority.push_back(StoneCost);

	for (size_t i = 0; i < resources_by_priority.size(); ++i) {
		this->AllocateLaborToResource(resources_by_priority[i]);
		if (this->Labor == 0) { //labor depleted
			return;
		}
	}
}

void CProvince::AllocateLaborToResource(int resource)
{
	if (this->Owner == NULL || !this->HasBuildingClass("town-hall")) { //no production if no town hall is in place, or if the province has no owner
		return;
	}
	
	if (this->Labor == 0) { //if there's no labor, nothing to allocate
		return;
	}
	
	if (this->ProductionCapacity[resource] > this->ProductionCapacityFulfilled[resource] && this->Labor >= DefaultResourceLaborInputs[resource]) {
		int employment_change = std::min(this->Labor / DefaultResourceLaborInputs[resource], this->ProductionCapacity[resource] - ProductionCapacityFulfilled[resource]);
		this->Labor -= employment_change * DefaultResourceLaborInputs[resource];
		ProductionCapacityFulfilled[resource] += employment_change;
		this->CalculateIncome(resource);
	}
	
	//recalculate food consumption (workers employed in producing food don't need to consume food)
	FoodConsumption = this->TotalWorkers * 100;
	FoodConsumption -= (this->ProductionCapacityFulfilled[GrainCost] * DefaultResourceLaborInputs[GrainCost]);
	FoodConsumption -= (this->ProductionCapacityFulfilled[MushroomCost] * DefaultResourceLaborInputs[MushroomCost]);
	FoodConsumption -= (this->ProductionCapacityFulfilled[FishCost] * DefaultResourceLaborInputs[FishCost]);
}

void CProvince::ReallocateLabor()
{
	for (int i = 0; i < MaxCosts; ++i) {
		this->ProductionCapacityFulfilled[i] = 0;
	}
	this->Labor = this->TotalWorkers * 100;
	this->AllocateLabor();
}

void CProvince::CalculateIncome(int resource)
{
	if (resource == -1) {
		return;
	}
	
	if (this->Owner == NULL || !this->HasBuildingClass("town-hall")) { //don't produce resources if no town hall is in place
		this->Income[resource] = 0;
		return;
	}
	
	this->Owner->Income[resource] -= this->Income[resource]; //first, remove the old income from the owner's income
	
	int income = 0;
	
	if (resource == ResearchCost) {
		// faction's research is 10 if all provinces have town halls, lumber mills and smithies
		if (this->HasBuildingClass("town-hall")) {
			income += 6;
		}
		if (this->HasBuildingClass("lumber-mill")) {
			income += 2;
		}
		if (this->HasBuildingClass("smithy")) {
			income += 2;
		}
			
		income *= 100 + this->Owner->ProductionEfficiencyModifier[resource] + this->ProductionEfficiencyModifier[resource] + this->GetAdministrativeEfficiencyModifier();
		income /= 100;
	} else {
		if (this->ProductionCapacityFulfilled[resource] > 0) {
			income = DefaultResourceOutputs[resource] * this->ProductionCapacityFulfilled[resource];
			int production_modifier = this->Owner->ProductionEfficiencyModifier[resource] + this->ProductionEfficiencyModifier[resource];
			if (resource != GrainCost && resource != MushroomCost && resource != FishCost) { //food resources don't lose production efficiency if administrative efficiency is lower than 100%, to prevent provinces from starving when conquered
				production_modifier += this->GetAdministrativeEfficiencyModifier();
			}
			income *= 100 + production_modifier;
			income /= 100;
		}
	}
	
	this->Income[resource] = income;
	
	this->Owner->Income[resource] += this->Income[resource]; //add the new income to the owner's income
}

void CProvince::CalculateIncomes()
{
	for (int i = 0; i < MaxCosts; ++i) {
		this->CalculateIncome(i);
	}
}

void CProvince::AddFactionClaim(int civilization_id, int faction_id)
{
	this->Claims[this->ClaimCount][0] = civilization_id;
	this->Claims[this->ClaimCount][1] = faction_id;
	this->ClaimCount += 1;
}

void CProvince::RemoveFactionClaim(int civilization_id, int faction_id)
{
	for (int i = 0; i < this->ClaimCount; ++i) {
		if (this->Claims[i][0] == civilization_id && this->Claims[i][1] == faction_id) {
			//if this claim's faction is the one we are removing, push every element of the array after it back one step
			for (int j = i; j < this->ClaimCount; ++j) {
				this->Claims[j][0] = this->Claims[j + 1][0];
				this->Claims[j][1] = this->Claims[j + 1][1];
			}
			break;
		}
	}
	this->ClaimCount -= 1;
}

bool CProvince::HasBuildingClass(std::string building_class_name)
{
	if (this->Civilization == -1 || building_class_name.empty()) {
		return false;
	}
	
	int building_type = PlayerRaces.GetCivilizationClassUnitType(this->Civilization, GetUnitTypeClassIndexByName(building_class_name));
	
	if (building_type == -1 && building_class_name == "mercenary-camp") { //special case for mercenary camps, which are a neutral building
		building_type = UnitTypeIdByIdent("unit-mercenary-camp");
	}
	
	if (building_type != -1 && this->SettlementBuildings[building_type] == true) {
		return true;
	}

	return false;
}

bool CProvince::HasFactionClaim(int civilization_id, int faction_id)
{
	for (int i = 0; i < this->ClaimCount; ++i) {
		if (this->Claims[i][0] == civilization_id && this->Claims[i][1] == faction_id) {
			return true;
		}
	}
	return false;
}

bool CProvince::HasResource(int resource, bool ignore_prospection)
{
	for (int i = 0; i < ProvinceTileMax; ++i) {
		int x = this->Tiles[i].x;
		int y = this->Tiles[i].y;
		if (x == -1 || y == -1) {
			break;
		}
		if (GrandStrategyGame.WorldMapTiles[x][y] && GrandStrategyGame.WorldMapTiles[x][y]->HasResource(resource, ignore_prospection)) {
			return true;
		}
	}
	return false;
}

bool CProvince::BordersProvince(int province_id)
{
	for (int i = 0; i < ProvinceMax; ++i) {
		if (this->BorderProvinces[i] != -1) {
			if (this->BorderProvinces[i] == province_id) {
				return true;
			}
		} else {
			break;
		}
	}
	return false;
}

bool CProvince::BordersFaction(int faction_civilization, int faction)
{
	for (int i = 0; i < ProvinceMax; ++i) {
		if (this->BorderProvinces[i] != -1) {
			if (GrandStrategyGame.Provinces[this->BorderProvinces[i]]->Owner == NULL) {
				continue;
			}
			if (GrandStrategyGame.Provinces[this->BorderProvinces[i]]->Owner->Civilization == faction_civilization && GrandStrategyGame.Provinces[this->BorderProvinces[i]]->Owner->Faction == faction) {
				return true;
			}
		} else {
			break;
		}
	}
	return false;
}

int CProvince::GetPopulation()
{
	return (this->TotalWorkers * 10000) * 2;
}

int CProvince::GetResourceDemand(int resource)
{
	int quantity = 0;
	if (resource == WoodCost) {
		quantity = 50;
		if (this->HasBuildingClass("lumber-mill")) {
			quantity += 50; // increase the province's lumber demand if it has a lumber mill built
		}
	} else if (resource == StoneCost) {
		quantity = 25;
	}
	
	if (quantity > 0 && GrandStrategyGame.CommodityPrices[resource] > 0) {
		quantity *= DefaultResourcePrices[resource];
		quantity /= GrandStrategyGame.CommodityPrices[resource];
	}

	return quantity;
}

int CProvince::GetAdministrativeEfficiencyModifier()
{
	int modifier = 0;
	
	if (this->Civilization != -1 && this->Owner != NULL) {
		modifier += this->Civilization == this->Owner->Civilization ? 0 : -25; //if the province is of a different culture than its owner, it gets a cultural penalty to its administrative efficiency modifier
	}
	
	return modifier;
}

int CProvince::GetRevoltRisk()
{
	int revolt_risk = 0;
	
	if (this->Civilization != -1 && this->Owner != NULL) {
		if (this->Civilization != this->Owner->Civilization) {
			revolt_risk += 3; //if the province is of a different culture than its owner, it gets plus 3% revolt risk
		}
		
		if (!this->HasFactionClaim(this->Owner->Civilization, this->Owner->Faction)) {
			revolt_risk += 2; //if the owner does not have a claim to the province, it gets plus 2% revolt risk
		}
	}
	
	return revolt_risk;
}

/**
**  Get the province's cultural name.
*/
std::string CProvince::GetCulturalName()
{
	if (this->Owner != NULL && !this->Water && !this->FactionCulturalNames[this->Owner->Civilization][this->Owner->Faction].empty() && this->Civilization == this->Owner->Civilization) {
		return this->FactionCulturalNames[this->Owner->Civilization][this->Owner->Faction];
	} else if (!this->Water && this->Civilization != -1 && !this->CulturalNames[this->Civilization].empty()) {
		return this->CulturalNames[this->Civilization];
	} else if (
		this->Water && this->ReferenceProvince != -1
		&& GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner != NULL
		&& !GrandStrategyGame.Provinces[this->ReferenceProvince]->Water
		&& !this->FactionCulturalNames[GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner->Civilization][GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner->Faction].empty()
		&& GrandStrategyGame.Provinces[this->ReferenceProvince]->Civilization == GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner->Civilization
	) {
		return this->FactionCulturalNames[GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner->Civilization][GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner->Faction];
	} else if (
		this->Water && this->ReferenceProvince != -1
		&& GrandStrategyGame.Provinces[this->ReferenceProvince]->Civilization != -1
		&& !this->CulturalNames[GrandStrategyGame.Provinces[this->ReferenceProvince]->Civilization].empty()
	) {
		return this->CulturalNames[GrandStrategyGame.Provinces[this->ReferenceProvince]->Civilization];
	} else {
		return this->Name;
	}
}

/**
**  Get the province's cultural settlement name.
*/
std::string CProvince::GetCulturalSettlementName()
{
	if (!this->Water && this->Owner != NULL && !this->FactionCulturalSettlementNames[this->Owner->Civilization][this->Owner->Faction].empty() && this->Civilization == this->Owner->Civilization) {
		return this->FactionCulturalSettlementNames[this->Owner->Civilization][this->Owner->Faction];
	} else if (!this->Water && this->Civilization != -1 && !this->CulturalSettlementNames[this->Civilization].empty()) {
		return this->CulturalSettlementNames[this->Civilization];
	} else {
		return this->SettlementName;
	}
}

/**
**  Generate a province name for the civilization.
*/
std::string CProvince::GenerateProvinceName(int civilization)
{
	std::string province_name;

	//10% chance that the province will be named after its settlement
	if (civilization != -1 && !this->CulturalSettlementNames[civilization].empty() && SyncRand(100) < 10) {
		return this->CulturalSettlementNames[civilization];
	}
	
	if (
		!PlayerRaces.ProvinceNames[civilization][0].empty()
		|| !PlayerRaces.ProvinceNamePrefixes[civilization][0].empty()
		|| PlayerRaces.LanguageNouns[civilization][0]
		|| PlayerRaces.LanguageVerbs[civilization][0]
		|| PlayerRaces.LanguageAdjectives[civilization][0]
	) {
		int ProvinceNameCount = 0;
		std::string ProvinceNames[PersonalNameMax];
		int ProvinceNamePrefixCount = 0;
		std::string ProvinceNamePrefixes[PersonalNameMax];
		int ProvinceNameSuffixCount = 0;
		std::string ProvinceNameSuffixes[PersonalNameMax];
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.ProvinceNames[civilization][i].empty()) {
				break;
			}
			ProvinceNames[ProvinceNameCount] = PlayerRaces.ProvinceNames[civilization][i];
			ProvinceNameCount += 1;
		}
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.ProvinceNamePrefixes[civilization][i].empty()) {
				break;
			}
			ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.ProvinceNamePrefixes[civilization][i];
			ProvinceNamePrefixCount += 1;
		}
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.ProvinceNameSuffixes[civilization][i].empty()) {
				break;
			}
			ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.ProvinceNameSuffixes[civilization][i];
			ProvinceNameSuffixCount += 1;
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageNouns[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->ProvinceName) { // nouns which can be used as province names without compounding
				if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->NameSingular) {
					ProvinceNames[ProvinceNameCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
					ProvinceNameCount += 1;
				}
				if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->NamePlural) {
					ProvinceNames[ProvinceNameCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
					ProvinceNameCount += 1;
				}
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->PrefixProvinceName) {
				if (PlayerRaces.LanguageNouns[civilization][i]->Uncountable) { // if is uncountable, use the nominative instead of the genitive
					if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixSingular) {
						ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
						ProvinceNamePrefixCount += 1;
					}
					if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixPlural) {
						ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
						ProvinceNamePrefixCount += 1;
					}
				} else {
					if (PlayerRaces.LanguageNouns[civilization][i]->PrefixSingular) {
						if (!PlayerRaces.LanguageNouns[civilization][i]->SingularGenitive.empty()) {
							ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularGenitive;
							ProvinceNamePrefixCount += 1;
						} else if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty()) { //if no genitive is present, use the nominative instead
							ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
							ProvinceNamePrefixCount += 1;
						}
					}
					if (PlayerRaces.LanguageNouns[civilization][i]->PrefixPlural) {
						if (!PlayerRaces.LanguageNouns[civilization][i]->PluralGenitive.empty()) {
							ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralGenitive;
							ProvinceNamePrefixCount += 1;
						} else if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty()) { //if no genitive is present, use the nominative instead
							ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
							ProvinceNamePrefixCount += 1;
						}
					}
				}
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->SuffixProvinceName) {
				if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->SuffixSingular) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
					ProvinceNameSuffixCount += 1;
				}
				if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->SuffixPlural) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
					ProvinceNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageVerbs[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageVerbs[civilization][i]->PrefixProvinceName) { // only using verb participles for now; maybe should add more possibilities?
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent.empty()) {
					ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent;
					ProvinceNamePrefixCount += 1;
				}
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast.empty()) {
					ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast;
					ProvinceNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageVerbs[civilization][i]->SuffixProvinceName) {
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent.empty()) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent;
					ProvinceNameSuffixCount += 1;
				}
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast.empty()) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast;
					ProvinceNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageAdjectives[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->ProvinceName) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					ProvinceNames[ProvinceNameCount] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					ProvinceNameCount += 1;
				}
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->PrefixProvinceName) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					ProvinceNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->SuffixProvinceName) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					ProvinceNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageNumerals[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageNumerals[civilization][i]->PrefixProvinceName) {
				if (!PlayerRaces.LanguageNumerals[civilization][i]->Word.empty()) {
					ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageNumerals[civilization][i]->Word;
					ProvinceNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageNumerals[civilization][i]->SuffixProvinceName) {
				if (!PlayerRaces.LanguageNumerals[civilization][i]->Word.empty()) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageNumerals[civilization][i]->Word;
					ProvinceNameSuffixCount += 1;
				}
			}
		}
		
		if (ProvinceNameCount > 0 || ProvinceNamePrefixCount > 0 || ProvinceNameSuffixCount > 0) {
			int ProvinceNameProbability = ProvinceNameCount * 10000 / (ProvinceNameCount + (ProvinceNamePrefixCount * ProvinceNameSuffixCount));
			if (SyncRand(10000) < ProvinceNameProbability) {
				province_name = ProvinceNames[SyncRand(ProvinceNameCount)];
			} else {
				std::string prefix = ProvinceNamePrefixes[SyncRand(ProvinceNamePrefixCount)];
				std::string suffix = ProvinceNameSuffixes[SyncRand(ProvinceNameSuffixCount)];
				
				if (PlayerRaces.RequiresPlural(prefix, civilization)) {
					suffix = PlayerRaces.GetPluralForm(suffix, civilization);
				}
				
				suffix = DecapitalizeString(suffix);
				if (prefix.substr(prefix.size() - 2, 2) == "gs" && suffix.substr(0, 1) == "g") { //if the last two characters of the prefix are "gs", and the first character of the suffix is "g", then remove the final "s" from the prefix (as in "Königgrätz")
					prefix = FindAndReplaceStringEnding(prefix, "gs", "g");
				}
				if (prefix.substr(prefix.size() - 1, 1) == "s" && suffix.substr(0, 1) == "s") { //if the prefix ends in "s" and the suffix begins in "s" as well, then remove the final "s" from the prefix (as in "Josefstadt", "Kronstadt" and "Leopoldstadt")
					prefix = FindAndReplaceStringEnding(prefix, "s", "");
				}
				province_name = prefix;
				province_name += suffix;
			}
		}
	}
	
	province_name = TransliterateText(province_name);
	
	return province_name;
}

/**
**  Generate a settlement name for the civilization.
**
**  @param l  Lua state.
*/
std::string CProvince::GenerateSettlementName(int civilization)
{
	std::string settlement_name;
	
	//10% chance that the settlement will be named after a named terrain feature in its tile, if there is any
	if (civilization != -1 && this->SettlementLocation.x != -1 && this->SettlementLocation.y != -1 && !GrandStrategyGame.WorldMapTiles[this->SettlementLocation.x][this->SettlementLocation.y]->CulturalNames[civilization].empty() && SyncRand(100) < 10) {
		return GrandStrategyGame.WorldMapTiles[this->SettlementLocation.x][this->SettlementLocation.y]->CulturalNames[civilization];
	}
	
	if (
		!PlayerRaces.SettlementNames[civilization][0].empty()
		|| !PlayerRaces.SettlementNamePrefixes[civilization][0].empty()
		|| PlayerRaces.LanguageNouns[civilization][0]
		|| PlayerRaces.LanguageVerbs[civilization][0]
		|| PlayerRaces.LanguageAdjectives[civilization][0]
	) {
		int SettlementNameCount = 0;
		std::string SettlementNames[PersonalNameMax];
		int SettlementNamePrefixCount = 0;
		std::string SettlementNamePrefixes[PersonalNameMax];
		int SettlementNameSuffixCount = 0;
		std::string SettlementNameSuffixes[PersonalNameMax];
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.SettlementNames[civilization][i].empty()) {
				break;
			}
			SettlementNames[SettlementNameCount] = PlayerRaces.SettlementNames[civilization][i];
			SettlementNameCount += 1;
		}
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.SettlementNamePrefixes[civilization][i].empty()) {
				break;
			}
			SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.SettlementNamePrefixes[civilization][i];
			SettlementNamePrefixCount += 1;
		}
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.SettlementNameSuffixes[civilization][i].empty()) {
				break;
			}
			SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.SettlementNameSuffixes[civilization][i];
			SettlementNameSuffixCount += 1;
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageNouns[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->SettlementName) { // nouns which can be used as settlement names without compounding
				if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->NameSingular) {
					SettlementNames[SettlementNameCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
					SettlementNameCount += 1;
				}
				if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->NamePlural) {
					SettlementNames[SettlementNameCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
					SettlementNameCount += 1;
				}
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->PrefixSettlementName) {
				if (PlayerRaces.LanguageNouns[civilization][i]->Uncountable) { // if is uncountable, use the nominative instead of the genitive
					if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixSingular) {
						SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
						SettlementNamePrefixCount += 1;
					}
					if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixPlural) {
						SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
						SettlementNamePrefixCount += 1;
					}
				} else {
					if (PlayerRaces.LanguageNouns[civilization][i]->PrefixSingular) {
						if (!PlayerRaces.LanguageNouns[civilization][i]->SingularGenitive.empty()) {
							SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularGenitive;
							SettlementNamePrefixCount += 1;
						} else if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty()) { //if no genitive is present, use the nominative instead
							SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
							SettlementNamePrefixCount += 1;
						}
					}
					if (PlayerRaces.LanguageNouns[civilization][i]->PrefixPlural) {
						if (!PlayerRaces.LanguageNouns[civilization][i]->PluralGenitive.empty()) {
							SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralGenitive;
							SettlementNamePrefixCount += 1;
						} else if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty()) { //if no genitive is present, use the nominative instead
							SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
							SettlementNamePrefixCount += 1;
						}
					}
				}
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->SuffixSettlementName) {
				if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->SuffixSingular) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
					SettlementNameSuffixCount += 1;
				}
				if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->SuffixPlural) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
					SettlementNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageVerbs[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageVerbs[civilization][i]->PrefixSettlementName) { // only using verb participles for now; maybe should add more possibilities?
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent.empty()) {
					SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent;
					SettlementNamePrefixCount += 1;
				}
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast.empty()) {
					SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast;
					SettlementNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageVerbs[civilization][i]->SuffixSettlementName) {
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent.empty()) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent;
					SettlementNameSuffixCount += 1;
				}
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast.empty()) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast;
					SettlementNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageAdjectives[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->SettlementName) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					SettlementNames[SettlementNameCount] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					SettlementNameCount += 1;
				}
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->PrefixSettlementName) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					SettlementNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->SuffixSettlementName) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					SettlementNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageNumerals[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageNumerals[civilization][i]->PrefixSettlementName) {
				if (!PlayerRaces.LanguageNumerals[civilization][i]->Word.empty()) {
					SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageNumerals[civilization][i]->Word;
					SettlementNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageNumerals[civilization][i]->SuffixSettlementName) {
				if (!PlayerRaces.LanguageNumerals[civilization][i]->Word.empty()) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageNumerals[civilization][i]->Word;
					SettlementNameSuffixCount += 1;
				}
			}
		}
		
		if (SettlementNameCount > 0 || SettlementNamePrefixCount > 0 || SettlementNameSuffixCount > 0) {
			int SettlementNameProbability = SettlementNameCount * 10000 / (SettlementNameCount + (SettlementNamePrefixCount * SettlementNameSuffixCount));
			if (SyncRand(10000) < SettlementNameProbability) {
				settlement_name = SettlementNames[SyncRand(SettlementNameCount)];
			} else {
				std::string prefix = SettlementNamePrefixes[SyncRand(SettlementNamePrefixCount)];
				std::string suffix = SettlementNameSuffixes[SyncRand(SettlementNameSuffixCount)];
				
				if (PlayerRaces.RequiresPlural(prefix, civilization)) {
					suffix = PlayerRaces.GetPluralForm(suffix, civilization);
				}
				
				suffix = DecapitalizeString(suffix);
				if (prefix.substr(prefix.size() - 2, 2) == "gs" && suffix.substr(0, 1) == "g") { //if the last two characters of the prefix are "gs", and the first character of the suffix is "g", then remove the final "s" from the prefix (as in "Königgrätz")
					prefix = FindAndReplaceStringEnding(prefix, "gs", "g");
				}
				if (prefix.substr(prefix.size() - 1, 1) == "s" && suffix.substr(0, 1) == "s") { //if the prefix ends in "s" and the suffix begins in "s" as well, then remove the final "s" from the prefix (as in "Josefstadt", "Kronstadt" and "Leopoldstadt")
					prefix = FindAndReplaceStringEnding(prefix, "s", "");
				}
				settlement_name = prefix;
				settlement_name += suffix;
			}
		}
	}
	
	settlement_name = TransliterateText(settlement_name);
	
	return settlement_name;
}

/**
**  Generate a settlement name for the civilization.
**
**  @param l  Lua state.
*/
std::string CProvince::GenerateTileName(int civilization, int terrain)
{
	std::string tile_name;
	
	if (
		PlayerRaces.LanguageNouns[civilization][0]
		|| PlayerRaces.LanguageVerbs[civilization][0]
		|| PlayerRaces.LanguageAdjectives[civilization][0]
		|| PlayerRaces.LanguageNumerals[civilization][0]
	) {
		int noun_name_count = 0;
		std::string noun_names[PersonalNameMax];
		int noun_name_ids[PersonalNameMax];
		int adjective_name_count = 0;
		std::string adjective_names[PersonalNameMax];
		int adjective_name_ids[PersonalNameMax];
		
		int noun_prefix_count = 0;
		std::string noun_prefixes[PersonalNameMax];
		int noun_prefix_ids[PersonalNameMax];
		int verb_prefix_count = 0;
		std::string verb_prefixes[PersonalNameMax];
		int verb_prefix_ids[PersonalNameMax];
		int adjective_prefix_count = 0;
		std::string adjective_prefixes[PersonalNameMax];
		int adjective_prefix_ids[PersonalNameMax];
		int numeral_prefix_count = 0;
		std::string numeral_prefixes[PersonalNameMax];
		int numeral_prefix_ids[PersonalNameMax];

		int noun_infix_count = 0;
		std::string noun_infixes[PersonalNameMax];
		int noun_infix_ids[PersonalNameMax];
		int verb_infix_count = 0;
		std::string verb_infixes[PersonalNameMax];
		int verb_infix_ids[PersonalNameMax];
		int adjective_infix_count = 0;
		std::string adjective_infixes[PersonalNameMax];
		int adjective_infix_ids[PersonalNameMax];
		int numeral_infix_count = 0;
		std::string numeral_infixes[PersonalNameMax];
		int numeral_infix_ids[PersonalNameMax];

		int noun_suffix_count = 0;
		std::string noun_suffixes[PersonalNameMax];
		int noun_suffix_ids[PersonalNameMax];
		int verb_suffix_count = 0;
		std::string verb_suffixes[PersonalNameMax];
		int verb_suffix_ids[PersonalNameMax];
		int adjective_suffix_count = 0;
		std::string adjective_suffixes[PersonalNameMax];
		int adjective_suffix_ids[PersonalNameMax];
		int numeral_suffix_count = 0;
		std::string numeral_suffixes[PersonalNameMax];
		int numeral_suffix_ids[PersonalNameMax];
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageNouns[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->TerrainName[terrain]) { // nouns which can be used as terrain names for this terrain type without compounding
				if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->NameSingular) {
					noun_names[noun_name_count] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
					noun_name_ids[noun_name_count] = i;
					noun_name_count += 1;
				}
				if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->NamePlural) {
					noun_names[noun_name_count] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
					noun_name_ids[noun_name_count] = i;
					noun_name_count += 1;
				}
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->PrefixTerrainName[terrain]) {
				if (PlayerRaces.LanguageNouns[civilization][i]->Uncountable) { // if is uncountable, use the nominative instead of the genitive
					if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixSingular) {
						noun_prefixes[noun_prefix_count] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
						noun_prefix_ids[noun_prefix_count] = i;
						noun_prefix_count += 1;
					}
					if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixPlural) {
						noun_prefixes[noun_prefix_count] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
						noun_prefix_count += 1;
					}
				} else {
					if (PlayerRaces.LanguageNouns[civilization][i]->PrefixSingular) {
						if (!PlayerRaces.LanguageNouns[civilization][i]->SingularGenitive.empty()) {
							noun_prefixes[noun_prefix_count] = PlayerRaces.LanguageNouns[civilization][i]->SingularGenitive;
							noun_prefix_ids[noun_prefix_count] = i;
							noun_prefix_count += 1;
						} else if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty()) { //if no genitive is present, use the nominative instead
							noun_prefixes[noun_prefix_count] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
							noun_prefix_ids[noun_prefix_count] = i;
							noun_prefix_count += 1;
						}
					}
					if (PlayerRaces.LanguageNouns[civilization][i]->PrefixPlural) {
						if (!PlayerRaces.LanguageNouns[civilization][i]->PluralGenitive.empty()) {
							noun_prefixes[noun_prefix_count] = PlayerRaces.LanguageNouns[civilization][i]->PluralGenitive;
							noun_prefix_ids[noun_prefix_count] = i;
							noun_prefix_count += 1;
						} else if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty()) { //if no genitive is present, use the nominative instead
							noun_prefixes[noun_prefix_count] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
							noun_prefix_ids[noun_prefix_count] = i;
							noun_prefix_count += 1;
						}
					}
				}
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->SuffixTerrainName[terrain]) {
				if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->SuffixSingular) {
					noun_suffixes[noun_suffix_count] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
					noun_suffix_ids[noun_suffix_count] = i;
					noun_suffix_count += 1;
				}
				if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->SuffixPlural) {
					noun_suffixes[noun_suffix_count] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
					noun_suffix_ids[noun_suffix_count] = i;
					noun_suffix_count += 1;
				}
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->InfixTerrainName[terrain]) {
				if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->InfixSingular) {
					noun_infixes[noun_infix_count] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
					noun_infix_ids[noun_infix_count] = i;
					noun_infix_count += 1;
				}
				if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->InfixPlural) {
					noun_infixes[noun_infix_count] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
					noun_infix_ids[noun_infix_count] = i;
					noun_infix_count += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageVerbs[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageVerbs[civilization][i]->PrefixTerrainName[terrain]) { // only using verb participles for now; maybe should add more possibilities?
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent.empty()) {
					verb_prefixes[verb_prefix_count] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent;
					verb_prefix_ids[verb_prefix_count] = i;
					verb_prefix_count += 1;
				}
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast.empty()) {
					verb_prefixes[verb_prefix_count] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast;
					verb_prefix_ids[verb_prefix_count] = i;
					verb_prefix_count += 1;
				}
			}
			if (PlayerRaces.LanguageVerbs[civilization][i]->SuffixTerrainName[terrain]) {
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent.empty()) {
					verb_suffixes[verb_suffix_count] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent;
					verb_suffix_ids[verb_suffix_count] = i;
					verb_suffix_count += 1;
				}
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast.empty()) {
					verb_suffixes[verb_suffix_count] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast;
					verb_suffix_ids[verb_suffix_count] = i;
					verb_suffix_count += 1;
				}
			}
			if (PlayerRaces.LanguageVerbs[civilization][i]->InfixTerrainName[terrain]) {
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent.empty()) {
					verb_infixes[verb_infix_count] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent;
					verb_infix_ids[verb_infix_count] = i;
					verb_infix_count += 1;
				}
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast.empty()) {
					verb_infixes[verb_infix_count] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast;
					verb_infix_ids[verb_infix_count] = i;
					verb_infix_count += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageAdjectives[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->TerrainName[terrain]) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					adjective_names[adjective_name_count] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					adjective_name_ids[adjective_name_count] = i;
					adjective_name_count += 1;
				}
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->PrefixTerrainName[terrain]) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					adjective_prefixes[adjective_prefix_count] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					adjective_prefix_ids[adjective_prefix_count] = i;
					adjective_prefix_count += 1;
				}
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->SuffixTerrainName[terrain]) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					adjective_suffixes[adjective_suffix_count] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					adjective_suffix_ids[adjective_suffix_count] = i;
					adjective_suffix_count += 1;
				}
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->InfixTerrainName[terrain]) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					adjective_infixes[adjective_infix_count] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					adjective_infix_ids[adjective_infix_count] = i;
					adjective_infix_count += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageNumerals[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageNumerals[civilization][i]->PrefixTerrainName[terrain]) {
				if (!PlayerRaces.LanguageNumerals[civilization][i]->Word.empty()) {
					numeral_prefixes[numeral_prefix_count] = PlayerRaces.LanguageNumerals[civilization][i]->Word;
					numeral_prefix_ids[numeral_prefix_count] = i;
					numeral_prefix_count += 1;
				}
			}
			if (PlayerRaces.LanguageNumerals[civilization][i]->SuffixTerrainName[terrain]) {
				if (!PlayerRaces.LanguageNumerals[civilization][i]->Word.empty()) {
					numeral_suffixes[numeral_suffix_count] = PlayerRaces.LanguageNumerals[civilization][i]->Word;
					numeral_suffix_ids[numeral_suffix_count] = i;
					numeral_suffix_count += 1;
				}
			}
			if (PlayerRaces.LanguageNumerals[civilization][i]->InfixTerrainName[terrain]) {
				if (!PlayerRaces.LanguageNumerals[civilization][i]->Word.empty()) {
					numeral_infixes[numeral_infix_count] = PlayerRaces.LanguageNumerals[civilization][i]->Word;
					numeral_infix_ids[numeral_infix_count] = i;
					numeral_infix_count += 1;
				}
			}
		}
		
		if (noun_name_count > 0 || adjective_name_count > 0 || noun_prefix_count > 0 || verb_prefix_count > 0 || adjective_prefix_count > 0 || numeral_prefix_count > 0) {
			int total_prefix_count = noun_prefix_count + verb_prefix_count + adjective_prefix_count + numeral_prefix_count;
			int total_suffix_count = noun_suffix_count + verb_suffix_count + adjective_suffix_count + numeral_suffix_count;
			int total_infix_count = noun_infix_count + verb_infix_count + adjective_infix_count + numeral_infix_count;
			int random_number = SyncRand(noun_name_count + adjective_name_count + (total_prefix_count * total_suffix_count) + ((total_prefix_count * total_suffix_count) / 2) * total_infix_count);
			if (random_number < noun_name_count + adjective_name_count) { //entire name
				random_number = SyncRand(noun_name_count + adjective_name_count);
				if (random_number < noun_name_count) {
					tile_name = noun_names[SyncRand(noun_name_count)];
				} else {
					tile_name = adjective_names[SyncRand(adjective_name_count)];
				}
			} else if (random_number < (noun_name_count + (total_prefix_count * total_suffix_count))) { //prefix + suffix
				std::string prefix;
				std::string suffix;
				int prefix_id;
				int suffix_id;
				std::string prefix_word_type;
				std::string suffix_word_type;
				
				//choose the word type of the prefix, and the prefix itself
				random_number = SyncRand(noun_prefix_count + verb_prefix_count + adjective_prefix_count + numeral_prefix_count);
				if (random_number < noun_prefix_count) {
					prefix_word_type = "noun";
					prefix_id = SyncRand(noun_prefix_count);
					prefix = noun_prefixes[prefix_id];
				} else if (random_number < (noun_prefix_count + verb_prefix_count)) {
					prefix_word_type = "verb";
					prefix_id = SyncRand(verb_prefix_count);
					prefix = verb_prefixes[prefix_id];
				} else if (random_number < (noun_prefix_count + verb_prefix_count + adjective_prefix_count)) {
					prefix_word_type = "adjective";
					prefix_id = SyncRand(adjective_prefix_count);
					prefix = adjective_prefixes[prefix_id];
				} else if (random_number < (noun_prefix_count + verb_prefix_count + adjective_prefix_count + numeral_prefix_count)) {
					prefix_word_type = "numeral";
					prefix_id = SyncRand(numeral_prefix_count);
					prefix = numeral_prefixes[prefix_id];
				}

				//choose the word type of the suffix, and the suffix itself
				random_number = SyncRand(noun_suffix_count + verb_suffix_count + adjective_suffix_count + numeral_suffix_count);
				if (random_number < noun_suffix_count) {
					suffix_word_type = "noun";
					suffix_id = SyncRand(noun_suffix_count);
					suffix = noun_suffixes[suffix_id];
				} else if (random_number < (noun_suffix_count + verb_suffix_count)) {
					suffix_word_type = "verb";
					suffix_id = SyncRand(verb_suffix_count);
					suffix = verb_suffixes[suffix_id];
				} else if (random_number < (noun_suffix_count + verb_suffix_count + adjective_suffix_count)) {
					suffix_word_type = "adjective";
					suffix_id = SyncRand(adjective_suffix_count);
					suffix = adjective_suffixes[suffix_id];
				} else if (random_number < (noun_suffix_count + verb_suffix_count + adjective_suffix_count + numeral_suffix_count)) {
					suffix_word_type = "numeral";
					suffix_id = SyncRand(numeral_suffix_count);
					suffix = numeral_suffixes[suffix_id];
				}

				if (prefix_word_type == "numeral" && PlayerRaces.LanguageNumerals[civilization][numeral_prefix_ids[prefix_id]]->Number > 1 && suffix_word_type == "noun") { // if requires plural (by being a numeral greater than one) and suffix is a noun
					//then replace the suffix with its plural form
					suffix = PlayerRaces.LanguageNouns[civilization][noun_suffix_ids[suffix_id]]->PluralNominative;
				}
					
				suffix = DecapitalizeString(suffix);
				if (prefix.substr(prefix.size() - 2, 2) == "gs" && suffix.substr(0, 1) == "g") { //if the last two characters of the prefix are "gs", and the first character of the suffix is "g", then remove the final "s" from the prefix (as in "Königgrätz")
					prefix = FindAndReplaceStringEnding(prefix, "gs", "g");
				}
				if (prefix.substr(prefix.size() - 1, 1) == "s" && suffix.substr(0, 1) == "s") { //if the prefix ends in "s" and the suffix begins in "s" as well, then remove the final "s" from the prefix (as in "Josefstadt", "Kronstadt" and "Leopoldstadt")
					prefix = FindAndReplaceStringEnding(prefix, "s", "");
				}
				tile_name = prefix;
				tile_name += suffix;
			} else if (random_number < (noun_name_count + (total_prefix_count * total_suffix_count) + ((total_prefix_count * total_suffix_count) / 2) * total_infix_count)) { //prefix + infix + suffix
				std::string prefix;
				std::string infix;
				std::string suffix;
				int prefix_id;
				int infix_id;
				int suffix_id;
				std::string prefix_word_type;
				std::string infix_word_type;
				std::string suffix_word_type;
				
				//choose the word type of the prefix, and the prefix itself
				random_number = SyncRand(noun_prefix_count + verb_prefix_count + adjective_prefix_count + numeral_prefix_count);
				if (random_number < noun_prefix_count) {
					prefix_word_type = "noun";
					prefix_id = SyncRand(noun_prefix_count);
					prefix = noun_prefixes[prefix_id];
				} else if (random_number < (noun_prefix_count + verb_prefix_count)) {
					prefix_word_type = "verb";
					prefix_id = SyncRand(verb_prefix_count);
					prefix = verb_prefixes[prefix_id];
				} else if (random_number < (noun_prefix_count + verb_prefix_count + adjective_prefix_count)) {
					prefix_word_type = "adjective";
					prefix_id = SyncRand(adjective_prefix_count);
					prefix = adjective_prefixes[prefix_id];
				} else if (random_number < (noun_prefix_count + verb_prefix_count + adjective_prefix_count + numeral_prefix_count)) {
					prefix_word_type = "numeral";
					prefix_id = SyncRand(numeral_prefix_count);
					prefix = numeral_prefixes[prefix_id];
				}

				//choose the word type of the infix, and the infix itself
				random_number = SyncRand(noun_infix_count + verb_infix_count + adjective_infix_count + numeral_infix_count);
				if (random_number < noun_infix_count) {
					infix_word_type = "noun";
					infix_id = SyncRand(noun_infix_count);
					infix = noun_infixes[infix_id];
				} else if (random_number < (noun_infix_count + verb_infix_count)) {
					infix_word_type = "verb";
					infix_id = SyncRand(verb_infix_count);
					infix = verb_infixes[infix_id];
				} else if (random_number < (noun_infix_count + verb_infix_count + adjective_infix_count)) {
					infix_word_type = "adjective";
					infix_id = SyncRand(adjective_infix_count);
					infix = adjective_infixes[infix_id];
				} else if (random_number < (noun_infix_count + verb_infix_count + adjective_infix_count + numeral_infix_count)) {
					infix_word_type = "numeral";
					infix_id = SyncRand(numeral_infix_count);
					infix = numeral_infixes[infix_id];
				}

				//choose the word type of the suffix, and the suffix itself
				random_number = SyncRand(noun_suffix_count + verb_suffix_count + adjective_suffix_count + numeral_suffix_count);
				if (random_number < noun_suffix_count) {
					suffix_word_type = "noun";
					suffix_id = SyncRand(noun_suffix_count);
					suffix = noun_suffixes[suffix_id];
				} else if (random_number < (noun_suffix_count + verb_suffix_count)) {
					suffix_word_type = "verb";
					suffix_id = SyncRand(verb_suffix_count);
					suffix = verb_suffixes[suffix_id];
				} else if (random_number < (noun_suffix_count + verb_suffix_count + adjective_suffix_count)) {
					suffix_word_type = "adjective";
					suffix_id = SyncRand(adjective_suffix_count);
					suffix = adjective_suffixes[suffix_id];
				} else if (random_number < (noun_suffix_count + verb_suffix_count + adjective_suffix_count + numeral_suffix_count)) {
					suffix_word_type = "numeral";
					suffix_id = SyncRand(numeral_suffix_count);
					suffix = numeral_suffixes[suffix_id];
				}

				if (prefix_word_type == "numeral" && PlayerRaces.LanguageNumerals[civilization][numeral_prefix_ids[prefix_id]]->Number > 1 && suffix_word_type == "noun") { // if requires plural (by being a numeral greater than one) and suffix is a noun
					//then replace the suffix with its plural form
					suffix = PlayerRaces.LanguageNouns[civilization][noun_suffix_ids[suffix_id]]->PluralNominative;
				}
					
				infix = DecapitalizeString(suffix);
				suffix = DecapitalizeString(suffix);
				if (prefix.substr(prefix.size() - 2, 2) == "gs" && infix.substr(0, 1) == "g") { //if the last two characters of the prefix are "gs", and the first character of the infix is "g", then remove the final "s" from the prefix (as in "Königgrätz")
					prefix = FindAndReplaceStringEnding(prefix, "gs", "g");
				}
				if (prefix.substr(prefix.size() - 1, 1) == "s" && infix.substr(0, 1) == "s") { //if the prefix ends in "s" and the infix begins in "s" as well, then remove the final "s" from the prefix (as in "Josefstadt", "Kronstadt" and "Leopoldstadt")
					prefix = FindAndReplaceStringEnding(prefix, "s", "");
				}
				if (infix.substr(infix.size() - 2, 2) == "gs" && suffix.substr(0, 1) == "g") { //if the last two characters of the infix are "gs", and the first character of the suffix is "g", then remove the final "s" from the infix (as in "Königgrätz")
					infix = FindAndReplaceStringEnding(infix, "gs", "g");
				}
				if (infix.substr(infix.size() - 1, 1) == "s" && suffix.substr(0, 1) == "s") { //if the infix ends in "s" and the suffix begins in "s" as well, then remove the final "s" from the infix (as in "Josefstadt", "Kronstadt" and "Leopoldstadt")
					infix = FindAndReplaceStringEnding(infix, "s", "");
				}
				tile_name = prefix;
				tile_name += infix;
				tile_name += suffix;
			}
		}
	}
	
	tile_name = TransliterateText(tile_name);
	
	return tile_name;
}

void CGrandStrategyFaction::SetTechnology(int upgrade_id, bool has_technology, bool secondary_setting)
{
	if (this->Technologies[upgrade_id] == has_technology) {
		return;
	}
	
	this->Technologies[upgrade_id] = has_technology;
	
	int change = has_technology ? 1 : -1;
		
	//add military score bonuses
	for (int z = 0; z < NumUpgradeModifiers; ++z) {
		if (UpgradeModifiers[z]->UpgradeId == upgrade_id) {
			for (size_t i = 0; i < UnitTypes.size(); ++i) {
				
				Assert(UpgradeModifiers[z]->ApplyTo[i] == '?' || UpgradeModifiers[z]->ApplyTo[i] == 'X');

				if (UpgradeModifiers[z]->ApplyTo[i] == 'X') {
					if (UpgradeModifiers[z]->Modifier.Variables[POINTS_INDEX].Value) {
						this->MilitaryScoreBonus[i] += UpgradeModifiers[z]->Modifier.Variables[POINTS_INDEX].Value * change;
					}
				}
			}
		}
	}
	
	if (!secondary_setting) { //if this technology is not being set as a result of another technology of the same class being researched
		if (has_technology) { //if value is true, mark technologies from other civilizations that are of the same class as researched too, so that the player doesn't need to research the same type of technology every time
			if (!AllUpgrades[upgrade_id]->Class.empty()) {
				for (size_t i = 0; i < AllUpgrades.size(); ++i) {
					if (AllUpgrades[upgrade_id]->Class == AllUpgrades[i]->Class) {
						this->SetTechnology(i, has_technology, true);
					}
				}
			}
		}
		
		for (int i = 0; i < MaxCosts; ++i) {
			if (AllUpgrades[upgrade_id]->GrandStrategyProductionEfficiencyModifier[i] != 0) {
				this->ProductionEfficiencyModifier[i] += AllUpgrades[upgrade_id]->GrandStrategyProductionEfficiencyModifier[i] * change;
				this->CalculateIncome(i);
			}
		}
	}
}

void CGrandStrategyFaction::CalculateIncome(int resource)
{
	if (resource == -1) {
		return;
	}
	
	if (this->ProvinceCount == 0) {
		this->Income[resource] = 0;
		return;
	}
	
	for (int i = 0; i < this->ProvinceCount; ++i) {
		int province_id = this->OwnedProvinces[i];
		GrandStrategyGame.Provinces[province_id]->CalculateIncome(resource);
	}
}

void CGrandStrategyFaction::CalculateIncomes()
{
	for (int i = 0; i < MaxCosts; ++i) {
		this->CalculateIncome(i);
	}
}

/**
**  Get the width of the world map.
*/
int GetWorldMapWidth()
{
	return GrandStrategyGame.WorldMapWidth;
}

/**
**  Get the height of the world map.
*/
int GetWorldMapHeight()
{
	return GrandStrategyGame.WorldMapHeight;
}

/**
**  Get the terrain type of a world map tile.
*/
std::string GetWorldMapTileTerrain(int x, int y)
{
	
	clamp(&x, 0, GrandStrategyGame.WorldMapWidth - 1);
	clamp(&y, 0, GrandStrategyGame.WorldMapHeight - 1);

	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	if (GrandStrategyGame.WorldMapTiles[x][y]->Terrain == -1) {
		return "";
	}
	
	return GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name;
}

/**
**  Get the terrain variation of a world map tile.
*/
int GetWorldMapTileTerrainVariation(int x, int y)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	Assert(GrandStrategyGame.WorldMapTiles[x][y]->Terrain != -1);
	Assert(GrandStrategyGame.WorldMapTiles[x][y]->Variation != -1);
	
	return GrandStrategyGame.WorldMapTiles[x][y]->Variation + 1;
}

std::string GetWorldMapTileProvinceName(int x, int y)
{
	
	clamp(&x, 0, GrandStrategyGame.WorldMapWidth - 1);
	clamp(&y, 0, GrandStrategyGame.WorldMapHeight - 1);

	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	if (GrandStrategyGame.WorldMapTiles[x][y]->Province != -1) {
		return GrandStrategyGame.Provinces[GrandStrategyGame.WorldMapTiles[x][y]->Province]->Name;
	} else {
		return "";
	}
}

bool WorldMapTileHasResource(int x, int y, std::string resource_name, bool ignore_prospection)
{
	clamp(&x, 0, GrandStrategyGame.WorldMapWidth - 1);
	clamp(&y, 0, GrandStrategyGame.WorldMapHeight - 1);

	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	if (resource_name == "any") {
		return GrandStrategyGame.WorldMapTiles[x][y]->Resource != -1 && (GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected || ignore_prospection);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return false;
	}
	
	return GrandStrategyGame.WorldMapTiles[x][y]->HasResource(resource, ignore_prospection);
}

/**
**  Get the ID of a world map terrain type
*/
int GetWorldMapTerrainTypeId(std::string terrain_type_name)
{
	for (int i = 0; i < WorldMapTerrainTypeMax; ++i) {
		if (!GrandStrategyGame.TerrainTypes[i]) {
			break;
		}
		
		if (GrandStrategyGame.TerrainTypes[i]->Name == terrain_type_name) {
			return i;
		}
	}
	return -1;
}

/**
**  Get the ID of a province
*/
int GetProvinceId(std::string province_name)
{
	for (int i = 0; i < ProvinceMax; ++i) {
		if (!GrandStrategyGame.Provinces[i]) {
			break;
		}
		
		if (!GrandStrategyGame.Provinces[i]->Name.empty() && GrandStrategyGame.Provinces[i]->Name == province_name) {
			return i;
		}
	}
	
	if (!province_name.empty()) {
		fprintf(stderr, "Can't find %s province.\n", province_name.c_str());
	}
	
	return -1;
}

/**
**  Set the size of the world map.
*/
void SetWorldMapSize(int width, int height)
{
	Assert(width <= WorldMapWidthMax);
	Assert(height <= WorldMapHeightMax);
	GrandStrategyGame.WorldMapWidth = width;
	GrandStrategyGame.WorldMapHeight = height;
	
	//create new world map tile objects for the size, if necessary
	if (!GrandStrategyGame.WorldMapTiles[width - 1][height - 1]) {
		for (int x = 0; x < GrandStrategyGame.WorldMapWidth; ++x) {
			for (int y = 0; y < GrandStrategyGame.WorldMapHeight; ++y) {
				if (!GrandStrategyGame.WorldMapTiles[x][y]) {
					WorldMapTile *world_map_tile = new WorldMapTile;
					GrandStrategyGame.WorldMapTiles[x][y] = world_map_tile;
					GrandStrategyGame.WorldMapTiles[x][y]->Position.x = x;
					GrandStrategyGame.WorldMapTiles[x][y]->Position.y = y;
				}
			}
		}
	}
}

/**
**  Set the terrain type of a world map tile.
*/
void SetWorldMapTileTerrain(int x, int y, int terrain)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	//if tile doesn't exist, create it now
	if (!GrandStrategyGame.WorldMapTiles[x][y]) {
		WorldMapTile *world_map_tile = new WorldMapTile;
		GrandStrategyGame.WorldMapTiles[x][y] = world_map_tile;
		GrandStrategyGame.WorldMapTiles[x][y]->Position.x = x;
		GrandStrategyGame.WorldMapTiles[x][y]->Position.y = y;
	}
	
	GrandStrategyGame.WorldMapTiles[x][y]->Terrain = terrain;
	
	if (terrain != -1 && GrandStrategyGame.TerrainTypes[terrain]) {
		//randomly select a variation for the world map tile
		if (GrandStrategyGame.TerrainTypes[terrain]->Variations > 0) {
			GrandStrategyGame.WorldMapTiles[x][y]->Variation = SyncRand(GrandStrategyGame.TerrainTypes[terrain]->Variations);
		} else {
			GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
		}
		
		int base_terrain = GrandStrategyGame.TerrainTypes[terrain]->BaseTile;
		if (base_terrain != -1 && GrandStrategyGame.TerrainTypes[base_terrain]) {
			//randomly select a variation for the world map tile
			if (GrandStrategyGame.TerrainTypes[base_terrain]->Variations > 0) {
				GrandStrategyGame.WorldMapTiles[x][y]->BaseTileVariation = SyncRand(GrandStrategyGame.TerrainTypes[base_terrain]->Variations);
			} else {
				GrandStrategyGame.WorldMapTiles[x][y]->BaseTileVariation = -1;
			}
		}
	}
}

void SetWorldMapTileProvince(int x, int y, std::string province_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	int old_province_id = GrandStrategyGame.WorldMapTiles[x][y]->Province;
	if (old_province_id != -1) { //if the tile is already assigned to a province, remove it from that province's tile arrays
		for (int i = 0; i < ProvinceTileMax; ++i) {
			if (GrandStrategyGame.Provinces[old_province_id]->Tiles[i].x == x && GrandStrategyGame.Provinces[old_province_id]->Tiles[i].y == y) { //if tile was found, push every element of the array after it back one step
				for (int j = i; j < ProvinceTileMax; ++j) {
					GrandStrategyGame.Provinces[old_province_id]->Tiles[j].x = GrandStrategyGame.Provinces[old_province_id]->Tiles[j + 1].x;
					GrandStrategyGame.Provinces[old_province_id]->Tiles[j].y = GrandStrategyGame.Provinces[old_province_id]->Tiles[j + 1].y;
					if (GrandStrategyGame.Provinces[old_province_id]->Tiles[j].x == -1 && GrandStrategyGame.Provinces[old_province_id]->Tiles[j].y == -1) { // if this is a blank tile slot
						break;
					}
				}
				break;
			}
			if (GrandStrategyGame.Provinces[old_province_id]->Tiles[i].x == -1 && GrandStrategyGame.Provinces[old_province_id]->Tiles[i].y == -1) { // if this is a blank tile slot
				break;
			}
		}
		
		if (GrandStrategyGame.WorldMapTiles[x][y]->Resource != -1) {
			int res = GrandStrategyGame.WorldMapTiles[x][y]->Resource;
			if (GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected) {
				GrandStrategyGame.Provinces[old_province_id]->ProductionCapacity[res] -= 1;
			}
			for (int i = 0; i < ProvinceTileMax; ++i) {
				if (GrandStrategyGame.Provinces[old_province_id]->ResourceTiles[res][i].x == x && GrandStrategyGame.Provinces[old_province_id]->ResourceTiles[res][i].y == y) { //if tile was found, push every element of the array after it back one step
					for (int j = i; j < ProvinceTileMax; ++j) {
						GrandStrategyGame.Provinces[old_province_id]->ResourceTiles[res][j].x = GrandStrategyGame.Provinces[old_province_id]->ResourceTiles[res][j + 1].x;
						GrandStrategyGame.Provinces[old_province_id]->ResourceTiles[res][j].y = GrandStrategyGame.Provinces[old_province_id]->ResourceTiles[res][j + 1].y;
						if (GrandStrategyGame.Provinces[old_province_id]->ResourceTiles[res][j].x == -1 && GrandStrategyGame.Provinces[old_province_id]->ResourceTiles[res][j].y == -1) { // if this is a blank tile slot
							break;
						}
					}
					break;
				}
				if (GrandStrategyGame.Provinces[old_province_id]->ResourceTiles[res][i].x == -1 && GrandStrategyGame.Provinces[old_province_id]->ResourceTiles[res][i].y == -1) { // if this is a blank tile slot
					break;
				}
			}
		}
	}

	int province_id = GetProvinceId(province_name);
	GrandStrategyGame.WorldMapTiles[x][y]->Province = province_id;
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		//now add the tile to the province's tile arrays
		for (int i = 0; i < ProvinceTileMax; ++i) {
			if (GrandStrategyGame.Provinces[province_id]->Tiles[i].x == x && GrandStrategyGame.Provinces[province_id]->Tiles[i].y == y) { //if tile is there already, stop
				break;
			}
			if (GrandStrategyGame.Provinces[province_id]->Tiles[i].x == -1 && GrandStrategyGame.Provinces[province_id]->Tiles[i].y == -1) { // if this is a blank tile slot
				GrandStrategyGame.Provinces[province_id]->Tiles[i].x = x;
				GrandStrategyGame.Provinces[province_id]->Tiles[i].y = y;
				break;
			}
		}
		
		if (GrandStrategyGame.WorldMapTiles[x][y]->Resource != -1) {
			int res = GrandStrategyGame.WorldMapTiles[x][y]->Resource;
			if (GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected) {
				GrandStrategyGame.Provinces[province_id]->ProductionCapacity[res] += 1;
			}
			for (int i = 0; i < ProvinceTileMax; ++i) {
				if (GrandStrategyGame.Provinces[province_id]->ResourceTiles[res][i].x == x && GrandStrategyGame.Provinces[province_id]->ResourceTiles[res][i].y == y) { //if tile is there already, stop
					break;
				}
				if (GrandStrategyGame.Provinces[province_id]->ResourceTiles[res][i].x == -1 && GrandStrategyGame.Provinces[province_id]->ResourceTiles[res][i].y == -1) { // if this is a blank tile slot
					GrandStrategyGame.Provinces[province_id]->ResourceTiles[res][i].x = x;
					GrandStrategyGame.Provinces[province_id]->ResourceTiles[res][i].y = y;
					break;
				}
			}
		}
	}
}

/**
**  Set the name of a world map tile.
*/
void SetWorldMapTileName(int x, int y, std::string name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	GrandStrategyGame.WorldMapTiles[x][y]->Name = name;
}

/**
**  Set the cultural name of a world map tile for a particular civilization.
*/
void SetWorldMapTileCulturalName(int x, int y, std::string civilization_name, std::string cultural_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	if (civilization != -1) {
		GrandStrategyGame.WorldMapTiles[x][y]->CulturalNames[civilization] = cultural_name;
	}
}

int GetRiverId(std::string river_name)
{
	for (int i = 0; i < RiverMax; ++i) {
		if (!GrandStrategyGame.Rivers[i] || GrandStrategyGame.Rivers[i]->Name.empty()) {
			if (!river_name.empty()) { //if the name is not empty and reached a blank spot, create a new river and return its ID
				if (!GrandStrategyGame.Rivers[i]) { //if river doesn't exist, create it now
					CRiver *river = new CRiver;
					GrandStrategyGame.Rivers[i] = river;
				}
				GrandStrategyGame.Rivers[i]->Name = river_name;
				return i;
			}
			break;
		}
		
		if (!GrandStrategyGame.Rivers[i]->Name.empty() && GrandStrategyGame.Rivers[i]->Name == river_name) {
			return i;
		}
	}
	
	return -1;
}

/**
**  Set river data for a world map tile.
*/
void SetWorldMapTileRiver(int x, int y, std::string direction_name, std::string river_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	int river_id = GetRiverId(river_name);
	
	if (direction_name == "north") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[North] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->River[Northwest] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->River[Northeast] = river_id;
	} else if (direction_name == "northeast") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[Northeast] = river_id;
	} else if (direction_name == "east") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[East] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->River[Northeast] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->River[Southeast] = river_id;
	} else if (direction_name == "southeast") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[Southeast] = river_id;
	} else if (direction_name == "south") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[South] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->River[Southwest] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->River[Southeast] = river_id;
	} else if (direction_name == "southwest") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[Southwest] = river_id;
	} else if (direction_name == "west") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[West] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->River[Northwest] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->River[Southwest] = river_id;
	} else if (direction_name == "northwest") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[Northwest] = river_id;
	} else {
		fprintf(stderr, "Error: Wrong direction set for river.\n");
	}
	
}

/**
**  Set riverhead data for a world map tile.
*/
void SetWorldMapTileRiverhead(int x, int y, std::string direction_name, std::string river_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	int river_id = GetRiverId(river_name);
	
	if (direction_name == "north") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[North] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[North] = river_id;
	} else if (direction_name == "northeast") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[Northeast] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[Northeast] = river_id;
	} else if (direction_name == "east") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[East] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[East] = river_id;
	} else if (direction_name == "southeast") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[Southeast] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[Southeast] = river_id;
	} else if (direction_name == "south") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[South] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[South] = river_id;
	} else if (direction_name == "southwest") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[Southwest] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[Southwest] = river_id;
	} else if (direction_name == "west") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[West] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[West] = river_id;
	} else if (direction_name == "northwest") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[Northwest] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[Northwest] = river_id;
	} else {
		fprintf(stderr, "Error: Wrong direction set for river.\n");
	}
	
}

/**
**  Set pathway data for a world map tile.
*/
void SetWorldMapTilePathway(int x, int y, std::string direction_name, std::string pathway_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	int direction;
	if (direction_name == "north") {
		direction = North;
	} else if (direction_name == "northeast") {
		direction = Northeast;
	} else if (direction_name == "east") {
		direction = East;
	} else if (direction_name == "southeast") {
		direction = Southeast;
	} else if (direction_name == "south") {
		direction = South;
	} else if (direction_name == "southwest") {
		direction = Southwest;
	} else if (direction_name == "west") {
		direction = West;
	} else if (direction_name == "northwest") {
		direction = Northwest;
	} else {
		fprintf(stderr, "Wrong direction (\"%s\") set for pathway.\n", direction_name.c_str());
		return;
	}
	
	int pathway_id;
	
	if (pathway_name == "none") {
		pathway_id = -1;
	} else if (pathway_name == "trail") {
		pathway_id = PathwayTrail;
	} else if (pathway_name == "road") {
		pathway_id = PathwayRoad;
	} else {
		fprintf(stderr, "Pathway \"%s\" does not exist.\n", pathway_name.c_str());
		return;
	}
	
	GrandStrategyGame.WorldMapTiles[x][y]->Pathway[direction] = pathway_id;
}

/**
**  Calculate the graphic tile for a world map tile.
*/
void CalculateWorldMapTileGraphicTile(int x, int y)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	Assert(GrandStrategyGame.WorldMapTiles[x][y]->Terrain != -1);
	
	int terrain = GrandStrategyGame.WorldMapTiles[x][y]->Terrain;
	
	if (terrain != -1 && GrandStrategyGame.TerrainTypes[terrain]) {
		//set the GraphicTile for this world map tile
		std::string graphic_tile = "tilesets/world/terrain/";
		graphic_tile += GrandStrategyGame.TerrainTypes[terrain]->Tag;
		
		std::string base_tile_filename;
		if (GrandStrategyGame.TerrainTypes[terrain]->BaseTile != -1) {
			if (GrandStrategyWorld == "Nidavellir") {
				base_tile_filename = "tilesets/world/terrain/dark_plains";
			} else {
				base_tile_filename = "tilesets/world/terrain/plains";
			}
		}
		
		if (GrandStrategyGame.TerrainTypes[terrain]->HasTransitions) {
			graphic_tile += "/";
			graphic_tile += GrandStrategyGame.TerrainTypes[terrain]->Tag;
			
			if (
				GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_north";
			}
			if (
				GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_south";
			}
			if (
				GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_west";
			}
			if (
				GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_east";
			}
			if (
				GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_northwest_outer";
			}
			if (
				GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_northeast_outer";
			}
			if (
				GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_southwest_outer";
			}
			if (
				GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_southeast_outer";
			}
			if (
				GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_northwest_inner";
			}
			if (
				GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_northeast_inner";
			}
			if (
				GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_southwest_inner";
			}
			if (
				GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_southeast_inner";
			}
			if (
				GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_inner";
			}
			if (
				/*
				GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y)
				*/
				graphic_tile.find("north", 0) == std::string::npos
				&& graphic_tile.find("south", 0) == std::string::npos
				&& graphic_tile.find("west", 0) == std::string::npos
				&& graphic_tile.find("east", 0) == std::string::npos
				&& graphic_tile.find("inner", 0) == std::string::npos
			) {
				graphic_tile += "_outer";
			}
			graphic_tile = FindAndReplaceString(graphic_tile, "_northwest_outer_northeast_outer_southwest_outer_southeast_outer", "_outer");
			graphic_tile = FindAndReplaceString(graphic_tile, "_northwest_outer_northeast_outer_southwest_outer", "_outer");
			graphic_tile = FindAndReplaceString(graphic_tile, "_northwest_outer_northeast_outer_southeast_outer", "_outer");
			graphic_tile = FindAndReplaceString(graphic_tile, "_northeast_outer_southwest_outer_southeast_outer", "_outer");
			graphic_tile = FindAndReplaceString(graphic_tile, "_northeast_outer_southwest_outer_southeast_outer", "_outer");
		}
		
		if (GrandStrategyGame.WorldMapTiles[x][y]->Variation != -1) {
			graphic_tile += "_";
			graphic_tile += std::to_string((long long) GrandStrategyGame.WorldMapTiles[x][y]->Variation + 1);
			
		}
		
		if (GrandStrategyGame.TerrainTypes[terrain]->BaseTile != -1 && GrandStrategyGame.WorldMapTiles[x][y]->BaseTileVariation != -1) {
			base_tile_filename += "_";
			base_tile_filename += std::to_string((long long) GrandStrategyGame.WorldMapTiles[x][y]->BaseTileVariation + 1);
		}
			
		graphic_tile += ".png";
		if (GrandStrategyGame.TerrainTypes[terrain]->BaseTile != -1) {
			base_tile_filename += ".png";
		}
		
		if (!CanAccessFile(graphic_tile.c_str()) && GrandStrategyGame.WorldMapTiles[x][y]->Variation != -1) {
			for (int i = GrandStrategyGame.WorldMapTiles[x][y]->Variation; i > -1; --i) {
				if (i >= 1) {
					graphic_tile = FindAndReplaceString(graphic_tile, std::to_string((long long) i + 1), std::to_string((long long) i));
				} else {
					graphic_tile = FindAndReplaceString(graphic_tile, "_1", "");
				}
				
				if (CanAccessFile(graphic_tile.c_str())) {
					break;
				}
			}
		}
		if (GrandStrategyGame.TerrainTypes[terrain]->BaseTile != -1) {
			if (!CanAccessFile(base_tile_filename.c_str()) && GrandStrategyGame.WorldMapTiles[x][y]->BaseTileVariation != -1) {
				for (int i = GrandStrategyGame.WorldMapTiles[x][y]->BaseTileVariation; i > -1; --i) {
					if (i >= 1) {
						base_tile_filename = FindAndReplaceString(base_tile_filename, std::to_string((long long) i + 1), std::to_string((long long) i));
					} else {
						base_tile_filename = FindAndReplaceString(base_tile_filename, "_1", "");
					}
					
					if (CanAccessFile(base_tile_filename.c_str())) {
						break;
					}
				}
			}
		}
		
		if (CGraphic::Get(graphic_tile) == NULL) {
			CGraphic *tile_graphic = CGraphic::New(graphic_tile, 64, 64);
			tile_graphic->Load();
		}
		GrandStrategyGame.WorldMapTiles[x][y]->GraphicTile = CGraphic::Get(graphic_tile);
		
		if (GrandStrategyGame.TerrainTypes[terrain]->BaseTile != -1) {
			if (CGraphic::Get(base_tile_filename) == NULL) {
				CGraphic *base_tile_graphic = CGraphic::New(base_tile_filename, 64, 64);
				base_tile_graphic->Load();
			}
			GrandStrategyGame.WorldMapTiles[x][y]->BaseTile = CGraphic::Get(base_tile_filename);
		}
	}
}

void AddWorldMapResource(std::string resource_name, int x, int y, bool discovered)
{
	int province_id = GrandStrategyGame.WorldMapTiles[x][y]->Province;
	if (GrandStrategyGame.WorldMapTiles[x][y]->Resource != -1) { //if tile already has a resource, remove it from the old resource's arrays
		int old_resource = GrandStrategyGame.WorldMapTiles[x][y]->Resource;
		for (int i = 0; i < WorldMapResourceMax; ++i) { // remove it from the world map resources array
			if (GrandStrategyGame.WorldMapResources[old_resource][i].x == x && GrandStrategyGame.WorldMapResources[old_resource][i].y == y) { //if tile was found, push every element of the array after it back one step
				for (int j = i; j < WorldMapResourceMax; ++j) {
					GrandStrategyGame.WorldMapResources[old_resource][j].x = GrandStrategyGame.WorldMapResources[old_resource][j + 1].x;
					GrandStrategyGame.WorldMapResources[old_resource][j].y = GrandStrategyGame.WorldMapResources[old_resource][j + 1].y;
					if (GrandStrategyGame.WorldMapResources[old_resource][j].x == -1 && GrandStrategyGame.WorldMapResources[old_resource][j].y == -1) { // if this is a blank tile slot
						break;
					}
				}
				break;
			}
			if (GrandStrategyGame.WorldMapResources[old_resource][i].x == -1 && GrandStrategyGame.WorldMapResources[old_resource][i].y == -1) { // if this is a blank tile slot
				break;
			}
		}
	
		if (province_id != -1) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected) {
				GrandStrategyGame.Provinces[province_id]->ProductionCapacity[old_resource] -= 1;
			}
			for (int i = 0; i < ProvinceTileMax; ++i) { //remove it from the province's resource tile array
				if (GrandStrategyGame.Provinces[province_id]->ResourceTiles[old_resource][i].x == x && GrandStrategyGame.Provinces[province_id]->ResourceTiles[old_resource][i].y == y) { //if tile was found, push every element of the array after it back one step
					for (int j = i; j < ProvinceTileMax; ++j) {
						GrandStrategyGame.Provinces[province_id]->ResourceTiles[old_resource][j].x = GrandStrategyGame.Provinces[province_id]->ResourceTiles[old_resource][j + 1].x;
						GrandStrategyGame.Provinces[province_id]->ResourceTiles[old_resource][j].y = GrandStrategyGame.Provinces[province_id]->ResourceTiles[old_resource][j + 1].y;
						if (GrandStrategyGame.Provinces[province_id]->ResourceTiles[old_resource][j].x == -1 && GrandStrategyGame.Provinces[province_id]->ResourceTiles[old_resource][j].y == -1) { // if this is a blank tile slot
							break;
						}
					}
					break;
				}
				if (GrandStrategyGame.Provinces[province_id]->ResourceTiles[old_resource][i].x == -1 && GrandStrategyGame.Provinces[province_id]->ResourceTiles[old_resource][i].y == -1) { // if this is a blank tile slot
					break;
				}
			}
		}
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource != -1) {
		for (int i = 0; i < WorldMapResourceMax; ++i) {
			if (GrandStrategyGame.WorldMapResources[resource][i].x == -1 && GrandStrategyGame.WorldMapResources[resource][i].y == -1) { //if this spot for a world map resource is blank
				GrandStrategyGame.WorldMapResources[resource][i].x = x;
				GrandStrategyGame.WorldMapResources[resource][i].y = y;
				GrandStrategyGame.WorldMapTiles[x][y]->Resource = resource;
				GrandStrategyGame.WorldMapTiles[x][y]->SetResourceProspected(resource, discovered);
				break;
			}
		}
		if (province_id != -1) {
			for (int i = 0; i < ProvinceTileMax; ++i) { //add tile to the province's respective resource tile array
				if (GrandStrategyGame.Provinces[province_id]->ResourceTiles[resource][i].x == x && GrandStrategyGame.Provinces[province_id]->ResourceTiles[resource][i].y == y) { //if tile is there already, stop
					break;
				}
				if (GrandStrategyGame.Provinces[province_id]->ResourceTiles[resource][i].x == -1 && GrandStrategyGame.Provinces[province_id]->ResourceTiles[resource][i].y == -1) { // if this is a blank tile slot
					GrandStrategyGame.Provinces[province_id]->ResourceTiles[resource][i].x = x;
					GrandStrategyGame.Provinces[province_id]->ResourceTiles[resource][i].y = y;
					break;
				}
			}
		}
	}
}

void SetWorldMapResourceProspected(std::string resource_name, int x, int y, bool discovered)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource != -1) {
		GrandStrategyGame.WorldMapTiles[x][y]->SetResourceProspected(resource, discovered);
	}
}

/**
**  Get the cultural name of a province
*/
std::string GetProvinceCulturalName(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		return GrandStrategyGame.Provinces[province_id]->GetCulturalName();
	}
	
	return "";
}

/**
**  Get the cultural name of a province pertaining to a particular civilization
*/
std::string GetProvinceCivilizationCulturalName(std::string province_name, std::string civilization_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			return GrandStrategyGame.Provinces[province_id]->CulturalNames[civilization];
		}
	}
	
	return "";
}

/**
**  Get the cultural name of a province pertaining to a particular faction
*/
std::string GetProvinceFactionCulturalName(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
			if (faction != -1) {
				return GrandStrategyGame.Provinces[province_id]->FactionCulturalNames[civilization][faction];
			}
		}
	}
	
	return "";
}

/**
**  Get the cultural name of a province
*/
std::string GetProvinceCulturalSettlementName(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		return GrandStrategyGame.Provinces[province_id]->GetCulturalSettlementName();
	}
	
	return "";
}

/**
**  Get the cultural settlement name of a province pertaining to a particular civilization
*/
std::string GetProvinceCivilizationCulturalSettlementName(std::string province_name, std::string civilization_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			return GrandStrategyGame.Provinces[province_id]->CulturalSettlementNames[civilization];
		}
	}
	
	return "";
}

/**
**  Get the cultural settlement name of a province pertaining to a particular faction
*/
std::string GetProvinceFactionCulturalSettlementName(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
			if (faction != -1) {
				return GrandStrategyGame.Provinces[province_id]->FactionCulturalSettlementNames[civilization][faction];
			}
		}
	}
	
	return "";
}

std::string GetProvinceAttackedBy(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		if (GrandStrategyGame.Provinces[province_id]->AttackedBy != NULL) {
			return PlayerRaces.Factions[GrandStrategyGame.Provinces[province_id]->AttackedBy->Civilization][GrandStrategyGame.Provinces[province_id]->AttackedBy->Faction]->Name;
		}
	}
	
	return "";
}

void SetProvinceName(std::string old_province_name, std::string new_province_name)
{
	int province_id = GetProvinceId(old_province_name);
	
	if (province_id == -1 || !GrandStrategyGame.Provinces[province_id]) { //if province doesn't exist, create it now
		province_id = GrandStrategyGame.ProvinceCount;
		if (!GrandStrategyGame.Provinces[province_id]) {
			CProvince *province = new CProvince;
			GrandStrategyGame.Provinces[province_id] = province;
		}
		GrandStrategyGame.Provinces[province_id]->ID = province_id;
		GrandStrategyGame.ProvinceCount += 1;
	}
	
	GrandStrategyGame.Provinces[province_id]->Name = new_province_name;
}

void SetProvinceWater(std::string province_name, bool water)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->Water = water;
	}
}

void SetProvinceOwner(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	int civilization_id = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction_id = PlayerRaces.GetFactionIndexByName(civilization_id, faction_name);
	
	if (province_id == -1 || !GrandStrategyGame.Provinces[province_id]) {
		return;
	}
	
	GrandStrategyGame.Provinces[province_id]->SetOwner(civilization_id, faction_id);
}

void SetProvinceCivilization(std::string province_name, std::string civilization_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		GrandStrategyGame.Provinces[province_id]->SetCivilization(civilization);
	}
}

void SetProvinceSettlementName(std::string province_name, std::string settlement_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->SettlementName = settlement_name;
	}
}

void SetProvinceSettlementLocation(std::string province_name, int x, int y)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->SettlementLocation.x = x;
		GrandStrategyGame.Provinces[province_id]->SettlementLocation.y = y;
		
		GrandStrategyGame.Provinces[province_id]->ProductionCapacity[FishCost] = 0;
		for (int sub_x = -1; sub_x <= 1; ++sub_x) { //add 1 capacity in fish production for every water tile adjacent to the settlement location
			if ((x + sub_x) < 0 || (x + sub_x) >= GrandStrategyGame.WorldMapWidth) {
				continue;
			}
			for (int sub_y = -1; sub_y <= 1; ++sub_y) {
				if ((y + sub_y) < 0 || (y + sub_y) >= GrandStrategyGame.WorldMapHeight) {
					continue;
				}
				if (!(sub_x == 0 && sub_y == 0)) {
					if (GrandStrategyGame.WorldMapTiles[x + sub_x][y + sub_y]->Terrain != -1 && GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x + sub_x][y + sub_y]->Terrain]->Name == "Water") {
						GrandStrategyGame.Provinces[province_id]->ProductionCapacity[FishCost] += 1;
					}
				}
			}
		}
		
		for (int i = 0; i < MaxDirections; ++i) { //if the settlement location has a river, add one fish production capacity
			if (GrandStrategyGame.WorldMapTiles[x][y]->River[i] != -1) {
				GrandStrategyGame.Provinces[province_id]->ProductionCapacity[FishCost] += 1;
				break;
			}
		}
	}
}

void SetProvinceCulturalName(std::string province_name, std::string civilization_name, std::string province_cultural_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			GrandStrategyGame.Provinces[province_id]->CulturalNames[civilization] = province_cultural_name;
		}
	}
}

void SetProvinceFactionCulturalName(std::string province_name, std::string civilization_name, std::string faction_name, std::string province_cultural_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
			if (faction != -1) {
				GrandStrategyGame.Provinces[province_id]->FactionCulturalNames[civilization][faction] = province_cultural_name;
			}
		}
	}
}

void SetProvinceCulturalSettlementName(std::string province_name, std::string civilization_name, std::string province_cultural_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			GrandStrategyGame.Provinces[province_id]->CulturalSettlementNames[civilization] = province_cultural_name;
		}
	}
}

void SetProvinceFactionCulturalSettlementName(std::string province_name, std::string civilization_name, std::string faction_name, std::string province_cultural_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
			if (faction != -1) {
				GrandStrategyGame.Provinces[province_id]->FactionCulturalSettlementNames[civilization][faction] = province_cultural_name;
			}
		}
	}
}

void SetProvinceReferenceProvince(std::string province_name, std::string reference_province_name)
{
	int province_id = GetProvinceId(province_name);
	int reference_province_id = GetProvinceId(reference_province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && reference_province_id != -1) {
		GrandStrategyGame.Provinces[province_id]->ReferenceProvince = reference_province_id;
	}
}

void SetProvinceSettlementBuilding(std::string province_name, std::string settlement_building_ident, bool has_settlement_building)
{
	int province_id = GetProvinceId(province_name);
	int settlement_building = UnitTypeIdByIdent(settlement_building_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && settlement_building != -1) {
		GrandStrategyGame.Provinces[province_id]->SetSettlementBuilding(settlement_building, has_settlement_building);
	}
}

void SetProvinceCurrentConstruction(std::string province_name, std::string settlement_building_ident)
{
	int province_id = GetProvinceId(province_name);
	int settlement_building;
	if (!settlement_building_ident.empty()) {
		settlement_building = UnitTypeIdByIdent(settlement_building_ident);
	} else {
		settlement_building = -1;
	}
	if (province_id != -1) {
		GrandStrategyGame.Provinces[province_id]->CurrentConstruction = settlement_building;
	}
}

void SetProvincePopulation(std::string province_name, int quantity)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		if (GrandStrategyGame.Provinces[province_id]->Civilization == -1) {
			return;
		}
		int worker_unit_type = PlayerRaces.GetCivilizationClassUnitType(GrandStrategyGame.Provinces[province_id]->Civilization, GetUnitTypeClassIndexByName("worker"));
	
		if (quantity > 0) {
			quantity /= 10000; // each population unit represents 10,000 people
			quantity /= 2; // only (working-age) adults are represented, so around half of the total population
			quantity = std::max(1, quantity);
		}
	
//		quantity -= GrandStrategyGame.Provinces[province_id]->TotalUnits - GrandStrategyGame.Provinces[province_id]->Units[worker_unit_type]; // decrease from the quantity to be set the population that isn't composed of workers
		// don't take military units in consideration for this population count for now (since they don't cost food anyway)
		quantity -= GrandStrategyGame.Provinces[province_id]->TotalWorkers - GrandStrategyGame.Provinces[province_id]->Units[worker_unit_type]; // decrease from the quantity to be set the population that isn't composed of workers
		quantity = std::max(0, quantity);
			
		GrandStrategyGame.Provinces[province_id]->SetUnitQuantity(worker_unit_type, quantity);
	}
}

void SetProvinceUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && unit_type != -1) {
		GrandStrategyGame.Provinces[province_id]->SetUnitQuantity(unit_type, quantity);
	}
}

void ChangeProvinceUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && unit_type != -1) {
		GrandStrategyGame.Provinces[province_id]->ChangeUnitQuantity(unit_type, quantity);
	}
}

void SetProvinceUnderConstructionUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && unit_type != -1) {
		GrandStrategyGame.Provinces[province_id]->UnderConstructionUnits[unit_type] = std::max(0, quantity);
	}
}

void SetProvinceMovingUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && unit_type != -1) {
		if (quantity > 0) {
			GrandStrategyGame.Provinces[province_id]->Movement = true;
		}
		GrandStrategyGame.Provinces[province_id]->MovingUnits[unit_type] = std::max(0, quantity);
	}
}

void SetProvinceAttackingUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && unit_type != -1) {
		GrandStrategyGame.Provinces[province_id]->SetAttackingUnitQuantity(unit_type, quantity);
	}
}

void SetProvinceHero(std::string province_name, std::string hero_ident, int value)
{
	int province_id = GetProvinceId(province_name);
	int unit_type_id = UnitTypeIdByIdent(hero_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && unit_type_id != -1) {
		GrandStrategyGame.Provinces[province_id]->SetHero(unit_type_id, value);
	}
}

void SetProvinceFood(std::string province_name, int quantity)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->PopulationGrowthProgress = std::max(0, quantity);
	}
}

void ChangeProvinceFood(std::string province_name, int quantity)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->PopulationGrowthProgress += quantity;
		GrandStrategyGame.Provinces[province_id]->PopulationGrowthProgress = std::max(0, GrandStrategyGame.Provinces[province_id]->PopulationGrowthProgress);
	}
}

void SetProvinceAttackedBy(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization_id = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		int faction_id = PlayerRaces.GetFactionIndexByName(civilization_id, faction_name);
		if (civilization_id != -1 && faction_id != -1) {
			GrandStrategyGame.Provinces[province_id]->AttackedBy = const_cast<CGrandStrategyFaction *>(&(*GrandStrategyGame.Factions[civilization_id][faction_id]));
		} else {
			GrandStrategyGame.Provinces[province_id]->AttackedBy = NULL;
		}
	}
}

void SetSelectedProvince(std::string province_name)
{
	int province_id = GetProvinceId(province_name);

	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.SelectedProvince = province_id;
	}
}

void AddProvinceClaim(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
			if (faction != -1) {
				GrandStrategyGame.Provinces[province_id]->AddFactionClaim(civilization, faction);
			} else {
				fprintf(stderr, "Can't find %s faction (%s) to add claim to province %s.\n", faction_name.c_str(), civilization_name.c_str(), province_name.c_str());
			}
		} else {
			fprintf(stderr, "Can't find %s civilization to add the claim of its %s faction claim to province %s.\n", civilization_name.c_str(), faction_name.c_str(), province_name.c_str());
		}
	} else {
		fprintf(stderr, "Can't find %s province to add %s faction (%s) claim to.\n", province_name.c_str(), faction_name.c_str(), civilization_name.c_str());
	}
}

void RemoveProvinceClaim(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
			if (faction != -1) {
				GrandStrategyGame.Provinces[province_id]->RemoveFactionClaim(civilization, faction);
			}
		}
	}
}

void UpdateProvinceMinimap(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->UpdateMinimap();
	}
}

/**
**  Clean the grand strategy variables.
*/
void CleanGrandStrategyGame()
{
	for (int x = 0; x < WorldMapWidthMax; ++x) {
		for (int y = 0; y < WorldMapHeightMax; ++y) {
			if (GrandStrategyGame.WorldMapTiles[x][y]) {
				GrandStrategyGame.WorldMapTiles[x][y]->Terrain = -1;
				GrandStrategyGame.WorldMapTiles[x][y]->Province = -1;
				GrandStrategyGame.WorldMapTiles[x][y]->BaseTileVariation = -1;
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
				GrandStrategyGame.WorldMapTiles[x][y]->Resource = -1;
				GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected = false;
				GrandStrategyGame.WorldMapTiles[x][y]->Name = "";
				for (int i = 0; i < MAX_RACES; ++i) {
					GrandStrategyGame.WorldMapTiles[x][y]->CulturalNames[i] = "";
				}
				for (int i = 0; i < MaxDirections; ++i) {
					GrandStrategyGame.WorldMapTiles[x][y]->Borders[i] = false;
					GrandStrategyGame.WorldMapTiles[x][y]->River[i] = -1;
					GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[i] = -1;
					GrandStrategyGame.WorldMapTiles[x][y]->Pathway[i] = -1;
				}
			} else {
				break;
			}
		}
	}
	
	for (int i = 0; i < ProvinceMax; ++i) {
		if (GrandStrategyGame.Provinces[i] && !GrandStrategyGame.Provinces[i]->Name.empty()) {
			GrandStrategyGame.Provinces[i]->Name = "";
			GrandStrategyGame.Provinces[i]->SettlementName = "";
			GrandStrategyGame.Provinces[i]->ID = -1;
			GrandStrategyGame.Provinces[i]->Civilization = -1;
			GrandStrategyGame.Provinces[i]->Owner = NULL;
			GrandStrategyGame.Provinces[i]->ReferenceProvince = -1;
			GrandStrategyGame.Provinces[i]->CurrentConstruction = -1;
			GrandStrategyGame.Provinces[i]->AttackedBy = NULL;
			GrandStrategyGame.Provinces[i]->TotalUnits = 0;
			GrandStrategyGame.Provinces[i]->TotalWorkers = 0;
			GrandStrategyGame.Provinces[i]->PopulationGrowthProgress = 0;
			GrandStrategyGame.Provinces[i]->FoodConsumption = 0;
			GrandStrategyGame.Provinces[i]->Labor = 0;
			GrandStrategyGame.Provinces[i]->ClaimCount = 0;
			GrandStrategyGame.Provinces[i]->MilitaryScore = 0;
			GrandStrategyGame.Provinces[i]->OffensiveMilitaryScore = 0;
			GrandStrategyGame.Provinces[i]->AttackingMilitaryScore = 0;
			GrandStrategyGame.Provinces[i]->Water = false;
			GrandStrategyGame.Provinces[i]->Coastal = false;
			GrandStrategyGame.Provinces[i]->Movement = false;
			GrandStrategyGame.Provinces[i]->SettlementLocation.x = -1;
			GrandStrategyGame.Provinces[i]->SettlementLocation.y = -1;
			for (int j = 0; j < MAX_RACES; ++j) {
				GrandStrategyGame.Provinces[i]->CulturalNames[j] = "";
				GrandStrategyGame.Provinces[i]->CulturalSettlementNames[j] = "";
				for (int k = 0; k < FactionMax; ++k) {
					GrandStrategyGame.Provinces[i]->FactionCulturalNames[j][k] = "";
					GrandStrategyGame.Provinces[i]->FactionCulturalSettlementNames[j][k] = "";
				}
			}
			for (int j = 0; j < MAX_RACES * FactionMax; ++j) {
				GrandStrategyGame.Provinces[i]->Claims[j][0] = -1;
				GrandStrategyGame.Provinces[i]->Claims[j][1] = -1;
			}
			for (size_t j = 0; j < UnitTypes.size(); ++j) {
				GrandStrategyGame.Provinces[i]->SettlementBuildings[j] = false;
				GrandStrategyGame.Provinces[i]->Units[j] = 0;
				GrandStrategyGame.Provinces[i]->UnderConstructionUnits[j] = 0;
				GrandStrategyGame.Provinces[i]->MovingUnits[j] = 0;
				GrandStrategyGame.Provinces[i]->AttackingUnits[j] = 0;
			}
			for (int j = 0; j < ProvinceMax; ++j) {
				GrandStrategyGame.Provinces[i]->BorderProvinces[j] = -1;
			}
			for (int j = 0; j < ProvinceTileMax; ++j) {
				GrandStrategyGame.Provinces[i]->Tiles[j].x = -1;
				GrandStrategyGame.Provinces[i]->Tiles[j].y = -1;
			}
			for (int j = 0; j < MaxCosts; ++j) {
				GrandStrategyGame.Provinces[i]->Income[j] = 0;
				GrandStrategyGame.Provinces[i]->ProductionCapacity[j] = 0;
				GrandStrategyGame.Provinces[i]->ProductionCapacityFulfilled[j] = 0;
				GrandStrategyGame.Provinces[i]->ProductionEfficiencyModifier[j] = 0;
				for (int k = 0; k < ProvinceTileMax; ++k) {
					GrandStrategyGame.Provinces[i]->ResourceTiles[j][k].x = -1;
					GrandStrategyGame.Provinces[i]->ResourceTiles[j][k].y = -1;
				}
			}
			GrandStrategyGame.Provinces[i]->Heroes.clear();
		} else {
			break;
		}
	}
	
	for (int i = 0; i < MAX_RACES; ++i) {
		for (int j = 0; j < FactionMax; ++j) {
			if (GrandStrategyGame.Factions[i][j]) {
				GrandStrategyGame.Factions[i][j]->CurrentResearch = -1;
				GrandStrategyGame.Factions[i][j]->ProvinceCount = 0;
				for (size_t k = 0; k < AllUpgrades.size(); ++k) {
					GrandStrategyGame.Factions[i][j]->Technologies[k] = false;
				}
				for (int k = 0; k < MaxCosts; ++k) {
					GrandStrategyGame.Factions[i][j]->Resources[k] = 0;
					GrandStrategyGame.Factions[i][j]->Income[k] = 0;
					GrandStrategyGame.Factions[i][j]->ProductionEfficiencyModifier[k] = 0;
					GrandStrategyGame.Factions[i][j]->Trade[k] = 0;
				}
				for (int k = 0; k < ProvinceMax; ++k) {
					GrandStrategyGame.Factions[i][j]->OwnedProvinces[k] = -1;
				}
				for (size_t k = 0; k < UnitTypes.size(); ++k) {
					GrandStrategyGame.Factions[i][j]->MilitaryScoreBonus[k] = 0;
				}
			} else {
				break;
			}
		}
	}
	
	for (int i = 0; i < RiverMax; ++i) {
		if (GrandStrategyGame.Rivers[i] && !GrandStrategyGame.Rivers[i]->Name.empty()) {
			GrandStrategyGame.Rivers[i]->Name = "";
			for (int j = 0; j < MAX_RACES; ++j) {
				GrandStrategyGame.Rivers[i]->CulturalNames[j] = "";
			}
		} else {
			break;
		}
	}
	
	for (int i = 0; i < MaxCosts; ++i) {
		GrandStrategyGame.CommodityPrices[i] = 0;
		for (int j = 0; j < WorldMapResourceMax; ++j) {
			if (GrandStrategyGame.WorldMapResources[i][j].x != -1 || GrandStrategyGame.WorldMapResources[i][j].y != -1) {
				GrandStrategyGame.WorldMapResources[i][j].x = -1;
				GrandStrategyGame.WorldMapResources[i][j].y = -1;
			} else {
				break;
			}
		}
	}
	
	for (size_t i = 0; i < GrandStrategyGame.Heroes.size(); ++i) {
		GrandStrategyGame.Heroes[i]->State = 0;
		GrandStrategyGame.Heroes[i]->Province = NULL;
	}
			
	GrandStrategyGame.WorldMapWidth = 0;
	GrandStrategyGame.WorldMapHeight = 0;
	GrandStrategyGame.ProvinceCount = 0;
	GrandStrategyGame.SelectedProvince = -1;
	GrandStrategyGame.PlayerFaction = NULL;
	
	//destroy minimap surface
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		if (GrandStrategyGame.MinimapSurfaceGL) {
			glDeleteTextures(1, &GrandStrategyGame.MinimapTexture);
			delete[] GrandStrategyGame.MinimapSurfaceGL;
			GrandStrategyGame.MinimapSurfaceGL = NULL;
		}
	} else
#endif
	{
		if (GrandStrategyGame.MinimapSurface) {
			VideoPaletteListRemove(GrandStrategyGame.MinimapSurface);
			SDL_FreeSurface(GrandStrategyGame.MinimapSurface);
			GrandStrategyGame.MinimapSurface = NULL;
		}
	}
	
	GrandStrategyGame.MinimapTextureWidth = 0;
	GrandStrategyGame.MinimapTextureHeight = 0;
	GrandStrategyGame.MinimapTileWidth = 0;
	GrandStrategyGame.MinimapTileHeight = 0;
	GrandStrategyGame.MinimapOffsetX = 0;
	GrandStrategyGame.MinimapOffsetY = 0;
	
	WorldMapOffsetX = 0;
	WorldMapOffsetY = 0;
	GrandStrategyMapWidthIndent = 0;
	GrandStrategyMapHeightIndent = 0;
}

void InitializeGrandStrategyGame()
{
	//do the same for the fog tile now
	std::string fog_graphic_tile = "tilesets/world/terrain/fog.png";
	if (CGraphic::Get(fog_graphic_tile) == NULL) {
		CGraphic *fog_tile_graphic = CGraphic::New(fog_graphic_tile, 96, 96);
		fog_tile_graphic->Load();
	}
	GrandStrategyGame.FogTile = CGraphic::Get(fog_graphic_tile);
	
	//and for the resource buildings
	for (int i = 0; i < MaxCosts; ++i) {
		std::string resource_building_graphics_file = "tilesets/world/sites/resource_building_" + DefaultResourceNames[i] + ".png";
		if (CanAccessFile(resource_building_graphics_file.c_str())) {
			if (CGraphic::Get(resource_building_graphics_file) == NULL) {
				CGraphic *resource_building_graphics = CGraphic::New(resource_building_graphics_file, 64, 64);
				resource_building_graphics->Load();
			}
			GrandStrategyGame.ResourceBuildingGraphics[i] = CGraphic::Get(resource_building_graphics_file);
		}
	}
	
	// set the settlement graphics
	for (int i = 0; i < MAX_RACES; ++i) {
		std::string settlement_graphics_file = "tilesets/world/sites/";
		settlement_graphics_file += PlayerRaces.Name[i];
		settlement_graphics_file += "_settlement.png";
		if (!CanAccessFile(settlement_graphics_file.c_str()) && PlayerRaces.ParentCivilization[i] != -1) {
			settlement_graphics_file = FindAndReplaceString(settlement_graphics_file, PlayerRaces.Name[i], PlayerRaces.Name[PlayerRaces.ParentCivilization[i]]);
		}
		if (CanAccessFile(settlement_graphics_file.c_str())) {
			if (CPlayerColorGraphic::Get(settlement_graphics_file) == NULL) {
				CPlayerColorGraphic *settlement_graphics = CPlayerColorGraphic::New(settlement_graphics_file, 64, 64);
				settlement_graphics->Load();
			}
			GrandStrategyGame.SettlementGraphics[i] = CPlayerColorGraphic::Get(settlement_graphics_file);
		}
		
		std::string barracks_graphics_file = "tilesets/world/sites/";
		barracks_graphics_file += PlayerRaces.Name[i];
		barracks_graphics_file += "_barracks.png";
		if (!CanAccessFile(barracks_graphics_file.c_str()) && PlayerRaces.ParentCivilization[i] != -1) {
			barracks_graphics_file = FindAndReplaceString(barracks_graphics_file, PlayerRaces.Name[i], PlayerRaces.Name[PlayerRaces.ParentCivilization[i]]);
		}
		if (CanAccessFile(barracks_graphics_file.c_str())) {
			if (CPlayerColorGraphic::Get(barracks_graphics_file) == NULL) {
				CPlayerColorGraphic *barracks_graphics = CPlayerColorGraphic::New(barracks_graphics_file, 64, 64);
				barracks_graphics->Load();
			}
			GrandStrategyGame.BarracksGraphics[i] = CPlayerColorGraphic::Get(barracks_graphics_file);
		}
	}
	
	// set the border graphics
	for (int i = 0; i < MaxDirections; ++i) {
		std::string border_graphics_file = "tilesets/world/terrain/";
		border_graphics_file += "province_border_";
		
		std::string national_border_graphics_file = "tilesets/world/terrain/";
		national_border_graphics_file += "province_national_border_";
		
		if (i == North) {
			border_graphics_file += "north";
			national_border_graphics_file += "north";
		} else if (i == Northeast) {
			border_graphics_file += "northeast_inner";
			national_border_graphics_file += "northeast_inner";
		} else if (i == East) {
			border_graphics_file += "east";
			national_border_graphics_file += "east";
		} else if (i == Southeast) {
			border_graphics_file += "southeast_inner";
			national_border_graphics_file += "southeast_inner";
		} else if (i == South) {
			border_graphics_file += "south";
			national_border_graphics_file += "south";
		} else if (i == Southwest) {
			border_graphics_file += "southwest_inner";
			national_border_graphics_file += "southwest_inner";
		} else if (i == West) {
			border_graphics_file += "west";
			national_border_graphics_file += "west";
		} else if (i == Northwest) {
			border_graphics_file += "northwest_inner";
			national_border_graphics_file += "northwest_inner";
		}
		
		border_graphics_file += ".png";
		national_border_graphics_file += ".png";
		
		if (CGraphic::Get(border_graphics_file) == NULL) {
			CGraphic *border_graphics = CGraphic::New(border_graphics_file, 84, 84);
			border_graphics->Load();
		}
		GrandStrategyGame.BorderGraphics[i] = CGraphic::Get(border_graphics_file);
		
		if (CPlayerColorGraphic::Get(national_border_graphics_file) == NULL) {
			CPlayerColorGraphic *national_border_graphics = CPlayerColorGraphic::New(national_border_graphics_file, 84, 84);
			national_border_graphics->Load();
		}
		GrandStrategyGame.NationalBorderGraphics[i] = CPlayerColorGraphic::Get(national_border_graphics_file);
	}
	
	// set the river and road graphics
	for (int i = 0; i < MaxDirections; ++i) {
		std::string river_graphics_file = "tilesets/world/terrain/";
		river_graphics_file += "river_";
		
		std::string rivermouth_graphics_file = "tilesets/world/terrain/";
		rivermouth_graphics_file += "rivermouth_";
		
		std::string riverhead_graphics_file = "tilesets/world/terrain/";
		riverhead_graphics_file += "riverhead_";
		
		std::string trail_graphics_file = "tilesets/world/terrain/";
		trail_graphics_file += "trail_";
		
		std::string road_graphics_file = "tilesets/world/terrain/";
		road_graphics_file += "road_";
		
		if (i == North) {
			river_graphics_file += "north";
			rivermouth_graphics_file += "north";
			riverhead_graphics_file += "north";
			trail_graphics_file += "north";
			road_graphics_file += "north";
		} else if (i == Northeast) {
			river_graphics_file += "northeast_inner";
			rivermouth_graphics_file += "northeast";
			riverhead_graphics_file += "northeast";
			trail_graphics_file += "northeast";
			road_graphics_file += "northeast";
		} else if (i == East) {
			river_graphics_file += "east";
			rivermouth_graphics_file += "east";
			riverhead_graphics_file += "east";
			trail_graphics_file += "east";
			road_graphics_file += "east";
		} else if (i == Southeast) {
			river_graphics_file += "southeast_inner";
			rivermouth_graphics_file += "southeast";
			riverhead_graphics_file += "southeast";
			trail_graphics_file += "southeast";
			road_graphics_file += "southeast";
		} else if (i == South) {
			river_graphics_file += "south";
			rivermouth_graphics_file += "south";
			riverhead_graphics_file += "south";
			trail_graphics_file += "south";
			road_graphics_file += "south";
		} else if (i == Southwest) {
			river_graphics_file += "southwest_inner";
			rivermouth_graphics_file += "southwest";
			riverhead_graphics_file += "southwest";
			trail_graphics_file += "southwest";
			road_graphics_file += "southwest";
		} else if (i == West) {
			river_graphics_file += "west";
			rivermouth_graphics_file += "west";
			riverhead_graphics_file += "west";
			trail_graphics_file += "west";
			road_graphics_file += "west";
		} else if (i == Northwest) {
			river_graphics_file += "northwest_inner";
			rivermouth_graphics_file += "northwest";
			riverhead_graphics_file += "northwest";
			trail_graphics_file += "northwest";
			road_graphics_file += "northwest";
		}
		
		std::string rivermouth_flipped_graphics_file;
		if (i == North || i == East || i == South || i == West) { //only non-diagonal directions get flipped rivermouth graphics
			rivermouth_flipped_graphics_file = rivermouth_graphics_file + "_flipped" + ".png";
		}
		
		std::string riverhead_flipped_graphics_file;
		if (i == North || i == East || i == South || i == West) { //only non-diagonal directions get flipped riverhead graphics
			riverhead_flipped_graphics_file = riverhead_graphics_file + "_flipped" + ".png";
		}
		
		river_graphics_file += ".png";
		rivermouth_graphics_file += ".png";
		riverhead_graphics_file += ".png";
		trail_graphics_file += ".png";
		road_graphics_file += ".png";
		
		if (CGraphic::Get(river_graphics_file) == NULL) {
			CGraphic *river_graphics = CGraphic::New(river_graphics_file, 84, 84);
			river_graphics->Load();
		}
		GrandStrategyGame.RiverGraphics[i] = CGraphic::Get(river_graphics_file);
		
		if (CGraphic::Get(rivermouth_graphics_file) == NULL) {
			CGraphic *rivermouth_graphics = CGraphic::New(rivermouth_graphics_file, 84, 84);
			rivermouth_graphics->Load();
		}
		GrandStrategyGame.RivermouthGraphics[i][0] = CGraphic::Get(rivermouth_graphics_file);
		
		if (!rivermouth_flipped_graphics_file.empty()) {
			if (CGraphic::Get(rivermouth_flipped_graphics_file) == NULL) {
				CGraphic *rivermouth_flipped_graphics = CGraphic::New(rivermouth_flipped_graphics_file, 84, 84);
				rivermouth_flipped_graphics->Load();
			}
			GrandStrategyGame.RivermouthGraphics[i][1] = CGraphic::Get(rivermouth_flipped_graphics_file);
		}
		
		if (CGraphic::Get(riverhead_graphics_file) == NULL) {
			CGraphic *riverhead_graphics = CGraphic::New(riverhead_graphics_file, 84, 84);
			riverhead_graphics->Load();
		}
		GrandStrategyGame.RiverheadGraphics[i][0] = CGraphic::Get(riverhead_graphics_file);
		
		if (!riverhead_flipped_graphics_file.empty()) {
			if (CGraphic::Get(riverhead_flipped_graphics_file) == NULL) {
				CGraphic *riverhead_flipped_graphics = CGraphic::New(riverhead_flipped_graphics_file, 84, 84);
				riverhead_flipped_graphics->Load();
			}
			GrandStrategyGame.RiverheadGraphics[i][1] = CGraphic::Get(riverhead_flipped_graphics_file);
		}
		
		if (CGraphic::Get(road_graphics_file) == NULL) { //use road graphics file for trails for now
			CGraphic *trail_graphics = CGraphic::New(road_graphics_file, 64, 64);
			trail_graphics->Load();
		}
		GrandStrategyGame.PathwayGraphics[PathwayTrail][i] = CGraphic::Get(road_graphics_file);
		
		if (CGraphic::Get(road_graphics_file) == NULL) {
			CGraphic *road_graphics = CGraphic::New(road_graphics_file, 64, 64);
			road_graphics->Load();
		}
		GrandStrategyGame.PathwayGraphics[PathwayRoad][i] = CGraphic::Get(road_graphics_file);
	}
	
	//load the move symbol
	std::string move_symbol_filename = "tilesets/world/sites/move.png";
	if (CGraphic::Get(move_symbol_filename) == NULL) {
		CGraphic *move_symbol_graphic = CGraphic::New(move_symbol_filename, 64, 64);
		move_symbol_graphic->Load();
	}
	GrandStrategyGame.SymbolMove = CGraphic::Get(move_symbol_filename);
	
	//load the attack symbol
	std::string attack_symbol_filename = "tilesets/world/sites/attack.png";
	if (CGraphic::Get(attack_symbol_filename) == NULL) {
		CGraphic *attack_symbol_graphic = CGraphic::New(attack_symbol_filename, 64, 64);
		attack_symbol_graphic->Load();
	}
	GrandStrategyGame.SymbolAttack = CGraphic::Get(attack_symbol_filename);
	
	//load the hero symbol
	std::string hero_symbol_filename = "tilesets/world/sites/hero.png";
	if (CGraphic::Get(hero_symbol_filename) == NULL) {
		CGraphic *hero_symbol_graphic = CGraphic::New(hero_symbol_filename, 64, 64);
		hero_symbol_graphic->Load();
	}
	GrandStrategyGame.SymbolHero = CGraphic::Get(hero_symbol_filename);
	
	//create grand strategy faction instances for all defined factions
	for (int i = 0; i < MAX_RACES; ++i) {
		for (int j = 0; j < FactionMax; ++j) {
			if (GrandStrategyGame.Factions[i][j]) { // no need to create a grand strategy instance for an already-created faction again
				continue;
			}
			
			if (PlayerRaces.Factions[i][j] && !PlayerRaces.Factions[i][j]->Name.empty()) { //if the faction is defined
				CGrandStrategyFaction *faction = new CGrandStrategyFaction;
				GrandStrategyGame.Factions[i][j] = faction;
				
				GrandStrategyGame.Factions[i][j]->Civilization = i;
				GrandStrategyGame.Factions[i][j]->Faction = j;
			} else {
				break;
			}
		}
	}
	
	//set resource prices to base prices
	for (int i = 0; i < MaxCosts; ++i) {
		GrandStrategyGame.CommodityPrices[i] = DefaultResourcePrices[i];
	}
}

void InitializeGrandStrategyMinimap()
{
	//calculate the minimap texture width and height
	if (GrandStrategyGame.WorldMapWidth >= GrandStrategyGame.WorldMapHeight) {
		GrandStrategyGame.MinimapTextureWidth = UI.Minimap.W;
		GrandStrategyGame.MinimapTextureHeight = UI.Minimap.H * GrandStrategyGame.WorldMapHeight / GrandStrategyGame.WorldMapWidth;
	} else {
		GrandStrategyGame.MinimapTextureWidth = UI.Minimap.W * GrandStrategyGame.WorldMapWidth / GrandStrategyGame.WorldMapHeight;
		GrandStrategyGame.MinimapTextureHeight = UI.Minimap.H;
	}

	//calculate the minimap tile width and height
	GrandStrategyGame.MinimapTileWidth = UI.Minimap.W * 1000 / GetWorldMapWidth();
	GrandStrategyGame.MinimapTileHeight = UI.Minimap.H * 1000 / GetWorldMapHeight();
	if (GetWorldMapWidth() >= GetWorldMapHeight()) {
		GrandStrategyGame.MinimapTileHeight = GrandStrategyGame.MinimapTileWidth;
	} else {
		GrandStrategyGame.MinimapTileWidth = GrandStrategyGame.MinimapTileHeight;
	}

	// create minimap surface
	#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		if (!GrandStrategyGame.MinimapSurfaceGL) {
			GrandStrategyGame.MinimapSurfaceGL = new unsigned char[GrandStrategyGame.MinimapTextureWidth * GrandStrategyGame.MinimapTextureHeight * 4];
			memset(GrandStrategyGame.MinimapSurfaceGL, 0, GrandStrategyGame.MinimapTextureWidth * GrandStrategyGame.MinimapTextureHeight * 4);
		}
		GrandStrategyGame.CreateMinimapTexture();
	} else
	#endif
	{
		if (!GrandStrategyGame.MinimapSurface) {
			GrandStrategyGame.MinimapSurface = SDL_CreateRGBSurface(SDL_SWSURFACE,  GrandStrategyGame.MinimapTextureWidth, GrandStrategyGame.MinimapTextureHeight, 32, TheScreen->format->Rmask, TheScreen->format->Gmask, TheScreen->format->Bmask, 0);
		}
	}

	GrandStrategyGame.UpdateMinimap();
	
	GrandStrategyGame.MinimapOffsetX = 0;
	GrandStrategyGame.MinimapOffsetY = 0;
	if (GetWorldMapWidth() <= UI.Minimap.W && GetWorldMapHeight() <= UI.Minimap.H) {
		if (GetWorldMapWidth() >= GetWorldMapHeight()) {
			GrandStrategyGame.MinimapOffsetY = (UI.Minimap.H - (GetWorldMapHeight() * std::max(GrandStrategyGame.MinimapTileHeight / 1000, 1))) / 2;
		} else {
			GrandStrategyGame.MinimapOffsetX = (UI.Minimap.W - (GetWorldMapWidth() * std::max(GrandStrategyGame.MinimapTileWidth / 1000, 1))) / 2;
		}
	} else {
		if (GetWorldMapWidth() >= GetWorldMapHeight()) {
			GrandStrategyGame.MinimapOffsetY = (UI.Minimap.H - ((GetWorldMapHeight() / std::max(1000 / GrandStrategyGame.MinimapTileHeight, 1)) * std::max(GrandStrategyGame.MinimapTileHeight / 1000, 1))) / 2;
		} else {
			GrandStrategyGame.MinimapOffsetX = (UI.Minimap.H - ((GetWorldMapWidth() / std::max(1000 / GrandStrategyGame.MinimapTileWidth, 1)) * std::max(GrandStrategyGame.MinimapTileWidth / 1000, 1))) / 2;
		}
	}
}

void SetGrandStrategyWorld(std::string world)
{
	GrandStrategyWorld = world;
}

void DoGrandStrategyTurn()
{
	GrandStrategyGame.DoTurn();
}

void DoProspection()
{
	GrandStrategyGame.DoProspection();
}

void CalculateProvinceBorders()
{
	for (int i = 0; i < ProvinceMax; ++i) {
		if (GrandStrategyGame.Provinces[i] && !GrandStrategyGame.Provinces[i]->Name.empty()) {
			for (int j = 0; j < ProvinceTileMax; ++j) {
				if (GrandStrategyGame.Provinces[i]->Tiles[j].x != -1 && GrandStrategyGame.Provinces[i]->Tiles[j].y != -1) {
					GrandStrategyGame.WorldMapTiles[GrandStrategyGame.Provinces[i]->Tiles[j].x][GrandStrategyGame.Provinces[i]->Tiles[j].y]->Province = i; //tell the tile it belongs to this province
				} else {
					break;
				}
			}
			
			for (int j = 0; j < ProvinceMax; ++j) { //clean border provinces
				if (GrandStrategyGame.Provinces[i]->BorderProvinces[j] == -1) {
					break;
				}
				GrandStrategyGame.Provinces[i]->BorderProvinces[j] = -1;
			}
			
			//calculate which of the province's tiles are border tiles, and which provinces it borders; also whether the province borders water (is coastal) or not
			int border_province_count = 0;
			for (int j = 0; j < ProvinceTileMax; ++j) {
				if (GrandStrategyGame.Provinces[i]->Tiles[j].x != -1 && GrandStrategyGame.Provinces[i]->Tiles[j].y != -1) {
					int x = GrandStrategyGame.Provinces[i]->Tiles[j].x;
					int y = GrandStrategyGame.Provinces[i]->Tiles[j].y;
					for (int sub_x = -1; sub_x <= 1; ++sub_x) {
						if ((x + sub_x) < 0 || (x + sub_x) >= GrandStrategyGame.WorldMapWidth) {
							continue;
						}
							
						for (int sub_y = -1; sub_y <= 1; ++sub_y) {
							if ((y + sub_y) < 0 || (y + sub_y) >= GrandStrategyGame.WorldMapHeight) {
								continue;
							}
							
							int second_province_id = GrandStrategyGame.WorldMapTiles[x + sub_x][y + sub_y]->Province;
							if (!(sub_x == 0 && sub_y == 0) && second_province_id != i && GrandStrategyGame.WorldMapTiles[x + sub_x][y + sub_y]->Terrain != -1) {
								if (second_province_id == -1 || GrandStrategyGame.Provinces[i]->Water == GrandStrategyGame.Provinces[second_province_id]->Water) {
									int direction = DirectionToHeading(Vec2i(x + sub_x, y + sub_y) - Vec2i(x, y)) + (32 / 2);
									if (direction % 32 != 0) {
										direction = direction - (direction % 32);
									}
									direction = direction / 32;
									
									GrandStrategyGame.WorldMapTiles[x][y]->Borders[direction] = true;
								}
								
								if (second_province_id != -1 && !GrandStrategyGame.Provinces[i]->BordersProvince(second_province_id)) { //if isn't added yet to the border provinces, do so now
									GrandStrategyGame.Provinces[i]->BorderProvinces[border_province_count] = second_province_id;
									border_province_count += 1;
								}
								
								if (second_province_id != -1 && GrandStrategyGame.Provinces[i]->Water == false && GrandStrategyGame.Provinces[second_province_id]->Water == true) {
									GrandStrategyGame.Provinces[i]->Coastal = true;
								}
							}
						}
					}
				} else {
					break;
				}
			}
		}
	}				
}

void CenterGrandStrategyMapOnTile(int x, int y)
{
	WorldMapOffsetX = x - (((UI.MapArea.EndX - UI.MapArea.X) / 64) / 2);
	if (WorldMapOffsetX < 0) {
		WorldMapOffsetX = 0;
	} else if (WorldMapOffsetX > GetWorldMapWidth() - 1 - ((UI.MapArea.EndX - UI.MapArea.X) / 64)) {
		WorldMapOffsetX = GetWorldMapWidth() - 1 - ((UI.MapArea.EndX - UI.MapArea.X) / 64);
	}

	WorldMapOffsetY = y - (((UI.MapArea.EndY - UI.MapArea.Y) / 64) / 2);
	if (WorldMapOffsetY < 0) {
		WorldMapOffsetY = 0;
	} else if (WorldMapOffsetY > GetWorldMapHeight() - 1 - ((UI.MapArea.EndY - UI.MapArea.Y) / 64)) {
		WorldMapOffsetY = GetWorldMapHeight() - 1 - ((UI.MapArea.EndY - UI.MapArea.Y) / 64);
	}
}

bool ProvinceBordersProvince(std::string province_name, std::string second_province_name)
{
	int province = GetProvinceId(province_name);
	int second_province = GetProvinceId(second_province_name);
	
	return GrandStrategyGame.Provinces[province]->BordersProvince(second_province);
}

bool ProvinceBordersFaction(std::string province_name, std::string faction_civilization_name, std::string faction_name)
{
	int province = GetProvinceId(province_name);
	int civilization = PlayerRaces.GetRaceIndexByName(faction_civilization_name.c_str());
	int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	
	if (civilization == -1 || faction == -1) {
		return false;
	}
	
	return GrandStrategyGame.Provinces[province]->BordersFaction(civilization, faction);
}

bool ProvinceHasBuildingClass(std::string province_name, std::string building_class)
{
	int province_id = GetProvinceId(province_name);
	
	return GrandStrategyGame.Provinces[province_id]->HasBuildingClass(building_class);
}

bool ProvinceHasClaim(std::string province_name, std::string faction_civilization_name, std::string faction_name)
{
	int province = GetProvinceId(province_name);
	int civilization = PlayerRaces.GetRaceIndexByName(faction_civilization_name.c_str());
	int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	
	if (civilization == -1 || faction == -1) {
		return false;
	}
	
	return GrandStrategyGame.Provinces[province]->HasFactionClaim(civilization, faction);
}

bool ProvinceHasResource(std::string province_name, std::string resource_name, bool ignore_prospection)
{
	int province_id = GetProvinceId(province_name);
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return false;
	}
	
	return GrandStrategyGame.Provinces[province_id]->HasResource(resource, ignore_prospection);
}

bool IsGrandStrategyBuilding(const CUnitType &type)
{
	if (type.BoolFlag[BUILDING_INDEX].value && !type.Class.empty() && type.Class != "farm" && type.Class != "watch-tower" && type.Class != "guard-tower") {
		return true;
	}
	return false;
}

std::string GetProvinceCivilization(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (GrandStrategyGame.Provinces[province_id]->Civilization != -1) {
		return PlayerRaces.Name[GrandStrategyGame.Provinces[province_id]->Civilization];
	} else {
		return "";
	}
}

bool GetProvinceSettlementBuilding(std::string province_name, std::string building_ident)
{
	int province_id = GetProvinceId(province_name);
	int building_id = UnitTypeIdByIdent(building_ident);
	
	return GrandStrategyGame.Provinces[province_id]->SettlementBuildings[building_id];
}

std::string GetProvinceCurrentConstruction(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	if (province_id != -1) {
		if (GrandStrategyGame.Provinces[province_id]->CurrentConstruction != -1) {
			return UnitTypes[GrandStrategyGame.Provinces[province_id]->CurrentConstruction]->Ident;
		}
	}
	
	return "";
}

int GetProvinceUnitQuantity(std::string province_name, std::string unit_type_ident)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	return GrandStrategyGame.Provinces[province_id]->Units[unit_type];
}

int GetProvinceUnderConstructionUnitQuantity(std::string province_name, std::string unit_type_ident)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	return GrandStrategyGame.Provinces[province_id]->UnderConstructionUnits[unit_type];
}

int GetProvinceMovingUnitQuantity(std::string province_name, std::string unit_type_ident)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	return GrandStrategyGame.Provinces[province_id]->MovingUnits[unit_type];
}

int GetProvinceAttackingUnitQuantity(std::string province_name, std::string unit_type_ident)
{
	int province_id = GetProvinceId(province_name);
	int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
	
	return GrandStrategyGame.Provinces[province_id]->AttackingUnits[unit_type_id];
}

int GetProvinceHero(std::string province_name, std::string hero_ident)
{
	int province_id = GetProvinceId(province_name);

	if (province_id == -1) {
		fprintf(stderr, "Can't find %s province.\n", province_name.c_str());
		return 0;
	}
	
	int unit_type_id = UnitTypeIdByIdent(hero_ident);
	
	if (unit_type_id == -1) {
		fprintf(stderr, "Can't find %s unit type.\n", hero_ident.c_str());
		return 0;
	}
	
	for (size_t i = 0; i < GrandStrategyGame.Provinces[province_id]->Heroes.size(); ++i) {
		if (GrandStrategyGame.Provinces[province_id]->Heroes[i]->Type->Slot == UnitTypes[unit_type_id]->Slot) {
			return GrandStrategyGame.Provinces[province_id]->Heroes[i]->State;
		}
	}
	return 0;
}

int GetProvinceLabor(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	return GrandStrategyGame.Provinces[province_id]->Labor;
}

int GetProvinceAvailableWorkersForTraining(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	return GrandStrategyGame.Provinces[province_id]->Labor / 100;
}

int GetProvinceTotalWorkers(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	return GrandStrategyGame.Provinces[province_id]->TotalWorkers;
}

int GetProvinceMilitaryScore(std::string province_name, bool attacker, bool count_defenders)
{
	int province_id = GetProvinceId(province_name);
	
	int military_score = 0;
	if (province_id != -1) {
		if (attacker) {
			military_score = GrandStrategyGame.Provinces[province_id]->AttackingMilitaryScore;
		} else if (count_defenders) {
			military_score = GrandStrategyGame.Provinces[province_id]->MilitaryScore;
		} else {
			military_score = GrandStrategyGame.Provinces[province_id]->OffensiveMilitaryScore;
		}
	}
	
	return std::max(1, military_score); // military score must be at least one, since it is a divider in some instances, and we don't want to divide by 0
}

std::string GetProvinceOwner(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id == -1 || GrandStrategyGame.Provinces[province_id]->Owner == NULL) {
		return "";
	}
	
	return PlayerRaces.Factions[GrandStrategyGame.Provinces[province_id]->Owner->Civilization][GrandStrategyGame.Provinces[province_id]->Owner->Faction]->Name;
}

bool GetProvinceWater(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id == -1) {
		return false;
	}
	
	return GrandStrategyGame.Provinces[province_id]->Water;
}

void SetPlayerFaction(std::string civilization_name, std::string faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	if (faction == -1) {
		return;
	}
	
	GrandStrategyGame.PlayerFaction = const_cast<CGrandStrategyFaction *>(&(*GrandStrategyGame.Factions[civilization][faction]));
}

void SetFactionResource(std::string civilization_name, std::string faction_name, std::string resource_name, int resource_quantity)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return;
	}
	
	GrandStrategyGame.Factions[civilization][faction]->Resources[resource] = resource_quantity;
}

void ChangeFactionResource(std::string civilization_name, std::string faction_name, std::string resource_name, int resource_quantity)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return;
	}
	
	GrandStrategyGame.Factions[civilization][faction]->Resources[resource] += resource_quantity;
}

int GetFactionResource(std::string civilization_name, std::string faction_name, std::string resource_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return 0;
	}
	
	return GrandStrategyGame.Factions[civilization][faction]->Resources[resource];
}

void CalculateFactionIncomes(std::string civilization_name, std::string faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	if (faction == -1 || GrandStrategyGame.Factions[civilization][faction]->ProvinceCount == 0) {
		return;
	}
	
	GrandStrategyGame.Factions[civilization][faction]->CalculateIncomes();
}

int GetFactionIncome(std::string civilization_name, std::string faction_name, std::string resource_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return 0;
	}
	
	return GrandStrategyGame.Factions[civilization][faction]->Income[resource];
}

void SetFactionTechnology(std::string civilization_name, std::string faction_name, std::string upgrade_ident, bool has_technology)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int upgrade_id = UpgradeIdByIdent(upgrade_ident);
	if (civilization != -1 && upgrade_id != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
		if (faction != -1) {
			GrandStrategyGame.Factions[civilization][faction]->SetTechnology(upgrade_id, has_technology);
		}
	}
}

bool GetFactionTechnology(std::string civilization_name, std::string faction_name, std::string upgrade_ident)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int upgrade_id = UpgradeIdByIdent(upgrade_ident);
	if (civilization != -1 && upgrade_id != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
		if (faction != -1) {
			return GrandStrategyGame.Factions[civilization][faction]->Technologies[upgrade_id];
		}
	}
	
	return false;
}

void SetFactionCurrentResearch(std::string civilization_name, std::string faction_name, std::string upgrade_ident)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int upgrade_id;
	if (!upgrade_ident.empty()) {
		upgrade_id = UpgradeIdByIdent(upgrade_ident);
	} else {
		upgrade_id = -1;
	}
	if (civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
		if (faction != -1) {
			GrandStrategyGame.Factions[civilization][faction]->CurrentResearch = upgrade_id;
		}
	}
}

std::string GetFactionCurrentResearch(std::string civilization_name, std::string faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	if (civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
		if (faction != -1) {
			if (GrandStrategyGame.Factions[civilization][faction]->CurrentResearch != -1) {
				return AllUpgrades[GrandStrategyGame.Factions[civilization][faction]->CurrentResearch]->Ident;
			}
		}
	}
	
	return "";
}

void AcquireFactionTechnologies(std::string civilization_from_name, std::string faction_from_name, std::string civilization_to_name, std::string faction_to_name)
{
	int civilization_from = PlayerRaces.GetRaceIndexByName(civilization_from_name.c_str());
	int civilization_to = PlayerRaces.GetRaceIndexByName(civilization_to_name.c_str());
	if (civilization_from != -1 && civilization_to != -1) {
		int faction_from = PlayerRaces.GetFactionIndexByName(civilization_from, faction_from_name);
		int faction_to = PlayerRaces.GetFactionIndexByName(civilization_to, faction_to_name);
		if (faction_from != -1 && faction_to != -1) {
			for (size_t i = 0; i < AllUpgrades.size(); ++i) {
				if (GrandStrategyGame.Factions[civilization_from][faction_from]->Technologies[i]) {
					GrandStrategyGame.Factions[civilization_to][faction_to]->SetTechnology(i, true);
				}
			}
		}
	}
}

bool IsGrandStrategyUnit(const CUnitType &type)
{
	if (!type.BoolFlag[BUILDING_INDEX].value && !type.BoolFlag[HERO_INDEX].value && type.DefaultStat.Variables[DEMAND_INDEX].Value > 0 && type.Class != "caravan") {
		return true;
	}
	return false;
}

bool IsMilitaryUnit(const CUnitType &type)
{
	if (IsGrandStrategyUnit(type) && type.Class != "worker") {
		return true;
	}
	return false;
}

void CreateProvinceUnits(std::string province_name, int player, int divisor, bool attacking_units, bool ignore_militia)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id == -1) {
		return;
	}
	
	for (size_t i = 0; i < UnitTypes.size(); ++i) {
		int units_to_be_created = 0;
		if (IsMilitaryUnit(*UnitTypes[i]) && UnitTypes[i]->Class != "militia") {
			if (!attacking_units) {
				units_to_be_created = GrandStrategyGame.Provinces[province_id]->Units[i] / divisor;
				GrandStrategyGame.Provinces[province_id]->ChangeUnitQuantity(i, - units_to_be_created);
			} else {
				units_to_be_created = GrandStrategyGame.Provinces[province_id]->AttackingUnits[i] / divisor;
				GrandStrategyGame.Provinces[province_id]->ChangeAttackingUnitQuantity(i, - units_to_be_created);
			}
		} else if (!attacking_units && UnitTypes[i]->Class == "worker" && !ignore_militia && !UnitTypes[i]->Civilization.empty()) { // create militia in the province depending on the amount of workers
			
			int militia_unit_type = PlayerRaces.GetCivilizationClassUnitType(PlayerRaces.GetRaceIndexByName(UnitTypes[i]->Civilization.c_str()), GetUnitTypeClassIndexByName("militia"));
			
			if (militia_unit_type != -1) {
				units_to_be_created = GrandStrategyGame.Provinces[province_id]->Units[militia_unit_type] / 2 / divisor; //half of the worker population as militia
			}
		}
		
		if (units_to_be_created > 0) {
			units_to_be_created *= BattalionMultiplier;
			for (int j = 0; j < units_to_be_created; ++j) {
				CUnit *unit = MakeUnit(*UnitTypes[i], &Players[player]);
				if (unit == NULL) {
					DebugPrint("Unable to allocate unit");
					return;
				} else {
					if (UnitCanBeAt(*unit, Players[player].StartPos)) {
						unit->Place(Players[player].StartPos);
					} else {
						const int heading = SyncRand() % 256;

						unit->tilePos = Players[player].StartPos;
						DropOutOnSide(*unit, heading, NULL);
					}
					UpdateForNewUnit(*unit, 0);
				}
			}
		}
	}
}

void ChangeFactionCulture(std::string old_civilization_name, std::string faction_name, std::string new_civilization_name)
{
	int old_civilization = PlayerRaces.GetRaceIndexByName(old_civilization_name.c_str());
	int old_faction = -1;
	if (old_civilization != -1) {
		old_faction = PlayerRaces.GetFactionIndexByName(old_civilization, faction_name);
	}
	
	int new_civilization = PlayerRaces.GetRaceIndexByName(new_civilization_name.c_str());
	int new_faction = -1;
	if (new_civilization != -1) {
		new_faction = PlayerRaces.GetFactionIndexByName(new_civilization, faction_name);
	}
	
	if (old_faction == -1 || new_faction == -1) {
		return;
	}
	
	AcquireFactionTechnologies(old_civilization_name, faction_name, new_civilization_name, faction_name);
	
	if (GrandStrategyGame.Factions[old_civilization][old_faction]->ProvinceCount > 0) {
		// replace existent units from the previous civilization with units of the new civilization
		
		for (int i = (GrandStrategyGame.Factions[old_civilization][old_faction]->ProvinceCount - 1); i >= 0; --i) {
			int province_id = GrandStrategyGame.Factions[old_civilization][old_faction]->OwnedProvinces[i];
			
			for (size_t j = 0; j < UnitTypes.size(); ++j) {
				if (
					!UnitTypes[j]->Class.empty()
					&& !UnitTypes[j]->Civilization.empty()
					&& !UnitTypes[j]->BoolFlag[BUILDING_INDEX].value
					&& UnitTypes[j]->DefaultStat.Variables[DEMAND_INDEX].Value > 0
					&& UnitTypes[j]->Civilization == old_civilization_name
					&& PlayerRaces.GetCivilizationClassUnitType(new_civilization, GetUnitTypeClassIndexByName(UnitTypes[j]->Class)) != -1
					&& PlayerRaces.GetCivilizationClassUnitType(new_civilization, GetUnitTypeClassIndexByName(UnitTypes[j]->Class)) != PlayerRaces.GetCivilizationClassUnitType(old_civilization, GetUnitTypeClassIndexByName(UnitTypes[j]->Class)) // don't replace if both civilizations use the same unit type
				) {
					GrandStrategyGame.Provinces[province_id]->ChangeUnitQuantity(PlayerRaces.GetCivilizationClassUnitType(new_civilization, GetUnitTypeClassIndexByName(UnitTypes[j]->Class)), GrandStrategyGame.Provinces[province_id]->Units[j]);
					GrandStrategyGame.Provinces[province_id]->UnderConstructionUnits[PlayerRaces.GetCivilizationClassUnitType(new_civilization, GetUnitTypeClassIndexByName(UnitTypes[j]->Class))] += GrandStrategyGame.Provinces[province_id]->UnderConstructionUnits[j];
					GrandStrategyGame.Provinces[province_id]->SetUnitQuantity(j, 0);
					GrandStrategyGame.Provinces[province_id]->UnderConstructionUnits[j] = 0;
					GrandStrategyGame.Provinces[province_id]->SetOwner(new_civilization, new_faction);
				}
			}
		}
	}

	for (int i = 0; i < MaxCosts; ++i) {
		GrandStrategyGame.Factions[new_civilization][new_faction]->Resources[i] = GrandStrategyGame.Factions[old_civilization][old_faction]->Resources[i];
	}
	
	if (GrandStrategyGame.Factions[old_civilization][old_faction] == GrandStrategyGame.PlayerFaction) {
		GrandStrategyGame.PlayerFaction = const_cast<CGrandStrategyFaction *>(&(*GrandStrategyGame.Factions[new_civilization][new_faction]));
	}
}

void SetFactionCommodityTrade(std::string civilization_name, std::string faction_name, std::string resource_name, int quantity)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return;
	}
	
	GrandStrategyGame.Factions[civilization][faction]->Trade[resource] = quantity;
}

void ChangeFactionCommodityTrade(std::string civilization_name, std::string faction_name, std::string resource_name, int quantity)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return;
	}
	
	GrandStrategyGame.Factions[civilization][faction]->Trade[resource] += quantity;
}

int GetFactionCommodityTrade(std::string civilization_name, std::string faction_name, std::string resource_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return 0;
	}
	
	return GrandStrategyGame.Factions[civilization][faction]->Trade[resource];
}

void SetCommodityPrice(std::string resource_name, int price)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return;
	}
	
	GrandStrategyGame.CommodityPrices[resource] = price;
}

int GetCommodityPrice(std::string resource_name)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return 0;
	}
	
	return GrandStrategyGame.CommodityPrices[resource];
}

void SetResourceBasePrice(std::string resource_name, int price)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return;
	}
	
	DefaultResourcePrices[resource] = price;
}

void SetResourceBaseLaborInput(std::string resource_name, int input)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return;
	}
	
	DefaultResourceLaborInputs[resource] = input;
}

void SetResourceBaseOutput(std::string resource_name, int output)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return;
	}
	
	DefaultResourceOutputs[resource] = output;
}

//@}
