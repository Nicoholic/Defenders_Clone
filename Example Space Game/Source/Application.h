#ifndef APPLICATION_H
#define APPLICATION_H

// include events
#include "Events/Playevents.h"
// Contains our global game settings
#include "GameConfig.h"
// Load all entities+prefabs used by the game 
#include "Entities/BulletData.h"
#include "Entities/PlayerData.h"
#include "Entities/EnemyData.h"
// Include all systems used by the game and their associated components
#include "Systems/PlayerLogic.h"
#include "Systems/VulkanRendererLogic.h"
#include "Systems/LevelLogic.h"
#include "Systems/PhysicsLogic.h"
#include "Systems/BulletLogic.h"
#include "Systems/EnemyLogic.h"
#include "Systems/LogicHelpers.h"
#include "../Level-Import/LevelDataLoad.h"
#include "../Level-Import/LightDataLoad.h"
#include "../Level-Import/LevelImportMacros.h"

// Allocates and runs all sub-systems essential to operating the game
class Application 
{
	// gateware libs used to access operating system
	GW::SYSTEM::GWindow window; // gateware multi-platform window
	GW::GRAPHICS::GVulkanSurface vulkan; // gateware vulkan API wrapper
	GW::INPUT::GController gamePads; // controller support
	GW::INPUT::GInput immediateInput; // twitch keybaord/mouse
	GW::INPUT::GBufferedInput bufferedInput; // event keyboard/mouse

	GW::AUDIO::GAudio audioEngine; // can create music & sound effects
	// third-party gameplay & utility libraries
	std::shared_ptr<flecs::world> game; // ECS database for gameplay
	std::shared_ptr<GameConfig> gameConfig; // .ini file game settings
	// ECS Entities and Prefabs that need to be loaded
	USG::BulletData weapons;
	USG::PlayerData players;
	USG::EnemyData enemies;
	// specific ECS systems used to run the game
	USG::PlayerLogic playerSystem;
	USG::VulkanRendererLogic vkRenderingSystem;
	USG::LevelLogic levelSystem;
	USG::PhysicsLogic physicsSystem;
	USG::BulletLogic bulletSystem;
	USG::EnemyLogic enemySystem;
	USG::LogicHelpers logicHelpers;
	// EventGenerator for Game Events
	GW::CORE::GEventGenerator eventPusher;
	// key press event cache (saves input events)
	// we choose cache over responder here for better ECS compatibility
	GW::CORE::GEventCache pressEvents;
	// Level Data to be rendered
	LEVEL_DATA levelLoading;
	LEVEL_DATA dynamicLevelData;// added 3/7/24 by DD
	H2B::LightDataLoad lightDataLoader;

	unsigned currLevelCount;
	flecs::system loadLevel;
	flecs::system toggleLineRenderer;
	flecs::system resetGame;
	bool lineRenderFilter = false;

public:
	bool Init();
	bool Run();
	bool Shutdown();
	//setting up pause functionality Mac :)
	void Pause();
	void Pausing();
	bool IsPaused() const;
	bool startGame = false;
	//3-21 Mac timer
	float Timer(double) const;
	double time = 0;
	//3-28
	//add your casenumber and value here!
	int c1 = 1,c2 = 1, c3 =1;
	void SetTimer(int, double);
private:
	bool InitWindow();
	bool InitInput();
	bool InitAudio();
	bool InitModels();// added by DD 3/7/24
	bool InitLighting();
	bool InitGraphics();
	bool InitEntities();
	bool InitSystems();
	bool GameLoop();
};

#endif 