// Cartridge RAM persistence test. Built by "make savetest" into obj/savetest.gb, with
// the same cartridge type as the game (MBC5 + RAM + battery), and into
// savetest_mbc1.gb and savetest_mbc3.gb with those mappers (MBC set by the Makefile).
//
// Each boot adds 1 to a counter in cartridge RAM. On a flashcart, power the Game Boy
// off and on a few times: if the count goes up, battery saves work. A clears the save.
// The globals below are read by tests/test_savetest.py through the .sym file.

#include <gb/gb.h>
#include <stdint.h>
#include <stdio.h>

// Cartridge RAM bank 0 starts at 0xA000. Layout: "APX1", boot count (16-bit,
// little endian), checksum.
#define SRAM ((volatile uint8_t *)0xA000)
#define SAVE_SIZE 7

#ifndef MBC
#define MBC 5   // the mapper in the header, shown on screen
#endif

static const uint8_t magic[4] = { 'A', 'P', 'X', '1' };

uint8_t  save_found;   // 1 if a valid save was there at boot
uint16_t boot_count;   // count after this boot
uint8_t  write_ok;     // 1 if the new count read back correctly

static uint8_t checksum(void)
{
    uint8_t i, sum = 0xA5;
    for (i = 0; i < SAVE_SIZE - 1; i++) sum ^= SRAM[i];
    return sum;
}

static void write_save(uint16_t count)
{
    uint8_t i;
    for (i = 0; i < 4; i++) SRAM[i] = magic[i];
    SRAM[4] = (uint8_t)count;
    SRAM[5] = (uint8_t)(count >> 8);
    SRAM[6] = checksum();
}

static uint8_t save_is_valid(void)
{
    uint8_t i;
    for (i = 0; i < 4; i++) {
        if (SRAM[i] != magic[i]) return 0;
    }
    return SRAM[6] == checksum();
}

void main(void)
{
    uint16_t read_back;

    ENABLE_RAM;
    SWITCH_RAM(0);
    save_found = save_is_valid();
    boot_count = save_found ? (uint16_t)(SRAM[4] | (SRAM[5] << 8)) + 1 : 1;
    write_save(boot_count);
    read_back = (uint16_t)(SRAM[4] | (SRAM[5] << 8));
    write_ok = (read_back == boot_count) && save_is_valid();
    DISABLE_RAM;

    printf("APEXLAB SAVE TEST\nCARTRIDGE: MBC%u\n\n", (uint16_t)MBC);
    printf(save_found ? "SAVE FOUND\n" : "NO SAVE: NEW ONE\n");
    printf("BOOT COUNT: %u\n", boot_count);
    printf(write_ok ? "WRITE: OK\n" : "WRITE: FAILED\n");
    printf("\nPOWER OFF AND ON.\nTHE COUNT SHOULD\nGO UP BY 1.\n\nA: CLEAR THE SAVE\n");

    while (1) {
        if (joypad() & J_A) {
            ENABLE_RAM;
            SRAM[0] = 0;
            DISABLE_RAM;
            printf("\nCLEARED. POWER\nOFF AND ON.\n");
            waitpadup();
        }
        vsync();
    }
}
