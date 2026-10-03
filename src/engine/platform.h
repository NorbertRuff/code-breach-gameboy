#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

// 1 on Game Boy Color (or GBA), 0 on the original Game Boy.
extern uint8_t is_cgb;

// Detects the hardware and prepares GBC colors. Call once at boot.
void platform_init(void);

#endif // PLATFORM_H
