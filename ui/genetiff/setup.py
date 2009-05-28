from distutils.core import setup,Extension
import os


setup( name = "genetiff",
       version = "0.02",
       description = "Numpy interface to Gene's TIFF library",
       author = ['Nathan Clack','Eugene Myers'],
       author_email = 'clackn@janelia.hhmi.org',
       url = 'http://www.hhmi.org/janelia/',
       package_dir = {'genetiff':''},
       packages = ['genetiff'],
       );
