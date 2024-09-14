// The Enemy system is responsible for enemy behaviors
#ifndef ENEMYLOGIC_H
#define ENEMYLOGIC_H

// Contains our global game settings
#include "../GameConfig.h"
#include "../Entities/EnemyData.h"
#include "LogicHelpers.h"

// example space game (avoid name collisions)
namespace USG
{
	class EnemyLogic
	{
		// shared connection to the main ECS engine
		std::shared_ptr<flecs::world> game;
		// non-ownership handle to configuration settings
		std::weak_ptr<const GameConfig> gameConfig;
		// handle to events
		GW::CORE::GEventGenerator eventPusher;
		GW::SYSTEM::GDaemon iframe;
		//GW::AUDIO::GAudio audioEngine;
		//GW::AUDIO::GMusic currentTrack;
	public:
		// attach the required logic to the ECS 
		bool Init(std::shared_ptr<flecs::world> _game,
			std::weak_ptr<const GameConfig> _gameConfig,
			GW::CORE::GEventGenerator _eventPusher);
		// control if the system is actively running
		bool Activate(bool runSystem);
		

		const char* scoreFilePath = "HighScore.txt";
		int score = 0;
		unsigned int ran = 0;
		int KIA = 0;
		float time = 0;
		//Mac 3-17
		//bool to set up run time for enemy
		bool active = true;
		// release any resources allocated by the system
		bool Shutdown();
		int travel;
		bool death = true;
	};

};

#endif