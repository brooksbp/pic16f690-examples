#include <pic16f690.h>
#include <stdint.h>

static __code uint16_t __at(_CONFIG) configword1 =
	_INTOSC & _WDTE_OFF & _PWRTE_OFF & _MCLRE_OFF &
	_CP_OFF & _BOR_OFF & _IESO_OFF & _FCMEN_OFF;

void main(void)
{
	IRCF2 = 1; IRCF1 = 1; IRCF0 = 1;  // Fosc = 8 MHz
	SCS = 1;

	// Disable analog input mode
	ANSEL = 0x00;
	ANSELH = 0x00;

	// Configure PORT I/O pins as output
	TRISA = 0x00;
	TRISC = 0x00;
	TRISB = 0x00;

	PORTA = 0x00;
	PORTB = 0x00;
	PORTC = 0x00;

	for (;;) {
		RC2 = 1;
		RC2 = 0;
	}
}
