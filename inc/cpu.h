#ifndef GUARD_NESINC_CPU_H_
#define GUARD_NESINC_CPU_H_

#include "bus.h"
#include "common.h"

/* (From https://www.nesdev.org/wiki/Status_flags)
 * 7  bit  0
 * ---- ----
 * NV1B DIZC
 * ││││ ││││
 * ││││ │││└─ Carry
 * ││││ │|└── Zero
 * ││││ │└─── Interrupt Disable
 * ││││ └──── Decimal
 * ││|└────── (No CPU effect)
 * │|└─────── (No CPU effect; always pushed as 1)
 * |└──────── Overflow
 * └───────── Negative
 */

#define ST_CARRY (1 << 0)
#define ST_ZERO (1 << 1)
#define ST_INTR (1 << 2)
#define ST_DECIMAL (1 << 3)
#define ST_BFLAG (1 << 4)
#define ST_UNUSED (1 << 5)
#define ST_OVERFLOW (1 << 6)
#define ST_NEGATIVE (1 << 7)

typedef struct _CPU {
	uint8_t regA; /* Register A */
	uint8_t regX; /* Register X */
	uint8_t regY; /* Register Y */

	uint8_t status;

	uint8_t stack; /* Stack "pointer" ($1000-$10FF) */

	Bus bus;
} CPU;

void cpuReset(CPU *cpu);

void cpuInit(CPU *cpu, Bus bus);
void cpuInitFromROM(CPU *cpu, ROM rom);

uint8_t cpuRead(CPU *cpu, const uint16_t ADDRESS);
uint16_t cpuRead16(CPU *cpu, const uint16_t ADDRESS);

void cpuWrite(CPU *cpu, const uint16_t ADDRESS, const uint8_t VALUE);
void cpuWrite16(CPU *cpu, const uint16_t ADDRESS, const uint16_t VALUE);

void cpuLoad(CPU *cpu, const uint8_t *CODE, const uint16_t SIZE);
void cpuLoadAndRun(CPU *cpu, const uint8_t *CODE, const uint16_t SIZE);

void cpuRun(CPU *cpu);

#endif	// GUARD_NESINC_CPU_H_
