#pragma once
typedef int bool;
typedef enum _t_enum_SEED_METHOD {SEED_ON_MHAT_CONTOURS, SEED_ON_GRID} enumSEED_METHOD;
#define MIN_LENPRJ (g_param.paramMIN_LENPRJ)
#define MIN_LENSQR (g_param.paramMIN_LENSQR)
#define MIN_LENGTH (g_param.paramMIN_LENGTH)
#define DUPLICATE_THRESHOLD (g_param.paramDUPLICATE_THRESHOLD)
#define FRAME_DELTA (g_param.paramFRAME_DELTA)
#define HALF_SPACE_TUNNELING_MAX_MOVES (g_param.paramHALF_SPACE_TUNNELING_MAX_MOVES)
#define HALF_SPACE_ASSYMETRY_THRESH (g_param.paramHALF_SPACE_ASSYMETRY_THRESH)
#define MAX_DELTA_OFFSET (g_param.paramMAX_DELTA_OFFSET)
#define MAX_DELTA_WIDTH (g_param.paramMAX_DELTA_WIDTH)
#define MAX_DELTA_ANGLE (g_param.paramMAX_DELTA_ANGLE)
#define MIN_SIGNAL (g_param.paramMIN_SIGNAL)
#define WIDTH_MAX (g_param.paramWIDTH_MAX)
#define WIDTH_MIN (g_param.paramWIDTH_MIN)
#define WIDTH_STEP (g_param.paramWIDTH_STEP)
#define ANGLE_STEP (g_param.paramANGLE_STEP)
#define OFFSET_STEP (g_param.paramOFFSET_STEP)
#define TLEN (g_param.paramTLEN)
#define MIN_SIZE (g_param.paramMIN_SIZE)
#define MIN_LEVEL (g_param.paramMIN_LEVEL)
#define HAT_RADIUS (g_param.paramHAT_RADIUS)
#define SEED_ACCUM_THRESH (g_param.paramSEED_ACCUM_THRESH)
#define SEED_ITERATION_THRESH (g_param.paramSEED_ITERATION_THRESH)
#define SEED_ITERATIONS (g_param.paramSEED_ITERATIONS)
#define SEED_ON_GRID_LATTICE_SPACING (g_param.paramSEED_ON_GRID_LATTICE_SPACING)
#define SEED_METHOD (g_param.paramSEED_METHOD)
#define IDENTITY_SOLVER_SHAPE_NBINS (g_param.paramIDENTITY_SOLVER_SHAPE_NBINS)
#define IDENTITY_SOLVER_VELOCITY_NBINS (g_param.paramIDENTITY_SOLVER_VELOCITY_NBINS)
#define COMPARE_IDENTITIES_DISTS_NBINS (g_param.paramCOMPARE_IDENTITIES_DISTS_NBINS)
#define HMM_RECLASSIFY_BASELINE_LOG2 (g_param.paramHMM_RECLASSIFY_BASELINE_LOG2)
#define HMM_RECLASSIFY_VEL_DISTS_NBINS (g_param.paramHMM_RECLASSIFY_VEL_DISTS_NBINS)
#define HMM_RECLASSIFY_SHP_DISTS_NBINS (g_param.paramHMM_RECLASSIFY_SHP_DISTS_NBINS)
#define SHOW_PROGRESS_MESSAGES (g_param.paramSHOW_PROGRESS_MESSAGES)
#define SHOW_DEBUG_MESSAGES (g_param.paramSHOW_DEBUG_MESSAGES)
typedef struct _t_params {
	int	paramMIN_LENPRJ;
	int	paramMIN_LENSQR;
	int	paramMIN_LENGTH;
	float	paramDUPLICATE_THRESHOLD;
	int	paramFRAME_DELTA;
	int	paramHALF_SPACE_TUNNELING_MAX_MOVES;
	float	paramHALF_SPACE_ASSYMETRY_THRESH;
	float	paramMAX_DELTA_OFFSET;
	float	paramMAX_DELTA_WIDTH;
	float	paramMAX_DELTA_ANGLE;
	float	paramMIN_SIGNAL;
	float	paramWIDTH_MAX;
	float	paramWIDTH_MIN;
	float	paramWIDTH_STEP;
	float	paramANGLE_STEP;
	float	paramOFFSET_STEP;
	int	paramTLEN;
	int	paramMIN_SIZE;
	int	paramMIN_LEVEL;
	float	paramHAT_RADIUS;
	float	paramSEED_ACCUM_THRESH;
	float	paramSEED_ITERATION_THRESH;
	int	paramSEED_ITERATIONS;
	int	paramSEED_ON_GRID_LATTICE_SPACING;
	enumSEED_METHOD	paramSEED_METHOD;
	int	paramIDENTITY_SOLVER_SHAPE_NBINS;
	int	paramIDENTITY_SOLVER_VELOCITY_NBINS;
	int	paramCOMPARE_IDENTITIES_DISTS_NBINS;
	float	paramHMM_RECLASSIFY_BASELINE_LOG2;
	int	paramHMM_RECLASSIFY_VEL_DISTS_NBINS;
	int	paramHMM_RECLASSIFY_SHP_DISTS_NBINS;
	bool	paramSHOW_PROGRESS_MESSAGES;
	bool	paramSHOW_DEBUG_MESSAGES;
} t_params;
t_params g_param;
int Load_Params_File(char *filename);
