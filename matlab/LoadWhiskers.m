function [whiskers,format] = LoadWhiskers(filename)
%LoadWhiskers     Reads .whisker files.
%
%   USAGE:
%
%   whiskers          = LoadWhiskers(filename)
%   [whiskers,format] = LoadWhiskers(filename)
%
%   <whiskers> is a struct array with a number of fields describing the shape of traced whisker-like segments.
%   <format>   is a string describing how the whisker data is stored.
%
%   EXAMPLES:
%
%     >> w = LoadWhiskers('/Users/clackn/Desktop/whisker-mp4/whisker_data_0250.whiskers') 
%                                                                                        
%     w =                                                                                 
%                                                                                        
%     27771x1 struct array with fields:                                                   
%         id                                                                              
%         time                                                                            
%         x                                                                               
%         y                                                                               
%         thick                                                                           
%         scores                                                                          
%
%   If the file doesn't exist or something goes wrong, you'll get a bunch of warnings:
%
%     >> LoadWhiskers('does-not-exist.whiskers')                                                              
%     ??? Error using ==> LoadWhiskers                                                    
%     Could not load whiskers. 
%
%  Author: Nathan Clack <clackn@janelia.hhmi.org>
%  Copyright 2010 Howard Hughes Medical Institute.
%  All rights reserved.
%  Use is subject to Janelia Farm Research Campus Software Copyright 1.1
%  license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).

[s,attr,id] = fileattrib(filename); % get abs path
if(s)
  [whiskers,format] = mexLoadWhiskers(attr.Name);
else
  error(attr)
end
