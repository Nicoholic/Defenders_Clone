// The level system is responsible for transitioning the various levels in the game
#ifndef PHYSICSLOGIC_H
#define PHYSICSLOGIC_H

// Contains our global game settings
#include "../GameConfig.h"
#include "../Components/Physics.h"

// example space game (avoid name collisions)
namespace USG
{
	class PhysicsLogic
	{
		// shared connection to the main ECS engine
		std::shared_ptr<flecs::world> game;
		// non-ownership handle to configuration settings
		std::weak_ptr<const GameConfig> gameConfig;
		// used to cache collision queries
		flecs::query<Collidable, ModelData> queryCache;
		// defines what to be tested
		static constexpr unsigned polysize = 4;
		struct SHAPE {
			GW::MATH::GCAPSULEF collisionShape;
			flecs::entity owner;
		};
		// vector used to save/cache all active collidables
		std::vector<SHAPE> testCache;
		std::map<unsigned, GW::MATH::GMATRIXF> collisionMap;
	public:
		// attach the required logic to the ECS 
		bool Init(std::shared_ptr<flecs::world> _game,
			std::weak_ptr<const GameConfig> _gameConfig, 
			std::map<unsigned, GW::MATH::GMATRIXF> _collisionMap);
		// control if the system is actively running
		bool Activate(bool runSystem);

		//Mac 3-17
		//bool to set up run time for physics
		bool active = true;

		// release any resources allocated by the system
		bool Shutdown();
	};

};

#endif