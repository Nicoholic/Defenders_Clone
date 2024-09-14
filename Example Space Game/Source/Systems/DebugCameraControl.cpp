#include "DebugCameraControl.h"

void USG::DebugCameraControl::Init(GW::INPUT::GInput _input) {
	input = _input;
}
bool USG::DebugCameraControl::GetDebugCamToggle() {
	return debugCamModeEnabled;
}

void USG::DebugCameraControl::GetInput(GW::MATH::GMATRIXF* _viewMatrix, flecs::entity _e, bool* _lineRender, unsigned* _greyscale, unsigned* _invertedColors) {
	float cameraTogglePressed = 0;
	auto keyState = input.GetState(G_KEY_F, cameraTogglePressed);
	if (cameraTogglePressed && keyState != GW::GReturn::REDUNDANT) {// checks key state and if redundant
		debugCamModeEnabled = !debugCamModeEnabled;
	}
	if (debugCamModeEnabled) {
		GW::MATH::GMatrix matrixProxy;
		GW::MATH::GVector vecProxy;
		GW::MATH::GMATRIXF cameraMatrix;
		matrixProxy.InverseF(*_viewMatrix, cameraMatrix);
		// do stuff
		float spacePressed;// move upward
		float shiftPressed;// move downward
		float upPressed;// move forward
		float downPressed; //move backward
		float leftPressed;// move left
		float rightPressed;// move right
		input.GetState(G_KEY_SPACE, spacePressed);
		input.GetState(G_KEY_LEFTSHIFT, shiftPressed);
		input.GetState(G_KEY_UP, upPressed);
		input.GetState(G_KEY_DOWN, downPressed);
		input.GetState(G_KEY_LEFT, leftPressed);
		input.GetState(G_KEY_RIGHT, rightPressed);
		float xMovement = (rightPressed - leftPressed) * CAMERA_SPEED;
		float yMovement = (spacePressed - shiftPressed) * CAMERA_SPEED;
		float zMovement = (upPressed - downPressed) * CAMERA_SPEED;
		GW::MATH::GVECTORF totalMovement;
		totalMovement.x = xMovement, totalMovement.y = 0, totalMovement.z = zMovement, totalMovement.w = 0;
		GW::MATH::GVECTORF verticalMovement;
		verticalMovement.x = 0, verticalMovement.y = yMovement;
		vecProxy.ScaleF(verticalMovement.xy(), _e.delta_time(), verticalMovement);
		vecProxy.ScaleF(totalMovement, _e.delta_time(), totalMovement);
		matrixProxy.TranslateGlobalF(cameraMatrix, verticalMovement.xy(), cameraMatrix);
		matrixProxy.TranslateLocalF(cameraMatrix, totalMovement, cameraMatrix);
		//Mouse Input

		float mouseX = 0;
		float mouseY = 0;

		if (input.GetMouseDelta(mouseX, mouseY) == GW::GReturn::REDUNDANT) {
			mouseY = 0;
			mouseX = 0;
		}
		matrixProxy.RotateXLocalF(cameraMatrix, (mouseY / 1080) * CAM_ROTATE_SPEED * _e.delta_time(), cameraMatrix);
		matrixProxy.RotateYGlobalF(cameraMatrix, (mouseX / 1080) * -CAM_ROTATE_SPEED * _e.delta_time(), cameraMatrix);

		matrixProxy.InverseF(cameraMatrix, *_viewMatrix);
	}
	// toggles for filter effects
	float zToggle;
	float xToggle;
	float cToggle;
	input.GetState(G_KEY_Z, zToggle);
	input.GetState(G_KEY_X, xToggle);
	input.GetState(G_KEY_C, cToggle);
	if (zToggle)
		*_lineRender = !(*_lineRender);
	*_greyscale = ceil(xToggle);
	*_invertedColors = ceil(cToggle);
}
