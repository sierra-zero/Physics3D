#include "core.h"

#include "debugLayer.h"

#include "../graphics/renderer.h"
#include "../graphics/meshLibrary.h"
#include "../graphics/mesh/arrayMesh.h"
#include "../graphics/mesh/pointMesh.h"
#include "../graphics/mesh/vectorMesh.h"
#include "../graphics/mesh/indexedMesh.h"
#include "../graphics/debug/visualDebug.h"

#include "../physics/datastructures/boundsTree.h"
#include "../physics/geometry/polyhedron.h"
#include "../physics/math/bounds.h"
#include "../physics/physical.h"
#include "../physics/layer.h"

#include "worlds.h"
#include "view/screen.h"
#include "ecs/material.h"
#include "shader/shaders.h"

namespace P3D::Application {

Vec4f colors[] {
	COLOR::BLUE,
	COLOR::GREEN,
	COLOR::YELLOW,
	COLOR::ORANGE,
	COLOR::RED,
	COLOR::PURPLE
};

void renderSphere(double radius, const Position& position, const Color& color) {
	Shaders::basicShader.updateMaterial(Comp::Material(color));
	Shaders::basicShader.updateModel(join(Mat3f::IDENTITY() * float(radius), castPositionToVec3f(position), Vec3f(0.0f,0.0f,0.0f),1.0f));

	Graphics::Library::sphere->render();
}

void renderBox(const GlobalCFrame& cframe, double width, double height, double depth, const Color& color) {
	Shaders::basicShader.updateMaterial(Comp::Material(color));
	Shaders::basicShader.updateModel(Mat4f(cframe.asMat4WithPreScale(DiagonalMat3{width, height, depth})));

	Graphics::Library::cube->render();
}

void renderBounds(const Bounds& bounds, const Color& color) {
	Vec3Fix diagonal = bounds.getDiagonal();
	Position position = bounds.getCenter();
	renderBox(GlobalCFrame(position), static_cast<double>(diagonal.x), static_cast<double>(diagonal.y), static_cast<double>(diagonal.z), color);
}

static Color getCyclingColor(int depth){
	Color color = colors[depth % 6];
	color.w = 0.3f;
	return color;
}

static void renderBoundsForDepth(const Bounds& bounds, int depth) {
	renderBounds(bounds.expanded((10 - depth) * 0.002), getCyclingColor(depth));
}

static void recursiveRenderColTree(const TreeNode& node, int depth) {
	if (node.isLeafNode()) {
		//renderBounds(node.bounds, GUI::COLOR::AQUA);
	} else {
		for (const TreeNode& node : node) {
			recursiveRenderColTree(node, depth + 1);
		}
	}

	renderBoundsForDepth(node.bounds, depth);
}

static bool recursiveColTreeForOneObject(const TreeNode& node, const Part* part, const Bounds& bounds, int depth) {
	if (node.isLeafNode()) {
		if (node.object == part)
			return true;
		/*for (const BoundedPhysical& p : *node.physicals) {
			if (p.object == physical) {
				return true;
			}
		}*/
	} else {
		//if (!intersects(node.bounds, bounds)) return false;
		for (const TreeNode& subNode : node) {
			if (recursiveColTreeForOneObject(subNode, part, bounds, depth+1)) {
				renderBoundsForDepth(node.bounds, depth);
				return true;
			}
		}
	}
	return false;
}

void DebugLayer::onInit(Engine::Registry64& registry) {

	// Origin init
	originMesh = new Graphics::ArrayMesh(nullptr, 1, 3, Graphics::Renderer::POINT);

	// Vector init
	vectorMesh = new Graphics::VectorMesh(nullptr, 0);

	// Point init
	pointMesh = new Graphics::PointMesh(nullptr, 0);

}

void DebugLayer::onUpdate(Engine::Registry64& registry) {

}

void DebugLayer::onEvent(Engine::Registry64& registry, Engine::Event& event) {

}

void DebugLayer::onRender(Engine::Registry64& registry) {
	using namespace Graphics;
	using namespace Graphics::Renderer;
	using namespace Graphics::Debug;
	using namespace Graphics::AppDebug;
	Screen* screen = static_cast<Screen*>(this->ptr);

	beginScene();
	enableBlending();

	graphicsMeasure.mark(GraphicsProcess::VECTORS);


	// Initialize vector log buffer
	AddableBuffer<ColoredVector>& vecLog = getVectorBuffer();
	AddableBuffer<ColoredPoint>& pointLog = getPointBuffer();

	for (const MotorizedPhysical* physical : screen->world->iterPhysicals()) {
		Position com = physical->getCenterOfMass();
		pointLog.add(ColoredPoint(com, ::Debug::CENTER_OF_MASS));
	}

	screen->world->syncReadOnlyOperation([this, &vecLog]() {
		Screen* screen = static_cast<Screen*>(this->ptr);
		if (screen->selectedPart != nullptr) {
			const GlobalCFrame& selectedCFrame = screen->selectedPart->getCFrame();
			Motion partMotion = screen->selectedPart->getMotion();
			Polyhedron asPoly = screen->selectedPart->hitbox.asPolyhedron();
			for (const Vec3f& corner : asPoly.iterVertices()) {
				vecLog.add(ColoredVector(selectedCFrame.localToGlobal(corner), partMotion.getVelocityOfPoint(selectedCFrame.localToRelative(corner)), ::Debug::VELOCITY));
			}

			if (colissionSpheresMode == SphereColissionRenderMode::SELECTED) {
				Physical& selectedPhys = *screen->selectedPart->parent;

				for(Part& part : selectedPhys.rigidBody) {
					Color yellow = Graphics::COLOR::YELLOW;
					yellow.w = 0.5;
					BoundingBox localBounds = screen->selectedPart->getLocalBounds();
					renderBox(screen->selectedPart->getCFrame().localToGlobal(CFrame(localBounds.getCenter())), localBounds.getWidth(), localBounds.getHeight(), localBounds.getDepth(), yellow);

					Color green = Graphics::COLOR::GREEN;
					green.w = 0.5;
					renderSphere(part.maxRadius, part.getPosition(), green);
				}
			}
		}

		if(colissionSpheresMode == SphereColissionRenderMode::ALL) {
			for(MotorizedPhysical* phys : screen->world->iterPhysicals()) {
				for(Part& part : phys->rigidBody) {
					Color yellow = Graphics::COLOR::YELLOW;
					yellow.w = 0.5;
					BoundingBox localBounds = part.getLocalBounds();
					renderBox(part.getCFrame().localToGlobal(CFrame(localBounds.getCenter())), localBounds.getWidth(), localBounds.getHeight(), localBounds.getDepth(), yellow);

					Color green = Graphics::COLOR::GREEN;
					green.w = 0.5;
					renderSphere(part.maxRadius, part.getPosition(), green);
				}
			}
		}

		if(colTreeRenderMode == -2) {
			if(screen->selectedPart != nullptr) {
				recursiveColTreeForOneObject(screen->selectedPart->layer->tree.rootNode, screen->selectedPart, screen->selectedPart->getBounds(), 0);
			}
		} else if(colTreeRenderMode >= 0) {
			recursiveRenderColTree(getLayerByID(screen->world->layers, colTreeRenderMode)->tree.rootNode, 0);
		}
	});


	disableDepthTest();

	// Update debug meshes
	graphicsMeasure.mark(GraphicsProcess::VECTORS);
	updateVectorMesh(vectorMesh, vecLog.data, vecLog.size);
	updatePointMesh(pointMesh, pointLog.data, pointLog.size);

	// Render vector mesh
	graphicsMeasure.mark(GraphicsProcess::VECTORS);
	Shaders::vectorShader.updateProjection(screen->camera.viewMatrix, screen->camera.projectionMatrix, screen->camera.cframe.position);
	vectorMesh->render();

	// Render point mesh
	graphicsMeasure.mark(GraphicsProcess::VECTORS);
	Shaders::pointShader.updateProjection(screen->camera.viewMatrix, screen->camera.projectionMatrix, screen->camera.cframe.position);
	pointMesh->render();

	// Render origin mesh
	graphicsMeasure.mark(GraphicsProcess::ORIGIN);
	Shaders::originShader.updateProjection(screen->camera.viewMatrix, screen->camera.getViewRotation(), screen->camera.projectionMatrix, screen->camera.orthoMatrix, screen->camera.cframe.position);
	originMesh->render();

	endScene();
}

void DebugLayer::onClose(Engine::Registry64& registry) {

}

};