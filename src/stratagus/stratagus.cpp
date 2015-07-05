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
/**@name stratagus.cpp - The main file. */
//
//      (c) Copyright 1998-2015 by Lutz Sammer, Francois Beerten,
//                                 Jimmy Salmon, Pali Rohár and cybermind
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

/**
** @mainpage
**
** @section Introduction Introduction
**
** Welcome to the source code documentation of the Stratagus engine.
** Extract the source documentation with doxygen (http://www.doxygen.org) tool.
**
** Any help to improve this documention is welcome. If you didn't
** understand something or you found an error or a wrong spelling
** or wrong grammar please write an email (including a patch :).
**
** @section Informations Informations
**
** Visit the https://launchpad.net/stratagus web page for the latest news and
** <A HREF="../index.html">Stratagus Info</A> for other documentations.
**
** @section Modules Modules
**
** This are the main modules of the Stratagus engine.
**
** @subsection Map Map
**
** Handles the map. A map is made from tiles.
**
** @see map.h @see map.cpp @see tileset.h @see tileset.cpp
**
** @subsection Unit Unit
**
** Handles units. Units are ships, flyers, buildings, creatures,
** machines.
**
** @see unit.h @see unit.cpp @see unittype.h @see unittype.cpp
**
** @subsection Missile Missile
**
** Handles missiles. Missiles are all other sprites on map
** which are no unit.
**
** @see missile.h @see missile.cpp
**
** @subsection Player Player
**
** Handles players, all units are owned by a player. A player
** could be controlled by a human or a computer.
**
** @see player.h @see player.cpp @see ::CPlayer
**
** @subsection Sound Sound
**
** Handles the high and low level of the sound. There are the
** background music support, voices and sound effects.
** Following low level backends are supported: OSS and SDL.
**
** @todo adpcm file format support for sound effects
** @todo better separation of low and high level, assembler mixing
** support.
** @todo Streaming support of ogg/mp3 files.
**
** @see sound.h @see sound.cpp
** @see script_sound.cpp @see sound_id.cpp @see sound_server.cpp
** @see unitsound.cpp
** @see sdl_audio.cpp
** @see ogg.cpp @see wav.cpp
**
** @subsection Video Video
**
** Handles the high and low level of the graphics.
** This also contains the sprite and linedrawing routines.
**
** See page @ref VideoModule for more information upon supported
** features and video platforms.
**
** @see video.h @see video.cpp
**
** @subsection Network Network
**
** Handles the high and low level of the network protocol.
** The network protocol is needed for multiplayer games.
**
** See page @ref NetworkModule for more information upon supported
** features and API.
**
** @see network.h @see network.cpp
**
** @subsection Pathfinder Pathfinder
**
** @see pathfinder.h @see pathfinder.cpp
**
** @subsection AI AI
**
** There are currently two AI's. The old one is very hardcoded,
** but does things like placing buildings better than the new.
** The old AI shouldn't be used.  The new is very flexible, but
** very basic. It includes none optimations.
**
** See page @ref AiModule for more information upon supported
** features and API.
**
** @see ai_local.h
** @see ai.h @see ai.cpp
**
** @subsection CCL CCL
**
** CCL is Craft Configuration Language, which is used to
** configure and customize Stratagus.
**
** @see script.h @see script.cpp
**
** @subsection Icon Icon
**
** @see icons.h @see icons.cpp
**
** @subsection Editor Editor
**
** This is the integrated editor, it shouldn't be a perfect
** editor. It is used to test new features of the engine.
**
** See page @ref EditorModule for more information upon supported
** features and API.
**
** @see editor.h @see editor.cpp
*/

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <ctype.h>

#ifdef USE_BEOS
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

extern void beos_init(int argc, char **argv);

#endif

#ifdef MAC_BUNDLE
#define Button ButtonOSX
#include <Carbon/Carbon.h>
#undef Button
#endif

#include "SDL.h"

#include "stratagus.h"

#include "ai.h"
#include "editor.h"
//Wyrmgus start
#include "font.h"	// for grand strategy mode tooltip drawing
//Wyrmgus end
#include "game.h"
#include "guichan.h"
#include "interface.h"
#include "iocompat.h"
#include "iolib.h"
#include "map.h"
#include "netconnect.h"
#include "network.h"
#include "parameters.h"
#include "player.h"
#include "replay.h"
#include "results.h"
#include "settings.h"
#include "sound_server.h"
#include "title.h"
#include "translate.h"
#include "ui.h"
#include "unit_manager.h"
#include "version.h"
#include "video.h"
#include "widgets.h"
#include "util.h"

#ifdef DEBUG
#include "missile.h" //for FreeBurningBuildingFrames
#endif

#ifdef USE_STACKTRACE
#include <stdexcept>
#include <stacktrace/call_stack.hpp>
#include <stacktrace/stack_exception.hpp>
#endif

#include <stdlib.h>
#include <stdio.h>

#ifdef USE_WIN32
#include <windows.h>
#include <dbghelp.h>
#endif

#if defined(USE_WIN32) && ! defined(NO_STDIO_REDIRECT)
#include "windows.h"
#define REDIRECT_OUTPUT
#endif

#if defined(USE_WIN32) && ! defined(REDIRECT_OUTPUT)
#include "SetupConsole_win32.h"
#endif

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::string StratagusLibPath;        /// Path for data directory

/// Name, Version, Copyright
//Wyrmgus start
//const char NameLine[] = NAME " V" VERSION ", " COPYRIGHT;
const char NameLine[] = NAME " v" VERSION ", " COPYRIGHT;
//Wyrmgus end

std::string CliMapName;          /// Filename of the map given on the command line
std::string MenuRace;

bool EnableDebugPrint;           /// if enabled, print the debug messages
bool EnableAssert;               /// if enabled, halt on assertion failures
bool EnableUnitDebug;            /// if enabled, a unit info dump will be created

/*============================================================================
==  MAIN
============================================================================*/

/**
**  Pre menu setup.
*/
void PreMenuSetup()
{
	//
	//  Initial menus require some gfx.
	//
	SetDefaultTextColors(FontYellow, FontWhite);

	LoadFonts();

	InitVideoCursors();

	if (MenuRace.empty()) {
		LoadCursors(PlayerRaces.Name[0]);
	} else {
		LoadCursors(MenuRace);
	}

	InitSettings();

	InitUserInterface();
	UI.Load();
}

/**
**  Run the guichan main menus loop.
**
**  @return          0 for success, else exit.
*/
static int MenuLoop()
{
	int status;

	initGuichan();
	InterfaceState = IfaceStateMenu;
	//  Clear screen
	Video.ClearScreen();
	Invalidate();

	ButtonUnderCursor = -1;
	OldButtonUnderCursor = -1;
	CursorState = CursorStatePoint;
	GameCursor = UI.Point.Cursor;

	// FIXME delete this when switching to full guichan GUI
	const std::string filename = LibraryFileName("scripts/guichan.lua");
	status = LuaLoadFile(filename);

	// We clean up later in Exit
	return status;
}

//----------------------------------------------------------------------------

/**
**  Print headerline, copyright, ...
*/
static void PrintHeader()
{
	std::string CompileOptions =
#ifdef DEBUG
		"DEBUG "
#endif
#ifdef USE_ZLIB
		"ZLIB "
#endif
#ifdef USE_BZ2LIB
		"BZ2LIB "
#endif
#ifdef USE_VORBIS
		"VORBIS "
#endif
#ifdef USE_THEORA
		"THEORA "
#endif
#ifdef USE_FLUIDSYNTH
	"FLUIDSYNTH "
#endif
#ifdef USE_MIKMOD
		"MIKMOD "
#endif
#ifdef USE_MNG
		"MNG "
#endif
#ifdef USE_OPENGL
		"OPENGL "
#endif
#ifdef USE_GLES
		"GLES "
#endif
#ifdef USE_WIN32
		"WIN32 "
#endif
#ifdef USE_LINUX
		"LINUX "
#endif
#ifdef USE_BSD
		"BSD "
#endif
#ifdef USE_BEOS
		"BEOS "
#endif
#ifdef USE_MAC
		"MAC "
#endif
#ifdef USE_X11
		"X11 "
#endif
#ifdef USE_MAEMO
		"MAEMO "
#endif
#ifdef USE_TOUCHSCREEN
		"TOUCHSCREEN "
#endif
		"";

	fprintf(stdout,
			"%s\n  written by Lutz Sammer, Fabrice Rossi, Vladi Shabanski, Patrice Fortier,\n"
			"  Jon Gabrielson, Andreas Arens, Nehal Mistry, Jimmy Salmon, Pali Rohar,\n"
			"  cybermind and others.\n"
			"\t" HOMEPAGE "\n"
			"Compile options %s",
			NameLine, CompileOptions.c_str());
}

void PrintLicense()
{
	printf("\n"
		   "\n"
		   "Stratagus may be copied only under the terms of the GNU General Public License\n"
		   "which may be found in the Stratagus source kit.\n"
		   "\n"
		   "DISCLAIMER:\n"
		   "This software is provided as-is.  The author(s) can not be held liable for any\n"
		   "damage that might arise from the use of this software.\n"
		   "Use it at your own risk.\n"
		   "\n");
}


/**
**  Exit the game.
**
**  @param err  Error code to pass to shell.
*/
void Exit(int err)
{
	if (GameRunning) {
		StopGame(GameExit);
		return;
	}

	StopMusic();
	QuitSound();
	NetworkQuitGame();

	ExitNetwork1();
#ifdef DEBUG
	CleanModules();
	FreeBurningBuildingFrames();
	FreeSounds();
	FreeGraphics();
	FreePlayerColors();
	FreeButtonStyles();
	FreeAllContainers();
	freeGuichan();
	DebugPrint("Frames %lu, Slow frames %d = %ld%%\n" _C_
			   FrameCounter _C_ SlowFrameCounter _C_
			   (SlowFrameCounter * 100) / (FrameCounter ? FrameCounter : 1));
	lua_settop(Lua, 0);
	lua_close(Lua);
	DeInitVideo();
#endif

	fprintf(stdout, "%s", _("Thanks for playing Stratagus.\n"));
	exit(err);
}

/**
**  Do a fatal exit.
**  Called on out of memory or crash.
**
**  @param err  Error code to pass to shell.
*/
void ExitFatal(int err)
{
#ifdef USE_STACKTRACE
	throw stacktrace::stack_runtime_error((const char*)err);
#endif
	exit(err);
}

/**
**  Display the usage.
*/
static void Usage()
{
	PrintHeader();
	printf(
		"\n\nUsage: %s [OPTIONS] [map.smp|map.smp.gz]\n"
		"\t-a\t\tEnables asserts check in engine code (for debugging)\n"
		"\t-c file.lua\tConfiguration start file (default stratagus.lua)\n"
		"\t-d datapath\tPath to stratagus data (default current directory)\n"
		"\t-D depth\tVideo mode depth = pixel per point\n"
		"\t-e\t\tStart editor (instead of game)\n"
		"\t-E file.lua\tEditor configuration start file (default editor.lua)\n"
		"\t-F\t\tFull screen video mode\n"
		"\t-h\t\tHelp shows this page\n"
		"\t-i\t\tEnables unit info dumping into log (for debugging)\n"
		"\t-I addr\t\tNetwork address to use\n"
		"\t-l\t\tDisable command log\n"
		"\t-N name\t\tName of the player\n"
#if defined(USE_OPENGL) || defined(USE_GLES)
		"\t-o\t\tDo not use OpenGL or OpenGL ES 1.1\n"
		"\t-O\t\tUse OpenGL or OpenGL ES 1.1\n"
#endif
		"\t-p\t\tEnables debug messages printing in console\n"
		"\t-P port\t\tNetwork port to use\n"
		"\t-s sleep\tNumber of frames for the AI to sleep before it starts\n"
		"\t-S speed\tSync speed (100 = 30 frames/s)\n"
		"\t-u userpath\tPath where stratagus saves preferences, log and savegame\n"
		"\t-v mode\t\tVideo mode resolution in format <xres>x<yres>\n"
		"\t-W\t\tWindowed video mode\n"
#if defined(USE_OPENGL) || defined(USE_GLES)
		"\t-Z\t\tUse OpenGL to scale the screen to the viewport (retro-style). Implies -O.\n"
#endif
		"map is relative to StratagusLibPath=datapath, use ./map for relative to cwd\n",
		Parameters::Instance.applicationName.c_str());
}

#ifdef REDIRECT_OUTPUT

static std::string stdoutFile;
static std::string stderrFile;

static void CleanupOutput()
{
	fclose(stdout);
	fclose(stderr);

	struct stat st;
	if (stat(stdoutFile.c_str(), &st) == 0 && st.st_size == 0) {
		unlink(stdoutFile.c_str());
	}
	if (stat(stderrFile.c_str(), &st) == 0 && st.st_size == 0) {
		unlink(stderrFile.c_str());
	}
}

static void RedirectOutput()
{
	char path[MAX_PATH];
	int pathlen;

	pathlen = GetModuleFileName(NULL, path, sizeof(path));
	while (pathlen > 0 && path[pathlen] != '\\') {
		--pathlen;
	}
	path[pathlen] = '\0';

	stdoutFile = std::string(path) + "\\stdout.txt";
	stderrFile = std::string(path) + "\\stderr.txt";

	if (!freopen(stdoutFile.c_str(), "w", stdout)) {
		printf("freopen stdout failed");
	}
	if (!freopen(stderrFile.c_str(), "w", stderr)) {
		printf("freopen stderr failed");
	}
	atexit(CleanupOutput);
}
#endif

void ParseCommandLine(int argc, char **argv, Parameters &parameters)
{
	for (;;) {
		switch (getopt(argc, argv, "ac:d:D:eE:FhiI:lN:oOP:ps:S:u:v:WZ?")) {
			case 'a':
				EnableAssert = true;
				continue;
			case 'c':
				parameters.luaStartFilename = optarg;
				continue;
			case 'd': {
				StratagusLibPath = optarg;
				size_t index;
				while ((index = StratagusLibPath.find('\\')) != std::string::npos) {
					StratagusLibPath[index] = '/';
				}
				continue;
			}
			case 'D':
				Video.Depth = atoi(optarg);
				continue;
			case 'e':
				Editor.Running = EditorCommandLine;
				continue;
			case 'E':
				parameters.luaEditorStartFilename = optarg;
				continue;
			case 'F':
				VideoForceFullScreen = 1;
				Video.FullScreen = 1;
				continue;
			case 'i':
				EnableUnitDebug = true;
				continue;
			case 'I':
				CNetworkParameter::Instance.localHost = optarg;
				continue;
			case 'l':
				CommandLogDisabled = true;
				continue;
			case 'N':
				parameters.LocalPlayerName = optarg;
				continue;
#if defined(USE_OPENGL) || defined(USE_GLES)
			case 'o':
				ForceUseOpenGL = 1;
				UseOpenGL = 0;
				if (ZoomNoResize) {
					fprintf(stderr, "Error: -Z only works with OpenGL enabled\n");
					Usage();
					ExitFatal(-1);
				}
				continue;
			case 'O':
				ForceUseOpenGL = 1;
				UseOpenGL = 1;
				continue;
#endif
			case 'P':
				CNetworkParameter::Instance.localPort = atoi(optarg);
				continue;
			case 'p':
				EnableDebugPrint = true;
				continue;
			case 's':
				AiSleepCycles = atoi(optarg);
				continue;
			case 'S':
				VideoSyncSpeed = atoi(optarg);
				continue;
			case 'u':
				Parameters::Instance.SetUserDirectory(optarg);
				continue;
			case 'v': {
				char *sep = strchr(optarg, 'x');
				if (!sep || !*(sep + 1)) {
					fprintf(stderr, "%s: incorrect format of video mode resolution -- '%s'\n", argv[0], optarg);
					Usage();
					ExitFatal(-1);
				}
				Video.Height = atoi(sep + 1);
				*sep = 0;
				Video.Width = atoi(optarg);
				if (!Video.Height || !Video.Width) {
					fprintf(stderr, "%s: incorrect format of video mode resolution -- '%sx%s'\n", argv[0], optarg, sep + 1);
					Usage();
					ExitFatal(-1);
				}
#if defined(USE_OPENGL) || defined(USE_GLES)
				if (ZoomNoResize) {
					Video.ViewportHeight = Video.Height;
					Video.ViewportWidth = Video.Width;
					Video.Height = 0;
					Video.Width = 0;
				}
#endif
				continue;
			}
			case 'W':
				VideoForceFullScreen = 1;
				Video.FullScreen = 0;
				continue;
#if defined(USE_OPENGL) || defined(USE_GLES)
			case 'Z':
				ForceUseOpenGL = 1;
				UseOpenGL = 1;
				ZoomNoResize = 1;
				Video.ViewportHeight = Video.Height;
				Video.ViewportWidth = Video.Width;
				Video.Height = 0;
				Video.Width = 0;
				continue;
#endif
			case -1:
				break;
			case '?':
			case 'h':
			default:
				Usage();
				ExitFatal(-1);
		}
		break;
	}

	if (argc - optind > 1) {
		fprintf(stderr, "too many files\n");
		Usage();
		ExitFatal(-1);
	}

	if (argc - optind) {
		size_t index;
		CliMapName = argv[optind];
		while ((index = CliMapName.find('\\')) != std::string::npos) {
			CliMapName[index] = '/';
		}
	}
}

#ifdef USE_WIN32
static LONG WINAPI CreateDumpFile(EXCEPTION_POINTERS *ExceptionInfo)
{
	HANDLE hFile = CreateFile("crash.dmp", GENERIC_READ | GENERIC_WRITE,    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,     NULL);
	MINIDUMP_EXCEPTION_INFORMATION mei;
	mei.ThreadId = GetCurrentThreadId();
	mei.ClientPointers = TRUE;
	mei.ExceptionPointers = ExceptionInfo;
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mei, NULL, NULL);
	fprintf(stderr, "Stratagus crashed!\n");
	fprintf(stderr, "A mini dump file \"crash.dmp\" has been created in the Stratagus folder.\n");
	//Wyrmgus start
//	fprintf(stderr, "Please send it to our bug tracker: https://bugs.launchpad.net/stratagus\n");
	fprintf(stderr, "Please send it to our bug tracker: https://github.com/Andrettin/Wyrmgus/issues\n");
//	fprintf(stderr, "and tell us what have you done to cause this bug.\n");
	fprintf(stderr, "and tell us what was happening when this bug occurred.\n");
	//Wyrmgus end
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

/**
**  The main program: initialise, parse options and arguments.
**
**  @param argc  Number of arguments.
**  @param argv  Vector of arguments.
*/
int stratagusMain(int argc, char **argv)
{
#ifdef REDIRECT_OUTPUT
	RedirectOutput();
#endif
#ifdef USE_BEOS
	//  Parse arguments for BeOS
	beos_init(argc, argv);
#endif
#ifdef USE_WIN32
	SetUnhandledExceptionFilter(CreateDumpFile);
#endif
#if defined(USE_WIN32) && ! defined(REDIRECT_OUTPUT)
	SetupConsole();
#endif
	//  Setup some defaults.
#ifndef MAC_BUNDLE
	StratagusLibPath = ".";
#else
	freopen("/tmp/stdout.txt", "w", stdout);
	freopen("/tmp/stderr.txt", "w", stderr);
	// Look for the specified data set inside the application bundle
	// This should be a subdir of the Resources directory
	CFURLRef pluginRef = CFBundleCopyResourceURL(CFBundleGetMainBundle(),
												 CFSTR(MAC_BUNDLE_DATADIR), NULL, NULL);
	CFStringRef macPath = CFURLCopyFileSystemPath(pluginRef,  kCFURLPOSIXPathStyle);
	const char *pathPtr = CFStringGetCStringPtr(macPath, CFStringGetSystemEncoding());
	Assert(pathPtr);
	StratagusLibPath = pathPtr;
#endif
#ifdef USE_STACKTRACE
	try {
#endif
	Parameters &parameters = Parameters::Instance;
	parameters.SetDefaultValues();
	parameters.SetLocalPlayerNameFromEnv();

	if (argc > 0) {
		parameters.applicationName = argv[0];
	}

	// FIXME: Parse options before or after scripts?
	ParseCommandLine(argc, argv, parameters);
	// Init the random number generator.
	InitSyncRand();

	makedir(parameters.GetUserDirectory().c_str(), 0777);

	// Init Lua and register lua functions!
	InitLua();
	LuaRegisterModules();

	// Initialise AI module
	InitAiModule();

	LoadCcl(parameters.luaStartFilename);

	PrintHeader();
	PrintLicense();

	// Setup video display
	InitVideo();

	// Setup sound card
	if (!InitSound()) {
		InitMusic();
	}

#ifndef DEBUG           // For debug it's better not to have:
	srand(time(NULL));  // Random counter = random each start
#endif

	//  Show title screens.
	SetDefaultTextColors(FontYellow, FontWhite);
	LoadFonts();
	SetClipping(0, 0, Video.Width - 1, Video.Height - 1);
	Video.ClearScreen();
	ShowTitleScreens();

	// Init player data
	ThisPlayer = NULL;
	//Don't clear the Players structure as it would erase the allowed units.
	// memset(Players, 0, sizeof(Players));
	NumPlayers = 0;

	UnitManager.Init(); // Units memory management
	PreMenuSetup();     // Load everything needed for menus

	MenuLoop();

	Exit(0);
#ifdef USE_STACKTRACE
	} catch (const std::exception &e) {
		fprintf(stderr, "Stratagus crashed!\n");
		//Wyrmgus start
//		fprintf(stderr, "Please send this call stack to our bug tracker: https://bugs.launchpad.net/stratagus\n");
		fprintf(stderr, "Please send this call stack to our bug tracker: https://github.com/Andrettin/Wyrmgus/issues\n");
//		fprintf(stderr, "and tell us what have you done to cause this bug.\n");
		fprintf(stderr, "and tell us what was happening when this bug occurred.\n");
		//Wyrmgus end
		fprintf(stderr, " === exception state traceback === \n");
		fprintf(stderr, "%s", e.what());
		exit(1);
	}
#endif
	return 0;
}

//Wyrmgus start
//Grand Strategy elements

bool GrandStrategy = false;				///if the game is in grand strategy mode
std::string GrandStrategyWorld;
int WorldMapOffsetX;
int WorldMapOffsetY;
bool GrandStrategyMapWidthIndent;
bool GrandStrategyMapHeightIndent;
CGrandStrategyGame GrandStrategyGame;

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
}

/**
**  Draw the grand strategy map.
*/
void CGrandStrategyGame::DrawMap()
{
	int grand_strategy_map_width = Video.Width + 64;
	int grand_strategy_map_height = Video.Height - 16 - 186;
	
	int width_indent = 0;
	int height_indent = 0;
	if (GrandStrategyMapWidthIndent) {
		width_indent = -32;
	}
	if (GrandStrategyMapHeightIndent) {
		height_indent = -32;
	}
	
	for (int x = WorldMapOffsetX; x <= (WorldMapOffsetX + floor(static_cast<double>(grand_strategy_map_width / 64))); ++x) {
		for (int y = WorldMapOffsetY; y <= std::min((WorldMapOffsetY + static_cast<int>(floor(static_cast<double>(grand_strategy_map_height / 64)))), (GetWorldMapHeight() - 1)); ++y) {
			if (!GetWorldMapTileGraphicTile(x, y).empty()) {
				if (GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->BaseTile != -1) { // should be changed into a more dynamic setting than being based on GrandStrategyWorld
					std::string base_tile_filename;
					if (GrandStrategyWorld == "Nidavellir") {
						base_tile_filename = "tilesets/world/terrain/dark_plains.png";
					} else {
						base_tile_filename = "tilesets/world/terrain/plains.png";
					}
					if (CGraphic::Get(base_tile_filename) == NULL) {
						CGraphic *base_tile_graphic = CGraphic::New(base_tile_filename, 64, 64);
						base_tile_graphic->Load();
					}
					CGraphic::Get(base_tile_filename)->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
				}
				
				if (CGraphic::Get(GrandStrategyGame.WorldMapTiles[x][y]->GraphicTile) == NULL) {
					CGraphic *tile_graphic = CGraphic::New(GrandStrategyGame.WorldMapTiles[x][y]->GraphicTile, 64, 64);
					tile_graphic->Load();
				}
				CGraphic::Get(GrandStrategyGame.WorldMapTiles[x][y]->GraphicTile)->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
			}
		}
	}
	
	//if is clicking on a tile, draw a square on its borders
	if (UI.MapArea.Contains(CursorScreenPos) && GrandStrategyGame.WorldMapTiles[GrandStrategyGame.GetTileUnderCursor().x][GrandStrategyGame.GetTileUnderCursor().y]->Terrain != -1 && (MouseButtons & LeftButton)) {
		int tile_screen_x = ((GrandStrategyGame.GetTileUnderCursor().x - WorldMapOffsetX) * 64) + UI.MapArea.X + width_indent;
		int tile_screen_y = ((GrandStrategyGame.GetTileUnderCursor().y - WorldMapOffsetY) * 64) + UI.MapArea.Y + height_indent;
			
//		clamp(&tile_screen_x, 0, Video.Width);
//		clamp(&tile_screen_y, 0, Video.Height);
			
		Video.DrawRectangle(ColorWhite, tile_screen_x, tile_screen_y, 64, 64);
	}
	
	
	std::string fog_graphic_tile = "tilesets/world/terrain/fog.png";
	if (CGraphic::Get(fog_graphic_tile) == NULL) {
		CGraphic *fog_tile_graphic = CGraphic::New(fog_graphic_tile, 96, 96);
		fog_tile_graphic->Load();
	}
	//draw fog over terra incognita
	for (int x = WorldMapOffsetX; x <= (WorldMapOffsetX + floor(static_cast<double>(grand_strategy_map_width / 64))); ++x) {
		for (int y = WorldMapOffsetY; y <= std::min((WorldMapOffsetY + static_cast<int>(floor(static_cast<double>(grand_strategy_map_height / 64)))), (GetWorldMapHeight() - 1)); ++y) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->Terrain == -1) {
				CGraphic::Get(fog_graphic_tile)->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent - 16, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 16, true);
			}
		}
	}
}

/**
**  Draw the grand strategy tile tooltip.
*/
void CGrandStrategyGame::DrawTileTooltip(int x, int y)
{
	std::string tile_tooltip = GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name;
	//province?
	tile_tooltip += " (";
	tile_tooltip += std::to_string((_Longlong)x);
	tile_tooltip += ", ";
	tile_tooltip += std::to_string((_Longlong)y);
	tile_tooltip += ")";
	CLabel(GetGameFont()).Draw(UI.StatusLine.TextX, UI.StatusLine.TextY, tile_tooltip);
}

/**
**  Draw the grand strategy tile tooltip.
*/
Vec2i CGrandStrategyGame::GetTileUnderCursor()
{
	Vec2i tile_under_cursor(0, 0);
	
	int width_indent = 0;
	int height_indent = 0;
	if (GrandStrategyMapWidthIndent) {
		width_indent = -32;
	}
	if (GrandStrategyMapHeightIndent) {
		height_indent = -32;
	}
			
	tile_under_cursor.x = WorldMapOffsetX + ((CursorScreenPos.x - UI.MapArea.X - width_indent) / 64);
	tile_under_cursor.y = WorldMapOffsetY + ((CursorScreenPos.y - UI.MapArea.Y - height_indent) / 64);
	
	return tile_under_cursor;
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

/**
**  Get the graphic tile of a world map tile.
*/
std::string GetWorldMapTileGraphicTile(int x, int y)
{
	
	clamp(&x, 0, GrandStrategyGame.WorldMapWidth - 1);
	clamp(&y, 0, GrandStrategyGame.WorldMapHeight - 1);

	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	if (GrandStrategyGame.WorldMapTiles[x][y]->Terrain == -1) {
		return "";
	}
	
	Assert(GrandStrategyGame.WorldMapTiles[x][y]->Terrain != -1);

	return GrandStrategyGame.WorldMapTiles[x][y]->GraphicTile;
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
**  Set the size of the world map.
*/
void SetWorldMapSize(int width, int height)
{
	Assert(width <= WorldMapWidthMax);
	Assert(height <= WorldMapHeightMax);
	GrandStrategyGame.WorldMapWidth = width;
	GrandStrategyGame.WorldMapHeight = height;
	
	//create new world map tile objects for the size, if necessary
	for (int x = 0; x < WorldMapWidthMax; ++x) {
		for (int y = 0; y < WorldMapHeightMax; ++y) {
			if (!GrandStrategyGame.WorldMapTiles[x][y]) {
				WorldMapTile *world_map_tile = new WorldMapTile;
				GrandStrategyGame.WorldMapTiles[x][y] = world_map_tile;
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
	}
	
	GrandStrategyGame.WorldMapTiles[x][y]->Terrain = terrain;
	
	if (terrain != -1 && GrandStrategyGame.TerrainTypes[terrain]) {
		//randomly select a variation for the world map tile
		if (GrandStrategyGame.TerrainTypes[terrain]->Variations > 0) {
			GrandStrategyGame.WorldMapTiles[x][y]->Variation = SyncRand(GrandStrategyGame.TerrainTypes[terrain]->Variations);
		} else {
			GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
		}
	}
}

/**
**  Set the terrain type of a world map tile.
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
		
		if (GrandStrategyGame.TerrainTypes[terrain]->HasTransitions) {
			graphic_tile += "/";
			graphic_tile += GrandStrategyGame.TerrainTypes[terrain]->Tag;
			if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_north";
			} else if (GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_south";
			} else if (GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_west";
			} else if (GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_east";
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_outer";
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northeast_outer";
			} else if (GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_southwest_outer";
			} else if (GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_southeast_outer";
			} else if (GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_southwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_north_south";
			} else if (GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_west_east";
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_northeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_southwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northeast_southwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northeast_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_southwest_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_northeast_southwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_northeast_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_southwest_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northeast_southwest_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_northeast_southwest_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_north_southwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_north_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_west_northeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_west_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_east_northwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_east_southwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_south_northwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_south_northeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_northeast_outer";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_southwest_outer";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northeast_southeast_outer";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_southwest_southeast_outer";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_outer_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northeast_outer_southwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_southwest_outer_northeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_southeast_outer_northwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_north_southwest_inner_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_south_northwest_inner_northeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_west_northeast_inner_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_east_northwest_inner_southwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_outer";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else {
				graphic_tile += "_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			}
		}
		
		if (GrandStrategyGame.WorldMapTiles[x][y]->Variation != -1) {
			graphic_tile += "_";
			graphic_tile += std::to_string((_Longlong)GrandStrategyGame.WorldMapTiles[x][y]->Variation + 1);
		}
		
		graphic_tile += ".png";
		
		if (!CanAccessFile(graphic_tile.c_str())) {
			graphic_tile = FindAndReplaceString(graphic_tile, "2", "1");
		}
		
		GrandStrategyGame.WorldMapTiles[x][y]->GraphicTile = graphic_tile;
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
				delete GrandStrategyGame.WorldMapTiles[x][y];
			}
		}
	}
	GrandStrategyGame.WorldMapWidth = 0;
	GrandStrategyGame.WorldMapHeight = 0;
}
//Wyrmgus end

//@}
