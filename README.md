BLEES
=====

<img src="https://raw.githubusercontent.com/lab11/blees/master/media/blees.png" alt="BLEES" width="20%;" align="left">
BLEES: Bluetooth Low Energy Environment Sensors.
<br /><br />
BLEES is a 1 inch round sensor tag for sensing the ambient environment. It monitors
temperature, humidity, light, pressure, and movement and reports its readings
over BLE.
<br /><br /><br />

Hardware
--------
The BLEES hardware is a small, once inch round sensor board that mounts onto a [Squall](https://github.com/helena-project/squall) BLE sensor tag. The board 
currently has four sensors:

1. Temperature and Humidity (Si7021)
2. Pressure (LPS331AP)
3. Light (TSL2561)
4. Accelerometer (ADXL362)

The BLEES hardware is located in the `hardware` directory, where you can find
the Eagle design files.

Software
--------
The [Squall](https://github.com/helena-project/squall) uses the software
located in the `software` directory. Follow the directions at [Lab11's
Wiki](http://lab11.eecs.umich.edu/wiki/doku.php?id=eecs582w15:setup:start) to
get your machine set up to build and flash the Squall.

The primary application is
[`software/apps/blees`](https://github.com/lab11/blees/tree/master/software/apps/blees).
This application samples the
sensors and makes this data available as both broadcast advertisements and the
Environmental Sensing Service for a connected device.

Data Reference
--------------
### Advertisement data
BLEES transmits advertisements every 1000 ms containing application data.
[Here's a useful BLE Advertisement
primer](http://www.argenox.com/bluetooth-low-energy-ble-v4-0-development/library/a-ble-advertising-primer/).
The advertisements come in two forms: environmental data and eddystone.
Advertisements alternate between the two evenly.

The environmental data advertised is same data as is available through the
environmental sensing service, but broadcast so several nearby devices can
access it concurrently. The data includes, in order, four bytes for pressure,
two bytes for humidity, two bytes for temperature, two bytes for light
illuminance, and one byte for acceleration state. An example application that
collects BLE advertisements from BLEES devices and displays the data can be
found at `data_collection/advertisements/blees_adv.js`

[Eddystone](https://github.com/google/eddystone) is a protocol for connecting
BLE devices to Internet resources. BLEES advertises the URL of an application
that can be interpreted by the Summon framework in order to automatically
generate a user interface. (see next heading)


### Environmental Sensing Service
BLEES is also connectable and supports the Environmental Sensing Service. While
connected to, it continues to advertise, but only one connection at a time is
allowed. The environmental sensing service is defined by the
[BLE SIG](https://www.bluetooth.org/en-us/specification/assigned-numbers/environmental-sensing-service-characteristics).


Summon App
----------
[Summon](https://github.com/lab11/summon) is a UI application for BLE devices.
Rather than requiring every user
to install a new app for every BLE device, Summon allows BLE devices to
point to their own HTML/JS based interface and loads it in a single
application. BLEES supports the summon architecture and provides
a Summon application.


Cloning
-------
To clone both this repository and the required submodules,
clone with the `--recursive` option: 
`git clone --recursive git@github.com:lab11/blees.git`

Otherwise, you can initialize the submodule and keep submodules up to
date by doing `git submodule update --init --recursive`

If you think all of this is ridiculous and git should just handle submodules automatically, use this:
https://gist.github.com/brghena/fc4483a2df83c47660a5

Other Hardware
--------------

Included in this repo are:

### Blink

<img src="https://raw.githubusercontent.com/lab11/blees/master/media/blink_large_1000x790.jpg" width="25%" align="left">

Blink is a PIR sensor that is also based on Squall. Like BLEES, it supports Summon.
The nRF51822 app is [here](https://github.com/lab11/blees/tree/master/software/apps/blink).

