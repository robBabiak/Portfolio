/* -----------------------------------------------------------------------------------
   -- ro_base.hpp
   -- Copyright Robert Babiak, 2016
   ----------------------------------------------------------------------------------- */
#ifndef __RO_BASE_HPP__
#define __RO_BASE_HPP__
#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <list>
#include "commandObject.hpp"
#include "commandProperty.hpp"
#include "commandList.hpp"
#include "axisAlignedBoundingBox.hpp"

#include "yaml-cpp/yaml.h"
#include "ViewOptions.hpp"

class RO_Base;
class RO_Iterator;
class Scene;

typedef boost::shared_ptr<RO_Base> RO_BasePtr;
typedef std::vector< RO_BasePtr > RO_BaseVector;
typedef std::vector< RO_Base* > RenderObjectVector;
typedef RenderObjectVector * RenderObjectVectorPtr;

typedef boost::shared_ptr<RO_Iterator> RO_IteratorPtr;
#define DEG2RAD 0.017453292519943295f

// -----------------------------------------------------------------------------------
// Anythime a render object(RO) changes a setting locally, it is responsible
// for resetting the value before exiting, this prevents the structure being copied
// every call at the expense of the individual RO, needing to save the original value
// if they modify it.
struct RenderSettings
{
    float alpha;
    GLint modelLocation;
    GLint tsModelLocation;
    GLint vpLocation;
    GLint tsVPLocation;
    GLint defaultShaderID;
    GLint tokenShaderID;
    glm::mat4 ProjViewMat;

    // -----
    RenderSettings( void ):
        alpha(1.0f)
    {};
    RenderSettings( ViewOptionsPtr opt, GLint transLoc, GLint _modelLocation, GLint _tsModelLocation, GLint _tsVPLocation, GLint _dsID, GLint _tsID):
        alpha(1.0f)
      , modelLocation(_modelLocation)
      , tsModelLocation(_tsModelLocation)
      , tsVPLocation(_tsVPLocation)
      , defaultShaderID(_dsID)
      , tokenShaderID(_tsID)
    {
        CalculateProjView(opt, transLoc);
    };

    void Reset()
    {
        alpha = 1.0;
        modelLocation = -1;
        vpLocation = -1;
        tsModelLocation = -1;
        tsVPLocation = -1;
        defaultShaderID = -1;
        tokenShaderID = -1;
        ProjViewMat = glm::mat4(1.f);
    }
    void CalculateProjView(ViewOptionsPtr opt, GLint transLoc)
    {
        glm::mat4 PMat = glm::perspective(opt->orthoView().x * DEG2RAD, (opt->viewportSize().x / opt->viewportSize().y), opt->orthoView().y, opt->orthoView().z);
        glm::mat4 VMat = glm::translate(glm::mat4(1.f), -1.0f * opt->cameraPosition());

        ProjViewMat = PMat * VMat;
        vpLocation = transLoc;
    }
};

typedef RenderSettings *RenderSettingsPtr;

// -----------------------------------------------------------------------------------
class RO_Base : public BaseCommandObject
{
protected:
    DEF_PROP(int,           enabled,    1)
    DEF_PROP(float,         rotation,   2)
    DEF_PROP(float,         scaleX,     3)
    DEF_PROP(float,         scaleY,     4)
    DEF_PROP(glm::vec3,     position,   5)
    DEF_PROP(float,         alpha,      6)
    DEF_LIST(std::string,   renderSet,  7)
    DEF_PROP(std::string,   visibleState, 8)


    friend class RO_Iterator;

protected:              // Common variables for the hierarchy
    Scene          *scene;              // which scene are we part of! This is a naked C pointer, so no reference counting problems.
    glm::mat4       currentTransform;   // The current transformation, from the last transform call.
    object          controller;
    glm::mat4 T, R, S, I;

private:


public:
    RO_Base(Scene * _scene);
    virtual ~RO_Base();
    virtual void Init(Scene * _scene) {};
    virtual void Shutdown(void){};
    static void Boost(void);
    virtual std::string __repr__();
    virtual python::object GetSelf();

    virtual void DumpNode(int indent) { printf("%*sBase %-30s\t(%6.1f,%6.1f,%6.1f)\t%6.1f°\n", indent, " ", name.c_str(), position.pyGet().x, position.pyGet().y, position.pyGet().z, rotation.pyGet());}
    virtual void Debug(void);
    void Test(stringList lst);

    virtual void DecodeYaml(YAML::Node node);
    virtual void DecodeMapYaml(YAML::Node node);

    virtual void SetState(std::string renderSetName, std::string stateName);
    virtual void SetState(std::string stateName);

public:                             // BaseCommandObject
    virtual void ApplyCommand(CommandObjectPtr cmd); // Apply a command

protected:                          // the shared interface for render objects
    bool Intersects(stringList &list);
    virtual bool Collectable(stringList &list);

public:                             // RO_Base
    std::string     name;               // name of this render object! NOTE: Only usable on python side, doesn't support thread safety!
    virtual glm::mat4 Transform(glm::mat4 parentsTransform);
    virtual void PyTransform(glm::mat4 parentsTransform);
    virtual void CollectRenderables(RenderObjectVectorPtr renderables, stringList &renderSet) = 0;
    virtual void RenderObject(RenderSettingsPtr settings) = 0;
    virtual RO_IteratorPtr Find(std::string);  // Return a iterator
    virtual void FindItems(std::string searchName, RO_Iterator * itr) {}; // If the derived object supports children then this will add any found items.

protected:                           // AABB caculation
    AxisAlignedBoundingBox aabb;
public:                            // AABB caculation
    virtual void ComputeAABB(glm::mat4 parentsTransform);
    virtual AxisAlignedBoundingBox GetAABB(bool bGlobal);
    AxisAlignedBoundingBox GetAABB() { return GetAABB(true); }

public:                             // public access for command properties. Will enforce use of the python values.
    ExposeGetterSetter(enabled)
    ExposeGetterSetter(rotation)
    ExposeGetterSetter(scaleX)
    ExposeGetterSetter(scaleY)
    ExposeGetterSetter(position)
    ExposeGetterSetter(alpha)
    // ExposeGetterSetter(renderSet) // not supported yet.
    ExposeGetterSetter(visibleState)

};

// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
class RO_Iterator
{
private:            // The internal storage
    RO_BaseVector                   items;

    inline RO_BaseVector::const_iterator cbegin() const {return items.cbegin();};
    inline RO_BaseVector::const_iterator cend() const {return items.cend();};
    inline RO_BaseVector::size_type size() const {return items.size();};
    inline RO_BasePtr getItem(int n) const {return items[n];};
public:             // The public exposed objects.
    RO_Iterator( void );        // Construct
    ~RO_Iterator( void );       // Construct
    void Add(RO_BasePtr itm);   // Add a item to the iterator
    static void Boost(void);    // Boost it!

};
#endif
