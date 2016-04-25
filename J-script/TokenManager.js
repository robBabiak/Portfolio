/* -----------------------------------------------------------------------------------
    Token:
        This is a wrapper object for a token and handles the two way communication to the
        server, has similar properties to that of a inventory item.

    Events: (<token>, <name>, <state>, <data>)
        on_<property name>  - will fire with states of [OK, UPDATE, FAIL] when the value changes.


    TokenManager:
        The token manager will handle the collection of tokens that are visible to this client.
        tokens will be added and removed as they become visible to the client.

    Functions:
        PrimeToken          - This will prime an token by asking the server for the token, and subscribing
                              to update on this token.
        PrimeTokens         - Same as PrimeItem but can pass multiple itemID to prime as parameters.
        GetToken            - Gets a token that has been previously primed.

    Events: (<tokenID>, <token>)
        on_update_token     - will fire when an update is received for any token. <token> is the update to apply
        on_add_token        - will fire when a child token is added to a token.
        on_remove_token     - will fire when a child token is removed from an token
        on_prime_token      - will fire when a new token is primed  and the data has been received.
        on_unprime_token    - will fire when a token has requested unsubscripted from the server.
                              the token is removed from the local invItems even if the server call fails.

   ----------------------------------------------------------------------------------- */

// -----------------------------------------------------------------------------------
function Token(tokenManager, data)
{
    try
    {
        this.tokenManager = tokenManager;
        this.tokenTokenID = "Token_" + data.item_id;
        this.data = data;
        this.attributes = {};

        var self = this;
        var i;
        for (i in data)
        {
             if (i === "attributes")
                continue;

            (function(self, i, mgr)
            {
                Object.defineProperty(self, i,
                {
                    get: function ()
                    {
                        var k = i;
                        return self.data[k];
                    },
                    set: function(v)
                    {
                        var k = i;
                        func = function(result)
                        {
                            var k = i;
                            if (result.Status  === "OK")
                            {
                                self.data[k] = result[k];
                                self.TriggerEvent(k, "OK", v);
                            }
                            else if (result.Status === "FAIL")
                            {
                                self.data[k] = result[k];
                                self.TriggerEvent(k, "FAIL", result[k]);
                            }
                        };
                        var funcName = self.tokenTokenID + ".Change" + k.charAt(0).toUpperCase() + k.substr(1).toLowerCase();
                        mgr.webSocket.RPC_CALL(funcName, func, self.data[k], v );
                        self.TriggerEvent(k, "SET", v);

                    },
                    enumerable: true,
                    configurable: false
                });
            }) (self, i, tokenManager);
        }

        for (i in data.attributes)
        {
            (function(self, i, mgr)
            {
                Object.defineProperty(self.attributes, i,
                {
                    get: function ()
                    {
                        var k = i;
                        return self.data.attributes[k];
                    },
                    set: function(v)
                    {
                        var k = i;
                        func = function(result)
                        {
                            var k = i;
                            if (result.Status  === "OK")
                            {
                                self.data.attributes[k] = result[k];
                                self.TriggerEvent(k, "OK", v);
                            }
                            else if (result.Status === "FAIL")
                            {
                                self.data.attributes[k] = result[k];
                                self.TriggerEvent(k, "FAIL", result[k]);
                            }
                        };
                        var funcName = self.tokenTokenID + ".AttrChange" + k.charAt(0).toUpperCase() + k.substr(1).toLowerCase();
                        mgr.webSocket.RPC_CALL(funcName, func, self.data.attributes[k], v );
                        self.TriggerEvent(k, "SET", v);

                    },
                    enumerable: true,
                    configurable: false
                });
            }) (self, i, tokenManager);
        }

        // console.log("New Token ", self.item_id, self.menus);


    }
    catch (ex)
    {
        console.error("Inventory Manager exception:" + ex.stack, ex);
    }
}

// -----------------------------------------------------------------------------------
Token.prototype.GetMenus = function(func)
{
    funcName = this.tokenTokenID + ".GetMenus";
    this.tokenManager.webSocket.RPC_CALL(funcName, func);
};

// -----------------------------------------------------------------------------------
Token.prototype.DoMenuAction = function(evnt)
{
    var menuData = evnt.data.menuData;
    var token = evnt.data.token;
    var funcName = token.tokenTokenID + ".DoMenuAction";
    func = function(result)
    {
        if (result.Status  === "OK")
        {
            // console.log("Menu action OK");
        }
        else if (result.Status === "FAIL")
        {
            console.log("Menu action Fail", result, evnt);
        }
    };

    token.tokenManager.webSocket.RPC_CALL(funcName, func, menuData.action, menuData.params);
};

// -----------------------------------------------------------------------------------
Token.prototype.DoEvent = function()
{
    var funcName = this.tokenTokenID + "." + arguments[0];
    var args = [];
    args.push(funcName);
    for (var i = 1; i < arguments.length; i++)
    {
        args.push(arguments[i]);
    }

    console.log("token.DoEvent", args);
    this.tokenManager.webSocket.Send.apply(token.tokenManager.webSocket, args);
};

// -----------------------------------------------------------------------------------
Token.prototype.TriggerEvent = function( name, state, data, oldValue)
{
    var evenName = "on_" + name;
    $(window).trigger("on_" + this.item_id + "_" + name, [name, state, data, oldValue]);
    // console.log("Token.TriggerEvent", "on_" + this.item_id + "_" + name, [name, state, data, oldValue]);
    if (this.hasOwnProperty(evenName))
    {
        this[evenName](this, name, state, data, oldValue);
    }

};

// -----------------------------------------------------------------------------------
Token.prototype.Update = function( delta )
{
    for (var k in delta)
    {
        if (k === "item_id") continue;  // Every update sends the item_id so we know what is being updated, but item_id is immutable
        if (delta.hasOwnProperty(k) && this.data.hasOwnProperty(k))
        {
            var oldValue = this.data[k];
            this.data[k] = delta[k];
            if (k === "location_id")
            {
                $(window).trigger("on_" + oldValue + "leave_location", token);
                $(window).trigger("on_" + delta[k] + "enter_location", token);
            }
            this.TriggerEvent(k, "UPDATE", delta[k], oldValue);
        }
    }
};

// -----------------------------------------------------------------------------------
Token.prototype.UpdateAttr = function( delta )
{
    for (var k in delta)
    {
        if (k === "item_id" || k ==="attr_id") continue;  // Every update sends the item_id so we know what is being updated, but item_id is immutable
        if (delta.hasOwnProperty(k) && this.data.attributes.hasOwnProperty(k))
        {
            var oldValue = this.data.attributes[k];
            this.data.attributes[k] = delta[k];
            this.TriggerEvent(k, "UPDATEATTR", delta[k], oldValue);
        }
    }
};

// -----------------------------------------------------------------------------------
Token.prototype.OnDragEnd = function( event, marker, plugin )
{
    pos = plugin.getXYCoordinate(marker.getLatLng());
    // console.log("OnDragEnd",  pos, this);
    this.position = [pos.x, pos.y, this.position[2]];

};

// -----------------------------------------------------------------------------------
Token.prototype.IsDraggable = function()
{
    // console.log("IsDraggable", this.item_id, this.attributes.tokenDraggable);
    if (this.attributes.GMOnly == 1 || this.owner_id === 0)
    {
        return this.attributes.tokenDraggable !== 0;
    }
    else
    {
        return this.attributes.tokenDraggable !== 0 && this.owner_id == userID;
    }
};

// -----------------------------------------------------------------------------------
Token.prototype.HasMenu = function()
{
    return this.attributes.tokenHasMenu !== 0;
};

// -----------------------------------------------------------------------------------
Token.prototype.GMOnly = function()
{
    if (this.data.attributes.hasOwnProperty("GMOnly") )
    {
        return this.data.attributes.GMOnly;
    }
    return 0;
};


/* -----------------------------------------------------------------------------------
   -- TokenManager.js
   -- Copyright Robert Babiak, 2016
   ----------------------------------------------------------------------------------- */

// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
function TokenManager(socket)
{
    try
    {
        var self=this;
        this.webSocket = socket;
        this.tokens = {};
        this.GMMode = false;

        // Make a remote fucniton, that can be called on the web socket and will execute the remote function.
        this.webSocket.MakeRemoteFunction("SubscribeToken", "token.SubscribeToken", function (token)
        {
            (function (self, token)
            {
                // Make a new Inventory Item, and add it to the inventory manager
                if (token.Status !== "OK")
                {
                    delete self.tokens[token.item_id];
                    console.error("Subscribe was not OK " + token.Status + " | " + token.message);
                    return;
                }
                i = new Token(self, token.Token);
                self.tokens[i.item_id] = i;
                // console.log("Primed Token", i);
                self.TriggerEvent("prime_token", i.item_id, i);
                $(window).trigger("on_" + token.Token.location_id + "_enter_location", i);

                if (self.tokens.hasOwnProperty(i.location_id))
                {
                    self.tokens[i.location_id].TriggerEvent("child_primed", "OK", i);
                }

                // add a  opcode to handle invUpdates for this item to the socket.
                self.webSocket.addOpcodeFunction("tokenUpdate_" + token.Token.item_id, function(opcode, token)
                {
                    // if we have the item then call update on it.
                    if (self.tokens.hasOwnProperty(token.item_id))
                    {
                        self.tokens[token.item_id].Update(token);
                        self.TriggerEvent("update_token", token.item_id, token);
                    }
                }, self);
                // add a  opcode to handle invUpdates for this item to the socket.
                self.webSocket.addOpcodeFunction("tokenAttrUpdate_" + token.Token.item_id, function(opcode, token)
                {
                    // if we have the item then call update on it.
                    if (self.tokens.hasOwnProperty(token.item_id) && self.tokens[token.item_id].attributes.hasOwnProperty(token.attr_id))
                    {
                        self.tokens[token.item_id].UpdateAttr(token);
                        self.TriggerEvent("update_token_attribute", token.item_id, token);
                    }
                }, self);
                self.webSocket.addOpcodeFunction("tokenMenuUpdate_" + token.Token.item_id, function(opcode, menu)
                {
                    // if we have the item then call update on it.
                    if (self.tokens.hasOwnProperty(token.item_id))
                    {
                        self.tokens[token.item_id].UpdateMenu(menu);
                        self.TriggerEvent("update_token_menu", token.item_id, menu);
                    }
                }, self);
            })(self, token);
        });

        self.webSocket.addOpcodeFunction("tokenAdd", function(opcode, data)
        {
            if (!self.tokens.hasOwnProperty(data.token.item_id))
            {
                self.PrimeToken(data.token.item_id);

                self.tokens[data.token.item_id] = null; //new Token(self, data.token);
                self.TriggerEvent("add_token", data.token.item_id, data.token);
            }
        }, self);

        self.webSocket.addOpcodeFunction("tokenRemove", function(opcode, data)
        {
            // if we have the item then call update on it.
            if (self.tokens.hasOwnProperty(data.tokenID))
            {
                $(window).trigger("on_" + token.location_id + "_leave_location", token);
                token = self.tokens[data.tokenID];
                delete self.tokens[data.tokenID];

                self.TriggerEvent("remove_token", data.tokenID, token);
            }
        }, self);

        this.webSocket.MakeRemoteFunction("UnsubscribeToken", "inventory.UnsubscribeIToken", function (token)
        {
            (function (self, token)
            {
                if (item.Status !== "OK")
                {
                    console.error("Unsubscribe was not OK " + token.Status + " | " + token.message);
                    return;
                }
            })(self, token);
        });

        // This will notify all listeners on these locations that this token is already primed.
        $(window).on("mapDisplayInitalized", function(evnt, data)
        {
            for (var x in self.tokens)
            {
                var token = self.tokens[x];
                if (token !== null)
                {
                    console.log("   ", x, self.tokens[x], token);
                    $(window).trigger("on_" + token.location_id + "_enter_location", token);
                }
            }
        });
        $(window).on("onToggleGMControlls", function (evnt, showControls)
        {
            this.GMMode = showControls;
            for (var tokenID in self.tokens)
            {
                var token = self.tokens[tokenID];
                if ( token.GMOnly() === 1 && token.marker !== undefined && token.marker !== null)
                {
                    token.marker.setVisibility(showControls ? 1 : 0);
                }
            }
        });
        $(window).trigger("TokenManagerInitalized", this);

    }
    catch (ex)
    {
        console.error("Token Manager exception:"  + ex.stack, ex);
    }
}


// -----------------------------------------------------------------------------------
TokenManager.prototype.PrimeToken = function( tokenID )
// This will prime a location and establish the two way conection to the server.
{
    if (!this.tokens.hasOwnProperty(tokenID))
    {
        (function (that, socket, tokenID)
        {
            that.tokens[tokenID] = null;
            socket.SubscribeToken( tokenID );
        })(this, this.webSocket, tokenID);
    }

};

// -----------------------------------------------------------------------------------
TokenManager.prototype.UnprimeToken = function( tokenID )
// This will prime a location and establish the two way conection to the server.
{
    if (!this.tokens.hasOwnProperty(tokenID))
    {
        (function (that, socket, tokenID)
        {
            socket.UnsubscribeItem( tokenID );
            if (self.tokens.hasOwnProperty(tokenID))
            {
                var token = self.tokens[tokenID];
                delete self.tokens[tokenID];
                that.TriggerEvent("unprime_token", tokenID, token);
            }

        })(this, this.webSocket, tokenID);
    }

};

TokenManager.prototype.PrimeTokens = function( )
// This will prime a location and establish the two way conection to the server.
{
    for (var i = 0; i < arguments.length; i++)
    {
        (function (that, f)
        {
            that.PrimeToken(f);
        }) (this, arguments[i]);
    }

};

TokenManager.prototype.PrimeTokenList = function( lst )
// This will prime a location and establish the two way conection to the server.
{
    for (var i = 0; i < lst.length; i++)
    {
        (function (that, f)
        {
            that.PrimeToken(f);
        }) (this, lst[i]);
    }

};

// -----------------------------------------------------------------------------------
TokenManager.prototype.GetToken = function( tokenID )
// This will prime a location and establish the two way conection to the server.
{
    if (!this.tokens.hasOwnProperty(tokenID))
    {
        return "**UNPRIMED**";
    }
    if (this.tokens[tokenID] === null)
    {
        return "**PRIMING**";
    }
    return this.tokens[tokenID];
};

// -----------------------------------------------------------------------------------
TokenManager.prototype.TriggerEvent = function( name, tokenID, token)
{
    var eventName = "on_" + name;
    $(window).trigger(eventName, [tokenID, token]);
    $(window).trigger("on_" + tokenID + "_" + name, [tokenID, token]);
    // console.log("TokenManager::TriggerEvent:", eventName, token);
    if (this.hasOwnProperty(eventName))
    {
        this[eventName](tokenID, token);
    }
};
// -----------------------------------------------------------------------------------
TokenManager.prototype.GetTokenOfType = function(typeName)
{
    for(var tokenID in this.tokens)
    {
        if (this.tokens[tokenID].tokenType === typeName)
            return this.tokens[tokenID];
    }
};
