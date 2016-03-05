Blink App
=========

Advertise Eddystone and manufacturer specific data when motion is detected.

Hardware
--------

Squall with a digital PIR sensor attached to pin 25.

Data Format
-----------

Manufacturer specific data in the advertisement:

| Index | Bytes        | Description                            |
| ----- | ------------ | -------------------------------------- |
| 0     | `0x07`       | Length                                 |
| 1     | `0xff`       | Header for manufacturer specific data  |
| 2     | `0x02e0`     | University of Michigan manufacturer ID |
| 4     | `0x13`       | Service identifier for Blink data      |
| 5     | `0x00`       | 0 if no motion right now, 1 if motion  |
| 6     | `0x00`       | 0 if no motion since last advertisement packet, 1 if motion in that time |
| 7     | `0x00`       | 0 if no motion in the last minute, 1 if motion |
