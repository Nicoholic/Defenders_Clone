#include <random>
#include "BulletLogic.h"
#include "../Components/Identification.h"
#include "../Components/Physics.h"
#include "../Components/Visuals.h"
#include "../Components/Gameplay.h"
#define deltaTime static_cast<float>(e.delta_time())
using namespace USG; // Example Space Game
//rand test mac
// Connects logic to traverse any players and allow a controller to manipulate them
bool USG::BulletLogic::Init(std::shared_ptr<flecs::world> _game,
	std::weak_ptr<const GameConfig> _gameConfig, LogicHelpers _logicHelper)
{
	// save a handle to the ECS & game settings
	game = _game;
	gameConfig = _gameConfig;
	logicHelper = _logicHelper;


	std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();
	//Harlan 3 / 19
	//added to added bulletSpeed  in defaults.ini
	bulletSpeed = (*readCfg).at("Player1").at("bulletSpeed").as<float>();
	
	// destroy any bullets that have the CollidedWith relationship
	//Harlan 3/18 
	//took out damage for the moment will add when enemies come back
	
	
	game->system<Bullet, Damage, LightData, ModelData>("Bullet System")
		.each([this](flecs::entity e, Bullet, Damage &d, LightData &ld, ModelData& t) {

		//Harlan 3/18
		//this is what makes the bullets move  
		
		GW::MATH::GMatrix::TranslateLocalF(t.positionMatrix, GW::MATH::GVECTORF{ 0,0,-100 * bulletSpeed * deltaTime ,1000 * bulletSpeed * deltaTime }, t.positionMatrix);
		ld.lightPos = t.positionMatrix;
		// damage anything we come into contact with
		// 
		

		e.each<CollidedWith>([&e, d](flecs::entity hit) {
			if (hit.has<Enemy>()) {
				int current = hit.get<Health>()->value;
				hit.set<Health>({ current - d.value });
				// reduce the amount of hits but the charged shot
				e.destruct();
				hit.destruct();
			}
			});
		// if you have collidedWith realtionship then be destroyed
		
		GW::MATH::GVECTORF screenBounds = logicHelper.FindScreenBounds();
		if (screenBounds.data[SCREEN_BOUNDS_ARR_POS::LEFT] > t.positionMatrix.row4.z || screenBounds.data[SCREEN_BOUNDS_ARR_POS::RIGHT] < t.positionMatrix.row4.z ||
			screenBounds.data[SCREEN_BOUNDS_ARR_POS::BOTTOM] > t.positionMatrix.row4.y || screenBounds.data[SCREEN_BOUNDS_ARR_POS::TOP] < t.positionMatrix.row4.y) {
			e.destruct();
		}
			});

	return true;
}

// Free any resources used to run this system
bool USG::BulletLogic::Shutdown()
{
	game->entity("Bullet System").destruct();
	// invalidate the shared pointers
	game.reset();
	gameConfig.reset();
	return true;
}

// Toggle if a system's Logic is actively running
bool USG::BulletLogic::Activate(bool runSystem)
{
	if (runSystem) {
		game->entity("Bullet System").enable();
	}
	else {
		game->entity("Bullet System").disable();
	}
	return false;
}
