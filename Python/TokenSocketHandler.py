from ..WebService import SocketHandlerBase
from traceback import format_exc

class TokenSocketHandler(SocketHandlerBase.SocketHandlerBase):

    def __init__(self, tokenMgr, validator):
        SocketHandlerBase.SocketHandlerBase.__init__(self, validator)

        self.tokenMgr = tokenMgr

    # -----------------------------------------------------------------------------------
    def SubscribeToken(self, tokenID, _userID=None, *args, **kwargs):
        try:
            tok = self.tokenMgr.GetToken(int(tokenID))
            if tok is not None:
                web = tok.Subscribe(_userID, self.webListeners[_userID], self.validator)
                self.webListeners[_userID].RegisterHandler(*web)
            return {"Status": "OK",
                    "HandlerID": web[0],
                    "Token": tok.GetWeb()
                    }
        except Exception, e:

            return {"Status": "Exception", "message": str(e) + "\n" + format_exc(), "item_id": tokenID}

    # -----------------------------------------------------------------------------------
    def UnsubscribeToken(self, tokenID, _userID=None, *args, **kwargs):
        try:
            tok = self.tokenMgr.GetToken(int(tokenID))
            if tok is not None:
                hndlID = tok.Unsubscribe(_userID)
                self.webListeners[_userID].UnregisterHandler(hndlID)
            return {"Status": "OK"}
        except Exception, e:
            return {"Status": "Exception", "message": str(e)}

    # -----------------------------------------------------------------------------------
    def CheckOwnership(self, tokenID, userID):
        tok = self.tokenMgr.GetToken(int(tokenID))
        self.ValidateProperty(tok, userID, None, None, None)

    # -----------------------------------------------------------------------------------
    def AddListener(self, userID, socket):
        self.webListeners[userID] = socket

    # -----------------------------------------------------------------------------------
    def RemoveListener(self, userID, socket):
        if userID in self.webListeners:
            del self.webListeners[userID]

    # -----------------------------------------------------------------------------------
    def Send(self, eventName, excludeUserID=None, **kwargs):
        for userID, socket in self.webListeners.iteritems():
            if excludeUserID is None or excludeUserID == userID:
                continue
            socket.Send(eventName, **kwargs)

    # -----------------------------------------------------------------------------------
    def SendTo(self, userID, eventName, **kwargs):
        if userID in self.webListeners:
            self.webListeners[userID].Send(eventName, **kwargs)
