def roc(score,predicate):
  """ roc(score,predicate) -> (precision,recall,threshold)
  """
  s = score.ravel()
  p = predicate.ravel()

  map = list( reversed(s.argsort()) )#ascending
  T   = s[map]
  P   = p[map]+0.0; #implict conversion to double, avoids having to import numpy

  TP  = P.cumsum()     # count positives at each threshold
  FN  = (1-P).cumsum() # count negatives at each threshold
  FP  = TP[-1] - TP;   # total positive less the true positives
  #TN  = FN[-1] - FN;
  return TP/(TP+FP), TP/(TP+FN), T

