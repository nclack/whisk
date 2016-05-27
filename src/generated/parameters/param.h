/*
 * Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : June 2010
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#pragma once
#ifndef SHARED_EXPORT
#ifdef _MSC_VER
#define SHARED_EXPORT __declspec(dllexport)
#else
#define SHARED_EXPORT
#endif
#endif
typedef enum _t_enum_SEED_METHOD {SEED_EVERYWHERE, SEED_ON_MHAT_CONTOURS, SEED_ON_GRID} enumSEED_METHOD;
#define MIN_LENPRJ (Params()->paramMIN_LENPRJ)
#define MIN_LENSQR (Params()->paramMIN_LENSQR)
#define MIN_LENGTH (Params()->paramMIN_LENGTH)
#define DUPLICATE_THRESHOLD (Params()->paramDUPLICATE_THRESHOLD)
#define FRAME_DELTA (Params()->paramFRAME_DELTA)
#define HALF_SPACE_TUNNELING_MAX_MOVES (Params()->paramHALF_SPACE_TUNNELING_MAX_MOVES)
#define HALF_SPACE_ASSYMETRY_THRESH (Params()->paramHALF_SPACE_ASSYMETRY_THRESH)
#define MAX_DELTA_OFFSET (Params()->paramMAX_DELTA_OFFSET)
#define MAX_DELTA_WIDTH (Params()->paramMAX_DELTA_WIDTH)
#define MAX_DELTA_ANGLE (Params()->paramMAX_DELTA_ANGLE)
#define MIN_SIGNAL (Params()->paramMIN_SIGNAL)
#define WIDTH_MAX (Params()->paramWIDTH_MAX)
#define WIDTH_MIN (Params()->paramWIDTH_MIN)
#define WIDTH_STEP (Params()->paramWIDTH_STEP)
#define ANGLE_STEP (Params()->paramANGLE_STEP)
#define OFFSET_STEP (Params()->paramOFFSET_STEP)
#define TLEN (Params()->paramTLEN)
#define MIN_SIZE (Params()->paramMIN_SIZE)
#define MIN_LEVEL (Params()->paramMIN_LEVEL)
#define HAT_RADIUS (Params()->paramHAT_RADIUS)
#define SEED_THRESH (Params()->paramSEED_THRESH)
#define SEED_ACCUM_THRESH (Params()->paramSEED_ACCUM_THRESH)
#define SEED_ITERATION_THRESH (Params()->paramSEED_ITERATION_THRESH)
#define SEED_ITERATIONS (Params()->paramSEED_ITERATIONS)
#define SEED_SIZE_PX (Params()->paramSEED_SIZE_PX)
#define SEED_ON_GRID_LATTICE_SPACING (Params()->paramSEED_ON_GRID_LATTICE_SPACING)
#define SEED_METHOD (Params()->paramSEED_METHOD)
#define IDENTITY_SOLVER_SHAPE_NBINS (Params()->paramIDENTITY_SOLVER_SHAPE_NBINS)
#define IDENTITY_SOLVER_VELOCITY_NBINS (Params()->paramIDENTITY_SOLVER_VELOCITY_NBINS)
#define COMPARE_IDENTITIES_DISTS_NBINS (Params()->paramCOMPARE_IDENTITIES_DISTS_NBINS)
#define HMM_RECLASSIFY_BASELINE_LOG2 (Params()->paramHMM_RECLASSIFY_BASELINE_LOG2)
#define HMM_RECLASSIFY_VEL_DISTS_NBINS (Params()->paramHMM_RECLASSIFY_VEL_DISTS_NBINS)
#define HMM_RECLASSIFY_SHP_DISTS_NBINS (Params()->paramHMM_RECLASSIFY_SHP_DISTS_NBINS)
#define SHOW_PROGRESS_MESSAGES (Params()->paramSHOW_PROGRESS_MESSAGES)
#define SHOW_DEBUG_MESSAGES (Params()->paramSHOW_DEBUG_MESSAGES)
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
	float	paramSEED_THRESH;
	float	paramSEED_ACCUM_THRESH;
	float	paramSEED_ITERATION_THRESH;
	int	paramSEED_ITERATIONS;
	int	paramSEED_SIZE_PX;
	int	paramSEED_ON_GRID_LATTICE_SPACING;
	enumSEED_METHOD	paramSEED_METHOD;
	int	paramIDENTITY_SOLVER_SHAPE_NBINS;
	int	paramIDENTITY_SOLVER_VELOCITY_NBINS;
	int	paramCOMPARE_IDENTITIES_DISTS_NBINS;
	float	paramHMM_RECLASSIFY_BASELINE_LOG2;
	int	paramHMM_RECLASSIFY_VEL_DISTS_NBINS;
	int	paramHMM_RECLASSIFY_SHP_DISTS_NBINS;
	char	paramSHOW_PROGRESS_MESSAGES;
	char	paramSHOW_DEBUG_MESSAGES;
} t_params;
SHARED_EXPORT t_params* Params();
SHARED_EXPORT int Load_Params_File(char *filename);
SHARED_EXPORT int Print_Params_File(char *filename);
