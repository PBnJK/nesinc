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

#define BFLAG1_BIT (uint8_t)(1 << 4)
#define BFLAG2_BIT (uint8_t)(1 << 5)
#define OVERFLOW_BIT (uint8_t)(1 << 6)
#define SIGN_BIT (uint8_t)(1 << 7)

typedef union _CPUStatus {
	struct {
		uint8_t carry : 1;
		uint8_t zero : 1;
		uint8_t interrupt : 1;
		uint8_t decimal : 1;
		uint8_t bFlag1 : 1;
		uint8_t bFlag2 : 1;
		uint8_t overflow : 1;
		uint8_t negative : 1;
	};

	uint8_t bits;
} CPUStatus;

typedef struct _CPU {
	uint8_t regA; /* Register A */
	uint8_t regX; /* Register X */
	uint8_t regY; /* Register Y */

	CPUStatus status;

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
