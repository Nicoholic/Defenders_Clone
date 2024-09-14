#include "PhysicsLogic.h"
#include "../Components/Physics.h"

bool USG::PhysicsLogic::Init(	std::shared_ptr<flecs::world> _game, 
								std::weak_ptr<const GameConfig> _gameConfig, 
								std::map<unsigned, GW::MATH::GMATRIXF> _collisionMap)
{
	// save a handle to the ECS & game settings
	game = _game;
	gameConfig = _gameConfig;
	collisionMap = _collisionMap;
	// **** MOVEMENT ****
	// update velocity by acceleration
	game->system<Velocity, const Acceleration>("Acceleration System")
		.each([](flecs::entity e, Velocity& v, const Acceleration& a) {
		GW::MATH::GVECTORF accel;
		GW::MATH::GVector::ScaleF(a.value, e.delta_time(), accel);
		GW::MATH::GVector::AddVectorF(accel, v.value, v.value);
	});
	// update position by velocity
	game->system<Orientation, const Velocity>("Translation System")
		.each([](flecs::entity e, Orientation& p, const Velocity& v) {
		GW::MATH::GVECTORF speed;
		GW::MATH::GVector::ScaleF(v.value, e.delta_time(), speed);
		// adding is simple but doesn't account for orientation
		GW::MATH::GVector::AddVectorF(speed, p.value.row3, p.value.row3);
	});
	// **** CLEANUP ****
	// clean up any objects that end up offscreen
	//Harlan 4/2
	//changed to model data was Orientation
	game->system< ModelData>("Cleanup System")
		.each([](flecs::entity e, ModelData& p) {
		if (p.positionMatrix.row4.y > 500 || p.positionMatrix.row4.y < -200 ||
			p.positionMatrix.row4.z > 10000 || p.positionMatrix.row4.z < -5000) {
			e.destruct();
		
		}
	});
	// **** COLLISIONS ****
	// due to wanting to loop through all collidables at once, we do this in two steps:
	// 1. A System will gather all collidables into a shared std::vector
	// 2. A second system will run after, testing/resolving all collidables against each other
	queryCache = game->query<Collidable, ModelData>();
	// only happens once per frame at the very start of the frame
	struct CollisionSystem {}; // local definition so we control iteration count (singular)
	game->entity("Detect-Collisions").add<CollisionSystem>();
	game->system<CollisionSystem>()
		.each([this, _collisionMap](CollisionSystem& s) {
		// This the base shape all objects use & draw, this might normally be a component collider.(ex:sphere/box)
		
		// collect any and all collidable objects
		queryCache.each([this, _collisionMap](flecs::entity e, Collidable& c, ModelData& md) {
			SHAPE temp;
			GW::MATH::GMatrix matrixProxy;
			GW::MATH::GVector vecProxy;
			GW::MATH::GCAPSULEF capsule;
			GW::MATH::GMATRIXF collisionMatrix = collisionMap[md.modelIndex];
			float yPos = collisionMatrix.row4.y;
			collisionMatrix.row4.x = 0, collisionMatrix.row4.y = 0, collisionMatrix.row4.z = 0;
			GW::MATH::GVECTORF capsuleTop = {0,0.5,0}, capsuleBottom = { 0,-0.5,0 };

			matrixProxy.VectorXMatrixF(collisionMatrix, capsuleTop, capsuleTop);// scale from identity to model identity
			matrixProxy.VectorXMatrixF(collisionMatrix, capsuleBottom, capsuleBottom);

			GW::MATH::GMATRIXF resizeMatrix = md.positionMatrix;// use a copy of positionMatrix
			resizeMatrix.row4.x = 0, resizeMatrix.row4.y = 0, resizeMatrix.row4.z = 0;

			matrixProxy.VectorXMatrixF(resizeMatrix, capsuleTop, capsuleTop);// scale to model size
			matrixProxy.VectorXMatrixF(resizeMatrix, capsuleBottom, capsuleBottom);

			GW::MATH::GVECTORF collisionPos = md.positionMatrix.row4;
			collisionPos.y += yPos;

			vecProxy.AddVectorF(collisionPos, capsuleTop, capsuleTop);// translate position
			vecProxy.AddVectorF(collisionPos, capsuleBottom, capsuleBottom);

			capsule.start_x = capsuleTop.x, capsule.start_y = capsuleTop.y, capsule.start_z = capsuleTop.z;
			capsule.end_x = capsuleBottom.x, capsule.end_y = capsuleBottom.y, capsule.end_z = capsuleBottom.z;
			float radius = FLT_MAX;
			for (size_t i = 0; i < 3; i++) {
				for (size_t j = 0; j < 3; j++) {
					if (collisionMatrix.data[(i * 4) + j] != 0 && abs(collisionMatrix.data[(i * 4) + j] < radius)) {
						radius = abs(collisionMatrix.data[(i * 4) + j] * md.positionMatrix.data[(i * 4) + j]);
					}
				}
			}
			capsule.radius = radius/2;

			temp.collisionShape = capsule;
			temp.owner = e;
			testCache.push_back(temp);
		});
		// loop through the testCahe resolving all collisions
		for (int i = 0; i < testCache.size(); ++i) {
			// the inner loop starts at the entity after you so you don't double check collisions
			for (int j = i + 1; j < testCache.size(); ++j) {

				// test the two world space polygons for collision
				// possibly make this cheaper by leaving one of them local and using an inverse matrix
				GW::MATH::GCollision::GCollisionCheck result;
				/*GW::MATH2D::GCollision2D::TestPolygonToPolygon2F(
					testCache[i].poly, polysize, testCache[j].poly, polysize, result);*/
				GW::MATH::GCollision::TestCapsuleToCapsuleF(testCache[i].collisionShape, testCache[j].collisionShape, result);
				if (result == GW::MATH::GCollision::GCollisionCheck::COLLISION) {
					// Create an ECS relationship between the colliders
					// Each system can decide how to respond to this info independently
					testCache[j].owner.add<CollidedWith>(testCache[i].owner);
					testCache[i].owner.add<CollidedWith>(testCache[j].owner);
				}
			}
		}
		// wipe the test cache for the next frame (keeps capacity intact)
		testCache.clear();
	});
	return true;
}

bool USG::PhysicsLogic::Activate(bool runSystem)
{
	if (runSystem) {
		game->entity("Acceleration System").enable();
		game->entity("Translation System").enable();
		game->entity("Cleanup System").enable();
	}
	else {
		game->entity("Acceleration System").disable();
		game->entity("Translation System").disable();
		game->entity("Cleanup System").disable();
	}
	return true;
}

bool USG::PhysicsLogic::Shutdown()
{
	queryCache.destruct(); // fixes crash on shutdown
	game->entity("Acceleration System").destruct();
	game->entity("Translation System").destruct();
	game->entity("Cleanup System").destruct();
	return true;
}
