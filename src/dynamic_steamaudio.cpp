#include "dynamic_steamaudio.hpp"

namespace MetaAudio
{

#define SetSteamAudioFunctionPointer(library, __function_name__) \
    __function_name__ = reinterpret_cast<decltype(MetaAudio::SteamAudio::__function_name__)>(GetProcAddress(library, #__function_name__));

  SteamAudio::SteamAudio()
  {
    HINSTANCE hSteamAudioInstance = LoadLibrary("phonon.dll");
    if (hSteamAudioInstance)
    {
      SetSteamAudioFunctionPointer(hSteamAudioInstance, iplCleanup);
      SetSteamAudioFunctionPointer(hSteamAudioInstance, iplDestroyEnvironment);
      SetSteamAudioFunctionPointer(hSteamAudioInstance, iplDestroyScene);
      SetSteamAudioFunctionPointer(hSteamAudioInstance, iplDestroyStaticMesh);
      SetSteamAudioFunctionPointer(hSteamAudioInstance, iplGetDirectSoundPath);
      SetSteamAudioFunctionPointer(hSteamAudioInstance, iplCreateScene);
      SetSteamAudioFunctionPointer(hSteamAudioInstance, iplCreateStaticMesh);
      SetSteamAudioFunctionPointer(hSteamAudioInstance, iplCreateEnvironment);
      SetSteamAudioFunctionPointer(hSteamAudioInstance, iplCreateContext);
      SetSteamAudioFunctionPointer(hSteamAudioInstance, iplDestroyContext);
      SetSteamAudioFunctionPointer(hSteamAudioInstance, iplGetMixedEnvironmentalAudio);

      library_instance = std::shared_ptr<HINSTANCE>(new HINSTANCE(hSteamAudioInstance), [&](HINSTANCE* handle) { iplCleanup(); FreeLibrary(*handle); delete handle; });
    }
  }
}