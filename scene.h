#ifndef SCENE_H
#define SCENE_H
#include<vector>
#include "DynamicObject.h"
class Scene
{
private:
	std::string name;
	std::vector<std::shared_ptr<Object>> staticEntities;
	std::vector<std::shared_ptr<DynamicObject>> dynamicEntities;
public:
	Scene() = default;
	Scene(std::string);
	DynamicObject& addNewDynamicEntity(std::shared_ptr<DynamicObject> obj);
	Object& addNewStaticEntity(std::shared_ptr<Object> obj);
	~Scene() = default;
};
#endif