/* Parse BLEES advertisements */

var parse_advertisement = function (advertisement, cb) {

    if (advertisement.localName === 'BLEES') {
        var service_uuid = advertisement.serviceData[0].uuid;
        var service_data = advertisement.serviceData[0].data;

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

            var out = {
                device: 'BLEES',
                pressure_pascals: pressure,
                humidity_percent: humidity,
                temperature_celcius: temp,
                light_lux: light,
                acceleration_advertisement: imm_accel,
                acceleration_interval: min_accel
            };

            cb(out);
            return;
        }
    }

    cb(null);
}


module.exports = {
    parseAdvertisement: parse_advertisement
};
