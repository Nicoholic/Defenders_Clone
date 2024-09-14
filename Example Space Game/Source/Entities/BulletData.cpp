#include "BulletData.h"
#include "../Components/Identification.h"
#include "../Components/Visuals.h"
#include "../Components/Physics.h"
#include "Prefabs.h"
#include "../Components/Gameplay.h"


bool USG::BulletData::Load(	std::shared_ptr<flecs::world> _game,
							std::weak_ptr<const GameConfig> _gameConfig,
							GW::AUDIO::GAudio _audioEngine)
{
	// Grab init settings for players
	std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();
	// Create prefab for lazer weapon
	// other attributes
	//GW::MATH::GVECTORF scale = (*readCfg).at("Lazers").at("xscale").as<GW::MATH::GVECTORF>();
	int dmg = (*readCfg).at("Lazers").at("damage").as<int>();
	float angle = (*readCfg).at("Lazers").at("angle").as<float>();
	std::string fireFX = (*readCfg).at("Lazers").at("fireFX").as<std::string>();
	// Load sound effect used by this bullet prefab
	GW::AUDIO::GSound shoot;
	shoot.Create(fireFX.c_str(), _audioEngine, 0.15f); // we need a global music & sfx volumes
	// add prefab to ECS

	auto lazerPrefab = _game->prefab()
		.set<Damage>({ 10 })
		.set<ModelData>({ GW::MATH::GIdentityMatrixF, 3 })
		.set<GW::AUDIO::GSound>(shoot.Relinquish())
		.override<LightData>()
		.override<Bullet>() // Tag this prefab as a bullet (for queries/systems)
		.override<Collidable>(); // can be collided with

	// register this prefab by name so other systems can use it
	RegisterPrefab("Lazer Bullet", lazerPrefab);
	
	return true;
}

bool USG::BulletData::Unload(std::shared_ptr<flecs::world> _game)
{
	// remove all bullets and their prefabs
	_game->defer_begin(); // required when removing while iterating!
	_game->each([](flecs::entity e, Bullet&) {
		e.destruct(); // destroy this entitiy (happens at frame end)
	});
	_game->defer_end(); // required when removing while iterating!

	// unregister this prefab by name
	UnregisterPrefab("Lazer Bullet");

	return true;
}
