/* -----------------------------------------------------------------------------------
   -- CommandObject.cpp
   -- Copyright Robert Babiak, 2016
   ----------------------------------------------------------------------------------- */
#include "commandObject.hpp"
#include "utils.hpp"
#include "boost/any.hpp"
#include "profiler.hpp"
using boost::any_cast;

#define PRELOAD_CACHE 10

void BaseCommandObject::Boost()
{
    docstring_options doc_options(true);
    class_ < BaseCommandObject, boost::noncopyable>("BaseCommandObject", "A command Object for changing things", no_init);
}
// -----------------------------------------------------------------------------------
// This class will mange the cache of command objects, ensuring they are allocated
// as needed and destroyed on shutdown.
namespace Command_Object_Cache
{
    void noop_deleter(void*) { };

    CommandObjectCachePtr py_get_singleton()
    {
        // The Shared pointers are non deleting pointers.
        // Life of this object is manually controlled!
        return CommandObjectCache::GetInstance();
    }

}


CommandObjectCache* CommandObjectCache::instance = NULL;


// -----------------------------------------------------------------------------------
CommandObject::CommandObject(void):
    cmd(CMD_INVALID),
    inCache(1),
    // the arguments of the command
    dest(),
    profileTimeStart(0.0f)

{
    Reset();
}

// -----------------------------------------------------------------------------------
CommandObject::~CommandObject(void)
{
    Reset(); // ensure that all pointers are cleaned up
}

void CommandObject::Boost()
{
    docstring_options doc_options(true);
    class_ < CommandObject, boost::noncopyable>("CommandObject", "A command Object for changing things", no_init)
    .def("GetCommand", &CommandObject::GetCommand, return_value_policy<reference_existing_object>(), "Carefull with this, if the command is not added to the command buffer or returned to the cache, python can strand a command instance in the eather. ")
    .def("__str__", &CommandObject::__str__)
    .staticmethod("GetCommand")

    .def("ReturnCommand", &CommandObject::ReturnCommand, "Return a command to the command cache for reuse. ")
    .staticmethod("ReturnCommand")
    .def("Return", &CommandObject::Return, "Return this command to the cache, any reference after this call should not be used.")

    .add_property("cmd", &CommandObject::GetCmd,    &CommandObject::SetCmd, "This is the command to be executed, See _Graphics.Commands for details")
    .add_property("dest", &CommandObject::GetDest,  &CommandObject::SetDest)
    .add_property("int1", &CommandObject::GetInt1,  &CommandObject::SetInt1)
    .add_property("int2", &CommandObject::GetInt2,  &CommandObject::SetInt2)
    .add_property("str1", &CommandObject::GetStr1,  &CommandObject::SetStr1)
    .add_property("str2", &CommandObject::GetStr2,  &CommandObject::SetStr2)
    .add_property("ptr1", &CommandObject::GetPtr1,  &CommandObject::SetPtr1)
    .add_property("ptr2", &CommandObject::GetPtr2,  &CommandObject::SetPtr2)
    .add_property("float1", &CommandObject::GetFloat1,  &CommandObject::SetFloat1)
    .add_property("float2", &CommandObject::GetFloat2,  &CommandObject::SetFloat2)
    .add_property("float3", &CommandObject::GetFloat3,  &CommandObject::SetFloat3)
    ;
}

// -----------------------------------------------------------------------------------
//  Get a command from the cache.
CommandObjectPtr CommandObject::GetCommand(Commands cmdType)
{
    return CommandObjectCache::GetInstance()->GetCommand(cmdType);
}
// -----------------------------------------------------------------------------------
// return a command to the cache.
void CommandObject::ReturnCommand (CommandObjectPtr cmd)
{
    CommandObjectCache::GetInstance()->ReturnCommand(cmd);
}

// -----------------------------------------------------------------------------------
// return a command to the cache.
void CommandObject::Return()
{
    ReturnCommand(this);
}

// -----------------------------------------------------------------------------------
// apply this command to it's destination
void CommandObject::Apply()
{
    IN_CACHE(0);
    GLFW_THREAD_CHECK(); // commands can only be applied from the GLFW thread!
    assert(dest && "Apply:: Dest not set");
    dest->ApplyCommand(this);
}

// -----------------------------------------------------------------------------------
// reset the cache, we have guard bits for sanity checking this.
// It might just be faster and better to skip the guards and
// just clear them.
void CommandObject::Reset(void)
{
    cmd = CMD_INVALID;
    id = 0;
    dest = nullPtr;
    any1 = any2 = any3 = any4 = any5 = any6 = NULL;
}
// -----------------------------------------------------------------------------------
// return a dubug display of this object
std::string CommandObject::__str__(void)
{
    std::ostringstream stringStream;
    stringStream << GetStringCommands(cmd);
    stringStream << "(" << id << ")";
    stringStream << "[";
    if (!any1.empty())
    {
        stringStream << " 1:" << any1.type().name();
        // stringStream <<  boost::lexical_cast< std::string>(any1) << ", " ;
    }
    if (!any2.empty())
    {
        stringStream << " 2:";
        // stringStream << any_cast < decltype(any2.type()) > (any2) << ", " ;
    }
    if (!any3.empty())
    {
        stringStream << " 3:";
        // stringStream << any_cast < decltype(any3.type()) > (any3) << ", " ;
    }
    if (!any4.empty())
    {
        stringStream << " 4:";
        // stringStream << any_cast < decltype(any4.type()) > (any4) << ", " ;
    }
    if (!any5.empty())
    {
        stringStream << " 5:";
        // stringStream << any_cast < decltype(any5.type()) > (any5) << ", " ;
    }
    if (!any6.empty())
    {
        stringStream << " 6:";
        // stringStream << any_cast < decltype(any6.type()) > (any6) << ", " ;
    }
    stringStream << "]";
    return stringStream.str();
}
// -----------------------------------------------------------------------------------
// Boost magic for doing a singlton object as a constructor in python.
// every instance of a commandObjectCache will return the same object.



// -----------------------------------------------------------------------------------
CommandObjectCache::CommandObjectCache() : commandCache(), numCommands(0)
{
    INIT_PROFILE(COMMAND_SIZE, "Commands:CacheSize")
    INIT_PROFILE(COMMAND_LIVE, "Commands:ActiveTime")
    // Preload the cache with a few objects to start with,
    // this cache can grow unbounded as things happen.
    for (int i = 0; i< PRELOAD_CACHE; ++i)
    {
        CommandObject * cmd = new CommandObject();

        commandCache.push_back(  CommandObjectPtr(cmd) );
        PROFILE_INC(COMMAND_SIZE)
        ++numCommands;
    }
}

//----------------------
CommandObjectCache::~CommandObjectCache()
{
    for (CommandObjectDeque::iterator it = commandCache.begin(); it != commandCache.end(); ++it)
    {
        PROFILE_DEC(COMMAND_SIZE)
        delete (*it);
    }
    commandCache.clear();
}

void CommandObjectCache::Boost()
{
    class_ < CommandObjectCache, boost::noncopyable>("CommandObjectCache", "The Manager of the cache", no_init)
    .def("__init__", make_constructor(&Command_Object_Cache::py_get_singleton))
    .def("GetNumber", &CommandObjectCache::GetNumber)
    ;
}

//----------------------
// returns the number allocated, primarily for debugging exposure
int CommandObjectCache::GetNumber()
{
    return numCommands;
};

//----------------------
// Return a command, or allocate one if needed.
CommandObjectPtr CommandObjectCache::GetCommand(Commands cmdType)
{
    if (commandCache.empty()) // is the cache empty
    {
        commandCache.push_back( CommandObjectPtr(new CommandObject()) );
        PROFILE_INC(COMMAND_SIZE)
        ++numCommands;
    }
    CommandObjectPtr cmd = commandCache.front();
    PROFILE_SET_START_TIME(COMMAND_LIVE, cmd->profileTimeStart)
    commandCache.pop_front();
    // printf("GetCommand %s - InCache:%d\n", GetStringCommands(cmdType), p->inCache);
    assert(cmd->inCache == 1 && "Returning command that was not placed into the cache!");
    cmd->inCache = 0;
    cmd->SetCmd(cmdType);
    return cmd;
}
//----------------------
// Done with a command, return it to the cache for reuse.
void CommandObjectCache::ReturnCommand( CommandObjectPtr cmd )
{
    // printf("Returning %s\n", GetStringCommands(cmd->cmd));
    cmd->Reset();
    assert(cmd->inCache == 0 && "Returning command that is already int he cache!");
    cmd->inCache = 1;
    PROFILE_SET_END_TIME(COMMAND_LIVE, cmd->profileTimeStart)
    commandCache.push_back(cmd);
}

// -----------------------------------------------------------------------------------
void CommandObjectCache::StartInstance()
{
    if (instance == NULL)
    {
        instance = new CommandObjectCache();
    }
}

// -----------------------------------------------------------------------------------
CommandObjectCachePtr CommandObjectCache::GetInstance()
{
    return CommandObjectCachePtr (instance, Command_Object_Cache::noop_deleter);
}

void CommandObjectCache::StopInstance()
{
    delete instance;
}
