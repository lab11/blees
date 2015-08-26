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

var device_connected = false;
var timer;
var touchduration = 5000; //length of time we want the user to touch before we do something
var should_i_connect = false;
var should_i_disconnect = false;

var app = {
    // Application Constructor
    initialize: function() {

        document.addEventListener("deviceready", app.onAppReady, false);
        document.addEventListener("resume", app.onAppReady, false);
        document.addEventListener("pause", app.onPause, false);

        bleesimg.addEventListener('touchend', app.onTouch, false);                // if bulb image touched, goto: onToggle

        bleesimg.addEventListener('touchstart', app.onStartTimer, false);                // if bulb image touched, goto: onToggle


        presimg.addEventListener('touchend', app.onTouchPres, false);             // if bulb image touched, goto: onToggle
        humimg.addEventListener('touchend', app.onTouchHum, false);             // if bulb image touched, goto: onToggle
        tempimg.addEventListener('touchend', app.onTouchTemp, false);             // if bulb image touched, goto: onToggle
        lightimg.addEventListener('touchend', app.onTouchLight, false);           // if bulb image touched, goto: onToggle
        accimg.addEventListener('touchend', app.onTouchAcc, false);             // if bulb image touched, goto: onToggle

        bulbimg.addEventListener('touchend', app.onTouchBulb, false);             // if bulb image touched, goto: onToggle

    },
    onStartTimer: function(device){
        should_i_connect = false;
        timer = setTimeout(app.onLongPress, touchduration); 
    },
    onLongPress: function(){
        app.log("timer expired");
        should_i_connect = true;
    },
    // App Ready Event Handler
    onAppReady: function() {
        if (window.gateway) {                                                       // if UI opened through Summon,
            deviceId = window.gateway.getdeviceId();                                // get device ID from Summon
            deviceName = window.gateway.getDeviceName();                            // get device name from Summon
        }
        document.getElementById("title").innerHTML = String(deviceId);
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
            app.log("Found " + deviceName + " (" + deviceId + ")!");
            app.onParseAdvData(device);
            ble.stopScan(app.onStopScan, app.onError);
            //app.onStartConnection(device);
        }
    },
    onStopScan: function(device) {
        app.log("stopped scanning");
    },
    onStartConnection: function(device) {
        bluetoothle.initialize(app.onInitialized, app.onError, {"request": false, "statusReceiver": false});    //initialize plugin
        bluetoothle.connect(app.onConnectOther, app.onError, { "address": deviceId });                          //connect to peripheral- need this for descriptors
        ble.connect(deviceId, app.onConnect, app.onAppReady);                                                   // if device matches, connect; if connected, goto: onConnect
    },
    onInitialized: function(device) {
        app.log("Initialized..");
    },
    onConnectOther: function(device) {
        app.log("Connecting..");
    },
    // BLE Device Connected Callback
    onConnect: function(device) {
        app.log("Connected to " + deviceName + " (" + deviceId + ")!");
        device_connected = true;
        app.onReadAllSensors(device);
        bluetoothle.discover(app.onDiscoverDescriptors, app.onError, {"address": deviceId} );        
    },
    onDiscoverDescriptors: function(device) {

        bluetoothle.readDescriptor(app.readSuccess, app.onError, {
              "address": deviceId,
              "serviceUuid": "181a",
              "characteristicUuid": "2a6f",
              "descriptorUuid": "290d"

        });

    },
    readSuccess: function(device){
        app.log("Looks like we can read descriptors...");
        app.log( (bluetoothle.encodedStringToBytes(device.value))[0] );
        app.log( (bluetoothle.encodedStringToBytes(device.value))[1] );
        app.log( (bluetoothle.encodedStringToBytes(device.value))[2] );
        app.log( (bluetoothle.encodedStringToBytes(device.value))[3] );
    },
    onTouch: function(device) {
        //ble.startScan([], app.onDiscover, app.onAppReady);                          // start BLE scan; if device discovered, goto: onDiscover
        
        clearTimeout(timer);
        if (device_connected){
            if (should_i_connect){
                app.log("Disconnecting from BLEES device!");
                bluetoothle.disconnect(app.ondisconnectsuccess, app.onError, { "address": deviceId});                
                ble.disconnect(deviceId);
                device_connected = false;
                app.onAppReady();
            }
            else { 
                app.onReadAllSensors(device);
            }
        }
        else {
            if (should_i_connect) {
                app.log("Connecting to BLEES device!");
                app.onStartConnection(device);
            }
            else {
                app.log("Scanning...");
                ble.startScan([], app.onDiscover, app.onAppReady);                          // start BLE scan; if device discovered, goto: onDiscover
            }
        }
        should_i_connect = false;
        

    },
    ondisconnectsuccess: function() {
        app.log("disconnected success!");
    },
    onTouchPres: function(device) {
        if(device_connected){
            app.log("Getting pressure...");
            ble.read(deviceId, serviceUuid, pressureUuid, app.onReadPres, app.onError);
        } 
        else{
            app.log("Please connect to use this feature.")
        }
    },
    onTouchHum: function(device) {
        if(device_connected){
            app.log("Getting humidity...");
            ble.read(deviceId, serviceUuid, humidityUuid, app.onReadHum, app.onError);
        }
        else{
            app.log("Please connect to use this feature.")
        }
    },
    onTouchTemp: function(device) {
        if(device_connected){
            app.log("Getting temperature...");
            ble.read(deviceId, serviceUuid, temperatureUuid, app.onReadTemp, app.onError);
        }
        else{
            app.log("Please connect to use this feature.")
        }
    },
    onTouchLight: function(device) {
        if(device_connected){
            app.log("Getting lux...");
            ble.read(deviceId, serviceUuid, luxUuid, app.onReadLux, app.onError);  
        }
        else{
            app.log("Please connect to use this feature.")
        }
    },
    onTouchAcc: function(device) {
        if(device_connected){
            app.log("Getting acceleration...");
            ble.read(deviceId, serviceUuid, accelerationUuid, app.onReadAcc, app.onError);  
        }
        else{
            app.log("Please connect to use this feature.")
        }
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
    onParseAdvData: function(device){
        //Parse Advertised Data
        var adData = new Uint8Array(device.advertising);
        app.log("Parsing advertised data...");
        app.log( "Pressure: " + (( (adData[17] * 16777216) + (adData[16] * 65536 ) + (adData[15] * 256) + adData[14] )/10) + " Pa" );
        app.log( "Humidity: " + (( (adData[19] * 256) + adData[18] )/100) + String.fromCharCode(37) );
        app.log( "Temperature: " + (( (adData[21] * 256) + adData[20])/100) + " " + String.fromCharCode(176) + "C");
        app.log( "Lux: " + ( (adData[23] * 256) + adData[22]) + " lux" );
        var accdata = adData[24];
        app.log("Immediate Acceleration: " + ((accdata & 17) >> 4) );
        app.log("Interval Acceleration: " + (accdata & 1) );
    },
    onReadAllSensors: function(device) {
        app.log("Getting sensor data...");
        ble.read(deviceId, serviceUuid, pressureUuid, app.onReadPres, app.onError);
        ble.read(deviceId, serviceUuid, humidityUuid, app.onReadHum, app.onError);  
        ble.read(deviceId, serviceUuid, temperatureUuid, app.onReadTemp, app.onError); 
        ble.read(deviceId, serviceUuid, luxUuid, app.onReadLux, app.onError);  
        ble.read(deviceId, serviceUuid, accelerationUuid, app.onReadAcc, app.onError);
    },
    // BLE Characteristic Read Callback
    onReadPres: function(data) {
        //app.log("where is pressure");
        app.log("Pressure: " + (app.buffToUInt32Decimal(data))/10 + " Pa");                 // display read value as string
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
    // Function to Log Text to Screen
    log: function(string) {
        document.querySelector("#console").innerHTML += (new Date()).toLocaleTimeString() + " : " + string + "<br />"; 
        document.querySelector("#console").scrollTop = document.querySelector("#console").scrollHeight;
    }
};

app.initialize();