// The Bullet system is responsible for inflicting damage and cleaning up bullets
#ifndef LOGICHELPERS_H
#define LOGICHELPERS_H

// the renderer is the only logic class that can't use the logic helpers class
#include "VulkanRendererLogic.h"
#include "../../flecs-3.1.4/flecs.h"
#define SCREEN_BOUND_DISTANCE_X 230
#define SCREEN_BOUND_DISTANCE_Y 330

// example space game (avoid name collisions)
namespace USG
{
	enum SCREEN_BOUNDS_ARR_POS {
		LEFT, RIGHT, TOP, BOTTOM
	};
	class LogicHelpers
	{
		GW::MATH::GMatrix matrixProxy;
		VulkanRendererLogic* vkRenderLogic;
		std::shared_ptr<flecs::world> game;
		flecs::system helperSystem;
		flecs::system updatePos;
		float moveSpeed = 0.5f;
	public:
		// added 3/25/24 by DD
		void Init(std::shared_ptr<flecs::world> _game, VulkanRendererLogic* _vkRenderLogic);
		/// <summary>
		/// Finds Screen Bounds
		/// </summary>
		/// <returns>LEFT, RIGHT, TOP, BOTTOM</returns>
		GW::MATH::GVECTORF FindScreenBounds();

		// control if the system is actively running
		bool Activate(bool runSystem);

		//3-28 Mac
		//bool to set up run time for background
		bool active = true;
	};
};

#endif