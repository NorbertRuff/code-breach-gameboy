#ifndef BANKING_H
#define BANKING_H

#include <gb/gb.h>
#include <stdint.h>

// Reads data that lives in another ROM bank: switch to the bank, run the statement,
// switch back. Only from bank 0 code. A statement with commas outside parentheses
// (like "uint8_t a, b;") must go into a function.
#define WITH_ROM_BANK(bank, statement)          \
    do {                                        \
        uint8_t saved_bank_ = CURRENT_BANK;     \
        SWITCH_ROM(bank);                       \
        statement;                              \
        SWITCH_ROM(saved_bank_);                \
    } while (0)

// Same for one asset, by name:
//   WITH_BANK(greybox, set_bkg_data(0, greybox_TILE_COUNT, greybox_tiles));
#define WITH_BANK(name, statement) WITH_ROM_BANK(BANK(name), statement)

#endif // BANKING_H
