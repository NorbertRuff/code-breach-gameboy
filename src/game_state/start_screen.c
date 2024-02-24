#include <gb/gb.h>
#include <stdio.h>

#include "../../res/splashscreen.h"
#include "../utils.h"
#include "start_screen.h"

void init_splash_screen()
{
	set_bkg_data(0, splashscreen_tileset_size, splashscreen_tileset);
	set_bkg_tiles(splashscreen_tileset_X, splashscreen_tileset_Y, splashscreen_tileset_width, splashscreen_tileset_height, splashscreen_tilemap);
	fade_in_screen(10);
}

void update_splash_screen(uint16_t *text_animation_counter)
{
	if (*text_animation_counter == 8000)
	{
		set_bkg_tiles(press_start_tileset_X, press_start_tileset_Y, press_start_tileset_width, press_start_tileset_height, press_start_blank_tilemap);
		(*text_animation_counter) = 0;
		return;
	}
	else if (*text_animation_counter == 4000)
	{
		set_bkg_tiles(press_start_tileset_X, press_start_tileset_Y, press_start_tileset_width, press_start_tileset_height, press_start_tilemap);
		(*text_animation_counter)++;
		return;
	}
	else
	{
		(*text_animation_counter)++;
		return;
	}
}