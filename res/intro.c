#include <stdio.h>
#include <gb/gb.h>
#include "intro.h"
#include "utils.h"
#include "splashscreen.h"

extern unsigned char title[];

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
	return;
}

// // Set the tiles for our background in vram
// set_bkg_data(0, splashscreen_tileset_size, splashscreen_tileset);
// set_bkg_tiles(0, 0, 20, 18, splashscreen_tilemap);

void init_splash_screen()
{
	set_bkg_data(0, splashscreen_tileset_size, splashscreen_tileset);
	set_bkg_tiles(splashscreen_tileset_X, splashscreen_tileset_Y, splashscreen_tileset_width, splashscreen_tileset_height, splashscreen_tilemap);
	fade_in(10);
	return;
}

void update_splash_screen(uint16_t *text_animation_counter)
{
	if (*text_animation_counter == 3000)
	{
		set_bkg_tiles(press_start_tileset_X, press_start_tileset_Y, press_start_tileset_width, press_start_tileset_height, press_start_blank_tilemap);
		(*text_animation_counter) = 0;
		return;
	}
	else if (*text_animation_counter == 500)
	{
		set_bkg_tiles(press_start_tileset_X, press_start_tileset_Y, press_start_tileset_width, press_start_tileset_height, press_start_tilemap);
		(*text_animation_counter) ++;
		return;
	}
	else 
	{
		(*text_animation_counter) ++;
		return;
	}

}