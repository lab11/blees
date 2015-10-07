BLEES
=====

<p align="center">
  <img src="https://raw.githubusercontent.com/lab11/blees/master/media/blees.png" alt="BLEES" width="20%;">
</p>

BLEES: Bluetooth Low Energy Environment Sensors.

BLEES is a 1 inch round sensor tag for sensing the ambient environment. It monitors
temperature, humidity, light, pressure, and movement and reports its readings
over BLE.

Hardware
--------
The BLEES hardware is a small, once inch round sensor board that mounts onto a [Squall](https://github.com/helena-project/squall) BLE sensor tag. The board 
currently has four sensors:

1. Temperature and Humidity
2. Pressure
3. Light
4. Accelerometer

The BLEES hardware is located in the `hardware` directory, where you can find
the Eagle design files.

Software
--------
The [Squall](https://github.com/helena-project/squall) uses the software located 
in the `software` directory. Follow the directions at [Lab11's Wiki](http://lab11.eecs.umich.edu/wiki/doku.php?id=eecs582w15:setup:start) to 
get your machine set up to build and flash the Squall.

The software sends out the data from the BLEES sensors in BLE advertisements. Specifically,
we encode the data in the "manufacturer data" section of a BLE advertisement and scan response packet.
This means that we do not have to establish a connection between the Squall and a scanning device, 
saving energy by not requiring any devices to pair with the BLEES device, and allowing us to send
the sensor data to many different scanning devices at once.

[Here's a useful BLE Advertisement primer](http://www.argenox.com/bluetooth-low-energy-ble-v4-0-development/library/a-ble-advertising-primer/)

Summon App
-----------
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
date by doing `git submodule update --init --recursive`.

If you think all of this is ridiculous and git should just handle submodules automatically, use this:
https://gist.github.com/brghena/fc4483a2df83c47660a5
