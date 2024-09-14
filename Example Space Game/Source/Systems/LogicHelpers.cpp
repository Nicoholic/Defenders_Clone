#include "LogicHelpers.h"
#include "../Components/Identification.h"
#include "../../gateware-main/Gateware.h"
#define deltaTime static_cast<float>(e.delta_time())

void USG::LogicHelpers::Init(std::shared_ptr<flecs::world> _game, VulkanRendererLogic* _vkRenderLogic) {
	vkRenderLogic = _vkRenderLogic;
	game = _game;
	helperSystem = game->system<Flipped>()
		.each([this](flecs::entity e, Flipped& f) {

		moveSpeed = -moveSpeed;

			});

	struct UpdateMovement {};


	game->entity("Level Movement").add<UpdateMovement>();
	if (game->entity("Level Movement").has<UpdateMovement>()) {
		std::cout << "Level Helpers: got it" << std::endl;
	}

	
	updatePos = game->system<UpdateMovement>().kind(flecs::OnUpdate)
		.each([this](flecs::entity e, UpdateMovement& um) {
		for (size_t i = 0; i < vkRenderLogic->levelDataWorldMats.size(); i++) {
			GW::MATH::GMatrix::TranslateLocalF(vkRenderLogic->levelDataWorldMats[i], GW::MATH::GVECTORF{0,0,-100 * moveSpeed * deltaTime ,1000 * moveSpeed * deltaTime }, vkRenderLogic->levelDataWorldMats[i]);
		}
			});

}

GW::MATH::GVECTORF  USG::LogicHelpers::FindScreenBounds() {
	GW::MATH::GVECTORF screenBounds = vkRenderLogic->GetCameraPos();
	return GW::MATH::GVECTORF{ screenBounds.z - SCREEN_BOUND_DISTANCE_X, screenBounds.z + SCREEN_BOUND_DISTANCE_X,
		screenBounds.y + SCREEN_BOUND_DISTANCE_Y, screenBounds.y - SCREEN_BOUND_DISTANCE_Y };
}

//active function for background
bool USG::LogicHelpers::Activate(bool runSystem)
{
	if (runSystem) {
		game->entity("Level Movement").enable();
	}
	else {
		game->entity("Level Movement").disable();
	}
	return false;
}