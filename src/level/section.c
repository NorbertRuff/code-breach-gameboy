#include "section.h"
#include "../engine/camera.h"
#include "../sound/sound.h"

#define GATE_ROW_FRAMES 2       // an opening gate retracts one row every 2 frames
#define NOT_OPENING 0xFF

static uint8_t opening_gate = NOT_OPENING;   // index of the gate that is opening
static uint8_t frames_to_next_row;
static uint8_t touching_closed_gate;         // the denied sound plays once per touch

// World x just right of a gate: where the next section starts.
static uint16_t gate_right(const Gate* gate)
{
    return ((uint16_t)gate->column + 1) * TILE_SIZE;
}

// Fragments the current section asks for: its gate's quota, or in the last section
// (no gate) all of its fragments, which are bonus.
uint8_t section_quota(void)
{
    uint8_t section = current_level.section;
    if (section < current_level.gate_count) return current_level.gates[section].quota;
    return current_level.section_fragments[section];
}

// Where the player starts the current section: the level's start, or just past the gate
// that opened it, feet on the gate's bottom row.
void section_start_position(uint16_t* x, uint8_t* y)
{
    if (current_level.section == 0) {
        *x = current_level.player_start_x;
        *y = current_level.player_start_y;
        return;
    }
    const Gate* gate = &current_level.gates[current_level.section - 1];
    *x = gate_right(gate) + SPRITE_LEFT_W;
    *y = gate->bottom_row * TILE_SIZE + TILE_SIZE - 1;
}

// Sets the gates for a restart of the current section: the ones behind it open, the
// rest closed. The camera and the player can't go back past the last one opened.
void sections_restart(void)
{
    Gate* gate = current_level.gates;
    for (uint8_t i = 0; i < current_level.gate_count; i++, gate++) {
        gate->open_rows = (i < current_level.section) ? gate->bottom_row - LEVEL_MAP_TOP_ROW + 1 : 0;
    }
    opening_gate = NOT_OPENING;
    camera_lock_left(current_level.section ? gate_right(&current_level.gates[current_level.section - 1]) : 0);
}

// Is the body against the gate, from the left and not below it? Walking into a closed
// gate stops the body's right edge on the pixel just before it.
static uint8_t touches_gate(const Player* player, const Gate* gate)
{
    uint16_t right = TO_PIXELS(player->x) + SPRITE_RIGHT_W;
    return right + 1 == (uint16_t)gate->column * TILE_SIZE && (uint8_t)(player->y >> 8) <= gate->bottom_row;
}

// Touching the current section's gate with its quota collected opens it: the next
// section starts, the gate is its checkpoint, and the camera won't go back past it once
// it gets there. The gate then retracts one row every GATE_ROW_FRAMES frames. Without
// the quota, touching it buzzes once.
void sections_update(const Player* player)
{
    if (opening_gate != NOT_OPENING && --frames_to_next_row == 0) {
        frames_to_next_row = GATE_ROW_FRAMES;
        if (level_open_gate_row(&current_level.gates[opening_gate])) opening_gate = NOT_OPENING;
    }

    uint8_t section = current_level.section;
    if (section >= current_level.gate_count) return;
    Gate* gate = &current_level.gates[section];
    if (!touches_gate(player, gate)) {
        touching_closed_gate = 0;
        return;
    }
    // Too few fragments? A buzz when the touch starts, so the player knows why.
    if (current_level.section_collected[section] < gate->quota) {
        if (!touching_closed_gate) sound_play(EFFECT_DENIED);
        touching_closed_gate = 1;
        return;
    }

    current_level.section++;
    opening_gate = section;
    frames_to_next_row = 1;
    camera_lock_left(gate_right(gate));
    sound_play(EFFECT_GATE);
}
