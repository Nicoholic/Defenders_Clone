// define all ECS components related to drawing
#ifndef VISUALS_H
#define VISUALS_H

// example space game (avoid name collisions)
namespace USG
{
	struct ToggleLineRender{};
	struct ChangeLevel { unsigned levelNumber; };// added 3/9/24 by DD
	struct LightData {// added 3/24/24 by DD
		GW::MATH::GMATRIXF lightPos;
		unsigned lightID;
	};
	struct Color { GW::MATH2D::GVECTOR3F value; };

	struct Material {
		Color diffuse = { 1, 1, 1 };
	};
};

#endif