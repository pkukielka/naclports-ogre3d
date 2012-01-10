#!/bin/bash
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# nacl-ffmpeg-0.5.sh
#
# usage:  nacl-ffmpeg-0.5.sh
#
# this script downloads, patches, and builds ffmpeg for Native Client 
#

readonly URL=http://commondatastorage.googleapis.com/nativeclient-mirror/nacl/ffmpeg-0.5.tar.bz2
#readonly URL=http://ffmpeg.org/releases/ffmpeg-0.5.tar.bz2
readonly PATCH_FILE=nacl-ffmpeg-0.5.patch
readonly PACKAGE_NAME=ffmpeg-0.5

source ../../build_tools/common.sh


CustomConfigureStep() {
  export PKG_CONFIG_PATH=${NACL_SDK_USR_LIB}/pkgconfig
  export PKG_CONFIG_LIBDIR=${NACL_SDK_USR_LIB}
  export PATH=${NACL_BIN_PATH}:${PATH};
  MakeDir ${NACL_PACKAGES_REPOSITORY}/${PACKAGE_NAME}/${PACKAGE_NAME}-build
  ChangeDir ${NACL_PACKAGES_REPOSITORY}/${PACKAGE_NAME}/${PACKAGE_NAME}-build
  ../configure \
    --cross-prefix=${NACL_CROSS_PREFIX}- \
    --target-os=linux \
    --enable-gpl \
    --enable-static \
    --enable-cross-compile \
    --disable-shared \
    --disable-ssse3 \
    --disable-mmx \
    --disable-mmx2 \
    --disable-amd3dnow \
    --disable-amd3dnowext \
    --disable-indevs \
    --disable-protocols \
    --disable-network \
    --enable-protocol=file \
    --enable-libmp3lame \
    --enable-libvorbis \
    --enable-libtheora \
    --disable-ffmpeg \
    --disable-ffplay \
    --disable-ffserver \
    --disable-demuxer=rtsp \
    --disable-demuxer=image2 \
    --prefix=${NACL_SDK_USR} \
    --libdir=${NACL_SDK_USR_LIB}
}


CustomPostConfigureStep() {
  touch strings.h
  echo "#define strcasecmp stricmp" >> config.h
}


CustomBuildAndInstallStep() {
  DefaultBuildStep
  make install
  DefaultTouchStep
}


CustomPackageInstall() {
  DefaultPreInstallStep
  DefaultDownloadBzipStep
  DefaultExtractBzipStep
  DefaultPatchStep
  CustomConfigureStep
  CustomPostConfigureStep
  CustomBuildAndInstallStep
  DefaultCleanUpStep
}


CustomPackageInstall
exit 0
