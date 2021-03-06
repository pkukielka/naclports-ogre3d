## About naclports-ogre3d

Naclports-ogre3d is an effort to make Ogre3d part of naclports.
It compile it you should set those environment flags:
```
export NACL_GLIBC=1
export NACL_SDK_ROOT=/path_to_your_nacl_sdk/pepper_2x
```
I was testing it with pepper_23. 
This port have few limitations like no cg plugin, disabled samples
or GLES2 rendering system instead of GL (look at cmake flags for details)
but it should be good enough to get you started with Ogre and NaCL.

## External Native Client SDK Packages

The libraries directory contains bash scripts and patch files to build
common libraries for Native Client. The bash scripts will download, patch
build and copy the binary library and developer header files into your
Native Client SDK.

The scripts require that centain tools are present in the host system.
You will need at least these (but probably more):
- autoconf, automake
- pkg-config
- gettext
- libglib2.0-dev >= 2.26.0 (if you want to build nacl-glib)

Before you can use the Makefile in this directory, you must set the
NACL_SDK_ROOT environment variable to top directory of a version of the
Native Client SDK (the directory containing toolchain/).
This path should be absolute.

The Makefile can build all of the libraries. Package dependencies are 
built into the Makefile. For example, 'make vorbis' will build
libvorbis-1.2.3 and libogg-1.1.4.  'make all' will build all of the libraries.

Headers and libraries are installed where nacl-gcc and nacl-g++ will
be able to automatically find them without having to add extra -I or -L
options.  (Currently, these scripts will generate a gcc "specs" file
to add the required extra paths.)

The source code and build output for each package is placed in:

  naclports/src/out/repository          for 32-bit builds
  naclports/src/out/repository64        for 64-bit builds

NOTE:  These external libraries each have their own licenses for use.
Please read and understand these licenses before using these packages
in your projects.

NOTE to Windows users:  These scripts are written in bash and must be
launched from a Cygwin shell.

To add a package:
1. Make sure you have a writable version of the repository
    gclient config https://naclports.googlecode.com/svn/trunk/src
2. Add a directory to the libraries directory using the name and version of
    your new package.  For example, nacl-esidl-0.1.5
3. Add the build script to that directory.
4. Optionally build a tarball.  If you choose to do this, you will need to
    create a checksum for it using naclports/src/build_tools/sha1sum.py.  Redirect 
    the script output to a .sha1 file so that the common.sh script can pick it up.  E.g.:
    python scripts/sha1sum.py mytarball.zip > scripts/nacl-esidl-0.1.5/nacl-esidl-0.1.5.sha1
5. Add the invocation of your script to the Makefile

