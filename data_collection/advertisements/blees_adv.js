#!/usr/bin/env node

var noble = require('noble')

noble.on('stateChange', function (state) {
    if (state === 'poweredOn') {
        noble.startScanning([], true);
    }
});

var num_packets = 0;

noble.on('discover', function (peripheral) {

    if (peripheral.advertisement.localName === 'BLEES') {
        num_packets += 1;

        var address = peripheral.address;

        var service_uuid = peripheral.advertisement.serviceData[0].uuid;
        var service_data = peripheral.advertisement.serviceData[0].data;

        // BLE ESS Service
        if (parseInt(service_uuid, 16) == 0x181A) {
            // service data is environmental data collected by BLEES

            var pressure = service_data.readUIntLE(0,4)/10;
            var humidity = service_data.readUIntLE(4,2)/100;
            var temp     = service_data.readUIntLE(6,2)/100;
            var light    = service_data.readUIntLE(8,2);
            var accel    = service_data.readUIntLE(10,1);

            var imm_accel = ((accel & 0xF0) != 0);
            var min_accel = ((accel & 0x0F) != 0);

            console.log("BLEES (" + address + ")");
            console.log('  Temperature:  ' + temp.toFixed(1) + ' °C');
            console.log('  Humidity:     ' + humidity.toFixed(1) + '%');
            console.log('  Light:        ' + light.toFixed(1) + ' lx');
            console.log('  Pressure:     ' + pressure.toFixed(1) + ' pascals');
            if (imm_accel) {
                console.log('  Acceleration since last advertisement');
            }
            if (min_accel) {
                console.log('  Acceleration in last sampling interval');
            }
        }
    }
});

