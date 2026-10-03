#ifndef BANKING_H
#define BANKING_H

#include <gb/gb.h>
#include <stdint.h>

// Reads an asset that lives in another ROM bank: switch to its bank, run the
// statement, switch back. Only from bank 0 code, one asset at a time.
//   WITH_BANK(background, set_bkg_data(0, background_TILE_COUNT, background_tiles));
#define WITH_BANK(name, statement)              \
    do {                                        \
        uint8_t saved_bank_ = CURRENT_BANK;     \
        SWITCH_ROM(BANK(name));                 \
        statement;                              \
        SWITCH_ROM(saved_bank_);                \
    } while (0)

#endif // BANKING_H
