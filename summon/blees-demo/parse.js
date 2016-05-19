/* Parse BLEES advertisements */

var parse_advertisement = function (advertisement, cb) {

    if (advertisement.localName === 'BLEES') {
        if (advertisement.manufacturerData) {
            // Need at least 3 bytes. Two for manufacturer identifier and
            // one for the service ID.
            if (advertisement.manufacturerData.length >= 3) {
                // Check that manufacturer ID and service byte are correct
                var manufacturer_id = advertisement.manufacturerData.readUIntLE(0, 2);
                var service_id = advertisement.manufacturerData.readUInt8(2);
                if (manufacturer_id == 0x02E0 && service_id == 0x12) {
                    // OK! This looks like a BLEES packet
                    if (advertisement.manufacturerData.length >= 14) {
                        var sensor_data = advertisement.manufacturerData.slice(3);

                        var pressure = sensor_data.readUIntLE(0,4)/10;
                        var humidity = sensor_data.readUIntLE(4,2)/100;
                        var temp     = sensor_data.readUIntLE(6,2)/100;
                        var light    = sensor_data.readUIntLE(8,2);
                        var accel    = sensor_data.readUIntLE(10,1);

                        var sequence_num = -1;
                        if (sensor_data.length >= 15) {
                            sequence_num = sensor_data.readUIntLE(11,4);
                        }

                        var imm_accel = ((accel & 0xF0) != 0);
                        var min_accel = ((accel & 0x0F) != 0);

                        var out = {
                            device: 'BLEES',
                            pressure_pascals: pressure,
                            humidity_percent: humidity,
                            temperature_celcius: temp,
                            light_lux: light,
                            acceleration_advertisement: imm_accel,
                            acceleration_interval: min_accel,
                            sequence_number: sequence_num,
                        };

                        cb(out);
                        return;
                    }
                }
            }
        }
    }

    cb(null);
}


module.exports = {
    parseAdvertisement: parse_advertisement
};
