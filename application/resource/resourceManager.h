#pragma once

#include "core.h"

#include <typeinfo>

#include "resource.h"

class ResourceManager {
	friend Resource;

private:
	struct CountedResource {
		Resource* value;
		int count;
	};

	std::unordered_map<ResourceType, Resource*> defaultResources;
	std::unordered_map<std::string, CountedResource> resources;

	static ResourceManager& instance() {
		static ResourceManager resourceManager;

		return resourceManager;
	}

	static void onResourceNameChange(Resource* changedResource, const std::string& newName) {
		ResourceManager manager = instance();

		auto iterator = manager.resources.find(changedResource->getName());

		if (iterator == manager.resources.end()) {
			Log::error("The changed resource is not found (%s)", changedResource->getName().c_str());
			return;
		} else {
			if (manager.resources.find(newName) != manager.resources.end()) {
				Log::error("Resource name has already been taken (%s)", newName.c_str());
				return;
			} else {
				CountedResource countedResource = iterator->second;
				countedResource.value->name = newName;
				manager.resources.emplace(newName, countedResource);
				manager.resources.erase(iterator);
			}
		}
	}

	static void onResourcePathChange(Resource* changedResource, const std::string& newPath) {
		ResourceManager manager = instance();

		auto iterator = manager.resources.find(changedResource->getName());

		if (iterator == manager.resources.end()) {
			Log::error("The changed resource is not found (%s)", changedResource->getName().c_str());
			return;
		} else {
			auto& resource = iterator->second.value;

			if (resource->getName() == resource->getPath()) {
				onResourceNameChange(changedResource, newPath);	
			} else {
				//? Load new resource
			}

			resource->path = newPath;
		}
	}

	void initDefaultResources() {
		
	}

	template<typename T>
	static T* getDefaultResource() {
		ResourceManager manager = instance();

		auto iterator = manager.defaultResources.find(T::getStaticType());

		if (iterator != manager.defaultResources.end())
			return static_cast<T*>(iterator->second);
		else {
			Resource* defaultResource = manager.add<T>(T::getDefaultPath());

			if (defaultResource) {
				manager.defaultResources[T::getStaticType()] = defaultResource;
				return static_cast<T*>(defaultResource);
			} else {
				return nullptr;
			}
		}
	}

	ResourceManager() {
		initDefaultResources();
	}

public:
	template<typename T> 
	static T* get(const std::string& name) {
		ResourceManager& manager = instance();

		auto iterator = manager.resources.find(name);

		if (iterator != manager.resources.end()) {
			return static_cast<T*>(iterator->second.value);
		} else {
			Log::warn("Resource not loaded (%s), trying to load requested resource...", name.c_str());
			T* resource = ResourceManager::add<T>(name);

			if (resource) {
				return resource;
			} else {
				Log::warn("Resource could not be loaded (%s), using default resource", name.c_str());
				return manager.getDefaultResource<T>();
			}
		}
	}

	template<typename T>
	static T* add(const std::string& path, std::string name = "") {
		if (name.empty())
			name = path;

		ResourceManager& manager = instance();

		auto iterator = manager.resources.find(name);

		if (iterator != manager.resources.end()) {
			auto& resource = (*iterator).second;
			resource.count++; 
			return static_cast<T*>(resource.value);
		} else {
			T* resource = T::getAllocator().load(path, name);

			if (resource == nullptr) {
				Log::warn("Resource could not be loaded (%s)", path.c_str());
				return nullptr;
			} else {
				CountedResource countedResource = { resource, 1 };
				manager.resources.emplace(name, countedResource);

				return resource;
			}
		}
	}

	static bool exists(const std::string& name) {
		ResourceManager& manager = instance();

		auto iterator = manager.resources.find(name);
		return iterator != manager.resources.end();
	}

	template<typename T>
	static std::vector<T*> getResourcesOfClass() {
		std::vector<T*> resources;
		ResourceManager& manager = ResourceManager::instance();

		for (auto iterator : manager.resources) {
			if (typeid(T) == typeid(iterator.second.value))
				resources.push_back(static_cast<T*>(iterator.second.value));
		}
	}

	static std::vector<Resource*> getResourcesOfType(ResourceType type) {
		std::vector<Resource*> resources;
		ResourceManager& manager = ResourceManager::instance();
		
		for (auto iterator : manager.resources) {
			if (iterator.second.value->getType() == type)
				resources.push_back(iterator.second.value);
		}

		return resources;
	}

	static std::vector<Resource*> getResources() {
		std::vector<Resource*> resources;
		ResourceManager& manager = ResourceManager::instance();

		for (auto iterator : manager.resources) {
			resources.push_back(iterator.second.value);
		}

		return resources;
	}
};