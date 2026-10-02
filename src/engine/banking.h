#ifndef BANKING_H
#define BANKING_H

#include <gb/gb.h>
#include <stdint.h>

// Assets made with "png2asset -b 255" live in a ROM bank chosen by autobank
// (BANKREF(name) in the generated .c). Code must map that bank in before it reads
// the data, and map the old bank back afterwards.
//
//   WITH_BANK(background, set_bkg_data(0, background_TILE_COUNT, background_tiles));
//
// Only use this from code that is NOT itself banked (no "#pragma bank" in the file):
// a banked function would switch its own code out from under itself.
// Data from two different assets can't be read in one WITH_BANK: they may be in
// different banks.
#define WITH_BANK(name, statement)              \
    do {                                        \
        uint8_t saved_bank_ = CURRENT_BANK;     \
        SWITCH_ROM(BANK(name));                 \
        statement;                              \
        SWITCH_ROM(saved_bank_);                \
    } while (0)

#endif // BANKING_H
