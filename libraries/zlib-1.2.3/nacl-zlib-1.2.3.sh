#!/bin/bash
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# nacl-zlib-1.2.3.sh
#
# usage:  nacl-zlib-1.2.3.sh
#
# this script downloads, patches, and builds zlib for Native Client 
#

readonly URL=http://commondatastorage.googleapis.com/nativeclient-mirror/nacl/zlib-1.2.3.tar.gz
#readonly URL=http://www.zlib.net/zlib-1.2.3.tar.gz
readonly PATCH_FILE=
readonly PACKAGE_NAME=zlib-1.2.3

source ../../build_tools/common.sh


CustomConfigureStep() {
  Banner "Configuring ${PACKAGE_NAME}"
  ChangeDir ${NACL_PACKAGES_REPOSITORY}/${PACKAGE_NAME}
  # TODO: side-by-side install
  CC=${NACLCC} AR="${NACLAR} -r" RANLIB=${NACLRANLIB} CFLAGS="-Dunlink=puts" ./configure\
     --prefix=${NACL_SDK_USR}
}


CustomPackageInstall() {
  DefaultPreInstallStep
  DefaultDownloadStep
  DefaultExtractStep
  # zlib doesn't need patching, so no patch step
  CustomConfigureStep
  DefaultBuildStep
  DefaultInstallStep
  DefaultCleanUpStep
}


CustomPackageInstall
exit 0
