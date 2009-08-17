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
 * NOTE: This is unused right now
 */

#include "compat.h"
#include "common.h"
#include "utilities.h"
#include "error.h"
#include <string.h>
#include <assert.h>
#include "deque.h"

#if 0
#define TEST_DEQUE_1

#define WARNING_POP_FROM_EMPTY
#endif

SHARED_EXPORT
Deque *Deque_Alloc(int size_hint)
{ Deque *self = Guarded_Malloc( sizeof(Deque), "Deque_Alloc 1" );
  size_hint = ( size_hint>1 ) ? size_hint : 2; // size must by >= 2
  self->store = Guarded_Malloc( sizeof(void*)*size_hint, "Deque_Alloc 2");
  self->size_bytes  = size_hint*sizeof(void*);
  self->front = size_hint/2-1; //when empty front points just behind back
  self->back  = size_hint/2;
  return self;
}

SHARED_EXPORT
void Deque_Free(Deque *self)
{ if(self)
  { if(self->store) free(self->store);
    free(self);
  }
}

SHARED_EXPORT 
inline void Deque_Reset ( Deque *self )
{ int n = self->size_bytes/sizeof(void*)/2;
  memset( self->store, 0, self->size_bytes );
  self->front = n-1;
  self->back = 0;
}

SHARED_EXPORT
void Deque_Squeeze(Deque *self)
{ size_t f = self->front,
         b = self->back;
  size_t n =  f - b + 1;
  self->size_bytes  = (n+2)*sizeof(void*);
  memmove(self->store+1,self->store + b, self->size_bytes);
  self->back  = 1;
  self->front = self->back + n - 1;
  self->store = Guarded_Realloc( self->store, self->size_bytes , "Deque_Squeeze" );
}

SHARED_EXPORT
inline void *Deque_Front(Deque *self)
{ return self->store[self->front];
}

SHARED_EXPORT
inline void *Deque_Back(Deque *self)
{ return self->store[self->back];
}

SHARED_EXPORT
inline int  Deque_Is_Empty(Deque *self)
{ return (self->front == self->back - 1);
}

SHARED_EXPORT
inline Deque *_Deque_Push_Front_Unsafe( Deque* self, void *item )
{ self->store[++self->front] = item;
  return self;
}

SHARED_EXPORT
inline Deque *Deque_Push_Front( Deque* self, void *item )
{ size_t s = self->size_bytes/sizeof(void*);
  if( self->front >= s - 2 )
    self->store = request_storage( self->store, &self->size_bytes, sizeof(void*), s+1, "Deque: realloc on push front" );
  return _Deque_Push_Front_Unsafe( self, item );
}

SHARED_EXPORT
inline void *_Deque_Pop_Front_Unsafe( Deque* self)
{ return self->store[self->front--];
}

SHARED_EXPORT
inline void *Deque_Pop_Front( Deque *self )
{ if( self->front >= self->back )
    return _Deque_Pop_Front_Unsafe(self);
#ifdef WARNING_POP_FROM_EMPTY
  else
    warning("Pop front requested from empty Deque\n");
#endif
  return NULL;
}

SHARED_EXPORT
inline Deque *_Deque_Push_Back_Unsafe( Deque* self, void *item )
{ self->store[--self->back] = item;
  return self;
}

SHARED_EXPORT
inline Deque *Deque_Push_Back( Deque *self, void *item )
{ if( self->back <= 1 )  // then, need to move things
  { size_t f = self->front,
           b = self->back,
           s = self->size_bytes/sizeof(void*),
           rem = s-f-2,
           n = f-b+1;
    // 1. Can we just move the store's active area? If not, then resize.
    if( !rem ) 
      self->store = request_storage( self->store, &self->size_bytes, sizeof(void*), s+1, "Deque: realloc on push back" );
    // 2. Move active area of store to end
    s = self->size_bytes/sizeof(void*);
    rem = s-f-2;
    n = f-b+1;
    memmove( self->store+s-n-2, self->store+b-1, sizeof(void*)*(n+2) );
    self->back  = b + rem;
    self->front = f + rem;
  }
  return _Deque_Push_Back_Unsafe(self,item);
}

SHARED_EXPORT
inline void *_Deque_Pop_Back_Unsafe( Deque* self)
{ return self->store[self->back++];
}

SHARED_EXPORT
inline void *Deque_Pop_Back( Deque *self )
{ if( self->front >= self->back )
    return _Deque_Pop_Back_Unsafe(self);
#ifdef WARNING_POP_FROM_EMPTY
  else
    warning("Pop back requested from empty Deque\n");
#endif
  return NULL;
}

#ifdef TEST_DEQUE_1

int main(int argc, char* argv[])
{ 
  
  progress("Deque: test alloc and hinting\n");
  { int i=10;
    while(i--)
    { Deque *d = Deque_Alloc( i );
      Deque_Free(d);
    }
  }

  progress("Deque: test push front\n");
  { int i = 10000000,
        j = 0,
       *p = NULL;
    Deque *d = Deque_Alloc( i );

    progress("\tPushing %d items on front.\n",i);
    while(i--) Deque_Push_Front( d, &i );
    Deque_Squeeze(d);
    while( (p=Deque_Pop_Front(d)) ) 
    { assert(*p=i);
      j++;
    }
    progress("\t%d Items popped from front.\n",j);
    assert( d->front == d->back - 1 );


    i=20000000, j=0;
    progress("\tPushing %d items on front.\n",i);
    while(i--) Deque_Push_Front( d, &i );
    while( (p=Deque_Pop_Back(d)) )
    { assert(*p=i);
      j++;
    }
    progress("\t%d Items popped from back.\n",j);
    assert( d->front == d->back - 1 );
    Deque_Free(d);
  }
  
  progress("Deque: test push back\n");
  { int i = 10000000,
        j = 0,
       *p = NULL;
    Deque *d = Deque_Alloc( i );

    progress("\tPushing %d items on back.\n",i);
    while(i--) Deque_Push_Back( d, &i );
    Deque_Squeeze(d);
    while( (p=Deque_Pop_Front(d)) )
    { assert(*p=i);
      j++;
    }
    progress("\t%d Items popped from front.\n",j);
    assert( d->front == d->back - 1 );


    i=20000000, j=0;
    progress("\tPushing %d items on back.\n",i);
    while(i--) Deque_Push_Back( d, &i );
    while( (p=Deque_Pop_Back(d)) )
    { assert(*p=i);
      j++;
    }
    progress("\t%d Items popped from back.\n",j);
    assert( d->front == d->back - 1 );
    Deque_Free(d);
  }
  
  progress("Deque: test push mixed\n");
  { int i = 10000000,
        j = 0,
       *p = NULL;
    Deque *d = Deque_Alloc( i );

    progress("\tPushing %d items on (mixed).\n",i);
    while(i--) 
      if( i&1 )
        Deque_Push_Back( d, &i );
      else
        Deque_Push_Front( d, &i );
    Deque_Squeeze(d);
    while( (p=Deque_Pop_Front(d)) )
    { assert(*p=i);
      j++;
    }
    progress("\t%d Items popped from front.\n",j);
    assert( d->front == d->back - 1 );


    i=20000000, j=0;
    progress("\tPushing %d items on (mixed).\n",i);
    while(i--) 
      if( i&1 )
        Deque_Push_Back( d, &i );
      else
        Deque_Push_Front( d, &i );
    while( (p=Deque_Pop_Back(d)) )
    { assert(*p=i);
      j++;
    }
    progress("\t%d Items popped from back.\n",j);
    assert( d->front == d->back - 1 );

    Deque_Free(d);
  }
}

#endif
