#include "test.h"

#include <stdio.h>
#include <string.h>

#include "cpu.h"
#include "ppu.h"
#include "rom.h"

#define XSTR(X) #X
#define STR(X) XSTR(X)

#define TEST_FN(N) static bool N(void)

#define RUN_TEST(F)                         \
	printf("* Running '%s'... \n", STR(F)); \
	if( !F() ) {                            \
		printf("Bailing...");               \
		return false;                       \
	}                                       \
	printf("OK\n");

#define RUN_ROM(F)              \
	printf("\n");               \
	ROM rom;                    \
	romCreateFromFile(&rom, F); \
	CPU cpu;                    \
	cpuInitFromROM(&cpu, rom);  \
	cpuRun(&cpu);

#define TEST_PPU \
	PPU ppu;     \
	ppuInitEmpty(&ppu)

#define TEST(S, ...)                                   \
	CPU cpu;                                           \
	const uint8_t PROG[(S) + 1] = {__VA_ARGS__, 0x00}; \
	_loadTestCode(&cpu, PROG, S);                      \
	cpuRun(&cpu)

#define RET(C)                                          \
	do {                                                \
		if( (C) ) {                                     \
			return true;                                \
		} else                                          \
			printf("Test failed!\n\nDumping CPU...\n"); \
		_dumpCPU(&cpu);                                 \
		return false;                                   \
	} while( false )

static void _dumpCPU(CPU *cpu) {
	printf("\n*** CPU DUMP ***\n");
	printf("A=%02x X=%02x Y=%02x S=%02x\n\n", cpu->regA, cpu->regX, cpu->regY,
		   cpu->stack);
	printf("NV1BDIZC\n");
	printf("%x%x%x%x%x%x%x%x\n\n", cpu->status.negative, cpu->status.overflow,
		   cpu->status.bFlag2, cpu->status.bFlag1, cpu->status.decimal,
		   cpu->status.interrupt, cpu->status.zero, cpu->status.carry);
}

static void _loadTestCode(CPU *cpu, const uint8_t *CODE, const uint16_t SIZE) {
	ROM rom;
	romCreateTestROM(&rom);

	cpuInitFromROM(cpu, rom);
	cpuLoad(cpu, CODE, SIZE);
}

TEST_FN(_adc) {
	TEST(4, 0xA9, 0x34, 0x69, 0x02);
	RET((cpu.regA == 0x36) && (cpu.status.overflow == 0));
}

TEST_FN(_adcOverflow) {
	TEST(4, 0xA9, 0xFF, 0x69, 0x03);
	RET((cpu.regA == 0x02) && (cpu.status.carry == 1));
}

TEST_FN(_and) {
	TEST(4, 0xA9, 0xFF, 0x29, 0x01);
	RET(cpu.regA == 0x01);
}

TEST_FN(_andZeroOut) {
	TEST(4, 0xA9, 0x55, 0x29, 0xAA);
	RET(cpu.regA == 0x00);
}

TEST_FN(_aslCFlag_set) {
	TEST(3, 0xA9, 0x7F, 0x0A);
	RET((cpu.regA == (0x7F << 1)) && (cpu.status.carry == 0));
}

TEST_FN(_aslCFlag_nset) {
	TEST(3, 0xA9, 0x80, 0x0A);
	RET((cpu.regA == 0) && (cpu.status.carry == 1));
}

TEST_FN(_lda) {
	TEST(2, 0xA9, 0x05);

	RET((cpu.regA == 0x05) && (cpu.status.zero == 0) &&
		(cpu.status.negative == 0));
}

TEST_FN(_ldaZFlag_set) {
	TEST(2, 0xA9, 0x00);
	RET(cpu.status.zero == 1);
}

TEST_FN(_ldaZFlag_nset) {
	TEST(2, 0xA9, 0x12);
	RET(cpu.status.zero == 0);
}

TEST_FN(_ldaNFlag_set) {
	TEST(2, 0xA9, 0x80);
	RET(cpu.status.negative == 1);
}

TEST_FN(_ldaNFlag_nset) {
	TEST(2, 0xA9, 0x79);
	RET(cpu.status.negative == 0);
}

TEST_FN(_ldaZP) {
	CPU cpu;
	const uint8_t PROG[3] = {0xA5, 0x10, 0x00};

	_loadTestCode(&cpu, PROG, 3);

	cpuWrite16(&cpu, 0x10, 0x55);

	cpuRun(&cpu);

	RET(cpu.regA == 0x55);
}

TEST_FN(_ldaZP_x) {
	CPU cpu;
	const uint8_t PROG[3] = {0xB5, 0x10, 0x00};

	_loadTestCode(&cpu, PROG, 3);

	cpuWrite16(&cpu, 0x12, 0x35);
	cpu.regX = 0x2;

	cpuRun(&cpu);

	RET(cpu.regA == 0x35);
}

TEST_FN(_ldaAbs) {
	CPU cpu;
	const uint8_t PROG[4] = {0xAD, 0xF2, 0x10, 0x00};

	_loadTestCode(&cpu, PROG, 4);

	cpuWrite(&cpu, 0x10F2, 0xFA);

	cpuRun(&cpu);

	RET(cpu.regA == 0xFA);
}

TEST_FN(_ldaAbs_x) {
	CPU cpu;
	const uint8_t PROG[4] = {0xBD, 0x08, 0x10, 0x00};

	_loadTestCode(&cpu, PROG, 4);

	cpuWrite(&cpu, 0x100A, 0xFA);
	cpu.regX = 0x02;

	cpuRun(&cpu);

	RET(cpu.regA == 0xFA);
}

TEST_FN(_ldaAbs_y) {
	CPU cpu;
	const uint8_t PROG[4] = {0xB9, 0x12, 0x10, 0x00};

	_loadTestCode(&cpu, PROG, 4);

	cpuWrite(&cpu, 0x1014, 0xFA);
	cpu.regY = 0x02;

	cpuRun(&cpu);

	RET(cpu.regA == 0xFA);
}

TEST_FN(_ldaInd_x) {
	CPU cpu;
	const uint8_t PROG[3] = {0xA1, 0x12, 0x00};

	_loadTestCode(&cpu, PROG, 3);

	cpuWrite16(&cpu, 0x0014, 0x0032);
	cpuWrite16(&cpu, 0x0032, 0x9999);
	cpu.regX = 0x02;

	cpuRun(&cpu);

	RET(cpu.regA == 0x99);
}

TEST_FN(_ldaInd_y) {
	CPU cpu;
	const uint8_t PROG[3] = {0xB1, 0x20, 0x00};

	_loadTestCode(&cpu, PROG, 3);

	cpuWrite16(&cpu, 0x0020, 0x0040);
	cpuWrite16(&cpu, 0x0044, 0x4545);
	cpu.regY = 0x04;

	cpuRun(&cpu);

	RET(cpu.regA == 0x45);
}

TEST_FN(_sbc) {
	TEST(5, 0xA9, 0xD3, 0x18, 0xE9, 0x01);
	RET(cpu.regA == 0xD1);
}

TEST_FN(_sbcZeroSubtractsOne) {
	TEST(5, 0xA9, 0xD3, 0x18, 0xE9, 0x00);
	RET(cpu.regA == 0xD2);
}

TEST_FN(_sbcUnderflow) {
	TEST(5, 0xA9, 0x00, 0x18, 0xE9, 0x02);
	RET(cpu.regA == 0xFD);
}

TEST_FN(_staZP) {
	TEST(4, 0xA9, 0x69, 0x85, 0x31);
	RET(cpuRead(&cpu, 0x0031) == 0x69);
}

TEST_FN(_staZP_x) { /* TODO */
	return true;
}

TEST_FN(_staAbs) { /* TODO */
	return true;
}

TEST_FN(_staAbs_x) { /* TODO */
	return true;
}

TEST_FN(_staAbs_y) { /* TODO */
	return true;
}

TEST_FN(_staInd_x) { /* TODO */
	return true;
}

TEST_FN(_staInd_y) { /* TODO */
	return true;
}

TEST_FN(_tax) {
	TEST(3, 0xA9, 0x52, 0xAA);
	RET(cpu.regX == 0x52);
}

TEST_FN(_inx) {
	TEST(4, 0xA9, 0x12, 0xAA, 0xE8);
	RET(cpu.regX == 0x13);
}

TEST_FN(_inxOverflow) {
	TEST(5, 0xA9, 0xFF, 0xAA, 0xE8, 0xE8);
	RET(cpu.regX == 0x01);
}

/* LDA, TAX, INX and BRK working together
 */
TEST_FN(_smallTest) {
	TEST(4, 0xA9, 0xC0, 0xAA, 0xE8);
	RET(cpu.regX == 0xC1);
}

TEST_FN(_ppuVramW) {
	TEST_PPU;

	ppuWriteAddr(&ppu, 0x23);
	ppuWriteAddr(&ppu, 0x05);
	ppuWrite(&ppu, 0x66);

	return (ppu.vram[0x0305] == 0x66);
}

TEST_FN(_ppuVramR) {
	TEST_PPU;

	ppuWriteControl(&ppu, 0);
	ppu.vram[0x0305] = 0x66;

	ppuWriteAddr(&ppu, 0x23);
	ppuWriteAddr(&ppu, 0x05);

	ppuRead(&ppu);

	return (ppu.addr.address.full == 0x2306) && (ppuRead(&ppu) == 0x66);
}

TEST_FN(_ppuVramR_crossPage) {
	TEST_PPU;

	ppuWriteControl(&ppu, 0);
	ppu.vram[0x01FF] = 0x66;
	ppu.vram[0x0200] = 0x77;

	ppuWriteAddr(&ppu, 0x21);
	ppuWriteAddr(&ppu, 0xFF);

	ppuRead(&ppu);

	return (ppuRead(&ppu) == 0x66) && (ppuRead(&ppu) == 0x77);
}

TEST_FN(_ppuVramR_32Step) {
	TEST_PPU;

	ppuWriteControl(&ppu, 4);
	ppu.vram[0x01FF] = 0x66;
	ppu.vram[0x01FF + 32] = 0x77;
	ppu.vram[0x01FF + 64] = 0x88;

	ppuWriteAddr(&ppu, 0x21);
	ppuWriteAddr(&ppu, 0xFF);

	ppuRead(&ppu);

	return (ppuRead(&ppu) == 0x66) && (ppuRead(&ppu) == 0x77) &&
		   (ppuRead(&ppu) == 0x88);
}

TEST_FN(_ppuVram_horizontalMirror) {
	TEST_PPU;

	ppuWriteAddr(&ppu, 0x24);
	ppuWriteAddr(&ppu, 0x05);

	ppuWrite(&ppu, 0x66);

	ppuWriteAddr(&ppu, 0x28);
	ppuWriteAddr(&ppu, 0x05);

	ppuWrite(&ppu, 0x77);

	ppuWriteAddr(&ppu, 0x20);
	ppuWriteAddr(&ppu, 0x05);

	ppuRead(&ppu);
	if( ppuRead(&ppu) != 0x66 ) {
		printf("Failed read 1\n");
		return false;
	}

	ppuWriteAddr(&ppu, 0x2C);
	ppuWriteAddr(&ppu, 0x05);

	ppuRead(&ppu);
	return (ppuRead(&ppu) == 0x77);
}

TEST_FN(_ppuVram_verticalMirror) {
	PPU ppu;
	ppuInitEmptyVertical(&ppu);

	ppuWriteAddr(&ppu, 0x20);
	ppuWriteAddr(&ppu, 0x05);

	ppuWrite(&ppu, 0x66);

	ppuWriteAddr(&ppu, 0x2C);
	ppuWriteAddr(&ppu, 0x05);

	ppuWrite(&ppu, 0x77);

	ppuWriteAddr(&ppu, 0x28);
	ppuWriteAddr(&ppu, 0x05);

	ppuRead(&ppu);
	if( ppuRead(&ppu) != 0x66 ) {
		printf("Failed read 1\n");
		return false;
	}

	ppuWriteAddr(&ppu, 0x24);
	ppuWriteAddr(&ppu, 0x05);

	ppuRead(&ppu);
	return (ppuRead(&ppu) == 0x77);
}

TEST_FN(_ppuVram_mirroring) {
	TEST_PPU;
	ppuWriteControl(&ppu, 0);
	ppu.vram[0x0305] = 0x66;

	ppuWriteAddr(&ppu, 0x63);
	ppuWriteAddr(&ppu, 0x05);

	ppuRead(&ppu);
	return (ppuRead(&ppu) == 0x66);
}

TEST_FN(_ppuStatusR_resetLatch) {
	TEST_PPU;
	ppu.vram[0x0305] = 0x66;

	ppuWriteAddr(&ppu, 0x21);
	ppuWriteAddr(&ppu, 0x23);
	ppuWriteAddr(&ppu, 0x05);

	ppuRead(&ppu);
	if( ppuRead(&ppu) == 0x66 ) {
		printf("Failed read 1\n");
		return false;
	}

	ppuReadStatus(&ppu);

	ppuWriteAddr(&ppu, 0x23);
	ppuWriteAddr(&ppu, 0x05);

	ppuRead(&ppu);
	return (ppuRead(&ppu) == 0x66);
}

TEST_FN(_ppuStatusR_resetVBlank) {
	TEST_PPU;
	ppu.status.vblankStarted = 1;

	const StatusReg STATUS = ppuReadStatus(&ppu);
	return (ppu.status.vblankStarted == 0) && (STATUS.vblankStarted == 1);
}

TEST_FN(_ppuOAM_RW) {
	TEST_PPU;

	ppuWriteOAMAddr(&ppu, 0x10);
	ppuWriteOAM(&ppu, 0x66);
	ppuWriteOAM(&ppu, 0x77);

	ppuWriteOAMAddr(&ppu, 0x10);
	if( ppuReadOAM(&ppu) != 0x66 ) {
		printf("Failed read 1\n");
		return false;
	}

	ppuWriteOAMAddr(&ppu, 0x11);
	return (ppuReadOAM(&ppu) == 0x77);
}

TEST_FN(_ppuOAM_DMA) {
	TEST_PPU;

	uint8_t data[256];
	memset(data, 0x66, 256);

	data[0] = 0x77;
	data[255] = 0x88;

	ppuWriteOAMAddr(&ppu, 0x10);
	ppuWriteOAMDMA(&ppu, data);

	ppuWriteOAMAddr(&ppu, 0x0F);
	if( ppuReadOAM(&ppu) != 0x88 ) {
		printf("Failed read 1\n");
		return false;
	}

	ppuWriteOAMAddr(&ppu, 0x10);
	if( ppuReadOAM(&ppu) != 0x77 ) {
		printf("Failed read 2\n");
		return false;
	}

	ppuWriteOAMAddr(&ppu, 0x11);
	return (ppuReadOAM(&ppu) == 0x66);
}

bool testRun(void) {
	printf("\nStarting test run...\n");

	printf("1. CPU Tests:\n");
	RUN_TEST(_adc);
	RUN_TEST(_adcOverflow);

	RUN_TEST(_and);
	RUN_TEST(_andZeroOut);

	RUN_TEST(_aslCFlag_set);
	RUN_TEST(_aslCFlag_nset);

	RUN_TEST(_inx);
	RUN_TEST(_inxOverflow);

	RUN_TEST(_lda);

	RUN_TEST(_ldaZFlag_set);
	RUN_TEST(_ldaZFlag_nset);

	RUN_TEST(_ldaNFlag_set);
	RUN_TEST(_ldaNFlag_nset);

	RUN_TEST(_ldaZP);
	RUN_TEST(_ldaZP_x);

	RUN_TEST(_ldaAbs);
	RUN_TEST(_ldaAbs_x);
	RUN_TEST(_ldaAbs_y);

	RUN_TEST(_ldaInd_x);
	RUN_TEST(_ldaInd_y);

	RUN_TEST(_sbc);
	RUN_TEST(_sbcZeroSubtractsOne);
	RUN_TEST(_sbcUnderflow);

	RUN_TEST(_staZP);
	RUN_TEST(_staZP_x);

	RUN_TEST(_staAbs);
	RUN_TEST(_staAbs_x);
	RUN_TEST(_staAbs_y);

	RUN_TEST(_staInd_x);
	RUN_TEST(_staInd_y);

	RUN_TEST(_tax);

	RUN_TEST(_smallTest);

	printf("2. PPU Tests:\n");

	RUN_TEST(_ppuVramW);

	RUN_TEST(_ppuVramR);
	RUN_TEST(_ppuVramR_crossPage);
	RUN_TEST(_ppuVramR_32Step);

	RUN_TEST(_ppuVram_horizontalMirror);
	RUN_TEST(_ppuVram_verticalMirror);
	RUN_TEST(_ppuVram_mirroring);

	RUN_TEST(_ppuStatusR_resetLatch);
	RUN_TEST(_ppuStatusR_resetVBlank);

	RUN_TEST(_ppuOAM_RW);
	RUN_TEST(_ppuOAM_DMA);

	printf("\nAll tests OK!!\n");

	return true;
}
