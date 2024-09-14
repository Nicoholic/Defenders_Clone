// The rendering system is responsible for drawing all objects
#ifndef DEBUGCAMERACONTROL
#define DEBUGCAMERACONTROL

#define CAMERA_SPEED 700
#define CAM_ROTATE_SPEED 50.0f

// Contains our global game settings

// example space game (avoid name collisions)
namespace USG
{
	// created by DD 3/14/24
	class DebugCameraControl
	{
		GW::INPUT::GInput input;
		bool debugCamModeEnabled = false;
	public:
		void Init(GW::INPUT::GInput _input);
		bool GetDebugCamToggle();
		void GetInput(GW::MATH::GMATRIXF* _viewMatrix, flecs::entity _e, bool* _lineRender, unsigned* _greyscale, unsigned* _invertedColors);
	};
};

#endif


