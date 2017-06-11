
function applog(text) {
    console.log(text);
    var node = document.createElement("p");
    var t = document.createTextNode(text);
    node.appendChild(t);
    document.getElementById("console").appendChild(node);
}


document.getElementById("find").onclick = function () { 
    applog("Button Clicked - Searching for devices!");
    let filters = [];
    navigator.bluetooth.requestDevice({ 
        filters: [{
            services: ['c098e5c0-0000-0000-0000-001300000000']
        }]
    }).then(device => {
        document.getElementById("title").innerHTLM = String(device.id)
        return device.gatt.connect()
    }).then(server => {
        return server.getPrimaryService(parseInt('0xc098e5c0000000000000001300000000'))
    }).then(service => {
        return service.getCharacteristic(parseInt('0x1301'))
        return service.getCharacteristic(parseInt('0x1302'))
        return service.getCharacteristic(parseInt('0x1303'))
    }).then(characteristic => {
        if(characteristic.readValue() == 1) {
            document.getElementById("tempVal").ineerHTML = "yes"
        } else {
            document.getElementById("tempVal").ineerHTML = "no"
        }
    })
}
