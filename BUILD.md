# Building the CNC code

The code for the controller and the hub can be built from:
* AVR Studio using the project supplied
* Linux or WSL
* Windows in powershell
* Docker using the Dockerfile

## With AVR studio
Simply open the solution with the studio, and hit F7. 
Debug and release work, and debugging is possible.
Note: The studio build will not create a CRC HEX file, so the CRC flash check cannot be used.

## With Linux or WSL
For Ubuntu/Debian Linux, you can use the standard avr-gcc package:
 $ sudo apt install srecord gcc-avr avr-libc

If you do so, you should also install the pack for the tiny.
 $  sudo wget https://packs.download.microchip.com/Microchip.ATtiny_DFP.3.1.260.atpack -O /tmp/dfp.zip \
  && mkdir -p /opt/ATtiny_DFP.2.0.368 \
  && cd /opt/ATtiny_DFP.2.0.368 \
  && unzip /tmp/dfp.zip

Alternatively, install the Linux compiler from Zak:
 $ sudo wget https://github.com/ZakKemble/avr-gcc-build/releases/download/v14.1.0-1/avr-gcc-14.1.0-x64-linux.tar.bz2 -O /tmp/avr-gcc.tar.xz \
  && cd /opt \
  && tar xjvf /tmp/avr-gcc.tar.xz \
  && rm -f /tmp/avr-gcc.tar.xz \
  && cd /opt \
  && ln -s /opt/avr-gcc-* /opt/avr-gcc \
  && echo 'PATH=$PATH:/opt/avr-gcc/bin' >> /etc/bash.bashrc

## Using the dockerfile
The Dockerfile will build a cnc_pneumatic builder image.
In Linux, the script ./build.bash will take care of building the image and the code.
To use the toolchain manually, invoke the script with shell:
 $ ./build.bash shell
You're in a Docker container image. Hit CTRL-D or exit to get out.
In Windows, you can use the Docker Desktop and build from powershell. 

## Using Windows powershell
Assuming AVR Studio is installed, it should build fine. Alternatively, you can also install Zak windows package.
