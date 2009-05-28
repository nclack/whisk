from distutils.core import setup,Extension
import os

setup( name = "whiskui",
       version = "0.01",
       description = "Browser and annotater for whisker tracking data",
       author = 'Nathan Clack',
       author_email = 'clackn@janelia.hhmi.org',
       url = 'http://www.hhmi.org/janelia/',
       package_dir = {'whisk.ui':''},
       packages = [ 'whisk.ui',
                    'whisk.ui.genetiff',
                    'whisk.ui.whiskerdata'],
       ext_modules = [genetiff] );
