# pic16f690-examples

We'll use ```sdcc``` to compile code and ```pk2cmd``` to load it into flash program memory. See [Install tools](#install-tools) for details.

## Run a program

Wire up the PICkit 2 to the PIC16F690:

![PICkit 2](images/pickit2.jpg)

## System clock

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
