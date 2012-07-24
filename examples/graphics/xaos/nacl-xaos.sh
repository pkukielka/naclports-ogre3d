#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# nacl-xaos.sh
#
# usage:  nacl-xoas.sh
#
# this script downloads, patches, and builds xaos for Native Client.
#

readonly URL=http://downloads.sourceforge.net/xaos/xaos-3.5.tar.gz
readonly PATCH_FILE=nacl_xaos-3.5.patch
PACKAGE_NAME=xaos-3.5

source ../../../build_tools/common.sh

EXAMPLE_DIR=${NACL_SRC}/examples/graphics/xaos

CustomPatchStep() {
  local the_patch="${START_DIR}/${PATCH_FILE}"
  local src_dir="${NACL_PACKAGES_REPOSITORY}/${PACKAGE_NAME}"
  Banner "Patching ${PACKAGE_NAME}"
  echo "Patch: ${the_patch}"
  echo "Src dir: ${src_dir}"
  cd ${src_dir}
  patch -p1 -g0 < ${the_patch}
  echo "copy nacl driver"
  cp -r "${START_DIR}/nacl-ui-driver" "src/ui/ui-drv/nacl"
}

CustomConfigureStep() {
  Banner "Configuring ${PACKAGE_NAME}"

  # export the nacl tools
  export CC=${NACLCC}
  export CXX=${NACLCXX}
  # NOTE: non-standard flag NACL_LDFLAGS because of some more hacking below
  export CFLAGS="-g -D__NO_MATH_INLINES=1"
  export LDFLAGS="-Wl,--undefined=PPP_GetInterface \
                  -Wl,--undefined=PPP_ShutdownModule \
                  -Wl,--undefined=PPP_InitializeModule \
                  -Wl,--undefined=original_main"
  if [ ${NACL_PACKAGES_BITSIZE} == "pnacl" ] ; then
    export CFLAGS="${CFLAGS} -O3"
    export LDFLAGS="${LDFLAGS} -O0 -static"
  else
     export CFLAGS="${CFLAGS} -O2"
  fi
  export AR=${NACLAR}
  export RANLIB=${NACLRANLIB}
  export PKG_CONFIG_PATH=${NACL_SDK_USR_LIB}/pkgconfig
  export PKG_CONFIG_LIBDIR=${NACL_SDK_USR_LIB}

  export LIBS="-L${NACL_SDK_USR_LIB} -lppapi -lpthread -lstdc++ -lm -lnosys"

  CONFIG_FLAGS="--with-png=no \
      --with-long-double=no \
      --host=nacl \
      --with-x11-driver=no \
      --with-sffe=no"

  ChangeDir ${NACL_PACKAGES_REPOSITORY}/${PACKAGE_NAME}

  # xaos does not work with a build dir which is separate from the
  # src dir - so we copy src -> build
  Remove ${PACKAGE_NAME}-build
  local tmp=${NACL_PACKAGES_REPOSITORY}/${PACKAGE_NAME}.tmp
  Remove ${tmp}
  cp -r ${NACL_PACKAGES_REPOSITORY}/${PACKAGE_NAME} ${tmp}
  mv ${tmp} ${PACKAGE_NAME}-build

  cd ${PACKAGE_NAME}-build
  echo "Directory: $(pwd)"
  echo "run autoconf"
  rm ./configure
  autoconf
  echo "Configure options: ${CONFIG_FLAGS}"
  ./configure ${CONFIG_FLAGS}
}

CustomInstallStep(){
  local out_dir=${NACL_PACKAGES_REPOSITORY}/${PACKAGE_NAME}
  local build_dir=${out_dir}/${PACKAGE_NAME}-build
  local publish_dir="${NACL_PACKAGES_PUBLISH}/${PACKAGE_NAME}"
  local arch=${NACL_PACKAGES_BITSIZE}
  if [[ "${arch}" != "pnacl" ]] ; then
      arch="x86-${arch}"
  fi

  MakeDir ${publish_dir}
  install ${START_DIR}/xaos.html ${publish_dir}
  install ${START_DIR}/xaos.nmf ${publish_dir}
  # Not used yet
  install ${build_dir}/help/xaos.hlp ${publish_dir}
  install ${build_dir}/bin/xaos ${publish_dir}/xaos_${arch}.nexe
  DefaultTouchStep
}

CustomPackageInstall() {
  DefaultPreInstallStep
  DefaultDownloadStep
  DefaultExtractStep
  CustomPatchStep
  CustomConfigureStep
  DefaultBuildStep
  if [ ${NACL_PACKAGES_BITSIZE} == "pnacl" ] ; then
    DefaultTranslateStep ${PACKAGE_NAME} ${PACKAGE_NAME}-build/bin/xaos
  fi
  CustomInstallStep
  DefaultCleanUpStep
}

CustomPackageInstall
exit 0
