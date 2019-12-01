#include <pic16f690.h>
#include <stdint.h>

static __code uint16_t __at(_CONFIG) configword1 =
	_INTOSC & _WDTE_OFF & _PWRTE_OFF & _MCLRE_OFF &
	_CP_OFF & _BOR_OFF & _IESO_OFF & _FCMEN_OFF;

void isr(void) __interrupt(0)
{
	T0IF = 0;  // Clear timer interrupt flag.

	RC2 = 0;
	RC2 = 1;   // Set RC2 high for one cycle.
	RC2 = 0;
}

void main(void)
{
	IRCF2 = 1; IRCF1 = 1; IRCF0 = 1;  // Fosc = 8 MHz
	SCS = 1;

	TRISC = 0x00;  // Configure PORTC pins as digital output.
	PORTC = 0x00;  // Output logic zero on PORTC pins.

	T0CS = 0;      // Use the internal instruction cycle clock (Fosc/4) as TMR0 clock source.
	PSA = 1;       // Assign the prescaler to the WDT so that it doesn't affect TMR0.

	INTCON = 0;    // Disable all interrupt enables and flags.
	GIE = 1;       // Global interrupt enable.
	T0IE = 1;      // Enable Timer0 interrupt.

	TMR0 = 0;      // Reset counter to 0.

	for (;;) {
		// Do nothing!
	}
}
