// define all ECS components related to identification
#ifndef IDENTIFICATION_H
#define IDENTIFICATION_H

// example space game (avoid name collisions)
namespace USG
{
	struct Player {};
	struct Bullet {};
	struct Enemy {};
	struct PowerUp {};
	struct Lives {};
	struct Flipped {};
	struct ControllerID {
		unsigned index = 0;
	};
};

#endif