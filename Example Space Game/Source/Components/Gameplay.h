// define all ECS components related to gameplay
#ifndef GAMEPLAY_H
#define GAMEPLAY_H

// example space game (avoid name collisions)
namespace USG
{
	struct Damage { int value; };
	struct Health { int value; };
	struct Firerate { float value; };
	struct Cooldown { float value; };
	struct RandPowerUP { int value; };
	
	


	// gameplay tags (states)
	struct Firing {};
	struct Testing {};
	struct Charging {};
	struct SpeedUP {};
	struct SmallShip {};

	

	// powerups
	struct ChargedShot { int max_destroy; };
};

#endif