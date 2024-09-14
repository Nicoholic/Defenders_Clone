#include <random>
#include "LevelLogic.h"
#include "../Components/Identification.h"
#include "../Components/Physics.h"
#include "../Components/Gameplay.h"
#include "../Entities/Prefabs.h"
#include "../Utils/Macros.h"

using namespace USG; // Example Space Game

// Connects logic to traverse any players and allow a controller to manipulate them
bool USG::LevelLogic::Init(std::shared_ptr<flecs::world> _game,
	std::weak_ptr<const GameConfig> _gameConfig,
	GW::AUDIO::GAudio _audioEngine)
{
	// save a handle to the ECS & game settings
	game = _game;
	gameConfig = _gameConfig;
	audioEngine = _audioEngine;
	// create an asynchronus version of the world
	gameAsync = game->async_stage(); // just used for adding stuff, don't try to read data
	gameLock.Create();
	// Pull enemy Y start location from config file
	std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();
	//float enemy1startX = (*readCfg).at("Enemy1").at("xstart").as<float>();
	float enemy1startY = (*readCfg).at("Enemy1").at("ystart").as<float>();
	float enemy1accmax = (*readCfg).at("Enemy1").at("accmax").as<float>();
	float enemy1accmin = (*readCfg).at("Enemy1").at("accmin").as<float>();
	// level one info
	float spawnDelay = (*readCfg).at("Level1").at("spawndelay").as<float>();
	auto tempMTX = GW::MATH::GMATRIXF{ 5, 0, 0, 0,
										0, 5, 0, 0,
										0, 0, 5, 0,
									  -12, 245, -4050, 1 };
	//enemy counter
	static int a = 0;

	GW::MATH::GMatrix::TranslateLocalF(tempMTX, GW::MATH::GVECTORF{ 5, 0, 0 ,0 }, tempMTX);
	// spins up a job in a thread pool to invoke a function at a regular interval

	timedEvents.Create(spawnDelay * 1000, [this, enemy1startY, enemy1accmax, enemy1accmin]() {
		//paused enemy spawns when game is in paused state!
		if (active == true)
		{
			// Load and play level one's music
			//Nathan Thorpe (3/5)
			//Updating music
			if (runMusic == true)
			{
				currentTrack.Create("../Music/Houston_Weve_Got_Aliens.wav", audioEngine, 0.15f);
				currentTrack.Play(runMusic);
				runMusic = false;
			}

			// compute random spawn location
			std::random_device rd;  // Will be used to obtain a seed for the random number engine
			std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
			std::uniform_real_distribution<float> x_range(-0.9f, +0.9f);
			std::uniform_real_distribution<float> a_range(enemy1accmin, enemy1accmax);
			//Nathan 3/20
			//Random amplitude, fequency, and id
			std::uniform_real_distribution<float> amp_range(-100, +60);
			std::uniform_real_distribution<float> feq_range(-100, 40);
			std::uniform_int_distribution<int> eID(static_cast<unsigned int>(BEE_ENEMY_ID), static_cast<unsigned int>(WASP_ENEMY_ID));
			std::uniform_int_distribution<int> iD(1, 2);
			float Xstart = x_range(gen); // normal rand() doesn't work great multi-threaded 
			float accel = a_range(gen);
			float Ape = amp_range(gen);
			float freck = feq_range(gen);
			unsigned int id = eID(gen);
			int Pow = iD(gen);
			// grab enemy type 1 prefab

			flecs::entity et1;
			flecs::entity et2;
			flecs::entity powerUp;
			gameLock.LockSyncWrite();
			if (RetreivePrefab("Enemy Type1", et1)) {

				// you must ensure the async_stage is thread safe as it has no built-in synchronization

			// this method of using prefabs is pretty conveinent
				gameAsync.entity().is_a(et1)
					// Nathan 3/14
					//Gonna update this to make Amp and Freq random variables
					//.set<Velocity>({ 1,0 })
					//.set<Acceleration>({ 10, accel })
					////Nathan 3/14
					////Remember to update the defaults accmin and accmax
					//.set<Start>({})
					.set<Health>({ 10 })
					.set<Amplitude>({ Ape })
					.set<Fequency>({ freck })
					.set_override<ModelData>({ 5, 0, 0, 0,
												   0, 5, 0, 0,
												   0, 0, 5, 0,
												 -12, 245, -3750,1, id });

				//.set<Position>({ enemy1startY, Xstart })
			}

			if (RetreivePrefab("Enemy Type2", et2))
			{
				gameAsync.entity().is_a(et2)
					.set<Health>({ 10 })
					.set<Amplitude>({ Ape })
					.set<Fequency>({ freck })
					.set_override<ModelData>({ 5, 0, 0, 0,
												   0, 5, 0, 0,
												   0, 0, 5, 0,
												 -12, 245, -3750,1,id });
				spawn++;
			}

			spawnPOW = true;
			if (RetreivePrefab("Enemy Type3", powerUp)) {

				int a = 0;
				//	// this method of using prefabs is pretty conveinent
				gameAsync.entity().is_a(powerUp)
					.set<RandPowerUP>({ Pow })
					//Nathan 3/30
					//Added functionality to wave formula; tweaked position
					.set<Amplitude>({ Ape })
					.set<Fequency>({ freck })
					.set_override<ModelData>({ 5, 0, 0, 0,
											   0, 5, 0, 0,
											   0, 0, 5, 0,
											 -12, 545, -3981, 0 });
				spawnPOW = false;
				spawn++;
			}
			gameLock.UnlockSyncWrite();
		}
		}, 2000); // wait 2 seconds to start enemy wave (change back to 2 after collision  and startPOS works)


	// create a system the runs at the end of the frame only once to merge async changes
	struct LevelSystem {}; // local definition so we control iteration counts
	game->entity("Level System").add<LevelSystem>();
	// only happens once per frame at the very start of the frame
	game->system<LevelSystem>().kind(flecs::OnLoad) // first defined phase
		.each([this](flecs::entity e, LevelSystem& s) {
		// merge any waiting changes from the last frame that happened on other threads
		gameLock.LockSyncWrite();
		gameAsync.merge();
		gameLock.UnlockSyncWrite();
			});
	return true;
}



// Free any resources used to run this system
bool USG::LevelLogic::Shutdown()
{
	timedEvents = nullptr; // stop adding enemies
	gameAsync.merge(); // get rid of any remaining commands
	game->entity("Level System").destruct();
	// invalidate the shared pointers
	game.reset();
	gameConfig.reset();
	return true;
}

// Toggle if a system's Logic is actively running
bool USG::LevelLogic::Activate(bool runSystem)
{
	if (runSystem) {
		game->entity("Level System").enable();
	}
	else {
		game->entity("Level System").disable();
	}
	return false;
}

// **** SAMPLE OF MULTI_THREADED USE ****
//flecs::world world; // main world
//flecs::world async_stage = world.async_stage();
//
//// From thread
//lock(async_stage_lock);
//flecs::entity e = async_stage.entity().child_of(parent)...
//unlock(async_stage_lock);
//
//// From main thread, periodic
//lock(async_stage_lock);
//async_stage.merge(); // merge all commands to main world
//unlock(async_stage_lock);