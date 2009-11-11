from distutils.core import setup
from distutils.extension import Extension
from Pyrex.Distutils import build_ext
import sys

name = "pyPortMidi"
description="Python Wrappings for PortMidi",
version="0.0.3"
url = 'http://sound.media.mit.edu/~harrison/pyportmidi/',
long_description = '''
pyPortMidi: support streaming realtime audio from Python
            using the cross-platform PortMidi C library
            '''
author = 'John Harrison'
author_email = 'harrison@media.mit.edu'
cmdclass = {'build_ext': build_ext}
scripts = ['test.py','README.TXT']

if sys.platform == 'win32':
    print "Found Win32 platform"
    setup(
        name = name,
        description=description,
        version=version,
        url = url,
        long_description = long_description,
        author = author,
        author_email = author_email,
        cmdclass = cmdclass,
        scripts = scripts,
        ext_modules=[ 
        Extension("pypm", ["pypm.pyx"],
                  library_dirs = ["../Release"],
                  libraries = ["portmidi", "winmm"],
                  include_dirs = ["../porttime"],
#                  define_macros = [("_WIN32_", None)]) # needed by portmidi.h
                  extra_compile_args = ["/DWIN32"]) # needed by portmidi.h
        ]
)
elif sys.platform == 'darwin':
    print "Found darwin (OS X) platform"
    setup(
        name = name,
        description=description,
        version=version,
        url = url,
        long_description = long_description,
        author = author,
        author_email = author_email,
        cmdclass = cmdclass,
        scripts = scripts,
        ext_modules=[ 
        Extension("pypm", ["pypm.pyx"],
                  library_dirs=["./OSX"],
                  libraries = ["portmidi"],
                  extra_link_args=["-framework", "CoreFoundation",
                                   "-framework", "CoreMIDI",
                                   "-framework", "CoreAudio"])
        ]
    )
else:
    print "Assuming Linux platform"
    setup(
        name = name,
        description=description,
        version=version,
        url = url,
        long_description = long_description,
        author = author,
        author_email = author_email,
        cmdclass = cmdclass,
        scripts = scripts,
        ext_modules=[ 
        Extension("pypm", ["pypm.pyx"],
                  library_dirs=["./linux"],
                  libraries = ["portmidi", "asound", "pthread"]
                  )
        ]
       
    )
