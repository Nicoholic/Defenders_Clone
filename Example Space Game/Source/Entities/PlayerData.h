// This class populates all player entities 
#ifndef PLAYERDATA_H
#define PLAYERDATA_H
#include "../../Level-Import/LevelDataLoad.h"
// Contains our global game settings
#include "../GameConfig.h"

// example space game (avoid name collisions)
namespace USG
{
	class PlayerData
	{
	public:
		// Load required entities and/or prefabs into the ECS 
		bool Load(	std::shared_ptr<flecs::world> _game,
					std::weak_ptr<const GameConfig> _gameConfig, LEVEL_DATA levelData, GW::MATH::GMATRIXF playerPOS);
		// Unload the entities/prefabs from the ECS
		bool Unload(std::shared_ptr<flecs::world> _game);
	};

};

#endif