#include "PlayerData.h"
#include "../Components/Identification.h"
#include "../Components/Visuals.h"
#include "../Components/Physics.h"
#include "Prefabs.h"
#include "../Components/Gameplay.h"



bool USG::PlayerData::Load(std::shared_ptr<flecs::world> _game,
	std::weak_ptr<const GameConfig> _gameConfig, LEVEL_DATA levelData, GW::MATH::GMATRIXF playerPOS)
{
	// Grab init settings for players
	std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();
	//GW::MATH::GVECTORF scale = (*readCfg).at("Player1").at("scale").as<GW::MATH::GVECTORF>();

	//Harlan 3 / 12
	//added to add rotation in Degrees in defaults.ini
	float angle = (*readCfg).at("Player1").at("angle").as<float>();
	float scaleX = (*readCfg).at("Player1").at("scaleX").as<float>();
	float scaleY = (*readCfg).at("Player1").at("scaleY").as<float>();
	float scaleZ = (*readCfg).at("Player1").at("scaleZ").as<float>();
	float scaleW = (*readCfg).at("Player1").at("scaleW").as<float>();

	
	unsigned int indexNumber = (*readCfg).at("Player1").at("indexNumber").as<unsigned int>();
	const unsigned int tempIndex = indexNumber;
	//Harlan 
	//putting scaling for the player in a vector to call in the operation below
	GW::MATH::GVECTORF scale = { scaleX,scaleY,scaleZ,scaleW };
	GW::MATH::GMatrix::ScaleLocalF(playerPOS, scale, playerPOS);
	//Harlan 
	//Rotating the mtx of the player 
	//x is a var so that we can change it for flipping the player 
	//z is hard coded to prevent change 
	GW::MATH::GMatrix::RotateXLocalF(playerPOS, G_DEGREE_TO_RADIAN_F(angle), playerPOS);
	GW::MATH::GMatrix::RotateZLocalF(playerPOS, G_DEGREE_TO_RADIAN_F(90), playerPOS);
	//can scale if we want to here is the code for it change the vector to whatever you like 
	
		// Create Player One
		_game->entity("FV_X1_Fighter")
		.set([&](Position& p, Orientation& o, Material& m, ControllerID& c, ModelData& t, Health& h, Damage& d) {
		c = { 0 };
		//Harlan 3/12
		o = {playerPOS};
		d = { 1 };
		//harlan 03/17
		// index 0 is player 
		t = { o.value, tempIndex };
			})
			.set_override<Health>({3})
			.add<Collidable>()
			.add<Player>(); // Tag this entity as a Player


			return true;
}

bool USG::PlayerData::Unload(std::shared_ptr<flecs::world> _game)
{
	// remove all players
	_game->defer_begin(); // required when removing while iterating!
	_game->each([](flecs::entity e, Player&) {
		e.destruct(); // destroy this entitiy (happens at frame end)
		});
	_game->defer_end(); // required when removing while iterating!

	return true;
}
