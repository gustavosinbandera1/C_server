var WebSocketClient = require('websocket').client;
const data = "45,PIT905,01,01,00,05,04,4,4,4,0,0,0,071";
var client = new WebSocketClient();
 

function getCRC(inputString) {
    var Total = 0;
    for (var i = 0; i < inputString.length; i++) {
        var hex = inputString[i].charCodeAt(0).toString(16);
        Total += parseInt(hex, 16);
    }
    var hexACK = Total.toString(16);
    var ascACK = parseInt(hexACK.substr(hexACK.length - 2, hexACK.length), 16); //ASCII VALUE OF LAST TWO DIGITS OF HEX ACK

    if (ascACK <= 9) //single digit ACK
        ascACK = "00" + ascACK;
    else if (ascACK <= 99) //two digit ACK
        ascACK = "0" + ascACK;

    return ascACK;
}

client.on('connectFailed', function(error) {
    console.log('Connect Error: ' + error.toString());
});
 
client.on('connect', function(connection) {
    console.log('WebSocket Client Connected');
    connection.on('error', function(error) {
        console.log("Connection Error: " + error.toString());
    });
    connection.on('close', function() {
        console.log('websocket Connection Closed');
    });
    connection.on('message', function(message) {
        if (message.type === 'utf8') {
            getCRC(message);
            console.log("Received: '" + message.utf8Data + "'");
        }
    });
    
    function sendPacket() {
        if (connection.connected) {
            connection.sendUTF(data.toString());
            setTimeout(sendPacket, 2000);
        }
    }
    sendPacket();
});
 
//client.connect('ws://localhost:2579/', 'lws-server-status');
//client.connect('ws://localhost:2579/', 'dumb-increment-protocol');
//client.connect('ws://localhost:2579/', 'lws-status');
//client.connect('ws://localhost:2579/', 'lws-mirror-protocol');
//client.connect('ws://localhost:7681/', 'lws-minimal');
//client.connect('ws://localhost:7681/', 'example-standalone-protocol');
client.connect('ws://localhost:7681/', 'lws-websocket');