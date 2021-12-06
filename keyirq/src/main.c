#include <gba.h>
#include <stdio.h>

enum {
	K_NONE = 0x03FF,
	K_A = K_NONE ^ 0x0001,
	K_B = K_NONE ^ 0x0002,
	K_AB = K_A & K_B,
};

void _setupAndA(void) {
	REG_KEYCNT = 0xC001;
}

void _setupAndB(void) {
	REG_KEYCNT = 0xC002;
}

void _setupAndAB(void) {
	REG_KEYCNT = 0xC003;
}

void _clear(void) {
	REG_KEYCNT = 0xC3FF;
}

void _noop(void) {
	__asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop");
}

struct Step {
	const char* text;
	void (*setup)(void);
	uint16_t check;
	bool irq;
} steps[] = {
	// Basic IRQ checks
	{ "Hold A...", _setupAndA, K_A, true },
	// Make sure the IRQ stays cleared (edge triggered)
	{ NULL, _noop, K_A, false },

	// Check if the IRQ fires if additional keys then get pressed
	{ "Hold A+B...", _noop, K_AB, true },
	// Check that adding an already held key to desired keys asserts a new IRQ
	{ NULL, _setupAndAB, K_AB, true },
	// Check that clearing a key from desired keys does not re-trigger the (still asserted) IRQ
	{ NULL, _setupAndB, K_AB, false },
	// Make the IRQ test an untriggerable IRQ to lower the line
	{ NULL, _clear, K_AB, false },
	// Check that reconfiguring the IRQ check to be what the user is already holding asserts a new IRQ
	{ NULL, _setupAndAB, K_AB, true },

	{ "Release everything...", _clear, K_NONE, false },
	// Make the user hold the IRQ-triggering keys before configuring the IRQ
	{ "Hold A...", _noop, K_A, false },
	// Check that reconfiguring the IRQ check to be what the user is already holding asserts a new IRQ
	{ NULL, _setupAndA, K_A, true },
	// Add an extra key to deassert the IRQ
	{ NULL, _setupAndAB, K_A, false },
	// Check that going back to the old key combination reasserts the IRQ
	{ NULL, _setupAndA, K_A, true },

	{ "Hold A+B...", _noop, K_AB, true },

	// Check that going back to the old key combination reasserts the IRQ
	{ "Release B, keep holding A..", _noop, K_A, true },
	{ 0 }
};

int main(void) {
	irqInit();
	consoleDemoInit();
	irqEnable(IRQ_VBLANK);

	int failed = 0;

	struct Step* step = &steps[0];

	puts("MAKE SURE TO KEEP HOLDING KEYS"
	     "UNLESS THE TEST SAYS NOT TO,\n"
	     "OTHERWISE THE TEST MAY FAIL");
	puts(step->text);
	step->setup();
	while (step->check) {
		VBlankIntrWait();
		if (REG_KEYINPUT == step->check) {
			uint16_t ime = REG_IME;
			REG_IME = 0;
			bool irq = !!(REG_IF & IRQ_KEYPAD);
			if (irq) {
				REG_IF |= IRQ_KEYPAD;
			}
			REG_IME = ime;

			if (irq == step->irq) {
				iprintf("PASS ");
			} else {
				iprintf("FAIL ");
				++failed;
			}
			++step;
			if (step->text) {
				iprintf("\n");
				puts(step->text);
			}
			if (step->setup) {
				step->setup();
			}
		}
	}
	puts("\nTest complete");
	if (failed) {
		BG_PALETTE[0] = 0x001A;
	} else {
		BG_PALETTE[0] = 0x0340;		
	}
	while (1) {
		VBlankIntrWait();
	}

	return 0;
}
