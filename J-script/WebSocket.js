/* -----------------------------------------------------------------------------------
   -- WebSocket.js
   -- Copyright Robert Babiak, 2016
   ----------------------------------------------------------------------------------- */

// -----------------------------------------------------------------------------------
function ProtocolWebSocket(port, protocol, name, protoFuncs, callback)
{
    try
    {
        var host = "ws://" + window.location.hostname + ":" + port + "/" + name;
        console.log(host);
        console.log(protocol);
        ProtocolWebSocket_self=this;
        this.rpcID = 0;
        this.rpcCalls = {};
        this.handlers = protoFuncs;
        this.socket = new WebSocket(host, protocol);
        this.socket.protocolHandler = this;
        this.socket.onopen = function (e)
        {
            console.log("Socket opened." + e);
            $(window).trigger("webSocketInitalized", ProtocolWebSocket_self);
            if (typeof(callback) === 'function') {
                callback();
            }
        };

        this.socket.onclose = function (e)
        {
            console.log("Socket closed." + e );
        };

        this.socket.onmessage = this.OnMessage;

        this.socket.onerror = function (e)
        {
            console.log("Socket error:", e);
        };
    }
    catch (ex)
    {
        console.log("Socket exception:", ex);
    }
}

// -----------------------------------------------------------------------------------
ProtocolWebSocket.prototype.OnMessage = function(e)
// Receive a message from the server.
// For RPC calls we store a callback that will be executed when the returned message is
// sent from the server. If the server fails to send a response (error) the callback
// function will be abandoned and the web socket will leak memory.
{
    var data = JSON.parse(e.data);
    var opcode = data["opcode"];
    if (opcode.indexOf("__RPC.") === 0)
    {
        // WE have a RPC callback
        if ((typeof this.protocolHandler.rpcCalls[opcode] !== "undefined") )
        {
            // Look up the RPC call in the stored callbacks and
            // execute the callback function with the data.
            this.protocolHandler.rpcCalls[opcode](data["data"]);
            // remove the expected result return.
            delete this.protocolHandler.rpcCalls[opcode];
        }
        else
        {
            console.error("Unknown rpc callback!" + opcode);

        }
        return;
    }

    // If the opdcode is defined in the object, then this is a protocol message.
    // find the handler for this opcode and call it.
    if ((typeof opcode !== "undefined") && this.protocolHandler.handlers.hasOwnProperty(opcode))
    {
        var fn = this.protocolHandler.handlers[opcode];
        if (fn.constructor === Array)
        {
            fn[0].call(fn[1], opcode, data.data);
        }
        else
        {
            fn.call(this, opcode, data.data);
        }
    }
    else
    {
        console.error("received unknown protocol message :", e.data);
    }
};

// -----------------------------------------------------------------------------------
// The basic send to the server. Take the arguments and bundle them into an object
// with the opcode (1st arg) and then JSON serialize it.
ProtocolWebSocket.prototype.Send = function()
{
    var opcode = arguments[0];
    var args = [];
    for (var i = 1; i < arguments.length; i++)
    {
        args.push(arguments[i]);
    }
    var dat = {opcode:opcode, args:args};
    var strData = JSON.stringify(dat);
    this.socket.send(strData);
};

// -----------------------------------------------------------------------------------
// Make a RPC call. This is simular to send, but store the handler to be called
// on the response from the server. IF the server fails to send a response
// then the handler will be stranded and hang around forever.
ProtocolWebSocket.prototype.RPC_CALL = function( opcode,  returnCode)
{
    var args = [];
    for (var i = 2; i < arguments.length; i++)
    {
        args.push(arguments[i]);
    }
    var dat = {opcode:opcode, args:args};
    if(typeof returnCode !== "undefined")
    {
        this.rpcID = this.rpcID + 1;

        dat.rpcCall = opcode + "_" + (this.rpcID);
        this.rpcCalls["__RPC." + dat.rpcCall] = returnCode;
    }
    this.socket.send(JSON.stringify(dat));
};

// -----------------------------------------------------------------------------------
// Make a remote function, this creates a function on the web socket that can be called
// as if it was a local function, but behind the scene will make a RPC call to the server
ProtocolWebSocket.prototype.MakeRemoteFunction = function(name, opcode, returnCode)
{
    ProtocolWebSocket.prototype[name] = ProtocolWebSocket.prototype.RPC_CALL.bind(this, opcode, returnCode);
};

// -----------------------------------------------------------------------------------
// Close the socket
ProtocolWebSocket.prototype.close = function()
{
    this.socket.close();
    this.socket = null;
}

// -----------------------------------------------------------------------------------
// This will allow an opcode to be added with a handler and the object to call it on
// lthis allows for the server to send data to the clients that will be handled
// by what ever system needs it. This is the primary mechanism in which clients receive
// updates based on other client actions.
ProtocolWebSocket.prototype.addOpcodeFunction = function(opcode, func, handlerObj)
{
    if (handlerObj !== undefined)
    {
        this.handlers[opcode] = [func, handlerObj];
    }
    else
    {
        console.error("addOpcodeFunction without handleObject is depricated");
        this.handlers[opcode] = func;
    }
}
