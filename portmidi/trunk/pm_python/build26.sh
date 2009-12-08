# compile pypm.c -- pretty straightforward but it needs to include
# Python.h, so notice that the -I(nclude) directory that points to 
# my copy of Python-2.6.4. Python's make install copies Python.h to
# /usr/local/include/python2.6/Python.h, but your system may behave
# differently
gcc -fPIC -c -o pypm.o -I../pm_common -I../porttime -I$HOME/Python-2.6.4/Include -I$HOME/Python-2.6.4 pypm.c
# This links pypm.so. It assumes you have compiled libportmidi.so in ../Release
gcc -shared -o pypm.so  pypm.o -lportmidi -L../Release
# Install the pypm.so extension. The location should be the "site-packages"
# folder for the version of Python you are trying to extend
sudo cp pypm.so /usr/local/lib/python2.6/site-packages
# My linux had an old libportmidi.so in /usr/lib. I "hide" it by
# renaming it (in case this breaks some software). Meanwhile, I have
# LD_LIBRARY_PATH set to /usr/local/lib, where a new libportmidi.so is
# installed
sudo mv /usr/lib/libportmidi.so /usr/lib/libportmidi.so.ORIG

