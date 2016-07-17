Solar BLEES App
===============

This app provides BLEES-like functionality off a solar panel

Programming
-----------

    make flash ID=c0:98:e5:30:00:01

Advertisement
-------------

BLEES alternates between advertising environmental data and the
[Eddystone protocol](https://github.com/google/eddystone). It works with the
[Summon](https://github.com/lab11/summon) project that provides a browser-based
UI for BLE devices.

### Data Format

Manufacturer specific data in the advertisement:

| Index | Bytes        | Description                            |
| ----- | ------------ | -------------------------------------- |
| 0     | `0x07`       | Length                                 |
| 1     | `0xff`       | Header for manufacturer specific data  |
| 2     | `0x02e0`     | University of Michigan manufacturer ID |
| 4     | `0x12`       | Service identifier for BLEES data      |
| 5-8   | `0x00000000` | Pressure in deci-pascals               |
| 8-9   | `0x0000`     | Humidity in centi-percent              |
| 10-11 | `0x0000`     | Temperature in centi-degrees Celsius   |
| 12-13 | `0x0000`     | Light in lux                           |
| 14    | `0x00`       | Acceleration as explained below        |
| 15-18 | `0x00000000` | Sequence number                        |

*Acceleration* is defined as boolean flags with `bit 0` indicating acceleration
in the last minute and `bit 4` indicating current acceleration.

Ex: 0x01 = acceleration in the last minute, but not currently

### Eddystone URL

Change the `PHYSWEB_URL` define to change where the BLEES Eddystone URL points to.


