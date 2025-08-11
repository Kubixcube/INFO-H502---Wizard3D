#include "scene.h"

Scene::Scene(std::string n) : name(n)
{
}

DynamicObject& Scene::addNewDynamicEntity(std::shared_ptr<DynamicObject> obj)
{
    dynamicEntities.push_back(obj);
    return *dynamicEntities.back();
}

Object& Scene::addNewStaticEntity(std::shared_ptr<Object> obj)
{
    staticEntities.push_back(obj);
    return *staticEntities.back();
}
