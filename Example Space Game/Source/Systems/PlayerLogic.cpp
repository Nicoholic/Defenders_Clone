#include "PlayerLogic.h"
#include <time.h>
#include "../Components/Identification.h"
#include "../Components/Physics.h"
#include "../Components/Visuals.h"
#include "../Components/Gameplay.h"
#include "../Entities/Prefabs.h"
#include "../Events/Playevents.h"


using namespace USG; // Example Space Game
using namespace GW::INPUT; // input libs
using namespace GW::AUDIO; // audio libs

// Connects logic to traverse any players and allow a controller to manipulate them
bool USG::PlayerLogic::Init(std::shared_ptr<flecs::world> _game,
	std::weak_ptr<const GameConfig> _gameConfig,
	GW::INPUT::GInput _immediateInput,
	GW::INPUT::GBufferedInput _bufferedInput,
	GW::INPUT::GController _controllerInput,
	GW::AUDIO::GAudio _audioEngine,
	GW::CORE::GEventGenerator _eventPusher)
{
	// Hello World! (3/5)
	// save a handle to the ECS & game settings
	game = _game;
	gameConfig = _gameConfig;
	immediateInput = _immediateInput;
	bufferedInput = _bufferedInput;
	controllerInput = _controllerInput;
	audioEngine = _audioEngine;
	// Init any helper systems required for this task
	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
	int width = (*readCfg).at("Window").at("width").as<int>();
	speed = (*readCfg).at("Player1").at("speed").as<float>();
	bulletType = (*readCfg).at("Player1").at("bulletType").as<unsigned int>();
	bulletScaleX = (*readCfg).at("Lazers").at("bulletScaleX").as<float>();
	bulletScaleY = (*readCfg).at("Lazers").at("bulletScaleY").as<float>();
	bulletScaleZ = (*readCfg).at("Lazers").at("bulletScaleZ").as<float>();
	bulletScaleW = (*readCfg).at("Lazers").at("bulletScaleW").as<float>();


	// add logic for updating players
	playerSystem = game->system<Player, ModelData, ControllerID, Orientation, Health>("Player System")
		.iter([this](flecs::iter it, Player*, ModelData* t, ControllerID* c, Orientation* o, Health* h) {

		for (auto i : it) {
			// left-right movement
			float xaxis = 0, input = 0, zaxis = 0, input_w = 0, input_s = 0, conInput = 0;
			// Use the controller/keyboard to move the player around the screen			
			if (c[i].index == 0) { // enable keyboard controls for player 1


				immediateInput.GetState(G_KEY_W, input); xaxis += input;
				immediateInput.GetState(G_KEY_S, input); xaxis -= input;


				immediateInput.GetState(G_KEY_A, input_s); zaxis -= input_s;
				immediateInput.GetState(G_KEY_D, input_w); zaxis += input_w;
			}

			//Halan 03/18
				// applying the movement logic to the local matrix of the player 
		
			
				
				controllerInput.GetState(c[i].index, G_LY_AXIS, conInput); xaxis += conInput;
				controllerInput.GetState(c[i].index, G_LX_AXIS, conInput); zaxis += conInput;

			
				

			

			//Halan 03/18
			// applying the movement logic to the local matrix of the player 

			t[i].positionMatrix.row4.y += xaxis * speed;
			t[i].positionMatrix.row4.z += zaxis * speed;


			//Harlan 04/03
			// keeps the player in the bounds of the screen y axis
			t[i].positionMatrix.row4.y = G_LARGER(t[i].positionMatrix.row4.y, 0);
			t[i].positionMatrix.row4.y = G_SMALLER(t[i].positionMatrix.row4.y, 500);


			//Harlan 04/03
			// keeps the player in the bounds of the screen z axis
			//since the level moves the player never needs to leave where the camera can see
			//this will need to vars 
			t[i].positionMatrix.row4.z = G_LARGER(t[i].positionMatrix.row4.z, -4152);
			t[i].positionMatrix.row4.z = G_SMALLER(t[i].positionMatrix.row4.z, -3840);



			// fire weapon if they are in a firing state
			if (it.entity(i).has<Firing>()) {

				FireLasers(it.world(), t[i]);
				it.entity(i).remove<Firing>();

			}

			//Harlan 3/14
			//logic to flip the player 
			if (it.entity(i).has<Flipped>())
			{
				FlipPlayer(t[i]);
				it.entity(i).remove<Flipped>();

			}

			if (it.entity(i).has<SpeedUP>())
			{
				auto tempSpeed = speed;
				speed = 5.0f;
				currtime = 1;
				// timer here 
				if (hasPowerUp == false)
				{
					//std::cout << "--- Speed --- \n" << "Got speeeededed up \n";
					it.entity(i).remove<SpeedUP>();
					speed = tempSpeed;
				}
			}
		}



		// process any cached button events after the loop (happens multiple times per frame)
		ProcessInputEvents(it.world());
			});

	// Create an event cache for when the spacebar/'A' button is pressed
	pressEvents.Create(Max_Frame_Events); // even 32 is probably overkill for one frame

	// register for keyboard and controller events
	bufferedInput.Register(pressEvents);
	controllerInput.Register(pressEvents);

	// create the on explode handler
	onExplode.Create([this](const GW::GEvent& e) {
		USG::PLAY_EVENT event; USG::PLAY_EVENT_DATA eventData;
		if (+e.Read(event, eventData)) {
			// only in here if event matches
			//std::cout << "Enemy Was Destroyed!\n";
		}
		});
	_eventPusher.Register(onExplode);

	return true;
}

// Free any resources used to run this system
bool USG::PlayerLogic::Shutdown()
{
	playerSystem.destruct();
	game.reset();
	gameConfig.reset();

	return true;
}

// Toggle if a system's Logic is actively running
bool USG::PlayerLogic::Activate(bool runSystem)
{
	if (playerSystem.is_alive()) {
		(runSystem) ?
			playerSystem.enable()
			: playerSystem.disable();
		return true;
	}
	return false;
}

bool USG::PlayerLogic::ProcessInputEvents(flecs::world& stage)
{
	// pull any waiting events from the event cache and process them
	GW::GEvent event;
	while (+pressEvents.Pop(event)) {
		bool fire = false;
		flipped = false;
		GController::Events controller;
		GController::EVENT_DATA c_data;
		GBufferedInput::Events keyboard;
		GBufferedInput::EVENT_DATA k_data;
		// these will only happen when needed
		if (+event.Read(keyboard, k_data)) {
			if (keyboard == GBufferedInput::Events::KEYPRESSED) {
				if (k_data.data == G_KEY_SPACE) {
					fire = true;
				}
			}
			if (keyboard == GBufferedInput::Events::KEYPRESSED) {
				if (k_data.data == G_KEY_E) {
					flipped = true;
					currtime = 1;
				}
			}
		}
		if (+event.Read(controller, c_data)) {
			if (controller == GController::Events::CONTROLLERBUTTONVALUECHANGED) {
				if (c_data.inputValue > 0 && c_data.inputCode == G_SOUTH_BTN || c_data.inputValue > 0 && c_data.inputCode == G_RIGHT_SHOULDER_BTN)
					fire = true;

			}

		}

		if (+event.Read(controller, c_data)) {
			if (controller == GController::Events::CONTROLLERBUTTONVALUECHANGED) {
				if (c_data.inputValue > 0 && c_data.inputCode == G_WEST_BTN)
				{
					flipped = true;
					
				}
			}

		}



		if (fire) {
			//Harlan 3/18
			// grab player one and set them to a firing state
			stage.entity("FV_X1_Fighter").add<Firing>();

		}
		if (flipped)
		{
			//Harlan 3/18
			//had to change the name to match the one in playerData
			stage.entity("FV_X1_Fighter").add<Flipped>();

		}

	}
	return true;
}

// play sound and launch two laser rounds
//Harlan 3/14
//added orientation to grab how the player is facing and set velocity as needed 
bool USG::PlayerLogic::FireLasers(flecs::world& stage, USG::ModelData bulletMTX)
{
	// Grab the prefab for a laser round
	flecs::entity bullet;
	//Harlan 3/18 
	// ---note---
	//yes its retreviving the prefab of lazer but we are setting the model with the index number 
	RetreivePrefab("Lazer Bullet", bullet);
	const unsigned int tempBullet = 3;
	GW::MATH::GVECTORF scale = { bulletScaleX,bulletScaleY,bulletScaleZ,bulletScaleW };
	GW::MATH::GMATRIXF temp = bulletMTX.positionMatrix;
	GW::MATH::GMatrix::ScaleLocalF(bulletMTX.positionMatrix, scale, bulletMTX.positionMatrix);
	auto laserLeft = stage.entity().is_a(bullet)
		.set_override<ModelData>({ bulletMTX.positionMatrix, tempBullet })
		.set_override<LightData>({ bulletMTX.positionMatrix,0 });


	GW::AUDIO::GSound shoot = *bullet.get<GW::AUDIO::GSound>();
	shoot.Play();

	return true;
}

//Harlan 3/14
//logic to flip the player 
bool USG::PlayerLogic::FlipPlayer(USG::ModelData& playerMTX)
{
	flipped = false;
	auto temp = playerMTX.positionMatrix;
	//Harlan 03/18
	//updated the flip logic must change the modelData not the orientation
	GW::MATH::GMatrix::RotateXLocalF(temp,
		G_DEGREE_TO_RADIAN_F(180), playerMTX.positionMatrix);
	return true;
}



