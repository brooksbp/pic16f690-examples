# pic16f690-examples

We'll use ```sdcc``` to compile code and ```pk2cmd``` to load it into flash program memory. See [Install tools](#install-tools) for details.

## Run a program

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

## GPIO

```
sdcc -mpic14 -p16f690 --stack-size 8 --use-non-free led.c
pk2cmd -PPIC16f690 -Fled.hex -M -A5.0 -T
```

## Install tools

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
