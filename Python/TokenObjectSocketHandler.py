import functools
import types
from ..WebService import SocketHandlerBase
from traceback import format_exc

class TokenObjectSocketHandler(SocketHandlerBase.SocketHandlerBase):
    def __init__(self, token, validator):
        SocketHandlerBase.SocketHandlerBase.__init__(self, validator)
        self.token = token
        self.__init_change_handlers()

    # -----------------------------------------------------------------------------------
    def SendUpdate(self, excludeOwnerID=None, **kwargs):
        kwargs["item_id"] = self.token.item_id
        self.Send("tokenUpdate_%s" % self.token.item_id, excludeOwnerID, **kwargs)

    # -----------------------------------------------------------------------------------
    def SendAttrUpdate(self, excludeOwnerID=None, **kwargs):
        kwargs["item_id"] = self.token.item_id
        self.Send("tokenAttrUpdate_%s" % self.token.item_id, excludeOwnerID, **kwargs)

    # -----------------------------------------------------------------------------------
    def OnItemChanged(self, item, key, value):
        a = {"item_id": item.item_id}
        if key in ["x", "y", "z"]:
            a["position"] = (item.x, item.y, item.z)
        else:
            a[key] = value
        self.SendUpdate(**a)

    # -----------------------------------------------------------------------------------
    def OnAttributeChanged(self, item, key, value):
        a = {"item_id": item.item_id, "attr_id": key}
        a[key] = value
        self.SendAttrUpdate(**a)

    # -----------------------------------------------------------------------------------
    #   The Change Event handling from the
    # -----------------------------------------------------------------------------------
    def __Change__(self, propID, oldVal, newVal, _userID, *args, **kwargs):
        prop = getattr(self.token, propID)
        try:
            oldVal = prop.__class__(oldVal)
            newVal = prop.__class__(newVal)
        except Exception, e:
            res = {"Status": "FAIL", "message": e}
            res[propID] = prop
            return res

        if getattr(self.token, propID) == oldVal:
            valRes = self.ValidateProperty(self.token, _userID, propID, oldVal, newVal)
            if valRes is not False:
                if isinstance(valRes, types.TupleType):
                    newVal = valRes[0]
                setattr(self.token, propID, newVal)
                self.token.BroadcastToListeners("OnItemChanged", propID, newVal, exclude=self)
                upd = {"excludeOwnerID": _userID}
                upd[propID] = newVal
                self.SendUpdate(**upd)

                res = {"Status": "OK"}
                res[propID] = newVal
                return res

        res = {"Status": "FAIL"}
        res[propID] = getattr(self.token, propID)
        return res

    # -----------------------------------------------------------------------------------
    def __AttrChange__(self, propID, oldVal, newVal, _userID, *args, **kwargs):
        if (getattr(self.token.item.attributes, propID) == oldVal):
            self.token.item.attributes.__setattr_no_broadcast__(propID, newVal)
            self.token.BroadcastToListeners("OnAttributeChanged", propID, newVal, exclude=self)
            upd = {"item_id": self.token.item_id, "attr_id": propID}
            upd[propID] = newVal
            self.SendAttrUpdate(**upd)
            res = {"Status": "OK"}
            res[propID] = newVal
            return res
        else:
            res = {"Status": "FAIL"}
            res[propID] = getattr(self.token.item.attributes, propID)
            return res

    # -----------------------------------------------------------------------------------
    def __init_change_handlers(self):
        self.ChangeQuantity = functools.partial(self.__Change__, "quantity")
        self.ChangeLocation_id = functools.partial(self.__Change__, "location_id")
        self.ChangeSub_location = functools.partial(self.__Change__, "sub_location")
        self.ChangeType_id = functools.partial(self.__Change__, "type_id")
        self.ChangeOwner_id = functools.partial(self.__Change__, "owner_id")
        self.ChangeQuantity = functools.partial(self.__Change__, "quantity")
        self.ChangeName = functools.partial(self.__Change__, "name")
        self.ChangePosition = functools.partial(self.__Change__, "position")
        # self.ChangeX = functools.partial(self.__Change__, "x")
        # self.ChangeY = functools.partial(self.__Change__, "y")
        # self.ChangeZ = functools.partial(self.__Change__, "z")
        self.ChangeRotation = functools.partial(self.__Change__, "rotation")
        self.ChangeVisible = functools.partial(self.__Change__, "visible")
        self.ChangeRespath = functools.partial(self.__Change__, "respath")
        self.ChangeSize = functools.partial(self.__Change__, "size")
        self.ChangeType_name = self.ChangeDummy
        self.ChangeTokentype = self.ChangeDummy

        for k in self.token.item.attributes.GetWeb().iterkeys():
            key = "AttrChange" + k[0:1].upper() + k[1:].lower()
            setattr(self, key, functools.partial(self.__AttrChange__, k))

    # -----------------------------------------------------------------------------------
    def ChangeDummy(self, *args, **kwargs):
        """ A dummy change fucntion to support the dummy property"""
        return {"Status": "OK"}

    # -----------------------------------------------------------------------------------
    def GetMenus(self, *args, **kwargs):
        try:
            menu = self.token.GetWebMenus()
            if menu is not None:
                return {"Status": "OK", "menu": menu}
            else:
                return {"Status": "FAIL"}
        except Exception, e:
            return {"Status": "EXCEPTION", "message": str(e) + "\n" + format_exc(), "item_id": self.token.item_id}

    # -----------------------------------------------------------------------------------
    def DoMenuAction(self, menuAction, params, *args, **kwargs):
        try:
            print "sock::DoMenuAction", menuAction, args, kwargs
            res = self.token.DoMenuAction(menuAction, params, *args, **kwargs)
            if res is not None:
                return {"Status": "OK", "result": res}
            else:
                return {"Status": "FAIL"}
        except Exception, e:
            return {"Status": "EXCEPTION", "message": str(e) + "\n" + format_exc(), "item_id": self.token.item_id}

    # -----------------------------------------------------------------------------------
    def SetActive(self, *args, **kwargs):
        self.token.renderSet.add("**ALL**")
        self.token.renderSet.add("**TABLE**")
        self.token.attributes.NPCActive = 1
        self.token.attributes.tokenDraggable = 1
        self.token.attributes.GMOnly = 0
        self.token.visible = 1

    # -----------------------------------------------------------------------------------
    def SetInactive(self, *args, **kwargs):
        self.token.renderSet.clear()
        self.token.attributes.NPCActive = 0
        self.token.attributes.tokenDraggable = 0
        self.token.attributes.GMOnly = 1
        self.token.visible = 0
