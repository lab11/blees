
function applog(text) {
    console.log(text);
    var node = document.createElement("p");
    var t = document.createTextNode(text);
    node.appendChild(t);
    document.getElementById("console").appendChild(node);
}

var lastUpdateChar;
var lastMinChar


document.getElementById("find").onclick = function () { 
    applog("Button Clicked - Searching for devices!");
    let filters = [];
    navigator.bluetooth.requestDevice({ 
        filters: [{
            services: ['c3050f56-bb8c-96ab-da48-d04ebe3ba6c0']
        }]
    }).then(device => {
        applog('Got device! tring to connect...');
        applog(device.id);

        setTimeout(function() {
            document.getElementById("title").innerHTML = String(device.id);
        }, 0);

        return device.gatt.connect()
    }).then(server => {
        applog('Connected - trying to read service');
        return server.getPrimaryService('c3050f56-bb8c-96ab-da48-d04ebe3ba6c0');
    }).then(service => {
        applog('Getting characteristics');
        service.getCharacteristic('c3050f58-bb8c-96ab-da48-d04ebe3ba6c0').then(characteristic => {
            lastUpdateChar = characteristic;
            var intervalID = setInterval(readLastUpdate,1000);
        });
        service.getCharacteristic('c3050f59-bb8c-96ab-da48-d04ebe3ba6c0').then(characteristic => {
            lastMinChar = characteristic;
            var intervalID = setInterval(readLastMin,1000);
        });
    })
}

function readLastUpdate() {
    lastUpdateChar.readValue().then(value => {
        if(value.getUint8(0) == 1) {
            applog("setting last to yes");
            document.getElementById("tempVal").innerHTML = "yes"
        } else {
            applog("setting last to no");
            document.getElementById("tempVal").innerHTML = "no"
        }
    });
}

function readLastMin() {
    lastMinChar.readValue().then(value => {
        if(value.getUint8(0) == 1) {
            applog("setting last to yes");
            document.getElementById("luxVal").innerHTML = "yes"
        } else {
            applog("setting last to no");
            document.getElementById("luxVal").innerHTML = "no"
        }
    });
}
