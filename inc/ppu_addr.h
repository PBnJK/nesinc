#ifndef GUARD_NESINC_PPU_ADDRESS_REGISTER_H_
#define GUARD_NESINC_PPU_ADDRESS_REGISTER_H_

#include "common.h"

typedef struct _AddrReg {
	union {
		struct {
			uint8_t lo;
			uint8_t hi;
		};

		uint16_t full;
	} address;

	bool hiPtr;
} AddrReg;

void addrInit(AddrReg *addr);

void addrSet(AddrReg *addr, const uint16_t DATA);
uint16_t addrGet(AddrReg *addr);

void addrUpdate(AddrReg *addr, const uint8_t DATA);
void addrIncrement(AddrReg *addr, const uint8_t INCREMENT);

void addrResetLatch(AddrReg *addr);

#endif	// GUARD_NESINC_PPU_ADDRESS_REGISTER_H_
