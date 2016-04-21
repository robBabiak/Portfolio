## -----------------------------------------------------------------------------------
## -- ServiceManager.py
## -- Copyright Robert Babiak, 2016
## -----------------------------------------------------------------------------------
import __builtin__
from Util import KeyVal
from collections import defaultdict
import time
import pkgutil
import stackless
import threading
from Serivce import SERVICE_STATES
__Services_Version__ = "1.0.0"


class ServiceException(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)


# ======================================================================================
class ServiceManager(object):
    def __init__(self):
        self.__version__ = __Services_Version__
        self._running_services = {}
        self._preinit_services = {}
        self.__registered_services__ = {}
        self.__registered_services_events__ = defaultdict(list)
        self.lastTick = None
        self.preference = 1
        self.helpers = KeyVal()
        self.threadID = id(threading.currentThread())
        self.queuedEvents = stackless.channel()
        self.queuedEvents.preference = 1

        stackless.tasklet(self.HandleQueuedScatterEvents)().run()

        self.exitTaskletManager = False

    # -----------------------------------------------------------------------------------
    def ExpandServiceIntoNamespace(self):
        """
            A Handy debug function that puts all the services into the sm namespace
        """
        for k, v in self._running_services.iteritems():
            setattr(self, k, v)

    # -----------------------------------------------------------------------------------
    def GetService(self, service_name):
        """
        find out if there is a service, if it is not running start the service.
        """
        if service_name not in self.__registered_services__:
            raise ServiceException("Service Not found: " + service_name + " : " + str(self.__registered_services__.keys()))

        if service_name in self._running_services:
            return self._running_services[service_name]

        return self.StartService(service_name)

    # -----------------------------------------------------------------------------------
    def StartService(self, service_name):
        """
        Start a service, this drives the initialization routine of services. The services are
        added to a list of preinit services. Then it will try calling preInit and then initialize
        any services that are in it's dependency list. Once all dependency are loaded then OnInit
        is called to finalize the initialization, this allows the initialization of one service
        to utilize other services. If we ever get a codependent situation then this will need to be
        revisited to make sure that we can handle that case.
        """
        self._preinit_services[service_name] = "Pre Init"

        svc_class = self.__registered_services__[service_name]
        svc = svc_class(self)
        try:
            self._preinit_services[service_name] = svc

            # -- Pre Init --
            try:
                if not svc.OnPreInit():
                    raise ServiceException("Service Failed PreInit: " + service_name)

            except Exception, e:
                raise ServiceException("Service OnPreInit Exception: " + service_name + " -- " + str(e))

            for dependant_name in getattr(svc_class, "__DEPENDANT_SERVICES__", []):
                aliasName = dependant_name

                # supprt a tuple for a alias for this service
                if isinstance(dependant_name, tuple):
                    aliasName = dependant_name[1]
                    dependant_name = dependant_name[0]

                if dependant_name in self._running_services:
                    setattr(svc, aliasName, self._running_services[dependant_name])

                elif dependant_name in self._preinit_services:
                    setattr(svc, aliasName, self._running_services[dependant_name])

                else:
                    setattr(svc, aliasName, self.StartService(dependant_name))

            # -- Init --
            try:
                svc.OnInit()
            except Exception, e:
                raise ServiceException("Service OnInit Exception: " + service_name + " -- " + str(e))

            if svc.state != SERVICE_STATES.RUNNING:
                raise ServiceException("Service Failed to enter running state: " + service_name)

            self._running_services[service_name] = self._preinit_services[service_name]

            svc.OnPostInit()

            if __debug__:
                setattr(self, "_%s_" % service_name, svc)

            return self._running_services[service_name]

        finally:
            del self._preinit_services[service_name]
        return None

    # -----------------------------------------------------------------------------------
    def ScatterEvent(self, event_name, *args, **kwargs):
        """
        Scatter a event to all services listening for that event. This makes communication
        between services generic and loosely bound.
        """
        if (id(threading.currentThread()) != self.threadID):
            self.queuedEvents.send((event_name, args, kwargs))
            return
        self.ProcessScatterEvent(event_name, *args, **kwargs)

    # -----------------------------------------------------------------------------------
    def ProcessScatterEvent(self, event_name, *args, **kwargs):
        services = set()
        services.update(self.__registered_services_events__[event_name])
        services.update(self.__registered_services_events__["*"])
        for each in services:
            svc = self.GetService(each)
            func = getattr(svc, "Event_" + event_name, None)
            # if the function is not found, lets see if it supports the all event.
            if func is not None:
                stackless.tasklet(func)(*args, **kwargs)
            else:
                func = getattr(svc, "Event_ALL_EVENTS", None)
                if func is not None:
                    stackless.tasklet(func)(event_name, *args, **kwargs).run()

    # -----------------------------------------------------------------------------------
    def HandleQueuedScatterEvents(self):
        while(1):
            event_name, args, kwargs = self.queuedEvents.receive()
            # evt = self.queuedEvents.receive()
            self.ProcessScatterEvent(event_name, *args, **kwargs)

    # -----------------------------------------------------------------------------------
    def Sleep(self, time):
        StacklessSync.Sleep(time)

    # -----------------------------------------------------------------------------------
    def TickServices(self):
        """
        This can provide a heart beat to a services, It needs to be re written to make use of
        each service requesting a heart beat time, and then a thread pumping all services
        and keeping a queue of services in order of there next heart beat time.
        """
        now = time.clock()
        if self.lastTick is None:
            self.lastTick = now
            return
        #figure out timers
        delatT = now - self.lastTick
        self.lastTick = now
        for svc in self._running_services.itervalues():
            if hasattr(svc, "OnTick"):
                svc.OnTick(delatT)

    # -----------------------------------------------------------------------------------
    def SystemShutdown(self):
        import Graphics
        Graphics._ViewManager().Shutdown()
        self.exitTaskletManager = True
        # tell all services to shut down!
        for service in self._running_services.values():
            try:
                print "shutting down - ", service.__ID__
                service.state = SERVICE_STATES.STOPPED
                service.OnSystemShutdown()
            except Exception, e:
                print "Exception in Service", service.__ID__, e

        # empty the events that are pending
        while self.queuedEvents.balance > 0:
            self.queuedEvents.receive()

        print "Shutdown complete!"

    # -----------------------------------------------------------------------------------
    def ServicesLoaded(self):
        """ Called after the services are discovered, and any that are Auto starting will
            Be started at this point.
        """
                # if service_name not in self.__registered_services__:

        for svcName, svc in self.__registered_services__.iteritems():
            if (getattr(svc, "__AUTO_START__", False)):
                # print "-- Auto Start Service --", svcName
                self.GetService(svcName)


# ======================================================================================
def install():
    # register the service manager into the built in name space.
    if getattr(__builtin__, "sm", None) is None:
        __builtin__.sm = ServiceManager()

    # import all the services and let themselves register
    for _, name, _ in pkgutil.iter_modules(['Services']):
        try:
            __import__("Services." + name)
        except Exception, e:
            print "Error loading service " + name + " " + str(e)
            print "------------------------ Traceback"
            import traceback
            traceback.print_exc()
            print "------------------------ End Traceback"
    sm.ServicesLoaded()


# ======================================================================================
def RegisterService(service_name, service_class):
    # print "-- Registering Service --", service_name
    sm = __builtin__.sm
    sm.__registered_services__[service_name] = service_class

    if hasattr(service_class, "__EVENTS__"):
        for each in service_class.__EVENTS__:
            sm.__registered_services_events__[each].append(service_name)


# ======================================================================================
def RegisterNamedService(service_class):
    # print "-- Registering Service --", service_class.__ID__
    sm = __builtin__.sm
    sm.__registered_services__[service_class.__ID__] = service_class

    if hasattr(service_class, "__EVENTS__"):
        for each in service_class.__EVENTS__:
            sm.__registered_services_events__[each].append(service_class.__ID__)
