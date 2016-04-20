from Util import KeyVal, AddConst

SERVICE_STATES = KeyVal(NONE=0, RUNNING=1, STOPPED=2)
AddConst("SERVICE_STATES", SERVICE_STATES)


class Service(object):
    __ID__ = "*UNDEFINED*"
    #__EVENTS__ = [] # registered events
    #__DEPENDANT_SERVICES__ = [] # This service depends on the following

    def __init__(self, service_manager):
        self.serviceManager = service_manager
        self.state = SERVICE_STATES.NONE

    # -----------------------------------------------------------------------------------
    def OnPreInit(self):
        """
            This will be the pre initialization, all services must be in a state that they
            can be connected to but not functional.
        """
        return True

    # -----------------------------------------------------------------------------------
    def OnInit(self):
        """
            Services can do there initialization here that depends on other services
        """
        self.state = SERVICE_STATES.RUNNING

    # -----------------------------------------------------------------------------------
    def OnPostInit(self):
        """
            Services can do any initialization that requires starting having the service in a running state first
        """
        pass

    # -----------------------------------------------------------------------------------
    def OnSystemShutdown(self):
        """
            The system is shutting down, So any last min work that needs to be done, it needs to be quick
        """
        pass

    # -----------------------------------------------------------------------------------
    def CreateMenu(self, menuBar, frame, *args, **kwargs):
        pass
