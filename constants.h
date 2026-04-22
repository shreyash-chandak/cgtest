#pragma once
/* =====================================================================
 *  constants.h — Configurable world constants
 * ===================================================================== */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ── Window (externs live in globals.h / globals.cpp) ── */

/* ── Colosseum arena ── */
static const float COL_R_OUT = 30.0f;
static const float COL_R_IN  = 27.0f;
static const float COL_H     = 10.0f;
static const float COL_TIERS = 3.0f;
static const int   COL_SEG   = 64;

/* ── Tracks ── */
static const float TRK_A_OUT = 22.0f;
static const float TRK_B_OUT = 17.0f;
static const float TRK_A_IN  =  9.0f;
static const float TRK_B_IN  =  6.5f;
static const float ROAD_W    =  3.5f;
static const int   TRK_SEG   = 128;

/* ── Arena floor ── */
static const float ARENA_R   = 26.0f;

/* ── Buildings ── */
static const int   NUM_B     = 5;
static const float B_HALF    = 1.25f;
static const float STORY_H   = 2.5f;

/* ── Torches ── */
static const int   NUM_TORCH  = 16;
static const float TORCH_R    = 25.5f;
static const float TORCH_H    = 0.5f;
static const float FLAME_R    = 0.12f;

/* ── Trees / statue ── */
static const int   NUM_TREE   = 20;

/* ── Car ── */
static const float CAR_L    = 2.4f;
static const float CAR_W    = 1.2f;
static const float CAR_BH   = 0.40f;
static const float CAR_CH   = 0.35f;
static const float WHL_R    = 0.18f;
static const float WHL_W    = 0.12f;
static const float SPD_INC  = 0.5f;
static const float STR_DEG  = 2.5f;
static const float MAX_SPD  = 15.0f;

/* ── Fan / windmill ── */
static const float FAN_R    = 0.65f;
static const float FAN_BW   = 0.13f;
static const float FAN_BT   = 0.03f;
static const float FAN_BSPD = 90.0f;
static const float FAN_SINC = 15.0f;

/* ── Gimbal spotlight ── */
static const float LT_ARM    = 1.2f;
static const float LT_BULB_R = 0.12f;
static const float SW_SPD    = 1.5f;
static const float SW_MAX    = 30.0f;

/* ── Lighting slots ──
   MAX_LIGHTS must match fragment.glsl #define. */
static const int MAX_LIGHTS      = 10;
static const int LIGHT_GIMBAL_0  = 0;  /* slots 0-4: building gimbals      */
static const int LIGHT_HEADLIGHT = 5;  /* slots 5-6: car headlights        */
static const int LIGHT_TORCH_0   = 7;  /* slots 7-9: 3 nearest torches     */
static const int NUM_HEADLIGHTS  = 2;
static const int NUM_TORCH_SLOTS = 3;

/* ── Time-of-day periods ──
   Each period lasts PERIOD_DUR seconds; blend over PERIOD_FADE. */
static const int   NUM_PERIODS  = 4;   /* 0=early morning 1=noon 2=evening 3=night */
static const float PERIOD_DUR   = 30.0f; /* seconds per period                    */
static const float PERIOD_FADE  = 5.0f;  /* cross-fade duration (seconds)         */

/* Headlight toggle key */
static const int HEADLIGHT_KEY  = 0;   /* placeholder — actual key in input.cpp  */

/* ── Camera ── */
static const float CAM_NEAR = 0.1f;
static const float CAM_FAR  = 300.0f;
static const float CAM_FOV  = 60.0f;
static const float GND_SWIV = 30.0f;

/* ── Bonus ── */
static const float BULLET_F = 0.1f;