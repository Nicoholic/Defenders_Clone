#ifndef MACROS_H
#define MACROS_H
// collection of macros added by DD 3/7/24
#define PI atan(1)*4
#define ROTATE_90 0.5f * PI
#define ROTATE_180 1.0f * PI
#define UP_VEC GW::MATH::GVECTORF{ 0,1,0,0 }

// RENDERER MACROS
#define FOV 65*PI/180
#define NEAR_PLANE 0.1
#define FAR_PLANE 10000
#define REFRESH_RATE 16
#define MAX_MATRIX_SIZE 1000
#define MAX_NUM_LIGHT_SOURCES 20
#define NUM_DESCRIPTOR_POOLS 2
#define DELTA_TIME_ARRAY_SIZE 16383

enum {
	staticStepBinding, dynamicStepBinding, uiStepBinding
};

enum {
	PLAYER_MODEL_ID, BEE_ENEMY_ID, WASP_ENEMY_ID
};
#endif