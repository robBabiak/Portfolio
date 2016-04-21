/* -----------------------------------------------------------------------------------
   -- ro_image.hpp
   -- Copyright Robert Babiak, 2016
   ----------------------------------------------------------------------------------- */
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>

#include <boost/enable_shared_from_this.hpp>
#include <syslog.h>

using namespace boost::python;
using namespace std;

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"

#include "renderObject.hpp"
#include "graphicsEnums.hpp"
#include "scene.hpp"
#include "viewManager.hpp"
#include "ro_image.hpp"

// -----------------------------------------------------------------------------------
RO_Image::RO_Image(Scene * _scene):
    RO_Base(_scene)
  , textureID(0)
  , INIT_PROP(resPath)
  // , INIT_PROP_DEF(size, glm::vec3(64, 64, 0))
  , INIT_PROP_DEF(size, glm::vec3(-1, -1, 0))
  , INIT_PROP_DEF(visionRange, 0.0f)
{
}
RO_Image::RO_Image(Scene * _scene, string _resPath):
    RO_Base(_scene)
  , textureID(0)
  , INIT_PROP_DEF(resPath, _resPath)
  // , INIT_PROP_DEF(size, glm::vec3(64, 64, 0))
  , INIT_PROP_DEF(size, glm::vec3(-1, -1, 0))
  , INIT_PROP_DEF(visionRange, 0.0f)
{
}

// -----------------------------------------------------------------------------------
RO_Image::~RO_Image()
{
}

// -----------------------------------------------------------------------------------
void RO_Image::Boost(void)
{
    class_ < RO_Image, bases<RO_Base>, RO_ImagePtr >("RO_Image", no_init)
        CMDPROP(resPath, RO_Image)
        CMDPROP(size, RO_Image)
        CMDPROP(visionRange, RO_Image)
        .def("debug", &RO_Image::Debug)
    ;
    class_< RO_ImageVector >("_RO_ImageVector")
        .def(vector_indexing_suite< RO_ImageVector >())
    ;
}

// -----------------------------------------------------------------------------------
std::string RO_Image::__repr__()
{
    std::ostringstream stringStream;
    stringStream << "Image " << name << ": ";
    stringStream << " Enabled: " << (enabled() ? "T" : "F");

    stringStream << " Pos: (" << position()[0] << ", " << position()[1] << ", " << position()[2] << ")";
    stringStream << " Rot: " << rotation();
    stringStream << " Scale: (" << scaleX() << ", " << scaleY() << ")";
    stringStream << " Vis: " << visibleState();
    stringStream << " Vange: " << visionRange();
    stringStream << " Render: ";
    PYFor_const(renderSet, it)
    {
        stringStream << *it << ",";
    }
    stringStream << " Res: " << resPath();
    stringStream << " Size: (" << size()[0] << ", " << size()[1] << ", " << size()[2] << ")";

    return  stringStream.str();

}

// -----------------------------------------------------------------------------------
python::object RO_Image::GetSelf()
{
    return python::object(GetThis<RO_Image>());
}

// -----------------------------------------------------------------------------------
glm::mat4 RO_Image::Transform(glm::mat4 parentsTransform)
{
    I = glm::mat4();

    T = glm::translate(I, position());
    R = glm::rotate(I, rotation() * 0.0174532925f, glm::vec3(0.0f, 0.0f, 1.0f));
    S = glm::scale(I, glm::vec3((size().x / BASEQUADSIDELENGTH) * scaleX(), (size().y / BASEQUADSIDELENGTH) * scaleY(), 1.0));

    currentTransform = parentsTransform * T * R * S;
    return currentTransform;
}
// -----------------------------------------------------------------------------------
void RO_Image::PyTransform(glm::mat4 parentsTransform)
{
    currentTransform = parentsTransform; // set identity matrix
    currentTransform = glm::rotate(currentTransform, rotation.pyGet() * 0.0174532925f, glm::vec3(0.0f,0.0f, 1.0f));
    currentTransform = glm::translate( currentTransform, position.pyGet());
    currentTransform = glm::scale(currentTransform, glm::vec3((size.pyGet().x / BASEQUADSIDELENGTH) * scaleX.pyGet(), (size.pyGet().y / BASEQUADSIDELENGTH) * scaleY.pyGet(), 1.0));
}

// -----------------------------------------------------------------------------------
void RO_Image::RenderObject(RenderSettingsPtr settings)
{
    if (textureID == 0)
    {
        if (scene == NULL)
        {
            printf("ERROR: Scene not Set for Render, skipping! (%s) %s\n", name.c_str(), resPath().c_str());
            return;
        }
        textureID = scene->GetTexture(resPath());
        // if we have the default size, then get the resource native size.
        // this is needed in case the resource doesn't get a size set at creation. in whichcase
        // default to the size set in the res pack.
        if (size.pyGet().x < 0 || size.pyGet().y < 0)
        {
            // force both sides to agree...
            glm::vec3 v = scene->GetTextureSize(resPath());
            size.pySet(v);
            // size.set(v);
        }

    }
    GL_CHECK_ERROR("GL Error: Start Render Object")
    GLint currentShaderProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentShaderProgram);
    GL_CHECK_ERROR("GL Error: Get current render program")
    if (currentShaderProgram == settings->defaultShaderID)
    {
        glUniformMatrix4fv(settings->modelLocation, 1, GL_FALSE, &(currentTransform[0][0]));
        GL_CHECK_ERROR("GL Error: glUniformMatrix4fv: modelLocation")
        glUniformMatrix4fv(settings->vpLocation, 1, GL_FALSE, &(settings->ProjViewMat[0][0]));
        GL_CHECK_ERROR("GL Error: glUniformMatrix4fv: vpLocation")

    }
    else if (currentShaderProgram == settings->tokenShaderID)
    {
        glUniformMatrix4fv(settings->tsModelLocation, 1, GL_FALSE, &(currentTransform[0][0]));
        GL_CHECK_ERROR("GL Error: glUniformMatrix4fv: tsModelLocation")
        glUniformMatrix4fv(settings->tsVPLocation, 1, GL_FALSE, &(settings->ProjViewMat[0][0]));
        GL_CHECK_ERROR("GL Error: glUniformMatrix4fv: tsVPLocation")
    }

    glActiveTexture(GL_TEXTURE0);
    GL_CHECK_ERROR("GL Error: glActiveTexture")
    glBindTexture(GL_TEXTURE_2D, textureID);
    GL_CHECK_ERROR("GL Error: glBindTexture:")
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    GL_CHECK_ERROR("GL Error: glDrawElements:")
    glBindTexture(GL_TEXTURE_2D, 0);
    GL_CHECK_ERROR("GL Error: glBindTexture:")
}

// -----------------------------------------------------------------------------------
void RO_Image::CollectRenderables(RenderObjectVectorPtr renderables, stringList &renderSet)
{
    if (Collectable(renderSet))
    {
        renderables->push_back(this);//shared_from_this());
    }
}

// -----------------------------------------------------------------------------------
void RO_Image::ApplyCommand(CommandObjectPtr cmd)
{
    if (resPath.ApplyCommand(cmd) == 1){} else
    if (size.ApplyCommand(cmd) == 1){} else
    if (visionRange.ApplyCommand(cmd) == 1){} else
    RO_Base::ApplyCommand(cmd);
}

// -----------------------------------------------------------------------------------
void RO_Image::Debug(void)
{
    printf("TextureID %u\n", textureID);
    resPath.Debug("");
    position.Debug("");
    size.Debug("size");
    visionRange.Debug("");
    float sX = size().x;
    float sY = size().y;
    glm::vec4 v1(0.f, 0.f, 0.f, 1.f), v2(sX, 0.f, 0.f, 1.f), v3(sX, sY, 0.f, 1.f), v4(0.f, sY, 0.f, 1.f);
    printf("v1(%3.1f, %3.1f) v2(%3.1f,%3.1f) v3(%3.1f,%3.1f) v4(%3.1f,%3.1f)\n",
        v1.x, v1.y, v2.x, v2.y, v3.x, v3.y, v4.x, v4.y);
    v1 = currentTransform * v1;
    v2 = currentTransform * v2;
    v3 = currentTransform * v3;
    v4 = currentTransform * v4;
    printf("v1(%3.1f, %3.1f) v2(%3.1f,%3.1f) v3(%3.1f,%3.1f) v4(%3.1f,%3.1f)\n",
        v1.x, v1.y, v2.x, v2.y, v3.x, v3.y, v4.x, v4.y);
     printf("[%3.1f, %3.1f, %3.1f, %3.1f]\n[%3.1f, %3.1f, %3.1f, %3.1f]\n[%3.1f, %3.1f, %3.1f, %3.1f]\n[%3.1f, %3.1f, %3.1f, %3.1f]\n",
           currentTransform[0].x, currentTransform[1].x, currentTransform[2].x, currentTransform[3].x,
           currentTransform[0].y, currentTransform[1].y, currentTransform[2].y, currentTransform[3].y,
           currentTransform[0].z, currentTransform[1].z, currentTransform[2].z, currentTransform[3].z,
           currentTransform[0].w, currentTransform[1].w, currentTransform[2].w, currentTransform[3].w );

    printf("Transform Matrix:\n");
    for (int i = 0; i < 4; i++){
        printf("[");
        for (int j = 0; j < 4; j++)
            printf("%3.1f ", currentTransform[i][j]);
        printf("]\n");
    }


    RO_Base::Debug();

}

// -----------------------------------------------------------------------------------
void RO_Image::DecodeYaml(YAML::Node node)
{
    RO_Base::DecodeYaml(node);
    if (node["resPath"]) resPath << node;
    if (node["size"]) size << node;
}

// -----------------------------------------------------------------------------------
void RO_Image::ComputeAABB(glm::mat4 parentsTransform)
{
    glm::mat4 trans = parentsTransform; // set identity matrix
    trans = glm::translate( trans, position.pyGet());
    trans = glm::rotate(trans, rotation.pyGet() * 0.0174532925f, glm::vec3(0.0f,0.0f, 1.0f));
    trans = glm::scale(trans, glm::vec3(scaleX.pyGet(), scaleY.pyGet(), 1.0));
    float sX = size.pyGet().x;
    float sY = size.pyGet().y;
    if (sX < 0 || sY < 0)
    {
        // ok the size has not been initalized.
        if (sX < 0) sX = 64; // this is the old default sizes.
        if (sY < 0) sY = 64;
    }

    glm::vec4 v1(0.f, 0.f, 0.f, 1.f), v2(sX, 0.f, 0.f, 1.f), v3(sX, sY, 0.f, 1.f), v4(0.f, sY, 0.f, 1.f);
    v1 = trans * v1;
    v2 = trans * v2;
    v3 = trans * v3;
    v4 = trans * v4;

    glm::vec3 v1a(v1.x, v1.y, v1.z), v2a(v2.x, v2.y, v2.z), v3a(v3.x, v3.y, v3.z), v4a(v4.x, v4.y, v4.z);

    aabb.Add(v1a);
    aabb.Add(v2a);
    aabb.Add(v3a);
    aabb.Add(v4a);
    // cout << "POS:" << resPath.pyGet().c_str() << " | " << position.pyGet() << " | [" << sX << ", " << sY << "] | " ;
    // cout << "Mat:" << trans << " | ";
    // cout << "V1:" << v1a << " | ";
    // cout << "V2:" << v2a << " | ";
    // cout << "V3:" << v3a << " | ";
    // cout << "V4:" << v4a << " | ";
    // cout << name << "("<<aabb.min << ") - (" << aabb.max << ")\n";
}
