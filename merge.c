/**********************************************************************************\
 *                                                                                 *
 * Whisker segment merging.                                                        *
 *                                                                                 *
 * [TODO] Merges duplicate segments (segments that mostly overlap).                *
 *        + Need a fast way of detecting collisions and overlaps                   *
 *          See CollisionTable                                                     *
 *        + Need a fast way of computing overlaps (within a tolerance)             *
 *          See Trace_Overlap                                                      *
 *        + Need an averaging proceedure                                           *
 * [TODO] Joins multiple segments from the same whisker into a single segment.     *
 *                                                                                 *
 * Author: Nathan Clack                                                            *
 * Date: 2009-05-25                                                                *
 * Copyright (c) 2009 HHMI. Free downloads and distribution are allowed as long    *
 * as proper credit is given to the author.  All other rights reserved.            *
 \*********************************************************************************/

#include "compat.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "common.h"
#include "error.h"
#include "trace.h"

#if 0
#define DEBUG_REMOVE_OVERLAPPING_WHISKERS_ONE_FRAME
#define DEBUG_COLLISIONTABLE_REQUEST_DEPTH
#define DEBUG_REMOVE_OVERLAPPING_WHISKERS_MULTI_FRAME
#define DEBUG_COLLISIONTABLE_REMOVE
#define DEBUG_TRACE_OVERLAP_ONE_SIDE
#define DEBUG_ALLOC_COLLISIONTABLE
#endif

/* ==============
 * CollisionTable
 * ==============
 *
 * This is a scalar (integer) volume.
 *
 * The `width` and `height` correspond to an image's width and height divided by
 * the `scale`. So these first two dimensions correspond with an image.
 *
 * The third dimension, the 'depth', is for holding the id's of whisker
 * segments passing over a particular x,y position. Actually, the first 
 * plane (depth=0) holds the hit count.  The rest holds the id's.
 *
 * In terms of memory usage, worst case is that all segments overlap at the
 * same point.  Then the depth will be the number of whisker segments + 1.  In
 * practice, the number of overlapping segments is ~5.  
 *
 * Most of the image is never hit, so much of the allocated space goes unused;
 * a sparse representation would be much more memory efficient.  However, this
 * table never gets that big and can be reused for every frame in the movie
 * without reallocating, so memory management isn't so much of an issue.
 *
 * The other problem with this approach is inserts/removals of id's at points
 * are O(d).
 *
 * Using linked lists would be more memory efficient, something necessary if 
 * hundreds or thousands of overlaps happen on any pixel.  There's a bit of
 * a speed tradeoff since it requires dereferencing each iteration and 
 * there's no way to optimize cache hits.  For now, I'm sticking with the table 
 * since I expect to d to be low.
 *
 * TODO: possible optimization: arrange table so hit records are contiguous.
 *       This means that on realloc a single pass has to be made to rearrange
 *       the data.  The upside is searching and modifying the hit records 
 *       makes better use of the cache.  Should give ~2X speed boost.
 */

typedef struct CollisionTable
{ unsigned int *table;
  int stride;
  int area;
  int depth;
  float scale;
} CollisionTable;

void CollisionTable_Reset( CollisionTable *this )
{ memset(this->table, 0, sizeof(unsigned int)*(this->area) );
}

CollisionTable *Alloc_CollisionTable(int width, int height, float scale, int depth_hint)
{ CollisionTable *this = Guarded_Malloc( sizeof(CollisionTable), "Create_CollisionTable");
  this->stride = (int) width/scale + 1;
  this->area   = ((int) (height/scale)+1) * this->stride;
  this->depth  = depth_hint;
  this->scale  = scale;

#ifdef DEBUG_ALLOC_COLLISIONTABLE
  debug("this->stride: %d\n", this->stride);
  debug("this->area  : %d\n", this->area  );
  debug("this->depth : %d\n", this->depth );
  debug("this->scale : %f\n", this->scale );
#endif

  this->table  = Guarded_Malloc( sizeof(unsigned int)*(this->area)*(this->depth+1)*2, 
                                 "Create_CollisionTable");
  CollisionTable_Reset(this);
  return this; 
}

void Free_CollisionTable( CollisionTable* this )
{ if(this)
  { if( this->table ) 
      free( this->table );
    free(this);
  }
}

void CollisionTable_Request_Depth( CollisionTable* this, int depth )
{ 
#ifdef DEBUG_COLLISIONTABLE_REQUEST_DEPTH
  //debug("depth: %3d\trequested: %3d\n",this->depth, depth);
#endif
  if( depth >= this->depth )
  { depth = 1.2 * depth + 10;
#ifdef DEBUG_COLLISIONTABLE_REQUEST_DEPTH
    debug("Reallocating collision table for depth of %d.\n",depth);
#endif
    this->table = Guarded_Realloc( this->table,
                                   sizeof(unsigned int)*(this->area)*(depth+1)*2, 
                                   "CollisionTable_Request_Depth" );
    this->depth = depth;
  }
}

void CollisionTable_Push( CollisionTable *this, int x, int y, int id, int index )
{ int p = x + y * this->stride;
  unsigned int *cnt = this->table + p;
  int area = this->area;
  int d = *cnt;

  // Check to see if id already counted ~O(d)
  while( d-- )
    if( cnt[ (2*d+1)*area ] == id )
      return;
  
  // Append
  CollisionTable_Request_Depth( this, *cnt+1 ); // possible realloc
  cnt = this->table + p;     // old *cnt might be invalid 
  (*cnt)++;
  p += (*cnt * 2 - 1) * area; // (*cnt-1)*2+1 = *cnt*2 - 1
  this->table[p       ] = id;
  this->table[p + area] = index;
}

void print_hits( CollisionTable *this, int p )
{ int area = this->area;
  unsigned int *c = this->table + p;
  int d = *c;
  printf("At %5d:  %4d items\n",p,d);
  while(d--)
    printf("\t%4d:id:%5d\tindex:%5d\n",d,c[(2*d+1)*area], c[(2*d+2)*area] );
}

void CollisionTable_Remove( CollisionTable *this, int x, int y, int id )
{ int p = x + y * this->stride;
  unsigned int *cnt = this->table + p;
  int area = this->area;
  int d = *cnt;

#ifdef DEBUG_COLLISIONTABLE_REMOVE
  debug("\tRemoving id:%5d from point (%3d,%3d)=%5d. %3d left.\n",id,x,y,p,d-1);
  print_hits(this,p);
#endif

  // remove id and shrink list ~O(d)
  while( d-- )
  { unsigned int *cur = cnt + (2*d+1)*area;
    if( *cur == id )
    { int i,n = *cnt - d - 1;                               // remove and copy down the rest
      for(i=0;i<n;i++)
      { cur[2*area*i]     = cur[2*(i+1)*area];
        cur[(2*i+1)*area] = cur[(2*i+3)*area]; 
      }
      (*cnt)--;                                             // decriment count
#ifdef DEBUG_COLLISIONTABLE_REMOVE
      print_hits(this,p);
#endif
      return;
    }
  }
}

void CollisionTable_Add_Segment( CollisionTable *table, Whisker_Seg *w, int id )
{ int n = w->len;
  float scale = table->scale;
  while(n--)
    CollisionTable_Push( table, 
                         (int) (w->x[n] / scale),
                         (int) (w->y[n] / scale),
                         id, n );
}

void CollisionTable_Remove_Segment( CollisionTable *table, Whisker_Seg *w, int id )
{ int n = w->len;
  float scale = table->scale;
  while(n--)
    CollisionTable_Remove( table, 
                           (int) (w->x[n] / scale),
                           (int) (w->y[n] / scale),
                           id );
}

void CollisionTable_Add_Segments( CollisionTable *table, Whisker_Seg *wv, int n )
{ while(n--)
    CollisionTable_Add_Segment( table, wv+n, n );
}

void CollisionTable_Counts_To_File( CollisionTable *this, char *filename )
{ FILE *fp = fopen(filename,"wb");
  if(fp == NULL)
    goto Err;
  fwrite(this->table, sizeof(unsigned int), this->area, fp);
  fclose(fp);
  return;
Err:
  warning("Could not open file at:\n\t%s\n",filename);
}

/* --------------------
 * CollisionTableCursor
 * --------------------
 * Used for iterating over collisions in the table.
 *
 * e.g.
 * { CollisionTableCursor *cursor = Alloc_CollisionTableCursor();
 *   while( CollisionTable_Next( table, cursor ) )
 *     ... operate on cursor results ...
 *   Free_CollisionTableCursor(cur);
 * }
 */

typedef struct _CollisionTableCursor
{ int p;
  unsigned int *hit;   // a 4-element array with id0, index0, id1, index1.
  int stride; // the stride between succesive elements of the hit array
} CollisionTableCursor;

CollisionTableCursor *Alloc_CollisionTableCursor(void)
{ CollisionTableCursor *this = Guarded_Malloc( sizeof(CollisionTableCursor), "Alloc_CollisionTableCursor" );
  this->p = 0;
  return this;
}

void Free_CollisionTableCursor(CollisionTableCursor* this)
{ if( this ) free( this );
}

// Returns cursor for table corresponding to the next pixel with 2 or more
// hits.
//
// Reentrant with same cursor.  Search will start at position indicated by
// cursor, so if hits aren't removed between calls it will always return the
// same hit.
int CollisionTable_Next( CollisionTable *this, CollisionTableCursor *cursor )
{ int p;
  int area = this->area;
  unsigned int *table = this->table;

  for( p = cursor->p; p<area; p++ )
    if( table[p] >= 2 )
    { cursor->p      = p;
      cursor->hit    = table + p + area;
      cursor->stride = area;
      return table[p];
    }
  return 0;
}

/*
 * Trace Overlap
 * -------------
 *
 * After an approximate intersection between two whiskers is found, the indices
 * bounding the total overlapping region must be computed.
 *
 * Returns a four element array of indices two (beg, end) pairs.
 * The returned array is a static result so it's only valid for one call.
 */

inline float _trace_overlap_dist( Whisker_Seg *wa, Whisker_Seg *wb, int ia, int ib )
{ float dx = wa->x[ia] - wb->x[ib],
        dy = wa->y[ia] - wb->y[ib];
  return dx*dx + dy*dy;
}

inline int _trace_overlap_bounds_check( Whisker_Seg *w, int i )
{ return i>=0 && i<w->len;
}

void _trace_overlap_one_side( Whisker_Seg *wa, Whisker_Seg *wb, int *ia, int *ib, int step, int sign, float thresh )
{ float d = 0.0;
  int ta = *ia, tb = *ib;
  int astep = sign*step,
      bstep =      step;
  // Crawl along segments in one direction until no further steps can
  // be made without exceeding the threshold distance.
#ifdef DEBUG_TRACE_OVERLAP_ONE_SIDE
    debug("Overlap - one side:\n"
          "  step: %d         \n"
          "  sign: %d         \n"
          "  ia: %3d  ib: %3d \n"
          "Moves:             \n",step,sign,ta,tb);
#endif
  while( d < thresh                            &&
         _trace_overlap_bounds_check( wa, ta + astep ) &&
         _trace_overlap_bounds_check( wb, tb + bstep ) )
  { int moves[6] = { ta + astep      , tb + bstep,  // + +
                     ta + astep      , tb        ,  // +
                     ta              , tb + bstep };//   +
    int argmin, i;

#ifdef DEBUG_TRACE_OVERLAP_ONE_SIDE
    debug("\t%3d  %3d   %f\n",ta,tb,d);
#endif

    // Find move with minimum distance
    d = FLT_MAX;
    for(i=0; i<3; i++)
    { float t = _trace_overlap_dist(wa,wb, moves[2*i], moves[2*i+1]);
      if( t < d )
      { argmin = i;
        d = t;
      }
    }
    if( d < thresh )
    { ta = moves[2*argmin];
      tb = moves[2*argmin+1];
    }
  }

  // If a boundary is hit, make sure the index on the other segment
  // can catch up.
  { float last = d;
    if( !_trace_overlap_bounds_check(wa,ta + astep) )        // hit a bound - optimize b
    { while( _trace_overlap_bounds_check(wb,tb + bstep) &&
             (d = _trace_overlap_dist(wa, wb, ta, tb + bstep)) < last )
      { tb += bstep;
        last = d;
#ifdef DEBUG_TRACE_OVERLAP_ONE_SIDE
        debug("\t%3d  %3d * %f\n",ta,tb,d);
#endif
      }  
    } else if( !_trace_overlap_bounds_check(wb,tb) )         // hit b bound - optimize a
    { while( _trace_overlap_bounds_check(wa,ta + astep) &&   
             (d = _trace_overlap_dist(wa, wb, ta + astep, tb)) < last )
      { ta += astep;
        last = d;
#ifdef DEBUG_TRACE_OVERLAP_ONE_SIDE
        debug("\t%3d  %3d * %f\n",ta,tb,d);
#endif
      }  
    }
  }
  *ia = ta; //update ia and ib;
  *ib = tb; 
}

int *Trace_Overlap( CollisionTableCursor *cursor, Whisker_Seg* wv, float thresh )
{ static int res[4] = {-1,-1,-1,-1};
  Whisker_Seg *wa = wv + cursor->hit[ 0                ],
              *wb = wv + cursor->hit[ 2*cursor->stride ];
  int ia = cursor->hit[   cursor->stride ], 
      ib = cursor->hit[ 3*cursor->stride ];
  int dax, day, dbx, dby;
  int sign;

  // determine relative direction of indexing
  // - have to be careful about edge cases
  if( ia == wa->len-1 || ib == wb->len-1 )
  { if( ia != 0 && ib != 0 )
    { dax = wa->x[ia-1] - wa->x[ia];
      day = wa->y[ia-1] - wa->y[ia];
      dbx = wb->x[ib-1] - wb->x[ib];
      dby = wb->y[ib-1] - wb->y[ib];
    } else if( ia == 0 )              
    { dax = wa->x[ia+1] - wa->x[ia];
      day = wa->y[ia+1] - wa->y[ia];
      dbx = - wb->x[ib-1] + wb->x[ib];
      dby = - wb->y[ib-1] + wb->y[ib];
    } else if( ib == 0 )              
    { dax = - wa->x[ia-1] + wa->x[ia];
      day = - wa->y[ia-1] + wa->y[ia];
      dbx = wb->x[ib+1] - wb->x[ib];
      dby = wb->y[ib+1] - wb->y[ib];
    }                                 
  } else {                            
    dax = wa->x[ia+1] - wa->x[ia];
    day = wa->y[ia+1] - wa->y[ia];
    dbx = wb->x[ib+1] - wb->x[ib];
    dby = wb->y[ib+1] - wb->y[ib];
  }
  sign = 1;
  if( abs(dax) > abs(day) )
  { if( dax*dbx < 0)
      sign = -1;
  } else if( day*dby < 0 )
    sign = -1;

  res[0] = ia;
  res[2] = ib;
  _trace_overlap_one_side( wa, wb, res,   res+2,  1, sign, thresh );
  res[1] = ia;
  res[3] = ib;
  _trace_overlap_one_side( wa, wb, res+1, res+3, -1, sign, thresh );
  if( res[0] > res[1] ) 
    SWAP( res[0], res[1] );
  if( res[2] > res[3] ) 
    SWAP( res[2], res[3] );

  return res;
}

int _cmp_whisker_seg_frame( const void *a, const void *b )
{ return ((Whisker_Seg*)a)->time - ((Whisker_Seg*)b)->time;
}

inline int _is_whisker_interval_significant( Whisker_Seg *w, int a, int b, float thresh )
{ return (b-a) >= (thresh * w->len);
}

inline float _whisker_seg_score_sum( Whisker_Seg *w )
{ float s  = 0,
       *sc = w->scores;
  int n = w->len;
  while(n--)
    s += sc[n];
  return s;
}

/* ===========================
 * Remove_Overlapping_Whiskers
 * ===========================
 *
 * Finds duplicate whisker segments and resolves these duplications by removing
 * the lower scoring duplicate.
 *
 * Removal consists of rearranging the array, in place, so the removed segments
 * are at the end.  This function returns the number of whiskers to keep.  This
 * is done because the whisker segment array is usually allocated as a block. 
 *
 * See TEST_COLLISIONTABLE_3 for an example.
 *
 * RETURN
 * ------
 *
 * Returns the number of kept whiskers.
 *
 * The whisker segment array, `wv`, is rearranged so the kept segments are
 * all at the beginning.  Whiskers also get sorted by time (ascending).
 *
 * ARGUMENTS
 * ---------
 *
 * wv    : Whisker_Seg* [NOTE: this function changes the order of elements]
 * wv_n  : int
 *
 *  An array of whisker segements `wv_n` long.
 *
 * scale : float
 *
 *  The ratio between the units (pixels) in segments and the pixels in the
 *  CollisionTable.  For example, if the scale is 2.0, whiskers traced from a
 *  400x300 px image will end up creating a 200x150 px CollisionTable.
 *
 * dist_thresh : float
 *
 *  Two points are considered "overlapping" if they are seperated by less than
 *  this distance.  Used in finding the intervals of overlapping segments.
 *
 * overlap_thresh : float
 *
 *  The fraction of a whisker segment that must be overlapping another segment
 *  to be considered a duplicate.
 *
 */
int Remove_Overlapping_Whiskers_Multi_Frame( Whisker_Seg *wv, int wv_n, float scale, float dist_thresh, float overlap_thresh )
{ int i,j, w, h;
  static uint8_t *keepers = NULL;
  static size_t keepers_size = 0;
  CollisionTable *table;
  
  qsort( wv, wv_n, sizeof(Whisker_Seg), &_cmp_whisker_seg_frame ); // dunno if this is strictly necessary
#ifdef DEBUG_REMOVE_OVERLAPPING_WHISKERS_MULTI_FRAME
  assert( wv[0].time < wv[wv_n-1].time );
#endif
  
  keepers = request_storage( keepers, &keepers_size, sizeof(uint8_t), wv_n, "Expand keepers" );
  memset(keepers,1, sizeof(uint8_t)*wv_n);

  Estimate_Image_Shape_From_Segments(wv, wv_n, &w, &h );
#ifdef DEBUG_REMOVE_OVERLAPPING_WHISKERS_MULTI_FRAME
  debug("Computing table for width: %3d and height: %3d\n",w,h);
#endif
  table = Alloc_CollisionTable( w, h, scale, 5 );
  
  for( i=0; i<wv_n; i++ )
  { int iframe = wv[i].time;
    Whisker_Seg *wvf = wv      + i;
    uint8_t *mask        = keepers + i;

    j = i;                         // find index of next frame
    while( j++ < wv_n )
      if( wv[j].time != iframe )    
        break;

    CollisionTable_Reset( table );
    CollisionTable_Add_Segments( table, wvf , j-i ); 

    { int n;
      int area = table->area;
      CollisionTableCursor cursor = {0,0};
      
      while( (n = CollisionTable_Next(table, &cursor)) )
      { int id2   = cursor.hit[2*area],
            id    = cursor.hit[0],
            index = cursor.hit[area],
            *ovlp;
#ifdef DEBUG_REMOVE_OVERLAPPING_WHISKERS_MULTI_FRAME
        assert(id    <  j-i);
        assert(index <  wvf[id].len);
#endif

        ovlp = Trace_Overlap( &cursor, wvf, dist_thresh );
#ifdef DEBUG_REMOVE_OVERLAPPING_WHISKERS_MULTI_FRAME
        assert( ovlp[0] <= ovlp[1] );
        assert( ovlp[2] <= ovlp[3] );
#endif

        if( _is_whisker_interval_significant( wvf+id , ovlp[0], ovlp[1], overlap_thresh ) ||
            _is_whisker_interval_significant( wvf+id2, ovlp[2], ovlp[3], overlap_thresh )  )
        { if( _whisker_seg_score_sum( wvf+id ) > _whisker_seg_score_sum( wvf+id2 ) )
          { // Keep id, remove id2
            mask[id2] = 0;
            CollisionTable_Remove_Segment( table, wvf+id2, id2 );
          } else 
          { // Keep id2, remove id
            mask[id ] = 0;
            CollisionTable_Remove_Segment( table, wvf+id, id );
          }
        } else
        { int x = cursor.p % table->stride,
              y = cursor.p / table->stride;
          CollisionTable_Remove( table, x, y, id );
        }
      }
      i = j-1;
    }
#ifdef DEBUG_REMOVE_OVERLAPPING_WHISKERS_MULTI_FRAME
    { int n = table->area;
      while(n--)
        assert( table->table[n] <= 1 );
    }
#endif
  }

  // Move keepers to beginning and others to end
  // stable resorting
  for(i=0, j=0; j<wv_n; ) // i is the destination, j is the source
    if( keepers[j] )
    { Whisker_Seg *a = wv + i++,
                  *b = wv + j++,
                  c  = *a;
      *a = *b;
      *b = c;
    }
    else
      j++;

#ifdef DEBUG_REMOVE_OVERLAPPING_WHISKERS_MULTI_FRAME
  debug("Found %d keepers.  Originally %d whiskers (removed %d).\n", i, wv_n, wv_n-i);
#endif

  Free_CollisionTable(table);
  return i;
}

int Remove_Overlapping_Whiskers_One_Frame( Whisker_Seg *wv, 
                                           int wv_n, 
                                           int w, 
                                           int h, 
                                           float scale, 
                                           float dist_thresh, 
                                           float overlap_thresh )
{ int i,j;
  static uint8_t *keepers = NULL;
  static size_t keepers_size = 0;
  static CollisionTable *table = NULL;
  static int area;
  int n;
  CollisionTableCursor cursor = {0,0};

  keepers = request_storage( keepers, &keepers_size, sizeof(uint8_t), wv_n, "Expand keepers" );
  memset(keepers,1, sizeof(uint8_t)*wv_n);

  if( !table )
  { table = Alloc_CollisionTable( w, h, scale, 5 );
    area= table->area;
  }
  CollisionTable_Reset( table );
  CollisionTable_Add_Segments( table, wv , wv_n ); 
    
  while( (n = CollisionTable_Next(table, &cursor)) )
  { int id2   = cursor.hit[2*area],
        id    = cursor.hit[0],
        index = cursor.hit[area],
        *ovlp;
#ifdef DEBUG_REMOVE_OVERLAPPING_WHISKERS_ONE_FRAME
    assert(id    <  wv_n);
    assert(index <  wv[id].len);
#endif

    ovlp = Trace_Overlap( &cursor, wv, dist_thresh );
#ifdef DEBUG_REMOVE_OVERLAPPING_WHISKERS_ONE_FRAME
    assert( ovlp[0] <= ovlp[1] );
    assert( ovlp[2] <= ovlp[3] );
#endif

    if( _is_whisker_interval_significant( wv+id , ovlp[0], ovlp[1], overlap_thresh ) ||
        _is_whisker_interval_significant( wv+id2, ovlp[2], ovlp[3], overlap_thresh )  )
    { if( _whisker_seg_score_sum( wv+id ) > _whisker_seg_score_sum( wv+id2 ) )
      { // Keep id, remove id2
        keepers[id2] = 0;
        CollisionTable_Remove_Segment( table, wv+id2, id2 );
      } else 
      { // Keep id2, remove id
        keepers[id ] = 0;
        CollisionTable_Remove_Segment( table, wv+id, id );
      }
    } else
    { int x = cursor.p % table->stride,
          y = cursor.p / table->stride;
      CollisionTable_Remove( table, x, y, id );
    }
  }
#ifdef DEBUG_REMOVE_OVERLAPPING_WHISKERS_ONE_FRAME
  { int n = table->area;
    while(n--)
      assert( table->table[n] <= 1 );
  }
#endif
  // Move keepers to beginning and others to end
  // stable resorting
  for(i=0, j=0; j<wv_n; ) // i is the destination, j is the source
    if( keepers[j] )
    { Whisker_Seg *a = wv + i++,
                  *b = wv + j++,
                  c  = *a;
      *a = *b;
      *b = c;
    }
    else
      j++;

#ifdef DEBUG_REMOVE_OVERLAPPING_WHISKERS_ONE_FRAME
  debug("Found %d keepers.  Originally %d whiskers (removed %d).\n", i, wv_n, wv_n-i);
#endif

  return i;
}

#ifdef TEST_COLLISIONTABLE_1
#include "whisker_io.h"
#include "utilities.h"
static char *Spec[] = {"<source:string> <dest:string>", NULL};
int main(int argc, char *argv[]) 
{ Whisker_Seg *wv;
  int wv_n;
  CollisionTable *table;
  int w,h;

  printf(
      "|-----------------------                                       \n"
      "| CollisionTable Test #1                                       \n"
      "|-----------------------                                       \n"
      "|                                                              \n"
      "| This test adds all the segments from the input whiskers file \n"
      "| to a single CollisionsTable, iterates over hits, and removes \n"
      "| the whiskers as they're hit.                                 \n"
      "|                                                              \n"
      "| On the way, a couple checks are made to ensure the data looks\n"
      "| ok.  This is mostly a stress test of a few functions.        \n"
      "|                                                              \n"
      "| Outputs the hit counts in the destination file in a raw      \n"
      "| format. See CollisionTable_Counts_To_File for details.       \n"
      "|--                                                            \n");
  Process_Arguments(argc,argv,Spec,0);                               
  wv = Load_Whiskers( Get_String_Arg("source"), NULL, &wv_n);
  
  Estimate_Image_Shape_From_Segments( wv, wv_n, &w, &h );
  debug("Computing table for width: %3d and height: %3d\n",w,h);
  table = Alloc_CollisionTable( w, h, 2, 5 );
  CollisionTable_Add_Segments( table, wv, wv_n ); 
  CollisionTable_Counts_To_File( table, Get_String_Arg("dest") );

  { int n, count=0;
    int area = table->area;
    CollisionTableCursor cursor = {0,0};
    while( n = CollisionTable_Next(table, &cursor) )
    { int id2, id = cursor.hit[2*n*area];
      int index = cursor.hit[(2*n+1)*area];
      int *ovlp;
      assert(id    <  wv_n);
      assert(index <  wv[id].len);
    //printf("(%3d,%3d) id: %5d\tindex: %5d\n", cursor.p % table->stride,
    //                                          cursor.p / table->stride,
    //                                          id,
    //                                          cursor.hit[(2*n+1)*area] );
      ovlp = Trace_Overlap( &cursor, wv, 2.0 );
      id  = cursor.hit[0];
      id2 = cursor.hit[2*area];
    //printf("Result Overlap: id: %5d\t%3d to %3d\n"
    //       "                id: %5d\t%3d to %3d\n", id , ovlp[0], ovlp[1], 
    //                                                id2, ovlp[2], ovlp[3] );
      assert( ovlp[0] <= ovlp[1] );
      assert( ovlp[2] <= ovlp[3] );
      CollisionTable_Remove_Segment( table, wv+id, id );
      CollisionTable_Remove_Segment( table, wv+id2, id2 );
      count++;
    }
    printf("Called Next/Remove %d times.\n",count);
  }
  { int n = table->area;
    while(n--)
      assert( table->table[n] <= 1 );
  }

  Free_Whisker_Seg_Vec(wv,wv_n);
  Free_CollisionTable(table);

  return 0;
}
#endif

#ifdef TEST_COLLISIONTABLE_2
#include "whisker_io.h"
#include "utilities.h"


static char *Spec[] = {"<source:string> <dest:string>", NULL};
int main(int argc, char *argv[])
{ int i, wv_n, w, h, count;
  Whisker_Seg *wv;
  CollisionTable *table;
  unsigned int *stack;
  FILE *fp;

  printf(
      "|-----------------------                                                \n"
      "| CollisionTable Test #2                                                \n"
      "|-----------------------                                                \n"
      "|                                                                       \n"
      "| This test simulates the merge without actually manipulating whiskers. \n"
      "| Whiskers are added to a CollisionTable frame by frame.  One           \n"
      "| CollisionTable is allocated and re-used. Counts from each frame are   \n"
      "| written to a raw stack output. After identifyng overlapping whiskers  \n"
      "| the interval of overlap is computed and used to determine which       \n"
      "| whisker to remove.                                                    \n"
      "|                                                                       \n"
      "|--                                                                     \n");
  Process_Arguments(argc,argv,Spec,0);
  wv = Load_Whiskers( Get_String_Arg("source"), NULL, &wv_n);

  fp = fopen( Get_String_Arg("dest"), "wb" );
  if(!fp) error("Couldn't open destination file %s", Get_String_Arg("dest"));

  Estimate_Image_Shape_From_Segments( wv, wv_n, &w, &h );
  debug("Computing table for width: %3d and height: %3d\n",w,h);
  table = Alloc_CollisionTable( w, h, 2, 5 );

  qsort( wv, wv_n, sizeof(Whisker_Seg), &_cmp_whisker_seg_frame ); // dunno if this is strictly necessary
  assert( wv[0].time < wv[wv_n-1].time );
  
  count = 0;
  for( i=0; i<wv_n; i++ )
  { int j = i;
    int iframe = wv[i].time;
    Whisker_Seg *wvf = wv      + i;
    while( j++ < wv_n )
      if( wv[j].time != iframe )    
        break;

    CollisionTable_Reset( table );
    CollisionTable_Add_Segments( table, wvf , j-i ); 

    { int n;
      int area = table->area;
      CollisionTableCursor cursor = {0,0};
      
      while( (n = CollisionTable_Next(table, &cursor)) )
      { int id2 = cursor.hit[2*area],
            id  = cursor.hit[0];
        int index = cursor.hit[area];
        int *ovlp;
        assert(id    <  j-i);
        assert(index <  wvf[id].len);

        ovlp = Trace_Overlap( &cursor, wvf, 2.0 );
        assert( ovlp[0] <= ovlp[1] );
        assert( ovlp[2] <= ovlp[3] );

        if( _is_whisker_interval_significant( wvf+id , ovlp[0], ovlp[1], 0.8 ) ||
            _is_whisker_interval_significant( wvf+id2, ovlp[2], ovlp[3], 0.8 )  )
        { if( _whisker_seg_score_sum( wvf+id ) > _whisker_seg_score_sum( wvf+id2 ) )
          { CollisionTable_Remove_Segment( table, wvf+id2, id2 ); // Keep id, remove id2
          } else 
          { CollisionTable_Remove_Segment( table, wvf+id, id );   // Keep id2, remove id
          }
        } else
        { int x = cursor.p % table->stride,
              y = cursor.p / table->stride;
          CollisionTable_Remove( table, x, y, id );
        }
      count++;
      assert( count < table->area );
      }
      i = j-1;
      fwrite( table->table, 
              table->area , 
              sizeof(unsigned int), 
              fp );
    }
    { int n = table->area;
      while(n--)
        assert( table->table[n] <= 1 );
    }
  }
  debug("Called Next %d times.\n",count);

  fclose(fp);
  printf("Stack dimensions: %3d x %3d x %3d\n", wv[wv_n-1].time+1, table->area/table->stride, table->stride);
  Free_Whisker_Seg_Vec(wv,wv_n);
  Free_CollisionTable(table);

  return 0;
}
#endif

#ifdef TEST_COLLISIONTABLE_3
#include "whisker_io.h"
#include "utilities.h"

static char *Spec[] = {"<source:string> <dest:string>", NULL};
int main(int argc, char *argv[])
{ int i, wv_n;
  Whisker_Seg *wv;

  printf(
      "------------------------                                                \n"
      "| CollisionTable Test #3                                                \n"
      "------------------------                                                \n"
      "|                                                                       \n"
      "| This is the real deal.  Loads a whisker's file, and frame-by-frame    \n"
      "| searches for overlaps using the CollisionTable.  Some whiskers are    \n"
      "| removed and the good ones are output to the destination file.         \n"
      "|                                                                       \n"
      "--                                                                      \n");
  Process_Arguments                 ( argc,argv,Spec,0);

  wv = Load_Whiskers                ( Get_String_Arg("source"), NULL, &wv_n);
  i  = Remove_Overlapping_Whiskers_Multi_Frame  ( wv, wv_n, 2.0, 2.0, 0.8 );
  Save_Whiskers                     ( Get_String_Arg("dest"), NULL, wv, i );

  debug("Wrote out %d keepers.  Originally %d whiskers (removed %d).\n", i, wv_n, wv_n-i);
  Free_Whisker_Seg_Vec(wv,wv_n);
  return 0;
}
#endif
