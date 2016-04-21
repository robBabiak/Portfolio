/* -----------------------------------------------------------------------------------
   -- CommandObject.hpp
   -- Copyright Robert Babiak, 2016
   ----------------------------------------------------------------------------------- */
#ifndef __COMMAND_OBJECT__HPP__
#define __COMMAND_OBJECT__HPP__
#include <stdio.h>
#include <cmath>
#include <boost/python.hpp>
#include <unistd.h>
#include <assert.h>
#include <deque>
#include <Vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/any.hpp>
#include <boost/container/detail/config_begin.hpp>
#include <boost/container/detail/workaround.hpp>
#include <boost/container/container_fwd.hpp>

#include "commandObject.hpp"
#include "graphicsEnums.hpp"
#include "profiler.hpp"
#include "nullPointer.hpp"

using namespace boost::python;
using namespace std;
using boost::any_cast;

#define IN_CACHE(a) assert(inCache == a && "IN Cache Check filed: "#a)

// forward defs
class CommandObject;
typedef boost::shared_ptr<void> voidPtr;
typedef CommandObject * CommandObjectPtr;
typedef deque<CommandObjectPtr> CommandObjectDeque;
typedef vector<CommandObjectPtr> CommandObjectVector;


class BaseCommandObject : public boost::enable_shared_from_this<BaseCommandObject>
{
public:
    BaseCommandObject(void): boost::enable_shared_from_this<BaseCommandObject>() {};
    ~BaseCommandObject(void){};
    virtual void ApplyCommand(CommandObjectPtr cmd) = 0;

    template <typename T>
        boost::shared_ptr<T> GetThis(void) {return dynamic_pointer_cast<T> (shared_from_this());};
public:
    static void Boost();

};
typedef boost::shared_ptr<BaseCommandObject> BaseCommandObjectPtr;
typedef boost::shared_ptr<const BaseCommandObject> constBaseCommandObjectPtr;



class CommandObject
{
friend class CommandObjectCache;
protected:
    unsigned int inCache:1;                  // this has been returned to the cache

private:                // Argument storage
    Commands cmd;                   // The command to execute
    long id;                     // number argument
    BaseCommandObjectPtr dest;      // The destination that we are operating on
    boost::any any1;                // The Any should replace the non Any, less memory, and greater flexability.
    boost::any any2;
    boost::any any3;
    boost::any any4;
    boost::any any5;
    boost::any any6;
    // Add other types here.

public:                 // the command collection and recycling services
    static CommandObjectPtr GetCommand(Commands cmdType);           // this will get a new command if  there is none on the list
    static void ReturnCommand (CommandObjectPtr cmd);    // return the command to the unused list
    void Return();
    void Apply();
    std::string __str__(void);

public:                 // External data interface
    double profileTimeStart;
    void Reset();                           // Reset the class ready for the next usage

    Commands GetCmd(void)                   { IN_CACHE(0); return cmd;}    // Return the command
    void SetCmd(Commands _cmd)              { IN_CACHE(0); cmd = _cmd;};// Set the command ID

    long GetID(void)                        { IN_CACHE(0); return id;}// Return the ID
    void SetID(long _id)                    { IN_CACHE(0); id = _id;}; // Set the command ID

    BaseCommandObjectPtr GetDest(void)      { IN_CACHE(0); return dest;}// return the obect do operatoe on
    void SetDest(const BaseCommandObjectPtr p) { IN_CACHE(0); dest = p;};// Set the Base command object that this command will apply to

    // GEt and set the Any data
    void GetData1(boost::any &a)            { IN_CACHE(0); a = any1;};
    void SetData1(boost::any a)             { IN_CACHE(0); any1 = a;};

    void GetData2(boost::any &a)            { IN_CACHE(0); a = any2;};
    void SetData2(boost::any a)             { IN_CACHE(0); any2 = a;};

    void GetData3(boost::any &a)            { IN_CACHE(0); a = any3;};
    void SetData3(boost::any a)             { IN_CACHE(0); any3 = a;};

    void GetData4(boost::any &a)            { IN_CACHE(0); a = any4;};
    void SetData4(boost::any a)             { IN_CACHE(0); any4 = a;};

    void GetData5(boost::any &a)            { IN_CACHE(0); a = any5;};
    void SetData5(boost::any a)             { IN_CACHE(0); any5 = a;};

    void GetData6(boost::any &a)            { IN_CACHE(0); a = any6;};
    void SetData6(boost::any a)             { IN_CACHE(0); any6 = a;};


    // templated get and set for when we know the type going in and out
    template <typename P> P Get1()          {IN_CACHE(0); return any_cast< P > (any1);}
    template <typename P> void Set1(P item) {IN_CACHE(0); any1 = item;}
    template <typename P> P Get2()          {IN_CACHE(0); return any_cast< P > (any2);}
    template <typename P> void Set2(P item) {IN_CACHE(0); any2 = item;}
    template <typename P> P Get3()          {IN_CACHE(0); return any_cast< P > (any3);}
    template <typename P> void Set3(P item) {IN_CACHE(0); any3 = item;}
    template <typename P> P Get4()          {IN_CACHE(0); return any_cast< P > (any4);}
    template <typename P> void Set4(P item) {IN_CACHE(0); any4 = item;}
    template <typename P> P Get5()          {IN_CACHE(0); return any_cast< P > (any5);}
    template <typename P> void Set5(P item) {IN_CACHE(0); any5 = item;}
    template <typename P> P Get6()          {IN_CACHE(0); return any_cast< P > (any6);}
    template <typename P> void Set6(P item) {IN_CACHE(0); any6 = item;}


    // Concreat versions for exposing to python, May not need so could remove.
    // --------- REMEBER all the 1 versions share memory, so don't use str1 and ptr1 at the same time. ------
    // These really needs to go away!
    void SetInt1(int v)                     { Set1<int>(v); }
    void SetInt2(int v)                     { Set2<int>(v); }
    void SetStr1(std::string v)             { Set1<std::string>(v); }
    void SetStr2(std::string v)             { Set2<std::string>(v); }
    void SetPtr1(voidPtr v)                 { Set1<voidPtr>(v); }
    void SetPtr2(voidPtr v)                 { Set2<voidPtr>(v); }
    void SetFloat1(float v)                 { Set1<float>(v); }
    void SetFloat2(float v)                 { Set2<float>(v); }
    void SetFloat3(float v)                 { Set3<float>(v); }

    int GetInt1(void)                       { return Get1<int>(); }
    int GetInt2(void)                       { return Get2<int>(); }
    std::string GetStr1(void)               { return Get1<std::string>(); }
    std::string GetStr2(void)               { return Get2<std::string>(); }
    voidPtr GetPtr1(void)                   { return Get1<voidPtr>(); }
    voidPtr GetPtr2(void)                   { return Get2<voidPtr>(); }
    int GetFloat1(void)                     { return Get1<float>(); }
    int GetFloat2(void)                     { return Get2<float>(); }
    int GetFloat3(void)                     { return Get3<float>(); }


protected:                 // Constructor Destructor
    CommandObject(void);        // protected constructor and destructor as they should
    ~CommandObject(void);       // only ever be created and destroyed by the cache.
public:
    static void Boost();

};

typedef CommandObject * CommandObjectPtr;

class CommandObjectCache;
typedef boost::shared_ptr<CommandObjectCache> CommandObjectCachePtr;



// -----------------------------------------------------------------------------------


class CommandObjectCache
{
    DEFINE_PROFILE(COMMAND_SIZE)
    DEFINE_PROFILE(COMMAND_LIVE)
private:                // The storage and Singleton

    CommandObjectDeque          commandCache;       // the cache of commands
    static CommandObjectCache*  instance;           // the singleton instance.
    int                         numCommands;        // the number of commands allocated to date
public:
    CommandObjectCache();                           // Constructor
    ~CommandObjectCache();                          // Destructor
    static void Boost();
    static void StartInstance();                           // Static Instance interface
    static CommandObjectCachePtr GetInstance();            // ..
    static void StopInstance();                            // ..

    int GetNumber();                                // The number of objects in the cache

    CommandObjectPtr GetCommand(Commands cmdType);  // Get a command of the type from the cache
    void ReturnCommand( CommandObjectPtr cmd );     // Return the object to the cache.

};


#endif
