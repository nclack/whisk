/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
/**
 * DEQUE 
 *
 * Implimented as a resizable array.
 * Holds pointers to objects.
 *
 * TODO: Impliment as resizable circular buffer
 *       + front and back operations are symmetric
 *       + If the size of the active area of the store remains constant
 *          never reallocs.
 *
 */

#ifndef H_DEQUE
#define H_DEQUE

typedef struct _Deque
{ void **store;
  size_t front;  //offsets in store wrt base store pointer, so store can be realloced
  size_t back;  
  size_t size_bytes;
} Deque;

SHARED_EXPORT   Deque *Deque_Alloc      ( int size_hint );
SHARED_EXPORT   void   Deque_Free       ( Deque *self );
SHARED_EXPORT   void   Deque_Reset      ( Deque *self );
SHARED_EXPORT   void   Deque_Squeeze    ( Deque *self );
SHARED_EXPORT   void  *Deque_Front      ( Deque *self);
SHARED_EXPORT   int    Deque_Is_Empty   ( Deque *self);
SHARED_EXPORT   void  *Deque_Back       ( Deque *self);
SHARED_EXPORT   Deque *Deque_Push_Front ( Deque *self, void *item );
SHARED_EXPORT   void  *Deque_Pop_Front  ( Deque *self );
SHARED_EXPORT   Deque *Deque_Push_Back  ( Deque *self, void *item );
SHARED_EXPORT   void  *Deque_Pop_Back   ( Deque *self );

#endif
