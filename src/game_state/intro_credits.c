#include <gb/gb.h>
#include <stdio.h>

#include "../utils.h"
#include "intro_credits.h"

void show_intro_credits()
{
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
	printf("                    ");
	return;
}

void handle_intro_credits()
{
	fade_in_screen(10);
	show_intro_credits();
	performant_delay(10);

	for (uint8_t i = 0; i < 32; i++)
	{
		scroll_bkg(0, 4);
		performant_delay(5);
	}

	performant_delay(10);
	fade_out_screen(10);
	scroll_bkg(0, -128);
}