/* JavaScript for ESS Summon UI */

var deviceId = "C0:98:E5:30:45:BF";                                                 // while testing, replace with address of a BLE peripheral
var deviceName = "BLE Device";                                                      // while testing, replace with desired name
var serviceUuid = "181A";                                                           // ESS UUID
var writeValue = "Written Name";                                                    // value to write to characteristic
var essdescriptorUuid = "290D";

var pressureUuid = "2A6D";                                                          // pressure characteristic UUID to read or write
var humidityUuid = "2A6F";                                                          // humidity characteristic UUID to read or write
var temperatureUuid = "2A6E";                                                       // temperature characteristic UUID to read or write
var luxUuid = "C512";                                                               // lux characteristic UUID to read or write
var accelerationUuid = "F801";                                                      // acceleration characteristic UUID to read or write


var app = {
    // Application Constructor
    initialize: function() {
        document.addEventListener("deviceready", app.onAppReady, false);
        document.addEventListener("resume", app.onAppReady, false);
        document.addEventListener("pause", app.onPause, false);
        bleesimg.addEventListener('touchstart', app.onTouch, false);                // if bulb image touched, goto: onToggle
        lightimg.addEventListener('touchstart', app.onTouchLight, false);           // if bulb image touched, goto: onToggle
        tempimg.addEventListener('touchstart', app.onTouchTemp, false);             // if bulb image touched, goto: onToggle
        bulbimg.addEventListener('touchstart', app.onTouchBulb, false);             // if bulb image touched, goto: onToggle

    },
    // App Ready Event Handler
    onAppReady: function() {
        if (window.gateway) {                                                       // if UI opened through Summon,
            deviceId = window.gateway.getdeviceId();                                // get device ID from Summon
            deviceName = window.gateway.getDeviceName();                            // get device name from Summon
        }
        ble.isEnabled(app.onEnable);                                                // if BLE enabled, goto: onEnable
    },
    // App Paused Event Handler
    onPause: function() {                                                           // if user leaves app, stop BLE
        ble.disconnect(deviceId);
        ble.stopScan();
    },
    // Bluetooth Enabled Callback
    onEnable: function() {
        app.onPause();                                                              // halt any previously running BLE processes
        ble.startScan([], app.onDiscover, app.onAppReady);                          // start BLE scan; if device discovered, goto: onDiscover
        app.log("Searching for " + deviceName + " (" + deviceId + ").");
    },
    // BLE Device Discovered Callback
    onDiscover: function(device) {
        if (device.id == deviceId) {
            app.log("Found " + deviceName + " (" + deviceId + ")! Connecting.");
            ble.connect(deviceId, app.onConnect, app.onAppReady);                   // if device matches, connect; if connected, goto: onConnect
        }
    },
    // BLE Device Connected Callback
    onConnect: function(device) {
        app.log("Connected to " + deviceName + " (" + deviceId + ")!");
        ble.read(deviceId, serviceUuid, pressureUuid, app.onReadPres, app.onError);
        ble.read(deviceId, serviceUuid, humidityUuid, app.onReadHum, app.onError);  
        ble.read(deviceId, serviceUuid, temperatureUuid, app.onReadTemp, app.onError); 
        ble.read(deviceId, serviceUuid, luxUuid, app.onReadLux, app.onError);  
        ble.read(deviceId, serviceUuid, accelerationUuid, app.onReadAcc, app.onError);
    },
    onTouch: function(device) {
        app.log("Getting sensor data...");
        ble.read(deviceId, serviceUuid, pressureUuid, app.onReadPres, app.onError);
        ble.read(deviceId, serviceUuid, humidityUuid, app.onReadHum, app.onError);  
        ble.read(deviceId, serviceUuid, temperatureUuid, app.onReadTemp, app.onError); 
        ble.read(deviceId, serviceUuid, luxUuid, app.onReadLux, app.onError);  
        ble.read(deviceId, serviceUuid, accelerationUuid, app.onReadAcc, app.onError);  
    },
    onTouchLight: function(device) {
        app.log("Getting lux...");
        ble.read(deviceId, serviceUuid, luxUuid, app.onReadLux, app.onError);  
    },
    onTouchTemp: function(device) {
        app.log("Getting temperature...");
        ble.read(deviceId, serviceUuid, temperatureUuid, app.onReadTemp, app.onError);  
    },
    onTouchBulb: function(device) {
        app.log("bulb touched");

        var lightoptions = {
            'androidTheme': window.plugins.actionsheet.ANDROID_THEMES.THEME_HOLO_LIGHT, // default is THEME_TRADITIONAL
            'title': 'Configure Lux Trigger Settings',
            'buttonLabels': ['Inactive', 'Fixed Interval', 'No Less', 'Value Change', 'While less than', 'While less than or equal to',
                                'While greater than', 'While greater than or equal to', 'While equal to', 'While not equal to'],
            'androidEnableCancelButton' : true, // default false
            'winphoneEnableCancelButton' : true, // default false
            'addCancelButtonWithLabel': 'Cancel',
            'position': [20, 40] // for iPad pass in the [x, y] position of the popover
        };
        window.plugins.actionsheet.show(lightoptions, app.bulbcallback, app.onError);
    },
    bulbcallback: function(buttonIndex) {
        //app.log("bulbcallback");
        // like other Cordova plugins (prompt, confirm) the buttonIndex is 1-based (first button is index 1)
        app.log('button index clicked: ' + buttonIndex);
        //var buff = new ArrayBuffer(32);
        //var uint32view = new Uint8Array(buff);
        app.log("hi");
        //var params = new Uint64Array(buttonIndex);
        app.log("ho");
        if (buttonIndex == 1){
            app.log("got one");
            /*
            var bparams = {
                'address': deviceId,
                'serviceUuid': serviceUuid,
                'characteristicUuid': humidityUuid
            };
            app.log("please work");
            bluetoothle.read(app.bulbwritecallback, app.onError, bparams);
            */
            app.log("didn't work");
            //app.log(readval);
            //ble.write(deviceId, serviceUuid, luxUuid, buff, app.bulbwritecallback, app.onError);
        }
        else{
            app.log("yono");
            var input_val = window.prompt("What value?", "0x0000");
            var output_val = parseInt(input_val, 10);
            if (input_val) {
                app.log("gotit");
            }
        }
    },
    bulbwritecallback: function() {
        app.log("got input");
    },
    // BLE Characteristic Read Callback
    onReadPres: function(event) {
        app.log("Pressure: " + (app.buffToUInt32Decimal(data))/10 + " " + "Pa");                 // display read value as string
    },
    // BLE Characteristic Read Callback
    onReadHum: function(data) {
        app.log("Humidity: " + (app.buffToUInt16Decimal(data))/100 + String.fromCharCode(37));                 // display read value as string
    },
    // BLE Characteristic Read Callback
    onReadTemp: function(data) {
        app.log("Temperature: " + (app.buffToInt16Decimal(data))/100 + " " + String.fromCharCode(176) + "C");                 // display read value as string
    },
    // BLE Characteristic Read Callback
    onReadLux: function(data) {
        app.log("Lux: " + app.buffToInt16Decimal(data) + " lux");                 // display read value as string
    },
    // BLE Characteristic Read Callback
    onReadAcc: function(data) {
        var acc = app.buffToUInt8Decimal(data);
        app.log("Immediate Acceleration: " + ((acc & 17) >> 4) );                 // display read value as string
        app.log("Interval Acceleration: " + (acc & 1) );                 // display read value as string

    },
    // BLE Characteristic Write Callback
    onWrite : function() {
        app.log("Characeristic Written: " + writeValue);                            // display write success
    },
    // BLE Characteristic Read/Write Error Callback
    onError: function() {                                                           // on error, try restarting BLE
        app.log("Read/Write Error.")
        ble.isEnabled(deviceId,function(){},app.onAppReady);
        ble.isConnected(deviceId,function(){},app.onAppReady);
    },
    // Function to Convert String to Bytes (to Write Characteristics)
    stringToBytes: function(string) {
        array = new Uint8Array(string.length);
        for (i = 0, l = string.length; i < l; i++) array[i] = string.charCodeAt(i);
        return array.buffer;
    },
    buffToUInt32Decimal: function(buffer) {
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
    // Function to Log Text to Screen
    log: function(string) {
        document.querySelector("#console").innerHTML += (new Date()).toLocaleTimeString() + " : " + string + "<br />"; 
    }
};

app.initialize();