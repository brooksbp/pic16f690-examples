# pic16f690-examples

The [PIC16F690](https://microchip.com/wwwproducts/en/PIC16F690) is an 8-bit microcontroller from Microchip. Here are some examples:

  * [System clock](#system-clock) - configure the clock
  * [GPIO](#gpio) - toggle GPIO pins
  * [Timer with interrupt](#timer-with-interrupt) - configure a timer to interrupt the processor

[PIC16F631/677/685/687/689/690 datasheet](http://ww1.microchip.com/downloads/en/DeviceDoc/40001262F.pdf)

### Install tools

We'll use ```sdcc``` to compile code and ```pk2cmd``` to load it into flash program memory.

Build SDCC from source since Debian does not package a version of SDCC that supports pic14 and pic16 ports because of license requirements by Microchip.

```
sudo apt-get install bison flex libboost-dev g++ gputils texinfo zlib1g-dev automake autoconf-archive libtool
svn checkout https://svn.code.sf.net/p/sdcc/code/trunk sdcc-code
cd sdcc-code/sdcc
./configure
make -j
sudo make install
```

Build pk2cmd from source since there is no Debian package.

```
sudo apt-get install libusb-dev
wget http://ww1.microchip.com/downloads/en/DeviceDoc/pk2cmdv1.20LinuxMacSource.tar.gz
extract pk2cmdv1.20LinuxMacSource.tar.gz
cd pk2cmdv1.20LinuxMacSource
make linux
sudo make install
sudo cp /usr/share/pk2/PK2DeviceFile.dat /usr/local/bin
```

### Run a program

Wire up the PICkit 2 to the PIC16F690.

![](https://i.postimg.cc/wjCZbywj/IMG-1332.jpg)

**Note**: a decoupling capacitor should be placed between Vdd and Vss. We will add that later.

Write a program ([empty.c](empty.c)) in C:

```c
#include <pic16f690.h>

void main(void)
{
        // Does nothing!
}
```

Compile and run it!

```
$ sdcc -mpic14 -p16f690 --stack-size 8 --use-non-free empty.c
message: Using default linker script "/usr/share/gputils/lkr/16f690_g.lkr".

$ pk2cmd -PPIC16f690 -Fempty.hex -M -A5.0 -T
PICkit 2 Program Report
26-11-2019, 19:49:07
Device Type: PIC16F690

Program Succeeded.

Operation Succeeded
```

**Note**: ```--stack-size 8``` is used because the PIC16F690 can store 8 addresses in its stack:

![](https://i.postimg.cc/zvP8LD8H/stack.png)

## System clock

![](https://i.postimg.cc/QMjw3GZ2/system-clock.jpg)

Let's use the internal oscillator at 8 MHz *and* output it on the CLKOUT pin ([system-clock-intosc.c](system-clock-intosc.c)):

```c
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
```

FOSC<2:0> of the configuration word register is set to internal oscillator with CLKOUT (```_INTOSC```). The 8 MHz high frequency oscillator is configured via IRCF<2:0> of the OSCCON register. And, then the internal oscillator is configured by setting the SCS bit of the OSCCON register.

**Note**: take a couple minutes to read ```pic16f690.h```. It can be found here ```/usr/local/share/sdcc/non-free/include/pic14/pic16f690.h```.

**Investigate**: The datasheet is somewhat inconsistent about the difference between FOSC<2:0> and SCS. SCS appears to control the MUX that selects between external and internal oscillator, despite its definition, while functionality such as CLKOUT as configured in FOSC<2:0> still takes effect.

If we probe CLKOUT, we'll see a **2 MHz** square wave since CLKOUT is defined to be Fosc/4:

![](https://i.postimg.cc/BZYtd4HK/system-clock-intosc-scope1.png)

How long does it take to switch to 8 MHz?

![](https://i.postimg.cc/ZqsKXLyh/startup.png)

Holy smokes! **380 us** from power up!

## GPIO

The PIC16F690 has general purpose I/O pins which are grouped into three ports. TRISA, TRISB, and TRISC can be configured to specify whether a pin is input or output, and PORTA, PORTB, PORTC can be read to get the value of an input pin or written to set the value of an output pin.

**Note:** When using PORTA pins, ANSEL and ANSELH must be set to 0 to configure the pins as digital I/O. If the pin is configured as analog input, the digital I/O circuitry is disabled.

How fast can we toggle a GPIO pin?

[gpio1.c](gpio1.c)

```c
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
```

![](https://i.postimg.cc/JhJ0D8Zt/gpio1.png)

One loop iteration takes 6 instructions. Let's check the assembly (gpio1.asm):

```asm
_00106_DS_:
;       .line   27; "gpio1.c"   RC2 = 1;
        BANKSEL _PORTCbits
        BSF     _PORTCbits,2
;       .line   28; "gpio1.c"   RC2 = 0;
        BCF     _PORTCbits,2
        GOTO    _00106_DS_
```

If we look at section 2.2 Data Memory Organization of the PIC16F690 datasheet, we see that data memory is partitioned into four banks. The bank is selected by configuring RP<1:0> of the STATUS register. For example, to modify the OSCCON register, we must switch to Bank 1 first, and then write to address 0x8F.

BANKSEL is not an instruction, but an assembler directive. To see the complete disassembly, see the list file (gpio1.lst):

```asm
                                           _00106_DS_:
                                           ;    .line   27; "gpio1.c"   RC2 = 1;
0000e7   1283     bcf     0x03, 0x5             BANKSEL _PORTCbits
0000e8   1303     bcf     0x03, 0x6
0000e9   1507     bsf     0x07, 0x2             BSF     _PORTCbits,2
                                           ;    .line   28; "gpio1.c"   RC2 = 0;
0000ea   1107     bcf     0x07, 0x2             BCF     _PORTCbits,2
0000eb   28e7     goto    0x00e7                GOTO    _00106_DS_
```

Here we see that BANKSEL clears both RP bits in the STATUS register, which selects Bank 0. Notice the STATUS register is always at address 0x3 in each bank.

So... we don't need to switch banks every time the loop body executes... let's rewrite the loop in assembly [gpio2.c](gpio2.c):

```c
__asm
        banksel PORTC
loop:
        bsf PORTC, 2
        bcf PORTC, 2
        goto loop
__endasm ;
}
```

Now we should expect the body of the loop to take 4 cycles since BSF and BCF take 1 cycle each and GOTO takes 2 cycles:

![](https://i.postimg.cc/3xrJrpJS/gpio2.png)

With a couple extra NOPs we can get a 50% duty cycle square wave. However, it has a jitter of ~10ns and is ~1.1kHz faster than expected:

![](https://i.postimg.cc/901KL450/gpio3.png)

## Timer with interrupt

Timer0 is an 8-bit counter that can be configured to increment according to either an internal or external clock source. When the timer overflows from 0xFF to 0x00, an interrupt will be generated.

Example [timer-interrupt.c](timer-interrupt.c):

```c
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
```

![](https://i.postimg.cc/B65VvmgR/Capture.jpg)

From the list file we see the interrupt service routine located at address 0x4 in program memory with quite a few instructions for saving/restoring a some special function registers:

```asm
                                           ;    .line   8; "timer-interrupt.c"  void isr(void) __interrupt(0)
000004   00f2     movwf   0x72                  MOVWF   WSAVE
000005   0e03     swapf   0x03, 0x0             SWAPF   STATUS,W
000006   0183     clrf    0x03                  CLRF    STATUS
000007   00f1     movwf   0x71                  MOVWF   SSAVE
000008   080a     movf    0x0a, 0x0             MOVF    PCLATH,W
000009   018a     clrf    0x0a                  CLRF    PCLATH
00000a   00f0     movwf   0x70                  MOVWF   PSAVE
00000b   0804     movf    0x04, 0x0             MOVF    FSR,W
00000c   1283     bcf     0x03, 0x5             BANKSEL ___sdcc_saved_fsr
00000d   1303     bcf     0x03, 0x6
00000e   00ac     movwf   0x2c                  MOVWF   ___sdcc_saved_fsr
                                           ;    .line   10; "timer-interrupt.c" T0IF = 0;  // Clear timer interrupt flag.
00000f   1283     bcf     0x03, 0x5             BANKSEL _INTCONbits
000010   1303     bcf     0x03, 0x6
000011   110b     bcf     0x0b, 0x2             BCF     _INTCONbits,2
                                           ;    .line   12; "timer-interrupt.c" RC2 = 0;
000012   1107     bcf     0x07, 0x2             BCF     _PORTCbits,2
                                           ;    .line   13; "timer-interrupt.c" RC2 = 1;  // Set RC2 high for one cycle.
000013   1507     bsf     0x07, 0x2             BSF     _PORTCbits,2
                                           ;    .line   14; "timer-interrupt.c" RC2 = 0;
000014   1107     bcf     0x07, 0x2             BCF     _PORTCbits,2
                                           ;    .line   15; "timer-interrupt.c" }
000015   1283     bcf     0x03, 0x5             BANKSEL ___sdcc_saved_fsr
000016   1303     bcf     0x03, 0x6
000017   082c     movf    0x2c, 0x0             MOVF    ___sdcc_saved_fsr,W
000018   1283     bcf     0x03, 0x5             BANKSEL FSR
000019   1303     bcf     0x03, 0x6
00001a   0084     movwf   0x04                  MOVWF   FSR
00001b   0870     movf    0x70, 0x0             MOVF    PSAVE,W
00001c   008a     movwf   0x0a                  MOVWF   PCLATH
00001d   0183     clrf    0x03                  CLRF    STATUS
00001e   0e71     swapf   0x71, 0x0             SWAPF   SSAVE,W
00001f   0083     movwf   0x03                  MOVWF   STATUS
000020   0ef2     swapf   0x72, 0x1             SWAPF   WSAVE,F
000021   0e72     swapf   0x72, 0x0             SWAPF   WSAVE,W
                                           END_OF_INTERRUPT:
000022   0009     retfie                        RETFIE
```
