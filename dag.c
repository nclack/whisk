#include<stdio.h>
#include<stdlib.h>

#include "utilities.h"
#include "dag.h"
#include "string.h"
#include "math.h"

#include "match.h"

#ifndef INFINITY
  #ifdef WIN32
    #define INFINITY (1.0e15)
  #else
    #define INFINITY (1.0/0.0)
  #endif
#endif

void  Init_Node_Array(Node *array, int n)
{ memset( array, 0, n*sizeof(Node) );
}

Edge *Create_Edge(Node *source, Node* sink, double score) 
{ Edge *p,*e = (Edge*) Guarded_Malloc( sizeof(Edge) , "Create Edge");
  e->source = source;
  e->sink   = sink;
  p = source->e;
  if(p)
  { e->next       = p;
    e->prev       = p->prev;
    e->prev->next = e;
    p->prev       = e;
    
  } else 
  { source->e = e;
    e->next = e;
    e->prev = e;
  }

  e->score = score;
  e->mark = 0;
  return e;
}

Edge *Prune_Edge(Edge *e)
/* Cut and deallocate edge *
 * returns next edge       */
{ Node *source = e->source;
  Edge *a,*c;
  
  source->e = e;
  if( e == e->next ) /* parent out-degree is 1 */
  { source->e = NULL;
  } else 
  { a = e->prev;
    c = e->next;
    a->next = c;
    c->prev = a;
    source->e = c;
  }
  
  free(e);
  return source->e;
}

Node * bipartite_matching_ex (Node **source, int nsource, 
                              Node **dest  , int ndest,
                              Node  *graph , int sz,
                              double (*distance)(Node *a, Node *b, Node *graph, int sz),
                              void (*distance_hook)(double* d, int nsource, int ndest, void *parm),
                              void *parm)
/*
 * Returns a new graph with the mapping.  The node data for the new graph    *
 * points to the source and destination input nodes.                         *
 *                                                                           *
 * The graph is returned as an array of nodes nsources+ndest long.           *
 *                                                                           *
 * The returned array of nodes needs to be freed when done.                  *
 *                                                                           */
{ double *d;
  Assignment m;
  /* map is a graph representation of the results                            *
   * the data of each node in this map is one of the input nodes             *
   */
  Node  *map = (Node*) Guarded_Malloc( sizeof(Node)*(nsource+ndest), "Bipartite Matching" );
  int i,j;

  d = (double*) Guarded_Malloc( sizeof(double)*nsource*ndest, "Allocate distance matrix" );

  memset( map, 0, sizeof(Node)*(nsource+ndest) );
  
  /* calc distances */
  { for( i=0; i<nsource; i++ )
      for( j=0; j<ndest; j++ )  
        d[i+j*nsource] = distance( source[i], dest[j], graph, sz );
    /* init map */
    for( i=0; i< nsource; i++ )
      map[i].data = (void*) source[i];
    for( j=0; j< ndest; j++ )
      map[nsource+j].data = (void*) dest[j];
  }

  /* The hook can be used to modify the distance matrix */
  if( distance_hook != NULL )
    distance_hook(d,nsource,ndest,parm);
  
  /* bipartite matching - hungarian algorithm */
  m = match( d, nsource, ndest );
                                                 
  /* make edges according to assignment in new graph */
  for( i=0; i< m.n; i++ ) /*index into sources*/
  { j = m.assignment[i];
    if( j>=0 ) /* a value of less than zero indicates no match */
      Create_Edge( map + i, map + nsource + j, d[i+j*nsource] );
  }
  free(d);
  return map;
}
                           
Node * Bipartite_Matching (Node **source, int nsource, 
                           Node **dest  , int ndest,
                           Node  *graph , int sz,
                           double (*distance)(Node *a, Node *b, Node *graph, int sz) )
{ return bipartite_matching_ex(source,nsource,dest,ndest,graph,sz,distance,NULL,NULL);
}

/*
 * Bipartite matching n-best
 *
 * Before doing the bipartite matching, filter so only n-best scores are used
 * for each row.  For the others, the score is set to infinity.
 *
 * This requires a sort on each row so the filtering is 
 * O( nsource * ndest * log(ndest) ).
 */
          /*
int (*hook_filter_n_best_comparitor)(void *d, const void *a, const void *b)
{ return ((double*)d)[*a] - ((double)d)[*b];
}

double hook_filter_n_best(double *d, int nsource, int ndest, void *parm)
{ int nbest = *(int*)parm;
  int i,j;
  int indices[ndest];

  for(i=0;i<ndest;i++)
    indices[ndest] = i;

  for(i=0; i<nsource; i++)
  { qsort_r(  
  }
}

Node* Bipartite_Matching_N_Best (Node **source, int nsource, 
                                 Node **dest  , int ndest,
                                 Node  *graph , int sz,
                                 int nbest,
                                 double (*distance)(Node *a, Node *b, Node *graph, int sz) )
{
}
            */
static int height( Node* n )
{
  Edge *e;
  int d = 0, h;
  
  if(n->val >= 0.0 ) /* indicates height has already been computed */
    return n->val;   /* return precomputed value                   */

  if(n->e)           /* compute - height := max(height of children)*/
  { for( e = n->e->next; e != n->e; e = e->next )
    { h = height( e->sink );
      d = (d>h) ? d : h;
    }
    h = height( e->sink ); /* and one to grow on                   */
    d = (d>h) ? d : h;

    d++;             /* count the step to children                 */
  }
  n->val = d;        /* update self                                */
  return d;
}

void Compute_Height(Node* array, int sz)
{ int i;
  Edge *e;
  Node *n;

  for( i=0; i<sz; i++)
    array[i].val = -1.0; /* indicates uncomputed */

  for( i=0; i<sz; i++)
    height( &array[i] );
}

void Compute_InDegree(Node* array, int sz)
{ int i;
  Edge *e;
  Node *n;
  
  for( i=0; i<sz; i++ ) 
    array[i].indegree = 0;

  for( i=0; i<sz; i++ )
  { n = array+i;
    if(n->e)
    { for( e = n->e->next; e != n->e; e = e->next )
        e->sink->indegree++;
      e->sink->indegree++;
    }
  }
}

void handler_compute_shortest_paths(Node *n, void *data)
{ Edge *e;
  double s;
  if( n->e )
  { for( e = n->e->next; e != n->e; e = e->next )
    { s = e->source->val + e->score;
      if( s < e->sink->val )
      { e->sink->best_incoming = e;
        e->sink->val = s;
      }
    } 
    s = e->source->val + e->score; 
    if( s < e->sink->val )
    { e->sink->best_incoming = e; 
      e->sink->val = s;
    }
  }
}

void Compute_Shortest_Paths(Node *source, Node *array, int sz)
/*                                                           *
 * For each node, n, in array in topological order:          *
 *  - assigns n.val = shortest distance from source to n     *
 *  - assigns n.best_incoming to the incoming edge along the *
 *            shortest path                                  *
 * Runs in time linear with sz.                              */
{ int i;

  for( i=0; i<sz; i++ )
  { array[i].val = INFINITY;
    array[i].best_incoming = NULL;
  }
  source->val = 0.0;

  Topological_Sort( array, sz, NULL, handler_compute_shortest_paths );
}

void Topological_Sort(Node *array, int sz, void* state, void (*handler)(Node*,void*))
/*
 * Processes nodes using the handler in topological order (that is: from `root`
 * to `dependent`). See `handler_label_node` for an example handler.
 */
{ int i;
  Edge *e;
  Node *n;
  static Node **queue = NULL;
  static unsigned int maxlen = 0;
  unsigned int front, back;
  
  Compute_InDegree(array, sz);

  /*
  ** Init the queue
  */
  if( 2*sz > maxlen )
  { maxlen = 2*sz;
    queue = (Node**) 
        Guarded_Realloc(queue, maxlen*sizeof(Node*), "topological sort");
  }
  front = back = 0;
  
  for( i=0; i<sz; i++ )
    if( !array[i].indegree )
      queue[ back++%maxlen ] = array+i;

  /*
   ** process the queue
   */
  while( (front%maxlen) != (back%maxlen) )
  { n = queue[front++%maxlen];
    (*handler)(n,state);       /* call the handler */

    if(n->e) 
    { for( e = n->e->next; e != n->e; e = e->next )
      { e->sink->indegree--;
        if( !e->sink->indegree )           
          queue[ back++%maxlen ] = e->sink;
      }
      e->sink->indegree--; 
      if( !e->sink->indegree )           
        queue[ back++%maxlen ] = e->sink;
    }
  }
}

void Get_Next_Nodes(Node *array, int sz, Node **source, int nsource, Node*** result, int* nres)
{ int i, count=0;
  Edge *e;
  Node *n, **r; 

  /* init */
  for( i=0; i<nsource; i++ )
  { n = source[i];
    e = n->e;
    if(e)
    { e->sink->val = 0;
      for( e = n->e->next; e != n->e; e = e->next )
        e->sink->val = 0;
    }
  }

  /* count */
  count = 0;
  for( i=0; i<nsource; i++ )
  { n = source[i];
    e = n->e;
    if(e)
    { if(e->sink->val == 0)
      { count++;
        e->sink->val = 1;
      }
      for( e = n->e->next; e != n->e; e = e->next )
      { if(e->sink->val == 0)
        { count++;
          e->sink->val = 1;
        }
      }
    }
  }
  *nres = count;
  if( count == 0 )
  { (*result) = NULL;
    return;
  }

  /* collect */
  (*result) = (Node**)Guarded_Malloc( sizeof(Node*)*count, "Get next nodes" );
  r = (*result);

  count = 0;
  for( i=0; i<nsource; i++ )
  { n = source[i];
    e = n->e;
    if(e)
    {
      if(e->sink->val==1)
      { r[count++] = e->sink;
        e->sink->val = 0;
      }
      for( e = n->e->next; e != n->e; e = e->next )
      { if(e->sink->val==1)
        { r[count++] = e->sink;
          e->sink->val = 0;
        }
      }
    }
  }
  *nres = count;
  return;
}

static void handler_label_node(Node* n, void* state)
{ unsigned int *i = (unsigned int*) state;
  n->label = (*i)++;
}

void Label_Topological_Order(Node* array, int sz)
{ int count=0;
  Topological_Sort( array, sz, &count, &handler_label_node );
}

static int select_all(Node *n)
{ return 1; 
}

void Dump_Dot_File( FILE *file, Node *array, int sz)
{ Dump_Dot_File_Select( file, array, sz, select_all );
}

void Dump_Dot_File_Select( FILE *file, Node *array, int sz, int (*select)(Node *))
{ int i,count=0;
  Edge *e;
  Node *n;
  char text[256];

  fprintf(file, "digraph L0 {\n");
  fprintf(file, "\tsize=8\n"); 
  fprintf(file, "\tordering=out\n");
  fprintf(file, "\tnode [shape=box]\n");

  for( i=0; i<sz; i++)
  { n = array + i;
    if(n->msg && select(n) )
    { sprintf(text, "\"%d: %s\"", n->label, n->msg);
      fprintf(file, "\t%d [label = %s];\n",n->label, text);
    }
  }

  for( i=0; i<sz; i++)
  { n = array+i;
    if(n->e && select(n) ) 
    { for( e=n->e->next; e != n->e; e=e->next )
      //  fprintf(file, "\t%d -> %d;\n", n->label, e->sink->label);
      //fprintf(file, "\t%d -> %d;\n", n->label, e->sink->label);
        fprintf(file, "\t%d -> %d [label=\"%g\"];\n", n->label, e->sink->label, e->score);
      fprintf(file, "\t%d -> %d [label=\"%g\"];\n", n->label, e->sink->label, e->score);
    }
  }
  fprintf(file, "}");
}
