#ifndef _H_DAG
#define _H_DAG

#include<stdio.h>
#include<stdlib.h>

typedef struct _Edge Edge;

typedef struct
{ Edge         *e;
  void         *data;
  char         *msg;
  int           type;  /* to indicate the Node subtype...user defined */
  unsigned int  label;

  /* Volitile properties                      *
   * - These are subject to change on any     *
   *   operation on the graph. So their value *
   *   can only be gauranteed temporarily     */
  double        val;       
  unsigned int  indegree;  
  Edge         *best_incoming;
} Node;

struct _Edge
{ Edge    *next;
  Edge    *prev;
  Node    *source;
  Node    *sink;

  int     mark;
  double  score;
};

void  Init_Node_Array           (Node *array, int n);
Edge *Create_Edge               (Node *source, Node* sink, double score); 
Edge *Prune_Edge                (Edge *e);
Node *Bipartite_Matching        (Node **source, int nsource, 
                                 Node **dest  , int ndest,
                                 Node  *graph , int sz,
                                 double (*distance)(Node *a, Node *b, Node *graph, int sz));

void  Compute_Height            (Node *array, int sz);
void  Compute_InDegree          (Node *array, int sz);
void  Compute_Shortest_Paths    (Node *source, Node *array, int sz);

void  Get_Next_Nodes            (Node *array, int sz, 
                                 Node **source, int nsource, 
                                 Node ***result, int* nres);

void  Topological_Sort          (Node *array, int sz, void* state, void (*handler)(Node*,void*));
void  Label_Topological_Order   (Node* array, int sz);
void  Dump_Dot_File             (FILE *file, Node *array, int sz);
void  Dump_Dot_File_Select      (FILE *file, Node *array, int sz, int (*select)(Node *));

#endif // _H_DAG
