#pragma once

#include "part.h"
#include "physical.h"
#include <vector>
#include <mutex>

class World {
private:
	size_t getTotalVertexCount();

public:
	std::vector<Physical> physicals;
	std::mutex lock;

	Physical* selectedPhysical = nullptr;
	Vec3 localSelectedPoint;
	Vec3 magnetPoint;

	World();

	World(const World&) = delete;
	World(World&&) = delete;
	World& operator=(const World&) = delete;
	World& operator=(World&&) = delete;

	void tick(double deltaT);
	void addObject(Physical p);

	virtual void applyExternalForces(const Shape* transformedShapes);
};
