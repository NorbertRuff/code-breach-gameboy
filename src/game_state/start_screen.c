#include <gb/gb.h>
#include <stdio.h>

#include "../graphics/splashscreen.h"
#include "../engine/banking.h"
#include "../utils.h"
#include "start_screen.h"

void init_splash_screen()
{
	WITH_BANK(splashscreen, {
		set_bkg_data(0, splashscreen_tileset_size, splashscreen_tileset);
		set_bkg_tiles(splashscreen_tileset_X, splashscreen_tileset_Y, splashscreen_tileset_width, splashscreen_tileset_height, splashscreen_tilemap);
	});
	fade_in_screen(10);
}

void update_splash_screen(uint16_t *frame_counter)
{
	(*frame_counter)++;
	if (*frame_counter == PRESS_START_SHOWN_FRAMES)
	{
		WITH_BANK(splashscreen, set_bkg_tiles(press_start_tileset_X, press_start_tileset_Y, press_start_tileset_width, press_start_tileset_height, press_start_blank_tilemap));
	}
	else if (*frame_counter >= PRESS_START_CYCLE_FRAMES)
	{
		WITH_BANK(splashscreen, set_bkg_tiles(press_start_tileset_X, press_start_tileset_Y, press_start_tileset_width, press_start_tileset_height, press_start_tilemap));
		*frame_counter = 0;
	}
}