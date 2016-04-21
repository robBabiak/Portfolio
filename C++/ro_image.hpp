/* -----------------------------------------------------------------------------------
   -- ro_image.cpp
   -- Copyright Robert Babiak, 2016
   ----------------------------------------------------------------------------------- */
#ifndef __RO_IMAGE_HPP__
#define __RO_IMAGE_HPP__
#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <list>
#include "sdtGraphics.hpp"
#include "commandObject.hpp"
#include "commandProperty.hpp"
#include "ro_base.hpp"
#include "ro_list.hpp"
#include "ro_state.hpp"


// -----------------------------------------------------------------------------------
class RO_Image;

typedef boost::shared_ptr<RO_Image> RO_ImagePtr;
typedef std::vector< RO_ImagePtr > RO_ImageVector;

class RO_Image: public RO_Base
{

    DEF_PROP(std::string, resPath, 100)
    DEF_PROP(glm::vec3, size, 101)
    DEF_PROP(float, visionRange, 102)

private:                // internal variables
    GLuint textureID, vaoID, vboID;           // the openGL texture for this object
    void BuildGraphics();
    // glm::vec4 * GetCurrentVerts();
    // void UpdateVBO(glm::vec4 * CurrentVerts);
    void DrawQuad();            // Draw a quad with this texture on it.

public:
    virtual void CollectRenderables(RenderObjectVectorPtr renderables, stringList &renderSet);
    virtual void RenderObject(RenderSettingsPtr settings);
    virtual glm::mat4 Transform(glm::mat4 parentsTransform);
    virtual void PyTransform(glm::mat4 parentsTransform);
    inline float GetVisionRange() {return visionRange();}
    inline glm::vec3 GetPosition() {return position();}
    void ApplyCommand(CommandObjectPtr cmd);

    void Debug(void);
    virtual void DumpNode(int indent) { printf("%*sImage(%u) %-30s\t(%6.1f,%6.1f,%6.1f)\t%6.1f°\t[%3.1f,%3.1f]\t {%6.1f,%6.1f}\n", indent, " ", textureID, resPath().c_str(), position.pyGet().x, position.pyGet().y, position.pyGet().z, rotation.pyGet(), scaleX.pyGet(), scaleY.pyGet(), size.pyGet().x, size.pyGet().y);}
    virtual std::string __repr__();

    virtual void DecodeYaml(YAML::Node node);
public:                            // AABB caculation
    virtual void ComputeAABB(glm::mat4 parentsTransform);

public:
    RO_Image(Scene * _scene);
    RO_Image(Scene * _scene, string _resPath);
    ~RO_Image();
    static void Boost(void);
    virtual python::object GetSelf();
    // GLfloat verts[20];

};
#endif
