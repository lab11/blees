#!/usr/bin/env node

var noble = require('noble')

var ESS_SERVICE_UUID16 = 0x181A;
var UMICH_COMPANY_ID = 0x02E0;
var BLEES_SERVICE_ID = 0x12;

noble.on('stateChange', function (state) {
    if (state === 'poweredOn') {
        noble.startScanning([], true);
    }
});

noble.on('discover', function (peripheral) {

    var advertisement = peripheral.advertisement;
    if (peripheral.advertisement.localName === 'BLEES') {

        // old packet format for blees
        if (advertisement.serviceData !== undefined && advertisement.serviceData
                && advertisement.serviceData[0]) {
            var service_uuid = peripheral.advertisement.serviceData[0].uuid;
            var service_data = peripheral.advertisement.serviceData[0].data;

            // BLE ESS Service
            if (parseInt(service_uuid, 16) == 0x181A) {
                // service data is environmental data collected by BLEES
                console.log("WARNING: Old BLEES packet format");
                parse_blees_data(peripheral, service_data);
            }
        }

        // new packet format for blees
        if (advertisement.manufacturerData !== undefined && advertisement.manufacturerData
                && advertisement.manufacturerData.length >= 3) {
            var company_id = advertisement.manufacturerData.readUIntLE(0,2);
            var service_id = advertisement.manufacturerData.readUInt8(2);

            if (company_id == UMICH_COMPANY_ID && service_id == BLEES_SERVICE_ID) {
                // found a new-format BLEES
                var data = advertisement.manufacturerData.slice(3);
                parse_blees_data(peripheral, data);
            }
        }
    }
});

function parse_blees_data (peripheral, data) {
    var address = peripheral.address;
    var pressure = data.readUIntLE(0,4)/10;
    var humidity = data.readUIntLE(4,2)/100;
    var temp     = data.readUIntLE(6,2)/100;
    var light    = data.readUIntLE(8,2);
    var accel    = data.readUIntLE(10,1);

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
    console.log('');
}

