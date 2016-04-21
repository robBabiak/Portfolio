#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>

#include <boost/enable_shared_from_this.hpp>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "yaml-cpp/yaml.h"

using namespace boost::python;
using namespace std;

#include "renderObject.hpp"
#include "graphicsEnums.hpp"
#include "scene.hpp"
#include "viewManager.hpp"
#include "commandObject.hpp"
#include "utils.hpp"


// -----------------------------------------------------------------------------------
// Forward Decls
void delete_item(RO_BaseVector& container, typename RO_BaseVector::size_type i);
void append(RO_BaseVector& container,  typename RO_BaseVector::value_type const& v);

static boost::python::object stringList_as_str(stringList const& self)
{
    using boost::python::str;
    std::string s = "";
    for (stringList::const_iterator it = self.begin(); it != self.end(); ++it)
    {
        s +=  *it + ", ";
    }
    return str(s);
}
static boost::python::object stringList_as_list(stringList const& self)
{
    using boost::python::list;
    using boost::python::str;
    list l;
    std::string s = "";
    for (stringList::const_iterator it = self.begin(); it != self.end(); ++it)
    {
        l.append(str(*it));
        // s +=  *it + ", ";
    }
    return l;
}

// -----------------------------------------------------------------------------------
// The scene object that will operate in the render thread, the proxy (def below)
// will run in the pyhton thread
// -----------------------------------------------------------------------------------
RO_Base::RO_Base(Scene * _scene) : BaseCommandObject()
    , scene(_scene)
    , name("base")
    , controller()
    , INIT_PROP_DEF(enabled, 1)
    , INIT_PROP_DEF(rotation, 0.0f)
    , INIT_PROP_DEF(scaleX, 1.0f)
    , INIT_PROP_DEF(scaleY, 1.0f)
    , INIT_PROP(position)
    , INIT_PROP_DEF(alpha, 1.0f)
    , INIT_LIST_DEF(renderSet, stringList(1, "**ALL**"))
    , INIT_PROP_DEF(visibleState, "")
    , aabb()

{
    renderSet.SetKeepSorted(1);
}
// -----------------------------------------------------------------------------------
RO_Base::~RO_Base()
{}

// -----------------------------------------------------------------------------------
void RO_Base::Boost(void)
{

    renderSetType::Boost("CMDLST_String");
    BOOST_DUPLICATE_GUARD(stringList)
    class_<stringList>("stringList")
        .def(vector_indexing_suite<stringList>())
        .def("__str__", stringList_as_str)
        .def("__repr__", stringList_as_str)
        ;
    END_BOOST_DUPLICATE_GUARD()

    BOOST_DUPLICATE_GUARD(RO_Base)
    class_ < RO_Base, RO_BasePtr, bases<BaseCommandObject>, boost::noncopyable >("RO_Base", no_init)
        .def_readwrite("name", &RO_Base::name)
        CMDPROP(enabled,    RO_Base)
        CMDPROP(rotation,   RO_Base)
        CMDPROP(scaleX,     RO_Base)
        CMDPROP(scaleY,     RO_Base)
        CMDPROP(position,   RO_Base)
        CMDPROP(alpha,      RO_Base)
        CMDLIST(renderSet,  RO_Base)
        CMDPROP(visibleState,  RO_Base)
        .def("Find", &RO_Base::Find)
        .def("Test", &RO_Base::Test)
        .def("Debug", &RO_Base::Debug)
        .def("DumpNode", &RO_Base::DumpNode)
        .def("GetSelf", &RO_Base::GetSelf)
        .def("__repr__", &RO_Base::__repr__)
        .def_readwrite("controller", &RO_Base::controller)
        .def_readonly("aabb", &RO_Base::aabb)
        // .def("CollectRenderables", pure_virtual(&RO_Base::CollectRenderables))
        // .def("RenderObject", pure_virtual(&RO_Base::RenderObject))
    ;
    END_BOOST_DUPLICATE_GUARD()

    BOOST_DUPLICATE_GUARD(RO_BaseVector)
    class_< RO_BaseVector >("_RO_BaseVector")
        .def(vector_indexing_suite< RO_BaseVector, true >())
        .def("append", append, with_custodian_and_ward<1,2>()) // to let container keep value
        .def("__delitem__", delete_item)
    ;
    END_BOOST_DUPLICATE_GUARD()

}

// -----------------------------------------------------------------------------------
std::string RO_Base::__repr__()
{
    std::ostringstream stringStream;
    stringStream << "Base " << name << ": ";
    stringStream << " Enabled: " << (enabled() ? "T" : "F");

    stringStream << " Pos: (" << position()[0] << ", " << position()[1] << ", " << position()[2] << ")";
    stringStream << " Rot: " << rotation();
    stringStream << " Scale: (" << scaleX() << ", " << scaleY() << ")";
    stringStream << " Vis: " << visibleState();
    stringStream << " Render: ";
    PYFor_const(renderSet, it)
    {
        stringStream << *it << ",";
    }

    return  stringStream.str();
}

// -----------------------------------------------------------------------------------
python::object RO_Base::GetSelf()
{
    return python::object(GetThis<RO_Base>());
}

// -----------------------------------------------------------------------------------
void RO_Base::ApplyCommand(CommandObjectPtr cmd)
{
    if (enabled.ApplyCommand(cmd) == 1){} else
    if (rotation.ApplyCommand(cmd) == 1){} else
    if (scaleX.ApplyCommand(cmd) == 1){} else
    if (scaleY.ApplyCommand(cmd) == 1){} else
    if (position.ApplyCommand(cmd) == 1){} else
    if (alpha.ApplyCommand(cmd) == 1){} else
    if (renderSet.ApplyCommand(cmd) == 1){} else
    if (visibleState.ApplyCommand(cmd) == 1){} else
    ; // the end of the apply chain.
}

// -----------------------------------------------------------------------------------
glm::mat4 RO_Base::Transform(glm::mat4 parentsTransform)
{
    I = glm::mat4();

    T = glm::translate(I, position());
    R = glm::rotate(I, rotation() * 0.0174532925f, glm::vec3(0.0f,0.0f, 1.0f));
    S = glm::scale(I, glm::vec3(scaleX(), scaleY(), 1.0));

    currentTransform = parentsTransform * T * R * S;
    return currentTransform;
}

// -----------------------------------------------------------------------------------
void RO_Base::PyTransform(glm::mat4 parentsTransform)
{
    currentTransform = parentsTransform; // set identity matrix
    currentTransform = glm::rotate(currentTransform, rotation.pyGet() * 0.0174532925f, glm::vec3(0.0f,0.0f, 1.0f));
    currentTransform = glm::translate( currentTransform, position.pyGet());
    currentTransform = glm::scale(currentTransform, glm::vec3(scaleX.pyGet(), scaleY.pyGet(), 1.0));
}

// -----------------------------------------------------------------------------------
// This depends on the lists being sorted order.
// and will return tru if there is any overlap.
bool RO_Base::Intersects(stringList &list)
{
    renderSetType::const_iterator rsit = renderSet.cbegin();
    stringList::const_iterator slit = list.cbegin();
    while( slit != list.cend() && rsit != renderSet.cend() )
    {
        if( *slit < *rsit )
        {
            ++slit;
        }
        else if ( *rsit < *slit )
        {
            ++rsit;
        }
        else
         return true;
    }
    return false;
}
// -----------------------------------------------------------------------------------
bool RO_Base::Collectable(stringList &list)
{
    if (!enabled())
    {
        return 0;
    }
    // If there is no renderset for this element, then it is renderable by default
    // this allows children to inherit there parents renderSet
    // if (renderSet().size() == 0)
    // {
    //     return 1;
    // }

    return Intersects(list);
}

// -----------------------------------------------------------------------------------
RO_IteratorPtr RO_Base::Find(std::string searchName)
{
     RO_Iterator *itr = new RO_Iterator();
     FindItems(searchName, itr);
     return boost::shared_ptr<RO_Iterator>(itr);
}

// -----------------------------------------------------------------------------------
void RO_Base::Debug(void)
{
    printf("RO_BASE:\n");
    enabled.Debug("    ");
    rotation.Debug("    ");
    scaleX.Debug("    ");
    scaleY.Debug("    ");
    position.Debug("    ");
    alpha.Debug("    ");
    renderSet.Debug("    ");
    visibleState.Debug("    ");

}

void RO_Base::Test(stringList lst)
{
    printf("Test:\n");
    for (std::vector<std::string>::const_iterator it = lst.begin(); it < lst.end(); it++)
    {
        printf("    %s\n", (*it).c_str());
    }
}

// -----------------------------------------------------------------------------------
void RO_Base::DecodeYaml(YAML::Node node)
{
    if (node["enabled"])    enabled << node;
    if (node["rotation"])   rotation << node;
    if (node["scaleX"])     scaleX << node;
    if (node["scaleY"])     scaleY << node;
    if (node["position"])
    {
        YAML::Node n = node["position"];

        position.pySet(glm::vec3(n[0].as<float>(), n[1].as<float>(), n[2].as<float>() ));

    }
    // if (node["position"])   position << node;
    if (node["alpha"])      alpha << node;
    if (node["Attribs"])
    {
        DecodeMapYaml(node["Attribs"]);
    }
    if (node["__renderSet"])
    {
        YAML::Node renderSetNodes = node["__renderSet"];
        for (YAML::const_iterator it=renderSetNodes.begin(); it!=renderSetNodes.end(); ++it)
        {
            renderSet.push_back( it->as<string>() );
        }
    }
    // renderSet << node;
}

// -----------------------------------------------------------------------------------
void RO_Base::DecodeMapYaml(YAML::Node node)
{
    if (node["VisibleState"])
    {
        visibleState.pySet(node["VisibleState"].as<std::string>());
    }
}

// -----------------------------------------------------------------------------------
void RO_Base::SetState(std::string renderSetName, std::string stateName)
{
    if (stateName == visibleState.pyGet())
    {
        PyFind(renderSet, renderSetName, it);
        if (it == renderSet.pyend())
        {
            renderSet.push_back(renderSetName);
        }
    }
    else
    {
        PyFind(renderSet, renderSetName, it);
        if (it != renderSet.pyend())
        {
            renderSet.erase(it);
        }
    }
}
// -----------------------------------------------------------------------------------
void RO_Base::SetState(std::string stateName)
{
    PyFind(renderSet, "**ALL**", it);
    if (stateName == visibleState.pyGet())
    {
        // Add **ALL** to the render set if it isn't there already.
        if (it == renderSet.pyend())
        {
            renderSet.push_back("**ALL**");
        }
    }
    else
    {
        // remove **ALL** from the render set if it exists
        if (it != renderSet.pyend())
        {
            renderSet.erase(it);
        }

    }
}
// -----------------------------------------------------------------------------------
void RO_Base::ComputeAABB(glm::mat4 parentsTransform)
{
    aabb.Add(position.pyGet());
}

// -----------------------------------------------------------------------------------
AxisAlignedBoundingBox RO_Base::GetAABB(bool)
{
    return aabb;
}

// ===================================================================================
// -----------------------------------------------------------------------------------
void delete_item(RO_BaseVector& container, typename RO_BaseVector::size_type i)
{
    printf("Deleteing Item \n");
    container.erase(container.begin()+i);
}
// -----------------------------------------------------------------------------------
void append(RO_BaseVector& container,  typename RO_BaseVector::value_type const& v)
{
    printf("Appending Item\n");
    container.push_back(v);
}

// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
RO_Iterator::RO_Iterator( void ) : items()
{}

// -----------------------------------------------------------------------------------
RO_Iterator::~RO_Iterator( void )
{}

// -----------------------------------------------------------------------------------
void RO_Iterator::Add(RO_BasePtr itm)
{
    items.push_back(itm);
}

// -----------------------------------------------------------------------------------
void RO_Iterator::Boost(void)
{
    class_<RO_Iterator, RO_IteratorPtr>("RO_BASE_ITTERATOR", no_init)
        .def("__iter__", boost::python::range(&RO_Iterator::cbegin, &RO_Iterator::cend))
        .def("__len__", &RO_Iterator::size)
        .def("__getitem__", &RO_Iterator::getItem)
    ;
}
