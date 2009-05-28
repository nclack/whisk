#ifndef _H_TRAJECTORY
#define _H_TRAJECTORY

#include "common.h"
#include "compat.h"
#include "trace.h"
#include "dag.h"

#define DISTANCE_CUTOFF  10.0
#define LOST_DISTANCE    9.9
#define LOST_PER_FRAME  0

extern enum {
  NL_WHISKER_SEG = 0,
  NL_LOST
} NodeLabel;

/* 
 * FUNCTIONS
 */

int  write_trajectory( FILE* file, Node* root, int depth, int path_id,
                       int (*follow)(int,Node*,Edge**,int,void*, 
                                     void (*output_func)(void*,Edge**,int,int)) );
int  follow_minimal_edge_score( int path_id, Node* root, 
                                Edge** path, int depth, 
                                void *arg, void (*output_func)(void*,Edge**,int,int) );
int  follow_minimal_edge_val  ( int path_id, Node* root, 
                                Edge** path, int depth, 
                                void *arg, void (*output_func)(void*,Edge**,int,int) );
int  follow_all_edges         ( int path_id, Node* root, 
                                Edge** path, int depth, 
                                void *arg, void (*output_func)(void*,Edge**,int,int) );
int  follow_best_incoming     ( int path_id, Node* root, 
                                Edge** path, int depth, 
                                void *arg, void (*output_func)(void*,Edge**,int,int) ); 
void output_trajectory_tofile ( void *arg, Edge **path, int depth, int pathid );
void get_forsaken( Node *graph,       int sz, 
                   Node ***orphans,   int *norphans,
                   Node ***childless, int *nchildless);

double distance_shortest_path(Node *a, Node *b, Node *graph, int sz);
double distance_pairwise(Node *a, Node *b, Node *graph, int sz);

void local_best_paths(  Node  *graph,  int sz,
                        Node **source, int nsource );

#endif //_H_TRAJECTORY
 
