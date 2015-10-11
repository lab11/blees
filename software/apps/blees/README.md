BLEES App
=========

This app provides the Environmental Sensing Service for BLEES.

Programming
-----------

    make flash ID=c0:98:e5:30:00:01

Advertisement
-------------

BLEES alternates between advertising environmental data and the
[Eddystone protocol](https://github.com/google/eddystone). It works with the
[Summon](https://github.com/lab11/summon) project that provides a browser-based
UI for BLE devices.

### Eddystone URL

Change the `PHYSWEB_URL` define to change where the BLEES Eddystone URL points to.

Services
--------

Standard environmental sensing service [ESS](https://www.bluetooth.org/en-us/specification/assigned-numbers/environmental-sensing-service-characteristics)

