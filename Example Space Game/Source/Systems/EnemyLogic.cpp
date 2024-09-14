#include <random>
#include "EnemyLogic.h"
#include "../Components/Identification.h"
#include "../Components/Physics.h"
#include "../Components/Gameplay.h"
#include "../Events/Playevents.h"
#include"../../Level-Import/HighScores.h"
#define deltaTime static_cast<float>(e.delta_time())
using namespace USG; // Example Space Game

// Connects logic to traverse any players and allow a controller to manipulate them
bool USG::EnemyLogic::Init(std::shared_ptr<flecs::world> _game,
	std::weak_ptr<const GameConfig> _gameConfig,
	GW::CORE::GEventGenerator _eventPusher)
{
	// save a handle to the ECS & game settings
	game = _game;
	gameConfig = _gameConfig;
	eventPusher = _eventPusher;
	std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();

	int travel = (*readCfg).at("Enemy1").at("travelspeed").as<int>();


	// destroy any bullets that have the CollidedWith relationship
	game->system<Enemy, Health, ModelData, Damage>("Enemy System")
		.each([this](flecs::entity e, Enemy, Health& h, ModelData& t, Damage& d) {
		// if you have no health left be destroyed
		auto tempScore = score;
		//GW::AUDIO::GAudio tempAudio = audioEngine;
		//GW::AUDIO::GMusic tempMusic = currentTrack;
		/*iframe.Create(1000, [&e, d, tempScore](flecs::entity hit)
			{*/
				e.each<CollidedWith>([this, e, d, tempScore/*, tempMusic, tempAudio*/](flecs::entity hit) {
					if (hit.has<Player>() && hit.get<Health>()->value > 0) {
						int current = hit.get<Health>()->value;
						// timer go here 
						if (true)
						{
							hit.set<Health>({ current - d.value });
							hit.remove<CollidedWith>();
						}

						if (hit.get<Health>()->value == 0)
						{
							std::cout << "You Lose !!! :) \n" << "Total Score: " << tempScore << std::endl;
							hit.remove<ModelData>();
							hit.add<LevelReset>();
							hit.remove<CollidedWith>();
							hit.set<Health>({ 3 });
							e.destruct();// this killed me ;-;
							
							//tempMusic.Create("../Music/Billions_and_Billions.wav", tempAudio, 0.15f);
							//tempMuisc.Play(true);
						}
						
						/*std::cout << "---Enemy Logic--- \n" << "Health of Player: " << hit.get<Health>()->value << "\n";*/
					}
				});
		/*}, 1000);*/

		if (e.get<Health>()->value <= 0) {
			// play explode sound
			e.destruct();
			USG::PLAY_EVENT_DATA x;
			GW::GEvent explode;
			explode.Write(USG::PLAY_EVENT::ENEMY_DESTROYED, x);
			eventPusher.Push(explode);
			//Nathan 3/30
			//Score based on enemy typed killed
			if (e.get<ModelData>()->modelIndex == 1)
			{
				score += 100;
				KIA += 1;

			}
			else if (e.get<ModelData>()->modelIndex == 2)
			{
				score += 200;
				KIA += 1;
			}
			score -= 1.009;
			std::cout << "---Enemy Logic--- \n"
				<< "Score: " << score << "\n Killed: " << KIA << std::endl;
			if (KIA == 25)
			{

				EnemyLogic::Shutdown();


				system("cls");
				std::cout << "YOU WIN!!! :) \n" << "Total Score: " << score << std::endl;

				string player_name;
				int player_score = score;
				std::cout << "Enter player name: ";
				std::cin >> player_name;
				updateScores(player_name, player_score);
				printTop5Scores();

				
				e.world().remove_all(e);

			}

		}



			});


	game->system<PowerUp, Health, ModelData, Amplitude, Fequency>("PowerUp System")
		.each([this](flecs::entity e, PowerUp, Health& h, ModelData& t, Amplitude& a, Fequency& f) {

		auto powerUpMTX = e.get<ModelData>()->positionMatrix;
		powerUpMTX.row4.y = a.value * sin((2 * PI * f.value * time));
		powerUpMTX.row4.x = a.value * cos((2 * PI * f.value * time));
		//Nathan 3/30
		//Swaped input and output; tweaked values and applied matrix
		GW::MATH::GMatrix::TranslateLocalF(t.positionMatrix, GW::MATH::GVECTORF{ (-powerUpMTX.row4.y / 10000),(-0.15),(-powerUpMTX.row4.x / 10000) ,4 }, powerUpMTX);
		e.set_override<ModelData>({ powerUpMTX, 0 });
		if (e.get<Health>()->value == 0) {

			//Harlan 04/2
			//This successfully grabs the player 
			auto temp = e.world().lookup("FV_X1_Fighter");

			if (e.get<RandPowerUP>()->value == 1)
			{
				temp.add<SpeedUP>();
				
			}


			else if (e.get<RandPowerUP>()->value == 2)
			{
				temp.add<SmallShip>();
				
			}

			else if (e.get<RandPowerUP>()->value == 3)
			{
				temp.add<Testing>();
				
			}
			e.destruct();
		}

			});

	//Nathan 3/14
	//Creating enemy curving
	// 3/19
	//got enemies to start to jitter, indicating that the use of sine waves is working; futher work required
	game->system<Amplitude, Fequency, ModelData>("Enemy Movement")
		.each([this, travel](flecs::entity e, Amplitude& amp, Fequency& f, ModelData t) {
		auto temp = e.get<ModelData>()->positionMatrix;
		GW::MATH::GVECTORF accel;
		float time = 0;
		time += e.delta_time();
		accel.y = amp.value * sin((2 * PI * f.value * time));
		GW::MATH::GMatrix::ScaleLocalF(t.positionMatrix, accel, temp);
		GW::MATH::GMatrix::TranslateLocalF(t.positionMatrix, GW::MATH::GVECTORF{ 0,(accel.y / 1000),(travel * time) / 10,(travel * time) / 60 }, temp);
		e.set_override<ModelData>({ temp, 1 });

			});

	//Nathan 3/30
	//Second Enemy type Proper
	game->system<Amplitude, Fequency, ModelData, Start>("Wasp Movement")
		.each([this, travel](flecs::entity e, Amplitude& amp, Fequency& f, ModelData t, Start& s) {
		auto temp = e.get<ModelData>()->positionMatrix;
		GW::MATH::GVECTORF accel;
		float time = 0;
		time += e.delta_time();
		accel.x = amp.value * cos((4 * PI * f.value * time));
		GW::MATH::GMatrix::ScaleLocalF(t.positionMatrix, accel, temp);
		GW::MATH::GMatrix::TranslateLocalF(t.positionMatrix, GW::MATH::GVECTORF{ 0,(accel.x / 1000),(travel * time) / 10,(travel * time) / 60 }, temp);
		e.set<ModelData>({ temp, 2 });

			});

	//Nathan 3/21
	//Made the Wasp a seperate entity

	return true;
}


// Free any resources used to run this system
bool USG::EnemyLogic::Shutdown()
{
	game->entity("Enemy System").destruct();
	game->entity("Enemy Movement").destruct();
	game->entity("PowerUp System").destruct();
	game->entity("Wasp Movement").destruct();
	game.reset();
	gameConfig.reset();
	return true;
}



// Toggle if a system's Logic is actively running
bool USG::EnemyLogic::Activate(bool runSystem)
{
	if (runSystem) {
		game->entity("Enemy System").enable();
		game->entity("Enemy Movement").enable();
		game->entity("Wasp Movement").enable();
		game->entity("PowerUp System").enable();
	}
	else {
		game->entity("Enemy System").disable();
		game->entity("PowerUp System").disable();
		game->entity("Enemy Movement").disable();
		game->entity("Wasp Movement").disable();
	}
	return false;
}

