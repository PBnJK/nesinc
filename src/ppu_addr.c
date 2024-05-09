#include "ppu_addr.h"

#include <stdio.h>

void addrInit(AddrReg *addr) {
	addr->address.hi = 0;
	addr->address.lo = 0;

	addr->hiPtr = true;
}

void addrSet(AddrReg *addr, const uint16_t DATA) {
	addr->address.hi = (uint8_t)(DATA >> 8);
	addr->address.lo = (uint8_t)(DATA & 0xFF);
}

uint16_t addrGet(AddrReg *addr) {
	return addr->address.full;
}

void addrUpdate(AddrReg *addr, const uint8_t DATA) {
	if( addr->hiPtr ) {
		addr->address.hi = DATA;
	} else {
		addr->address.lo = DATA;
	}

	const uint16_t VALUE = addrGet(addr);
	if( VALUE > 0x3FFF ) {
		addrSet(addr, VALUE & 0x3FFF);
	}

	addr->hiPtr = !addr->hiPtr;
}

void addrIncrement(AddrReg *addr, const uint8_t INCREMENT) {
	const uint8_t LO = addr->address.lo;
	addr->address.lo += INCREMENT;

	if( LO > addr->address.lo ) {
		++addr->address.hi;
	}

	const uint16_t VALUE = addrGet(addr);
	if( VALUE > 0x3FFF ) {
		addrSet(addr, VALUE & 0x3FFF);
	}
	
	printf("+ + Address is now %04X\n", addr->address.full);
}

void addrResetLatch(AddrReg *addr) {
	addr->hiPtr = true;
}
