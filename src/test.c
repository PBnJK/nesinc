#include "test.h"

#include <stdio.h>

#include "cpu.h"
#include "rom.h"

#define XSTR(X) #X
#define STR(X) XSTR(X)

#define TEST_FN(N) static bool N(void)

#define RUN_TEST(F)                       \
	printf("* Running '%s'... ", STR(F)); \
	if( !F() ) {                          \
		printf("Bailing...");             \
		return false;                     \
	}                                     \
	printf("OK\n");

#define RUN_ROM(F)              \
	printf("\n");               \
	ROM rom;                    \
	romCreateFromFile(&rom, F); \
	CPU cpu;                    \
	cpuInitFromROM(&cpu, rom);  \
	cpuRun(&cpu);

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

#define BIN(V) ((V) ? 1 : 0)

static void _dumpCPU(CPU *cpu) {
	printf("\n*** CPU DUMP ***\n");
	printf("A=%02x X=%02x Y=%02x S=%02x\n\n", cpu->regA, cpu->regX, cpu->regY,
		   cpu->stack);
	printf("NV1BDIZC\n");
	printf("%x%x%x%x%x%x%x%x\n\n", BIN(cpu->status & ST_NEGATIVE),
		   BIN(cpu->status & ST_OVERFLOW), BIN(cpu->status & ST_UNUSED),
		   BIN(cpu->status & ST_BFLAG), BIN(cpu->status & ST_DECIMAL),
		   BIN(cpu->status & ST_INTR), BIN(cpu->status & ST_ZERO),
		   BIN(cpu->status & ST_CARRY));
}

static void _loadTestCode(CPU *cpu, const uint8_t *CODE, const uint16_t SIZE) {
	ROM rom;
	romCreateTestROM(&rom);

	cpuInitFromROM(cpu, rom);
	cpuLoad(cpu, CODE, SIZE);
}

TEST_FN(_adc) {
	TEST(4, 0xA9, 0x34, 0x69, 0x02);
	RET((cpu.regA == 0x36) && ((cpu.status & ST_OVERFLOW) == 0));
}

TEST_FN(_adcOverflow) {
	TEST(4, 0xA9, 0xFF, 0x69, 0x03);
	RET((cpu.regA == 0x02) && ((cpu.status & ST_CARRY) != 0));
}

TEST_FN(_and) {
	TEST(4, 0xA9, 0xFF, 0x29, 0x01);
	RET(cpu.regA == 0x01);
}

TEST_FN(_andZeroOut) {
	TEST(4, 0xA9, 0x55, 0x29, ~0x55);
	RET(cpu.regA == 0x00);
}

TEST_FN(_aslCFlag_set) {
	TEST(3, 0xA9, 0x7F, 0x0A);
	RET((cpu.regA == (0x7F << 1)) && ((cpu.status & ST_CARRY) == 0));
}

TEST_FN(_aslCFlag_nset) {
	TEST(3, 0xA9, 0x80, 0x0A);
	RET((cpu.regA == 0) && ((cpu.status & ST_CARRY) != 0));
}

TEST_FN(_lda) {
	TEST(2, 0xA9, 0x05);

	RET((cpu.regA == 0x05) && ((cpu.status & ST_ZERO) == 0) &&
		((cpu.status & ST_NEGATIVE) == 0));
}

TEST_FN(_ldaZFlag_set) {
	TEST(2, 0xA9, 0x00);
	RET((cpu.status & ST_ZERO) == ST_ZERO);
}

TEST_FN(_ldaZFlag_nset) {
	TEST(2, 0xA9, 0x12);
	RET((cpu.status & ST_ZERO) != ST_ZERO);
}

TEST_FN(_ldaNFlag_set) {
	TEST(2, 0xA9, 0x80);
	RET((cpu.status & ST_NEGATIVE) == ST_NEGATIVE);
}

TEST_FN(_ldaNFlag_nset) {
	TEST(2, 0xA9, 0x79);
	RET((cpu.status & ST_NEGATIVE) != ST_NEGATIVE);
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

TEST_FN(_nestest) {
	RUN_ROM("test/nestest.nes");
	return true;
}

TEST_FN(_blarggInstrTest) {
	RUN_ROM("test/blargg_instr_test.nes");
	return true;
}

bool testRun(void) {
	printf("\nStarting test run...\n");

	/*RUN_TEST(_adc);
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
	RUN_TEST(_nestest);*/

	RUN_TEST(_blarggInstrTest);

	printf("\nAll tests OK!!\n");

	return true;
}
