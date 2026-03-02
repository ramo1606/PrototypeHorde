#pragma once

#include "Component.h"
#include "Defs.h"
#include <vector>
#include <map>
using namespace std;

class Scene;

class SceneObject 
{
public:
	SceneObject(Scene* Scene, const char* Name = nullptr);
	virtual ~SceneObject();

public:
	int ID;
	void SetName(const char* Name);
	const char* GetName() const;
	void SetParent(SceneObject* parent);
	bool IsActive;

	template<class T>
	T* GetComponent()
	{
		T* foundComponent = nullptr;
		map<Component::eComponentType, Component*>::iterator it = _Components.begin();
		while (_Components.end() != it)
		{
			foundComponent = dynamic_cast<T*>(it->second);
			if (foundComponent != nullptr)
			{
				break;
			}
			++it;
		}
		return foundComponent;
	}

	template<class T>
	T* CreateAndAddComponent()
	{
		T* component = new T();
		if (!AddComponent(component))
		{
			delete component;
			component = nullptr;
		}
		return component;
	}

	virtual bool AddComponent(Component* Component);
	Component* GetComponent(Component::eComponentType ComponentType);
	bool HasComponent(Component::eComponentType ComponentType);
	virtual Component* RemoveComponent(Component::eComponentType ComponentType, bool destroy = true);

	virtual bool Update(float ElapsedSeconds);
	virtual bool Draw();

public:
	SceneObject* Parent;
	//SceneObject* FirstChild;
	//SceneObject* NextSibling;
	//SceneObject* PrevSibling;

	vector<SceneObject*> _Children;

protected:
	Scene* _Scene;

	char _Name[MAX_SCENE_OBJECT_NAME];
	map<Component::eComponentType, Component*> _Components;

	friend class SceneRenderPass;
	friend class ShadowMapRenderPass;
};