from numpy import *
"""
/*** Problem 2 ***********************************************************/
/* Source:
 * http://www.comp.leeds.ac.uk/roger/HiddenMarkovModels/html_dev/viterbi_algorithm/s3_pg4.html
 */
"""
states = ["sunny",
          "cloudy",
          "rainy" ]

SUNNY =0
CLOUDY=1
RAINY =2
NSTATE=3

observables = [ 
  "dry",
  "dryish",
  "damp",
  "soggy"]

DRY   =0
DRYISH=1
DAMP  =2
SOGGY =3
NOBS  =4

sprob = array(   [ 0.63, 0.17, 0.20])                                    
tprob = array(   [[0.5  , 0.25 , 0.25 ],## src = rows, dest = cols       
                  [0.375, 0.125, 0.375],                                 
                  [0.125, 0.675, 0.375]])                                
eprob = array(   [[0.60, 0.20, 0.15, 0.05],## state = rows, obs = cols   
                  [0.25, 0.25, 0.25, 0.25],                     
                  [0.05, 0.10, 0.35, 0.50]])                    
sequence = [ DRY, 
             DAMP, 
             SOGGY, 
             DRY, 
             DAMP, 
             SOGGY ]; ## should give sunny -> rainy -> rainy -> cloudy -> rainy -> rainy with vprob 2.5754047e-5
expected = 2.5754047e-5;

from trace import viterbi_log2
t,p,labels = viterbi_log2( sequence, log2(sprob), log2(tprob), log2(eprob), do_checks = True )
print 2**t,2**p
for e in labels:
  print states[e]
