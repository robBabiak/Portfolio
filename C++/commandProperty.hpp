/* -----------------------------------------------------------------------------------
   -- CommandProperty.hpp
   -- Copyright Robert Babiak, 2016
   ----------------------------------------------------------------------------------- */
#ifndef __COMMAND_PROPERTY_HPP__
#define __COMMAND_PROPERTY_HPP__

#include <boost/python.hpp>
#include "commandObject.hpp"
#include <boost/any.hpp>
#include <ostream>
#include "glm.hpp"
#include "yaml-cpp/yaml.h"
#include "glm.hpp"

// external decleration for the command processor.
extern void StaticQueueCommand(CommandObjectPtr cmd);


using boost::any_cast;

// -----------------------------------------------------------------------------------
// Macros for defining the property!
// Type, name, ID
#define DEF_PROP(T, N, _ID) CommandProperty <T> N; static const int N##ID = _ID; typedef CommandProperty< T > N##Type; typedef  T  N##BaseType; T GET_PROP_##N() const {return N.pyGet();} void SET_PROP_##N( T v) { N.pySet(v);}
// name, ID
#define INIT_PROP( N ) N( #N, N##ID, this)
// name, ID, value
#define INIT_PROP_DEF(N, V) N( #N, N##ID, this, V)
// Class, ID
#define CMDPROP(N, C) .add_property(#N,  &C::GET_PROP_##N, &C::SET_PROP_##N)
// Name
#define ExposeGetterSetter(N) N##BaseType Get##N() { return N.pyGet();} void Set##N(N##BaseType v) { N.pySet(v);}

// -----------------------------------------------------------------------------------

template <typename T>
class CommandProperty
{
private:                        // the internal data storage
    std::string name;                       // The name of this object!
    int ID;                                 // the ID of this property!
    BaseCommandObject *parent;              // the parent object of this property
    T pyValue;                              // the python value
    T cValue;                               // the C side value
    int changeBit:1;                        // the bit that indicates the value changed.
protected:                      // The interface to derived classes
    void DoUpdate(T value)
    {
        CommandObjectPtr cmd = CommandObject::GetCommand(CMD_STD_UPDATE);
        cmd->SetDest(parent->shared_from_this());
        cmd->SetID(ID);
        cmd->SetData1(boost::any(value));
        StaticQueueCommand(cmd);
    }

public:                         // publicly exposed interface
    typedef T   valueType;
    // -----------------------------------------------------------------------------------
    // Constuctors with and without initalization values
    CommandProperty(std::string _name, int _ID, BaseCommandObject * _parent): name(_name), ID(_ID), parent(_parent), pyValue(), cValue(), changeBit(1) {};
    CommandProperty(std::string _name, int _ID, BaseCommandObject * _parent, T _value): name(_name), ID(_ID), parent(_parent), pyValue(_value), cValue(_value), changeBit(1) {};
    CommandProperty(std::string _name, int _ID, BaseCommandObject * _parent, const CommandProperty<T>  &othr): name(_name), ID(_ID), parent(_parent), pyValue(othr.pyValue), cValue(othr.cValue), changeBit(1) {};
    virtual ~CommandProperty(){};


    // -----------------------------------------------------------------------------------
    // what is our name!
    std::string & GetName() {return name;}

    // -----------------------------------------------------------------------------------
    // A short cut to access the C side data.
    T operator() () const
    {
        return cValue;
    }
    T operator() ()
    {
        return cValue;
    }
    // -----------------------------------------------------------------------------------
    // The accessors for the the python data
    T pyGet() const
    {
        return pyValue;
    }

    // -----------------------------------------------------------------------------------
    // And the settor for the python side of things.
    void pySet(T value)
    {
        DoUpdate(value);
        pyValue = value;
    }

    // -----------------------------------------------------------------------------------
    // Serializing in from a Yaml Node
    const YAML::Node& operator<< (const YAML::Node& node)
    {
        if (node[name].IsDefined())
        {
            pySet( node[name].template as< T >());
        } else
        {
            printf("CMDPROP '%s' Loading from YAML def:%d, Property node Defined ( ", name.c_str() , node[name].IsDefined());
            printf(" Property node Defined (");
            for (YAML::const_iterator it=node.begin();it!=node.end();++it)
            {
                std::cout << it->first.template as<string>() << "[" << (it->first.template as<string>() == name) << "], ";
            }
            printf(").\n");
        }
        return node;
    }

    // -----------------------------------------------------------------------------------
    // The C side of the getter setter pairs
    T get() const
    {
        return cValue;
    }

    // -----------------------------------------------------------------------------------
    // Sets the C side of things. Really shouldn't do this, as there is no way to sync back
    // the python side of things.
    void set(T v)
    {
        cValue = v;
    }

    // Flag to indicate that the value has changed since last check.
    bool changed() {return changeBit;}
    void clearChanged() { changeBit = 0;}

    // -----------------------------------------------------------------------------------
    // Handle the update command and transfer the data from the Py side to the C side.
    int ApplyCommand(CommandObjectPtr cmd)
    {
        if (cmd->GetID() != ID)
        {
            return 0;
        }

        if (cmd->GetCmd() == CMD_STD_UPDATE)
        {
            boost::any value;
            cmd->GetData1(value);

            cValue = any_cast< T >(value);
            changeBit = 1;
        }
        else
        {
            return 0;
        }
        return 1;
    }

    // -----------------------------------------------------------------------------------
    // print out the debug information, Unfortunately python vies this as the python class.
    // so our property can't add new methods to this class.
    // but this method can be called by the host class to display details.
    void Debug(std::string pre = "")
    {
        std::ostringstream stringStream;
        stringStream << "Property " << name.c_str() << "(" << ID <<")" << "PY:" << pyValue << " C:" << cValue;
        printf("%s%s\n", pre.c_str(), stringStream.str().c_str());
    }

    // -----------------------------------------------------------------------------------
    // This is a sync up between the py and C side. It is inherently dangerous if the py side can make changes to the record.
    // but if the object is going through the copy constructor then it is safe as the C side won't have been updated.
    void Sync(void)
    {
        cValue = pyValue;
    }
};

#endif
