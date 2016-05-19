#!/usr/bin/env node

var noble = require('noble')

var ESS_SERVICE_UUID16 = 0x181A;
var UMICH_COMPANY_ID = 0x02E0;
var BLEES_SERVICE_ID = 0x12;

var blees_choice = null;
if (process.argv.length >= 3) {
    blees_choice = process.argv[2];
}

noble.on('stateChange', function (state) {
    if (state === 'poweredOn') {
        noble.startScanning([], true);
        console.log("Scanning started");
        if (blees_choice != null) {
            console.log("Searching for " + blees_choice);
        }
    }
});

noble.on('discover', function (peripheral) {

    if (blees_choice != null && peripheral.address != blees_choice) {
        return;
    }

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
    var temp     = data.readIntLE(6,2)/100;
    var light    = data.readUIntLE(8,2);
    var accel    = data.readUIntLE(10,1);

    var sequence_num = -1;
    if (data.length >= 15) {
        sequence_num = data.readUIntLE(11,4);
    }

    var imm_accel = ((accel & 0xF0) != 0);
    var min_accel = ((accel & 0x0F) != 0);

    console.log("BLEES (" + address + ")");
    if (sequence_num != -1) {
        console.log('  Sequence Num: ' + sequence_num);
    }
    console.log('  Temperature:  ' + temp.toFixed(1) + ' °C');
    console.log('  Humidity:     ' + humidity.toFixed(1) + '%');
    console.log('  Light:        ' + light.toFixed(1) + ' lx');
    console.log('  Pressure:     ' + pressure.toFixed(1) + ' pascals');
    if (imm_accel) {
        console.log('  Acceleration currently active');
    }
    if (min_accel) {
        console.log('  Acceleration in last minute');
    }
    console.log('');
}

