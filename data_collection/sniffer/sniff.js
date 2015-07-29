#!/usr/bin/env node

var noble = require('noble')

noble.on('stateChange', function (state) {
    if (state === 'poweredOn') {
        noble.startScanning([], true);
    }
});

var num_packets = 0;

noble.on('discover', function (peripheral) {

    //if (peripheral.advertisement.localName === 'BLEES' && peripheral.address[15] == 'e' && peripheral.address[16] == '4') {
    if (peripheral.advertisement.localName === 'BLEES') {
        num_packets += 1;

        var address = peripheral.address;

        var msd = peripheral.advertisement.manufacturerData;
        var company_id = msd.readUInt16LE(0);

        var temp     = msd.readFloatLE(2);
        var humidity = msd.readFloatLE(6);
        var light    = msd.readFloatLE(10);
        var pressure = msd.readFloatLE(14);
        //var acceleration = msd.readFloatLE(18);

        console.log('Got BLEES packet #' + num_packets);

        console.log('  Address:      ' + address);

        console.log('  Temperature:  ' + temp.toFixed(1) + ' °C');
        console.log('  Humidity:     ' + humidity.toFixed(1) + '%');
        console.log('  Light:        ' + light.toFixed(1) + ' lx');
        console.log('  Pressure:     ' + pressure.toFixed(1) + ' mbar');
        //console.log('  Acceleration: ' + acceleration.toFixed(1) + ' g');
        console.log('');

    }

});

