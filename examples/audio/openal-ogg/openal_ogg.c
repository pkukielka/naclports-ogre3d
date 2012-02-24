/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/* This example loads an ogg file using the C Pepper URLLoader interface,
 * decodes the file using libvorbis/libogg, and loop plays the file using
 * OpenAL.  Various properties of the audio source and listener can be changed
 * through HTML controls which result in PostMessage calls interpreted below in
 * Messaging_HandleMessage.
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <string.h>

#include "AL/al.h"
#include "AL/alc.h"

/* Pepper includes */
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_module.h"
#include "ppapi/c/ppb_audio.h"
#include "ppapi/c/ppb_audio_config.h"
#include "ppapi/c/ppb_instance.h"
#include "ppapi/c/ppb_core.h"
#include "ppapi/c/ppb_url_loader.h"
#include "ppapi/c/ppb_url_request_info.h"
#include "ppapi/c/ppb_var.h"
#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_input_event.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/ppp_messaging.h"

#define INITIALIZE_OPENAL_ON_MAIN_THREAD 1

PP_Module g_module = 0;
PPB_GetInterface g_get_browser_interface = NULL;

const size_t BUFFER_READ_SIZE = 4096;
const char* OGG_FILE = "sample.ogg";

/* NOTE on PP_Instance: In general Pepper is designed such that a
 * single plugin process can implement multiple plugin instances.
 * This might occur, for example, if a plugin were instantiated by
 * multiple <embed ...> tags in a single page.
 *
 * This implementation assumes at most one instance per plugin,
 * consistent with limitations of the current implementation of
 * Native Client.
 */
struct PepperState {
  const PPB_Core* core_interface;
  const PPB_Instance* instance_interface;
  const PPB_URLRequestInfo* request_interface;
  const PPB_URLLoader* loader_interface;
  const PPB_Var* var_interface;
  PP_Instance instance;
  int ready;
  ALCdevice *alc_device;
  ALCcontext *alc_context;
  ALuint buffer;
  ALuint source;
  float source_pos[3];
  float source_vel[3];
  float listener_pos[3];
  float listener_vel[3];
  float pitch;
  float gain;
  sem_t audio_init_sem;
};
struct PepperState g_MyState;
int g_MyStateIsValid = 0;

char *ogg_file_contents = NULL;
size_t ogg_file_size = 0;
size_t ogg_file_alloced = 0;
void ReadSome(void* data);

extern void DecodeOggBuffer(void *inBuffer, size_t size,
                            char** outBuffer, int* outBufferSize,
                            int* outChannels, int* outRate);

void SetupAndPlayAudio() {
  g_MyState.source_pos[0] = 1.0f;
  g_MyState.source_pos[1] = 1.0f;
  g_MyState.source_pos[2] = 1.0f;

  g_MyState.source_vel[0] = 0.0f;
  g_MyState.source_vel[1] = 0.0f;
  g_MyState.source_vel[2] = 0.0f;

  g_MyState.listener_pos[0] = 0.0f;
  g_MyState.listener_pos[1] = 0.0f;
  g_MyState.listener_pos[2] = 0.0f;

  g_MyState.listener_vel[0] = 0.0f;
  g_MyState.listener_vel[1] = 0.0f;
  g_MyState.listener_vel[2] = 0.0f;

  g_MyState.pitch = 1.0f;
  g_MyState.gain = 1.0f;

  alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
  assert(alGetError() == AL_NO_ERROR);
  alSourcei(g_MyState.source, AL_LOOPING, AL_TRUE);
  assert(alGetError() == AL_NO_ERROR);
  alSourcefv(g_MyState.source, AL_POSITION, g_MyState.source_pos);
  assert(alGetError() == AL_NO_ERROR);
  alSourcef(g_MyState.source, AL_REFERENCE_DISTANCE, 1.0f);
  assert(alGetError() == AL_NO_ERROR);
  alSourcef(g_MyState.source, AL_MAX_DISTANCE, 70.0f);
  assert(alGetError() == AL_NO_ERROR);
  alSourcef(g_MyState.source, AL_GAIN, 1.0f);
  assert(alGetError() == AL_NO_ERROR);
  alSourcei(g_MyState.source, AL_BUFFER, g_MyState.buffer);
  assert(alGetError() == AL_NO_ERROR);
  alSourcef(g_MyState.source, AL_PITCH, g_MyState.pitch);
  assert(alGetError() == AL_NO_ERROR);
  alSourcef(g_MyState.source, AL_GAIN, g_MyState.gain);
  assert(alGetError() == AL_NO_ERROR);
  alSourcePlay(g_MyState.source);
  assert(alGetError() == AL_NO_ERROR);
}

void* DecodeAndPlayOggFile(void* data) {
  char* pcm_buffer;
  int buffer_size;
  int num_channels;
  int rate;
  DecodeOggBuffer(ogg_file_contents, ogg_file_size,
                  &pcm_buffer, &buffer_size, &num_channels, &rate);

  /* If OpenAL is initialized on a secondary thread, we need to wait
   * until it is done.
   */
  sem_wait(&g_MyState.audio_init_sem);
  /* Pass the decoded PCM buffer to OpenAL. */
  alBufferData(g_MyState.buffer,
               num_channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
               pcm_buffer,
               buffer_size,
               rate);
  assert(alGetError() == AL_NO_ERROR);

  free(pcm_buffer);
  free(ogg_file_contents);

  ogg_file_contents = NULL;
  ogg_file_alloced = 0;
  ogg_file_size = 0;

  SetupAndPlayAudio();
  g_MyState.ready = 1;
  return NULL;
}

static void ReadCallback(void* data, int32_t result) {
  if (result == PP_OK) {
    /* We're done reading the file. */
#if INITIALIZE_OPENAL_ON_MAIN_THREAD
    DecodeAndPlayOggFile(NULL);
#else
    /* Spawn a new thread so we can wait if needed. */
    pthread_t p;
    pthread_create(&p, NULL, DecodeAndPlayOggFile, NULL);
#endif
  } else if (result > 0) {
    /* 'result' bytes were read into memory. */
    ogg_file_size += (size_t)result;
    ReadSome(data);
  } else assert(0);
}

static void OpenCallback(void* data, int32_t result) {
  assert(result == PP_OK);
  ReadSome(data);
}

/* Read up to BUFFER_READ_SIZE bytes more from the URLLoader.
 * Allocate more space in the destination buffer if needed.
 */
void ReadSome(void* data) {
  while (ogg_file_alloced < ogg_file_size + BUFFER_READ_SIZE) {
    /* The buffer isn't big enough to hold BUFFER_READ_SIZE more bytes. */
    char *temp = (char*)malloc(ogg_file_alloced * 2);
    memcpy(temp, ogg_file_contents, ogg_file_size);
    free(ogg_file_contents);
    ogg_file_contents = temp;
    ogg_file_alloced *= 2;
  }

  struct PP_CompletionCallback cb =
      PP_MakeCompletionCallback(ReadCallback, data);
  int32_t read_ret = g_MyState.loader_interface->ReadResponseBody(
      (PP_Resource)data,
      ogg_file_contents + ogg_file_size,
      BUFFER_READ_SIZE,
      cb);
  assert(read_ret == PP_OK_COMPLETIONPENDING);
}

/* This function is part of the NaCl libopenal port and is
 * required to be called before OpenAL initialization.
 */
extern void alSetPpapiInfo(PP_Instance, PPB_GetInterface);

void* InitializeOpenAL(void* data) {
  /* PPAPI should be the default device in NaCl, hence 'NULL'. */
  g_MyState.alc_device = alcOpenDevice(NULL);
  assert(g_MyState.alc_device != NULL);

  g_MyState.alc_context = alcCreateContext(g_MyState.alc_device, 0);
  assert(g_MyState.alc_context != NULL);

  alcMakeContextCurrent(g_MyState.alc_context);
  alGenBuffers(1, &g_MyState.buffer);
  assert(alGetError() == AL_NO_ERROR);
  alGenSources(1, &g_MyState.source);
  assert(alGetError() == AL_NO_ERROR);
  sem_post(&g_MyState.audio_init_sem);
  return NULL;
}

static PP_Bool Instance_DidCreate(PP_Instance instance,
                                  uint32_t argc,
                                  const char* argn[],
                                  const char* argv[]) {
  g_MyState.instance = instance;
  g_MyState.ready = 0;
  g_MyStateIsValid = 1;

  /* This sets up OpenAL with PPAPI info. */
  alSetPpapiInfo(instance, g_get_browser_interface);

  const ALCchar *devices = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
  printf("Audio devices available:\n");
  while (devices[0] != '\0') {
    printf("\t%s\n", devices);
    devices = devices + strlen(devices)+1;
  }

  sem_init(&g_MyState.audio_init_sem, 0, 0);
#if INITIALIZE_OPENAL_ON_MAIN_THREAD
  InitializeOpenAL(NULL);
#else
  pthread_t p;
  pthread_create(&p, NULL, InitializeOpenAL, NULL);
#endif

  ogg_file_contents = (char*)malloc(BUFFER_READ_SIZE);
  ogg_file_alloced = BUFFER_READ_SIZE;

  PP_Resource request = g_MyState.request_interface->Create(instance);
  g_MyState.request_interface->SetProperty(
      request,
      PP_URLREQUESTPROPERTY_URL,
      g_MyState.var_interface->VarFromUtf8(OGG_FILE, strlen(OGG_FILE)));

  PP_Resource loader = g_MyState.loader_interface->Create(instance);
  struct PP_CompletionCallback cb =
      PP_MakeCompletionCallback(OpenCallback, (void*)loader);
  int32_t open_ret = g_MyState.loader_interface->Open(loader, request, cb);
  assert(open_ret == PP_OK_COMPLETIONPENDING);

  return PP_TRUE;
}

static void Instance_DidDestroy(PP_Instance instance) {
  assert(g_MyState.instance == instance && g_MyStateIsValid == 1);
  g_MyStateIsValid = 0;
}

static void Instance_DidChangeView(PP_Instance pp_instance,
                                   PP_Resource view_resource) {
}
static void Instance_DidChangeView_1_0(PP_Instance pp_instance,
                                       const struct PP_Rect* pos,
                                       const struct PP_Rect* clip) {
}

static void Instance_DidChangeFocus(PP_Instance pp_instance,
                                    PP_Bool has_focus) {
}

static PP_Bool Instance_HandleDocumentLoad(PP_Instance pp_instance,
                                           PP_Resource pp_url_loader) {
  return PP_FALSE;
}

static void Messaging_HandleMessage(PP_Instance pp_instance,
                                    struct PP_Var message) {
  if (g_MyState.ready == 0) return;
  if (message.type != PP_VARTYPE_STRING) return;

  uint32_t len;
  const char* str = g_MyState.var_interface->VarToUtf8(message, &len);
  int array_edit = 0;
  float *array = NULL;
  int array_index = 0;

  if (strstr(str, "source_pos")) {
    array_edit = 1;
    array = g_MyState.source_pos;
  } else if (strstr(str, "source_vel")) {
    array_edit = 1;
    array = g_MyState.source_vel;
  } else if (strstr(str, "listener_pos")) {
    array_edit = 1;
    array = g_MyState.listener_pos;
  } else if (strstr(str, "listener_vel")) {
    array_edit = 1;
    array = g_MyState.listener_vel;
  } else if (strstr(str, "pitch")) {
    double val = atof(strstr(str, "= ")+2);
    g_MyState.pitch = val;
  } else if (strstr(str, "gain")) {
    double val = atof(strstr(str, "= ")+2);
    g_MyState.gain = val;
  }

  if (array_edit) {
    if (strstr(str, "_x")) {
      array_index = 0;
    } else if (strstr(str, "_y")) {
      array_index = 1;
    } else if (strstr(str, "_z")) {
      array_index = 2;
    }
    double val = atof(strstr(str, "= ")+2);
    array[array_index] = val;
  }

  alSourcefv(g_MyState.source, AL_POSITION, g_MyState.source_pos);
  assert(alGetError() == AL_NO_ERROR);
  alSourcefv(g_MyState.source, AL_VELOCITY, g_MyState.source_vel);
  assert(alGetError() == AL_NO_ERROR);
  alListenerfv(AL_POSITION, g_MyState.listener_pos);
  assert(alGetError() == AL_NO_ERROR);
  alListenerfv(AL_VELOCITY, g_MyState.listener_vel);
  assert(alGetError() == AL_NO_ERROR);
  alSourcef(g_MyState.source, AL_PITCH, g_MyState.pitch);
  assert(alGetError() == AL_NO_ERROR);
  alSourcef(g_MyState.source, AL_GAIN, g_MyState.gain);
  assert(alGetError() == AL_NO_ERROR);
}

static PPP_Instance instance_interface = {
  &Instance_DidCreate,
  &Instance_DidDestroy,
  &Instance_DidChangeView,
  &Instance_DidChangeFocus,
  &Instance_HandleDocumentLoad,
};

static PPP_Messaging messaging_interface = {
  &Messaging_HandleMessage,
};

/* Global entrypoints --------------------------------------------------------*/

PP_EXPORT int32_t PPP_InitializeModule(PP_Module module,
                                       PPB_GetInterface get_browser_interface) {
  g_get_browser_interface = get_browser_interface;
  g_module = module;

  g_MyState.core_interface = (const PPB_Core*)
    get_browser_interface(PPB_CORE_INTERFACE);
  g_MyState.instance_interface = (const PPB_Instance*)
    get_browser_interface(PPB_INSTANCE_INTERFACE);
  g_MyState.request_interface = (const PPB_URLRequestInfo*)
    get_browser_interface(PPB_URLREQUESTINFO_INTERFACE);
  g_MyState.loader_interface = (const PPB_URLLoader*)
    get_browser_interface(PPB_URLLOADER_INTERFACE);
  g_MyState.var_interface = (const PPB_Var*)
    get_browser_interface(PPB_VAR_INTERFACE);

  if (!g_MyState.core_interface || !g_MyState.instance_interface ||
      !g_MyState.request_interface || !g_MyState.loader_interface ||
      !g_MyState.var_interface) {
    printf("Required interfaces are not available.\n");
    return -1;
  }

  /* These interfaces are used by OpenAL so check for them here
   * to make sure they're available.
   */
  if (get_browser_interface(PPB_AUDIO_INTERFACE) == NULL ||
      get_browser_interface(PPB_AUDIO_CONFIG_INTERFACE) == NULL) {
    printf("Audio interfaces are not available.\n");
    return -1;
  }

  return PP_OK;
}

PP_EXPORT void PPP_ShutdownModule() {
}

PP_EXPORT const void* PPP_GetInterface(const char* interface_name) {
  if (strcmp(interface_name, PPP_INSTANCE_INTERFACE) == 0)
    return &instance_interface;
  if (strcmp(interface_name, PPP_MESSAGING_INTERFACE) == 0)
    return &messaging_interface;
  return NULL;
}
