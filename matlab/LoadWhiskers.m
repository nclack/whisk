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

[whiskers,format] = mexLoadWhiskers(filename);