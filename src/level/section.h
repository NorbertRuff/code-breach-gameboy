#ifndef SECTION_H
#define SECTION_H

#include "level_manager.h"
#include "../player/player.h"

// Sections, gates and checkpoints (docs/LEVEL_DESIGN.md section 2): a gate closes each
// section but the last, and opens when the player touches it with the section's quota
// of fragments. The opened gate is the checkpoint for the next section.
uint8_t section_quota(void);
void section_start_position(uint16_t* x, uint8_t* y);
void sections_restart(void);
void sections_update(const Player* player);

#endif // SECTION_H
