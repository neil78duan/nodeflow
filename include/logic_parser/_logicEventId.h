/* file _logic_event.h
 *
 * define event id 
 *
 * create by duan 
 */

_APOLLO_DEFINE_EVENT_VAL(EVENT_UPDATE, 0, "tick()")
_APOLLO_DEFINE_EVENT_VAL(EVENT_START,1, "startUp()")
_APOLLO_DEFINE_EVENT_VAL(EVENT_SHUTDOWN, 2, "shutdown()")
_APOLLO_DEFINE_EVENT_VAL(EVENT_MODULE_LOADED, 3, "Loaded(module_name)")
_APOLLO_DEFINE_EVENT_VAL(EVENT_MODULE_UNLOADED, 4, "unloaded(module_name)")