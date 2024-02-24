#include <gb/gb.h>
#include <stdio.h>

#include "../utils.h"
#include "intro_credits.h"

void show_intro_credits(uint16_t sleep)
{
	fade_in_screen(10);
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

	performant_delay(sleep);
	fade_out_screen(10);
	return;
}