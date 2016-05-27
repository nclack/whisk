%{
/*
 * Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : June 2010
 * 
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "param.h"

#define ASRT(e) do {size_t v = (size_t)(e); assert(v); } while(0)

#define YYPRINT
#if 0
#define DEBUG_LEXER
#define DEBUG_GRAMMAR
#endif

#ifdef DEBUG_LEXER
  #define debug_lexer(...) printf(__VA_ARGS__)
#else
  #define debug_lexer(...)
#endif

#ifdef DEBUG_GRAMMAR
  #define debug_grammar(...) printf(__VA_ARGS__)
#else
  #define debug_grammar(...)
#endif

int  yylex  (void);
void yyerror(char const *);

int g_nparse_errors = 0;
#define MAX_PARSE_ERRORS (10)
#define PARSER_INCERR \
if(++g_nparse_errors>MAX_PARSE_ERRORS)\
{ fprintf(stderr,"Too many problems parsing parameter file. Aborting.\n");\
  YYABORT;\
}
t_params g_param;
enum eParamTrackingIndexes {
	indexMIN_LENPRJ,
	indexMIN_LENSQR,
	indexMIN_LENGTH,
	indexDUPLICATE_THRESHOLD,
	indexFRAME_DELTA,
	indexHALF_SPACE_TUNNELING_MAX_MOVES,
	indexHALF_SPACE_ASSYMETRY_THRESH,
	indexMAX_DELTA_OFFSET,
	indexMAX_DELTA_WIDTH,
	indexMAX_DELTA_ANGLE,
	indexMIN_SIGNAL,
	indexWIDTH_MAX,
	indexWIDTH_MIN,
	indexWIDTH_STEP,
	indexANGLE_STEP,
	indexOFFSET_STEP,
	indexTLEN,
	indexMIN_SIZE,
	indexMIN_LEVEL,
	indexHAT_RADIUS,
	indexSEED_THRESH,
	indexSEED_ACCUM_THRESH,
	indexSEED_ITERATION_THRESH,
	indexSEED_ITERATIONS,
	indexSEED_SIZE_PX,
	indexSEED_ON_GRID_LATTICE_SPACING,
	indexSEED_METHOD,
	indexIDENTITY_SOLVER_SHAPE_NBINS,
	indexIDENTITY_SOLVER_VELOCITY_NBINS,
	indexCOMPARE_IDENTITIES_DISTS_NBINS,
	indexHMM_RECLASSIFY_BASELINE_LOG2,
	indexHMM_RECLASSIFY_VEL_DISTS_NBINS,
	indexHMM_RECLASSIFY_SHP_DISTS_NBINS,
	indexSHOW_PROGRESS_MESSAGES,
	indexSHOW_DEBUG_MESSAGES,
	indexMAX
};
int g_found_parameters[indexMAX];
char *g_param_string_table[] = {
	"MIN_LENPRJ",
	"MIN_LENSQR",
	"MIN_LENGTH",
	"DUPLICATE_THRESHOLD",
	"FRAME_DELTA",
	"HALF_SPACE_TUNNELING_MAX_MOVES",
	"HALF_SPACE_ASSYMETRY_THRESH",
	"MAX_DELTA_OFFSET",
	"MAX_DELTA_WIDTH",
	"MAX_DELTA_ANGLE",
	"MIN_SIGNAL",
	"WIDTH_MAX",
	"WIDTH_MIN",
	"WIDTH_STEP",
	"ANGLE_STEP",
	"OFFSET_STEP",
	"TLEN",
	"MIN_SIZE",
	"MIN_LEVEL",
	"HAT_RADIUS",
	"SEED_THRESH",
	"SEED_ACCUM_THRESH",
	"SEED_ITERATION_THRESH",
	"SEED_ITERATIONS",
	"SEED_SIZE_PX",
	"SEED_ON_GRID_LATTICE_SPACING",
	"SEED_METHOD",
	"IDENTITY_SOLVER_SHAPE_NBINS",
	"IDENTITY_SOLVER_VELOCITY_NBINS",
	"COMPARE_IDENTITIES_DISTS_NBINS",
	"HMM_RECLASSIFY_BASELINE_LOG2",
	"HMM_RECLASSIFY_VEL_DISTS_NBINS",
	"HMM_RECLASSIFY_SHP_DISTS_NBINS",
	"SHOW_PROGRESS_MESSAGES",
	"SHOW_DEBUG_MESSAGES"};
%}
%token-table
%union{ int integral;
        float decimal;
        enumSEED_METHOD valSEED_METHOD;
       }
//Normal tokens
%token            COMMENT
%token <integral> INTEGRAL
%token <decimal>  DECIMAL
%type  <decimal>  decimal
//Enum value tokens
%token <valSEED_METHOD> TOK_SEED_EVERYWHERE "SEED_EVERYWHERE"
%token <valSEED_METHOD> TOK_SEED_ON_MHAT_CONTOURS "SEED_ON_MHAT_CONTOURS"
%token <valSEED_METHOD> TOK_SEED_ON_GRID "SEED_ON_GRID"
%%
input:   /*empty*/                   { debug_grammar("EMPTY INPUT\n"); }
       | input line                  { debug_grammar("LINE\n"); }
       ;
comment: /*empty*/                   { debug_grammar("\t\tCOMMENT EMPTY\n"); }
       | COMMENT                     { debug_grammar("\t\tCOMMENT TAIL\n"); }
       ;
line:   '\n'                        { debug_grammar("\tEMPTY LINE\n"); }
      | "MIN_LENPRJ" INTEGRAL comment '\n'  {g_param.paramMIN_LENPRJ=$<integral>2;g_found_parameters[indexMIN_LENPRJ]=1; debug_grammar("\tMIN_LENPRJ\n");}
      | "MIN_LENPRJ" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for MIN_LENPRJ.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "MIN_LENSQR" INTEGRAL comment '\n'  {g_param.paramMIN_LENSQR=$<integral>2;g_found_parameters[indexMIN_LENSQR]=1; debug_grammar("\tMIN_LENSQR\n");}
      | "MIN_LENSQR" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for MIN_LENSQR.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "MIN_LENGTH" INTEGRAL comment '\n'  {g_param.paramMIN_LENGTH=$<integral>2;g_found_parameters[indexMIN_LENGTH]=1; debug_grammar("\tMIN_LENGTH\n");}
      | "MIN_LENGTH" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for MIN_LENGTH.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "DUPLICATE_THRESHOLD" decimal comment '\n'  {g_param.paramDUPLICATE_THRESHOLD=$<decimal>2;g_found_parameters[indexDUPLICATE_THRESHOLD]=1; debug_grammar("\tDUPLICATE_THRESHOLD\n");}
      | "DUPLICATE_THRESHOLD" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for DUPLICATE_THRESHOLD.\n"
                                             "\tExpected a value of decimal type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "FRAME_DELTA" INTEGRAL comment '\n'  {g_param.paramFRAME_DELTA=$<integral>2;g_found_parameters[indexFRAME_DELTA]=1; debug_grammar("\tFRAME_DELTA\n");}
      | "FRAME_DELTA" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for FRAME_DELTA.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "HALF_SPACE_TUNNELING_MAX_MOVES" INTEGRAL comment '\n'  {g_param.paramHALF_SPACE_TUNNELING_MAX_MOVES=$<integral>2;g_found_parameters[indexHALF_SPACE_TUNNELING_MAX_MOVES]=1; debug_grammar("\tHALF_SPACE_TUNNELING_MAX_MOVES\n");}
      | "HALF_SPACE_TUNNELING_MAX_MOVES" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for HALF_SPACE_TUNNELING_MAX_MOVES.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "HALF_SPACE_ASSYMETRY_THRESH" decimal comment '\n'  {g_param.paramHALF_SPACE_ASSYMETRY_THRESH=$<decimal>2;g_found_parameters[indexHALF_SPACE_ASSYMETRY_THRESH]=1; debug_grammar("\tHALF_SPACE_ASSYMETRY_THRESH\n");}
      | "HALF_SPACE_ASSYMETRY_THRESH" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for HALF_SPACE_ASSYMETRY_THRESH.\n"
                                             "\tExpected a value of decimal type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "MAX_DELTA_OFFSET" decimal comment '\n'  {g_param.paramMAX_DELTA_OFFSET=$<decimal>2;g_found_parameters[indexMAX_DELTA_OFFSET]=1; debug_grammar("\tMAX_DELTA_OFFSET\n");}
      | "MAX_DELTA_OFFSET" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for MAX_DELTA_OFFSET.\n"
                                             "\tExpected a value of decimal type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "MAX_DELTA_WIDTH" decimal comment '\n'  {g_param.paramMAX_DELTA_WIDTH=$<decimal>2;g_found_parameters[indexMAX_DELTA_WIDTH]=1; debug_grammar("\tMAX_DELTA_WIDTH\n");}
      | "MAX_DELTA_WIDTH" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for MAX_DELTA_WIDTH.\n"
                                             "\tExpected a value of decimal type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "MAX_DELTA_ANGLE" decimal comment '\n'  {g_param.paramMAX_DELTA_ANGLE=$<decimal>2;g_found_parameters[indexMAX_DELTA_ANGLE]=1; debug_grammar("\tMAX_DELTA_ANGLE\n");}
      | "MAX_DELTA_ANGLE" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for MAX_DELTA_ANGLE.\n"
                                             "\tExpected a value of decimal type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "MIN_SIGNAL" decimal comment '\n'  {g_param.paramMIN_SIGNAL=$<decimal>2;g_found_parameters[indexMIN_SIGNAL]=1; debug_grammar("\tMIN_SIGNAL\n");}
      | "MIN_SIGNAL" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for MIN_SIGNAL.\n"
                                             "\tExpected a value of decimal type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "WIDTH_MAX" decimal comment '\n'  {g_param.paramWIDTH_MAX=$<decimal>2;g_found_parameters[indexWIDTH_MAX]=1; debug_grammar("\tWIDTH_MAX\n");}
      | "WIDTH_MAX" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for WIDTH_MAX.\n"
                                             "\tExpected a value of decimal type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "WIDTH_MIN" decimal comment '\n'  {g_param.paramWIDTH_MIN=$<decimal>2;g_found_parameters[indexWIDTH_MIN]=1; debug_grammar("\tWIDTH_MIN\n");}
      | "WIDTH_MIN" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for WIDTH_MIN.\n"
                                             "\tExpected a value of decimal type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "WIDTH_STEP" decimal comment '\n'  {g_param.paramWIDTH_STEP=$<decimal>2;g_found_parameters[indexWIDTH_STEP]=1; debug_grammar("\tWIDTH_STEP\n");}
      | "WIDTH_STEP" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for WIDTH_STEP.\n"
                                             "\tExpected a value of decimal type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "ANGLE_STEP" decimal comment '\n'  {g_param.paramANGLE_STEP=$<decimal>2;g_found_parameters[indexANGLE_STEP]=1; debug_grammar("\tANGLE_STEP\n");}
      | "ANGLE_STEP" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for ANGLE_STEP.\n"
                                             "\tExpected a value of decimal type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "OFFSET_STEP" decimal comment '\n'  {g_param.paramOFFSET_STEP=$<decimal>2;g_found_parameters[indexOFFSET_STEP]=1; debug_grammar("\tOFFSET_STEP\n");}
      | "OFFSET_STEP" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for OFFSET_STEP.\n"
                                             "\tExpected a value of decimal type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "TLEN" INTEGRAL comment '\n'  {g_param.paramTLEN=$<integral>2;g_found_parameters[indexTLEN]=1; debug_grammar("\tTLEN\n");}
      | "TLEN" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for TLEN.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "MIN_SIZE" INTEGRAL comment '\n'  {g_param.paramMIN_SIZE=$<integral>2;g_found_parameters[indexMIN_SIZE]=1; debug_grammar("\tMIN_SIZE\n");}
      | "MIN_SIZE" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for MIN_SIZE.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "MIN_LEVEL" INTEGRAL comment '\n'  {g_param.paramMIN_LEVEL=$<integral>2;g_found_parameters[indexMIN_LEVEL]=1; debug_grammar("\tMIN_LEVEL\n");}
      | "MIN_LEVEL" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for MIN_LEVEL.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "HAT_RADIUS" decimal comment '\n'  {g_param.paramHAT_RADIUS=$<decimal>2;g_found_parameters[indexHAT_RADIUS]=1; debug_grammar("\tHAT_RADIUS\n");}
      | "HAT_RADIUS" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for HAT_RADIUS.\n"
                                             "\tExpected a value of decimal type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "SEED_THRESH" decimal comment '\n'  {g_param.paramSEED_THRESH=$<decimal>2;g_found_parameters[indexSEED_THRESH]=1; debug_grammar("\tSEED_THRESH\n");}
      | "SEED_THRESH" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for SEED_THRESH.\n"
                                             "\tExpected a value of decimal type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "SEED_ACCUM_THRESH" decimal comment '\n'  {g_param.paramSEED_ACCUM_THRESH=$<decimal>2;g_found_parameters[indexSEED_ACCUM_THRESH]=1; debug_grammar("\tSEED_ACCUM_THRESH\n");}
      | "SEED_ACCUM_THRESH" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for SEED_ACCUM_THRESH.\n"
                                             "\tExpected a value of decimal type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "SEED_ITERATION_THRESH" decimal comment '\n'  {g_param.paramSEED_ITERATION_THRESH=$<decimal>2;g_found_parameters[indexSEED_ITERATION_THRESH]=1; debug_grammar("\tSEED_ITERATION_THRESH\n");}
      | "SEED_ITERATION_THRESH" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for SEED_ITERATION_THRESH.\n"
                                             "\tExpected a value of decimal type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "SEED_ITERATIONS" INTEGRAL comment '\n'  {g_param.paramSEED_ITERATIONS=$<integral>2;g_found_parameters[indexSEED_ITERATIONS]=1; debug_grammar("\tSEED_ITERATIONS\n");}
      | "SEED_ITERATIONS" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for SEED_ITERATIONS.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "SEED_SIZE_PX" INTEGRAL comment '\n'  {g_param.paramSEED_SIZE_PX=$<integral>2;g_found_parameters[indexSEED_SIZE_PX]=1; debug_grammar("\tSEED_SIZE_PX\n");}
      | "SEED_SIZE_PX" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for SEED_SIZE_PX.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "SEED_ON_GRID_LATTICE_SPACING" INTEGRAL comment '\n'  {g_param.paramSEED_ON_GRID_LATTICE_SPACING=$<integral>2;g_found_parameters[indexSEED_ON_GRID_LATTICE_SPACING]=1; debug_grammar("\tSEED_ON_GRID_LATTICE_SPACING\n");}
      | "SEED_ON_GRID_LATTICE_SPACING" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for SEED_ON_GRID_LATTICE_SPACING.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "SEED_METHOD" value comment '\n'  {g_param.paramSEED_METHOD=$<valSEED_METHOD>2;g_found_parameters[indexSEED_METHOD]=1; debug_grammar("\tSEED_METHOD\n");}
      | "SEED_METHOD" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for SEED_METHOD.\n"
                                             "\tExpected a value of :\n\t\tSEED_EVERYWHERE\n\t\tSEED_ON_MHAT_CONTOURS\n\t\tSEED_ON_GRID\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "IDENTITY_SOLVER_SHAPE_NBINS" INTEGRAL comment '\n'  {g_param.paramIDENTITY_SOLVER_SHAPE_NBINS=$<integral>2;g_found_parameters[indexIDENTITY_SOLVER_SHAPE_NBINS]=1; debug_grammar("\tIDENTITY_SOLVER_SHAPE_NBINS\n");}
      | "IDENTITY_SOLVER_SHAPE_NBINS" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for IDENTITY_SOLVER_SHAPE_NBINS.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "IDENTITY_SOLVER_VELOCITY_NBINS" INTEGRAL comment '\n'  {g_param.paramIDENTITY_SOLVER_VELOCITY_NBINS=$<integral>2;g_found_parameters[indexIDENTITY_SOLVER_VELOCITY_NBINS]=1; debug_grammar("\tIDENTITY_SOLVER_VELOCITY_NBINS\n");}
      | "IDENTITY_SOLVER_VELOCITY_NBINS" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for IDENTITY_SOLVER_VELOCITY_NBINS.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "COMPARE_IDENTITIES_DISTS_NBINS" INTEGRAL comment '\n'  {g_param.paramCOMPARE_IDENTITIES_DISTS_NBINS=$<integral>2;g_found_parameters[indexCOMPARE_IDENTITIES_DISTS_NBINS]=1; debug_grammar("\tCOMPARE_IDENTITIES_DISTS_NBINS\n");}
      | "COMPARE_IDENTITIES_DISTS_NBINS" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for COMPARE_IDENTITIES_DISTS_NBINS.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "HMM_RECLASSIFY_BASELINE_LOG2" decimal comment '\n'  {g_param.paramHMM_RECLASSIFY_BASELINE_LOG2=$<decimal>2;g_found_parameters[indexHMM_RECLASSIFY_BASELINE_LOG2]=1; debug_grammar("\tHMM_RECLASSIFY_BASELINE_LOG2\n");}
      | "HMM_RECLASSIFY_BASELINE_LOG2" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for HMM_RECLASSIFY_BASELINE_LOG2.\n"
                                             "\tExpected a value of decimal type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "HMM_RECLASSIFY_VEL_DISTS_NBINS" INTEGRAL comment '\n'  {g_param.paramHMM_RECLASSIFY_VEL_DISTS_NBINS=$<integral>2;g_found_parameters[indexHMM_RECLASSIFY_VEL_DISTS_NBINS]=1; debug_grammar("\tHMM_RECLASSIFY_VEL_DISTS_NBINS\n");}
      | "HMM_RECLASSIFY_VEL_DISTS_NBINS" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for HMM_RECLASSIFY_VEL_DISTS_NBINS.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "HMM_RECLASSIFY_SHP_DISTS_NBINS" INTEGRAL comment '\n'  {g_param.paramHMM_RECLASSIFY_SHP_DISTS_NBINS=$<integral>2;g_found_parameters[indexHMM_RECLASSIFY_SHP_DISTS_NBINS]=1; debug_grammar("\tHMM_RECLASSIFY_SHP_DISTS_NBINS\n");}
      | "HMM_RECLASSIFY_SHP_DISTS_NBINS" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for HMM_RECLASSIFY_SHP_DISTS_NBINS.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "SHOW_PROGRESS_MESSAGES" INTEGRAL comment '\n'  {g_param.paramSHOW_PROGRESS_MESSAGES=$<integral>2;g_found_parameters[indexSHOW_PROGRESS_MESSAGES]=1; debug_grammar("\tSHOW_PROGRESS_MESSAGES\n");}
      | "SHOW_PROGRESS_MESSAGES" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for SHOW_PROGRESS_MESSAGES.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | "SHOW_DEBUG_MESSAGES" INTEGRAL comment '\n'  {g_param.paramSHOW_DEBUG_MESSAGES=$<integral>2;g_found_parameters[indexSHOW_DEBUG_MESSAGES]=1; debug_grammar("\tSHOW_DEBUG_MESSAGES\n");}
      | "SHOW_DEBUG_MESSAGES" error comment '\n'  {
                                      fprintf(stderr,
                                             "Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                             "\tCould not interpret value for SHOW_DEBUG_MESSAGES.\n"
                                             "\tExpected a value of integral type.\n",
                                               @2.first_line,@2.first_column+1,
                                               @2.last_column+1);

                                       PARSER_INCERR
                                       yyerrok;
                                     }
      | comment '\n'                { debug_grammar("\tCOMMENT LINE\n"); }
      | error '\n'                  { fprintf(stderr,"Problem encountered loading parameters file at line %d, columns %d-%d\n"
                                                       "\tCould not interpret parameter name.\n",
                                               @1.first_line,@1.first_column+1,
                                               @1.last_column+1);
                                       PARSER_INCERR
                                       yyerrok;
                                     }
      ;
decimal: INTEGRAL         {$$=$1;}
       | DECIMAL          {$$=$1;}
value:   INTEGRAL         {$<integral>$=$1;}
       | DECIMAL          {$<decimal>$=$1;}
       | TOK_SEED_EVERYWHERE          {$<valSEED_METHOD>$=SEED_EVERYWHERE;}
       | TOK_SEED_ON_MHAT_CONTOURS          {$<valSEED_METHOD>$=SEED_ON_MHAT_CONTOURS;}
       | TOK_SEED_ON_GRID          {$<valSEED_METHOD>$=SEED_ON_GRID;}
       ;
%%
FILE *fp=NULL;                                               // when a file is opened, this points to the file
int yylex(void)
{ int c;
  static char *str = NULL;
  static size_t str_max_size = 0;
  ASRT(fp);


  if(!str)
  { str = malloc(1024);
    ASRT(str);
    str_max_size = 1024;
  }

  while( (c=getc(fp))==' '||c=='\t' ) ++yylloc.last_column;  // skip whitespace
  if(c==0)
  { if(feof(fp))
      return 0;
    if(ferror(fp))
    { fprintf(stderr,"\t lex - Got error: %d\n",ferror(fp));
    }
  }

  /* Step */
  yylloc.first_line = yylloc.last_line;
  yylloc.first_column = yylloc.last_column;
  debug_lexer("\t\t\t\t lex - Start: %d.%d\n",yylloc.first_line,yylloc.first_column);
  
  if(isalpha(c))                                 // process string tokens
  { int i,n=0;
    while(!isspace(c))
    { 
      ++yylloc.last_column;
      if( n >= str_max_size )                    // resize if necessary
      { str_max_size = 1.2*n+50;
        str = realloc(str,str_max_size);
        ASRT(str);
      }
      str[n++] = c;
      c = getc(fp);
    }
    ungetc(c,fp);
    str[n]='\0';
                                                // search token table
    for (i = 0; i < YYNTOKENS; i++)
    {
      if (yytname[i] != 0
          && yytname[i][0] == '"'
          && ! strncmp(yytname[i] + 1, str,strlen(str))
          && yytname[i][strlen(str) + 1] == '"'
          && yytname[i][strlen(str) + 2] == 0)
        break;
    }
    if(i<YYNTOKENS)
    {
      debug_lexer("\tlex - (%d) %s\n",i,str);
      return yytoknum[i];
    }
    else //nothing was found - put characters back on the stream
    { yylloc.last_column-=n;
      while(n--)
      { ungetc(str[n],fp);
      }
      c = getc(fp);
      ++yylloc.last_column;
    }
  }

  if(c=='.'||isdigit(c)||c=='-')     // process numbers
  { int n = 0;
    do
    { if( n >= str_max_size )        //resize if necessary
      { str_max_size = 1.2*n+50;
        str = realloc(str,str_max_size);
        ASRT(str);
      }
      str[n++] = c;
      c = getc(fp);
      ++yylloc.last_column;
    }while(c=='.'||isdigit(c));
    ungetc(c,fp);
    --yylloc.last_column;
    str[n]='\0';
    if(strchr(str,'.'))
    { yylval.decimal = atof(str);
      debug_lexer("\tlex - DECIMAL (%f)\n",yylval.decimal);
      return DECIMAL;
    } else
    { yylval.integral = atoi(str);
      debug_lexer("\tlex - INTEGRAL (%d)\n",yylval.integral);
      return INTEGRAL;
    }
  }

  if(c=='[')                                                 // process section headers as comments
  { while( getc(fp)!='\n' ) ++yylloc.last_column;           // read the rest of the line
    ungetc('\n',fp);
    debug_lexer("\tlex - COMMENT\n");
    return COMMENT;
  }
  if(c=='/')                                                 // process c-style end-of-line comments
  { int d;
    d=getc(fp);
    ++yylloc.last_column;
    if(d=='/' || d=='*')
    { while( getc(fp)!='\n' ) ++yylloc.last_column;        // read the rest of the line
      ungetc('\n',fp);
    }
    debug_lexer("\tlex - COMMENT\n");
    return COMMENT;
  }

  if(c==EOF)
  { debug_lexer("\tlex - EOF\n");
    fclose(fp);
    fp=0;
  }
  if(c=='\n')
  {
    ++yylloc.last_line;
    yylloc.last_column=0;
  }
  return c;
}

void yyerror(char const *s)
{ //fprintf(stderr,"Parse Error:\n---\n\t%s\n---\n",s);
}

SHARED_EXPORT
t_params *Params(void) {return &g_param;}

SHARED_EXPORT
int Load_Params_File(char *filename)
{ int sts; //0==success, 1==failure
  // FILE *fp is global
  g_nparse_errors=0;
  memset(g_found_parameters,0,sizeof(g_found_parameters));
  fp = fopen(filename,"r");
  if(!fp)
  { fprintf(stderr,"Could not open parameter file at %s.\n",filename);
    return 1;
  }
  sts = yyparse();
  if(fp) fclose(fp);
  sts |= (g_nparse_errors>0);
  {
    int i;
    for(i=0;i<indexMAX;++i)
      if(g_found_parameters[i]==0)
      { sts=1;
        fprintf(stderr,"Failed to load parameter: %s\n",g_param_string_table[i]);
      }
  }
  return sts;
}

SHARED_EXPORT
int Print_Params_File(char *filename)
{ int sts=0; //0==success, 1==failure
  FILE *fp;
  fp = fopen(filename,"w");
  if(!fp)
  { fprintf(stderr,"Could not open parameter file for writing: %s\n",filename);
    return 1;
  }
  {
    fprintf(fp,"[error]\n");
    fprintf(fp,"SHOW_DEBUG_MESSAGES     1\n");
    fprintf(fp,"SHOW_PROGRESS_MESSAGES  1\n");
    fprintf(fp,"\n");
    fprintf(fp,"[reclassify]\n");
    fprintf(fp,"HMM_RECLASSIFY_SHP_DISTS_NBINS 16\n");
    fprintf(fp,"HMM_RECLASSIFY_VEL_DISTS_NBINS 8096\n");
    fprintf(fp,"HMM_RECLASSIFY_BASELINE_LOG2   -500.0\n");
    fprintf(fp,"COMPARE_IDENTITIES_DISTS_NBINS 8096\n");
    fprintf(fp,"IDENTITY_SOLVER_VELOCITY_NBINS 8096\n");
    fprintf(fp,"IDENTITY_SOLVER_SHAPE_NBINS    16\n");
    fprintf(fp,"\n");
    fprintf(fp,"[trace]\n");
    fprintf(fp,"SEED_METHOD                    SEED_ON_GRID // Specify seeding method: may be SEED_ON_MHAT_CONTOURS, SEED_ON_GRID, or SEED_EVERYWHERE\n");
    fprintf(fp,"SEED_ON_GRID_LATTICE_SPACING   50           // (pixels)\n");
    fprintf(fp,"SEED_SIZE_PX                   4            // Width of the seed detector in pixels.\n");
    fprintf(fp,"SEED_ITERATIONS                1            // Maxium number of iterations to re-estimate a seed.\n");
    fprintf(fp,"SEED_ITERATION_THRESH          0.0          // (0 to 1) Threshold score determining when a seed should be reestimated.\n");
    fprintf(fp,"SEED_ACCUM_THRESH              0.0          // (0 to 1) Threshold score determining when to accumulate statistics\n");
    fprintf(fp,"SEED_THRESH                    0.99         // (0 to 1) Threshold score determining when to generate a seed\n");
    fprintf(fp,"\n");
    fprintf(fp,"HAT_RADIUS                     1.5          // Mexican-hat radius for whisker detection (seeding)\n");
    fprintf(fp,"MIN_LEVEL                      1            // Level-set threshold for mexican hat result.  Used for seeding on mexican hat contours.\n");
    fprintf(fp,"MIN_SIZE                       20           // Minimum # of pixels in an object considered for mexican-hat based seeding.\n");
    fprintf(fp,"\n");
    fprintf(fp,"                                            // detector banks parameterization.  If any of these change, the detector banks\n");
    fprintf(fp,"                                            // should be deleted.  They will be regenerated on the next run.\n");
    fprintf(fp,"                                            //\n");
    fprintf(fp,"TLEN                           8            // (px) half the size of the detector support.  If this is changed, the detector banks must be deleted.\n");
    fprintf(fp,"OFFSET_STEP                    .1           // pixels\n");
    fprintf(fp,"ANGLE_STEP                     18.          // divisions of pi/4\n");
    fprintf(fp,"WIDTH_STEP                     .2           // (pixels)\n");
    fprintf(fp,"WIDTH_MIN                      0.4          // (pixels) must be a multiple of WIDTH_STEP\n");
    fprintf(fp,"WIDTH_MAX                      6.5          // (pixels) must be a multiple of WIDTH_STEP\n");
    fprintf(fp,"MIN_SIGNAL                     5.0          // minimum detector response per detector column.  Typically: (2*TLEN+1)*MIN_SIGNAL is the threshold determining when tracing stops.\n");
    fprintf(fp,"MAX_DELTA_ANGLE                10.1         // (degrees)  The detector is constrained to turns less than this value at each step.\n");
    fprintf(fp,"MAX_DELTA_WIDTH                6.0          // (pixels)   The detector width is constrained to change less than this value at each step.\n");
    fprintf(fp,"MAX_DELTA_OFFSET               6.0          // (pixels)   The detector offset is constrained to change less than this value at each step.\n");
    fprintf(fp,"HALF_SPACE_ASSYMETRY_THRESH    0.25         // (between 0 and 1)  1 is completely insensitive to asymmetry\n");
    fprintf(fp,"HALF_SPACE_TUNNELING_MAX_MOVES 50           // (pixels)  This should be the largest size of an occluding area to cross\n");
    fprintf(fp,"\n");
    fprintf(fp,"FRAME_DELTA                    1            // [depricated?] used in compute_zone to look for moving objects\n");
    fprintf(fp,"DUPLICATE_THRESHOLD            5.0          // [depricated?]\n");
    fprintf(fp,"MIN_LENGTH                     20           // [depricated?]           If span of object is not 20 pixels will not use as a seed\n");
    fprintf(fp,"MIN_LENSQR                     100          // [depricated?]           (MIN_LENGTH/2)^2\n");
    fprintf(fp,"MIN_LENPRJ                     14           // [depricated?] [unused]  floor(MIN_LENGTH/sqrt(2))\n");
  }
  fclose(fp);
  return sts;
}

