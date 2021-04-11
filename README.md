# Europi

## Overview

The Europi is a 16hp Eurorack format sequencer powered by the Raspberry Pi single board computer. It is a digital sequencer that outputs Control Voltages (CV), Modulation and Gate signals, and can be controlled by a variety of signal sources including its own internal clock, external clocks, rotary encoders, MIDI sources and the like.

Europi has been designed to be modular and expandable. The main Europi module, which interfaces directly with the Raspberry Pi has the following features:

320 x 240 pixel 16-bit colour touchscreen display
Continuous rotary push-button encoder
4 x tactile 'soft' buttons
2 x 1 v/Oct CV output (16-Bit resolution over a 10 volt range)
2 x Modulation outputs (16-Bit resolution over a 10 volt range)
2 x Gate ouputs (+5 volt leading-edge triggered)
Clock Out
Clock In
Step 1 Out
Reset In
Hold In

The primary Europi module interfaces with a number of secondary modules.

The 6hp Europi Minion extends the output capabilities of the Europi by providing an additional 4 x CV and 4 x Gate outputs. Up to 8 Minions can be daisy-chained together so that, including the main Europi module, a complete system can support 34 x CV and 34 x Gate outputs.

The Europi MIDI Minion adds MIDI In, Out and Thru capability and can be used as a means of note entry, as a Clock source and also as a MIDI output. Up to 8 MIDI Minions can be connected to a Europi.

The intention is that, being open sourced, the initial hardware platform could be extended to include support for additional modules, control surfaces and such like, either by using the existing I2C bus that links the Europi to the Minions, or leveraging the USB capabilities of the Raspberry Pi.

## Open Source Hardware Certification

Europi Hardware is certified under the Open Source Hardware Certification Program. Certification Number UK000002

## Interfaces

The Europi is not an analogue sequencer, it is a digital sequencer that outputs analogue CV, Mod and Gate signals. Due to its digital nature the CV outputs are not infinitely variable, but are converted from digital to analogue using 16-Bit Digital to Analogue Converters  (DACs). This gives a resolution of around 6,000 discrete steps per octave over a 10 volt range, ie 500 per semitone. So, whilst the CV outputs are not continuously variable as in a true Analogue sequencer, the resolution is sufficiently high such that the difference isn't discernable in the vast majority of applications.

The output of the DACs have a full-scale output range of 0 to approx 5 volts. In reality, due to manufacturing tolerances, these outputs rarely reach a true zero and often have a minimum achievable output of around 20mv, so the DAC outputs are buffered via Op Amps that are biased with a slight negative voltage, and have an amplification factor of around 2.5 thus a true zero and full-scale 10v output can be set in software, which removes the need for complex and fiddly trimmer potentiometers.

The resistors biasing this output buffer circuit could be selected so that the output swings between, say +/-5v instead of the usual 0-10v, which would then be suitable for use as an audio output (via suitable drivers).

The CV and Gate outputs are connected to the Raspberry Pi using the main user Inter-Integrated Circuit (I2C) bus. The Raspberry Pi actually has two I2C busses, though the first (I2C-0) is generally reserved for querying 'hats' - add-on boards that sit on top of the Pi. The I2C bus is the slower of the Pi's two busses, but is the more flexible and extensible of the two. The faster SPI bus is used for the LCD display and its associated touchscreen interface. 

It is the I2C bus that is used to daisy-chain the various Minions to the main Europi. The I2C Bus is driven at 400KHz

The soft buttons, push-button rotary encoder, and external clock and reset inputs are connected to GPIO pins of the Raspberry Pi via suitable interface circuitry which provides over- and reverse-voltage protection.

Future expansion plans include a CV & Gate Input Minion for driving the Europi from External CV/Gate sources, a gesture-based controller, and a USB & HDMI Minion that connects the Raspberry Pi's USB output to a 2-Port Powered USB Hub, and brings the HDMI output through to a Eurorack panel. The latter is intended to make it simpler to pogram the Europi in situ, and opens the possibility for the connection of other control surfaces via USB.  


## Language and environment

Europi is written in C as it is fairly efficient and links directly with the PIGPIO library (see below). System and operational states are held in global variables. Opinions are divided over the use of global variables in C, though our view is that in a monolithic application such as this, global variables are both quick & efficient (less stack overheads) and simple to understand.

The GUI has been developed using Raylib (http://www.raylib.com), a free Open Source graphics library.

Touchscreen Input is interpreted by RayLib and mapped to Mouse Movements using the ts_uinput daemon, part of TSLIB.

## Principle of operation

Europi makes extensive use of the excellent PIGPIO library (https://github.com/joan2937/pigpio), in particular the I2C functions, PWM functions, and alert functions.

The main Internal clock is generated by establishing a PWM output on GPIO pin 18 (physical pin 12) against which a Callback function is registered. This callback function is therefore called each time the PWM output changes state. An internal counter divides this clock by 96 to give an internal resolution of 96 pulses per quarter note PPQN.

Due to the nature of the PIGPIO library, the PWM frequency can only be set to an integer number of cycles per second (Hz) and given the internal 96PPQN division, a range of typical BPM frequencies can be obtained using the following formula: BPM = (PWM/48)/60. So, for example, a PWM frequency of 200Hz will yield a corresponding BPM of 250 201Hz will yield 251.25 Etc. Obviously therefore, some BPM values cannot be realised using the internal clock due to this integer Hz limitation.

Using the external clock is a different matter as, presumably, an external oscillator can be set to any fractional frequency. The external clock is treated similarly to the internal clock - an alert function is registered against the GPIO pin to which it is applied (GPIO pin 12 Physical pin 32) and this is therefore called each time the external clock changes state. 

The MIDI Minion can alo be used as a clock source at the normal speed of 24 PPQN. 

Within the main body of the code, various control flags and counters are used to record the current operational state, internal vs external clock etc.

The main sequencer object is held in a single global structure called, understandably enough, Europi. Yes, this structure is inefficient. Yes, it would have saved space to malloc structures on the fly at startup, but  statically-defined structure such as this is easy to understand, and memory is not particularly at a premium in the Raspberry Pi.

The GUI has been developed using the Raylib graphics library (http://www.raylib.com). This was originally developed for games programming, but its 2D and text-handling elements are ideally suited to simple GUI development.

Although the Raspberry Pi can be configured to expose a TFT screen as a secondary framebuffer, it doesn't support hardware graphics acceleration when writing to it. I therefore chose to use Raylib to write to the default framebuffer /dev/fb0 and to use the free open source utility raspbi2fb (https://github.com/AndrewFromMelbourne/raspi2fb) to mirror /dev/fb0 to /dev/fb1 as this actually gives better graphic performance than writing directly to /dev/fb1

## Setup and Configuration

As always with Eurorack modules, care must be taken when attaching the module to a Eurorack PSU. We have included reverse-protection diodes to protect against inadvertent reversal of the supply connector, and also +12v and -12v marks are included on the silk screen to try and prevent errors. We have adopted the Doepfer 'standard' keyed shrouded header type of connector to also minimise potential errors.

Up to 8 Minions can be daisy-chained to a single Europi. There are two 6-way Minion connection headers on the main Europi board, one each side to make it easier to place the Minions to the left or right (or both) of the main Europi module. Minions can be daisy-chained in any order, the only stipulation being that each must be given a unique address on the I2C bus. Failure to do so will lead to unpredictable results.

The minion header mirrors the first 8 pins of the Raspberry Pi, so Minions can be connected using straight-through 8-way ribbon cables. Care must be taken to make sure Pin 1 is connected to Pin 1 etc.

As the Minion header mirrors the first 8 pins of the Raspberry Pi header, Minions can be connected directly to a Raspberry Pi without the need to include a Europi module, though obviously all of the functionality of the Europi would be missing from such a configuration (touchscreen LCD, rotary encoder etc) though it does represent a cheaper and simpler entry route into the realm of the Europi.

The Minion address is set using a 3-way DIP switch, using standard binary addressing: 000, 001, 010 .... 111 The address is read by the Minion when power is applied, so a Minion's address should never be changed whilst power is connected.

The MIDI Minion uses 2 separate jumpers to set one of 4 possible addresses, so 4 MIDI Minions can be connected to a Europi.

During startup, the Europi will scan the I2C bus looking for attached Minions, and will assign blocks of 4 Tracks to each Minion as it finds them. I2C address scanning starts from 000 so the Minion with the lowest address will be found first, and the highest last. Minion addresses do not have to be consecutive, just unique.

For ease of use, it would seem to make sense to assign Minion addresses in ascending order from left to right, though it's entirely up to you. During startup, the Europi will briefly flash all the Gate LEDs in the order it found them, which provides a useful indicator as to whether all Minions are in the correct order.

The second stage of the configuration is to set the Zero and Full scale levels for each CV output (both Europi and Minions). This only ever needs to be carried out once, or if the number or order of Minions is changed.

## Pi Configuration and Build Notes

These have been moved to the Wiki 
