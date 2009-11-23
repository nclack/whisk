#ifndef H_HMM_RECLASSIFY
#define H_HMM_RECLASSIFY

typedef struct _measurements_reference
{ Measurements  *frame;     // points to head of a set of whisker segmens (e.g. those in a frame)
  Measurements **whiskers;  // map: whisker id's --> whisker segments
  int nframe;               // the number of whisker segements(rows) in the frame
  int nwhiskers;            // the number of whisker id's in the domain of the map.
} Measurements_Reference;


#endif  //H_HMM_RECLASSIFY 



