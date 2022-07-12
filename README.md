# Improved Airtag Glitcher
_This Repo contains all docs and scripts needed to glitch & dump an Airtag._ 

_It is a fork [this](https://github.com/stacksmashing/airtag-glitcher) popular repo by Stacksmashing - i changed some things in the setup to be more understandable and code to be prettier, fixed some bugs and provided some docs for it so hardware hacking beginners know where to start._

**Before you start** be sure to **give Stacksmashing´s Devcon 29 talk a [watch](https://www.youtube.com/watch?v=paxErRRsrTU).**

It might also be useful to get familiar with all things provided in the "Useful Resources" section of this Repository.

## Hardware Setup
### What you need:
* Raspberry Pi Pico
* Airtag(s) (because you can ruin the easily when soldering)
* SWD Programmer (tested with J-Link)
* Small (fast switching) N-Channel-Mosfet 
* (Self-built) Level Converter 1.8V --> 3.3V
* Very thin wire & good soldering iron
* Oscilloscope *or a lot of time*

#### Wiring Diagram
![Wiring Diagram](https://github.com/ensingerphilipp/airtag-glitch-improved/blob/main/assets/wiring.jpg?raw=true)
_Every ground should in the end always be connected to any gnd pin on the Raspberry Pi Pico_

_This Diagram uses **Colin o Flynn´s Pin Definitions** for the airtag: Link in Useful Resources_

### Building the Setup
First you need to connect the Hardware for the glitch. **This might take some time.**

 1. Open up the Airtag without destroying it -> See guides on YouTube
 2. Build a uni-directional level shifter shifting from 1.8V to 3.3V according to the diagram above
 3. Connect / Solder the parts according to the diagram and test if everything works as intended

### Useful Notes:

#### Power delivery to Airtag
The Airtag is being powered by our Raspberry Pi Pico. If you take a quick look at the Airtag it has two distinct positive power connections that both need to be powered --> you have to solder a bridge between the contacts as they are not connected to each other.  

#### Soldering Airtag test points
Be extra careful, use very thin wires and max heat of 270°C to reduce chance of damaging the test pads (which fall of easily). 

#### Level Shifter #1
Due to the way the level shifter is constructed the Raspberry Pi Pico will receive **3.3 volts when** the Airtag is **powered down** and **0.0 volts when** the Airtag is **powered up** (1.8V are being sensed at NRF_VDD test point). If you change characteristics of the level shifter or use a prebuilt one you have to change and rebuild the Pico Firmware at
>while(gpio_get(IN_NRF_VDD));

and change it to
>while(!gpio_get(IN_NRF_VDD));

#### Level Shifter #2
I used a potentiometer as resistor in the level shifter. It was set in a way to behave like cmos logic.
> https://learn.sparkfun.com/tutorials/logic-levels/all

If you have a CMOS Logic anything over *0.606 x Nominal Voltage* is considered *HIGH* = 1
So i set the potentiometer to *activate the transistor* when it receives > ~ *1.09 Volts*  (0.606 x 1.8V).
*Why is this important?*
It is not, but setting it up the same way will reduce the time you need for adjusting the delay in the jupyter notebook (especially if you have no oscilloscope) as the point in time where the trigger is activated will vary greatly depending on when the level Shifter considers the Airtag to be powered on (*HIGH*).

#### N-Channel-Mosfet
I used the BSS123 Mosfet because i had that laying around
> https://www.onsemi.com/pdf/datasheet/bss123-d.pdf

If you decide to use another Mosfet - choose one that has similarly fast Turn–On Delay Time and Turn–On Rise Time to 
avoid having to mess a lot with the Delay specified in the Jupyter Notebook.

#### Raspberry  Pi Pico Pinout
![Raspberry Pi Pico Pinout](https://www.elektronik-kompendium.de/sites/raspberry-pi/bilder/raspberry-pi-pico-gpio.png)

#### Close Up Photo of Airtag soldering
![Close Up Photo of Airtag soldering](https://github.com/ensingerphilipp/airtag-glitch-improved/blob/main/assets/airtag_closeup_soldered.jpeg?raw=true)

#### Completed (messy) prototype setup
![setup_1](https://github.com/ensingerphilipp/airtag-glitch-improved/blob/main/assets/setup_1.jpeg?raw=true)
![setup_2](https://github.com/ensingerphilipp/airtag-glitch-improved/blob/main/assets/setup_2.jpeg?raw=true)

## Software Setup

### Dependencies
* some linux distro (might work on Windows with some changes)
* python 3.8+
* openocd for checking debug + dumping
* nrfjprog (J-Link only) for reset + flashing <-- might work with openocd as well
* minicom for serial monitor
* telnet enabled for dumping airtag

### Overview

Software consists of 2 Parts:

 1. Binary / Build yourself for Raspberry Pi Pico
	 Used to do the actual glitching on the airtag and manage delay and pulse length
 2. Jupyter Notebook
	 Script running on the computer that sends Commands to Raspberry Pi Pico over Serial connection
	 and checks with openocd if the glitch was successful. 

### Building the Binary
If you can not use  the pre-built binaries or need to change code:
 1. Get and install pico-sdk [here](https://github.com/raspberrypi/pico-sdk)
 2. Build:
 ```
export PICO_SDK_PATH=...
mkdir build
cd build
cmake ..
make
```

Otherwise you can just use **built_binary/pico_firmware_current.uf2** for the Raspberry

To flash the Binary, longpress and hold the _BOOTSEL_ button on the Raspberry Pi Pico and connect it via USB.
Copy the built .uf2 Binary to the RPI-RP2 Mass Storage Device.

### Setting up the Python environment

 1. Create & activate a Python Virtual Environment
  ```
python -m venv ./airtag-glitch
source airtag-glitch/bin/activate
```
2. Install all needed libraries + jupyter with pip (be sure to install pyserial, not serial)
  ```
pip install jupyter tqdm pyserial
```
3. cd into the notebook folder of the git repo and start up jupyter while in virtual environment
  ```
 cd airtag-glitcher/notebook
jupyter notebook
```
4. Connect J-Link and check if openocd command finds the testnrf.cfg config file by executing
  ```
 openocd -f "interface/jlink.cfg" -c "transport select swd" -f "testnrf.cfg" -c "exit"
 ---------------------------------------------------------------------------------------
Errors are ok as long as config is read correctly - otherwise copy the testnrf.cfg to the correct folder
```
5. Be sure that the current user has permissions for Serial Com Ports (for ubuntu add the user to dialout group and restart system - otherwise you have  to manually chmod 777 the serial port after connecting Raspberry)
```
usermod -a -G dialout $USER
```


## Glitching & Dumping 

 1. Have the Hardware and Software set up, be in virtual environment and have jupyter Notebook open
 2. Connect the j-Link and the Raspberry Pi Pico via USB
 3. Open minicom serial to Raspberry Pi Pico:
 ```
minicom -b 115200 -o -D /dev/ttyACM0
``` 
4. You will be greeted by minicom with the board having started up and ready for use:
![minicom_connected](https://github.com/ensingerphilipp/airtag-glitch-improved/blob/main/assets/minicom.png?raw=true)
5. Start the glitch script and let it run to completion - if it interrupted and printed a success message you can skip "Setting up the delay", otherwise the configured delay was not correct or you are unlucky

### Setting up the delay
If the preconfigured delays do not do the job or you used slightly different hardware - you will need to adjust the delay within the trange in the jupyter notebook at:
 ```
while True:
    for delay in trange(72500, 74000):
        set_delay(delay)
        for pulse in range(7, 9):
            set_pulse(pulse)
            glitch()
            time.sleep(0.05)
            if test_swd():
                print("Glitch successful - swd debug activated (APProtect disabled until powercycle)")
                sys.exit(0)
``` 

***But what delay values to use?***
#### Setting up the delay with oscilloscope
Set a long range of delay and connect Oscilloscope Channel 1 to GPIO19 (Glitch Trigger) and Channel 2 to
Airtag test point 28 (CPU_Core_Voltage) while the glitch script is running. 

Find when the CPU_Core_Voltage at Pin 28 slightly drops after the CPU Core being powered up. The Glitch Trigger from GPIO19 should happen just some us before this voltage drop. This does not have to be super exact as the trigger coming from pico is fluctuating a lot anyways but it should be around there.

You can now look where the trigger is coming in with regards to the voltage drop and adjust the delay higher or lower accordingly.

_Picture of CPU being powered on and the subsequent small drop in voltage (attention: glitch trigger in this picture is set way too early)_

![CPU being powered on and the subsequent small drop](https://github.com/ensingerphilipp/airtag-glitch-improved/blob/main/assets/cpu_vdd_big.jpeg?raw=true)

_Picture of the voltage drop zoomed in (glitch trigger should be happneing just a few us before this)_

![voltage drop zoomed ](https://github.com/ensingerphilipp/airtag-glitch-improved/blob/main/assets/cpu_vdd_close.jpeg?raw=true)

For more information look at Approtect bypass on NRF52 by _limitedresults_ in "Useful Resources" below.

#### Setting up delay without oscilloscope
Basically - you will have to guess the delay.
Set a broad range of delays and let it run overnight. E.g. trange(50000, 150000)

#### Glitching and Dumping (continuation)
Now that we have the correct delay set we can continue after Step 5 from above

 6. After the successful glitch connect openocd to the Airtag
 ```
openocd -f interface/jlink.cfg -c "transport select swd" -f testnrf.cfg -c "init;"
 ```
 It should look like this:
 
![openocd_no_APProtect](https://github.com/ensingerphilipp/airtag-glitch-improved/blob/main/assets/openocd_noAPProtect.png?raw=true)
 
 7. Connect to the session via telnet
  ```
telnet 127.0.0.1 4444
 ```
 8. You can now issue commands e.g. reading what flash config is used 
   ```
flash list
 ```
 9. Finally we can dump the Image of the Airtag or alternatively also read the flash banks separately  
   ```
dump_image airtag_flash_real.bin 0x0 0x80000
flash read_bank 0 bank0_flash.bin
flash read_bank 1 bank1_uicr.bin
 ``` 
 
![dumped](https://github.com/ensingerphilipp/airtag-glitch-improved/blob/main/assets/telnet_dump_image.png?raw=true)

You have successfully dumped the Airtag - now you can do whatever you wish with that - happy reversing! 😎😁
 

## Reflashing / Modification
**DO NOT USE OPENOCD** to flash an image to the airtag --> **YOU WILL BRICK IT** 

--- this section is ommited from public viewing due to moral concerns ---

## Useful Resources:
Stacksmashing´s Devcon 29 Talk about Airtags:
> https://www.youtube.com/watch?v=paxErRRsrTU

Airtag test point definitions by _Colin O’Flynn_
> https://github.com/colinoflynn/airtag-re

Approtect bypass on NRF52 by _limitedresults_
> https://limitedresults.com/2020/06/nrf52-debug-resurrection-approtect-bypass/
> https://limitedresults.com/2020/06/nrf52-debug-resurrection-approtect-bypass-part-2/

_limitedresults_ Attacking NRF52 Blackhat2020 Slides:
> https://limitedresults.com/wp-content/uploads/2020/12/eu-20-Limiteresults-Debug-Resurrection-On-nRF52-Series.pdf

Airtag general technical information by _Adam Catley_:
> https://adamcatley.com/AirTag
