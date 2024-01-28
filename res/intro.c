#include <stdio.h>
#include <gb/gb.h>
#include "intro.h"
#include "utils.h"
#include "splashscreen.h"
#include "blank_tiles.h"

void intro_credits(uint16_t time)
{
	fade_in(10);
	printf("                    ");
	printf("                    ");
	printf("                    ");
	printf("    Developed By    ");
	printf("                    ");
	printf("                    ");
	printf("                    ");
	printf("    Norbert Ruff    ");
	printf("                    ");
	printf("                    ");
	printf("                    ");
	printf("       v1.0.0       ");
	printf("                    ");
	printf("                    ");
	printf("                    ");
	printf("                    ");
	printf("        2024        ");

	performant_delay(time);
	fade_out(10);
}


// // Set the tiles for our background in vram
// set_bkg_data(0, splashscreen_tileset_size, splashscreen_tileset);
// set_bkg_tiles(0, 0, 20, 18, splashscreen_tilemap);

void init_splash_screen(uint8_t *game_state, uint16_t *text_animation_timer)
{

	*text_animation_timer = ANIMATION_CYCLE_TIME;
	*game_state = TITLE_SCREEN_STATE;
	set_bkg_data(0, splashscreen_tileset_size, splashscreen_tileset);
	set_bkg_tiles(splashscreen_tileset_X, splashscreen_tileset_Y, splashscreen_tileset_width, splashscreen_tileset_height, splashscreen_tilemap);
	fade_in(10);
}
