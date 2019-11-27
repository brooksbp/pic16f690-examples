#include <pic16f690.h>
#include <stdint.h>

static __code uint16_t __at(_CONFIG) config =
	_INTOSC & _WDTE_OFF & _PWRTE_OFF & _MCLRE_OFF &
	_CP_OFF & _BOR_OFF & _IESO_OFF & _FCMEN_OFF;

void main(void)
{
	IRCF2 = 1; IRCF1 = 1; IRCF0 = 1;  // Fosc = 8 MHz
	SCS = 1;
}
