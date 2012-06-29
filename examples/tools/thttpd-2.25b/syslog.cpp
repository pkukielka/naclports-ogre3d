/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <nacl-mounts/util/DebugPrint.h>
#include <cstdarg>
#include <cstdio>
#include "json/json.h"
#include "my_syslog.h"
#include "nacl_module.h"
#include "shelljob.h"

#define MAX_FMT_SIZE 4096
char formatted_string[MAX_FMT_SIZE];

void syslog(int level, const char* message, ...) {
  switch (level) {
    case LOG_INFO:
    dbgprintf("INFO: ");
    break;
    case LOG_WARN:
    dbgprintf("WARN: ");
    break;
    case LOG_ERR:
    dbgprintf("ERR: ");
    break;
    case LOG_CRIT:
    dbgprintf("CRIT: ");
    break;
  }
  va_list argptr;
  va_start(argptr, message);
  vsprintf(formatted_string, message, argptr);
  va_end(argptr);
  dbgprintf("%s", formatted_string);
  if (strlen(message) > 0 && message[strlen(message) - 1] != '\n') {
    dbgprintf("\n");
  }
}

extern "C" void network_error() {
  Json::Value writerRoot;
  writerRoot["result"] = 1;
  writerRoot["type"] = "network error";
  Json::StyledWriter writer;
  ShellJob* job = new ShellJob(writer.write(writerRoot),
    *(ThttpdInstance::getInstance()));
  ThttpdInstance::getInstance()->RunJob(job);
}

namespace pp {
/// Factory function called by the browser when the module is first loaded.
/// The browser keeps a singleton of this module.  It calls the
/// CreateInstance() method on the object you return to make instances.  There
/// is one instance per <embed> tag on the page.  This is the main binding
/// point for your NaCl module with the browser.
Module* CreateModule() {
  return new ThttpdModule();
}
}  // namespace pp


