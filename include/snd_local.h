#pragma once
#include <metahook.h>
#include <queue>

#include "exportfuncs.h"
#include "enginedef.h"
#include "plugins.h"
#include "aud_int_internal.h"

//internal structures

struct GainFading
{
  float current{ 0 };
  float elapsed_time{ 0 };
  float initial_value{ 0 };
  float last_target{ 0 };
  float target{ 0 };
};

namespace MetaAudio
{
  class AudioEngine;
  class SoundLoader;
  class VoxManager;
  class BaseSoundSource;
}

struct aud_channel_t
{
  sfx_t* sfx{ nullptr };
  float volume{ 1.0f };
  float pitch{ 1.0f };
  float attenuation{ 0 };
  int entnum{ 0 };
  int entchannel{ 0 };
  vec3_t origin{ 0.0f, 0.0f, 0.0f };
  uint64_t start{ 0 };
  uint64_t end{ 0 };
  //for sentence
  std::queue<voxword_t> words;
  //for voice sound
  sfxcache_t* voicecache{ nullptr };

  // For OpenAL
  alure::SharedPtr<MetaAudio::BaseSoundSource> sound_source;

  MetaAudio::VoxManager* vox{ nullptr };

  // For OpenAL EFX
  bool firstpass{ true };
  GainFading LowGain;
  GainFading MidGain;
  GainFading HighGain;

  aud_channel_t() = default;
  ~aud_channel_t();

  aud_channel_t(const aud_channel_t& other) = delete;
  aud_channel_t& aud_channel_t::operator=(const aud_channel_t& other) = delete;

  aud_channel_t(aud_channel_t&& other) = default;
  aud_channel_t& operator=(aud_channel_t&& other) = default;
};

struct aud_sfxcache_t
{
  //wave info
  uint64_t length{};
  uint64_t loopstart{};
  uint64_t loopend{};
  ALuint samplerate{};
  bool looping{};
  bool force_streaming{};
  alure::SampleType stype{};
  alure::ChannelConfig channels{};

  //OpenAL buffer
  alure::SharedPtr<alure::Decoder> decoder;
  alure::Buffer buffer;

  alure::Vector<ALubyte> data;
};

struct aud_engine_t
{
  //s_dma.c
  int(*SNDDMA_Init)(void);//hooked
  void(*S_Startup)(void);//deprecated, use SNDDMA_Init instead
  void(*S_Init)(void);//hooked
  void(*S_Shutdown)(void);//hooked
  sfx_t *(*S_FindName)(char *name, int *pfInCache);//hooked
  sfx_t *(*S_PrecacheSound)(char *name);//hooked
  void(*SND_Spatialize)(aud_channel_t *ch);
  void(*S_Update)(float *origin, float *forward, float *right, float *up);//hooked
  void(*S_StartDynamicSound)(int entnum, int entchannel, sfx_t *sfx, float *origin, float fvol, float attenuation, int flags, int pitch);//hooked
  void(*S_StartStaticSound)(int entnum, int entchannel, sfx_t *sfx, float *origin, float fvol, float attenuation, int flags, int pitch);//hooked
  void(*S_StopSound)(int entnum, int entchannel);//hooked
  void(*S_StopAllSounds)(qboolean clear);//hooked

  //s_mem.c
  aud_sfxcache_t *(*S_LoadSound)(sfx_t *s, aud_channel_t *ch);//hooked

  //s_mix.c
  void(*VOX_Shutdown)(void);
  void(*S_FreeChannel)(channel_t *ch);

  //voice_sound_engine_interface.cpp
  void(*VoiceSE_NotifyFreeChannel)(int iChannel);
  void(*VoiceSE_Idle)(float frametime);

  //sequence.c
  sentenceEntry_s*(*SequenceGetSentenceByIndex)(unsigned int index);

  //gl_rmain.c
  void(*R_DrawTEntitiesOnList)(int onlyClientDraw);
};

//snd_hook.cpp
void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);

void S_InstallHook(MetaAudio::AudioEngine* engine, MetaAudio::SoundLoader* loader);

void S_UninstallHook();

PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo);

//common
extern aud_engine_t gAudEngine;

extern int* cl_servercount;
extern int* cl_parsecount;
extern void* cl_frames;
extern int size_of_frame;
extern int* cl_viewentity;
extern int* cl_num_entities;
extern int* cl_waterlevel;

extern double* cl_time;
extern double* cl_oldtime;

extern float* g_SND_VoiceOverdrive;

extern char* (*rgpszrawsentence)[CVOXFILESENTENCEMAX];
extern int* cszrawsentences;