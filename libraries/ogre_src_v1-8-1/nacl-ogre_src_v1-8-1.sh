#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# nacl-ogre-1.8.1.sh
#
# usage:  nacl-ogre-1.8.1.sh
#
# this script downloads, patches, and builds OpenAL for Native Client
#

readonly URL=http://bitly.com/SEGjdI
readonly PATCH_FILE=nacl-ogre_src_v1-8-1.patch
readonly PACKAGE_NAME=ogre_src_v1-8-1

source ../../build_tools/common.sh

CustomConfigureStep() {
  Banner "Configuring ${PACKAGE_NAME}"

  MakeDir ${NACL_PACKAGES_REPOSITORY}/${PACKAGE_NAME}/build
  ChangeDir ${NACL_PACKAGES_REPOSITORY}/${PACKAGE_NAME}/build
  cmake .. -DCMAKE_TOOLCHAIN_FILE=../XCompile-nacl.txt \
           -DNACLCC=${NACLCC} \
           -DNACLCXX=${NACLCXX} \
           -DNACLAR=${NACLAR} \
           -DNACL_CROSS_PREFIX=${NACL_CROSS_PREFIX} \
           -DCMAKE_INSTALL_PREFIX=${NACL_SDK_USR} \
           -DOGRE_DEPENDENCIES_DIR=${NACL_SDK_USR} \
           -DOGRE_STATIC=true \
           -DOGRE_BUILD_RENDERSYSTEM_GL=false \
           -DOGRE_BUILD_RENDERSYSTEM_GLES2=true \
           -DOGRE_BUILD_PLATFORM_NACL=true \
           -DOGRE_BUILD_TOOLS=false \
           -DOGRE_BUILD_RENDERSYSTEM_D3D9=false \
           -DOGRE_CONFIG_ALLOCATOR=1 \
           -DOGRE_USE_BOOST=false \
           -DOGRE_CONFIG_THREADS=0 \
           -DOGRE_CONFIG_ENABLE_ZIP=false \
           -DOGRE_BUILD_PLUGIN_CG=false \
           -DOGRE_BUILD_SAMPLES=false
}

CustomPackageInstall() {
  DefaultPreInstallStep
  DefaultDownloadBzipStep
  DefaultExtractBzipStep
  DefaultPatchStep
  CustomConfigureStep
  DefaultBuildStep
  DefaultInstallStep
  DefaultCleanUpStep
}

CustomPackageInstall
exit 0

