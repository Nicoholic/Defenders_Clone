#include "EnemyData.h"
#include "../Components/Identification.h"
#include "../Components/visuals.h"
#include "../Components/Physics.h"
#include "../Entities/Prefabs.h"
#include "../Components/Gameplay.h"
#include "../../Level-Import/LevelDataLoad.h"

bool USG::EnemyData::Load(	std::shared_ptr<flecs::world> _game,
							std::weak_ptr<const GameConfig> _gameConfig,
							GW::AUDIO::GAudio _audioEngine, LEVEL_DATA levelData, GW::MATH::GMATRIXF startPOS)
{
	// Grab init settings for players
	std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();

	// Create prefab for lazer weapon
	// color
	float angle = (*readCfg).at("Enemy1").at("angle").as<float>();
	int health = (*readCfg).at("Enemy1").at("health").as<int>();
	//scale
	float xscale = (*readCfg).at("Enemy1").at("xscale").as<float>();
	float yscale = (*readCfg).at("Enemy1").at("yscale").as<float>();
	float zscale = (*readCfg).at("Enemy1").at("zscale").as<float>();
	float wscale = (*readCfg).at("Enemy1").at("wscale").as<float>();
	
	int tempRandPowerUP = (*readCfg).at("Enemy2").at("tempRandPowerUP").as<int>();
	// default projectile orientation & scale
	GW::MATH::GVECTORF scale = {xscale,yscale,zscale,wscale};
	startPOS = {-0,0,1,0
	,0,1,0,0,
	1,0,-0,0,
	2891,-230,500,1};
	// add prefab to ECS
	auto enemyPrefab = _game->prefab("Enemy Type1")
		// .set<> in a prefab means components are shared (instanced)
		.set<Material>({}) 
		.set<Orientation>({scale}) 
		//this should set the new orientation 
		.set_override<ModelData>({startPOS,1}) 
		// .override<> ensures a component is unique to each entity created from a prefab
		.set_override<Health>({ health })
		.set<Damage>({1})
		.override<Amplitude>()
		//.override<Damage>({1})
		.override<Fequency>()
		.add<Enemy>() // Tag this prefab as an enemy (for queries/systems)
		.add<Collidable>(); // can be collided with
	RegisterPrefab("Enemy Type1", enemyPrefab);
	

	
	

	auto waspPrefab = _game->prefab("Enemy Type2") 
		// .set<> in a prefab means components are shared (instanced)
		.set<Material>({}) 
		.set<Orientation>({ scale }) 
		//this should set the new orientation 
		.set_override<ModelData>({startPOS,2})
		// .override<> ensures a component is unique to each entity created from a prefab
		.set_override<Health>({ health }) 
		.override<Start>()
		.override<Amplitude>()
		.set<Damage>({1})
		.override<Fequency>() 
		.add<Enemy>() // Tag this prefab as an enemy (for queries/systems) 
		.add<Collidable>(); // can be collided with 
		
	// register this prefab by name so other systems can use it
	RegisterPrefab("Enemy Type2", waspPrefab); 




	//Harlan 3/20
	// 
	//set the rand in here 
	auto powerUp = _game->prefab("Enemy Type3")
		.set_override<ModelData>({startPOS ,0})
		.set<RandPowerUP>({ 3 })
		.set_override<Health>({ 1 })
		.override<Amplitude>()
		.set<Damage>({ 1 })
		.override<Fequency>()
		.add<PowerUp>()
		.add<Collidable>();
		
	// register this prefab by name so other systems can use it
	RegisterPrefab("Enemy Type3", powerUp);



	return true;
}

bool USG::EnemyData::Unload(std::shared_ptr<flecs::world> _game)
{
	// remove all bullets and their prefabs
	_game->defer_begin(); // required when removing while iterating!
	_game->each([](flecs::entity e, Enemy&) {
		e.destruct(); // destroy this entitiy (happens at frame end)
	});
	_game->defer_end(); // required when removing while iterating!

	return true;
}
