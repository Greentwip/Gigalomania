//---------------------------------------------------------------------------
#include "stdafx.h"

#include <cassert>
#include <ctime>
#include <cerrno> // n.b., needed on Linux at least

#include <stdexcept> // needed for Android at least

#ifdef _WIN32
#include <io.h> // for access
#define access _access
#elif __linux
#include <unistd.h> // for access
#endif

#include <sstream>
using std::stringstream;

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef _WIN32
#include <dirent.h>
#include <string.h>
#endif

#ifdef AROS
#include <proto/dos.h>
#endif

#undef min
#undef max

#include "game.h"
#include "utils.h"
#include "sector.h"
#include "gamestate.h"
#include "gui.h"
#include "player.h"
#include "tutorial.h"

#include "screen.h"
#include "image.h"
#include "sound.h"

const bool default_pref_sound_on_c = true;
const bool default_pref_music_on_c = true;
const bool default_pref_disallow_nukes_c = false;

Game::Game() {
	TrackedObject::initialise();

	scale_factor_w = 1.0f;
	scale_factor_h = 1.0f;
	scale_width = 0.0f;
	scale_height = 0.0f;
	// onemousebutton means UI can be used with one mouse button only
#if defined(__ANDROID__)
	onemousebutton = true;
#else
	onemousebutton = false;
#endif
	// mobile_ui means no mouse pointer
#if defined(__ANDROID__)
	mobile_ui = true;
#else
	mobile_ui = false;
#endif
	using_old_gfx = false;
	is_testing = false;

	application = NULL;
	screen = NULL;
	paused = false;
	gamestate = NULL;
	dispose_gamestate = NULL;
	lastmousepress_time = 0;

	frame_counter = 0;
	time_rate = 1;
	real_time = 0;
	real_loop_time = 0;
	game_time = 0;
	loop_time = 0;
	accumulated_time = 0.0f;
	mouseTime = -1;

	pref_sound_on = default_pref_sound_on_c;
	pref_music_on = default_pref_music_on_c;
	pref_disallow_nukes = default_pref_disallow_nukes_c;

	gameMode = GAMEMODE_SINGLEPLAYER;
	gameType = GAMETYPE_SINGLEISLAND;
	difficulty_level = DIFFICULTY_EASY;
	human_player = 0;
	tutorial = NULL;

	gameStateID = GAMESTATEID_UNDEFINED;
	state_changed = false;
	gameResult = GAMERESULT_UNDEFINED;

	start_epoch = 0;
	n_sub_epochs = 4;
	selected_island = 0;
	for(int i=0;i<max_islands_per_epoch_c;i++) {
		completed_island[i] = NULL;
		for(int j=0;j<n_epochs_c;j++) {
			maps[j][i] = NULL;
		}
	}
	map = NULL;
	n_men_store = 0;
	n_player_suspended = 0;

	background = NULL;
	for(int i=0;i<n_players_c;i++) {
		player_heads_select[i] = NULL;
		player_heads_alliance[i] = NULL;
	}
	grave = NULL;
	for(int i=0;i<MAP_N_COLOURS;i++)
		land[i] = NULL;
	for(int i=0;i<n_epochs_c;i++) {
		fortress[i] = NULL;
		mine[i] = NULL;
		factory[i] = NULL;
		lab[i] = NULL;
		men[i] = NULL;
	}
	unarmed_man = NULL;
	for(int i=0;i<n_players_c;i++) {
		for(int j=0;j<n_flag_frames_c;j++) {
			flags[i][j] = NULL;
		}
	}
	panel_design = NULL;
	panel_lab = NULL;
	panel_factory = NULL;
	panel_shield = NULL;
	panel_defence = NULL;
	panel_attack = NULL;
	panel_bloody_attack = NULL;
	panel_twoattack = NULL;
	for(int i=0;i<N_BUILDINGS;i++) {
		panel_build[i] = NULL;
		panel_building[i] = NULL;
	}
	panel_knowndesigns = NULL;
	panel_bigdesign = NULL;
	panel_biglab = NULL;
	panel_bigfactory = NULL;
	panel_bigshield = NULL;
	panel_bigdefence = NULL;
	panel_bigattack = NULL;
	panel_bigbuild = NULL;
	panel_bigknowndesigns = NULL;
	for(int i=0;i<10;i++) {
		numbers_blue[i] = NULL;
		numbers_grey[i] = NULL;
		numbers_white[i] = NULL;
		numbers_orange[i] = NULL;
		numbers_yellow[i] = NULL;
		numbers_largegrey[i] = NULL;
		numbers_largeshiny[i] = NULL;
		for(int j=0;j<n_players_c;j++)
			numbers_small[j][i] = NULL;
	}
	numbers_half = NULL;
	for(int i=0;i<n_font_chars_c;i++) {
		letters_large[i] = NULL;
		letters_small[i] = NULL;
	}
	for(int i=0;i<n_players_c;i++)
		mouse_pointers[i] = NULL;
	for(int i=0;i<n_playershields_c;i++)
		playershields[i] = NULL;
	building_health = NULL;
	dash_grey = NULL;
	icon_shield = NULL;
	icon_defence = NULL;
	icon_weapon = NULL;
	for(int i=0;i<n_shields_c;i++)
		icon_shields[i] = NULL;
	for(int i=0;i<n_epochs_c;i++) {
		icon_defences[i] = NULL;
		icon_weapons[i] = NULL;
		numbered_defences[i] = NULL;
		numbered_weapons[i] = NULL;
	}
	for(int i=0;i<N_ID;i++) {
		icon_elements[i] = NULL;
	}
	for(int i=0;i<12;i++) {
		icon_clocks[i] = NULL;
	}
	icon_infinity = NULL;
	icon_bc = NULL;
	icon_ad = NULL;
	icon_ad_shiny = NULL;
	for(int i=0;i<n_players_c;i++) {
		icon_towers[i] = NULL;
		icon_armies[i] = NULL;
	}
	icon_nuke_hole = NULL;
	mine_gatherable_small = NULL;
	mine_gatherable_large = NULL;
	icon_ergo = NULL;
	icon_trash = NULL;
	for(int i=0;i<n_coast_c;i++)
		coast_icons[i] = NULL;
	map_sq_offset = 0;
	map_sq_coast_offset = 0;
	for(int i=0;i<MAP_N_COLOURS;i++) {
		for(int j=0;j<n_map_sq_c;j++)
			map_sq[i][j] = NULL;
	}
	for(int i=0;i<n_epochs_c;i++) {
		n_defender_frames[i] = 0;
	}
	for(int i=0;i<n_players_c;i++) {
		for(int j=0;j<n_epochs_c;j++) {
			for(int k=0;k<max_defender_frames_c;k++) {
				defenders[i][j][k] = NULL;
			}
		}
		nuke_defences[i] = NULL;
	}
	for(int i=0;i<n_epochs_c+1;i++) {
		for(int j=0;j<n_attacker_directions_c;j++) {
			n_attacker_frames[i][j] = 0;
		}
	}
	for(int i=0;i<n_players_c;i++) {
		for(int j=0;j<n_epochs_c+1;j++) {
			for(int k=0;k<n_attacker_directions_c;k++) {
				for(int l=0;l<max_attacker_frames_c;l++) {
					attackers_walking[i][j][k][l] = NULL;
				}
			}
		}
		for(int j=0;j<n_epochs_c;j++) {
			planes[i][j] = NULL;
		}
		for(int j=0;j<n_nuke_frames_c;j++) {
			nukes[i][j] = NULL;
			saucers[i][j] = NULL;
		}
	}
	for(int i=0;i<n_epochs_c;i++) {
		for(int j=0;j<N_ATTACKER_AMMO_DIRS;j++)
			attackers_ammo[i][j] = NULL;
	}
	icon_openpitmine = NULL;
	for(int i=0;i<n_trees_c;i++) {
		for(int j=0;j<n_tree_frames_c;j++)
			icon_trees[i][j] = NULL;
	}
	flashingmapsquare = NULL;
	mapsquare = NULL;
	arrow_left = NULL;
	arrow_right = NULL;
	for(int i=0;i<n_death_flashes_c;i++)
		death_flashes[i] = NULL;
	for(int i=0;i<n_blue_flashes_c;i++)
		blue_flashes[i] = NULL;
	for(int i=0;i<n_explosions_c;i++)
		explosions[i] = NULL;
	for(int i=0;i<2;i++)
		icon_mice[i] = NULL;
	for(int i=0;i<3;i++)
		icon_speeds[i] = NULL;
	smoke_image = NULL;
	background_islands = NULL;

	// speech
	s_design_is_ready = NULL;
	s_ergo = NULL;
	s_advanced_tech = NULL;
	s_fcompleted = NULL;
	s_on_hold = NULL;
	s_running_out_of_elements = NULL;
	s_tower_critical = NULL;
	s_sector_destroyed = NULL;
	s_mine_destroyed = NULL;
	s_factory_destroyed = NULL;
	s_lab_destroyed = NULL;
	s_itis_all_over = NULL;
	s_conquered = NULL;
	s_won = NULL;
	s_weve_nuked_them = NULL;
	s_weve_been_nuked = NULL;
	for(int i=0;i<n_players_c;i++) {
		s_alliance_yes[i] = NULL;
		s_alliance_no[i] = NULL;
		s_alliance_ask[i] = NULL;
		s_quit[i] = NULL;
	}
	s_cant_nuke_ally = NULL;

	// effects
	s_explosion = NULL;
	s_scream = NULL;
	s_buildingdestroyed = NULL;
	s_guiclick = NULL;
	s_biplane = NULL;
	s_jetplane = NULL;
	s_spaceship = NULL;

	music = NULL;

	for(int i=0;i<n_epochs_c;i++) {
		invention_shields[i] = NULL;
		invention_defences[i] = NULL;
		invention_weapons[i] = NULL;
	}
	for(int i=0;i<N_ID;i++) {
		elements[i] = NULL;
	}
	for(int i=0;i<n_players_c;i++) {
		players[i] = NULL;
	}
}

Game::~Game() {
	if( gamestate != NULL ) {
		LOG("delete gamestate %d\n", gamestate);
		delete gamestate;
		gamestate = NULL;
	}
	if( tutorial != NULL ) {
		LOG("delete tutorial\n");
		delete tutorial;
		tutorial = NULL;
	}
	LOG("delete maps\n");
	for(int i=0;i<n_epochs_c;i++) {
		for(int j=0;j<max_islands_per_epoch_c;j++) {
			if( maps[i][j] != NULL ) {
				delete maps[i][j];
				maps[i][j] = NULL;
			}
		}
	}
	map = NULL;
	if( screen != NULL ) {
		LOG("delete screen %d\n", screen);
		delete screen;
		screen = NULL;
	}
	LOG("clean up tracked objects\n");
	TrackedObject::cleanup();
	// no longer need to stop music, as it's deleted as a TrackedObject
	//stopMusic();
	LOG("free sound\n");
	freeSound();
	LOG("delete application %d\n", application);
	delete application;
	LOG("exiting...\n");
	cleanupLogFile();
}

bool Game::oneMouseButtonMode() const {
	return onemousebutton || application->isBlankMouse();
}

Game *game_g = NULL;

#ifdef WINRT
const char *maps_dirname = "Assets/islands";
#else
const char *maps_dirname = "islands";
#endif
#if !defined(__ANDROID__) && defined(__linux)
const char *alt_maps_dirname = "/usr/share/gigalomania/islands";
#endif

//bool use_amigadata = true;

const unsigned char shadow_alpha_c = (unsigned char)160;

const int epoch_dates[n_epochs_c] = {-10000, -2000, 1, 900, 1400, 1850, 1914, 1950, 1980, 2100};
const char *epoch_names[n_epochs_c] = { "FIRST", "SECOND", "THIRD", "FOURTH", "FIFTH", "SIXTH", "SEVENTH", "EIGHTH", "NINTH", "TENTH" };

bool Game::isDemo() const {
	return human_player == PLAYER_DEMO;
}

const char autosave_filename[] = "autosave.sav";
const char autosave_bad_filename[] = "autosave_bad.sav";
const char autosave_old_filename[] = "autosave_old.sav";
const bool autosave_survive_uninstall = false; // important for autosave state to be deleted upon uninstall if possible, so that any problems can be fixed by a reinstall

bool validDifficulty(DifficultyLevel difficulty) {
	return difficulty >= 0 && difficulty < DIFFICULTY_N_LEVELS;
}

int Game::getMenPerEpoch() const {
	ASSERT( gameType == GAMETYPE_ALLISLANDS );
	if( difficulty_level == DIFFICULTY_EASY )
		return 150;
	else if( difficulty_level == DIFFICULTY_MEDIUM )
		return 120;
	else if( difficulty_level == DIFFICULTY_HARD )
		return 100;
	else if( difficulty_level == DIFFICULTY_ULTRA )
		return 75;
	ASSERT(false);
	return 0;
}

int Game::getMenAvailable() const {
	if( start_epoch == end_epoch_c && gameType == GAMETYPE_ALLISLANDS )
		return n_player_suspended;
	return n_men_store;
}

int Game::getNSuspended() const {
	return n_player_suspended;
}

const Map *Game::getMap() const {
	return map;
}

Map *Game::getMap() {
	return map;
}

Map::Map(MapColour colour,int n_opponents,const char *name) {
	//*this->filename = '\0';
	this->colour = colour;
	this->n_opponents = n_opponents;
	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			sector_at[x][y] = false;
			sectors[x][y] = NULL;
			reserved[x][y] = false;
			//panels[x][y] = NULL;
		}
	}
	/*for(int i=0;i<N_ID;i++) {
	this->elements[i] = 0;
	}*/
	//clearTemp();
	/*strncpy(this->name,name,MAP_MAX_NAME);
	this->name[MAP_MAX_NAME] = '\0';*/
	this->name = name;
}

Map::~Map() {
	freeSectors();
}

/*void Map::clearTemp() {
	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			temp[x][y] = false;
		}
	}
}*/

const Sector *Map::getSector(int x, int y) const {
	ASSERT(x >= 0 && x < map_width_c && y >= 0 && y < map_height_c);
	Sector *sector = this->sectors[x][y];
	return sector;
}

Sector *Map::getSector(int x, int y) {
	ASSERT(x >= 0 && x < map_width_c && y >= 0 && y < map_height_c);
	Sector *sector = this->sectors[x][y];
	return sector;
}

bool Map::isSectorAt(int x, int y) const {
	ASSERT(x >= 0 && x < map_width_c && y >= 0 && y < map_height_c);
	return this->sector_at[x][y];
}

void Map::newSquareAt(int x,int y) {
	ASSERT(x >= 0 && x < map_width_c && y >= 0 && y < map_height_c);
	this->sector_at[x][y] = true;
}

void Map::createSectors(PlayingGameState *gamestate, int epoch) {
	ASSERT_EPOCH(epoch);
	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			if( sector_at[x][y] ) {
				this->sectors[x][y] = new Sector(gamestate, epoch, x, y, this->getColour());
				/*for(int i=0;i<N_ID;i++) {
				this->sectors[x][y]->setElements(i, this->elements[i]);
				}*/
			}
		}
	}
}

#if 0
void Map::checkSectors() const {
	//LOG("Map::checkSectors()\n");
#ifdef _WIN32
	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			if( sectors[x][y] != NULL ) {
				//LOG("check sector at %d, %d : %d\n", x, y, sectors[x][y]);
				if( !_CrtIsValidHeapPointer( sectors[x][y] ) ) {
					LOG("invalid sector at %d, %d\n", x, y);
					LOG("    ptr %d\n", sectors[x][y]);
					ASSERT( false );
				}
			}
		}
	}
#endif
	//LOG("Map::checkSectors exit\n");
}
#endif

void Map::freeSectors() {
    //LOG("Map::freeSectors()\n");
	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			if( sectors[x][y] != NULL ) {
                //LOG("free sector at %d, %d\n", x, y);
                //LOG("    ptr %d\n", sectors[x][y]);
				delete sectors[x][y];
				sectors[x][y] = NULL;
			}
		}
	}
	//current_sector = NULL;
    //LOG("Map::freeSectors exit\n");
}

/*void Map::setElements(int id,int n_elements) {
ASSERT_ELEMENT_ID(id);
this->elements[id] = n_elements;
}*/

void Map::findRandomSector(int *rx,int *ry) const {
	while(true) {
		int x = rand() % map_width_c;
		int y = rand() % map_height_c;
		if( sector_at[x][y] ) {
			*rx = x;
			*ry = y;
			break;
		}
	}
}

void Map::canMoveTo(bool temp[map_width_c][map_height_c], int sx,int sy,int player) const {
	ASSERT(player != -1 );
	ASSERT(this->sector_at[sx][sy]);
	//clearTemp();
	//bool temp[map_width_c][map_height_c];

	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			temp[x][y] = false;
		}
	}

	temp[sx][sy] = true;
	bool changed = false;
	do {
		changed = false;

		for(int x=0;x<map_width_c;x++) {
			for(int y=0;y<map_height_c;y++) {
				if( temp[x][y] ) {
					// can we move through this square?
					if( ( x == sx && y == sy ) ||
						sectors[x][y]->getPlayer() == player ||
						( sectors[x][y]->getPlayer() == -1 &&
						!sectors[x][y]->enemiesPresent(player) ) ) {
							for(int c=0;c<4;c++) {
								int cx = x, cy = y;
								if( c == 0 )
									cy--;
								else if( c == 1 )
									cx++;
								else if( c == 2 )
									cy++;
								else if( c == 3 )
									cx--;
								if( cx >= 0 && cy >= 0 && cx < map_width_c && cy < map_height_c
									&& sector_at[cx][cy]
								&& !sectors[x][y]->isNuked()
									&& !temp[cx][cy] ) {
										temp[cx][cy] = true;
										changed = true;
								}
							}
					}
				}
			}
		}
	} while( changed );
}

void Map::calculateStats() const {
	// deaths = start + births - remaining
	for(int i=0;i<n_players_c;i++) {
		if( game_g->players[i] != NULL ) {
			game_g->players[i]->setNDeaths( game_g->players[i]->getNMenForThisIsland() + game_g->players[i]->getNBirths() );
			game_g->players[i]->setNSuspended(0);
		}
	}
	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			if( this->sector_at[x][y] ) {
				Sector *sector = this->sectors[x][y];
				if( sector->getPlayer() != -1 && game_g->players[sector->getPlayer()] != NULL ) {
					// check for players[sector->getPlayer()] not being NULL should be redundant, but just to be safe
					game_g->players[sector->getPlayer()]->addNDeaths( - sector->getPopulation() );
					if( sector->isShutdown() && game_g->getGameResult() == GAMERESULT_WON ) {
						game_g->players[sector->getPlayer()]->addNSuspended(sector->getPopulation());
					}
				}
				for(int i=0;i<n_players_c;i++) {
					if( game_g->players[i] != NULL ) {
						game_g->players[i]->addNDeaths( - sector->getArmy(i)->getTotalMen() );
					}
				}
			}
		}
	}
	/*for(int i=0;i<n_players_c;i++) {
	if( players[i] != NULL )
	ASSERT( players[i]->n_deaths >= 0 );
	}*/
}

void Map::saveStateSectors(stringstream &stream) const {
	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			if( this->sector_at[x][y] ) {
				Sector *sector = this->sectors[x][y];
				sector->saveState(stream);
			}
		}
	}
}

/*bool Map::mapIs(char *that_name) {
//return strcmp( this->name, that_name ) == 0;
return this->name == that_name;
}*/

int Map::getNSquares() const {
	int n_squares = 0;
	for(int y=0;y<map_height_c;y++) {
		for(int x=0;x<map_width_c;x++) {
			if( this->sector_at[x][y] ) {
				n_squares++;
			}
		}
	}
	return n_squares;
}

void Map::draw(int offset_x, int offset_y) const {
    for(int y=0;y<map_height_c;y++) {
		for(int x=0;x<map_width_c;x++) {
			if( this->sector_at[x][y] ) {
				int icon = 0;
				if( y > 0 && this->sector_at[x][y-1] )
					icon += 1;
				if( x < map_width_c-1 && this->sector_at[x+1][y] )
					icon += 2;
				if( y < map_height_c-1 && this->sector_at[x][y+1] )
					icon += 4;
				if( x > 0 && this->sector_at[x-1][y] )
					icon += 8;
				ASSERT( icon >= 0 && icon < 16 );
				if( game_g->map_sq[colour][icon] == NULL ) {
					LOG("map name: %s\n", this->getName());
					LOG("ERROR map icon not available [%d,%d]: %d, %d\n", x, y, colour, icon);
					ASSERT( game_g->map_sq[colour][icon] != NULL );
				}
				int map_x = offset_x - game_g->getMapSqOffset() + 16 * x;
				int map_y = offset_y - game_g->getMapSqOffset() + 16 * y;
				int coast_map_x = offset_x - game_g->getMapSqCoastOffset() + 16 * x;
				int coast_map_y = offset_y - game_g->getMapSqCoastOffset() + 16 * y;
                //LOG("draw at: %d, %d : %d, %d\n", x, y, map_sq[colour][icon]->getWidth(), map_sq[colour][icon]->getHeight());
                game_g->map_sq[colour][icon]->draw(map_x, map_y);
				//LOG(">>> %d %d %d\n", icon, icon & 1, 4 & 1);
				if( (icon & 1) == 0 && game_g->coast_icons[0] != NULL )
					game_g->coast_icons[0]->draw(coast_map_x, coast_map_y);
				if( (icon & 2) == 0 && game_g->coast_icons[1] != NULL )
					game_g->coast_icons[1]->draw(coast_map_x, coast_map_y);
				if( (icon & 4) == 0 && game_g->coast_icons[2] != NULL )
					game_g->coast_icons[2]->draw(coast_map_x, coast_map_y);
				if( (icon & 8) == 0 && game_g->coast_icons[3] != NULL )
					game_g->coast_icons[3]->draw(coast_map_x, coast_map_y);

				// now do corners
				if( x > 0 && y > 0 && this->sector_at[x-1][y] && this->sector_at[x][y-1] && !this->sector_at[x-1][y-1] && game_g->coast_icons[4] != NULL )
					game_g->coast_icons[4]->draw(coast_map_x, coast_map_y);
				if( x < map_width_c-1 && y > 0 && this->sector_at[x+1][y] && this->sector_at[x][y-1] && !this->sector_at[x+1][y-1] && game_g->coast_icons[5] != NULL )
					game_g->coast_icons[5]->draw(coast_map_x, coast_map_y);
				if( x > 0 && y < map_height_c-1 && this->sector_at[x-1][y] && this->sector_at[x][y+1] && !this->sector_at[x-1][y+1] && game_g->coast_icons[6] != NULL )
					game_g->coast_icons[6]->draw(coast_map_x, coast_map_y);
				if( x < map_width_c-1 && y < map_height_c-1 && this->sector_at[x+1][y] && this->sector_at[x][y+1] && !this->sector_at[x+1][y+1] && game_g->coast_icons[7] != NULL )
					game_g->coast_icons[7]->draw(coast_map_x, coast_map_y);
			}
		}
	}
}

void Game::updatedEpoch() {
	ASSERT( start_epoch >= 0 && start_epoch < n_epochs_c );
	n_sub_epochs = 4;
	//if( start_epoch == n_epochs_c-1 )
	if( start_epoch == end_epoch_c )
		n_sub_epochs = 0;
	else if( start_epoch + n_sub_epochs > n_epochs_c ) {
		n_sub_epochs = n_epochs_c - start_epoch;
	}
}

void Game::setCurrentIsand(int start_epoch, int selected_island) {
	this->start_epoch = start_epoch;
	this->selected_island = selected_island;
	this->updatedEpoch();
	this->setCurrentMap();
}

void Game::setEpoch(int epoch) {
	LOG("set epoch %d\n", epoch);
	ASSERT( epoch >= 0 && epoch < n_epochs_c );
	start_epoch = epoch;
	updatedEpoch();

	selected_island = 0;

	if( gameType == GAMETYPE_ALLISLANDS ) {
		// skip islands we've completed
		while( completed_island[selected_island] ) {
			selected_island++;
			if( selected_island == max_islands_per_epoch_c || maps[start_epoch][selected_island] == NULL ) {
				LOG("error, should be at least one island on this epoch that isn't completed\n");
				ASSERT( false );
			}
		}
	}

	map = maps[start_epoch][selected_island];
    ASSERT( map != NULL );
    gamestate->reset();
}

void Game::drawProgress(int percentage) const {
	const int width = (int)(screen->getWidth() * 0.25f);
	const int height = (int)(screen->getHeight()/15.0f);
	const int xpos = (int)(screen->getWidth()*0.5f - width*0.5f);
	const int ypos = (int)(screen->getHeight()*0.5f - height*0.5f);

    screen->clear(); // n.b., needed for Qt/Symbian, where background defaults to white
	screen->fillRect(xpos, ypos, width+1, height+1, 255, 255, 255);
	int progress_width = (int)(((width-1) * percentage) / 100.0f);
	screen->fillRect(xpos+1, ypos+1, progress_width, height-1, 183, 28, 28);
	
	screen->refresh();
}

void Game::newGame() {
	gamestate->fadeScreen(false, 0, NULL);
	LOG("newGame()\n");
	ASSERT( gameStateID == GAMESTATEID_PLACEMEN );
	if( gameType == GAMETYPE_SINGLEISLAND )
		n_men_store = 1000;
	else
		n_men_store = getMenPerEpoch();

	for(int i=0;i<max_islands_per_epoch_c;i++)
		completed_island[i] = false;
	//completed_island[0] = true;

	n_player_suspended = 0;

	setEpoch(0);

	//n_men_store = 1000;
	//setEpoch(9);
}

void Game::setClientPlayer(int set_client_player) {
	this->human_player = set_client_player;
	if( gamestate != NULL ) {
		gamestate->setClientPlayer(set_client_player);
	}
}

void Game::nextEpoch() {
	LOG("nextEpoch()\n");
	start_epoch++;
	if( start_epoch == n_epochs_c ) {
		ASSERT( gameType == GAMETYPE_SINGLEISLAND );
		start_epoch = 0;
	}
	for(int i=0;i<max_islands_per_epoch_c;i++)
		completed_island[i] = false;
	setEpoch(start_epoch);
}

void Game::nextIsland() {
	if( gameType == GAMETYPE_SINGLEISLAND ) {
		selected_island++;
		if( selected_island == max_islands_per_epoch_c || maps[start_epoch][selected_island] == NULL )
			selected_island = 0;
	}
	else {
		// skip islands we've completed
		do {
			selected_island++;
			if( selected_island == max_islands_per_epoch_c || maps[start_epoch][selected_island] == NULL )
				selected_island = 0;
		} while( completed_island[selected_island] );
	}
	LOG("next island: %d\n", selected_island);

	map = maps[start_epoch][selected_island];
	LOG("map name: %s\n", map==NULL ? "NULL?!" : map->getName());
	gamestate->reset();
    LOG("done reset\n");
}

bool Game::loadGameInfo(DifficultyLevel *difficulty, int *player, int *n_men, int suspended[n_players_c], int *epoch, bool completed[max_islands_per_epoch_c], const char *filename) const {
    //LOG("loadGameInfo(%d)\n",slot);
	ASSERT( gameStateID == GAMESTATEID_PLACEMEN );

    //LOG("loading: %s\n", filename);
	FILE *file = fopen(filename, "rb+");
	if( file == NULL ) {
        //LOG("FAILED to open file\n");
		return false;
	}

	const int bufsize = 1024;
	char buffer[bufsize+1] = "";
	if( fgets(buffer, bufsize, file) == NULL ) { // header line
		return false;
	}

	// data line
	/*for(int i=0;i<8;i++) {
	buffer[i] = fgetc(file);
	}*/
	for(int i=0;;) {
		ASSERT( i <= bufsize );
		//buffer[i] = fgetc(file);
		int c = fgetc(file);
		if( c == EOF ) {
			buffer[i] = '\0';
			break;
		}
		buffer[i] = (char)c;
		i++;
	}

	char *ptr = buffer;

	*difficulty = (DifficultyLevel)*((int *)ptr);
	ptr += sizeof(int);
	if( !validDifficulty( *difficulty ) )
		return false;

	*player = *((int *)ptr);
	ptr += sizeof(int);
	if( !validPlayer( *player ) )
		return false;

	*n_men = *((int *)ptr);
	ptr += sizeof(int);
	if( *n_men < 0 )
		return false;

	for(int i=0;i<n_players_c;i++) {
		suspended[i] = *((int *)ptr);
		ptr += sizeof(int);
		if( suspended[i] < 0 )
			return false;
	}

	*epoch = *((int *)ptr);
	ptr += sizeof(int);
	if( *epoch < 0 || *epoch >= n_epochs_c )
		return false;

	for(int i=0;i<max_islands_per_epoch_c;i++) {
		int val = *((int *)ptr);
		if( val != 0 && val != 1 )
			return false;
		completed[i] = (val==1);
		ptr += sizeof(int);
	}

	fclose(file);
	return true;
}

bool Game::loadGame(const char *filename) {
	LOG("loadGame(%s)\n",filename);
	ASSERT( gameType == GAMETYPE_ALLISLANDS );
	ASSERT( gameStateID == GAMESTATEID_PLACEMEN );

	DifficultyLevel temp_difficulty = DIFFICULTY_EASY;
	int temp_player = 0;
	int temp_n_men_store = 0;
	int temp_start_epoch = 0;
	int temp_suspended[n_players_c];
	bool temp_completed[max_islands_per_epoch_c];
	if( loadGameInfo(&temp_difficulty, &temp_player, &temp_n_men_store, temp_suspended, &temp_start_epoch, temp_completed, filename) ) {
		difficulty_level = temp_difficulty;
		setClientPlayer(temp_player);
		n_men_store = temp_n_men_store;
		n_player_suspended = temp_suspended[temp_player];
		for(int i=0;i<max_islands_per_epoch_c;i++)
			completed_island[i] = temp_completed[i];
		setEpoch(temp_start_epoch);
		return true;
	}
	return false;
}

const char *Game::getFilename(int slot) const {
	char name[300] = "";
	sprintf(name, "game_%d.SAV", slot);
	const char *filename = getApplicationFilename(name, true); // important for save game files to survive an uninstall
    //LOG("filename: %s\n", filename);
	return filename;
}

bool Game::loadGameInfo(DifficultyLevel *difficulty, int *player, int *n_men, int suspended[n_players_c], int *epoch, bool completed[max_islands_per_epoch_c], int slot) {
	ASSERT( slot >= 0 && slot < n_slots_c );
	const char *filename = getFilename(slot);
	bool ok = loadGameInfo(difficulty, player, n_men, suspended, epoch, completed, filename);
	delete [] filename;
	return ok;
}

bool Game::loadGame(int slot) {
	LOG("loadGame(%d)\n",slot);
	ASSERT( slot >= 0 && slot < n_slots_c );
	const char *filename = getFilename(slot);
	bool ok = loadGame(filename);
	delete [] filename;
	return ok;
}

void Game::saveGame(int slot) const {
	LOG("saveGame(%d)\n",slot);
	ASSERT( gameType == GAMETYPE_ALLISLANDS );
	ASSERT( gameStateID == GAMESTATEID_PLACEMEN );
	ASSERT( slot >= 0 && slot < n_slots_c );

	const char *filename = getFilename(slot);
	FILE *file = fopen(filename, "wb+");
	delete [] filename;
	if( file == NULL ) {
		LOG("FAILED to open file\n");
		return;
	}

	const int savVersion = 1;
	fprintf(file, "GLMSAV%d.%d.%d\n", majorVersion, minorVersion, savVersion);

	char buffer[256] = "";
	char *ptr = buffer;

	*((int *)ptr) = difficulty_level;
	ptr += sizeof(int);

	*((int *)ptr) = human_player;
	ptr += sizeof(int);

	*((int *)ptr) = n_men_store;
	ptr += sizeof(int);

	int n_suspended[n_players_c]; // no longer use n_suspended for all players, but still need to keep save game files compatible
	for(int i=0;i<n_players_c;i++) {
		n_suspended[i] = 0;
	}
	n_suspended[human_player] = n_player_suspended;
	for(int i=0;i<n_players_c;i++) {
		*((int *)ptr) = n_suspended[i];
		ptr += sizeof(int);
	}

	*((int *)ptr) = start_epoch;
	ptr += sizeof(int);

	for(int i=0;i<max_islands_per_epoch_c;i++) {
		int val = completed_island[i] ? 1 : 0;
		*((int *)ptr) = val;
		ptr += sizeof(int);
	}

	ptrdiff_t n_bytes = ptr - buffer;
	int sum = 0;
	for(int i=0;i<n_bytes;i++) {
		sum += buffer[i];
	}

	LOG("checksum %d\n", sum);

	*((int *)ptr) = sum;
	ptr += sizeof(int);

	*ptr = '\0';

	//fprintf(file, "%s\n", buffer);
	//fwrite(buffer, (int)ptr - (int)buffer, 1, file);
	ptrdiff_t diff = ptr - buffer;
	fwrite(buffer, diff, 1, file);

	fclose(file);
}

bool Game::validPlayer(int player) const {
	bool valid = player >= 0 && player < n_players_c;
	if( !valid ) {
		LOG("ERROR invalid player %d\n", player);
	}
	return valid;
}

void Game::addTextEffect(TextEffect *effect) {
	gamestate->addTextEffect(effect);
}

void Game::stopMusic() {
	if( music != NULL ) {
		delete music;
		music = NULL;
	}
}

void Game::fadeMusic(int duration_ms) const {
	if( music != NULL ) {
		music->fadeOut(duration_ms);
	}
}

void Game::playMusic() {
	stopMusic();

	if( !pref_music_on ) {
		// don't play any music
	}
	else if( gameStateID == GAMESTATEID_CHOOSEPLAYER ) {
	}
	else if( gameStateID == GAMESTATEID_PLACEMEN ) {
#ifdef WINRT
		music = Sample::loadMusic("Assets/music/mainscreen.ogg");
#else
		music = Sample::loadMusic("music/mainscreen.ogg");
#endif
		// n.b., a music structure is always created, even if we fail to load, so no need to check for NULL pointers here (though we do elsewhere, as music not created if pref_music_on is false)
		music->play(SOUND_CHANNEL_MUSIC, -1);
	}
	else if( gameStateID == GAMESTATEID_PLAYING ) {
#ifdef WINRT
		music = Sample::loadMusic("Assets/music/gamemusic.ogg");
#else
		music = Sample::loadMusic("music/gamemusic.ogg");
#endif
		// n.b., a music structure is always created, even if we fail to load, so no need to check for NULL pointers here (though we do elsewhere, as music not created if pref_music_on is false)
		music->play(SOUND_CHANNEL_MUSIC, -1);
	}
	else if( gameStateID == GAMESTATEID_ENDISLAND ) {
		if( gameResult == GAMERESULT_WON )
#ifdef WINRT
			music = Sample::loadMusic("Assets/music/victory.ogg");
#else
			music = Sample::loadMusic("music/victory.ogg");
#endif
		else
#ifdef WINRT
			music = Sample::loadMusic("Assets/music/defeat.ogg");
#else
			music = Sample::loadMusic("music/defeat.ogg");
#endif
		// n.b., a music structure is always created, even if we fail to load, so no need to check for NULL pointers here (though we do elsewhere, as music not created if pref_music_on is false)
		music->play(SOUND_CHANNEL_MUSIC, 0); // don't loop
	}
}

bool Game::loadSamples() {
#ifdef WINRT
	string sound_dir = "Assets/sound/";
#else
	string sound_dir = "sound/";
#endif

	s_design_is_ready = Sample::loadSample(sound_dir + "the_design_is_finished.wav");
	s_design_is_ready->setText("the design is finished");
	s_ergo = Sample::loadSample(sound_dir + "ergonomically_terrific.wav");
	s_ergo->setText("ergonomically terrific!");
	s_fcompleted = Sample::loadSample(sound_dir + "the_production_run_s_completed.wav");
	s_fcompleted->setText("the production run's completed");
	s_advanced_tech = Sample::loadSample(sound_dir + "we_ve_advanced_a_tech_level.wav");
	s_advanced_tech->setText("we've advanced a tech level!");
	s_running_out_of_elements = Sample::loadSample(sound_dir + "we_re_running_out_of_elements.wav");
	s_running_out_of_elements->setText("we're running out of elements");
	s_tower_critical = Sample::loadSample(sound_dir + "tower_critical.wav");
	s_tower_critical->setText("tower critical!");
	s_sector_destroyed = Sample::loadSample(sound_dir + "the_sector_s_been_destroyed.wav");
	s_sector_destroyed->setText("the sector's been destroyed!");
	s_mine_destroyed = Sample::loadSample(sound_dir + "the_mine_is_destroyed.wav");
	s_mine_destroyed->setText("the mine is destroyed");
	s_factory_destroyed = Sample::loadSample(sound_dir + "the_factory_s_been_destroyed.wav");
	s_factory_destroyed->setText("the factory's been destroyed");
	s_lab_destroyed = Sample::loadSample(sound_dir + "the_lab_s_been_destroyed.wav");
	s_lab_destroyed->setText("the lab's been destroyed");
	s_itis_all_over = Sample::loadSample(sound_dir + "it_s_all_over.wav");
	s_itis_all_over->setText("game over");
	s_conquered = Sample::loadSample(sound_dir + "we_ve_conquered_the_sector.wav");
	s_conquered->setText("we've conquered the sector");
	s_won = Sample::loadSample(sound_dir + "we_ve_won.wav");
	s_won->setText("we've won!");
	s_weve_nuked_them = Sample::loadSample(sound_dir + "we_ve_nuked_them.wav");
	s_weve_nuked_them->setText("we've nuked them!");
	s_weve_been_nuked = Sample::loadSample(sound_dir + "we_ve_been_nuked.wav");
	s_weve_been_nuked->setText("we've been nuked!");
	s_on_hold = Sample::loadSample(sound_dir + "putting_you_on_hold.wav");
	// no text for s_on_hold
	s_alliance_yes[0] = new Sample();
	s_alliance_yes[0]->setText("red team says okay");
	s_alliance_yes[1] = new Sample();
	s_alliance_yes[1]->setText("green team says okay");
	s_alliance_yes[2] = new Sample();
	s_alliance_yes[2]->setText("yellow team says okay");
	s_alliance_yes[3] = new Sample();
	s_alliance_yes[3]->setText("blue team says okay");
	s_alliance_no[0] = new Sample();
	s_alliance_no[0]->setText("red team says no");
	s_alliance_no[1] = new Sample();
	s_alliance_no[1]->setText("green team says no");
	s_alliance_no[2] = new Sample();
	s_alliance_no[2]->setText("yellow team says no");
	s_alliance_no[3] = new Sample();
	s_alliance_no[3]->setText("blue team says no");
	s_alliance_ask[0] = new Sample();
	s_alliance_ask[0]->setText("red team requests alliance");
	s_alliance_ask[1] = new Sample();
	s_alliance_ask[1]->setText("green team requests alliance");
	s_alliance_ask[2] = new Sample();
	s_alliance_ask[2]->setText("yellow team requests alliance");
	s_alliance_ask[3] = new Sample();
	s_alliance_ask[3]->setText("blue team requests alliance");

	// text messages without corresponding samples
	//s_cant_nuke_ally = new Sample(NULL);
	s_cant_nuke_ally = new Sample();
	s_cant_nuke_ally->setText("cannot nuke our ally");

	// no samples or text messages, but keep supported in case we re-add samples later
	s_quit[0] = new Sample();
	s_quit[1] = new Sample();
	s_quit[2] = new Sample();
	s_quit[3] = new Sample();

	// sound effects
	s_explosion = Sample::loadSample(sound_dir + "bomb.wav");
#if !defined(__ANDROID__) && defined(__linux)
        if( s_explosion == NULL || errorSound() ) {
            if( s_explosion != NULL ) {
                delete s_explosion;
                s_explosion = NULL;
            }
		resetErrorSound();
		sound_dir = "/usr/share/gigalomania/" + sound_dir;
		LOG("look in %s for sound\n", sound_dir.c_str());
        s_explosion = Sample::loadSample(sound_dir + "bomb.wav");
	}
#endif

    s_scream = Sample::loadSample(sound_dir + "pain1.wav");
	s_buildingdestroyed = Sample::loadSample(sound_dir + "woodbrk.wav");
    s_guiclick = Sample::loadSample(sound_dir + "misc_menu_3.wav");
    s_biplane = Sample::loadSample(sound_dir + "biplane.ogg");
    s_jetplane = Sample::loadSample(sound_dir + "jetplane.ogg");
    s_spaceship = Sample::loadSample(sound_dir + "spaceship.ogg");

	bool ok = !errorSound();
	return ok;
}

bool remapLand(Image *image,MapColour colour) {
	for(int y=0;y<image->getHeight();y++) {
		for(int x=0;x<image->getWidth();x++) {
			unsigned char c = image->getPixelIndex(x,y);
			if( c == 0 ) {
				// leave as 0
			}
			else if( c == 1 ) {
				// light
				int c2 = 0;
				if( colour == MAP_ORANGE )
					c2 = 15;
				else if( colour == MAP_GREEN )
					c2 = 1;
				else if( colour == MAP_BROWN )
					c2 = 14;
				else if( colour == MAP_WHITE )
					c2 = 2;
				else if( colour == MAP_DBROWN )
					c2 = 6;
				else if( colour == MAP_DGREEN )
					c2 = 12;
				else if( colour == MAP_GREY )
					c2 = 1;
				else {
					ASSERT(false);
				}
				image->setPixelIndex(x, y, c2);
			}
			else if( c == 2 ) {
				// medium
				int c2 = 0;
				if( colour == MAP_ORANGE )
					c2 = 14;
				else if( colour == MAP_GREEN )
					c2 = 12;
				else if( colour == MAP_BROWN )
					c2 = 6;
				else if( colour == MAP_WHITE )
					c2 = 1;
				else if( colour == MAP_DBROWN )
					c2 = 8;
				else if( colour == MAP_DGREEN )
					c2 = 7;
				else if( colour == MAP_GREY )
					c2 = 5;
				else {
					ASSERT(false);
				}
				image->setPixelIndex(x, y, c2);
			}
			else if( c == 3 ) {
				// dark
				int c2 = 0;
				if( colour == MAP_ORANGE )
					c2 = 6;
				else if( colour == MAP_GREEN )
					c2 = 7;
				else if( colour == MAP_BROWN )
					c2 = 8;
				else if( colour == MAP_WHITE )
					c2 = 5;
				else if( colour == MAP_DBROWN )
					c2 = 3;
				else if( colour == MAP_DGREEN )
					c2 = 8;
				else if( colour == MAP_GREY )
					c2 = 4;
				else {
					ASSERT(false);
				}
				image->setPixelIndex(x, y, c2);
			}
			else {
				//LOG("land slabs should only have colour indices 0-3\n");
				//return false;
			}
		}
	}
	return true;
}

void Game::convertToHiColor(Image *image) const {
	if( using_old_gfx ) {
		// create alpha from color keys
		image->createAlphaForColor(true, 255, 0, 255, 127, 0, 127, shadow_alpha_c);
	}
	image->convertToHiColor(false);
}

void Game::processImage(Image *image, bool old_smooth) const {
    //LOG("    convert to hi color\n");
	convertToHiColor(image);
    //LOG("    scale\n");
    image->scale(scale_factor_w, scale_factor_h);
    //LOG("    set scale\n");
    image->setScale(scale_width, scale_height);
#if SDL_MAJOR_VERSION == 1
	// with SDL 2, we let SDL do smoothing when scaling the graphics on the GPU
	if( using_old_gfx && old_smooth ) {
        image->smooth();
	}
#endif
    //LOG("    done\n");
}

bool Game::loadAttackersWalkingImages(const string &gfx_dir, int epoch) {
	char filename[300] = "";
	sprintf(filename, "attacker_walking_%d.png", epoch);
	Image *gfx_image = Image::loadImage(gfx_dir + filename);
	// if NULL, look for direction specific graphics
	if( gfx_image != NULL ) {
	    gfx_image->setScale(scale_width/scale_factor_w, scale_height/scale_factor_h); // so the copying will work at the right scale for the input image
	}
	for(int dir=0;dir<n_attacker_directions_c;dir++) {
		bool direction_specific = false;
		if( gfx_image == NULL ) {
			// load direction specific image
			//LOG("try loading direction specific images for epoch %d dir %d\n", epoch, dir);
			direction_specific = true;
			sprintf(filename, "attacker_walking_%d_%d.png", epoch, dir);
			gfx_image = Image::loadImage(gfx_dir + filename);
			if( gfx_image == NULL ) {
				LOG("failed to load attacker walking image for epoch %d dir %d\n", epoch, dir);
				return false;
			}
		    gfx_image->setScale(scale_width/scale_factor_w, scale_height/scale_factor_h); // so the copying will work at the right scale for the input image
		}
		int height_per_frame = gfx_image->getScaledHeight();
		int width_per_frame = height_per_frame;
		n_attacker_frames[epoch][dir] = gfx_image->getScaledWidth()/width_per_frame;
		//LOG("epoch %d, direction %d has %d frames\n", epoch, dir, n_attacker_frames[epoch][dir]);
		// need to update max_attacker_frames_c if we ever want to allow more frames!
		ASSERT( n_attacker_frames[epoch][dir] <= max_attacker_frames_c );
		for(int player=0;player<n_players_c;player++) {
			int n_frames = n_attacker_frames[epoch][dir];
			for(int frame=0;frame<n_frames;frame++) {
				attackers_walking[player][epoch][dir][frame] = gfx_image->copy(width_per_frame*frame, 0, width_per_frame, height_per_frame);
				int r = 0, g = 0, b = 0;
				PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)player);
				attackers_walking[player][epoch][dir][frame]->remap(240, 0, 0, r, g, b);
				processImage(attackers_walking[player][epoch][dir][frame]);
			}
		}
		if( direction_specific ) {
			delete gfx_image;
			gfx_image = NULL;
		}
	}
	if( gfx_image != NULL ) {
		delete gfx_image;
	}
	return true;
}

void Game::calculateScale(const Image *image) {
#if SDL_MAJOR_VERSION == 1
	scale_factor_w = ((float)(scale_width*default_width_c))/(float)image->getWidth();
	scale_factor_h = ((float)(scale_height*default_height_c))/(float)image->getHeight();
	LOG("scale factor for images = %f X %f\n", scale_factor_w, scale_factor_h);
#else
	// with SDL 2, we don't scale the graphics, and instead set the logical size to match the graphics
	scale_factor_w = 1.0f;
	scale_factor_h = 1.0f;
	scale_width = ((float)(image->getWidth()))/(float)default_width_c;
	scale_height = ((float)(image->getHeight()))/(float)default_height_c;
	LOG("scale width/height of logical resolution = %f X %f\n", scale_width, scale_height);
	screen->setLogicalSize((int)(scale_width*default_width_c), (int)(scale_height*default_height_c), true);
#endif
}

bool Game::loadOldImages() {
	// progress should go from 0 to 80%
	LOG("try using old graphics\n");
	using_old_gfx = true;

	background = Image::loadImage("data/mlm_sunrise");

	if( background == NULL )
		return false;
	drawProgress(20);
	// do equivalent for calculateScale(), but for a 320x240 image (n.b., not 320x256)
#if SDL_MAJOR_VERSION == 1
	scale_factor_w = scale_width;
	scale_factor_h = scale_height;
	LOG("scale factor for images = %f X %f\n", scale_factor_w, scale_factor_h);
#else
	// with SDL 2, we don't scale the graphics, and instead set the logical size to match the graphics
	scale_factor_w = 1.0f;
	scale_factor_h = 1.0f;
	scale_width = 1.0f;
	scale_height = 1.0f;
	LOG("scale width/height of logical resolution = %f X %f\n", scale_width, scale_height);
	screen->setLogicalSize((int)(scale_width*default_width_c), (int)(scale_height*default_height_c), false); // don't smooth, as doesn't look too good with old graphics
#endif

	// nb, still scale if scale_factor==1, as this is a way of converting to 8bit
	processImage(background);

	for(int i=0;i<n_players_c;i++) {
		player_heads_select[i] = NULL;
		player_heads_alliance[i] = NULL;
	}
	grave = NULL;

	Image *image_slabs = Image::loadImage("data/mlm_slabs");
	if( image_slabs == NULL )
		return false;
	drawProgress(30);
	for(int i=0;i<MAP_N_COLOURS;i++) {
		land[i] = image_slabs->copy();
		//land[i]->scale(scale_factor, scale_factor); // also forces to 8bit (since should be paletted)
		land[i]->scale(scale_factor_w, scale_factor_h); // also forces to 8bit (since should be paletted)
		land[i]->setScale(scale_width, scale_height);
		if( !land[i]->isPaletted() ) {
			LOG("land slabs should be paletted\n");
			return false;
		}
		else if( land[i]->getNColors() < 16 ) { // should be 256 by now anyway, due to scaling
			LOG("land slabs should have at least 16 colours\n");
			return false;
		}
		else {
			// 1, 2, 3, 4, 5, 6, 7, 8, 12, 14, 15
			land[i]->setColor(0, 255, 0, 255);
			land[i]->setColor(1, 176, 176, 176);
			land[i]->setColor(2, 240, 240, 240);
			land[i]->setColor(3, 0, 0, 0);
			land[i]->setColor(4, 64, 64, 64);
			land[i]->setColor(5, 112, 112, 112);
			land[i]->setColor(6, 160, 64, 0);
			land[i]->setColor(7, 0, 96, 0);
			land[i]->setColor(8, 112, 36, 16);
			land[i]->setColor(12, 0, 192, 0);
			land[i]->setColor(14, 240, 112, 16);
			land[i]->setColor(15, 240, 240, 0);
		}
		if( !remapLand(land[i], (MapColour)i) ) {
			LOG("failed to remap land\n");
			return false;
		}
		convertToHiColor(land[i]);
#if SDL_MAJOR_VERSION == 1
		// with SDL 2, we let SDL do smoothing when scaling the graphics on the GPU
		land[i]->smooth();
#endif
	}
	delete image_slabs;
	image_slabs = NULL;

	for(int i=0;i<n_epochs_c;i++) {
		fortress[i] = NULL;
		mine[i] = NULL;
		factory[i] = NULL;
	}

	Image *buildings = Image::loadImage("data/mlm_buildings");
	if( buildings == NULL )
		return false;
	buildings->setColor(0, 255, 0, 255);
	processImage(buildings);

	for(int i=0;i<n_epochs_c;i++) {
		fortress[i] = buildings->copy( 0, 60*i, 58, 60);
		mine[i] = buildings->copy( 64, 60*i, 58, 60);
		factory[i] = buildings->copy( 192, 60*i, 58, 60);
		lab[i] = buildings->copy( 128, 60*i, 58, 60);
	}
	delete buildings;
	buildings = NULL;
	drawProgress(40);

	Image *icons = Image::loadImage("data/mlm_icons");
	if( icons == NULL )
		return false;
	if( !icons->scaleTo((int)(scale_width*default_width_c)) ) // must use this method, as still using alongside the new gfx images
		return false;
	//icons->scale(scale_factor, scale_factor);
	icons->setScale(scale_width, scale_height);
	icons->setColor(0, 255, 0, 255);
	convertToHiColor(icons);
#if SDL_MAJOR_VERSION == 1
	// with SDL 2, we let SDL do smoothing when scaling the graphics on the GPU
	icons->smooth();
#endif

	for(int i=0;i<n_epochs_c;i++)
		men[i] = icons->copy(16*i, 0, 16, 16);

	unarmed_man = icons->copy(80, 32, 16, 16);

	panel_design = icons->copy(304, 0, 16, 16);
	//panel_design_dark = panel_design->copy();
	panel_lab = icons->copy(16, 33, 16, 16);
	panel_shield = icons->copy(240, 0, 16, 16);
	panel_defence = icons->copy(256, 0, 16, 16);
	panel_attack = icons->copy(272, 0, 16, 16);
	panel_knowndesigns = icons->copy(240, 16, 16, 16);
	panel_factory = icons->copy(48, 33, 16, 16);
	panel_bloody_attack = icons->copy(240, 32, 16, 16);

	mine_gatherable_small = icons->copy(160, 0, 16, 16);

	//panel_build[BUILDING_TOWER] = icons->copy(224, 63, 19, 16); // not yet used
	panel_build[BUILDING_MINE] = icons->copy(256, 63, 19, 16);
	panel_build[BUILDING_FACTORY] = icons->copy(288, 63, 19, 16);
	panel_build[BUILDING_LAB] = icons->copy(192, 63, 19, 16);

	panel_building[BUILDING_TOWER] = icons->copy(0, 33, 16, 14);
	panel_building[BUILDING_MINE] = icons->copy(32, 33, 16, 14);
	//panel_building[BUILDING_FACTORY] = icons->copy(48, 33, 16, 14);
	//panel_building[BUILDING_LAB] = icons->copy(16, 33, 16, 14);
	panel_building[BUILDING_FACTORY] = panel_factory;
	panel_building[BUILDING_LAB] = panel_lab;

	mine_gatherable_large = icons->copy(160, 64, 32, 16);

	panel_bigdesign = icons->copy(256, 48, 32, 15);
	panel_biglab = icons->copy(96, 48, 32, 16);
	panel_bigfactory = icons->copy(128, 48, 32, 16);
	panel_bigshield = icons->copy(160, 48, 32, 15);
	panel_bigdefence = icons->copy(192, 48, 32, 15);
	panel_bigattack = icons->copy(224, 48, 32, 15);
	panel_bigbuild = icons->copy(32, 49, 32, 15);
	panel_bigknowndesigns = icons->copy(288, 48, 32, 15);
	panel_twoattack = icons->copy(64, 33, 15, 15);

	for(int i=0;i<10;i++)
		numbers_blue[i] = icons->copy(16*i, 64, 6, 8);
	for(int i=0;i<10;i++)
		numbers_grey[i] = icons->copy(16*i, 72, 6, 8);
	for(int i=0;i<10;i++)
		numbers_white[i] = icons->copy(16*i, 80, 6, 8);
	for(int i=0;i<10;i++)
		numbers_orange[i] = icons->copy(16*i, 88, 6, 8);
	for(int i=0;i<10;i++)
		numbers_yellow[i] = icons->copy(16*i, 119, 6, 8);
	for(int i=0;i<3;i++)
		numbers_largeshiny[i] = icons->copy(16*i, 269, 6, 15);
	for(int i=3;i<10;i++)
		numbers_largeshiny[i] = NULL; // not used
	for(int i=0;i<10;i++)
		numbers_largegrey[i] = icons->copy(64 + 16*i, 269, 6, 15);
	for(int i=0;i<10;i++)
		numbers_small[0][i] = icons->copy(16*i, 126, 5, 7);
	for(int i=0;i<10;i++)
		numbers_small[1][i] = icons->copy(16*i, 134, 5, 7);
	for(int i=0;i<10;i++)
		numbers_small[2][i] = icons->copy(160 + 16*i, 126, 5, 7);
	for(int i=0;i<10;i++)
		numbers_small[3][i] = icons->copy(160 + 16*i, 134, 5, 7);
	numbers_half = icons->copy(160, 119, 6, 8);

	for(int i=0;i<n_font_chars_c;i++) {
		letters_large[i] = NULL;
		letters_small[i] = NULL;
	}
	for(int i=0;i<4;i++)
		letters_large[i] = icons->copy(256 + 16*i, 269, 6, 15);
	for(int i=0;i<20;i++)
		letters_large[4+i] = icons->copy(16*i, 284, 6, 15);
	for(int i=0;i<2;i++)
		letters_large[24+i] = icons->copy(16*i, 299, 6, 15);

	for(int i=0;i<15;i++)
		letters_small[i] = icons->copy(80 + 16*i, 299, 6, 8);
	for(int i=0;i<11;i++)
		letters_small[15+i] = icons->copy(80 + 16*i, 307, 6, 8);

	for(int i=0;i<n_players_c;i++)
		mouse_pointers[i] = icons->copy(176 + 16*i, 0, 16, 16);

	for(int i=0;i<13;i++)
		icon_clocks[i] = icons->copy(16*i, 16, 16, 16);

	/*for(int i=0;i<3;i++)
		icon_mice[i] = icons->copy(256 + 16*i, 16, 16, 19);*/
	for(int i=0;i<2;i++)
		icon_mice[i] = icons->copy(256 + 16*i, 16, 16, 19);

	for(int i=0;i<n_death_flashes_c;i++)
		death_flashes[i] = icons->copy(272 + 16*i, 346, 16, 19);

	for(int i=0;i<5;i++)
		blue_flashes[i] = icons->copy(225 + 16*i, 173, 16, 16);
	blue_flashes[5] = icons->copy(305, 165, 15, 19);
	blue_flashes[6] = icons->copy(305, 184, 15, 17);

	for(int i=0;i<n_explosions_c;i++)
		explosions[i] = NULL;

	for(int i=0;i<n_players_c;i++) {
		icon_towers[i] = icons->copy(160 + 16*i, 81, 6, 6);
		icon_armies[i] = icons->copy(160 + 16*i, 87, 4, 4);
	}

	for(int i=0;i<N_ID;i++)
		icon_elements[i] = icons->copy(16*i, 141, 16, 16);

	for(int i=0;i<n_players_c;i++)
		flags[i][0] = icons->copy(160 + 16*i, 157, 16, 15);
	for(int i=0;i<n_players_c;i++)
		flags[i][1] = icons->copy(224 + 16*i, 157, 16, 15);
	for(int i=0;i<n_players_c;i++)
		flags[i][2] = icons->copy(160 + 16*i, 172, 16, 15);
	for(int i=0;i<n_players_c;i++)
		flags[i][3] = flags[i][1];

	for(int i=0;i<n_playershields_c;i++)
		playershields[i] = icons->copy(16*i, 189, 16, 14);

	for(int i=0;i<n_shields_c;i++)
		icon_shields[i] = icons->copy(112 + 16*i, 32, 16, 16);
	for(int i=0;i<n_epochs_c;i++)
		icon_defences[i] = icons->copy(16*i, 253, 16, 16);
	for(int i=0;i<n_epochs_c;i++)
		icon_weapons[i] = icons->copy(16*i, 237, 16, 16);
	for(int i=0;i<n_epochs_c;i++)
		numbered_defences[i] = icons->copy(16*i, 173, 16, 16);
	for(int i=0;i<n_epochs_c;i++)
		numbered_weapons[i] = icons->copy(16*i, 157, 16, 16);

	for(int i=0;i<3;i++)
		icon_speeds[i] = icons->copy(272 + 16*i, 204, 16, 18);

	building_health = icons->copy(0, 203, 41, 5);
	dash_grey = icons->copy(144, 114, 5, 2);
	icon_shield = icons->copy(272, 315, 16, 16);
	icon_defence = icons->copy(288, 315, 16, 16);
	icon_weapon = icons->copy(304, 315, 16, 16);

	icon_infinity = icons->copy(112, 104, 12, 8);

	icon_bc = icons->copy(240, 276, 12, 8);
	icon_ad = icons->copy(224, 276, 12, 8);
	icon_ad_shiny = icons->copy(48, 276, 12, 8);

	icon_nuke_hole = icons->copy(288, 99, 9, 9);

	icon_ergo = icons->copy(176, 111, 16, 16);
	icon_trash = icons->copy(192, 111, 16, 16);

	mapsquare = icons->copy(288, 81, 17, 17);
	flashingmapsquare = icons->copy(192, 92, 17, 17);

	delete icons;
	icons = NULL;
	drawProgress(50);

	for(int i=0;i<n_coast_c;i++)
		coast_icons[i] = NULL;

	Image *smallmap = Image::loadImage("data/mlm_smallmap");
	if( smallmap == NULL )
		return false;
	smallmap->setColor(0, 255, 0, 255);
	processImage(smallmap);
	map_sq_offset = 3;
	map_sq_coast_offset = 3;

	for(int i=0;i<MAP_N_COLOURS;i++) {
		for(int j=0;j<n_map_sq_c;j++)
			map_sq[i][j] = NULL;
	}

	map_sq[MAP_ORANGE][14] = smallmap->copy(0, 0, 22, 22);
	map_sq[MAP_ORANGE][7] = smallmap->copy(32, 0, 22, 22);
	map_sq[MAP_ORANGE][11] = smallmap->copy(64, 0, 22, 22);
	map_sq[MAP_ORANGE][4] = smallmap->copy(96, 0, 22, 22);
	map_sq[MAP_ORANGE][5] = smallmap->copy(128, 0, 22, 22);
	map_sq[MAP_ORANGE][1] = smallmap->copy(160, 0, 22, 22);
	map_sq[MAP_ORANGE][2] = smallmap->copy(192, 0, 22, 22);
	map_sq[MAP_ORANGE][8] = smallmap->copy(224, 0, 22, 22);

	map_sq[MAP_GREEN][6] = smallmap->copy(256, 0, 22, 22);
	map_sq[MAP_GREEN][12] = smallmap->copy(288, 0, 22, 22);
	map_sq[MAP_GREEN][3] = smallmap->copy(0, 22, 22, 22);
	map_sq[MAP_GREEN][9] = smallmap->copy(32, 22, 22, 22);
	map_sq[MAP_GREEN][5] = smallmap->copy(64, 22, 22, 22);
	map_sq[MAP_GREEN][10] = smallmap->copy(96, 22, 22, 22);

	map_sq[MAP_BROWN][6] = smallmap->copy(128, 22, 22, 22);
	map_sq[MAP_BROWN][4] = smallmap->copy(160, 22, 22, 22);
	map_sq[MAP_BROWN][1] = smallmap->copy(192, 22, 22, 22);
	map_sq[MAP_BROWN][8] = smallmap->copy(224, 22, 22, 22);
	map_sq[MAP_BROWN][0] = smallmap->copy(256, 22, 22, 22);

	map_sq[MAP_DBROWN][6] = smallmap->copy(32, 66, 22, 22);
	map_sq[MAP_DBROWN][12] = smallmap->copy(64, 66, 22, 22);
	map_sq[MAP_DBROWN][7] = smallmap->copy(96, 66, 22, 22);
	map_sq[MAP_DBROWN][13] = smallmap->copy(128, 66, 22, 22);
	map_sq[MAP_DBROWN][3] = smallmap->copy(160, 66, 22, 22);
	map_sq[MAP_DBROWN][11] = smallmap->copy(192, 66, 22, 22);
	map_sq[MAP_DBROWN][9] = smallmap->copy(224, 66, 22, 22);
	map_sq[MAP_DBROWN][4] = smallmap->copy(256, 66, 22, 22);
	map_sq[MAP_DBROWN][5] = smallmap->copy(288, 66, 22, 22);
	map_sq[MAP_DBROWN][1] = smallmap->copy(0, 88, 22, 22);
	map_sq[MAP_DBROWN][2] = smallmap->copy(32, 88, 22, 22);
	map_sq[MAP_DBROWN][10] = smallmap->copy(64, 88, 22, 22);

	map_sq[MAP_WHITE][1] = smallmap->copy(224, 44, 22, 22);
	map_sq[MAP_WHITE][2] = smallmap->copy(256, 44, 22, 22);
	map_sq[MAP_WHITE][3] = smallmap->copy(96, 44, 22, 22);
	map_sq[MAP_WHITE][4] = smallmap->copy(192, 44, 22, 22);
	map_sq[MAP_WHITE][6] = smallmap->copy(288, 22, 22, 22);
	map_sq[MAP_WHITE][8] = smallmap->copy(0, 66, 22, 22);
	map_sq[MAP_WHITE][9] = smallmap->copy(160, 44, 22, 22);
	map_sq[MAP_WHITE][10] = smallmap->copy(288, 44, 22, 22);
	map_sq[MAP_WHITE][11] = smallmap->copy(128, 44, 22, 22);
	map_sq[MAP_WHITE][12] = smallmap->copy(32, 44, 22, 22);
	map_sq[MAP_WHITE][14] = smallmap->copy(0, 44, 22, 22);
	map_sq[MAP_WHITE][15] = smallmap->copy(64, 44, 22, 22);

	map_sq[MAP_DGREEN][6] = smallmap->copy(96, 88, 22, 22);
	map_sq[MAP_DGREEN][14] = smallmap->copy(128, 88, 22, 22);
	map_sq[MAP_DGREEN][12] = smallmap->copy(160, 88, 22, 22);
	map_sq[MAP_DGREEN][7] = smallmap->copy(192, 88, 22, 22);
	map_sq[MAP_DGREEN][15] = smallmap->copy(224, 88, 22, 22);
	map_sq[MAP_DGREEN][13] = smallmap->copy(256, 88, 22, 22);
	map_sq[MAP_DGREEN][3] = smallmap->copy(288, 88, 22, 22);
	map_sq[MAP_DGREEN][11] = smallmap->copy(0, 110, 22, 22);
	map_sq[MAP_DGREEN][9] = smallmap->copy(32, 110, 22, 22);

	map_sq[MAP_GREY][0] = smallmap->copy(0, 132, 22, 22);
	map_sq[MAP_GREY][1] = smallmap->copy(192, 110, 22, 22);
	map_sq[MAP_GREY][2] = smallmap->copy(224, 110, 22, 22);
	map_sq[MAP_GREY][4] = smallmap->copy(128, 110, 22, 22);
	map_sq[MAP_GREY][5] = smallmap->copy(160, 110, 22, 22);
	map_sq[MAP_GREY][8] = smallmap->copy(288, 110, 22, 22);
	map_sq[MAP_GREY][10] = smallmap->copy(256, 110, 22, 22);
	map_sq[MAP_GREY][12] = smallmap->copy(64, 110, 22, 22);
	map_sq[MAP_GREY][15] = smallmap->copy(96, 110, 22, 22);

	delete smallmap;
	smallmap = NULL;
	drawProgress(60);

	for(int i=0;i<n_players_c;i++) {
		for(int j=0;j<=n_epochs_c;j++) {
			for(int k=0;k<n_attacker_directions_c;k++) {
				for(int l=0;l<max_attacker_frames_c;l++) {
					attackers_walking[i][j][k][l] = NULL;
				}
			}
		}
	}
	for(int j=0;j<=n_epochs_c;j++) {
		for(int k=0;k<n_attacker_directions_c;k++) {
			n_attacker_frames[j][k] = 3;
		}
	}
	for(int i=0;i<n_epochs_c;i++)
		for(int j=0;j<N_ATTACKER_AMMO_DIRS;j++)
			attackers_ammo[i][j] = NULL;

	Image *armies = Image::loadImage("data/mlm_armies");
	if( armies == NULL ) {
		return false;
	}
	armies->setColor(0, 255, 0, 255);
	processImage(armies, false); // don't smooth, as it messes up the colour remapping!

	const int n_defender_frames_c = 8;
	ASSERT( n_defender_frames_c <= max_defender_frames_c );
	for(int i=0;i<n_epochs_c;i++) {
		n_defender_frames[i] = n_defender_frames_c;
	}
	for(int i=0;i<=5;i++) {
		for(int j=0;j<n_defender_frames_c;j++) {
			for(int k=0;k<n_players_c;k++)
				defenders[k][i][j] = armies->copy(16*j, 16 + 32*i, 16, 16);
		}
	}
	for(int j=0;j<n_defender_frames_c;j++) {
		for(int k=0;k<n_players_c;k++)
			defenders[k][6][j] = armies->copy(128 + 16*j, 192, 16, 16);
	}
	for(int j=0;j<2;j++) {
		for(int k=0;k<n_players_c;k++)
			defenders[k][7][j] = armies->copy(224 + 16*j, 240, 16, 16);
	}
	for(int j=2;j<4;j++) {
		for(int k=0;k<n_players_c;k++)
			defenders[k][7][j] = armies->copy(288 + 16*(j-2), 224, 16, 16);
	}
	for(int j=4;j<6;j++) {
		for(int k=0;k<n_players_c;k++)
			defenders[k][7][j] = armies->copy(256 + 16*(j-4), 240, 16, 16);
	}
	for(int j=6;j<8;j++) {
		for(int k=0;k<n_players_c;k++)
			defenders[k][7][j] = armies->copy(288 + 16*(j-6), 240, 16, 16);
	}
	for(int j=0;j<n_defender_frames_c;j++) {
		defenders[0][8][j] = armies->copy(192, 256, 16, 16);
		defenders[1][8][j] = armies->copy(192, 272, 16, 16);
		defenders[2][8][j] = armies->copy(208, 256, 16, 16);
		defenders[3][8][j] = armies->copy(208, 272, 16, 16);
	}
	for(int k=0;k<n_players_c;k++) {
		int kx = k / 2;
		int ky = k % 2;
		for(int j=0;j<n_defender_frames_c;j++) {
			int j2 = j % 4;
			if( j2 == 0 )
				j2 = 1;
			else if( j2 == 1 )
				j2 = 0;
			defenders[k][9][j] = armies->copy(192 + kx * 64 + j2 * 16, 288 + ky * 13, 16, 13);
		}
	}

	for(int i=0;i<n_players_c;i++) {
		for(int j=0;j<=5;j++) {
			for(int k=0;k<n_attacker_directions_c;k++) {
				int n_frames = n_attacker_frames[j][k];
				for(int l=0;l<n_frames;l++) {
					attackers_walking[i][j][k][l] = armies->copy(16*l + 64*k, 32*j, 16, 16);
					int r = 0, g = 0, b = 0;
					PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)i);
					attackers_walking[i][j][k][l]->remap(240, 0, 0, r, g, b);
				}
			}
		}
	}

	for(int i=0;i<n_players_c;i++) {
		for(int k=0;k<n_attacker_directions_c;k++) {
			int n_frames = n_attacker_frames[10][k];
			for(int l=0;l<n_frames;l++) {
					attackers_walking[i][10][k][l] = armies->copy(16*l + 64*k, 320, 16, 16);
					int r = 0, g = 0, b = 0;
					PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)i);
					attackers_walking[i][10][k][l]->remap(240, 0, 0, r, g, b);
			}
		}
	}

	for(int i=0;i<n_players_c;i++) {
		for(int j=0;j<n_epochs_c;j++)
			planes[i][j] = NULL;
		planes[i][6] = armies->copy(32*i, 192, 32, 32);
		planes[i][7] = armies->copy(32*i, 232, 32, 24);
	}

	for(int i=0;i<n_players_c;i++) {
		nukes[i][0] = armies->copy(48*i, 256, 16, 32);
		nukes[i][1] = armies->copy(48*i+16, 256, 32, 32);
	}
	for(int i=0;i<n_players_c;i++) {
		for(int j=0;j<n_saucer_frames_c;j++) {
			saucers[i][j] = armies->copy(32*j, 288, 32, 21);
		}
	}

	// ammo:
	// rock
	attackers_ammo[0][ATTACKER_AMMO_RIGHT] = armies->copy(272, 24, 16, 8);
	attackers_ammo[0][ATTACKER_AMMO_LEFT] = armies->copy(288, 24, 16, 8);
	attackers_ammo[0][ATTACKER_AMMO_UP] = armies->copy(288, 16, 16, 8);
	attackers_ammo[0][ATTACKER_AMMO_DOWN] = armies->copy(272, 16, 16, 8);
	// catapult/sword
	attackers_ammo[1][ATTACKER_AMMO_RIGHT] = armies->copy(272, 24, 16, 8);
	attackers_ammo[1][ATTACKER_AMMO_LEFT] = armies->copy(288, 24, 16, 8);
	attackers_ammo[1][ATTACKER_AMMO_UP] = armies->copy(288, 16, 16, 8);
	attackers_ammo[1][ATTACKER_AMMO_DOWN] = armies->copy(272, 16, 16, 8);
	/* spear
	attackers_ammo[1][ATTACKER_AMMO_RIGHT] = armies->copy(272, 32, 16, 8);
	attackers_ammo[1][ATTACKER_AMMO_LEFT] = armies->copy(272, 40, 16, 8);
	attackers_ammo[1][ATTACKER_AMMO_UP] = armies->copy(288, 32, 16, 16);
	attackers_ammo[1][ATTACKER_AMMO_DOWN] = armies->copy(304, 32, 16, 16);*/
	// pike
	attackers_ammo[2][ATTACKER_AMMO_RIGHT] = armies->copy(272, 64, 16, 8);
	attackers_ammo[2][ATTACKER_AMMO_LEFT] = armies->copy(272, 72, 16, 8);
	attackers_ammo[2][ATTACKER_AMMO_UP] = armies->copy(288, 64, 16, 16);
	attackers_ammo[2][ATTACKER_AMMO_DOWN] = armies->copy(304, 64, 16, 16);
	/* bow and arrow
	attackers_ammo[2][ATTACKER_AMMO_RIGHT] = armies->copy(272, 80, 16, 8);
	attackers_ammo[2][ATTACKER_AMMO_LEFT] = armies->copy(272, 88, 16, 8);
	attackers_ammo[2][ATTACKER_AMMO_UP] = armies->copy(288, 80, 16, 16);
	attackers_ammo[2][ATTACKER_AMMO_DOWN] = armies->copy(304, 80, 16, 16);*/
	// longbow
	attackers_ammo[3][ATTACKER_AMMO_RIGHT] = armies->copy(272, 96, 16, 8);
	attackers_ammo[3][ATTACKER_AMMO_LEFT] = armies->copy(272, 104, 16, 8);
	attackers_ammo[3][ATTACKER_AMMO_UP] = armies->copy(288, 96, 16, 16);
	attackers_ammo[3][ATTACKER_AMMO_DOWN] = armies->copy(304, 96, 16, 16);
	// trebuchet
	attackers_ammo[4][ATTACKER_AMMO_RIGHT] = armies->copy(256, 144, 16, 8);
	attackers_ammo[4][ATTACKER_AMMO_LEFT] = armies->copy(256, 144, 16, 8);
	attackers_ammo[4][ATTACKER_AMMO_UP] = armies->copy(256, 144, 16, 16);
	attackers_ammo[4][ATTACKER_AMMO_DOWN] = armies->copy(256, 144, 16, 16);
	// cannon
	attackers_ammo[5][ATTACKER_AMMO_RIGHT] = armies->copy(272, 160, 10, 9);
	attackers_ammo[5][ATTACKER_AMMO_LEFT] = armies->copy(272, 160, 10, 9);
	attackers_ammo[5][ATTACKER_AMMO_UP] = armies->copy(272, 160, 10, 9);
	attackers_ammo[5][ATTACKER_AMMO_DOWN] = armies->copy(272, 160, 10, 9);
	// bombs
	//attackers_ammo[6][ATTACKER_AMMO_BOMB] = armies->copy(304, 208, 16, 16);
	attackers_ammo[6][ATTACKER_AMMO_BOMB] = armies->copy(288, 206, 12, 12);

	delete armies;
	armies = NULL;

	for(int k=0;k<n_players_c;k++) {
		nuke_defences[k] = defenders[k][nuclear_epoch_c][0];
	}

	attackers_ammo[7][ATTACKER_AMMO_BOMB] = attackers_ammo[6][ATTACKER_AMMO_BOMB];
	attackers_ammo[9][ATTACKER_AMMO_BOMB] = attackers_ammo[6][ATTACKER_AMMO_BOMB];

    for(int i=0;i<n_saucer_frames_c;i++) {
        for(int k=0;k<n_players_c;k++) {
            int r = 0, g = 0, b = 0;
            PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)k);
            saucers[k][i]->remap(240, 0, 0, r, g, b);
        }
    }
    for(int i=0;i<n_epochs_c;i++) {
        if( defenders[0][i][0] == NULL )
            continue;
        for(int j=0;j<n_defender_frames_c;j++) {
            for(int k=0;k<n_players_c;k++) {
                int r = 0, g = 0, b = 0;
                PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)k);
                defenders[k][i][j]->remap(240, 0, 0, r, g, b);
            }
        }
    }
	drawProgress(65);

	Image *features = Image::loadImage("data/mlm_features");
	if( features == NULL )
		return false;
	features->setColor(0, 255, 0, 255);
	processImage(features);

	icon_openpitmine = features->copy(256, 118, 47, 24);

	for(int i=0;i<4;i++) {
		icon_trees[i][0] = features->copy(96 + 32*i, 114, 24, 28);
		for(int j=1;j<n_tree_frames_c;j++) {
			icon_trees[i][j] = icon_trees[i][0]; // no animation for old data available
		}
	}

	delete features;
	features = NULL;

	drawProgress(70);

	background_islands = Image::loadImage("data/mlm_sunrise");
	if( background_islands == NULL )
		return false;
	processImage(background_islands);

	drawProgress(80);

	return true;
}

bool Game::loadImages() {
    //int time_s = clock();
	// progress should go from 0 to 80%
#ifdef WINRT
	string gfx_dir = "Assets/gfx/";
#else
	string gfx_dir = "gfx/";
#endif

	background = Image::loadImage(gfx_dir + "starfield.jpg");
#if !defined(__ANDROID__) && defined(__linux)
	if( background == NULL ) {
		gfx_dir = "/usr/share/gigalomania/" + gfx_dir;
		LOG("look in %s for gfx\n", gfx_dir.c_str());
		background = Image::loadImage(gfx_dir + "starfield.jpg");
	}
#endif
	if( background == NULL ) {
		//return false;
		return loadOldImages();
	}
	drawProgress(20);
	//scale_factor = ((float)(scale_width*default_width_c))/(float)player_select->getWidth();
	//LOG("scale factor for images = %f\n", scale_factor);

	calculateScale(background);
	// nb, still scale if scale_factor==1, as this is a way of converting to 8bit
	processImage(background);
	drawProgress(25);

	Image *image_slabs = NULL;
	image_slabs = Image::loadImage(gfx_dir + "slabs.png");
	if( image_slabs == NULL )
		return false;
	drawProgress(30);
	for(int i=0;i<MAP_N_COLOURS;i++) {
		land[i] = image_slabs->copy();
		processImage(land[i]);
		//land[i]->setMaskColor(255, 0, 255); // need to set the mask colour now, to stop it being multiplied!
	}
	drawProgress(32);
	land[MAP_ORANGE]->brighten(187.5f/255.0f, 96.0f/255.0f, 42.0f/255.0f);
	land[MAP_GREEN]->brighten(52.0f/255.0f, 163.5f/255.0f, 52.0f/255.0f);
	land[MAP_BROWN]->brighten(116.0f/255.0f, 72.0f/255.0f, 36.0f/255.0f);
	land[MAP_WHITE]->brighten(163.5f/255.0f, 163.5f/255.0f, 163.5f/255.0f);
	land[MAP_DBROWN]->brighten(120.0f/255.0f, 76.0f/255.0f, 58.0f/255.0f);
	land[MAP_DGREEN]->brighten(26/255.0f, 120.0f/255.0f, 26.0f/255.0f);
	land[MAP_GREY]->brighten(94.0f/255.0f, 94.0f/255.0f, 94.0f/255.0f);
	delete image_slabs;
	image_slabs = NULL;

	Image *player_heads_select_all = Image::loadImage(gfx_dir + "player_heads_select.png");
	if( player_heads_select_all == NULL )
		return false;
	processImage(player_heads_select_all);
	for(int i=0;i<n_players_c;i++) {
		player_heads_select[i] = player_heads_select_all->copy(32*i, 0, 32, 25);
	}
	delete player_heads_select_all;

	Image *player_heads_alliance_all = Image::loadImage(gfx_dir + "player_heads_alliance.png");
	processImage(player_heads_alliance_all);
	if( player_heads_alliance_all == NULL )
		return false;
	for(int i=0;i<n_players_c;i++) {
		player_heads_alliance[i] = player_heads_alliance_all->copy(32*i, 0, 32, 41);
	}
	delete player_heads_alliance_all;

	grave = Image::loadImage(gfx_dir + "grave1.png");
	if( grave == NULL )
		return false;
	processImage(grave);

	/*Image *buildings = Image::loadImage(gfx_dir + "buildings.png");
	if( buildings != NULL ) {
		buildings_shadow = false; // done using alpha channel
		buildings->scale(scale_factor, scale_factor);
		buildings->setScale(scale_width, scale_height);
		for(int i=0;i<n_epochs_c;i++) {
			fortress[i] = buildings->copy( 0, 60*i, 58, 60);
			mine[i] = buildings->copy( 58, 60*i, 58, 60);
			factory[i] = buildings->copy( 174, 60*i, 58, 60);
			lab[i] = buildings->copy( 116, 60*i, 58, 60);
		}
	}*/
	for(int i=0;i<n_epochs_c;i++) {
		fortress[i] = NULL;
		mine[i] = NULL;
		factory[i] = NULL;
	}
	for(int i=0;i<n_epochs_c;i++) {
		stringstream filename;
		filename << gfx_dir << "building_tower_" << i << ".png";
		Image *temp = Image::loadImage(filename.str().c_str());
		if( temp == NULL ) {
			return false;
		}
		processImage(temp);
		//delete fortress[i];
		//fortress[i] = temp->copy(29, 9, 62, 51);
		fortress[i] = temp->copy(27, 9, 64, 51);
		delete temp;
	}
	drawProgress(34);
	for(int i=mine_epoch_c;i<n_epochs_c-1;i++) {
		stringstream filename;
		filename << gfx_dir << "building_mine_" << i << ".png";
		Image *temp = Image::loadImage(filename.str().c_str());
		if( temp == NULL ) {
			return false;
		}
		processImage(temp);
		mine[i] = temp->copy(28, 12, 66, 51);
		delete temp;
	}
	drawProgress(36);
	for(int i=factory_epoch_c;i<n_epochs_c-1;i++) {
		stringstream filename;
		filename << gfx_dir << "building_factory_" << i << ".png";
		Image *temp = Image::loadImage(filename.str().c_str());
		if( temp == NULL ) {
			return false;
		}
		processImage(temp);
		//factory[i] = temp->copy(24, 1, 70, 62);
		factory[i] = temp->copy(25, 1, 68, 62);
		delete temp;
	}
	drawProgress(38);
	for(int i=lab_epoch_c;i<n_epochs_c-1;i++) {
		stringstream filename;
		filename << gfx_dir << "building_lab_" << i << ".png";
		Image *temp = Image::loadImage(filename.str().c_str());
		if( temp == NULL ) {
			return false;
		}
		processImage(temp);
		//lab[i] = temp->copy(28, 12, 66, 51);
		lab[i] = temp->copy(31, 12, 52, 51);
		delete temp;
	}

	drawProgress(40);

	Image *icons = Image::loadImage(gfx_dir + "icons.png");
	if( icons == NULL )
		return false;
	/*if( !icons->scaleTo(scale_width*default_width_c) )
	return false;*/

    // need to do flags beforehand, due to colour remapping
    icons->setScale(scale_width/scale_factor_w, scale_height/scale_factor_h); // so the copying will work at the right scale for the input image
    for(int i=0;i<n_players_c;i++) { // different locations
        /*for(int j=0;j<3;j++)
            flags[i][j] = icons->copy(160 + 16*j, 144, 16, 16);
        flags[i][3] = icons->copy(160 + 16*1, 144, 16, 16);*/
        for(int j=0;j<4;j++)
            flags[i][j] = icons->copy(144 + 16*j, 144, 16, 16);
        for(int j=0;j<n_flag_frames_c;j++) {
            int r = 0, g = 0, b = 0;
            PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)i);
            flags[i][j]->remap(240, 0, 0, r, g, b);
            processImage(flags[i][j]);
        }
    }

    processImage(icons);

	for(int i=0;i<n_epochs_c;i++)
		men[i] = icons->copy(16*i, 0, 16, 16);

	unarmed_man = icons->copy(80, 32, 16, 16);

	panel_design = icons->copy(304, 0, 16, 16);
	//panel_design_dark = panel_design->copy();
	panel_lab = icons->copy(16, 33, 16, 16);
	panel_shield = icons->copy(240, 0, 16, 16);
	panel_defence = icons->copy(256, 0, 16, 16);
	panel_attack = icons->copy(272, 0, 16, 16);
	panel_knowndesigns = icons->copy(240, 16, 16, 16);
	panel_factory = icons->copy(48, 32, 16, 16);
	panel_bloody_attack = icons->copy(240, 32, 16, 16);

	mine_gatherable_small = icons->copy(160, 0, 16, 16);

	//panel_build[BUILDING_TOWER] = icons->copy(224, 63, 16, 16); // different size // not yet used
	//panel_build[BUILDING_MINE] = icons->copy(256, 63, 16, 16); // different size
	panel_build[BUILDING_MINE] = mine_gatherable_small;
	panel_build[BUILDING_FACTORY] = icons->copy(288, 64, 16, 16); // different size
	panel_build[BUILDING_LAB] = icons->copy(192, 64, 16, 16); // different size
	//panel_build[BUILDING_LAB] = panel_lab;

	panel_building[BUILDING_TOWER] = icons->copy(0, 33, 16, 16); // different size
	//panel_building[BUILDING_MINE] = icons->copy(32, 33, 16, 16); // different size
	panel_building[BUILDING_MINE] = mine_gatherable_small;
	panel_building[BUILDING_FACTORY] = panel_factory;
	panel_building[BUILDING_LAB] = panel_lab;

	mine_gatherable_large = mine_gatherable_small;
	panel_bigdesign = panel_design;
	panel_biglab = panel_lab;
	panel_bigfactory = panel_factory;
	panel_bigshield = panel_shield;
	panel_bigdefence = panel_defence;
	panel_bigattack = panel_attack;
	panel_bigbuild = icons->copy(48, 48, 16, 16);
	panel_bigknowndesigns = panel_knowndesigns;
	panel_twoattack = panel_attack;

	for(int i=0;i<n_players_c;i++)
		mouse_pointers[i] = icons->copy(176 + 16*i, 0, 16, 16);

	for(int i=0;i<13;i++)
		icon_clocks[i] = icons->copy(16*i, 16, 16, 16);

	/*for(int i=0;i<3;i++)
		icon_mice[i] = icons->copy(256 + 16*i, 16, 16, 16); // smaller size*/
	for(int i=0;i<2;i++)
		icon_mice[i] = icons->copy(256 + 16*i, 16, 16, 16); // smaller size

	for(int i=0;i<n_death_flashes_c;i++)
		death_flashes[i] = icons->copy(256 + 16*i, 32, 16, 16); // different location and smaller size

	for(int i=0;i<7;i++)
		blue_flashes[i] = icons->copy(208 + 16*i, 144, 16, 16); // different location (and size for i = 5, 6)

	for(int i=0;i<n_players_c;i++) {
		icon_towers[i] = icons->copy(160 + 16*i, 81, 6, 6);
		icon_armies[i] = icons->copy(160 + 16*i, 87, 4, 4);
	}

	for(int i=0;i<N_ID;i++)
		icon_elements[i] = icons->copy(16*i, 128, 16, 16); // different location

    /*for(int i=0;i<n_players_c;i++) { // different locations
		for(int j=0;j<3;j++)
			flags[i][j] = icons->copy(160 + 16*j, 144, 16, 16);
		flags[i][3] = flags[i][1];
		for(int j=0;j<n_flag_frames_c;j++) {
			int r = 0, g = 0, b = 0;
			PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)i);
			flags[i][j]->remap(240, 0, 0, r, g, b);
		}
    }*/

	for(int i=0;i<n_playershields_c;i++) {
		//playershields[i] = icons->copy(16*i, 176, 16, 14); // different location // original version for my gfx
		playershields[i] = icons->copy(16*i, 176, 16, 16); // different location
		//playershields[i] = icons->copy(16*i, 177, 16, 14); // different location
	}

	for(int i=0;i<n_shields_c;i++)
		icon_shields[i] = icons->copy(112 + 16*i, 32, 16, 16);
	for(int i=0;i<n_epochs_c;i++)
		icon_defences[i] = icons->copy(16*i, 240, 16, 16); // different location
	for(int i=0;i<n_epochs_c;i++)
		icon_weapons[i] = icons->copy(16*i, 224, 16, 16); // different location
	for(int i=0;i<n_epochs_c;i++)
		//numbered_defences[i] = icons->copy(16*i, 173, 16, 16);
		numbered_defences[i] = icon_defences[i]; // todo:
	for(int i=0;i<n_epochs_c;i++)
		//numbered_weapons[i] = icons->copy(16*i, 157, 16, 16);
		numbered_weapons[i] = icon_weapons[i]; // todo:

	for(int i=0;i<3;i++)
		icon_speeds[i] = icons->copy(272 + 16*i, 192, 16, 16); // different location and size

	building_health = icons->copy(0, 192, 38, 16);

	icon_shield = icons->copy(0, 64, 16, 16);
	icon_defence = icons->copy(16, 64, 16, 16);
	icon_weapon = icons->copy(32, 64, 16, 16);

	/*icon_infinity = icons->copy(128, 112, 12, 8); // original versions for my gfx
	icon_bc = icons->copy(224, 15, 12, 8);
	icon_ad = icons->copy(208, 16, 12, 8);
	icon_ad_shiny = icons->copy(208, 16, 12, 8);*/
	icon_infinity = icons->copy(128, 112, 16, 16);
	/*icon_bc = icons->copy(224, 15, 16, 16);
	icon_ad = icons->copy(208, 16, 16, 16);
	icon_ad_shiny = icons->copy(208, 16, 16, 16);*/

	numbers_half = icons->copy(112, 112, 7, 4);

	icon_nuke_hole = icons->copy(288, 96, 16, 16);

	icon_ergo = icons->copy(176, 112, 16, 16);
	icon_trash = icons->copy(192, 112, 16, 16);

	icons = Image::loadImage(gfx_dir + "explosions_test4.png");
	if( icons == NULL )
		return false;
	drawProgress(42);
	processImage(icons);
	for(int i=0;i<n_explosions_c;i++) {
		int x = (i % 10);
		int y = i/10;
		int w = 25,  h = 25;
		explosions[i] = icons->copy(x*w, y*h, w, h);
	}

	icons = Image::loadImage(gfx_dir + "icons64.png");
	if( icons == NULL )
		return false;
	drawProgress(45);
	// replace with new large icons
	/*if( !icons->scaleTo(scale_width*128) ) // may need to update width as more icons added!
	return false;*/
	processImage(icons);

	mapsquare = icons->copy(0, 0, 17, 17);
	flashingmapsquare = icons->copy(32, 0, 17, 17);
	arrow_left = icons->copy(64, 0, 32, 32);
	arrow_left->scaleAlpha(0.625f);
	arrow_right = icons->copy(96, 0, 32, 32);
	arrow_right->scaleAlpha(0.625f);

	icons = Image::loadImage(gfx_dir + "font.png");
	if( icons == NULL )
		return false;
	drawProgress(48);
	//processImage(icons);
	//const int font_w = 6;
	//const int font_h = 10;
	//const int font_w = 8;
	//const int font_h = 16;
	int font_w = 12;
	int font_h = 16;
    for(int i=0;i<10;i++) {
		//int xpos = 8*i;
		//int ypos = 0;
		//int xpos = 136+9*i;
		//int ypos = 1;
		int xpos = 196+13*i;
		int ypos = 1;
        //int xpos = 512+32*i+4;
		//int ypos = 0;
		numbers_blue[i] = icons->copy(xpos, ypos, font_w, font_h);
        numbers_grey[i] = icons->copy(xpos, ypos, font_w, font_h);
        numbers_white[i] = icons->copy(xpos, ypos, font_w, font_h);
        numbers_orange[i] = icons->copy(xpos, ypos, font_w, font_h);
        numbers_yellow[i] = icons->copy(xpos, ypos, font_w, font_h);
		for(int j=0;j<4;j++) {
            numbers_small[j][i] = icons->copy(xpos, ypos, font_w, font_h);
		}
		numbers_small[0][i]->brighten(1.0f, 0.0f, 0.0f);
		numbers_small[1][i]->brighten(0.0f, 1.0f, 0.0f);
		numbers_small[2][i]->brighten(1.0f, 1.0f, 0.0f);
		numbers_small[3][i]->brighten(0.0f, 0.0f, 1.0f);

		processImage(numbers_blue[i]);
		processImage(numbers_grey[i]);
		processImage(numbers_white[i]);
		processImage(numbers_orange[i]);
		processImage(numbers_yellow[i]);
		for(int j=0;j<4;j++) {
			processImage(numbers_small[j][i]);
		}
	}
	for(int i=0;i<n_font_chars_c;i++) {
		letters_small[i] = NULL;
	}
	for(int i=0;i<26;i++) {
		//int xpos = 80+8*i;
		//int ypos = 16;
		//int xpos = 289+9*i;
		//int ypos = 1;
		int xpos = 417+13*i;
		int ypos = 1;
        //int xpos = 1056+32*i+4;
		//int ypos = 0;
        letters_small[i] = icons->copy(xpos, ypos, font_w, font_h);
	}
    //numbers_half = icons->copy(288, 16, font_w, font_h);
	/*letters_small[font_index_period_c] = icons->copy(296, 16, font_w, font_h);
	letters_small[font_index_apostrophe_c] = icons->copy(304, 16, font_w, font_h);
	letters_small[font_index_exclamation_c] = icons->copy(312, 16, font_w, font_h);
	letters_small[font_index_question_c] = icons->copy(320, 16, font_w, font_h);*/
	letters_small[font_index_period_c] = icons->copy(170, 1, font_w, font_h);
	letters_small[font_index_apostrophe_c] = icons->copy(66, 1, font_w, font_h);
	letters_small[font_index_exclamation_c] = icons->copy(1, 1, font_w, font_h);
	letters_small[font_index_question_c] = icons->copy(391, 1, font_w, font_h);
	letters_small[font_index_comma_c] = icons->copy(144, 1, font_w, font_h);
	letters_small[font_index_dash_c] = icons->copy(157, 1, font_w, font_h);
	for(int i=0;i<n_font_chars_c;i++) {
		if( letters_small[i] != NULL )
			processImage(letters_small[i]);
	}
	dash_grey = letters_small[font_index_dash_c];

	delete icons;
	drawProgress(50);

	icons = Image::loadImage(gfx_dir + "font_large.png");
	if( icons == NULL )
		return false;
	drawProgress(48);
	font_w = 24;
	font_h = 32;
    for(int i=0;i<10;i++) {
		int xpos = 196+13*i;
		int ypos = 1;
		xpos *= 2;
		ypos *= 2;
        numbers_largegrey[i] = icons->copy(xpos, ypos, font_w, font_h);
        numbers_largeshiny[i] = icons->copy(xpos, ypos, font_w, font_h);

		processImage(numbers_largegrey[i]);
		processImage(numbers_largeshiny[i]);
	}
	for(int i=0;i<n_font_chars_c;i++) {
		letters_large[i] = NULL;
	}
	for(int i=0;i<26;i++) {
		int xpos = 417+13*i;
		int ypos = 1;
		xpos *= 2;
		ypos *= 2;
        letters_large[i] = icons->copy(xpos, ypos, font_w, font_h);
	}
	letters_large[font_index_period_c] = icons->copy(170*2, 1*2, font_w, font_h);
	letters_large[font_index_apostrophe_c] = icons->copy(66*2, 1*2, font_w, font_h);
	letters_large[font_index_exclamation_c] = icons->copy(1*2, 1*2, font_w, font_h);
	letters_large[font_index_question_c] = icons->copy(391*2, 1*2, font_w, font_h);
	letters_large[font_index_comma_c] = icons->copy(144*2, 1*2, font_w, font_h);
	letters_large[font_index_dash_c] = icons->copy(157*2, 1*2, font_w, font_h);
	for(int i=0;i<n_font_chars_c;i++) {
		if( letters_large[i] != NULL )
			processImage(letters_large[i]);
	}

    smoke_image = Image::createRadial((int)(scale_width * 16), (int)(scale_height * 16), 0.5f);
	processImage(smoke_image);

	for(int i=0;i<n_coast_c;i++)
		coast_icons[i] = NULL;
	map_sq_offset = 0;
    int map_width = (int)(scale_width * 16);
    int map_height = (int)(scale_height * 16);
    {
		unsigned char filter_max[3] = {255, 192, 84};
		unsigned char filter_min[3] = {120, 0, 0};
        map_sq[MAP_ORANGE][0] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max, filter_min, Image::NOISEMODE_PERLIN, 4);
	}
	{
		unsigned char filter_max[3] = {104, 255, 104};
		unsigned char filter_min[3] = {0, 72, 0};
        map_sq[MAP_GREEN][0] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max, filter_min, Image::NOISEMODE_PERLIN, 4);
	}
	{
		unsigned char filter_max[3] = {216, 144, 72};
		unsigned char filter_min[3] = {16, 0, 0};
        map_sq[MAP_BROWN][0] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max, filter_min, Image::NOISEMODE_PERLIN, 4);
	}
	{
		unsigned char filter_max[3] = {255, 255, 255};
		unsigned char filter_min[3] = {72, 72, 72};
        map_sq[MAP_WHITE][0] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max, filter_min, Image::NOISEMODE_PERLIN, 4);
	}
	{
		unsigned char filter_max[3] = {224, 152, 116};
		unsigned char filter_min[3] = {16, 0, 0};
        map_sq[MAP_DBROWN][0] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max, filter_min, Image::NOISEMODE_PERLIN, 4);
	}
	{
		unsigned char filter_max[3] = {52, 232, 52};
		unsigned char filter_min[3] = {0, 8, 0};
        map_sq[MAP_DGREEN][0] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max, filter_min, Image::NOISEMODE_PERLIN, 4);
	}
	{
		unsigned char filter_max[3] = {188, 188, 188};
		unsigned char filter_min[3] = {0, 0, 0};
        map_sq[MAP_GREY][0] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max, filter_min, Image::NOISEMODE_PERLIN, 4);
	}
	for(int i=0;i<MAP_N_COLOURS;i++) {
        map_sq[i][0]->setScale(scale_width, scale_height);
        for(int j=1;j<n_map_sq_c;j++) {
			map_sq[i][j] = map_sq[i][0]->copy(0, 0, 16, 16);
		}
    }
	drawProgress(53);

	map_sq_coast_offset = 0;
	unsigned char filter_max_ocean[3] = {180, 215, 240};
	unsigned char filter_min_ocean[3] = {60, 95, 115};
	coast_icons[0] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max_ocean, filter_min_ocean, Image::NOISEMODE_PERLIN, 4);
	coast_icons[0]->fadeAlpha(false, false);
	coast_icons[1] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max_ocean, filter_min_ocean, Image::NOISEMODE_PERLIN, 4);
	coast_icons[1]->fadeAlpha(true, true);
	coast_icons[2] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max_ocean, filter_min_ocean, Image::NOISEMODE_PERLIN, 4);
	coast_icons[2]->fadeAlpha(false, true);
	coast_icons[3] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max_ocean, filter_min_ocean, Image::NOISEMODE_PERLIN, 4);
	coast_icons[3]->fadeAlpha(true, false);

	coast_icons[4] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max_ocean, filter_min_ocean, Image::NOISEMODE_PERLIN, 4);
	coast_icons[4]->fadeAlpha(true, false);
	coast_icons[4]->fadeAlpha(false, false);
	coast_icons[5] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max_ocean, filter_min_ocean, Image::NOISEMODE_PERLIN, 4);
	coast_icons[5]->fadeAlpha(true, true);
	coast_icons[5]->fadeAlpha(false, false);
	coast_icons[6] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max_ocean, filter_min_ocean, Image::NOISEMODE_PERLIN, 4);
	coast_icons[6]->fadeAlpha(true, false);
	coast_icons[6]->fadeAlpha(false, true);
	coast_icons[7] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max_ocean, filter_min_ocean, Image::NOISEMODE_PERLIN, 4);
	coast_icons[7]->fadeAlpha(true, true);
	coast_icons[7]->fadeAlpha(false, true);

	for(int i=0;i<8;i++) {
        coast_icons[i]->setScale(scale_width, scale_height);
	}

	drawProgress(55);

	{
		// initialise
		for(int i=0;i<n_players_c;i++) {
			for(int j=0;j<=n_epochs_c;j++) {
				for(int k=0;k<n_attacker_directions_c;k++) {
					for(int l=0;l<max_attacker_frames_c;l++) {
						attackers_walking[i][j][k][l] = NULL;
					}
				}
			}
		}
		for(int j=0;j<=n_epochs_c;j++) {
			for(int k=0;k<n_attacker_directions_c;k++) {
				n_attacker_frames[j][k] = 0;
			}
		}
		for(int i=0;i<n_epochs_c;i++)
			for(int j=0;j<N_ATTACKER_AMMO_DIRS;j++)
				attackers_ammo[i][j] = NULL;

		Image *gfx_def_image = Image::loadImage(gfx_dir + "defenders.png");
		if( gfx_def_image == NULL )
			return false;
		drawProgress(58);
        gfx_def_image->setScale(scale_width/scale_factor_w, scale_height/scale_factor_h); // so the copying will work at the right scale for the input image
        for(int k=0;k<n_players_c;k++) {
            int r = 0, g = 0, b = 0;
            PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)k);
            for(int i=0;i<9;i++) {
				n_defender_frames[i] = 8;
				ASSERT( n_defender_frames[i] <= max_defender_frames_c );
				for(int j=0;j<n_defender_frames[i];j++) {
					defenders[k][i][j] = gfx_def_image->copy(16*i, 0, 16, 16);
                    defenders[k][i][j]->remap(240, 0, 0, r, g, b);
                    processImage(defenders[k][i][j]);
					if( i == 8 ) {
						defenders[k][i][j]->setOffset(-1, 0);
					}
                }
			}
		}
		delete gfx_def_image;

		gfx_def_image = Image::loadImage(gfx_dir + "defender_9.png");
		if( gfx_def_image == NULL )
			return false;
        gfx_def_image->setScale(scale_width/scale_factor_w, scale_height/scale_factor_h); // so the copying will work at the right scale for the input image
        for(int k=0;k<n_players_c;k++) {
            int r = 0, g = 0, b = 0;
            PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)k);
			n_defender_frames[9] = 11;
			ASSERT( n_defender_frames[9] <= max_defender_frames_c );
			for(int j=0;j<n_defender_frames[9];j++) {
				//defenders[k][9][j] = gfx_def_image->copy(16*j, 0, 16, 16);
				defenders[k][9][j] = gfx_def_image->copy(32*j+8, 9, 16, 18);
                defenders[k][9][j]->remap(240, 0, 0, r, g, b);
                processImage(defenders[k][9][j]);
				defenders[k][9][j]->setOffset(0, -4);
            }
		}
		delete gfx_def_image;

		for(int i=0;i<=5;i++) {
			if( !loadAttackersWalkingImages(gfx_dir, i) ) {
				return false;
			}
		}

		if( !loadAttackersWalkingImages(gfx_dir, n_epochs_c) ) {
			return false;
		}
		drawProgress(60);

		Image *gfx_planes = Image::loadImage(gfx_dir + "attacker_flying.png");
		if( gfx_planes == NULL )
			return false;
		drawProgress(62);
		/*if( !gfx_planes->scaleTo(scale_width*default_width_c) )
		return false;*/
        gfx_planes->setScale(scale_width/scale_factor_w, scale_height/scale_factor_h); // so the copying will work at the right scale for the input image
        // do remapping before scaling
        for(int i=0;i<n_players_c;i++) {
            int r = 0, g = 0, b = 0;
            PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)i);
            for(int j=0;j<n_saucer_frames_c;j++) {
                saucers[i][j] = gfx_planes->copy(32*j, 64, 32, 32);
                saucers[i][j]->remap(240, 0, 0, r, g, b);
                processImage(saucers[i][j]);
            }
        }
        // do remapping before scaling
		for(int i=0;i<n_players_c;i++) {
			//nukes[i][0] = gfx_planes->copy(64*i, 32, 32, 32);
			//nukes[i][1] = gfx_planes->copy(64*i+32, 32, 32, 32);
			nukes[i][0] = gfx_planes->copy(0, 32, 32, 32);
			nukes[i][1] = gfx_planes->copy(32, 32, 32, 32);
			int r = 0, g = 0, b = 0;
			PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)i);
			for(int j=0;j<2;j++) {
				nukes[i][j]->remap(240, 0, 0, r, g, b);
                processImage(nukes[i][j]);
			}
		}
		// now remap
        processImage(gfx_planes);
		for(int i=0;i<n_players_c;i++) {
			for(int j=0;j<n_epochs_c;j++)
				planes[i][j] = NULL;
			planes[i][6] = gfx_planes->copy(32*i, 0, 32, 32);
			planes[i][7] = gfx_planes->copy(128+32*i, 0, 32, 32);
		}
		delete gfx_planes;

		Image *gfx_ammo = Image::loadImage(gfx_dir + "attacker_ammo.png");
		if( gfx_ammo == NULL )
			return false;
		drawProgress(65);
		/*if( !gfx_ammo->scaleTo(scale_width*default_width_c) )
		return false;*/
		processImage(gfx_ammo);
		for(int i=0;i<6;i++) {
			attackers_ammo[i][ATTACKER_AMMO_RIGHT] = gfx_ammo->copy(0, 16*i, 16, 16);
			attackers_ammo[i][ATTACKER_AMMO_LEFT] = gfx_ammo->copy(16, 16*i, 16, 16);
			attackers_ammo[i][ATTACKER_AMMO_UP] = gfx_ammo->copy(32, 16*i, 16, 16);
			attackers_ammo[i][ATTACKER_AMMO_DOWN] = gfx_ammo->copy(48, 16*i, 16, 16);
		}
		// bombs
		attackers_ammo[6][ATTACKER_AMMO_BOMB] = gfx_ammo->copy(0, 96, 8, 16);
		delete gfx_ammo;

		/*nuke_defences[0] = armies->copy(192, 256, 16, 16);
		nuke_defences[1] = armies->copy(208, 256, 16, 16);
		nuke_defences[2] = armies->copy(192, 272, 16, 16);
		nuke_defences[3] = armies->copy(208, 272, 16, 16);*/
		for(int k=0;k<n_players_c;k++) {
			nuke_defences[k] = defenders[k][nuclear_epoch_c][0];
		}

		attackers_ammo[7][ATTACKER_AMMO_BOMB] = attackers_ammo[6][ATTACKER_AMMO_BOMB];
		attackers_ammo[9][ATTACKER_AMMO_BOMB] = Image::createRadial((int)(scale_width * 16), (int)(scale_height * 16), 1.0f, 0, 255, 255);
		processImage(attackers_ammo[9][ATTACKER_AMMO_BOMB]);

        for(int i=0;i<=n_epochs_c;i++) {
            if( i == 6 || i == 7 || i == 8 || i == 9 )
                continue;
			for(int player=0;player<n_players_c;player++) {
				for(int dir=0;dir<n_attacker_directions_c;dir++) {
					int n_frames = n_attacker_frames[i][dir];
					for(int frame=0;frame<n_frames;frame++) {
						int r = 0, g = 0, b = 0;
						PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)player);
						ASSERT(attackers_walking[player][i][dir][frame] != NULL);
						attackers_walking[player][i][dir][frame]->remap(240, 0, 0, r, g, b);
					}
				}
			}
        }
        for(int i=0;i<n_saucer_frames_c;i++) {
            for(int k=0;k<n_players_c;k++) {
                int r = 0, g = 0, b = 0;
                PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)k);
                saucers[k][i]->remap(240, 0, 0, r, g, b);
            }
        }
        for(int i=0;i<n_epochs_c;i++) {
            if( defenders[0][i][0] == NULL )
                continue;
            for(int j=0;j<n_defender_frames[i];j++) {
                for(int k=0;k<n_players_c;k++) {
                    int r = 0, g = 0, b = 0;
                    PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)k);
                    defenders[k][i][j]->remap(240, 0, 0, r, g, b);
                }
            }
        }
    }

	// features
	Image *gfx_features = Image::loadImage(gfx_dir + "features.png");
	if( gfx_features == NULL )
		return false;
	/*if( !gfx_features->scaleTo(scale_width*default_width_c) )
	return false;*/
	processImage(gfx_features);
	icon_openpitmine = gfx_features->copy(0, 0, 47, 24);

	icon_trees[0][0] = Image::loadImage(gfx_dir + "tree2_00.png");
	icon_trees[0][1] = Image::loadImage(gfx_dir + "tree2_01.png");
	icon_trees[0][2] = Image::loadImage(gfx_dir + "tree2_02.png");
	icon_trees[0][3] = Image::loadImage(gfx_dir + "tree2_03.png");

	icon_trees[1][0] = Image::loadImage(gfx_dir + "tree3_00.png");
	icon_trees[1][1] = Image::loadImage(gfx_dir + "tree3_01.png");
	icon_trees[1][2] = Image::loadImage(gfx_dir + "tree3_02.png");
	icon_trees[1][3] = Image::loadImage(gfx_dir + "tree3_03.png");

	// [2][] is the nuked tree image
	icon_trees[2][0] = Image::loadImage(gfx_dir + "deadtree1_00.png");
	for(int j=1;j<n_tree_frames_c;j++) {
		icon_trees[2][j] = icon_trees[2][0]->copy(); // no animation for nuked tree
	}

	icon_trees[3][0] = Image::loadImage(gfx_dir + "tree5_00.png");
	icon_trees[3][1] = Image::loadImage(gfx_dir + "tree5_01.png");
	icon_trees[3][2] = Image::loadImage(gfx_dir + "tree5_02.png");
	icon_trees[3][3] = Image::loadImage(gfx_dir + "tree5_03.png");

	for(int i=0;i<n_trees_c;i++) {
		for(int j=0;j<n_tree_frames_c;j++) {
			if( icon_trees[i][j] == NULL )
				return false;
			processImage(icon_trees[i][j]);
		}
	}

	icon_clutter.push_back(Image::loadImage(gfx_dir + "boulders.png"));
	icon_clutter.push_back(Image::loadImage(gfx_dir + "boulders2.png"));
	icon_clutter.push_back(Image::loadImage(gfx_dir + "bigboulder.png"));
	icon_clutter.push_back(Image::loadImage(gfx_dir + "rocks.png"));
	icon_clutter.push_back(Image::loadImage(gfx_dir + "plant.png"));
	icon_clutter.push_back(Image::loadImage(gfx_dir + "grass.png"));
	icon_clutter.push_back(Image::loadImage(gfx_dir + "grasses01.png"));
	icon_clutter.push_back(Image::loadImage(gfx_dir + "grasses02.png"));
	icon_clutter.push_back(Image::loadImage(gfx_dir + "grasses04.png"));
	icon_clutter.push_back(Image::loadImage(gfx_dir + "grasses05.png"));
	icon_clutter.push_back(Image::loadImage(gfx_dir + "shrub2-01.png"));
	icon_clutter.push_back(Image::loadImage(gfx_dir + "swirl01.png"));
	icon_clutter.push_back(Image::loadImage(gfx_dir + "weed01.png"));
	icon_clutter.push_back(Image::loadImage(gfx_dir + "weed02.png"));
	icon_clutter.push_back(Image::loadImage(gfx_dir + "weed03.png"));
	icon_clutter.push_back(Image::loadImage(gfx_dir + "weed04.png"));
	for(size_t i=0;i<icon_clutter.size();i++) {
		if( icon_clutter[i] == NULL )
			return false;
		processImage(icon_clutter[i]);
	}
	icon_clutter_nuked.push_back(Image::loadImage(gfx_dir + "bones.png"));
	icon_clutter_nuked.push_back(Image::loadImage(gfx_dir + "skulls.png"));
	for(size_t i=0;i<icon_clutter_nuked.size();i++) {
		if( icon_clutter_nuked[i] == NULL )
			return false;
		processImage(icon_clutter_nuked[i]);
	}
	drawProgress(70);

	background_islands = background;

	// finished loading/extracting images

	drawProgress(80);
    /*int time_taken = clock() - time_s;
    LOG("time taken to load images: %d (%d=1sec)\n", time_taken, CLOCKS_PER_SEC);*/
	return true;
}

void Game::setupInventions() {
	/*for(int i=0;i<n_shields_c;i++)
	invention_shields[i] = new Invention("SHIELD", Invention::SHIELD, start_epoch + i);*/
	for(int i=0;i<n_epochs_c;i++)
		invention_shields[i] = new Invention("SHIELD", Invention::SHIELD, i);

	invention_defences[0] = new Invention("STICK", Invention::DEFENCE, 0);
	invention_defences[1] = new Invention("SPEAR", Invention::DEFENCE, 1);
	invention_defences[2] = new Invention("SHORTBOW", Invention::DEFENCE, 2);
	invention_defences[3] = new Invention("CAULDRON OF OIL", Invention::DEFENCE, 3);
	invention_defences[4] = new Invention("CROSSBOW", Invention::DEFENCE, 4);
	invention_defences[5] = new Invention("RIFLE", Invention::DEFENCE, 5);
	invention_defences[6] = new Invention("MACHINE GUN", Invention::DEFENCE, 6);
	invention_defences[7] = new Invention("ROCKET", Invention::DEFENCE, 7);
	invention_defences[8] = new Invention("NUCLEAR DEFENCE", Invention::DEFENCE, 8);
	invention_defences[9] = new Invention("SDI LASER", Invention::DEFENCE, 9);

	invention_weapons[0] = new Weapon("ROCK WEAPON", 0, 1);
	//invention_weapons[1] = new Weapon("CATAPULT", 1, 1);
	invention_weapons[1] = new Weapon("SWORD", 1, 1);
	invention_weapons[2] = new Weapon("PIKE", 2, 1);
	invention_weapons[3] = new Weapon("LONGBOW", 3, 1);
	invention_weapons[4] = new Weapon("TREBUCHET", 4, 2);
	invention_weapons[5] = new Weapon("CANNON", 5, 3);
	invention_weapons[6] = new Weapon("BIPLANE", 6, 2);
	invention_weapons[7] = new Weapon("BOMBER", 7, 3);
	invention_weapons[8] = new Weapon("NUCLEAR MISSILE", 8, 0);
	invention_weapons[9] = new Weapon("SPACESHIP", 9, 8);
}

void Game::setupElements() {
	elements[WOOD] = new Element("WOOD", WOOD, Element::GATHERABLE);
	elements[ROCK] = new Element("ROCK", ROCK, Element::GATHERABLE);
	elements[BONE] = new Element("BONE", BONE, Element::GATHERABLE);
	elements[SLATE] = new Element("SLATE", SLATE, Element::GATHERABLE);
	elements[MOONLITE] = new Element("MOONLITE", MOONLITE, Element::OPENPITMINE);
	elements[PLANETARIUM] = new Element("PLANETARIUM", PLANETARIUM, Element::OPENPITMINE);
	elements[BETHLIUM] = new Element("BETHLIUM", BETHLIUM, Element::OPENPITMINE);
	elements[SOLARIUM] = new Element("SOLARIUM", SOLARIUM, Element::OPENPITMINE);
	elements[ARULDITE] = new Element("ARULDITE", ARULDITE, Element::DEEPMINE);
	elements[HERBIRITE] = new Element("HERBIRITE", HERBIRITE, Element::DEEPMINE);
	elements[YERIDIUM] = new Element("YERIDIUM", YERIDIUM, Element::DEEPMINE);
	elements[VALIUM] = new Element("VALIUM", VALIUM, Element::DEEPMINE);
	elements[PARASITE] = new Element("PARASITE", PARASITE, Element::DEEPMINE);
	elements[AQUARIUM] = new Element("AQUARIUM", AQUARIUM, Element::DEEPMINE);
	elements[PALADIUM] = new Element("PALADIUM", PALADIUM, Element::DEEPMINE);
	elements[ONION] = new Element("ONION", ONION, Element::DEEPMINE);
	elements[TEDIUM] = new Element("TEDIUM", TEDIUM, Element::DEEPMINE);
	elements[MORON] = new Element("MORON", MORON, Element::DEEPMINE);
	elements[MARMITE] = new Element("MAAMITE", MARMITE, Element::DEEPMINE);
	elements[ALIEN] = new Element("ALIEN", ALIEN, Element::DEEPMINE);
}

void Game::cleanupPlayers() {
	for(int i=0;i<n_players_c;i++) {
		if( players[i] )
			delete players[i];
		players[i] = NULL;
	}

	// need to reset alliance flags!
	Player::resetAllAlliances();
}

void Game::setupPlayers() {
	LOG("Game::setupPlayers()");
	cleanupPlayers();

	//   0 - Red team
	//   1 - Green team
	//   2 - Yellow team
	//   3 - Blue team

	int n_opponents = maps[start_epoch][selected_island]->getNOpponents();
	ASSERT( n_opponents+1 <= maps[start_epoch][selected_island]->getNSquares() );
	int n_free = 4;
	if( human_player != PLAYER_DEMO ) {
		players[human_player] = new Player(true, human_player);
		n_free--;
	}
	else {
		n_opponents++;
	}

	if( gameMode == GAMEMODE_SINGLEPLAYER ) {
		for(int i=0;i<n_opponents && n_free > 0;i++) {
			int indx = rand() % n_free;
			for(int j=0;j<4;j++) {
				if( players[j] == NULL ) {
					if( indx == 0 ) {
						players[j] = new Player(false, j);
						n_free--;
						break;
					}
					indx--;
				}
			}
		}
	}

}

// get the full desktop resolution
void Game::getDesktopResolution(int *user_width, int *user_height) const {
#if AROS
		// AROS doesn't have latest SDL version with SDL_GetVideoInfo, so use native code!
		getAROSScreenSize(user_width, user_height);
#elif defined(__MORPHOS__)
		// MorphOS doesn't have latest SDL version with SDL_GetVideoInfo, so use native code!
		getAROSScreenSize(user_width, user_height);
#else

#if SDL_MAJOR_VERSION == 1
		const SDL_VideoInfo *videoInfo = SDL_GetVideoInfo();
		*user_width = videoInfo->current_w;
		*user_height = videoInfo->current_h;
		LOG("desktop is %d x %d\n", *user_width, *user_height);
#else
		SDL_DisplayMode displayMode;
		if( SDL_GetDesktopDisplayMode(0, &displayMode) != 0 ) {
			LOG("SDL_GetDesktopDisplayMode failed!");
			*user_width = 640;
			*user_height = 480;
		}
		else {
			*user_width = displayMode.w;
			*user_height = displayMode.h;
			LOG("desktop is %d x %d\n", *user_width, *user_height);
		}
#endif

#endif
}

bool Game::openScreen(bool fullscreen) {
	if( !fullscreen ) {
		int user_width = 0, user_height = 0;
#if defined(WINRT)
		getDesktopResolution(&user_width, &user_height);
#elif defined(_WIN32)
		//#if 0
		// we do it using system calls instead of getDesktopResolution(), to ignore the start bar (if showing)
		RECT rect;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
		user_width = rect.right - rect.left;
		user_height = rect.bottom - rect.top;
		LOG("desktop is %d x %d\n", user_width, user_height);
		user_height -= GetSystemMetrics(SM_CYCAPTION); // also ignore the window height
		LOG("available height is %d\n", user_height);
#else
		getDesktopResolution(&user_width, &user_height);
#endif

#if SDL_MAJOR_VERSION == 1
		// Only allow integer scaling, otherwise have poor quality scaling (especially for the font)
		//user_width = 1184;
		//user_height = 720;

		if( user_width >= 4*default_width_c ) {
			scale_width = 4.0f;
			LOG("scale width 4x\n");
		}
		else if( user_width >= 3*default_width_c ) {
			scale_width = 3.0f;
			LOG("scale width 3x\n");
		}
		else if( user_width >= 2*default_width_c ) {
			scale_width = 2.0f;
			LOG("scale width 2x\n");
		}
		else if( user_width >= default_width_c ) {
			scale_width = 1.0f;
			LOG("scale width 1x\n");
		}
		else {
			LOG("desktop resolution too small even for 1x scale width\n");
			return false;
		}

		if( user_height >= 4*default_height_c ) {
			scale_height = 4.0f;
			LOG("scale height 4x\n");
		}
		else if( user_height >= 3*default_height_c ) {
			scale_height = 3.0f;
			LOG("scale height 3x\n");
		}
		else if( user_height >= 2*default_height_c ) {
			scale_height = 2.0f;
			LOG("scale height 2x\n");
		}
		else if( user_height >= default_height_c ) {
			scale_height = 1.0f;
			LOG("scale height 1x\n");
		}
		else {
			LOG("desktop resolution too small even for 1x scale height\n");
			return false;
		}

		// better to have uniform scaling, so we have 1:1 aspect ratio
		scale_width = std::min(scale_width, scale_height);
		scale_height = scale_width;

		//scale_width = 2.0f; scale_height = 1.5f;
		//scale_width = scale_height = 1.0f; // test
		//scale_width = scale_height = 2.0f; // test
		//scale_width = 1.0f;
		//scale_height = 1.0f;
		
		int screen_width = (int)(scale_width * default_width_c);
		int screen_height = (int)(scale_height * default_height_c);
#else
		// with SDL2, we let SDL do the scaling via SDL_RenderSetLogicalSize, so we don't have to do the scaling ourselves, and can set the screen width/height to whatever we like
		// for windowed mode, we pick a suitable size based on the available desktop space
		int screen_width = default_width_c;
		int screen_height = default_height_c;

		while( 2*screen_width <= user_width && 2*screen_height <= user_height ) {
			screen_width *= 2;
			screen_height *= 2;
		}
		//screen_width = 480;
		//screen_height = 320;
		//screen_width = 640;
		//screen_height = 480;
#endif

		screen = new Screen();
		if( !screen->open(screen_width, screen_height, fullscreen) )
			return false;

	}
	else {
		// fullscreen
		screen = new Screen();

#if SDL_MAJOR_VERSION == 1
		if( screen->open(4*default_width_c, 4*default_height_c, fullscreen) ) {
			scale_width = scale_height = 4.0f;
			LOG("scale 4x\n");
		}
		else if( screen->open(3*default_width_c, 3*default_height_c, fullscreen) ) {
			scale_width = scale_height = 3.0f;
			LOG("scale 3x\n");
		}
		else if( screen->open(2*default_width_c, 2*default_height_c, fullscreen) ) {
			scale_width = scale_height = 2.0f;
			LOG("scale 2x\n");
		}
		else if( screen->open(default_width_c, default_height_c, fullscreen) ) {
			scale_width = scale_height = 1.0f;
			LOG("scale 1x\n");
		}
		else {
			LOG("can't even open screen at 1x scale\n");
			return false;
		}
#else
		// With SDL2, we let SDL do the scaling via SDL_RenderSetLogicalSize, so we don't have to do the scaling ourselves
		// for fullscreen, the supplied width/height is ignored, as we always run at the native resolution.
		int user_width = 0, user_height = 0;
#ifdef _WIN32
		// However, on Windows 10, there is a bug in the task view that Gigalomania shows as a tiny window, unless we've opened the screen with the desktop resolution
		getDesktopResolution(&user_width, &user_height);
#endif
		if( !screen->open(user_width, user_height, fullscreen) ) {
			LOG("can't open screen\n");
			return false;
		}
#endif

	}

	char buffer[256] = "";
	sprintf(buffer, "Gigalomania, version %d.%d - Loading...", majorVersion, minorVersion);
	screen->setTitle(buffer);
	return true;
}

bool Game::readMapProcessLine(int *epoch, int *index, Map **l_map, char *line, const int MAX_LINE, const char *filename) {
	bool ok = true;
	line[ strlen(line) - 1 ] = '\0'; // trim new line
	line[ strlen(line) - 1 ] = '\0'; // trim carriage return
	//LOG("line: %s\n", line);
	if( *l_map == NULL ) {
		if( line[0] != '#' ) {
			LOG("expected first character to be '#'\n");
			ok = false;
			return ok;
		}

		//char name[MAX_LINE+1] = "";
		int n_opponents = -1;
		//char colname[MAX_LINE+1] = "";

		char *ptr = strtok(&line[1], " ");
		if( ptr == NULL ) {
			LOG("can't find map name\n");
			ok = false;
			return ok;
		}
		//strcpy(name, ptr);
		string name = ptr;

		ptr = strtok(NULL, " ");
		if( ptr == NULL ) {
			LOG("can't find epoch\n");
			ok = false;
			return ok;
		}
		*epoch = atoi(ptr);

		ptr = strtok(NULL, " ");
		if( ptr == NULL ) {
			LOG("can't find n_opponents\n");
			ok = false;
			return ok;
		}
		n_opponents = atoi(ptr);

		ptr = strtok(NULL, " ");
		if( ptr == NULL ) {
			LOG("can't find colour name\n");
			ok = false;
			return ok;
		}
		//strcpy(colname, ptr);
		string colname = ptr;

		MapColour map_colour = MAP_UNDEFINED_COL;
		if( strcmp(colname.c_str(), "ORANGE") == 0 ) {
			map_colour = MAP_ORANGE;
		}
		else if( strcmp(colname.c_str(), "GREEN") == 0 ) {
			map_colour = MAP_GREEN;
		}
		else if( strcmp(colname.c_str(), "BROWN") == 0 ) {
			map_colour = MAP_BROWN;
		}
		else if( strcmp(colname.c_str(), "WHITE") == 0 ) {
			map_colour = MAP_WHITE;
		}
		else if( strcmp(colname.c_str(), "DBROWN") == 0 ) {
			map_colour = MAP_DBROWN;
		}
		else if( strcmp(colname.c_str(), "DGREEN") == 0 ) {
			map_colour = MAP_DGREEN;
		}
		else if( strcmp(colname.c_str(), "GREY") == 0 ) {
			map_colour = MAP_GREY;
		}
		else {
			LOG("unknown map colour: %s\n", colname.c_str());
			ok = false;
			return ok;
		}

		*index = 0;
		while( *index < max_islands_per_epoch_c && maps[*epoch][*index] != NULL )
			*index = *index + 1;
		if( *index == max_islands_per_epoch_c ) {
			LOG("too many islands for this epoch\n");
			ok = false;
			return ok;
		}
		*l_map = maps[*epoch][*index] = new Map(map_colour, n_opponents, name.c_str());
		(*l_map)->setFilename(filename);
	}
	else {
		char *line_ptr = line;
		while( *line_ptr == ' ' || *line_ptr == '\t' )
			line_ptr++;
		char *ptr = strtok(line_ptr, " ");
		if( ptr == NULL ) {
			LOG("can't find first word\n");
			ok = false;
			return ok;
		}
		else if( strcmp(ptr, "SECTOR") == 0 ) {
			ptr = strtok(NULL, " ");
			if( ptr == NULL ) {
				LOG("can't find sec_x\n");
				ok = false;
				return ok;
			}
			int sec_x = atoi(ptr);
			if( sec_x < 0 || sec_x >= map_width_c ) {
				LOG("invalid map x %d\n", sec_x);
				ok = false;
				return ok;
			}

			ptr = strtok(NULL, " ");
			if( ptr == NULL ) {
				LOG("can't find sec_y\n");
				ok = false;
				return ok;
			}
			int sec_y = atoi(ptr);
			if( sec_y < 0 || sec_y >= map_height_c ) {
				LOG("invalid map y %d\n", sec_y);
				ok = false;
				return ok;
			}
			(*l_map)->newSquareAt(sec_x, sec_y);
		}
		else if( strcmp(ptr, "ELEMENT") == 0 ) {
			// ignore for now
		}
		else if( ptr[0] == '#' ) {
			// this line is a comment
		}
		else {
			LOG("unknown word: %s\n", ptr);
			ok = false;
			return ok;
		}
	}
	return ok;
}

bool Game::readLineFromRWOps(bool &ok, SDL_RWops *file, char *buffer, char *line, int MAX_LINE, int &buffer_offset, int &newline_index, bool &reached_end) {
	if( newline_index > 1 ) {
		// not safe to use strcpy on overlapping strings (undefined behaviour)
		int len = strlen(&buffer[newline_index-1]);
		memmove(buffer, &buffer[newline_index-1], len);
		buffer[len] = '\0';
		if( reached_end && buffer[0] == '\0' ) {
			return true;
		}
		buffer_offset = MAX_LINE - (newline_index-1);
	}
	if( !reached_end ) {
		// fill up buffer
		for(;;) {
			int n_read = file->read(file, &buffer[buffer_offset], 1, MAX_LINE-buffer_offset);
			//LOG("buffer offset %d , read %d\n", buffer_offset, n_read);
			if( n_read == 0 ) {
				// error or eof - don't quit yet, still need to finish reading buffer
				//LOG("read all of file\n");
				reached_end = true;
				break;
			}
			else {
				buffer[buffer_offset+n_read] = '\0';
				if( n_read < MAX_LINE-buffer_offset ) {
					// we didn't read all of the available buffer, and haven't reached end of file yet
					buffer_offset += n_read;
				}
				else {
					break;
				}
			}
		}
	}
	//LOG("buffer: %s\n", buffer);
	newline_index = 0;
	while( buffer[newline_index] != '\n' && buffer[newline_index] != '\0' ) {
		line[newline_index] = buffer[newline_index];
		newline_index++;
	}
	if( buffer[newline_index] == '\0' ) {
		LOG("file has too long line\n");
		ok = false;
		return true;
	}
	line[newline_index++] = '\n';
	line[newline_index++] = '\0';
	return false;
}

bool Game::readMap(const char *filename) {
	//LOG("readMap: %s\n", filename); // disabled logging to improve performance on startup
	bool ok = true;
	const int MAX_LINE = 4096;
	//const int MAX_LINE = 64;
	char line[MAX_LINE+1] = "";
	Map *l_map = NULL;
	int epoch = -1;
	int index = -1;

    char fullname[4096] = "";
	sprintf(fullname, "%s/%s", maps_dirname, filename);
	// open in binary mode, so that we parse files in an OS-independent manner
	// (otherwise, Windows will parse "\r\n" as being "\n", but Linux will still read it as "\n")
	//FILE *file = fopen(fullname, "rb");
	SDL_RWops *file = SDL_RWFromFile(fullname, "rb");
#if !defined(__ANDROID__) && defined(__linux)
	if( file == NULL ) {
		LOG("searching in /usr/share/gigalomania/ for islands folder\n");
		sprintf(fullname, "%s/%s", alt_maps_dirname, filename);
		file = SDL_RWFromFile(fullname, "rb");
	}
#endif
    if( file == NULL ) {
		LOG("failed to open file: %s\n", fullname);
		return false;
	}
	char buffer[MAX_LINE+1] = "";
	int buffer_offset = 0;
	bool reached_end = false;
	int newline_index = 0;
	while( ok ) {
		bool done = readLineFromRWOps(ok, file, buffer, line, MAX_LINE, buffer_offset, newline_index, reached_end);
		if( !ok )  {
			LOG("failed to read line for: %s\n", filename);
		}
		else if( done ) {
			break;
		}
		else {
			ok = readMapProcessLine(&epoch, &index, &l_map, line, MAX_LINE, filename);
		}
	}
	file->close(file);

	if( !ok && l_map != NULL ) {
		LOG("delete map that was created\n");
		delete l_map;
		ASSERT(epoch != -1); // should have been set, if l_map!=NULL
		ASSERT(index != -1); // should have been set, if l_map!=NULL
		l_map = maps[epoch][index] = NULL;
	}
	return ok;
}

int sortMapsFunc(const void *a, const void *b) {
	Map *map_a = *(Map **)a;
	Map *map_b = *(Map **)b;
	// handling for 'Ohm'...
	if( map_a->getName()[0] == '0' )
		return 1;
	else if( map_b->getName()[0] == '0' )
		return -1;
	return strcmp(map_a->getName(), map_b->getName());
}

bool Game::createMaps() {
	LOG("createMaps()...\n");

#if defined(WINRT) // @TODO use windows api to read folder contents
	readMap("0mega.map");
	readMap("alpha.map");
	readMap("binary.map");
	readMap("castle.map");
	readMap("devil.map");
	readMap("eep.map");
	readMap("final.map");
	readMap("font.map");
	readMap("globe.map");
	readMap("home.map");
	readMap("infinity.map");
	readMap("just.map");
	readMap("koala.map");
	readMap("loop.map");
	readMap("moon.map");
	readMap("ninth.map");
	readMap("oxygen.map");
	readMap("polar.map");
	readMap("quart.map");
	readMap("rare.map");
	readMap("semi.map");
	readMap("toxic.map");
	readMap("universal.map");
	readMap("vine.map");
	readMap("wreath.map");
	readMap("x.map");
	readMap("yen.map");
	readMap("zinc.map");
#elif defined(_WIN32)
    WIN32_FIND_DATAA findFileData;
	char maps_dirname_w[256];
	sprintf(maps_dirname_w, "%s\\*", maps_dirname);
	HANDLE handle = FindFirstFileA(maps_dirname_w, &findFileData);
	if( handle == INVALID_HANDLE_VALUE ) {
		LOG("Invalid File Handle. GetLastError reports %d\n", GetLastError());
		return false;
	}
	for(;;) {
		if( !readMap(findFileData.cFileName) ) {
			LOG("failed reading map: %s\n", findFileData.cFileName);
			// don't fail altogether, just ignore
		}
		if( FindNextFileA(handle, &findFileData) == 0 ) {
			FindClose(handle);
			DWORD error = GetLastError();
			if( error != ERROR_NO_MORE_FILES ) {
				LOG("error reading directory: %d\n", error);
				return false;
			}
			break;
		}
	}
#elif defined(__ANDROID__)
	// unclear how to read contents of a folder in assets, so we just hardcode the islands (not like the user can easily drop new files there)
	readMap("0mega.map");
	readMap("alpha.map");
	readMap("binary.map");
	readMap("castle.map");
	readMap("devil.map");
	readMap("eep.map");
	readMap("final.map");
	readMap("font.map");
	readMap("globe.map");
	readMap("home.map");
	readMap("infinity.map");
	readMap("just.map");
	readMap("koala.map");
	readMap("loop.map");
	readMap("moon.map");
	readMap("ninth.map");
	readMap("oxygen.map");
	readMap("polar.map");
	readMap("quart.map");
	readMap("rare.map");
	readMap("semi.map");
	readMap("toxic.map");
	readMap("universal.map");
	readMap("vine.map");
	readMap("wreath.map");
	readMap("x.map");
	readMap("yen.map");
	readMap("zinc.map");
#else

	DIR *dir = opendir(maps_dirname);

#if !defined(__ANDROID__) && defined(__linux)
	if( dir == NULL ) {
		LOG("searching in /usr/share/gigalomania/ for islands folder\n");
		dir = opendir(alt_maps_dirname);
	}
#endif
	if( dir == NULL ) {
		LOG("failed to open directory: %s\n", maps_dirname);
		LOG("error: %d\n", errno);
		return false;
	}
	for(;;) {
		errno = 0;
		dirent *ent = readdir(dir);
		if( ent == NULL ) {
			closedir(dir);
			if( errno ) {
				LOG("error reading directory: %d\n", errno);
				return false;
			}
			break;
		}
		if( !readMap(ent->d_name) ) {
			LOG("failed reading map: %s\n", ent->d_name);
			// don't fail altogether, just ignore
		}
	}
#endif
	LOG("done reading directory\n");

	for(int i=0;i<n_epochs_c;i++) {
		int n_islands = 0;
		while(n_islands < max_islands_per_epoch_c && maps[i][n_islands] != NULL)
			n_islands++;
		if( n_islands == 0 ) {
			LOG("can't find any islands for epoch %d\n", i);
			return false;
		}
		qsort((void *)&maps[i], n_islands, sizeof(maps[i][0]), sortMapsFunc);
	}
	return true;
}

void Game::disposeGameState() {
	ASSERT( dispose_gamestate == NULL );
	LOG("disposeGameState: %d\n", gamestate);
	dispose_gamestate = gamestate;
	gamestate = NULL;
}

void Game::setGameStateID(GameStateID state, GameState *new_gamestate) {
	LOG("setGameStateID(%d, %d)\n", state, new_gamestate);
	LOG("old gameStateID was %d\n", gameStateID);
	gameStateID = state;
	playMusic();

	GameState *old_gamestate = gamestate;
	if( gamestate != NULL ) {
		disposeGameState();
	}

	if( new_gamestate != NULL ) {
		gamestate = new_gamestate;
	}
	else if( gameStateID == GAMESTATEID_CHOOSEGAMETYPE )
		gamestate = new ChooseGameTypeGameState(human_player);
	else if( gameStateID == GAMESTATEID_CHOOSEDIFFICULTY )
		gamestate = new ChooseDifficultyGameState(human_player);
	else if( gameStateID == GAMESTATEID_CHOOSEPLAYER )
		gamestate = new ChoosePlayerGameState(human_player);
	else if( gameStateID == GAMESTATEID_CHOOSETUTORIAL )
		gamestate = new ChooseTutorialGameState(human_player);
	else if( gameStateID == GAMESTATEID_PLACEMEN )
		gamestate = new PlaceMenGameState(human_player);
	else if( gameStateID == GAMESTATEID_PLAYING ) {
		gamestate = new PlayingGameState(human_player);
		int map_x = 0, map_y = 0, n_men = 0;
		if( gameType == GAMETYPE_TUTORIAL ) {
			map_x = tutorial->getStartMapX();
			map_y = tutorial->getStartMapY();
			n_men = tutorial->getNMen();
			game_g->players[human_player]->setNMenForThisIsland( n_men ); // need to set this here - for non-tutorial, this is done in PlaceMenGameState::setStartMapPos()
		}
		else {
			map_x = static_cast<PlaceMenGameState *>(old_gamestate)->getStartMapX();
			map_y = static_cast<PlaceMenGameState *>(old_gamestate)->getStartMapY();
			n_men = human_player == PLAYER_DEMO ? 0 : players[human_player]->getNMenForThisIsland();
		}
		static_cast<PlayingGameState *>(gamestate)->createSectors(map_x, map_y, n_men);
		if( gameType == GAMETYPE_TUTORIAL ) {
			tutorial->initCards();
		}
	}
	else if( gameStateID == GAMESTATEID_ENDISLAND )
		gamestate = new EndIslandGameState(human_player);
	else if( gameStateID == GAMESTATEID_GAMECOMPLETE )
		gamestate = new GameCompleteGameState(human_player);
	else {
		ASSERT(false);
	}

	gamestate->reset();
	state_changed = false;
	//gameWon = GAME_PLAYING;
	/*if( state == GAMESTATE_PLACEMEN ) {
	placeMenInfo.init();
	}*/
	// cheat/test:
	/*if( gameStateID == GAMESTATEID_PLAYING && new_gamestate == NULL && gameType != GAMETYPE_TUTORIAL ) {
		// n.b., if we ever want this to run in tutorial mode, the old_gamestate won't be of type PlaceMenGameState
		int map_x = static_cast<PlaceMenGameState *>(old_gamestate)->getStartMapX();
		int map_y = static_cast<PlaceMenGameState *>(old_gamestate)->getStartMapY();
		Sector *start_sector = map->getSector(map_x, map_y);
		start_sector->cheat(human_player);
	}*/
}

void Game::setupTutorial(const string &id) {
	T_ASSERT(tutorial == NULL);
	tutorial = TutorialManager::setupTutorial(id);
}

void Game::startIsland() {
	ASSERT(gameStateID == GAMESTATEID_PLACEMEN);
	/*int map_x = static_cast<PlaceMenGameState *>(gamestate)->getStartMapX();
	int map_y = static_cast<PlaceMenGameState *>(gamestate)->getStartMapY();*/

	//setupPlayers();
	setGameStateID(GAMESTATEID_PLAYING);
	gamestate->fadeScreen(false, 0, NULL);
	gameResult = GAMERESULT_UNDEFINED;
}

void startIsland_g() {
	// wrapper so we can pass as a function pointer
	game_g->startIsland();
}

void Game::endIsland() {
	ASSERT(gameStateID == GAMESTATEID_PLAYING);
	map->calculateStats();
	map->freeSectors(); // must do this here rather than waiting until deleting PlayingGameState, as by that time the "map" variable may have switched to a different island
	n_player_suspended += players[human_player]->getNSuspended();
	setGameStateID(GAMESTATEID_ENDISLAND);
	gamestate->fadeScreen(false, 0, NULL);

	if( gameResult == GAMERESULT_QUIT ) {
		// pick a random non-human player
		int n_cpu = 0;
		for(int i=0;i<n_players_c;i++) {
			if( players[i] != NULL && !players[i]->isDead() && i != human_player ) {
				n_cpu++;
			}
		}
		if( n_cpu > 0 ) {
			int index = rand() % n_cpu;
			n_cpu = 0;
			for(int i=0;i<n_players_c;i++) {
				if( players[i] != NULL && !players[i]->isDead() && i != human_player ) {
					if( n_cpu == index ) {
						playSample( s_quit[i] );
						break;
					}
					n_cpu++;
				}
			}
		}
	}

	if( gameResult == GAMERESULT_WON && gameType == GAMETYPE_ALLISLANDS ) {
		//n_men_store -= n_men_for_this_island;
		n_men_store -= players[human_player]->getNMenForThisIsland();
		if( start_epoch == n_epochs_c-1 ) {
		}
		else {
			ASSERT( !completed_island[selected_island] );
			completed_island[selected_island] = true;
			// check advance to next epoch
			bool completed_epoch = true;
			for(int i=0;i<max_islands_per_epoch_c && maps[start_epoch][i] != NULL && completed_epoch;i++) {
				if( !completed_island[i] )
					completed_epoch = false;
			}
			LOG("completed epoch? %d\n", completed_epoch);
			if( completed_epoch ) {
				n_men_store += getMenPerEpoch();
				nextEpoch();
			}
			else {
				nextIsland();
			}
		}
	}
}

void endIsland_g() {
	// wrapper so we can pass as a function pointer
	game_g->endIsland();
}

void Game::returnToChooseIsland() {
	ASSERT(gameStateID == GAMESTATEID_ENDISLAND);
	if( gameType == GAMETYPE_TUTORIAL ) {
		LOG("delete tutorial %d\n", tutorial);
		delete tutorial;
		tutorial = NULL;
		setGameStateID(GAMESTATEID_CHOOSEGAMETYPE);
	}
	else if( gameResult == GAMERESULT_WON && gameType == GAMETYPE_ALLISLANDS ) {
		if( start_epoch == n_epochs_c-1 ) {
			setGameStateID(GAMESTATEID_GAMECOMPLETE);
		}
		else {
			setGameStateID(GAMESTATEID_PLACEMEN);
		}
	}
	else {
		setGameStateID(GAMESTATEID_PLACEMEN);
		// test
		//setGameStateID(GAMESTATEID_GAMECOMPLETE);
	}

	gamestate->fadeScreen(false, 0, NULL);

	if( human_player != PLAYER_DEMO )
		players[human_player]->setNMenForThisIsland(0);
}

void returnToChooseIsland_g() {
	// wrapper so we can pass as a function pointer
	game_g->returnToChooseIsland();
}

void Game::startNewGame() {
	ASSERT(gameStateID == GAMESTATEID_GAMECOMPLETE);
	setGameStateID(GAMESTATEID_PLACEMEN);
	//gamestate->fadeScreen(false, 0, NULL);
	newGame();
}

void startNewGame_g() {
	// wrapper so we can pass as a function pointer
	game_g->startNewGame();
}

void Game::placeTower() {
	ASSERT( gameStateID == GAMESTATEID_PLACEMEN );
	if( !state_changed ) {
		state_changed = true;
		gamestate->fadeScreen(true, 0, startIsland_g);
	}
}

//bool quit = false;
bool debugwindow = false;

void Game::requestQuit(bool force_quit) {
    if( !state_changed ) {
	    gamestate->requestQuit(force_quit);
	}
}

void Game::keypressReturn() {
    if( !state_changed ) {
	    gamestate->requestConfirm();
	}
}

void Game::togglePause() {
    if( gameStateID == GAMESTATEID_PLAYING ) {
        paused = !paused;
        if( paused ) {
            playSample(s_on_hold);

			// n.b., pausing music/looped-sounds absolutely important on Android, so music stops when game goes into background; but useful for other platforms too
			// note also that we don't pause all sounds, as it would immediately pause the "putting you on hold" sample!
			Sample::pauseMusic();
			Sample::pauseChannel(SOUND_CHANNEL_BIPLANE);
			Sample::pauseChannel(SOUND_CHANNEL_BOMBER);
			Sample::pauseChannel(SOUND_CHANNEL_SPACESHIP);
        }
		else {
			Sample::unpauseMusic();
			Sample::unpauseChannel(SOUND_CHANNEL_BIPLANE);
			Sample::unpauseChannel(SOUND_CHANNEL_BOMBER);
			Sample::unpauseChannel(SOUND_CHANNEL_SPACESHIP);
		}
    }
}

void Game::activate() {
	this->deleteState();
    if( gameStateID != GAMESTATEID_PLAYING ) {
		// when state is GAMESTATEID_PLAYING, we don't restart the music until the game is unpaused
		Sample::unpauseMusic();
	}
}

void Game::deactivate() {
    if( gameStateID != GAMESTATEID_PLAYING ) {
		Sample::pauseMusic();
	}
	else {
		if( !this->isPaused() ) {
			this->togglePause(); // automatically pause when application goes inactive
		}
	}
	this->saveState();
}

bool Game::isPaused() const {
	return paused;
}

void Game::setTimeRate(int time_rate) {
	this->time_rate = time_rate;
	LOG("time_rate = %d\n", time_rate);
}

void Game::setRealTime(int real_time) {
	this->real_time = real_time;
}

int Game::getRealTime() const {
	return real_time;
}

int Game::getRealLoopTime() const {
	return real_loop_time;
}

void Game::setGameTime(int game_time) {
	this->game_time = game_time;
}

int Game::getGameTime() const {
	return game_time;
}

int Game::getLoopTime() const {
	return loop_time;
}

void Game::updateTime(int time) {
	// prevent instability on slow machines
	const int max_interval_c = 200;
	if( time > max_interval_c )
		time = max_interval_c;

	real_loop_time = time;
	real_time += time;

	// Ideally we'd have always had time_rate being an integer, and have all usages of loop_time cope with that, but this would now be a significant change.
	// So we add this fix so that we don't have inaccuracy due to rounding. To test this code, disable wait() in Application::runMainLoop(), which means
	// we'll test this function with very small values of time.
	loop_time = (int)(time * time_ratio_c * time_rate + accumulated_time);
	accumulated_time = (time * time_ratio_c * time_rate + accumulated_time) - loop_time;
	//LOG("time %d loop time %d accumulated %f\n", time, loop_time, accumulated_time);

	game_time += loop_time;
	frame_counter = (getRealTime() * time_rate) / ticks_per_frame_c;
}

void Game::resetMouseClick() {
	mouseTime = -1;
}

int Game::getNClicks() {
	if( mouseTime == -1 )
		mouseTime = getRealTime();
	int time = getRealTime() - mouseTime;
	if( time < 2000 )
		return 1;
	else if( time < 5000 )
		return 2;
	return 3;
}

void Game::deleteState() const {
	const char *save_fullfilename = getApplicationFilename(autosave_filename, autosave_survive_uninstall);
	remove(save_fullfilename);
	delete [] save_fullfilename;
}

void Game::saveState() const {
    if( gameStateID == GAMESTATEID_UNDEFINED || gameStateID == GAMESTATEID_CHOOSEGAMETYPE || gameStateID == GAMESTATEID_CHOOSEDIFFICULTY || gameStateID == GAMESTATEID_CHOOSEPLAYER || gameStateID == GAMESTATEID_CHOOSETUTORIAL || gameStateID == GAMESTATEID_GAMECOMPLETE ) {
		// no need to save state
	}
	else if( gameType == GAMETYPE_TUTORIAL && gameStateID == GAMESTATEID_ENDISLAND ) {
		// no need to save state (and don't want to, otherwise this will resume to the islands screen instead the main menu)
	}
	else {
		stringstream stream;
		const int savegame_version_c = 1;
		stream << "<?xml version=\"1.0\" ?>\n";
		stream << "<savegame major=\"" << majorVersion << "\" minor=\"" << minorVersion << "\" savegame_version=\"" << savegame_version_c << "\">\n";
		stream << "<global ";
		stream << "game_type=\"" << gameType << "\" ";
		stream << "difficulty_level=\"" << difficulty_level << "\" ";
		stream << "human_player=\"" << human_player << "\" ";
		stream << "n_men_store=\"" << n_men_store << "\" ";
		stream << "n_player_suspended=\"" << n_player_suspended << "\" ";
		stream << "start_epoch=\"" << start_epoch << "\" ";
		stream << "selected_island=\"" << selected_island << "\" ";
		stream << "/>\n";

		stream << "<time real_time=\"" << getRealTime() << "\" game_time=\"" << getGameTime() << "\" />\n";

		for(int i=0;i<max_islands_per_epoch_c;i++) {
			stream << "<completed_island island_id=\"" << i << "\" complete=\"" << (completed_island[i] ? 1 : 0) << "\"/>\n";
		}

		gamestate->saveState(stream);

	    stream << "</savegame>\n";

		const char *save_fullfilename = getApplicationFilename(autosave_filename, autosave_survive_uninstall);
		SDL_RWops *file = SDL_RWFromFile(save_fullfilename, "w+");
		if( file == NULL ) {
			LOG("failed to open: %s\n", save_fullfilename);
			LOG("error: %s\n", SDL_GetError());
		}
		else {
			size_t length = stream.str().length();
			char *ptr = new char[length];
			stream.read(ptr, length);
			file->write(file, ptr, length, 1);
			file->close(file);
		}
		delete [] save_fullfilename;
	}
}

GameState *Game::loadStateParseXMLNode(const TiXmlNode *parent) {
	if( parent == NULL ) {
		return NULL;
	}
	bool read_children = true;
	GameState *new_gamestate = NULL;

	switch( parent->Type() ) {
		case TiXmlNode::TINYXML_DOCUMENT:
			break;
		case TiXmlNode::TINYXML_ELEMENT:
			{
				const char *element_name = parent->Value();
				const TiXmlElement *element = parent->ToElement();
				const TiXmlAttribute *attribute = element->FirstAttribute();
				if( strcmp(element_name, "savegame") == 0 ) {
					int save_major = -1, save_minor = -1;
					while( attribute != NULL ) {
						const char *attribute_name = attribute->Name();
						if( strcmp(attribute_name, "major") == 0 ) {
							save_major = static_cast<DifficultyLevel>(atoi(attribute->Value()));
						}
						else if( strcmp(attribute_name, "minor") == 0 ) {
							save_minor = static_cast<DifficultyLevel>(atoi(attribute->Value()));
						}
						else if( strcmp(attribute_name, "savegame_version") == 0 ) {
							int savegame_version = static_cast<DifficultyLevel>(atoi(attribute->Value()));
							LOG("save game version %d\n", savegame_version);
						}
						else {
							// don't throw an error here, to help backwards compatibility, but should throw an error in debug mode in case this is a sign of not loading something that we've saved
							LOG("unknown game/savegame attribute: %s\n", attribute_name);
							ASSERT(false);
						}
						attribute = attribute->Next();
					}
					LOG("saved game version %d.%d\n", save_major, save_minor);
					LOG("current game version %d.%d\n", majorVersion, minorVersion);
				}
				else if( strcmp(element_name, "global") == 0 ) {
					bool set_start_epoch = false;
					bool set_start_island = false;
					while( attribute != NULL ) {
						const char *attribute_name = attribute->Name();
						if( strcmp(attribute_name, "game_type") == 0 ) {
							gameType = static_cast<GameType>(atoi(attribute->Value()));
							if( gameType != GAMETYPE_SINGLEISLAND && gameType != GAMETYPE_ALLISLANDS && gameType != GAMETYPE_TUTORIAL ) {
								throw std::runtime_error("unknown game_type");
							}
						}
						else if( strcmp(attribute_name, "difficulty_level") == 0 ) {
							difficulty_level = static_cast<DifficultyLevel>(atoi(attribute->Value()));
							if( difficulty_level < 0 || difficulty_level >= DIFFICULTY_N_LEVELS ) {
								throw std::runtime_error("invalid difficulty_level");
							}
						}
						else if( strcmp(attribute_name, "human_player") == 0 ) {
							human_player = atoi(attribute->Value());
							if( human_player < 0 || selected_island >= n_players_c ) {
								throw std::runtime_error("invalid human_player");
							}
						}
						else if( strcmp(attribute_name, "n_men_store") == 0 ) {
							n_men_store = atoi(attribute->Value());
							if( n_men_store < 0 ) {
								throw std::runtime_error("invalid n_men_store");
							}
						}
						else if( strcmp(attribute_name, "n_player_suspended") == 0 ) {
							n_player_suspended = atoi(attribute->Value());
							if( n_player_suspended < 0 ) {
								throw std::runtime_error("invalid n_player_suspended");
							}
						}
						else if( strcmp(attribute_name, "start_epoch") == 0 ) {
							start_epoch = atoi(attribute->Value());
							if( start_epoch < 0 || start_epoch >= n_epochs_c ) {
								throw std::runtime_error("invalid start_epoch");
							}
							updatedEpoch();
							set_start_epoch = true;
						}
						else if( strcmp(attribute_name, "selected_island") == 0 ) {
							selected_island = atoi(attribute->Value());
							if( selected_island < 0 || selected_island >= max_islands_per_epoch_c ) {
								throw std::runtime_error("invalid selected_island");
							}
							set_start_island = true;
						}
						else {
							// don't throw an error here, to help backwards compatibility, but should throw an error in debug mode in case this is a sign of not loading something that we've saved
							LOG("unknown game/global attribute: %s\n", attribute_name);
							ASSERT(false);
						}
						attribute = attribute->Next();
					}
					if( set_start_epoch && set_start_island ) {
						map = maps[start_epoch][selected_island];
					}
					else {
						throw std::runtime_error("map not set");
					}
				}
				else if( strcmp(element_name, "completed_island") == 0 ) {
					int island_id = -1;
					bool complete = false;
					while( attribute != NULL ) {
						const char *attribute_name = attribute->Name();
						if( strcmp(attribute_name, "island_id") == 0 ) {
							island_id = atoi(attribute->Value());
							if( island_id < 0 || island_id >= max_islands_per_epoch_c ) {
								throw std::runtime_error("completed_island invalid island_id");
							}
						}
						else if( strcmp(attribute_name, "complete") == 0 ) {
							complete = atoi(attribute->Value())==1;
						}
						else {
							// don't throw an error here, to help backwards compatibility, but should throw an error in debug mode in case this is a sign of not loading something that we've saved
							LOG("unknown game/completed_island attribute: %s\n", attribute_name);
							ASSERT(false);
						}
						attribute = attribute->Next();
					}
					if( island_id == -1 ) {
						throw std::runtime_error("completed_island missing island_id");
					}
					completed_island[island_id] = complete;
				}
				else if( strcmp(element_name, "time") == 0 ) {
					// we need to set this at the Game level rather than the PlayingGamestate, so that the times are set before creating the map (otherwise messes up the saved times for the particle systems)
					while( attribute != NULL ) {
						const char *attribute_name = attribute->Name();
						if( strcmp(attribute_name, "real_time") == 0 ) {
							int real_time = atoi(attribute->Value());
							setRealTime(real_time);
						}
						else if( strcmp(attribute_name, "game_time") == 0 ) {
							int game_time = atoi(attribute->Value());
							setGameTime(game_time);
						}
						else {
							// don't throw an error here, to help backwards compatibility, but should throw an error in debug mode in case this is a sign of not loading something that we've saved
							LOG("unknown game/time attribute: %s\n", attribute_name);
							ASSERT(false);
						}
						attribute = attribute->Next();
					}
				}
				else if( strcmp(element_name, "playing_gamestate") == 0 ) {
					PlayingGameState *playing_gamestate = new PlayingGameState(human_player);
					try {
						if( map == NULL ) {
							throw std::runtime_error("playing_gamestate map not yet set");
						}
						map->createSectors(playing_gamestate, start_epoch);
						playing_gamestate->loadStateParseXMLNode(parent);
						if( gameType == GAMETYPE_TUTORIAL ) {
							if( tutorial == NULL ) {
								throw std::runtime_error("didn't set tutorial");
							}
						}
						//throw std::runtime_error("blah"); // test failing to load state
						new_gamestate = playing_gamestate;
						read_children = false;
					}
					catch(const std::runtime_error &error) {
						LOG("cleanup due to error loading state: %s\n", error.what());
						delete playing_gamestate;
						throw error;
					}
				}
				else {
					// don't throw an error here, to help backwards compatibility, but should throw an error in debug mode in case this is a sign of not loading something that we've saved
					LOG("unknown game tag: %s\n", element_name);
					ASSERT(false);
				}
			}
			break;
		case TiXmlNode::TINYXML_COMMENT:
			break;
		case TiXmlNode::TINYXML_UNKNOWN:
			break;
		case TiXmlNode::TINYXML_TEXT:
			break;
		case TiXmlNode::TINYXML_DECLARATION:
			break;
	}

	for(const TiXmlNode *child=parent->FirstChild();child!=NULL && read_children;child=child->NextSibling())  {
		GameState *sub_gamestate = loadStateParseXMLNode(child);
		if( sub_gamestate != NULL ) {
			if( new_gamestate != NULL ) {
				throw std::runtime_error("more than one gamestate defined");
			}
			new_gamestate = sub_gamestate;
		}
	}
	return new_gamestate;
}

bool Game::loadState() {
	bool ok = false;
	stringstream stream;
	const char *save_fullfilename = getApplicationFilename(autosave_filename, autosave_survive_uninstall);
	SDL_RWops *file = SDL_RWFromFile(save_fullfilename, "r");
	if( file == NULL ) {
		LOG("couldn't find or open saved state file: %s\n", save_fullfilename);
	}
	else {
		LOG("found a saved state file: %s\n", save_fullfilename);
#if SDL_MAJOR_VERSION == 1
		// SDL 1 doesn't have a size parameter
		SDL_RWseek(file, 0, RW_SEEK_END);
		size_t size = (size_t)SDL_RWtell(file);
		SDL_RWseek(file, 0, RW_SEEK_SET);
#else
		size_t size = (size_t)file->size(file);
#endif
		char *buffer = new char[size+1];
		if( file->read(file, buffer, 1, size) > 0 ) {
			file->close(file);
			// rename immediately so that if there's a crash while loading the saved state, the game doesn't repeatedly crash
			const char *save_old_fullfilename = getApplicationFilename(autosave_old_filename, autosave_survive_uninstall);
			remove(save_old_fullfilename);
			rename(save_fullfilename, save_old_fullfilename);

			buffer[size] = '\0';

			TiXmlDocument doc;
			if( doc.Parse(buffer) == NULL ) {
				LOG("failed to parse XML file, error row %d col %d\n", doc.ErrorRow(), doc.ErrorCol());
				LOG("error: %s\n", doc.ErrorDesc());
			}
			else {
				try {
					GameState *new_gamestate = loadStateParseXMLNode(&doc);
					// we create a new gamestate if playing a game
					if( new_gamestate != NULL ) {
						LOG("loaded PlayingGameState\n");
						int c_page = static_cast<PlayingGameState *>(new_gamestate)->getGamePanel()->getPage();
						setGameStateID(GAMESTATEID_PLAYING, new_gamestate);
						static_cast<PlayingGameState *>(new_gamestate)->getGamePanel()->setPage(c_page);
					}
					else {
						LOG("loaded PlaceMenGameState\n");
						setGameStateID(GAMESTATEID_PLACEMEN);
					}
					ok = true;
				}
				catch(const std::runtime_error &error) {
					LOG("caught error loading state: %s\n", error.what());
				}
			}
			if( !ok ) {
				LOG("rename bad save file\n");
				const char *save_bad_fullfilename = getApplicationFilename(autosave_bad_filename, autosave_survive_uninstall);
				remove(save_bad_fullfilename);
				rename(save_old_fullfilename, save_bad_fullfilename);

				if( gamestate != NULL ) {
					LOG("delete gamestate\n");
					delete gamestate;
					gamestate = NULL;
				}
				if( tutorial != NULL ) {
					LOG("delete tutorial\n");
					delete tutorial;
					tutorial = NULL;
				}
			}
		}
		else {
			file->close(file);
			const char *save_bad_fullfilename = getApplicationFilename(autosave_bad_filename, autosave_survive_uninstall);
			remove(save_bad_fullfilename);
			rename(save_fullfilename, save_bad_fullfilename);
		}
		delete [] buffer;
	}
	delete [] save_fullfilename;
	return ok;
}

void Game::mouseClick(int m_x, int m_y, bool m_left, bool m_middle, bool m_right, bool click) {
	const int mousepress_delay = 100;
	T_ASSERT( m_left || m_middle || m_right );
	if( click ) {
		//LOG("mouse click\n");
        gamestate->mouseClick(m_x, m_y, m_left, m_middle, m_right, click);
	}
	else {
		// use getTicks() not getRealTime(), as this function may be called at any occasion (due to mouse click events), so getRealTime() might not yet be updated!
		unsigned int ticks = game_g->getApplication()->getTicks();
		if( ticks - lastmousepress_time >= mousepress_delay ) {
			//LOG("mouse press: %d, %d\n", lastmousepress_time, ticks);
			lastmousepress_time = ticks;
			gamestate->mouseClick(m_x, m_y, m_left, m_middle, m_right, click);
		}
		else {
			//LOG("ignore mouse press\n");
		}
	}
}

void Game::updateGame() {
	if( !paused ) {
		int m_x = 0, m_y = 0;
		bool m_left = false, m_middle = false, m_right = false;
		bool m_res = screen->getMouseState(&m_x, &m_y, &m_left, &m_middle, &m_right);
		/*screen->getMouseCoords(&m_x, &m_y);
		game_g->getApplication()->getMousePressed(&m_left, &m_middle, &m_right);*/
		if( m_res ) {
			mouseClick(m_x, m_y, m_left, m_middle, m_right, false);
		}
		else {
			resetMouseClick();
		}
	}

	// update
	if( !paused ) {
		if( gameStateID == GAMESTATEID_PLAYING ) {
			for(int i=0;i<n_players_c;i++) {
				if( i != human_player && players[i] != NULL )
					players[i]->doAIUpdate(human_player, static_cast<PlayingGameState *>(gamestate));
			}
			//players[ enemy_player ]->doAIUpdate();
			gamestate->update();
			for(int y=0;y<map_height_c;y++) {
				for(int x=0;x<map_width_c;x++) {
					/*if( map->sectors[x][y] != NULL )
					map->sectors[x][y]->update();*/
					Sector *sector = map->getSector(x, y);
					if( sector != NULL ) {
						sector->update(human_player);
					}
				}
			}
		}
	}

	if( gameStateID == GAMESTATEID_PLAYING && !state_changed && gameMode != GAMEMODE_MULTIPLAYER_CLIENT ) {
		if( human_player != PLAYER_DEMO && !playerAlive(human_player) ) {
			playSample(s_itis_all_over);
			state_changed = true;
			gameResult = GAMERESULT_LOST;
			fadeMusic(SHORT_DELAY + 1000);
			gamestate->fadeScreen(true, SHORT_DELAY, endIsland_g);
		}
		else {
			bool all_dead = true;
			/*for(int i=0;i<n_players_c && all_dead;i++) {
			if( i != human_player && players[i] != NULL && playerAlive(i) )
			all_dead = false;
			}*/
			for(int i=0;i<n_players_c;i++) {
				if( i != human_player && players[i] != NULL && !players[i]->isDead() ) {
					if( playerAlive(i) )
						all_dead = false;
					else {
						players[i]->kill(static_cast<PlayingGameState *>(gamestate));
					}
				}
			}
			//if( !playerAlive(enemy_player) ) {
			if( all_dead || (tutorial != NULL && tutorial->getCard() == NULL && tutorial->autoEnd() ) ) {
				playSample(s_won);
				state_changed = true;
				gameResult = GAMERESULT_WON;
				fadeMusic(SHORT_DELAY + 1000);
				gamestate->fadeScreen(true, SHORT_DELAY, endIsland_g);
			}
		}
	}

	if( dispose_gamestate != NULL ) {
		LOG("delete dispose_gamestate %d (current gamestate is %d)\n", dispose_gamestate, gamestate);
		delete dispose_gamestate;
		LOG("done delete\n");
		dispose_gamestate = NULL;
	}
}

void Game::drawGame() const {
	// we now redraw even when paused, to display paused message
	gamestate->draw();
}

const char prefs_filename[] = "prefs";
const bool prefs_survive_uninstall = false; // no need for prefs file to save uninstall, and seems better to allow resetting to initial condition (especially on Android where this is typical behaviour)
const char onemousebutton_key[] = "onemousebutton";
const char sound_on_key[] = "sound_on";
const char music_on_key[] = "music_on";
const char disallow_nukes_key[] = "disallow_nukes";

bool Game::createApplication() {
	application = new Application();
	if( !application->init() ) {
		return false;
	}
	return true;
}

void Game::loadPrefs() {
	const char *prefs_fullfilename = getApplicationFilename(prefs_filename, prefs_survive_uninstall);
	SDL_RWops *prefs_file = SDL_RWFromFile(prefs_fullfilename, "rb");
	if( prefs_file != NULL ) {
		// reset
		pref_sound_on = false;
		pref_music_on = false;
		pref_disallow_nukes = false;
		onemousebutton = false;

		const int MAX_LINE = 4096;
		char line[MAX_LINE+1] = "";
		char buffer[MAX_LINE+1] = "";
		int buffer_offset = 0;
		bool reached_end = false;
		int newline_index = 0;
		bool ok = true;
		while( ok ) {
			bool done = readLineFromRWOps(ok, prefs_file, buffer, line, MAX_LINE, buffer_offset, newline_index, reached_end);
			if( !ok )  {
				LOG("failed to read line for: %s\n", prefs_fullfilename);
			}
			else if( done ) {
				break;
			}
			else {
				LOG("read prefs line: %s", line);
				if( strncmp(line, onemousebutton_key, strlen(onemousebutton_key)) == 0 ) {
					LOG("enable onemousebutton from prefs\n");
					onemousebutton = true;
				}
				else if( strncmp(line, sound_on_key, strlen(sound_on_key)) == 0 ) {
					LOG("enable pref_sound_on from prefs\n");
					pref_sound_on = true;
				}
				else if( strncmp(line, music_on_key, strlen(music_on_key)) == 0 ) {
					LOG("enable pref_music_on from prefs\n");
					pref_music_on = true;
				}
				else if( strncmp(line, disallow_nukes_key, strlen(disallow_nukes_key)) == 0 ) {
					LOG("enable pref_disallow_nukes from prefs\n");
					pref_disallow_nukes = true;
				}
			}
		}
		prefs_file->close(prefs_file);

#if defined(__ANDROID__)
		// force onemousebutton mode, just to be safe
		onemousebutton = true;
#endif
	}
	delete [] prefs_fullfilename;
}

void Game::savePrefs() const {
	const char *prefs_fullfilename = getApplicationFilename(prefs_filename, prefs_survive_uninstall);
	SDL_RWops *prefs_file = SDL_RWFromFile(prefs_fullfilename, "wb+");
	if( prefs_file == NULL ) {
		LOG("failed to open: %s\n", prefs_fullfilename);
	}
	else {
		if( onemousebutton ) {
			prefs_file->write(prefs_file, onemousebutton_key, sizeof(char), strlen(onemousebutton_key));
			prefs_file->write(prefs_file, "\n", sizeof(char), 1);
		}
		if( pref_sound_on ) {
			prefs_file->write(prefs_file, sound_on_key, sizeof(char), strlen(sound_on_key));
			prefs_file->write(prefs_file, "\n", sizeof(char), 1);
		}
		if( pref_music_on ) {
			prefs_file->write(prefs_file, music_on_key, sizeof(char), strlen(music_on_key));
			prefs_file->write(prefs_file, "\n", sizeof(char), 1);
		}
		if( pref_disallow_nukes ) {
			prefs_file->write(prefs_file, disallow_nukes_key, sizeof(char), strlen(disallow_nukes_key));
			prefs_file->write(prefs_file, "\n", sizeof(char), 1);
		}
		prefs_file->close(prefs_file);
	}
	delete [] prefs_fullfilename;
}

bool Game::testFindSoldiersBuildingNewTower(const Sector *sector, int *total, int *squares) const {
	bool found = false;
	*total = 0;
	*squares = 0;
	for(int cx=0;cx<map_width_c && !found;cx++) {
		for(int cy=0;cy<map_height_c && !found;cy++) {
			Sector *c_sector = game_g->getMap()->getSector(cx, cy);
			if( sector != c_sector ) {
				if( c_sector->getArmy(sector->getPlayer())->any(true) ) {
					found = true;
					*total = *total + c_sector->getArmy(sector->getPlayer())->getTotal();
					*squares = *squares + 1;
					if( c_sector->getPlayer() != -1 ) {
						throw string("shouldn't have sent soldiers to another tower");
					}
					for(int i=0;i<n_players_c;i++) {
						if( i != sector->getPlayer() && c_sector->getArmy(i)->any(true) ) {
							throw string("shouldn't have sent soldiers to a sector with other soldiers");
						}
					}
				}
			}
		}
	}
	return found;
}

void Game::runTests() {
	game_g->setTesting(true);

	human_player = rand() % 4;
	//human_player = 0;
	//human_player = 1;
	map = maps[start_epoch][selected_island];
	setGameStateID(GAMESTATEID_PLACEMEN);
	newGame();
	// check all maps are loaded
	for(int i=0;i<n_epochs_c;i++) {
		int expected_n_islands = i==n_epochs_c-1 ? 1 : max_islands_per_epoch_c;
		for(int j=0;j<expected_n_islands;j++) {
			if( maps[i][j] == NULL ) {
				LOG("failed to load island: %d , %d\n", i, j);
				throw string("failed to load island");
			}
		}
		for(int j=expected_n_islands;j<max_islands_per_epoch_c;j++) {
			if( maps[i][j] != NULL ) {
				LOG("unexpected island: %d , %d\n", i, j);
				throw string("unexpected island");
			}
		}
	}

	while(true) {
		LOG("###TEST: epoch %d island %d\n", start_epoch, selected_island);
		PlaceMenGameState *placeMenGameState = static_cast<PlaceMenGameState *>(gamestate);
		setupPlayers();
		int sx = 0, sy = 0;
		if( start_epoch == 0 && selected_island == 0 ) {
			sx = 1;
			sy = 2;
			placeMenGameState->getChooseMenPanel()->setNMen(15);
		}
		else if( start_epoch == 0 && selected_island == 1 ) {
			sx = 1;
			sy = 2;
			placeMenGameState->getChooseMenPanel()->setNMen(15);
		}
		else if( start_epoch == 0 && selected_island == 2 ) {
			sx = 1;
			sy = 2;
			placeMenGameState->getChooseMenPanel()->setNMen(15);
		}
		else if( start_epoch == 1 && selected_island == 0 ) {
			sx = 1;
			sy = 2;
			placeMenGameState->getChooseMenPanel()->setNMen(15);
		}
		else if( start_epoch == 4 && selected_island == 1 ) {
			sx = 3;
			sy = 4;
			placeMenGameState->getChooseMenPanel()->setNMen(15);
			map->setReserved(sx-3, sy, true);
		}
		else if( start_epoch == 5 && selected_island == 0 ) {
			sx = 1;
			sy = 4;
			placeMenGameState->getChooseMenPanel()->setNMen(15);
		}
		else {
			map->findRandomSector(&sx, &sy);
			placeMenGameState->getChooseMenPanel()->setNMen(10);
		}
		placeMenGameState->setStartMapPos(sx, sy); // will automatically switch to playing gamestate
		updateGame(); // needed to dispose the gamestate

		int ex = -1, ey = -1;
		for(int y=0;y<map_height_c && ex==-1;y++) {
			for(int x=0;x<map_width_c && ex==-1;x++) {
				Sector *sector = map->getSector(x, y);
				if( sector != NULL ) {
					if( sector->getPlayer() != -1 && sector->getPlayer() != human_player ) {
						ex = x;
						ey = y;
					}
				}
			}
		}
		if( ex == -1 || ey == -1 ) {
			throw string("couldn't find ai player");
		}

		// test saving state
		saveState();
#if defined(_WIN32) || defined(__linux) || (defined(__APPLE__) && defined(__MACH__))
		// ensure on a platform where access() is defined (it isn't available on AROS etc - we could write platform specific code, but not really worth it for now)
		if( access(getApplicationFilename(autosave_filename, autosave_survive_uninstall), 0) != 0 ) {
			throw string("save state file not created");
		}
#endif

		// test loading state
		delete gamestate;
		gamestate = NULL;
		if( !loadState() ) {
			throw string("failed to load state");
		}
		else if( gamestate == NULL ) {
			throw string("failed to create new gamestate when loading state");
		}
		else if( gameStateID != GAMESTATEID_PLAYING ) {
			throw string("expected playinggamestate when loading state");
		}
		else if( tutorial != NULL ) {
			throw string("didn't expect tutorial when loading state");
		}
#if defined(_WIN32) || defined(__linux) || (defined(__APPLE__) && defined(__MACH__))
		// ensure on a platform where access() is defined (it isn't available on AROS etc - we could write platform specific code, but not really worth it for now)
		else if( access(getApplicationFilename(autosave_filename, autosave_survive_uninstall), 0) == 0 ) {
			throw string("save state file should have been deleted");
		}
#endif

		PlayingGameState *playingGameState = static_cast<PlayingGameState *>(gamestate);
		// island specific testing
		if( start_epoch == 0 && selected_island == 0 ) {
			Sector *start_sector = map->getSector(sx, sy);
			if( start_sector->bestDesign(Invention::WEAPON, 0) != NULL ) {
				throw string("shouldn't be able to build a rock weapon yet");
			}
			while( start_sector->anyElements(ROCK) ) {
				start_sector->mineElement(human_player, ROCK);
			}
			Design *design = start_sector->canResearch(Invention::WEAPON, 0);
			if( design == NULL ) {
				throw string("can't design rock weapon");
			}
			else if( !design->isErgonomicallyTerrific() ) {
				throw string("rock weapon isn't ergonomically terrific");
			}
			playingGameState->setCurrentDesign(sx, sy, design);
			start_sector->invent(human_player);
			if( !start_sector->canBuildDesign(design) ) {
				throw string("can't build rock weapon");
			}
			for(int i=0;i<10;i++) {
				if( !playingGameState->assembleArmy(sx, sy, 0, 1) ) {
					throw string("can't assemble rock weapon");
				}
			}
			for(int i=0;i<4;i++) {
				if( !playingGameState->assembleArmyUnarmed(sx, sy, 1) ) {
					throw string("can't assemble unarmed");
				}
			}
			if( !playingGameState->moveAssembledArmyTo(sx, sy, sx+1, sy) ) {
				throw string("can't move assembled army");
			}
			Sector *target_sector = map->getSector(sx+1, sy);
			Army *army = target_sector->getArmy(human_player);
			if( army->getTotal() != 14 ) {
				throw string("unexpected army total (1)");
			}
			if( army->getTotalMen() != 14 ) {
				throw string("unexpected army total men");
			}
			if( army->getSoldiers(0) != 10 ) {
				throw string("unexpected army number of rock weapons");
			}
			if( !playingGameState->moveArmyTo(sx+1, sy, sx, sy) ) {
				throw string("can't return army");
			}
			army = start_sector->getArmy(human_player);
			if( army->getTotal() >= 14 ) {
				throw string("no men were lost from retreating");
			}
			int n_men = army->getTotal();
			int n_rocks = army->getSoldiers(0);
			int n_population = start_sector->getPopulation();
			if( !playingGameState->moveArmyTo(sx, sy, sx+1, sy) ) {
				throw string("can't move army out again");
			}
			army = target_sector->getArmy(human_player);
			if( army->getTotal() != n_men ) {
				throw string("unexpected army total (2)");
			}
			if( !playingGameState->returnArmy(sx, sy, sx+1, sy) ) {
				throw string("can't return army to tower");
			}
			if( army->getTotal() != 0 ) {
				throw string("shouldn't have any men in the target sector");
			}
			army = start_sector->getArmy(human_player);
			if( army->getTotal() != 0 ) {
				throw string("shouldn't have any men in the start sector");
			}
			if( start_sector->getPopulation() <= n_population ) {
				throw string("didn't return any men");
			}
			else if( start_sector->getPopulation() >= n_population+n_men ) {
				throw string("no men were lost from retreating to tower");
			}
			else if( start_sector->getStoredArmy()->getSoldiers(0) <= 0 ) {
				throw string("didn't return any rock weapons");
			}
			else if( start_sector->getStoredArmy()->getSoldiers(0) >= n_rocks ) {
				throw string("no rock weapons were lost from retreating to tower");
			}
		}
		else if( start_epoch == 0 && selected_island == 1 ) {
			Sector *start_sector = map->getSector(sx, sy);
			if( start_sector->bestDesign(Invention::WEAPON, 0) != NULL ) {
				throw string("shouldn't be able to build a rock weapon yet");
			}
			// test time with and without sleep for mining:
			for(int type=0;type<2;type++) {
				unsigned int time_s = game_g->getApplication()->getTicks();
				unsigned int elapsed_time = time_s;
				int i_mined = 0, i_mined_fraction = 0;
				start_sector->getElementStocks(&i_mined, &i_mined_fraction, ROCK);
				int i_total = i_mined*element_multiplier_c + i_mined_fraction;
				const int n_steps_c = 22; // must not be greater than element_multiplier_c*max_gatherables_stored_c/2
				for(;;) {
					if( type == 0 ) {
						game_g->getApplication()->wait();
					}
					unsigned int new_time = game_g->getApplication()->getTicks();
					updateTime(new_time - elapsed_time);
					elapsed_time = new_time;
					updateGame();
					int mined = 0, mined_fraction = 0;
					start_sector->getElementStocks(&mined, &mined_fraction, ROCK);
					int total = mined*element_multiplier_c + mined_fraction;
					if( total >= i_total + n_steps_c ) {
						break;
					}
				}
				int time = game_g->getApplication()->getTicks() - time_s;
				int expected_time = (int)(( mine_rate_c * gameticks_per_hour_c * n_steps_c ) / ( element_multiplier_c * n_gatherable_rate_c * time_ratio_c ));
				int allowed_error = 20;
				if( abs(time - expected_time) > allowed_error ) {
					LOG("mining took %d , expected %d\n", time, expected_time);
					throw string("unexpected time for designing");
				}
			}
			// now mine the rest
			while( start_sector->anyElements(ROCK) ) {
				start_sector->mineElement(human_player, ROCK);
			}
			Design *design = start_sector->canResearch(Invention::WEAPON, 0);
			if( design == NULL ) {
				throw string("can't design rock weapon");
			}
			else if( !design->isErgonomicallyTerrific() ) {
				throw string("rock weapon isn't ergonomically terrific");
			}
			playingGameState->setCurrentDesign(sx, sy, design);
			playingGameState->setNDesigners(sx, sy, 1);
			// test time with and without sleep for designing:
			for(int type=0;type<2;type++) {
				unsigned int time_s = game_g->getApplication()->getTicks();
				unsigned int elapsed_time = time_s;
				int i_halfdays = 0, i_hours = 0;
				start_sector->inventionTimeLeft(&i_halfdays, &i_hours);
				int i_total = i_halfdays*12 + i_hours;
				const int n_steps_c = 3;
				for(;;) {
					if( type == 0 ) {
						game_g->getApplication()->wait();
					}
					unsigned int new_time = game_g->getApplication()->getTicks();
					updateTime(new_time - elapsed_time);
					elapsed_time = new_time;
					updateGame();
					int halfdays = 0, hours = 0;
					start_sector->inventionTimeLeft(&halfdays, &hours);
					int total = halfdays*12 + hours;
					if( total <= i_total - n_steps_c ) {
						break;
					}
				}
				int time = game_g->getApplication()->getTicks() - time_s;
				int expected_time = (int)(( gameticks_per_hour_c * n_steps_c ) / time_ratio_c);
				int allowed_error = 100;
				if( abs(time - expected_time) > allowed_error ) {
					LOG("halfday took %d , expected %d\n", time, expected_time);
					throw string("unexpected time for designing");
				}
			}
		}
		else if( start_epoch == 0 && selected_island == 2 ) {
			Sector *start_sector = map->getSector(sx, sy);
			if( start_sector->bestDesign(Invention::DEFENCE, 0) != NULL ) {
				throw string("shouldn't be able to build a stick weapon yet");
			}
			while( start_sector->anyElements(WOOD) ) {
				start_sector->mineElement(human_player, WOOD);
			}
			Design *design_stick = start_sector->canResearch(Invention::DEFENCE, 0);
			if( design_stick == NULL ) {
				throw string("can't design stick weapon");
			}
			else if( design_stick->isErgonomicallyTerrific() ) {
				throw string("stick weapon shouldn't be ergonomically terrific");
			}
			playingGameState->setCurrentDesign(sx, sy, design_stick);
			start_sector->invent(human_player);
			if( start_sector->getEpoch() != start_epoch ) {
				throw string("not on expected epoch");
			}
			if( !start_sector->canBuildDesign(design_stick) ) {
				throw string("can't build stick weapon");
			}
			playingGameState->deployDefender(sx, sy, BUILDING_TOWER, 1, 0);
			if( start_sector->getBuilding(BUILDING_TOWER)->getTurretMan(1) != 0 ) {
				throw string("didn't deploy defender");
			}

			Design *design_shield = start_sector->canResearch(Invention::SHIELD, 1);
			if( design_shield == NULL ) {
				throw string("can't design shield");
			}
			else if( design_shield->isErgonomicallyTerrific() ) {
				throw string("shield shouldn't be ergonomically terrific");
			}
			playingGameState->setCurrentDesign(sx, sy, design_shield);
			start_sector->invent(human_player);
			if( start_sector->getEpoch() != start_epoch+1 ) {
				throw string("didn't advance a tech level");
			}
			if( !start_sector->canBuildDesign(design_shield) ) {
				throw string("can't build shield");
			}
			int max_health = start_sector->getBuilding(BUILDING_TOWER)->getMaxHealth();
			if( start_sector->getBuilding(BUILDING_TOWER)->getHealth() != max_health ) {
				throw string("building not at max health");
			}
			start_sector->getBuilding(BUILDING_TOWER)->addHealth(-20);
			if( start_sector->getBuilding(BUILDING_TOWER)->getHealth() != max_health-20 ) {
				throw string("building not at reduced health");
			}
			playingGameState->useShield(sx, sy, BUILDING_TOWER, 1);
			if( start_sector->getBuilding(BUILDING_TOWER)->getHealth() != max_health-10 ) {
				throw string("shield didn't work as expected");
			}
		}
		else if( start_epoch == 1 && selected_island == 0 ) {
			Sector *sector = map->getSector(ex, ey);
			players[sector->getPlayer()]->doAIUpdate(human_player, playingGameState);
			if( sector->getCurrentDesign() != NULL ) {
				throw string("ai shouldn't be able to design anything yet");
			}
			for(int i=0;i<N_ID;i++) {
				while( sector->anyElements((Id)i) ) {
					sector->mineElement(human_player, (Id)i);
				}
			}
			players[sector->getPlayer()]->doAIUpdate(human_player, playingGameState);
			if( sector->getCurrentDesign() == NULL ) {
				throw string("ai didn't design anything");
			}
			// now invent everything
			for(int i=0;i<3;i++) {
				for(int j=0;j<n_epochs_c;j++) {
					Design *design = sector->canResearch((Invention::Type)i, j);
					if( design != NULL ) {
						sector->setCurrentDesign(design);
						sector->invent(human_player);
					}
				}
			}
			players[sector->getPlayer()]->doAIUpdate(human_player, playingGameState);
			if( sector->canBuild(BUILDING_MINE) ) {
				if( sector->getBuilders(BUILDING_MINE) <= 0 ) {
					throw string("didn't start building mine");
				}
			}
			if( sector->canBuild(BUILDING_FACTORY) ) {
				if( sector->getBuilders(BUILDING_FACTORY) <= 0 ) {
					throw string("didn't start building factory");
				}
			}
			if( sector->canBuild(BUILDING_LAB) ) {
				throw string("shouldn't be able to build a lab");
			}
		}
		else if( start_epoch == 4 && selected_island == 1 ) {
			Sector *start_sector = map->getSector(sx, sy);
			if( !start_sector->canBuild(BUILDING_MINE) ) {
				throw string("can't build mine");
			}
			start_sector->buildBuilding(BUILDING_MINE);
			if( !start_sector->canBuild(BUILDING_FACTORY) ) {
				throw string("can't build factory");
			}
			start_sector->buildBuilding(BUILDING_FACTORY);
			if( start_sector->canBuild(BUILDING_LAB) ) {
				throw string("shouldn't be able to build lab yet");
			}
			for(int i=0;i<N_ID;i++) {
				while( start_sector->canMine((Id)i) && start_sector->anyElements((Id)i) ) {
					start_sector->mineElement(human_player, (Id)i);
				}
			}
			Design *design_shield0 = start_sector->canResearch(Invention::SHIELD, 4);
			if( design_shield0 == NULL ) {
				throw string("can't design shield0");
			}
			Design *design_crossbow = start_sector->canResearch(Invention::DEFENCE, 4);
			if( design_crossbow == NULL ) {
				throw string("can't design crossbow");
			}
			Design *design_catapult = start_sector->canResearch(Invention::WEAPON, 4);
			if( design_catapult == NULL ) {
				throw string("can't design catapult");
			}
			Design *design_biplane = start_sector->canResearch(Invention::WEAPON, 6);
			if( design_biplane != NULL ) {
				throw string("shouldn't be able to design biplane yet (no lab)");
			}
			playingGameState->setCurrentDesign(sx, sy, design_shield0);
			start_sector->invent(human_player);
			playingGameState->setCurrentDesign(sx, sy, design_crossbow);
			start_sector->invent(human_player);
			if( start_sector->getEpoch() != start_epoch ) {
				throw string("not on expected epoch");
			}
			playingGameState->setCurrentDesign(sx, sy, design_catapult);
			start_sector->invent(human_player);
			if( start_sector->getEpoch() != start_epoch+1 ) {
				throw string("didn't advance a tech level");
			}
			design_biplane = start_sector->canResearch(Invention::WEAPON, 6);
			if( design_biplane != NULL ) {
				throw string("still shouldn't be able to design biplane yet (no lab)");
			}
			if( !start_sector->canBuild(BUILDING_LAB) ) {
				throw string("can't build lab");
			}
			start_sector->buildBuilding(BUILDING_LAB);
			design_biplane = start_sector->canResearch(Invention::WEAPON, 6);
			if( design_biplane == NULL ) {
				throw string("can't design biplane");
			}
			playingGameState->setCurrentDesign(sx, sy, design_biplane);
			start_sector->invent(human_player);
			if( !start_sector->canBuildDesign(design_biplane) ) {
				throw string("can't build biplane");
			}
			playingGameState->setCurrentManufacture(sx, sy, design_biplane);
			start_sector->buildDesign();
			int n_men = start_sector->getPopulation();
			if( !playingGameState->assembleArmy(sx, sy, 6, 1) ) {
				throw string("can't assemble biplane");
			}
			if( playingGameState->assembleArmy(sx, sy, 6, 1) ) {
				throw string("shouldn't be able to assemble another biplane");
			}
			if( !playingGameState->assembleArmyUnarmed(sx, sy, 1) ) {
				throw string("can't assemble unarmed");
			}
			if( start_sector->getPopulation() != n_men - 3 ) {
				throw string("unexpected remaining population");
			}
			if( start_sector->getAssembledArmy()->getTotal() != 2 ) {
				throw string("unexpected assembled army");
			}
			if( playingGameState->moveAssembledArmyTo(sx, sy, sx-3, sy) ) {
				throw string("shouldn't have been able to move all the assembled army");
			}
			if( start_sector->getAssembledArmy()->getTotal() != 1 ) {
				throw string("unexpected assembled army after moving biplanes");
			}
			Army *storedArmy = start_sector->getStoredArmy();
			if( storedArmy->getTotal() != 0 ) {
				throw string("didn't expect a stored army");
			}
			playingGameState->returnAssembledArmy(sx, sy);
			if( storedArmy->getTotal() != 0 ) {
				// unarmed men shouldn't be kept in stored_army
				throw string("didn't expect a stored army after returning assembled army");
			}
			if( start_sector->getPopulation() != n_men - 2 ) {
				throw string("unexpected remaining population after returning unarmed man");
			}
			Sector *targ_sector = map->getSector(sx-3, sy);
			Army *army = targ_sector->getArmy(human_player);
			if( army->getTotal() != 1 ) {
				throw string("unexpected army total");
			}
			if( army->getTotalMen() != 2 ) {
				throw string("unexpected army total men");
			}
			if( army->getSoldiers(6) != 1 ) {
				throw string("unexpected army number of biplanes");
			}
			// now put some unarmed men to destination
			army->add(n_epochs_c, 10);
			if( army->getTotal() != 11 ) {
				throw string("unexpected army total after adding unarmed men");
			}
			if( playingGameState->moveArmyTo(sx-3, sy, sx, sy) ) {
				throw string("shouldn't have been able to move all the army back");
			}
			if( army->getTotal() != 10 ) {
				throw string("unexpected army total after returning biplanes");
			}
			if( !playingGameState->moveArmyTo(sx, sy, sx-3, sy) ) {
				throw string("should have been able to move all the biplanes back again");
			}
			if( army->getTotal() != 11 ) {
				throw string("unexpected army total after moving biplanes back again");
			}
			// now put a catapult to destination
			army->add(4, 1);
			if( army->getTotal() != 12 ) {
				throw string("unexpected army total after adding a catapult");
			}
			if( army->getTotalMen() != 14 ) {
				throw string("unexpected army total men after adding a catapult");
			}
			if( storedArmy->getTotal() != 0 ) {
				throw string("still didn't expect a stored army");
			}
			if( playingGameState->returnArmy(sx, sy, sx-3, sy) ) {
				throw string("shouldn't have been able to move all the army back to tower");
			}
			if( army->getTotal() != 11 ) {
				throw string("unexpected army total after returning biplanes to tower");
			}
			if( army->getSoldiers(4) != 1 ) {
				throw string("unexpected army number of catapults after returning biplanes to tower");
			}
			if( storedArmy->getTotal() != 1 ) {
				throw string("unexpected stored army after returning biplanes");
			}
			if( storedArmy->getSoldiers(6) != 1 ) {
				throw string("unexpected stored army number of biplanes after returning biplanes");
			}
		}
		else if( start_epoch == 5 && selected_island == 0 ) {
			Sector *start_sector = map->getSector(sx, sy);
			if( !start_sector->canBuild(BUILDING_MINE) ) {
				throw string("can't build mine");
			}
			start_sector->buildBuilding(BUILDING_MINE);
			if( !start_sector->canBuild(BUILDING_FACTORY) ) {
				throw string("can't build factory");
			}
			start_sector->buildBuilding(BUILDING_FACTORY);
			if( !start_sector->canBuild(BUILDING_LAB) ) {
				throw string("can't build lab");
			}
			start_sector->buildBuilding(BUILDING_LAB);
			for(int i=0;i<N_ID;i++) {
				while( start_sector->canMine((Id)i) && start_sector->anyElements((Id)i) ) {
					start_sector->mineElement(human_player, (Id)i);
				}
			}
			// first test with disallow nukes pref
			pref_disallow_nukes = true;
			Design *design = start_sector->canResearch(Invention::WEAPON, 8);
			if( design != NULL ) {
				throw string("didn't expect to design nuke when disallow nukes is on");
			}
			pref_disallow_nukes = false;
			design = start_sector->canResearch(Invention::WEAPON, 8);
			if( design == NULL ) {
				throw string("can't design nuke");
			}
			playingGameState->setCurrentDesign(sx, sy, design);
			start_sector->invent(human_player);
			if( !start_sector->canBuildDesign(design) ) {
				throw string("can't build nuke");
			}
			playingGameState->setCurrentManufacture(sx, sy, design);
			start_sector->buildDesign();
			if( !playingGameState->assembleArmy(sx, sy, 8, 1) ) {
				throw string("can't assemble nuke");
			}
			if( !playingGameState->nukeSector(sx, sy, sx+1, sy) ) {
				throw string("can't nuke sector");
			}
			// build another one
			if( !start_sector->canBuildDesign(design) ) {
				throw string("can't build nuke");
			}
			playingGameState->setCurrentManufacture(sx, sy, design);
			start_sector->buildDesign();
			if( !playingGameState->assembleArmy(sx, sy, 8, 1) ) {
				throw string("can't assemble nuke");
			}
			// shouldn't be able to nuke again
			if( playingGameState->nukeSector(sx, sy, sx+1, sy) ) {
				throw string("didn't expect to nuke sector again");
			}
		}
		else if( start_epoch == 9 && selected_island == 0 ) {
			// test AI behaviour
			Sector *sector = map->getSector(ex, ey);
			sector->cheat(human_player);
			// make sure we don't mine when no unmined elements remaining
			players[sector->getPlayer()]->doAIUpdate(human_player, playingGameState);
			Id element = MOONLITE; // should be a non-gatherable, that on its own can't be used to invent something on this island
			for(int i=0;i<N_ID;i++) {
				if( sector->getMiners((Id)i) > 0 ) {
					throw string("didn't expect miners when sector is used up");
				}
			}
			// now check we mine if there is some element present
			sector->setElements(element, 5*element_multiplier_c);
			players[sector->getPlayer()]->doAIUpdate(human_player, playingGameState);
			if( sector->getMiners(element) == 0 ) {
				throw string("didn't assign miners");
			}
			// now use up the element stocks
			for(int i=0;i<N_ID;i++) {
				int n = 0, fraction = 0;
				sector->getElementStocks(&n, &fraction, (Id)i);
				sector->reduceElementStocks((Id)i, n*element_multiplier_c+fraction);
				sector->getElementStocks(&n, &fraction, (Id)i);
				if( n > 0 || fraction > 0 ) {
					throw string("didn't get rid of all elements");
				}
			}
			// need to reset any design or manufacturer
			sector->setCurrentDesign(NULL);
			sector->setCurrentManufacture(NULL);
			// now check we trash the designs, and check we can't research them again
			players[sector->getPlayer()]->doAIUpdate(human_player, playingGameState);
			for(int i=0;i<n_epochs_c;i++) {
				if( sector->inventionKnown(Invention::WEAPON, i) )
					throw string("didn't expect invention to still be known");
				else if( sector->canResearch(Invention::WEAPON, i) )
					throw string("didn't expect to be able to research invention again");
				if( sector->inventionKnown(Invention::DEFENCE, i) )
					throw string("didn't expect invention to still be known");
				else if( sector->canResearch(Invention::DEFENCE, i) )
					throw string("didn't expect to be able to research invention again");
				if( sector->inventionKnown(Invention::SHIELD, i) )
					throw string("didn't expect invention to still be known");
				else if( sector->canResearch(Invention::SHIELD, i) )
					throw string("didn't expect to be able to research invention again");
			}
			// should have set miners, in case we can research something again
			if( sector->usedUp() ) {
				throw string("sector is deemed used up even though elements remain");
			}
			else if( sector->getMiners(element) == 0 ) {
				throw string("didn't assign miners after trashing designs");
			}
			// but if we have 6 or more stocks, then no need to mine after all
			sector->reduceElementStocks(element, -6*element_multiplier_c);
			int saved_population = sector->getPopulation();
			players[sector->getPlayer()]->doAIUpdate(human_player, playingGameState);
			if( !sector->usedUp() ) {
				throw string("sector isn't deemed used up");
			}
			else if( sector->getMiners(element) > 0 ) {
				throw string("shouldn't assign miners even though some remaining, as we already have stocks and the sector is deemed used up");
			}
			// check that the AI sent soldiers to a new empty sector
			int total = 0, squares = 0;
			bool found = testFindSoldiersBuildingNewTower(sector, &total, &squares);
			if( !found ) {
				throw string("didn't send soldiers to build another tower");
			}
			else if( squares != 1 ) {
				throw string("didn't send soldiers to expected number of squares");
			}
			else if( total != saved_population-1 ) {
				throw string("didn't send expected number of soldiers");
			}
			else if( sector->getPopulation() != 1 ) {
				throw string("remaining population not as expected");
			}
			// now add some more men, but make sure we don't evacuate if it means leaving weapons behind
			sector->setPopulation(40);
			sector->getStoredArmy()->add(9, 6); // 6*8 needs 48 men
			players[sector->getPlayer()]->doAIUpdate(human_player, playingGameState);
			found = testFindSoldiersBuildingNewTower(sector, &total, &squares);
			// should be unchanged from above
			if( !found ) {
				throw string("didn't send soldiers to build another tower");
			}
			else if( squares != 1 ) {
				throw string("didn't send soldiers to expected number of squares");
			}
			else if( total != saved_population-1 ) {
				throw string("didn't send expected number of soldiers");
			}
			else if( sector->getPopulation() != 40 ) {
				throw string("remaining population not as expected");
			}
			else if( sector->getStoredArmy()->getSoldiers(9) != 6 ) {
				throw string("unexpected number of remaining soldiers");
			}
			// now set enough
			sector->setPopulation(50);
			players[sector->getPlayer()]->doAIUpdate(human_player, playingGameState);
			found = testFindSoldiersBuildingNewTower(sector, &total, &squares);
			// should be sent to the same square
			if( !found ) {
				throw string("didn't send soldiers to build another tower");
			}
			else if( squares != 1 ) {
				throw string("didn't send soldiers to expected number of squares");
			}
			else if( total != saved_population-1 + 6+1 ) {
				throw string("didn't send expected number of soldiers");
			}
			else if( sector->getPopulation() != 1 ) {
				throw string("remaining population not as expected");
			}
			else if( sector->getStoredArmy()->getSoldiers(9) != 0 ) {
				throw string("unexpected number of remaining soldiers");
			}
			// now if we have more than 375, should still send men even if it means leaving weapons behind
			sector->setPopulation(376);
			sector->getStoredArmy()->add(9, 50); // 50*8 needs 400 men
			players[sector->getPlayer()]->doAIUpdate(human_player, playingGameState);
			found = testFindSoldiersBuildingNewTower(sector, &total, &squares);
			// should be sent to the same square
			if( !found ) {
				throw string("didn't send soldiers to build another tower");
			}
			else if( squares != 1 ) {
				throw string("didn't send soldiers to expected number of squares");
			}
			else if( total != saved_population-1 + 6+1 + 46+7 ) {
				throw string("didn't send expected number of soldiers");
			}
			else if( sector->getPopulation() != 1 ) {
				throw string("remaining population not as expected");
			}
			else if( sector->getStoredArmy()->getSoldiers(9) != 4 ) {
				throw string("unexpected number of remaining soldiers");
			}

			// test player shutting down the sector
			Sector *start_sector = map->getSector(sx, sy);
			if( start_sector->canShutdown() ) {
				throw string("shouldn't be able to shutdown the sector yet");
			}
			if( !playingGameState->assembleArmyUnarmed(sx, sy, 1) ) {
				throw string("can't assemble unarmed");
			}
			if( !playingGameState->moveAssembledArmyTo(sx, sy, sx, sy) ) {
				throw string("can't move assembled army");
			}
			if( !start_sector->canShutdown() ) {
				throw string("can't shutdown the sector");
			}
			playingGameState->shutdown(sx, sy);
		}

		endIsland();
		updateGame(); // needed to dispose the gamestate

		setGameStateID(GAMESTATEID_PLACEMEN);
		updateGame(); // needed to dispose the gamestate
		nextIsland();
		if( selected_island == 0 ) {
			nextEpoch();
			if( start_epoch == 0 )
				break;
		}
	}

	// now test loading
	gameType = GAMETYPE_ALLISLANDS;
	newGame();
	if( n_men_store != getMenPerEpoch() ) {
		throw string("unexpected number of men");
	}
	if( !loadGame("_test_savegames/game_0.SAV") ) {
		throw string("failed to load game");
	}
	if( human_player != gamestate->getClientPlayer() ) {
		throw string("human_player doesn't match gamestate client_player");
	}
	if( human_player != 2 ) {
		throw string("didn't set human_player from saved game");
	}

	// now test loading saved states
	delete gamestate;
	gamestate = NULL;
	copyFile("_test_savegames/autosave_bad.sav", getApplicationFilename(autosave_filename, autosave_survive_uninstall));
#if defined(_WIN32) || defined(__linux) || (defined(__APPLE__) && defined(__MACH__))
	// ensure on a platform where access() is defined (it isn't available on AROS etc - we could write platform specific code, but not really worth it for now)
	if( access(getApplicationFilename(autosave_filename, autosave_survive_uninstall), 0) != 0 ) {
		throw string("failed to copy save state file");
	}
#endif
	if( loadState() ) {
		throw string("didn't expect to load bad state");
	}
	else if( gamestate != NULL ) {
		throw string("didn't expect to create a gamestate when loading bad state");
	}
#if defined(_WIN32) || defined(__linux) || (defined(__APPLE__) && defined(__MACH__))
	// ensure on a platform where access() is defined (it isn't available on AROS etc - we could write platform specific code, but not really worth it for now)
	else if( access(getApplicationFilename(autosave_filename, autosave_survive_uninstall), 0) == 0 ) {
		throw string("save state file should have been deleted");
	}
#endif

	copyFile("_test_savegames/autosave_tutorial.sav", getApplicationFilename(autosave_filename, autosave_survive_uninstall));
#if defined(_WIN32) || defined(__linux) || (defined(__APPLE__) && defined(__MACH__))
	// ensure on a platform where access() is defined (it isn't available on AROS etc - we could write platform specific code, but not really worth it for now)
	if( access(getApplicationFilename(autosave_filename, autosave_survive_uninstall), 0) != 0 ) {
		throw string("failed to copy save state file");
	}
#endif
	if( !loadState() ) {
		throw string("failed to load state");
	}
	else if( gamestate == NULL ) {
		throw string("failed to create new gamestate when loading state");
	}
	else if( gameStateID != GAMESTATEID_PLAYING ) {
		throw string("expected playinggamestate when loading state");
	}
	else if( tutorial == NULL ) {
		throw string("didn't create tutorial when loading state");
	}
#if defined(_WIN32) || defined(__linux) || (defined(__APPLE__) && defined(__MACH__))
	// ensure on a platform where access() is defined (it isn't available on AROS etc - we could write platform specific code, but not really worth it for now)
	else if( access(getApplicationFilename(autosave_filename, autosave_survive_uninstall), 0) == 0 ) {
		throw string("save state file should have been deleted");
	}
#endif

	delete gamestate;
	gamestate = NULL;
	delete tutorial;
	tutorial = NULL;
	copyFile("_test_savegames/autosave_biplanes.sav", getApplicationFilename(autosave_filename, autosave_survive_uninstall));
#if defined(_WIN32) || defined(__linux) || (defined(__APPLE__) && defined(__MACH__))
	// ensure on a platform where access() is defined (it isn't available on AROS etc - we could write platform specific code, but not really worth it for now)
	if( access(getApplicationFilename(autosave_filename, autosave_survive_uninstall), 0) != 0 ) {
		throw string("failed to copy save state file");
	}
#endif
	if( !loadState() ) {
		throw string("failed to load state");
	}
	else if( gamestate == NULL ) {
		throw string("failed to create new gamestate when loading state");
	}
	else if( gameStateID != GAMESTATEID_PLAYING ) {
		throw string("expected playinggamestate when loading state");
	}
	else if( tutorial != NULL ) {
		throw string("didn't expect tutorial when loading state");
	}
#if defined(_WIN32) || defined(__linux) || (defined(__APPLE__) && defined(__MACH__))
	// ensure on a platform where access() is defined (it isn't available on AROS etc - we could write platform specific code, but not really worth it for now)
	else if( access(getApplicationFilename(autosave_filename, autosave_survive_uninstall), 0) == 0 ) {
		throw string("save state file should have been deleted");
	}
#endif
	{
		if( !map->isSectorAt(1, 3) ) {
			throw string("no sector");
		}
		const Sector *sector = map->getSector(1, 3);
		if( sector->getArmy(0)->getSoldiers(6) != 4 ) {
			throw string("unexpected number of biplanes");
		}
		else if( sector->getArmy(0)->getTotal() != 4 ) {
			throw string("unexpected number of soldiers");
		}
		for(int i=1;i<4;i++) {
			if( sector->getArmy(i)->getTotal() != 0 ) {
				throw string("unexpected number of enemy soldiers");
			}
		}
	}
}

void Game::copyFile(const char *src, const char *dst) const {
	SDL_RWops *read_file = SDL_RWFromFile(src, "r");
	if( read_file == NULL ) {
		throw string("couldn't open test save state file");
	}
	SDL_RWops *save_file = SDL_RWFromFile(dst, "w");
	if( save_file == NULL ) {
		throw string("couldn't copy save state file");
	}
#if SDL_MAJOR_VERSION == 1
	// SDL 1 doesn't have a size parameter
	SDL_RWseek(read_file, 0, RW_SEEK_END);
	size_t size = (size_t)SDL_RWtell(read_file);
	SDL_RWseek(read_file, 0, RW_SEEK_SET);
#else
	size_t size = (size_t)read_file->size(read_file);
#endif
	char *buffer = new char[size+1];
	if( read_file->read(read_file, buffer, 1, size) == 0 ) {
		read_file->close(read_file);
		throw string("failed to read from test save state file");
	}
	save_file->write(save_file, buffer, 1, size);
	save_file->close(save_file);
	read_file->close(read_file);
	delete [] buffer;
}

void playGame(int n_args, char *args[]) {
    LOG("playGame()\n");

	Player::resetAllAlliances(); // need to reset for Android, where variables aren't reinitialised if the native app restarts!
	game_g = new Game();

#ifdef _DEBUG
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF );  // TEST!
	debugwindow = true;
#endif

	bool fullscreen = true;
#if defined(__amigaos4__) || defined(AROS) || defined(__MORPHOS__)
	fullscreen = false; // run in windowed mode due to reported performance problems in fullscreen mode on AmigaOS 4; also randomly hangs on AROS in fullscreen mode; also included MorphOS just to be safe
#endif
#ifdef _DEBUG
	fullscreen = false;
#endif
	//debugwindow = true;
	//fullscreen = false;

#if !defined(__ANDROID__)
    // n.b., crashes when run on Galaxy Nexus (even though fine in the emulator)
	for(int i=0;i<n_args;i++) {
		if( strcmp(args[i], "fullscreen") == 0 )
			fullscreen = true;
		else if( strcmp(args[i], "windowed") == 0 )
			fullscreen = false;
		else if( strcmp(args[i], "debugwindow") == 0 )
			debugwindow = true;
		else if( strcmp(args[i], "onemousebutton") == 0 )
			game_g->setOneMouseButton(true);
		else if( strcmp(args[i], "mobile_ui") == 0 )
			game_g->setMobileUI(true);
		else if( strcmp(args[i], "server") == 0 )
			game_g->setGameMode(GAMEMODE_MULTIPLAYER_SERVER);
		else if( strcmp(args[i], "client") == 0 )
			game_g->setGameMode(GAMEMODE_MULTIPLAYER_CLIENT);
	}
#endif

#ifdef WINRT
	// @TODO
#elif _WIN32
	if( debugwindow ) {
		AllocConsole();
        /*FILE *dummy = NULL;
        freopen_s(&dummy, "con", "w", stdout);*/
        FILE *dummy = freopen("con", "w", stdout);
		SetConsoleTitleA("DEBUG:");
	}
#endif

	initFolderPaths();
	initLogFile();

	// set random seed - recommended way to do it from http://stackoverflow.com/questions/322938/recommended-way-to-initialize-srand
	unsigned int seed = (unsigned int)time(NULL);
	//seed = 72638; // test
	LOG("set random seed to %d\n", seed);
	srand( seed );

	//bool run_tests = true;
	bool run_tests = false;

        /*if( access("data", 0)==0 ) {
	use_amigadata = true;
	}
	else if( access("Mega Lo Mania", 0)==0 ) {
	use_amigadata = false;
	}
	else {
	LOG("can't find data folder\n");
	return;
	}*/
	LOG("onemousebutton?: %d\n", game_g->isOneMouseButton());
	LOG("mobile_ui?: %d\n", game_g->isMobileUI());

	if( !run_tests ) {
		game_g->loadPrefs();
	}

	// init application
	if( !game_g->createApplication() ) {
		LOG("failed to init application\n");
	}
	// init sound
	else if( !initSound() ) {
		// don't fail, just warn
		LOG("Failed to initialise sound system\n");
	}

	LOG("successfully opened libraries\n");

	bool ok = true;
	if( !game_g->openScreen(fullscreen) ) {
		LOG("failed to open screen\n");
		ok = false;

#ifdef WINRT
		// @TODO
#elif _WIN32
		MessageBoxA(NULL, "Failed to open screen", "Error", MB_OK|MB_ICONEXCLAMATION);
#endif
	}

    int time_s = clock();
	game_g->drawProgress(0);

	if( ok && !game_g->loadImages() ) {
		LOG("failed to load images\n");
		ok = false;
#ifdef WINRT
		// @TODO
#elif _WIN32
		MessageBoxA(NULL, "Failed to load images", "Error", MB_OK|MB_ICONEXCLAMATION);
#endif
	}
	LOG("time taken to load images: %d\n", clock() - time_s);
	// loadImages takes progress up to 80%
	if( !ok ) {
		LOG("delete game %d\n", game_g);
		delete game_g;
		game_g = NULL;
		return;
	}

	// n.b., still need to load samples even if sound failed to initialise, as we want the Sample objects for the textual display
	if( !game_g->loadSamples() ) {
		// don't fail, just warn
		LOG("warning - failed to load samples\n");
		// no longer show message - no longer an error, as the default install won't have any samples!
/*#ifdef _WIN32
		MessageBoxA(NULL, "Failed to load all samples", "Warning", MB_OK|MB_ICONEXCLAMATION);
#endif*/
	}
	game_g->drawProgress(85);

	game_g->setupInventions();
	game_g->drawProgress(87);
	game_g->setupElements();
	game_g->drawProgress(89);
	Design::setupDesigns();
	game_g->drawProgress(90);
	if( !game_g->createMaps() ) {
		LOG("failed to create maps\n");
		LOG("delete game %d\n", game_g);
		delete game_g;
		game_g = NULL;
#ifdef WINRT
		// @TODO
#elif _WIN32
		MessageBoxA(NULL, "Failed to create maps", "Error", MB_OK|MB_ICONEXCLAMATION);
#endif
		return;
	}
	game_g->drawProgress(95);

	for(size_t i=0;i<TrackedObject::getNumTags();i++) {
		TrackedObject *to = TrackedObject::getTag(i);
		if( to != NULL && strcmp( to->getClass(), "CLASS_IMAGE" ) == 0 ) {
			Image *image = (Image *)to;
			if( !image->convertToDisplayFormat() ) {
				LOG("failed to convertToDisplayFormat\n");
				LOG("delete game %d\n", game_g);
				delete game_g;
				game_g = NULL;
#ifdef WINRT
				//@TODO
#elif _WIN32
				MessageBoxA(NULL, "Failed to create texture images", "Error", MB_OK|MB_ICONEXCLAMATION);
#endif
				return;
			}
		}
	}

	game_g->drawProgress(100);
    int time_taken = clock() - time_s;
	LOG("time taken to load data: %d\n", time_taken);

	char buffer[256] = "";
	sprintf(buffer, "Gigalomania, version %d.%d", majorVersion, minorVersion);
	game_g->getScreen()->setTitle(buffer);

    LOG("all done!\n");

	if( run_tests ) {
		game_g->runTests();
	}
	else {
		if( !game_g->loadState() ) {
			game_g->setCurrentMap();
			game_g->setGameStateID(GAMESTATEID_CHOOSEGAMETYPE);
			//setGameStateID(GAMESTATEID_CHOOSEPLAYER);
		}

		game_g->getApplication()->runMainLoop();
	}

	LOG("delete game %d\n", game_g);
	delete game_g;
	game_g = NULL;
}

bool Game::playerAlive(int player) const {
	/*int n_player_sectors = 0;
	int n_army = 0;
	for(int x=0;x<map_width_c;x++) {
	for(int y=0;y<map_height_c;y++) {
	Sector *sector = map->sectors[x][y];
	if( sector != NULL ) {
	if( player == sector->getActivePlayer() )
	n_player_sectors++;
	n_army += sector->getArmy(player)->getTotal();
	}
	}
	}
	return ( n_player_sectors > 0 || n_army > 0 );*/
	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			//Sector *sector = map->sectors[x][y];
			Sector *sector = map->getSector(x, y);
			if( sector != NULL ) {
				if( player == sector->getActivePlayer() )
					return true;
				else if( sector->getArmy(player)->any(true) )
					return true;
			}
		}
	}
	return false;
}

#if defined(__ANDROID__)

// JNI for Android

#include <jni.h>
#include <android/log.h>

// see http://wiki.libsdl.org/SDL_AndroidGetActivity

void launchUrl(string url) {
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: launch url: %s", url.c_str());
    // retrieve the JNI environment.
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    if (!env)
    {
        __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: can't find env");
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: obtained env");

    // retrieve the Java instance of the SDLActivity
    jobject activity = (jobject)SDL_AndroidGetActivity();
    if (!activity)
    {
        __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: can't find activity");
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: obtained activity");

    // find the Java class of the activity. It should be SDLActivity or a subclass of it.
    jclass clazz( env->GetObjectClass(activity) );
    if (!clazz)
    {
        __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: can't find class");
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: obtained class");

    // find the identifier of the method to call
    jmethodID method_id = env->GetMethodID(clazz, "launchUrl", "(Ljava/lang/String;)V");
    if (!method_id)
    {
        __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: can't find launchUrl method");
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: obtained method");

    // effectively call the Java method
	jstring str = env->NewStringUTF(url.c_str());
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: about to call static method");
    env->CallVoidMethod( activity, method_id, str );
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: called method");
    
    // clean up the local references.
    env->DeleteLocalRef(str);
    env->DeleteLocalRef(activity);
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: done");
}
#endif
