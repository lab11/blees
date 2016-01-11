/* JavaScript for ESS Summon UI */

var deviceId = "C0:98:E5:30:DA:E4";                                                 // while testing, replace with address of a BLE peripheral
var deviceName = "BLE Device";                                                      // while testing, replace with desired name
var serviceUuid = "181A";                                                           // ESS UUID
var writeValue = "Written Name";                                                    // value to write to characteristic
var essdescriptorUuid = "290D";

var pressureUuid = "2A6D";                                                          // pressure characteristic UUID to read or write
var humidityUuid = "2A6F";                                                          // humidity characteristic UUID to read or write
var temperatureUuid = "2A6E";                                                       // temperature characteristic UUID to read or write
var luxUuid = "C512";                                                               // lux characteristic UUID to read or write
var accelerationUuid = "F801";                                                      // acceleration characteristic UUID to read or write

var ess_service = "181A";

// var device_connected = false;
var timer;
// var touchduration = 3000; //length of time we want the user to touch before we do something
// var connection_toggle = false;
// var is_init = false;

var last_update = 0;

var switch_visibility_console_check = "visible";
var switch_visibility_steadyscan_check = "visible";
var steadyscan_on = true;

// Load the swipe pane
$(document).on('pageinit',function(){
    $("#main_view").on("swipeleft",function(){
        $("#logPanel").panel( "open");
    });
});

function adv_bytes_to_noble_object (inadv) {
    var i = 0;
    var j = 0;
    // Return
    var advertisement = {
        serviceUuids: [],
        serviceData: []
    };

    var raw_adv = (new Uint8Array(inadv)).buffer;
    var eir = new DataView(raw_adv);

    while ((i + 1) < eir.byteLength) {
        var length = eir.getUint8(i);

        if (length < 1) {
            break;
        }

        var eirType = eir.getUint8(i + 1); // https://www.bluetooth.org/en-us/specification/assigned-numbers/generic-access-profile

        if ((i + length + 1) > eir.byteLength) {
            break;
        }

        // var bytes = eir.slice(i + 2).slice(0, length - 1);
        var bytes = raw_adv.slice(i + 2, i + 2 + length - 1);
        var bytesdv = new DataView(bytes);

        switch(eirType) {
            case 0x02: // Incomplete List of 16-bit Service Class UUID
            case 0x03: // Complete List of 16-bit Service Class UUIDs
                for (j = 0; j < bytes.byteLength; j += 2) {
                    var serviceUuid = bytesdv.getUint16(j, true).toString(16);
                    if (advertisement.serviceUuids.indexOf(serviceUuid) === -1) {
                        advertisement.serviceUuids.push(serviceUuid);
                    }
                }
                break;

            case 0x06: // Incomplete List of 128-bit Service Class UUIDs
            case 0x07: // Complete List of 128-bit Service Class UUIDs
                for (j = 0; j < bytes.byteLength; j += 16) {
                    var serviceUuidBytes = bytes.slice(j, j + 16);
                    var serviceUuidBytesDv = new DataView(serviceUuidBytes);
                    var serviceUuid = '';
                    for (var k=0; k<16; k++) {
                        serviceUuid += serviceUuidBytesDv.getUint8(k).toString(16);
                    }
                    if (advertisement.serviceUuids.indexOf(serviceUuid) === -1) {
                        advertisement.serviceUuids.push(serviceUuid);
                    }
                }
                break;

            case 0x08: // Shortened Local Name
            case 0x09: // Complete Local Name»
                var decoder = new TextDecoder('utf8');
                advertisement.localName = decoder.decode(bytesdv);
                console.log(advertisement.localName);
                break;

            case 0x0a: // Tx Power Level
                advertisement.txPowerLevel = bytesdv.getInt8(0);
                break;

            case 0x16: // Service Data, there can be multiple occurrences
                // var serviceDataUuid = bytes.slice(0, 2).toString('hex').match(/.{1,2}/g).reverse().join('');
                // var serviceData = bytes.slice(2, bytes.length);

                // advertisement.serviceData.push({
                //   uuid: serviceDataUuid,
                //   data: serviceData
                // });
                break;

            case 0xff: // Manufacturer Specific Data
                advertisement.manufacturerData = bytes;
                break;
        }

        i += (length + 1);
    }

    return advertisement;
}

var app = {
    // Application Constructor
    initialize: function() {
        app.log("BLEES init");

        document.addEventListener("deviceready", app.onAppReady, false);
        document.addEventListener("resume", app.onAppReady, false);
        document.addEventListener("pause", app.onPause, false);

        app.onAppReady();
    },
    // App Ready Event Handler
    onAppReady: function() {
        app.log("onAppReady");

        // Setup update for last data time
        setInterval(app.update_time_ago, 5000);

        if (typeof window.gateway != "undefined") {                               // if UI opened through Summon,
            deviceId = window.gateway.getDeviceId();                                // get device ID from Summon
            deviceName = window.gateway.getDeviceName();                            // get device name from Summon
            app.log("Opened via Summon..");
        }
        document.getElementById("title").innerHTML = String(deviceId);
        app.log("Checking if ble is enabled...");
        ble.isEnabled(app.onEnable);                                                // if BLE enabled, goto: onEnable
        // app.onEnable();
    },
    // App Paused Event Handler
    onPause: function() {
        app.log("on Pause");                                                           // if user leaves app, stop BLE
        ble.stopScan();
    },
    // Bluetooth Enabled Callback
    onEnable: function() {
        app.log("onEnable");
        // app.onPause();                                                              // halt any previously running BLE processes
        ble.startScan([], app.onDiscover, app.onAppReady);                          // start BLE scan; if device discovered, goto: onDiscover
        app.log("Searching for " + deviceName + " (" + deviceId + ").");
    },
    // BLE Device Discovered Callback
    onDiscover: function(device) {
        if (device.id == deviceId) {
            app.log("Found " + deviceName + " (" + deviceId + ")!");
            app.onParseAdvData(device);
        } else {
            app.log('Not BLEES (' + device.id + ')');

            // HACK:
            ble.stopScan();
            ble.startScan([], app.onDiscover, app.onAppReady);
        }
    },
   onParseAdvData: function(device){
        //Parse Advertised Data
        var advertisement = adv_bytes_to_noble_object(device.advertising);

        // Check this is something we can parse
        if (advertisement.localName == 'BLEES' &&
            advertisement.serviceUuids.indexOf('181a') !== -1) {

            var mandata = new Uint8Array(advertisement.manufacturerData);

            // Save when we got this.
            last_update = Date.now();

            //app.log("Parsing advertised data...");
            var pressure_pascals = (( (mandata[6] * 16777216) + (mandata[5] * 65536 ) + (mandata[4] * 256) + mandata[3] )/10);
            var pressure_mmHg = (pressure_pascals*0.007500616827042).toFixed(2);
            var pressure_atm = (pressure_pascals*0.00000986923266716).toFixed(4);
            var pressureOut = pressure_mmHg + ' mmHg<br />' + pressure_atm + ' atm';
            app.log( "Pressure: " + pressureOut);
            document.getElementById("presVal").innerHTML = String(pressureOut);

            var humidityOut = (( (mandata[8] * 256) + mandata[7] )/100) + String.fromCharCode(37);
            app.log( "Humidity: " + humidityOut);
            document.getElementById("humVal").innerHTML = String(humidityOut);

            var temp_celsius = (((mandata[10] * 256) + mandata[9])/100).toFixed(1);
            var temp_fahrenheit = ((temp_celsius * (9/5)) + 32).toFixed(1);
            var temperatureOut = temp_celsius + " " + String.fromCharCode(176) + "C";
            temperatureOut    += '<br />' + temp_fahrenheit + " " + String.fromCharCode(176) + "F";
            app.log( "Temperature: " + temperatureOut);
            document.getElementById("tempVal").innerHTML = String(temperatureOut);

            var luxOut = ( (mandata[12] * 256) + mandata[11]) + " lux" ;
            app.log( "Lux: " + luxOut);
            document.getElementById("luxVal").innerHTML = String(luxOut);

            var accdata = mandata[13];
            var immAcc = ((accdata & 17) >> 4);
            var intAcc = (accdata & 1);
            app.log("Immediate Acceleration: " + ((accdata & 17) >> 4) );
            app.log("Interval Acceleration: " + (accdata & 1) );

            if (intAcc) {
                document.getElementById('accLastIntCell').style.color = "#ED97B9";
                document.getElementById('accLastIntCell2').style.color = "#ED97B9";
                document.getElementById('accSpinnerInt').style.visibility = "visible";
                document.getElementById('accNotSpinnerInt').style.visibility = "hidden";
            } else {
                document.getElementById('accLastIntCell').style.color = "black";
                document.getElementById('accLastIntCell2').style.color = "black";
                document.getElementById('accSpinnerInt').style.visibility = "hidden";
                document.getElementById('accNotSpinnerInt').style.visibility = "visible";
            }

            app.update_time_ago();

            // if(steadyscan_on){
            //     app.onEnable();
            // }
        } else {
            // Not a BLEES packet...
            app.log('Advertisement was not BLEES.');
        }

    },
    // Function to Convert String to Bytes (to Write Characteristics)
    stringToBytes: function(string) {
        array = new Uint8Array(string.length);
        for (i = 0, l = string.length; i < l; i++) array[i] = string.charCodeAt(i);
        return array.buffer;
    },
    buffToUInt32Decimal: function(buffer) {
        //app.log("to32");
        var uint32View = new Uint32Array(buffer);
        return uint32View[0];
    },
    buffToUInt16Decimal: function(buffer) {
        var uint16View = new Uint16Array(buffer);
        return uint16View[0];
    },
    buffToInt16Decimal: function(buffer) {
        var int16View = new Int16Array(buffer);
        return int16View[0];
    },
    buffToUInt8Decimal: function(buffer) {
        var uint8View = new Uint8Array(buffer);
        return uint8View[0];
    },
    // Function to Convert Bytes to String (to Read Characteristics)
    bytesToString: function(buffer) {
        return String.fromCharCode.apply(null, new Uint8Array(buffer));
    },
    update_time_ago: function () {
        if (last_update > 0) {
            // Only do something after we've gotten a packet
            // Default output
            var out = 'Haven\'t gotten a packet in a while...';

            var now = Date.now();
            var diff = now - last_update;
            if (diff < 60000) {
                // less than a minute
                var seconds = Math.round(diff/1000);
                out = 'Last updated ' + seconds + ' second';
                if (seconds != 1) {
                    out += 's';
                }
                out += ' ago';

            } else if (diff < 120000) {
                out = 'Last updated about a minute ago';
            }

            document.querySelector("#data_update").innerHTML = out;
        }
    },
    // Function to Log Text to Screen
    log: function(string) {
        document.querySelector("#console").innerHTML += (new Date()).toLocaleTimeString() + " : " + string + "<br />";
        document.querySelector("#console").scrollTop = document.querySelector("#console").scrollHeight;
    }
};

app.initialize();
