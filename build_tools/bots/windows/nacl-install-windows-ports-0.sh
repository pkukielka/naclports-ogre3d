#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# nacl-install-windows-ports-0.sh
#
# usage:  nacl-install-windows-ports-0.sh
#
# This script builds the packages for Native Client that are designated to
# the bot named windows-ports-0.
#

source ../bot_common.sh

SCRIPT_DIR="$(cd $(dirname $0) && pwd)"
cd ${SCRIPT_DIR}/../../..
make clean

# ffmpeg also builds lame, vorbis, theora, and ogg
BuildPackage ffmpeg
BuildPackage nacl-mounts
BuildPackage sdl
BuildPackage scummvm
BuildPackage bochs
BuildPackage jsoncpp

echo "@@@BUILD_STEP ${NACL_PACKAGES_BITSIZE}-bit Summary@@@"
if [[ $RESULT != 0 ]] ; then
  echo "@@@STEP_FAILURE@@@"
fi

echo -e "$MESSAGES"

exit $RESULT
