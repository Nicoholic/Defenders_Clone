#include "Application.h"
#include "Components/Visuals.h"
#include "Components/Gameplay.h"
// open some Gateware namespaces for conveinence 
// NEVER do this in a header file!
using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;
using namespace USG;

bool Application::Init()
{
	eventPusher.Create();

	// load all game settigns
	gameConfig = std::make_shared<GameConfig>();
	// create the ECS system
	game = std::make_shared<flecs::world>();
	// init all other systems
	if (InitWindow() == false)
		return false;
	if (InitInput() == false)
		return false;
	if (InitAudio() == false)
		return false;
	if (InitModels() == false)// added 3/7/24 by DD
		return false;
	if (InitLighting() == false)// added 3/24/24 by DD
		return false;
	if (InitGraphics() == false)
		return false;
	if (InitEntities() == false)
		return false;
	if (InitSystems() == false)
		return false;

	resetGame = game->system<LevelReset>().kind(flecs::PostUpdate)
		.each([this](flecs::entity e, LevelReset& l) {
		//e.set<ChangeLevel>({ currLevelCount });
		e.add<ModelData>();
		players.Load(game, gameConfig, dynamicLevelData, levelLoading.GetPlayerPos());
		std::cout << "hit p to play" << std::endl;
		//enemySystem.death = false;
		e.set<Health>({ 3 });
		Pause();
		e.add<Collidable>();
		
		e.remove<LevelReset>();
			});
	return true;
}

bool Application::Run()
{
	VkClearValue clrAndDepth[2];
	clrAndDepth[0].color = { {0, 0, 0, 1} };
	clrAndDepth[1].depthStencil = { 1.0f, 0u };
	// grab vsync selection
	bool vsync = gameConfig->at("Window").at("vsync").as<bool>();
	// set background color from settings
	const char* channels[] = { "red", "green", "blue" };
	for (int i = 0; i < std::size(channels); ++i) {
		clrAndDepth[0].color.float32[i] =
			gameConfig->at("BackGroundColor").at(channels[i]).as<float>();
	}
	// create an event handler to see if the window was closed early
	bool winClosed = false;
	GW::CORE::GEventResponder winHandler;
	winHandler.Create([&winClosed](GW::GEvent e) {
		GW::SYSTEM::GWindow::Events ev;
		if (+e.Read(ev) && ev == GW::SYSTEM::GWindow::Events::DESTROY)
			winClosed = true;
		});
	window.Register(winHandler);
	while (+window.ProcessWindowEvents())
	{
		if (winClosed == true)
			return true;
		if (+vulkan.StartFrame(2, clrAndDepth))
		{
			if (GameLoop() == false) {
				vulkan.EndFrame(vsync);
				return false;
			}
			if (-vulkan.EndFrame(vsync)) {
				// failing EndFrame is not always a critical error, see the GW docs for specifics
			}
		}
		else
			return false;
	}
	return true;
}

bool Application::Shutdown()
{
	// disconnect systems from global ECS
	if (playerSystem.Shutdown() == false)
		return false;
	if (levelSystem.Shutdown() == false)
		return false;
	if (vkRenderingSystem.Shutdown() == false)
		return false;
	if (physicsSystem.Shutdown() == false)
		return false;
	if (bulletSystem.Shutdown() == false)
		return false;
	if (enemySystem.Shutdown() == false)
		return false;

	return true;
}

bool Application::InitWindow()
{
	// grab settings
	int width = gameConfig->at("Window").at("width").as<int>();
	int height = gameConfig->at("Window").at("height").as<int>();
	int xstart = gameConfig->at("Window").at("xstart").as<int>();
	int ystart = gameConfig->at("Window").at("ystart").as<int>();
	std::string title = gameConfig->at("Window").at("title").as<std::string>();
	// open window
	if (+window.Create(xstart, ystart, width, height, GWindowStyle::WINDOWEDLOCKED) &&
		+window.SetWindowName(title.c_str())) {
		return true;
	}
	return false;
}

bool Application::InitInput()
{
	if (-gamePads.Create())
		return false;
	if (-immediateInput.Create(window))
		return false;
	if (-bufferedInput.Create(window))
		return false;
	return true;
}

bool Application::InitAudio()
{
	if (-audioEngine.Create())
		return false;
	return true;
}

/// <summary>
/// Initialized level data imported from a file
/// DD 1/11/2024
/// </summary>
/// <returns>if model intialization fails</returns>
bool Application::InitModels() {
	std::string gameLevelPath = LEVEL_LOCATION;
	std::string h2bFolderPath = H2B_FOLDER_LOCATION;
	currLevelCount = 1;// automatically start at level1
	toggleLineRenderer = game->system<ToggleLineRender>().kind(flecs::PreUpdate)
		.each([this](flecs::entity e, ToggleLineRender& tl) {

		lineRenderFilter = !lineRenderFilter;
		e.add<ChangeLevel>();
		e.set<ChangeLevel>({ currLevelCount });
		e.remove<ToggleLineRender>();
			});
	if (levelLoading.LoadLevel(gameLevelPath + std::to_string(currLevelCount) + ".txt", h2bFolderPath)) {
		// load in models that need to move
		if (dynamicLevelData.LoadLevel(DYNAMIC_MODEL_LOCATION, h2bFolderPath)) {// added by DD 1/27/24
			loadLevel = game->system<ChangeLevel>().kind(flecs::PreUpdate)
				.each([this](flecs::entity e, ChangeLevel& l) {
				levelLoading.LoadLevel(LEVEL_LOCATION + std::to_string(l.levelNumber) + ".txt", H2B_FOLDER_LOCATION);
				vkRenderingSystem.DeleteRendererMemory();
				vkRenderingSystem.Init(game, gameConfig, vulkan, window, levelLoading, dynamicLevelData, lightDataLoader.lightData, immediateInput, lineRenderFilter);
				currLevelCount = e.get<ChangeLevel>()->levelNumber;
				e.remove<ChangeLevel>();
					});
			return true;
		}
	}
	return false;
}

bool Application::InitLighting() {
	if (lightDataLoader.LoadLights(LIGHT_DATA_LOCATION))
		return true;
	return false;
}

bool Application::InitGraphics()
{
#ifndef NDEBUG
	const char* debugLayers[] = {
		"VK_LAYER_KHRONOS_validation", // standard validation layer
		//"VK_LAYER_RENDERDOC_Capture" // add this if you have installed RenderDoc
	};
	if (+vulkan.Create(window, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT,
		sizeof(debugLayers) / sizeof(debugLayers[0]),
		debugLayers, 0, nullptr, 0, nullptr, false))
		return true;
#else
	if (+vulkan.Create(window, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
		return true;
#endif
	return false;
}

bool Application::InitEntities()
{
	// Load bullet prefabs
	if (weapons.Load(game, gameConfig, audioEngine) == false)
		return false;
	// Load the player entities
	if (players.Load(game, gameConfig, dynamicLevelData, levelLoading.GetPlayerPos()) == false)
		return false;
	// Load the enemy entities
	//Nathan 3/21
	//Updated load
	if (enemies.Load(game, gameConfig, audioEngine, dynamicLevelData,levelLoading.cameraMatrix) == false)
		return false;

	return true;
}

bool Application::InitSystems()
{
	// connect systems to global ECS
	if (playerSystem.Init(game, gameConfig, immediateInput, bufferedInput,
		gamePads, audioEngine, eventPusher) == false)
		return false;
	if (levelSystem.Init(game, gameConfig, audioEngine) == false)
		return false;
	if (vkRenderingSystem.Init(game, gameConfig, vulkan, window, levelLoading, dynamicLevelData, lightDataLoader.lightData, immediateInput, lineRenderFilter) == false)
		return false;
	logicHelpers.Init(game, &vkRenderingSystem);
	if (physicsSystem.Init(game, gameConfig, dynamicLevelData.collisionMap) == false)
		return false;
	if (bulletSystem.Init(game, gameConfig, logicHelpers) == false)
		return false;
	if (enemySystem.Init(game, gameConfig, eventPusher) == false)
		return false;

	return true;
}

bool Application::GameLoop()
{
	// compute delta time and pass to the ECS system
	static auto start = std::chrono::steady_clock::now();
	double elapsed = std::chrono::duration<double>(
		std::chrono::steady_clock::now() - start).count();
	start = std::chrono::steady_clock::now();
	// let the ECS system run
	//3-17 Pause Check! Mac
	Application::Pausing();
	//timer ticks every 5 seconds
	Application::SetTimer(6, elapsed);
	Application::SetTimer(121, elapsed);
	Application::SetTimer(10, elapsed);
	return game->progress(static_cast<float>(elapsed));
}

//3-17 Mac
void Application::Pause()
{
	if (Application::IsPaused() == true)
	{
		playerSystem.active = true;
		bulletSystem.active = true;
		physicsSystem.active = true;
		enemySystem.active = true;
		levelSystem.active = true;
		logicHelpers.active = true;

		playerSystem.Activate(playerSystem.active);
		bulletSystem.Activate(bulletSystem.active);
		physicsSystem.Activate(physicsSystem.active);
		enemySystem.Activate(enemySystem.active);
		levelSystem.Activate(levelSystem.active);
		logicHelpers.Activate(logicHelpers.active);
	}
	else
	{
		playerSystem.active = false;
		bulletSystem.active = false;
		physicsSystem.active = false;
		enemySystem.active = false;
		levelSystem.active = false;
		logicHelpers.active = false;

		playerSystem.Activate(playerSystem.active);
		bulletSystem.Activate(bulletSystem.active);
		physicsSystem.Activate(physicsSystem.active);
		enemySystem.Activate(enemySystem.active);
		levelSystem.Activate(levelSystem.active);
		logicHelpers.Activate(logicHelpers.active);
	}
}

//3-21 Mac Pause finally does pause things properly
void Application::Pausing()
{
	if (startGame == false)
	{
		playerSystem.active = false;
		bulletSystem.active = false;
		physicsSystem.active = false;
		enemySystem.active = false;
		levelSystem.active = false;
		logicHelpers.active = false;

		playerSystem.Activate(playerSystem.active);
		bulletSystem.Activate(bulletSystem.active);
		physicsSystem.Activate(physicsSystem.active);
		enemySystem.Activate(enemySystem.active);
		levelSystem.Activate(levelSystem.active);
		logicHelpers.Activate(logicHelpers.active);
	}
	// pull any waiting events from the event cache and process them
	GW::GEvent event;
	while (+pressEvents.Pop(event)) {
		GW::INPUT::GController::Events controller;
		GW::INPUT::GController::EVENT_DATA c_data;
		GW::INPUT::GBufferedInput::Events keyboard;
		GW::INPUT::GBufferedInput::EVENT_DATA k_data;
		// these will only happen when needed
		if (+event.Read(keyboard, k_data)) {
			if (keyboard == GW::INPUT::GBufferedInput::Events::KEYPRESSED) {
				if (k_data.data == G_KEY_P && startGame == true) {
					Application::Pause();
				}
				if (k_data.data == G_KEY_ENTER && startGame == false) {
					startGame = true;
					levelSystem.runMusic = true;
					Application::Pause();
				}
			/*	if (k_data.data == G_KEY_ESCAPE)
				{
					flecs::entity e;
					e.world().quit();
					Application::Shutdown();
				}*/
			}
		}

		else if (+event.Read(controller, c_data)) {
			if (controller == GW::INPUT::GController::Events::CONTROLLERBUTTONVALUECHANGED) {
				if (c_data.inputValue > 0 && c_data.inputCode == G_START_BTN)
				{
					startGame = true;
					levelSystem.runMusic = true;
					Application::Pause();
				}
			}
		}
	}
	// Create an event cache for when the spacebar/'A' button is pressed
	pressEvents.Create(32); // even 32 is probably overkill for one frame

	// register for keyboard and controller events
	bufferedInput.Register(pressEvents);
	gamePads.Register(pressEvents);

}

//3-18 Mac
bool Application::IsPaused() const
{
	if (playerSystem.active == false )
	{
		//std::cout << "game is paused";
		return true;
	}
	else if (playerSystem.active == true )
	{
		//std::cout << "game isn't paused";
		return false;
	}
	else
	{
		std::cout << "something is going wrong please fix! :)";
		return false;
	}
}

//3-21 Mac
float Application::Timer(double elapsed) const
{
	if (IsPaused() == true)
	{
		return time;
	}
	else
	{
		return time + elapsed;
	}
}

//3-28 full fix for set timer
void Application::SetTimer(int timeSet, double elapsed)
{
	time = Application::Timer(elapsed);
	int timeCycle = static_cast<int>(time);

	//checks what time it is and then sees if it is ready to go! :)

	switch (timeSet) {
		//case # -> the seconds that pass before being called!
	case 6:
		//make sure to add a c value set to 1 with this loop before using!
		if (c1 * timeSet < time)
		{
			c1++;
			//std::cout << "from application \n" << static_cast<int>(time)/2 << " seconds" << "\n";
		}
		break;

	case 10:
		//currtime is set to one in playerlogic so that it can be read
		if (playerSystem.currtime == 1)
		{
			//once set to the time + 10 it will wait until second if can be true
			playerSystem.currtime = time + timeSet;
			playerSystem.hasPowerUp = true;
		}
		//once true will turn off the value
		if (playerSystem.currtime < time && playerSystem.currtime != 0) 
		{
			playerSystem.hasPowerUp = false;
			playerSystem.currtime = 0; 
		}
		break;
	case 500:
		if (c2 * timeSet < time)
		{
			levelSystem.runMusic = true;
			c2++;
		}
		break;
	}

}
