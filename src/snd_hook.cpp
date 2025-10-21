#include <metahook.h>

#include "plugins.h"
#include "snd_local.h"
#include "AudioEngine.hpp"
#include "Loaders/SoundLoader.hpp"

#include <capstone.h>

aud_engine_t gAudEngine = {0};

int* cl_servercount = nullptr;
int* cl_parsecount = nullptr;
void* cl_frames = nullptr;
int size_of_frame = 0;
int* cl_viewentity = nullptr;
int* cl_num_entities = nullptr;
int* cl_waterlevel = nullptr;

double* cl_time = nullptr;
double* cl_oldtime = nullptr;

float* g_SND_VoiceOverdrive = nullptr;

char* (*rgpszrawsentence)[CVOXFILESENTENCEMAX] = nullptr;
int* cszrawsentences = nullptr;

#define S_INIT_SIG_BLOB "\x83\xEC\x08\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0"
#define S_INIT_SIG_NEW "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0"
#define S_INIT_SIG_HL25 S_INIT_SIG_NEW
#define S_INIT_SIG_SVENGINE S_INIT_SIG_NEW

#define S_STARTUP_SIG_BLOB "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x74\x2A\xA1\x2A\x2A\x2A\x2A\x85\xC0\x75\x2A\xE8\x2A\x2A\x2A\x2A\x85\xC0"
#define S_STARTUP_SIG_NEW S_STARTUP_SIG_BLOB
#define S_STARTUP_SIG_HL25     "\x83\x3D\x2A\x2A\x2A\x2A\x00\x74\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x75\x2A\xE8\x2A\x2A\x2A\x2A\x85\xC0"
#define S_STARTUP_SIG_SVENGINE S_STARTUP_SIG_HL25

#define S_SHUTDOWN_SIG_BLOB "\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC9\x3B\xC1\x74\x2A\xA1\x2A\x2A\x2A\x2A\x3B\xC1\x74\x2A\x89\x08"
#define S_SHUTDOWN_SIG_NEW S_SHUTDOWN_SIG_BLOB
#define S_SHUTDOWN_SIG_HL25 "\xE8\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x74\x2A\xA1\x2A\x2A\x2A\x2A\x85\xC0\x74\x2A\xC7\x00\x00\x00\x00\x00"
#define S_SHUTDOWN_SIG_SVENGINE S_SHUTDOWN_SIG_HL25

#define S_FINDNAME_SIG_BLOB "\x53\x55\x8B\x6C\x24\x0C\x33\xDB\x56\x57\x85\xED"
#define S_FINDNAME_SIG_NEW "\x55\x8B\xEC\x53\x56\x8B\x75\x08\x33\xDB\x85\xF6"
#define S_FINDNAME_SIG_HL25 "\x55\x8B\xEC\x53\x8B\x5D\x08\x56\x33\xF6\x57\x85"
#define S_FINDNAME_SIG_SVENGINE "\x53\x55\x8B\x6C\x24\x0C\x56\x33\xF6\x57\x85\xED\x75\x2A\x68"

#define S_PRECACHESOUND_SIG_BLOB "\xA1\x2A\x2A\x2A\x2A\x56\x85\xC0\x74\x2A\xD9\x05"
#define S_PRECACHESOUND_SIG_NEW "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x56\x85\xC0\x74\x2A\xD9\x05"
#define S_PRECACHESOUND_SIG_HL25 "\x55\x8B\xEC\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x74\x2A\xF3\x0F\x10\x05\x2A\x2A\x2A\x2A\x0F\x57\xC9"
#define S_PRECACHESOUND_SIG_SVENGINE "\x51\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x74\x2A\xD9\x05\x2A\x2A\x2A\x2A\xD9\xEE"

#define S_LOADSOUND_SIG_SVENGINE "\x81\xEC\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x2A\x00\x00\x8B\x8C\x24\x2A\x2A\x00\x00\x56\x8B\xB4\x24\x2A\x2A\x00\x00\x8A\x06\x3C\x2A"
#define S_LOADSOUND_SIG_HL25 "\x55\x8B\xEC\x81\xEC\x34\x05\x00\x00\xA1"
#define S_LOADSOUND_SIG_NEW2 "\x55\x8B\xEC\x81\xEC\x28\x05\x00\x00\x53\x8B\x5D\x08"
#define S_LOADSOUND_SIG_NEW "\x55\x8B\xEC\x81\xEC\x44\x05\x00\x00\x53\x56\x8B\x75\x08"
#define S_LOADSOUND_SIG_BLOB "\x81\xEC\x2A\x2A\x00\x00\x53\x8B\x9C\x24\x2A\x2A\x00\x00\x55\x56\x8A\x03\x57"

#define SND_SPATIALIZE_SIG_SVENGINE "\x83\xEC\x10\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x0C\x2A\x8B\x74\x24\x18\x8B\x46\x18"
#define SND_SPATIALIZE_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x14\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x2A\x8B\x75\x08\x8B\x46\x18\x3B\x05"
#define SND_SPATIALIZE_SIG_NEW "\x55\x8B\xEC\x83\xEC\x1C\x8B\x0D\x2A\x2A\x2A\x2A\x56"
#define SND_SPATIALIZE_SIG_BLOB "\x83\xEC\x34\x8B\x0D\x2A\x2A\x2A\x2A\x53\x56"

#define S_STARTDYNAMICSOUND_SIG_BLOB "\x83\xEC\x48\xA1\x2A\x2A\x2A\x2A\x53\x55\x56\x85\xC0\x57\xC7\x44\x24\x10\x00\x00\x00\x00"
#define S_STARTDYNAMICSOUND_SIG_NEW "\x55\x8B\xEC\x83\xEC\x48\xA1\x2A\x2A\x2A\x2A\x53\x56\x57\x85\xC0\xC7\x45\xFC\x00\x00\x00\x00"
#define S_STARTDYNAMICSOUND_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x5C\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x83\x3D\x2A\x2A\x2A\x2A\x2A\x8B\x45\x08"
#define S_STARTDYNAMICSOUND_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x54\x8B\x44\x24\x5C\x55"

#define S_STARTSTATICSOUND_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x48\x57\x8B\x7C\x24\x5C"
#define S_STARTSTATICSOUND_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x50\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x57"
#define S_STARTSTATICSOUND_SIG_NEW "\x55\x8B\xEC\x83\xEC\x44\x53\x56\x57\x8B\x7D\x10\x85\xFF\xC7\x45\xFC\x00\x00\x00\x00"
#define S_STARTSTATICSOUND_SIG_BLOB "\x83\xEC\x44\x53\x55\x8B\x6C\x24\x58\x56\x85\xED\x57"

#define S_STOPSOUND_SIG_SVENGINE "\xA1\x2A\x2A\x2A\x2A\x57\xBF\x04\x00\x00\x00\x3B\xC7"
#define S_STOPSOUND_SIG_HL25 "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x57\xBF\x04\x00\x00\x00\x3B\xC7"
#define S_STOPSOUND_SIG_NEW S_STOPSOUND_SIG_HL25
#define S_STOPSOUND_SIG_BLOB S_STOPSOUND_SIG_SVENGINE

#define S_SOUNDALLSOUNDS_SIG_SVENGINE "\x83\xEC\x08\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x2A\x2A\x2A\x2A\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\x0C\x00\x00\x00"
#define S_SOUNDALLSOUNDS_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x08\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x84\x2A\x2A\x2A\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\x24\x00\x00\x00"
#define S_SOUNDALLSOUNDS_SIG_NEW "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x85\xC0\x74\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\x0C\x00\x00\x00"
#define S_SOUNDALLSOUNDS_SIG_BLOB "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x74\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\x0C\x00\x00\x00"

#define S_UPDATE_SIG_SVENGINE "\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x2A\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x2A\x2A\x2A\x2A\x2A\x8B\x44\x24\x04\x8B\x0D\x2A\x2A\x2A\x2A\x2A\x2A\xD9\x00"
#define S_UPDATE_SIG_HL25     "\x55\x8B\xEC\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x2A\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x2A\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00"
#define S_UPDATE_SIG_NEW      "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x85\xC0\x0F\x2A\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x85\xC0\x0F\x2A\x2A\x2A\x2A\x2A\x8B\x45\x08\x2A\x2A\x8B\x08"
#define S_UPDATE_SIG_BLOB     "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x0F\x84\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x85\xC0\x0F\x8F\x2A\x2A\x00\x00"

#define SEQUENCE_GETSENTENCEBYINDEX_SIG_SVENGINE "\x8B\x0D\x2A\x2A\x2A\x2A\x2A\x33\x2A\x85\xC9\x2A\x2A\x8B\x2A\x24\x08\x8B\x41\x04\x2A\x2A\x3B\x2A\x2A\x2A\x8B\x49\x0C"
#define SEQUENCE_GETSENTENCEBYINDEX_SIG_HL25 "\x55\x8B\xEC\x8B\x0D\x2A\x2A\x2A\x2A\x56\x33"
#define SEQUENCE_GETSENTENCEBYINDEX_SIG_NEW "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x33\xC9\x85\xC0\x2A\x2A\x2A\x8B\x75\x08\x8B\x50\x04"
#define SEQUENCE_GETSENTENCEBYINDEX_SIG_BLOB "\xA1\x2A\x2A\x2A\x2A\x33\xC9\x85\xC0\x56\x2A\x2A\x8B\x74\x24\x08\x8B\x50\x04"

#define VOICESE_IDLE_SIG_SVENGINE "\x80\x3D\x2A\x2A\x2A\x2A\x00\xD9\x05\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\x74\x2A\xDD\xE1\xDF\xE0"
#define VOICESE_IDLE_SIG_HL25 "\x55\x8B\xEC\x80\x3D\x2A\x2A\x2A\x2A\x00\xF3\x0F\x10\x2A\x2A\x2A\x2A\x2A\xF3\x0F\x10\x2A\x2A\x2A\x2A\x2A"
#define VOICESE_IDLE_SIG_NEW "\x55\x8B\xEC\xA0\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\x84\xC0\x74\x2A\xD8\x1D"
#define VOICESE_IDLE_SIG_BLOB "\xA0\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\x84\xC0\x74\x2A\xD8\x1D"

#define VOICESE_NOTIFYFREECHANNEL_SIG_SVENGINE "\x83\xF8\x0A\x2A\x2A\x3D\xF4\x01\x00\x00\x2A\x2A\x50\xE8"
#define VOICESE_NOTIFYFREECHANNEL_SIG_GOLDSRC "\x83\xF8\x07\x2A\x2A\x3D\xF4\x01\x00\x00\x2A\x2A\x50\xE8"

#define R_DRAWTENTITIESONLIST_SIG_BLOB "\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x2A\x0F\x2A\x2A\x2A\x00\x00\x8B\x44\x24\x04"
#define R_DRAWTENTITIESONLIST_SIG_NEW2 R_DRAWTENTITIESONLIST_SIG_BLOB
#define R_DRAWTENTITIESONLIST_SIG_NEW "\x55\x8B\xEC\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x44\x0F\x8B\x2A\x2A\x2A\x2A\x8B\x45\x08"
#define R_DRAWTENTITIESONLIST_SIG_HL25 "\x55\x8B\xEC\x81\xEC\x2A\x00\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\xF3\x0F\x2A\x2A\x2A\x2A\x2A\x2A\x0F\x2E"
#define R_DRAWTENTITIESONLIST_SIG_SVENGINE "\x55\x8B\xEC\x83\xE4\x2A\x81\xEC\x2A\x00\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x00\x00\x00\xD9\x05\x2A\x2A\x2A\x2A\xD9\xEE"

void Engine_FillAddress_S_Init(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    if (gAudEngine.S_Init)
        return;

    PVOID S_Init_VA = nullptr;

    if (1)
    {
        /*
.text:01D96050                                     S_Init          proc near               ; CODE XREF: sub_1D65260+32B
.text:01D96050 68 08 CE E6 01                                      push    offset aSoundInitializ ; "Sound Initialization\n"
.text:01D96055 E8 76 DB F6 FF                                      call    sub_1D03BD0
.text:01D9605A E8 E1 3A 00 00                                      call    sub_1D99B40
.text:01D9605F 68 D8 D9 E5 01                                      push    offset aNosound ; "-nosound"
          */

        const char sigs[] = "Sound Initialization\n";
        auto Sound_Init_String = Search_Pattern_Data(sigs, DllInfo);
        if (!Sound_Init_String)
            Sound_Init_String = Search_Pattern_Rdata(sigs, DllInfo);
        if (Sound_Init_String)
        {
            char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A";
            *(DWORD*)(pattern + 1) = (DWORD)Sound_Init_String;
            auto Sound_Init_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
            if (Sound_Init_PushString)
            {
                S_Init_VA = Sound_Init_PushString;
            }
        }
    }

    if (!S_Init_VA)
    {
        if (g_iEngineType == ENGINE_SVENGINE)
        {
            S_Init_VA = Search_Pattern(S_INIT_SIG_SVENGINE, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
        {
            S_Init_VA = Search_Pattern(S_INIT_SIG_HL25, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC)
        {
            S_Init_VA = Search_Pattern(S_INIT_SIG_NEW, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
        {
            S_Init_VA = Search_Pattern(S_INIT_SIG_BLOB, DllInfo);
        }
    }

    gAudEngine.S_Init = (decltype(gAudEngine.S_Init))ConvertDllInfoSpace(S_Init_VA, DllInfo, RealDllInfo);
    Sig_FuncNotFound(S_Init);
}


void Engine_FillAddress_S_Startup(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    if (gAudEngine.S_Startup)
        return;

    PVOID S_Startup_VA = nullptr;

    if (1)
    {
        /*
.text:01D8E3AA                                     loc_1D8E3AA:                            ; CODE XREF: sub_1D8E280+6E��j
.text:01D8E3AA 68 D0 52 E5 01                                      push    offset aSTransferstere ; "S_TransferStereo16: DS::Lock Sound Buff"...
.text:01D8E3AF
.text:01D8E3AF                                     loc_1D8E3AF:                            ; CODE XREF: sub_1D8E280+14D��j
.text:01D8E3AF E8 BC E7 F9 FF                                      call    sub_1D2CB70
.text:01D8E3B4 83 C4 04                                            add     esp, 4
.text:01D8E3B7 E8 E4 D6 FF FF                                      call    S_Shutdown
.text:01D8E3BC E8 8F D3 FF FF                                      call    S_Startup
          */

        const char sigs[] = "S_Startup: SNDDMA_Init failed";
        auto S_Startup_String = Search_Pattern_Data(sigs, DllInfo);
        if (!S_Startup_String)
            S_Startup_String = Search_Pattern_Rdata(sigs, DllInfo);
        if (S_Startup_String)
        {
            char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
            *(DWORD*)(pattern + 1) = (DWORD)S_Startup_String;
            auto S_Startup_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
            if  (S_Startup_PushString)
            {
                S_Startup_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(S_Startup_PushString, 0x100, [](PUCHAR Candidate) {

                    if ((Candidate[-1] == 0x90 || Candidate[-1] == 0xCC) && 
                        Candidate[0] == 0xA1 &&
                        Candidate[5] == 0x85 &&
                        Candidate[6] == 0xC0)
                    {
                        return TRUE;
                    }

                    if ((Candidate[-1] == 0x90 || Candidate[-1] == 0xCC) &&
                        Candidate[0] == 0x83 &&
                        Candidate[1] == 0x3D &&
                        Candidate[6] == 0x00)
                    {
                        return TRUE;
                    }

                    return FALSE;
                });
            }
        }
    }

    if (!S_Startup_VA)
    {
        if (g_iEngineType == ENGINE_SVENGINE)
        {
            S_Startup_VA = Search_Pattern(S_STARTUP_SIG_SVENGINE, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
        {
            S_Startup_VA = Search_Pattern(S_STARTUP_SIG_HL25, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC)
        {
            S_Startup_VA = Search_Pattern(S_STARTUP_SIG_NEW, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
        {
            S_Startup_VA = Search_Pattern(S_STARTUP_SIG_BLOB, DllInfo);
        }
    }

    gAudEngine.S_Startup = (decltype(gAudEngine.S_Startup))ConvertDllInfoSpace(S_Startup_VA, DllInfo, RealDllInfo);
    Sig_FuncNotFound(S_Startup);

    {
        PVOID SNDDMA_Init_VA = nullptr;

        typedef struct S_Startup_SearchContexs
        {
            PVOID& SNDDMA_Init_VA;
            const mh_dll_info_t& DllInfo;
            const mh_dll_info_t& RealDllInfo;
        }S_Startup_SearchContext;

        S_Startup_SearchContext ctx = { SNDDMA_Init_VA, DllInfo, RealDllInfo };

        g_pMetaHookAPI->DisasmRanges(S_Startup_VA, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
            {
                auto pinst = (cs_insn*)inst;
                auto ctx = (S_Startup_SearchContext*)context;

                if (!ctx->SNDDMA_Init_VA && address[0] == 0xE8 && address[-5] != 0x68)
                {
                    ctx->SNDDMA_Init_VA = (PUCHAR)pinst->detail->x86.operands[0].imm;
                }

                if (ctx->SNDDMA_Init_VA)
                    return TRUE;

                if (address[0] == 0xCC)
                    return TRUE;

                if (pinst->id == X86_INS_RET)
                    return TRUE;

                return FALSE;
            }, 0, &ctx);


        gAudEngine.SNDDMA_Init = (decltype(gAudEngine.SNDDMA_Init))ConvertDllInfoSpace(SNDDMA_Init_VA, DllInfo, RealDllInfo);
        Sig_FuncNotFound(SNDDMA_Init);
    }
}

void Engine_FillAddress_S_Shutdown(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    if (gAudEngine.S_Shutdown)
        return;

    PVOID S_Shutdown_VA = nullptr;

    /*
.text:01D96C7D                                     loc_1D96C7D:                            ; CODE XREF: sub_1D96B60+85
.text:01D96C7D 68 F4 CF E6 01                                      push    offset aSClearbufferDs_0 ; "S_ClearBuffer: DS::Lock Sound Buffer Fa"...
.text:01D96C82 E8 39 CE F6 FF                                      call    sub_1D03AC0
.text:01D96C87 83 C4 04                                            add     esp, 4
.text:01D96C8A E8 61 F6 FF FF                                      call    S_Shutdown
.text:01D96C8F 5E                                                  pop     esi
.text:01D96C90 5B                                                  pop     ebx
.text:01D96C91 83 C4 08                                            add     esp, 8
.text:01D96C94 C3                                                  retn
    */

    const char sigs[] = "S_ClearBuffer: DS::Lock";
    auto S_ClearBuffer_String = Search_Pattern_Data(sigs, DllInfo);
    if (!S_ClearBuffer_String)
        S_ClearBuffer_String = Search_Pattern_Rdata(sigs, DllInfo);
    if (S_ClearBuffer_String)
    {
        char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
        *(DWORD*)(pattern + 1) = (DWORD)S_ClearBuffer_String;
        auto S_ClearBuffer_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
        if (S_ClearBuffer_PushString)
        {
            typedef struct S_Shutdown_SearchContext_s
            {
                PVOID& S_Shutdown_VA;
                const mh_dll_info_t& DllInfo; 
                const mh_dll_info_t& RealDllInfo;
            }S_Shutdown_SearchContext;

            S_Shutdown_SearchContext ctx = { S_Shutdown_VA, DllInfo, RealDllInfo };

            g_pMetaHookAPI->DisasmRanges(S_ClearBuffer_PushString + Sig_Length(pattern), 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
            {
                auto pinst = (cs_insn*)inst;
                auto ctx = (S_Shutdown_SearchContext*)context;

                if (!ctx->S_Shutdown_VA &&
                    ( address[0] == 0xE8 || address[0] == 0xE9))
                {
                    auto S_Shutdown_VA_candidate = (PUCHAR)pinst->detail->x86.operands[0].imm;

                    if (S_Shutdown_VA_candidate[0] == 0xE8)
                    {
                        ctx->S_Shutdown_VA = S_Shutdown_VA_candidate;
                    }
                }

                if (ctx->S_Shutdown_VA)
                    return TRUE;

                if (address[0] == 0xCC)
                    return TRUE;

                if (pinst->id == X86_INS_RET)
                    return TRUE;

                return FALSE;
            }, 0, &ctx);
        }
    }

    if (!S_Shutdown_VA)
    {
        if (g_iEngineType == ENGINE_SVENGINE)
        {
            S_Shutdown_VA = Search_Pattern(S_SHUTDOWN_SIG_SVENGINE, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
        {
            S_Shutdown_VA = Search_Pattern(S_SHUTDOWN_SIG_HL25, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC)
        {
            S_Shutdown_VA = Search_Pattern(S_SHUTDOWN_SIG_NEW, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
        {
            S_Shutdown_VA = Search_Pattern(S_SHUTDOWN_SIG_BLOB, DllInfo);
        }
    }

    gAudEngine.S_Shutdown = (decltype(gAudEngine.S_Shutdown))ConvertDllInfoSpace(S_Shutdown_VA, DllInfo, RealDllInfo);
    Sig_FuncNotFound(S_Shutdown);

    PVOID VOX_Shutdown_VA = GetCallAddress(S_Shutdown_VA);
    gAudEngine.VOX_Shutdown = (decltype(gAudEngine.VOX_Shutdown))ConvertDllInfoSpace(VOX_Shutdown_VA, DllInfo, RealDllInfo);
    Sig_FuncNotFound(VOX_Shutdown);
}

void Engine_FillAddress_S_FindName(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    if (gAudEngine.S_FindName)
        return;

    PVOID S_FindName_VA = nullptr;

    if (1)
    {
        /*
.text:01D97B9A 6A 00                                               push    0
.text:01D97B9C 50                                                  push    eax/edx
.text:01D97B9D E8 5E F4 FF FF                                      call    S_FindName
.text:01D97BA2 83 C4 08                                            add     esp, 8
.text:01D97BA5 85 C0                                               test    eax, eax
.text:01D97BA7 75 26                                               jnz     short loc_1D97BCF
.text:01D97BA9 8D 04 24                                            lea     eax, [esp+104h+var_104]
.text:01D97BAC 50                                                  push    eax
.text:01D97BAD 68 F0 D1 E6 01                                      push    offset aSSayReliableCa_0 ; "S_Say_Reliable: can't find sentence nam"...
.text:01D97BB2 E8 09 BF F6 FF                                      call    sub_1D03AC0
          */

        const char sigs[] = "S_Say_Reliable: can't find sentence";
        auto S_Say_Reliable_String = Search_Pattern_Data(sigs, DllInfo);
        if (!S_Say_Reliable_String)
            S_Say_Reliable_String = Search_Pattern_Rdata(sigs, DllInfo);
        if (S_Say_Reliable_String)
        {
            char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
            *(DWORD*)(pattern + 1) = (DWORD)S_Say_Reliable_String;
            auto S_Say_Reliable_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
            if (S_Say_Reliable_PushString)
            {
                char pattern2[] = "\x6A\x00\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
                auto S_FindName_Call = (PUCHAR)Search_Pattern_From_Size((S_Say_Reliable_PushString - 0x60), 0x60, pattern2);
                if (S_FindName_Call)
                {
                    S_FindName_VA = GetCallAddress(S_FindName_Call + 3);
                }
            }
        }
    }

    if (!S_FindName_VA)
    {
        if (g_iEngineType == ENGINE_SVENGINE)
        {
            S_FindName_VA = Search_Pattern(S_FINDNAME_SIG_SVENGINE, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
        {
            S_FindName_VA = Search_Pattern(S_FINDNAME_SIG_HL25, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC)
        {
            S_FindName_VA = Search_Pattern(S_FINDNAME_SIG_NEW, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
        {
            S_FindName_VA = Search_Pattern(S_FINDNAME_SIG_BLOB, DllInfo);
        }
    }

    gAudEngine.S_FindName = (decltype(gAudEngine.S_FindName))ConvertDllInfoSpace(S_FindName_VA, DllInfo, RealDllInfo);
    Sig_FuncNotFound(S_FindName);

    if (1)
    {
        typedef struct S_FindName_SearchContext_s
        {
            const mh_dll_info_t& DllInfo;
            const mh_dll_info_t& RealDllInfo;
        }S_FindName_SearchContext;

        S_FindName_SearchContext ctx = { DllInfo, RealDllInfo };

        g_pMetaHookAPI->DisasmRanges(S_FindName_VA, 0x120, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
            {
                auto pinst = (cs_insn*)inst;
                auto ctx = (S_FindName_SearchContext *)context;
                if (pinst->id == X86_INS_CMP && pinst->detail->x86.op_count == 2 &&
                    pinst->detail->x86.operands[0].type == X86_OP_REG &&
                    pinst->detail->x86.operands[1].type == X86_OP_MEM &&
                    (PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
                    (PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
                {

                    cl_servercount = (decltype(cl_servercount))pinst->detail->x86.operands[1].mem.disp;
                }

                if (cl_servercount)
                    return TRUE;

                if (address[0] == 0xCC)
                    return TRUE;

                if (pinst->id == X86_INS_RET)
                    return TRUE;

                return FALSE;
            }, 0, & ctx);

        Sig_VarNotFound(cl_servercount);
    }

}

void Engine_FillAddress_S_PrecacheSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    if (gAudEngine.S_PrecacheSound)
        return;

    PVOID S_PrecacheSound_VA = nullptr;

    /*
.text:01D210C8 E8 73 AC 06 00                                      call    S_PrecacheSound
.text:01D210CD 68 D8 D2 E3 01                                      push    offset aDebrisFlesh1Wa ; "debris/flesh1.wav"
.text:01D210D2 A3 30 83 D0 02                                      mov     dword_2D08330, eax
.text:01D210D7 E8 64 AC 06 00                                      call    S_PrecacheSound
          */
    const char sigs[] = "debris/flesh1.wav";
    auto S_PrecacheSound_String = Search_Pattern_Data(sigs, DllInfo);
    if (!S_PrecacheSound_String)
        S_PrecacheSound_String = Search_Pattern_Rdata(sigs, DllInfo);
    if (S_PrecacheSound_String)
    {
        char pattern[] = "\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xA3\x2A\x2A\x2A\x2A\xE8";
        *(DWORD*)(pattern + 6) = (DWORD)S_PrecacheSound_String;
        auto S_PrecacheSound_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
        if (S_PrecacheSound_PushString)
        {
            S_PrecacheSound_VA = GetCallAddress(S_PrecacheSound_PushString);
        }
    }

    if (!S_PrecacheSound_VA)
    {
        if (g_iEngineType == ENGINE_SVENGINE)
        {
            S_PrecacheSound_VA = Search_Pattern(S_PRECACHESOUND_SIG_SVENGINE, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
        {
            S_PrecacheSound_VA = Search_Pattern(S_PRECACHESOUND_SIG_HL25, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC)
        {
            S_PrecacheSound_VA = Search_Pattern(S_PRECACHESOUND_SIG_NEW, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
        {
            S_PrecacheSound_VA = Search_Pattern(S_PRECACHESOUND_SIG_BLOB, DllInfo);
        }
    }

    gAudEngine.S_PrecacheSound = (decltype(gAudEngine.S_PrecacheSound))ConvertDllInfoSpace(S_PrecacheSound_VA, DllInfo, RealDllInfo);
    Sig_FuncNotFound(S_PrecacheSound);
}

void Engine_FillAddress_S_LoadSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    if (gAudEngine.S_LoadSound)
        return;

    PVOID S_LoadSound_VA = nullptr;

    if (1)
    {
        /*
.text:01D98912 68 F0 52 ED 01                                      push    offset aSLoadsoundCoul ; "S_LoadSound: Couldn't load %s\n"
.text:01D98917 E8 24 70 F9 FF                                      call    sub_1D2F940
.text:01D9891C 83 C4 08                                            add     esp, 8
        */
        const char sigs[] = "S_LoadSound: Couldn't load %s";
        auto S_LoadSound_String = Search_Pattern_Data(sigs, DllInfo);
        if (!S_LoadSound_String)
            S_LoadSound_String = Search_Pattern_Rdata(sigs, DllInfo);
        if (S_LoadSound_String)
        {
            char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
            *(DWORD*)(pattern + 1) = (DWORD)S_LoadSound_String;
            auto S_LoadSound_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
            if (S_LoadSound_PushString)
            {
                S_LoadSound_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(S_LoadSound_PushString, 0x500, [](PUCHAR Candidate) {

                    if (Candidate[0] == 0x55 &&
                        Candidate[1] == 0x8B &&
                        Candidate[2] == 0xEC)
                    {
                        return TRUE;
                    }

                    if (Candidate[0] == 0x83 &&
                        Candidate[1] == 0xEC)
                    {
                        return TRUE;
                    }

                    //.text:01D98710 81 EC 48 05 00 00                                   sub     esp, 548h
                    if (Candidate[0] == 0x81 &&
                        Candidate[1] == 0xEC &&
                        Candidate[4] == 0x00 &&
                        Candidate[5] == 0x00)
                    {
                        return TRUE;
                    }

                    return FALSE;
                    });
            }
        }
    }

    if (!S_LoadSound_VA)
    {
        if (g_iEngineType == ENGINE_SVENGINE)
        {
            S_LoadSound_VA = Search_Pattern(S_LOADSOUND_SIG_SVENGINE, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
        {
            S_LoadSound_VA = Search_Pattern(S_LOADSOUND_SIG_HL25, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC)
        {
            S_LoadSound_VA = Search_Pattern(S_LOADSOUND_SIG_NEW, DllInfo);
            if (!S_LoadSound_VA)
                S_LoadSound_VA = Search_Pattern(S_LOADSOUND_SIG_NEW2, DllInfo);
            if (!S_LoadSound_VA)
                S_LoadSound_VA = Search_Pattern(S_LOADSOUND_SIG_BLOB, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
        {
            S_LoadSound_VA = Search_Pattern(S_LOADSOUND_SIG_BLOB, DllInfo);
        }
    }

    gAudEngine.S_LoadSound = (decltype(gAudEngine.S_LoadSound))ConvertDllInfoSpace(S_LoadSound_VA, DllInfo, RealDllInfo);
    Sig_FuncNotFound(S_LoadSound);
}

void Engine_FillAddress_SND_Spatialize(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    if (gAudEngine.SND_Spatialize)
        return;

    PVOID SND_Spatialize_VA = nullptr;

    if (!SND_Spatialize_VA)
    {
        if (g_iEngineType == ENGINE_SVENGINE)
        {
            SND_Spatialize_VA = Search_Pattern(SND_SPATIALIZE_SIG_SVENGINE, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
        {
            SND_Spatialize_VA = Search_Pattern(SND_SPATIALIZE_SIG_HL25, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC)
        {
            SND_Spatialize_VA = Search_Pattern(SND_SPATIALIZE_SIG_NEW, DllInfo);
            if (!SND_Spatialize_VA)
                SND_Spatialize_VA = Search_Pattern(SND_SPATIALIZE_SIG_BLOB, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
        {
            SND_Spatialize_VA = Search_Pattern(SND_SPATIALIZE_SIG_BLOB, DllInfo);
        }
    }

    gAudEngine.SND_Spatialize = (decltype(gAudEngine.SND_Spatialize))ConvertDllInfoSpace(SND_Spatialize_VA, DllInfo, RealDllInfo);
    Sig_FuncNotFound(SND_Spatialize);

    if (1)
    {
        char pattern[] = "\x85\xC0\x0F\x2A\x2A\x2A\x2A\x2A\x3B\x05\x2A\x2A\x2A\x2A\x0F\x2A\x2A\x2A\x2A\x2A";

        /*
.text:01D980C0                                     loc_1D980C0:                            ; CODE XREF: SND_Spatialize+1C↑j
.text:01D980C0 85 C0                                               test    eax, eax
.text:01D980C2 0F 8E A3 00 00 00                                   jle     loc_1D9816B
.text:01D980C8 3B 05 C8 AF 70 02                                   cmp     eax, cl_num_entities
.text:01D980CE 0F 8D 97 00 00 00                                   jge     loc_1D9816B
        */

        auto cl_num_entities_pattern = (PUCHAR)Search_Pattern_From_Size(SND_Spatialize_VA, 0x80, pattern);
        if (cl_num_entities_pattern)
        {
            cl_num_entities = (decltype(cl_num_entities))ConvertDllInfoSpace((*(PVOID*)(cl_num_entities_pattern + 10)), DllInfo, RealDllInfo);
        }
    }
    
    Sig_VarNotFound(cl_num_entities);
}

void Engine_FillAddress_S_StartDynamicSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    if (gAudEngine.S_StartDynamicSound)
        return;

    PVOID S_StartDynamicSound_VA = nullptr;

    if (1)
    {
        /*
.text:01D8C299 68 FC 45 E5 01                                      push    offset aSStartdynamics ; "S_StartDynamicSound: %s volume > 255"
.text:01D8C29E E8 ED 09 FA FF                                      call    sub_1D2CC90
.text:01D8C2A3 83 C4 08                                            add     esp, 8
        */
        const char sigs[] = "Warning: S_StartDynamicSound Ignored";
        auto S_StartDynamicSound_String = Search_Pattern_Data(sigs, DllInfo);
        if (!S_StartDynamicSound_String)
            S_StartDynamicSound_String = Search_Pattern_Rdata(sigs, DllInfo);
        if (S_StartDynamicSound_String)
        {
            char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
            *(DWORD*)(pattern + 1) = (DWORD)S_StartDynamicSound_String;
            auto S_StartDynamicSound_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
            if (S_StartDynamicSound_PushString)
            {
                S_StartDynamicSound_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(S_StartDynamicSound_PushString, 0x300, [](PUCHAR Candidate) {

                    if (Candidate[0] == 0x55 &&
                        Candidate[1] == 0x8B &&
                        Candidate[2] == 0xEC)
                    {
                        return TRUE;
                    }

                    if (Candidate[0] == 0x83 &&
                        Candidate[1] == 0xEC)
                    {
                        return TRUE;
                    }

                    return FALSE;
                });
            }
        }
    }

    if (!S_StartDynamicSound_VA)
    {
        if (g_iEngineType == ENGINE_SVENGINE)
        {
            S_StartDynamicSound_VA = Search_Pattern(S_STARTDYNAMICSOUND_SIG_SVENGINE, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
        {
            S_StartDynamicSound_VA = Search_Pattern(S_STARTDYNAMICSOUND_SIG_HL25, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC)
        {
            S_StartDynamicSound_VA = Search_Pattern(S_STARTDYNAMICSOUND_SIG_NEW, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
        {
            S_StartDynamicSound_VA = Search_Pattern(S_STARTDYNAMICSOUND_SIG_BLOB, DllInfo);
        }
    }

    gAudEngine.S_StartDynamicSound = (decltype(gAudEngine.S_StartDynamicSound))ConvertDllInfoSpace(S_StartDynamicSound_VA, DllInfo, RealDllInfo);
    Sig_FuncNotFound(S_StartDynamicSound);
}

void Engine_FillAddress_S_StartStaticSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    if (gAudEngine.S_StartStaticSound)
        return;

    PVOID S_StartStaticSound_VA = nullptr;

    if (1)
    {
        /*
.text:01D96FE6 68 C0 4D ED 01                                      push    offset aWarningSStarts ; "Warning: S_StartStaticSound Ignored, ca"...
.text:01D96FEB E8 50 89 F9 FF                                      call    sub_1D2F940
.text:01D96FF0 83 C4 04                                            add     esp, 4
        */
        const char sigs[] = "Warning: S_StartStaticSound Ignored";
        auto S_StartStaticSound_String = Search_Pattern_Data(sigs, DllInfo);
        if (!S_StartStaticSound_String)
            S_StartStaticSound_String = Search_Pattern_Rdata(sigs, DllInfo);
        if (S_StartStaticSound_String)
        {
            char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
            *(DWORD*)(pattern + 1) = (DWORD)S_StartStaticSound_String;
            auto S_StartStaticSound_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
            if (S_StartStaticSound_PushString)
            {
                S_StartStaticSound_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(S_StartStaticSound_PushString, 0x300, [](PUCHAR Candidate) {

                    if (Candidate[0] == 0x55 &&
                        Candidate[1] == 0x8B &&
                        Candidate[2] == 0xEC)
                    {
                        return TRUE;
                    }

                    if (Candidate[0] == 0x83 &&
                        Candidate[1] == 0xEC)
                    {
                        return TRUE;
                    }

                    return FALSE;
                });
            }
        }
    }

    if (!S_StartStaticSound_VA)
    {
        if (g_iEngineType == ENGINE_SVENGINE)
        {
            S_StartStaticSound_VA = Search_Pattern(S_STARTSTATICSOUND_SIG_SVENGINE, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
        {
            S_StartStaticSound_VA = Search_Pattern(S_STARTSTATICSOUND_SIG_HL25, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC)
        {
            S_StartStaticSound_VA = Search_Pattern(S_STARTSTATICSOUND_SIG_NEW, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
        {
            S_StartStaticSound_VA = Search_Pattern(S_STARTSTATICSOUND_SIG_BLOB, DllInfo);
        }
    }
    gAudEngine.S_StartStaticSound = (decltype(gAudEngine.S_StartStaticSound))ConvertDllInfoSpace(S_StartStaticSound_VA, DllInfo, RealDllInfo);
    Sig_FuncNotFound(S_StartStaticSound);
}

void Engine_FillAddress_S_StopSound(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    if (gAudEngine.S_StopSound)
        return;

    PVOID S_StopSound_VA = nullptr;

    /*
.data:01ED73A4 FC 13 E6 01                                         dd offset aSvcStopsound ; "svc_stopsound"
.data:01ED73A8 20 F5 D2 01                                         dd offset CL_StopSound
.data:01ED73AC 11                                                  db  11h
.data:01ED73AD 00                                                  db    0
.data:01ED73AE 00                                                  db    0
.data:01ED73AF 00                                                  db    0
    */
    const char sigs[] = "svc_stopsound";
    auto svc_stopsound_String = Search_Pattern_Data(sigs, DllInfo);
    if (!svc_stopsound_String)
        svc_stopsound_String = Search_Pattern_Rdata(sigs, DllInfo);
    if (svc_stopsound_String)
    {
        char pattern[] = "\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x11\x00\x00\x00";
        *(DWORD*)(pattern + 0) = (DWORD)svc_stopsound_String;
        auto svc_stopsound_Struct = (PUCHAR)Search_Pattern_Data(pattern, DllInfo);
        if (svc_stopsound_Struct)
        {
            PVOID CL_StopSound_VA = *(decltype(CL_StopSound_VA)*)(svc_stopsound_Struct + 4);

            char pattern2[] = "\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\xC3";
            auto S_StopSound_Call = (PUCHAR)Search_Pattern_From_Size(CL_StopSound_VA, 0x50, pattern2);
            if (S_StopSound_Call)
            {
                S_StopSound_VA = GetCallAddress(S_StopSound_Call);
            }
        }
    }

    if (!S_StopSound_VA)
    {
        if (g_iEngineType == ENGINE_SVENGINE)
        {
            S_StopSound_VA = Search_Pattern(S_STOPSOUND_SIG_SVENGINE, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
        {
            S_StopSound_VA = Search_Pattern(S_STOPSOUND_SIG_HL25, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC)
        {
            S_StopSound_VA = Search_Pattern(S_STOPSOUND_SIG_NEW, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
        {
            S_StopSound_VA = Search_Pattern(S_STOPSOUND_SIG_BLOB, DllInfo);
        }
    }

    gAudEngine.S_StopSound = (decltype(gAudEngine.S_StopSound))ConvertDllInfoSpace(S_StopSound_VA, DllInfo, RealDllInfo);
    Sig_FuncNotFound(S_StopSound);
}

void Engine_FillAddress_S_StopAllSounds(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    if (gAudEngine.S_StopAllSounds)
        return;

    PVOID S_StopAllSounds_VA = nullptr;

    const char sigs[] = "spk\0";
    auto spk_String = Search_Pattern_Data(sigs, DllInfo);
    if (!spk_String)
        spk_String = Search_Pattern_Rdata(sigs, DllInfo);
    if (spk_String)
    {
        /*
.text:101FD726 68 48 27 2C 10                                      push    offset aSpk     ; "spk"
.text:101FD72B E8 70 79 FB FF                                      call    sub_101B50A0
.text:101FD730 68 F0 EB 1F 10                                      push    offset S_StopAllSoundsC
.text:101FD735 68 4C 27 2C 10                                      push    offset aStopsound ; "stopsound"
        */
        char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A";
        *(DWORD*)(pattern + 1) = (DWORD)spk_String;
        auto spk_PushString = (PUCHAR)Search_Pattern_Data(pattern, DllInfo);
        if (spk_PushString)
        {
            auto S_StopAllSoundsC_VA = (PUCHAR) *(ULONG_PTR*)(spk_PushString + 11);
            char pattern2[] = "\x6A\x01\xE8";

            if (memcmp(S_StopAllSoundsC_VA, pattern2, sizeof(pattern2) - 1) == 0)
            {
                S_StopAllSounds_VA = GetCallAddress(S_StopAllSoundsC_VA + 2);
            }
        }
    }

    if (!S_StopAllSounds_VA)
    {
        if (g_iEngineType == ENGINE_SVENGINE)
        {
            S_StopAllSounds_VA = Search_Pattern(S_SOUNDALLSOUNDS_SIG_SVENGINE, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
        {
            S_StopAllSounds_VA = Search_Pattern(S_SOUNDALLSOUNDS_SIG_HL25, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC)
        {
            S_StopAllSounds_VA = Search_Pattern(S_SOUNDALLSOUNDS_SIG_NEW, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
        {
            S_StopAllSounds_VA = Search_Pattern(S_SOUNDALLSOUNDS_SIG_BLOB, DllInfo);
        }
    }

    gAudEngine.S_StopAllSounds = (decltype(gAudEngine.S_StopAllSounds))ConvertDllInfoSpace(S_StopAllSounds_VA, DllInfo, RealDllInfo);
    Sig_FuncNotFound(S_StopAllSounds);
}

void Engine_FillAddress_S_Update(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    if (gAudEngine.S_Update)
        return;

    PVOID S_Update_VA = nullptr;

    /*
.text:101FEBC2                                     loc_101FEBC2:                           ; CODE XREF: S_Update+247
.text:101FEBC2 53                                                  push    ebx
.text:101FEBC3 68 14 19 2C 10                                      push    offset aI_1     ; "----(%i)----\n"
.text:101FEBC8 E8 B3 D2 FB FF                                      call    sub_101BBE80
    */
    const char sigs[] = "----(%i)----\n";
    auto S_Update_String = Search_Pattern_Data(sigs, DllInfo);
    if (!S_Update_String)
        S_Update_String = Search_Pattern_Rdata(sigs, DllInfo);
    if (S_Update_String)
    {
        char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
        *(DWORD*)(pattern + 1) = (DWORD)S_Update_String;
        auto S_Update_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
        if (S_Update_PushString)
        {
            S_Update_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(S_Update_PushString, 0x500, [](PUCHAR Candidate) {

                if (Candidate[0] == 0x55 &&
                    Candidate[1] == 0x8B &&
                    Candidate[2] == 0xEC)
                {
                    return TRUE;
                }

                if (Candidate[0] == 0x83 &&
                    Candidate[1] == 0x3D &&
                    Candidate[6] == 0x00)
                {
                    return TRUE;
                }

                if (Candidate[0] == 0xA1 &&
                    Candidate[5] == 0x85 &&
                    Candidate[6] == 0xC0)
                {
                    return TRUE;
                }

                return FALSE;
            });
        }
    }

    if (!S_Update_VA)
    {
        if (g_iEngineType == ENGINE_SVENGINE)
        {
            S_Update_VA = Search_Pattern(S_UPDATE_SIG_SVENGINE, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
        {
            S_Update_VA = Search_Pattern(S_UPDATE_SIG_HL25, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC)
        {
            S_Update_VA = Search_Pattern(S_UPDATE_SIG_NEW, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
        {
            S_Update_VA = Search_Pattern(S_UPDATE_SIG_BLOB, DllInfo);
        }
    }

    gAudEngine.S_Update = (decltype(gAudEngine.S_Update))ConvertDllInfoSpace(S_Update_VA, DllInfo, RealDllInfo);
    Sig_FuncNotFound(S_Update);
}

void Engine_FillAddress_SequenceGetSentenceByIndex(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    if (gAudEngine.SequenceGetSentenceByIndex)
        return;

    PVOID SequenceGetSentenceByIndex_VA = nullptr;

    if (1)
    {
        const char pattern[] = "\x50\xFF\x15\x2A\x2A\x2A\x2A\x50\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0";
        auto SequenceGetSentenceByIndex_Call = (PUCHAR)Search_Pattern(pattern, DllInfo);
        if (SequenceGetSentenceByIndex_Call)
        {
            SequenceGetSentenceByIndex_VA = GetCallAddress(SequenceGetSentenceByIndex_Call + 8);
        }
        else
        {
            const char pattern[] = "\x50\xE8\x2A\x2A\x2A\x2A\x50\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0";
            auto SequenceGetSentenceByIndex_Call = (PUCHAR)Search_Pattern(pattern, DllInfo);
            if (SequenceGetSentenceByIndex_Call)
            {
                SequenceGetSentenceByIndex_VA = GetCallAddress(SequenceGetSentenceByIndex_Call + 7);
            }
        }
    }

    if (!gAudEngine.SequenceGetSentenceByIndex)
    {
        if (g_iEngineType == ENGINE_SVENGINE)
        {
            SequenceGetSentenceByIndex_VA = Search_Pattern(SEQUENCE_GETSENTENCEBYINDEX_SIG_SVENGINE, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
        {
            SequenceGetSentenceByIndex_VA = Search_Pattern(SEQUENCE_GETSENTENCEBYINDEX_SIG_HL25, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC)
        {
            SequenceGetSentenceByIndex_VA = Search_Pattern(SEQUENCE_GETSENTENCEBYINDEX_SIG_NEW, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
        {
            SequenceGetSentenceByIndex_VA = Search_Pattern(SEQUENCE_GETSENTENCEBYINDEX_SIG_BLOB, DllInfo);
        }
    }

    gAudEngine.SequenceGetSentenceByIndex = (decltype(gAudEngine.SequenceGetSentenceByIndex))ConvertDllInfoSpace(SequenceGetSentenceByIndex_VA, DllInfo, RealDllInfo);
    Sig_FuncNotFound(SequenceGetSentenceByIndex);
}

void Engine_FillAddress_VoiceSE_Idle(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    if (gAudEngine.VoiceSE_Idle)
        return;

    PVOID VoiceSE_Idle_VA = nullptr;

    if (g_iEngineType == ENGINE_SVENGINE)
    {
        VoiceSE_Idle_VA = Search_Pattern(VOICESE_IDLE_SIG_SVENGINE, DllInfo);
    }
    else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
    {
        VoiceSE_Idle_VA = Search_Pattern(VOICESE_IDLE_SIG_HL25, DllInfo);
    }
    else if (g_iEngineType == ENGINE_GOLDSRC)
    {
        VoiceSE_Idle_VA = Search_Pattern(VOICESE_IDLE_SIG_NEW, DllInfo);
        if (!VoiceSE_Idle_VA)
            VoiceSE_Idle_VA = Search_Pattern(VOICESE_IDLE_SIG_BLOB, DllInfo);
    }
    else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
    {
        VoiceSE_Idle_VA = Search_Pattern(VOICESE_IDLE_SIG_BLOB, DllInfo);
    }

    gAudEngine.VoiceSE_Idle = (decltype(gAudEngine.VoiceSE_Idle))ConvertDllInfoSpace(VoiceSE_Idle_VA, DllInfo, RealDllInfo);
    Sig_FuncNotFound(VoiceSE_Idle);

    PVOID g_SND_VoiceOverdrive_VA = nullptr;

    typedef struct
    {
        PVOID &g_SND_VoiceOverdrive_VA;
        bool ret_found;
        const mh_dll_info_t& DllInfo;
        const mh_dll_info_t& RealDllInfo;
    }VoiceSE_Idle_SearchContext;

    VoiceSE_Idle_SearchContext ctx = { g_SND_VoiceOverdrive_VA, false, DllInfo, RealDllInfo };

    g_pMetaHookAPI->DisasmRanges(VoiceSE_Idle_VA, 0x1000, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
        {
            auto pinst = (cs_insn*)inst;
            auto ctx = (VoiceSE_Idle_SearchContext*)context;

            if (1)
            {
                if (pinst->id == X86_INS_FSTP && pinst->detail->x86.op_count == 1 &&
                    pinst->detail->x86.operands[0].type == X86_OP_MEM &&
                    pinst->detail->x86.operands[0].mem.base == 0)
                {
                    ctx->g_SND_VoiceOverdrive_VA = (PVOID)pinst->detail->x86.operands[0].mem.disp;
                }

                if (pinst->id == X86_INS_MOVSS && pinst->detail->x86.op_count == 2 &&
                    pinst->detail->x86.operands[0].type == X86_OP_MEM &&
                    (PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
                    (PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
                {
                    ctx->g_SND_VoiceOverdrive_VA = (PVOID)pinst->detail->x86.operands[0].mem.disp;
                }
            }

            if (address[0] == 0xCC)
                return TRUE;

            if (pinst->id == X86_INS_RET)
            {
                ctx->ret_found = true;
                return TRUE;
            }

            return FALSE;
        }, 0, &ctx);

    if (ctx.ret_found && ctx.g_SND_VoiceOverdrive_VA)
    {
        g_SND_VoiceOverdrive = (decltype(g_SND_VoiceOverdrive))ConvertDllInfoSpace(g_SND_VoiceOverdrive_VA, DllInfo, RealDllInfo);
    }

    Sig_VarNotFound(g_SND_VoiceOverdrive);
}

void Engine_FillAddress_VoiceSE_NotifyFreeChannel(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    if (gAudEngine.VoiceSE_NotifyFreeChannel)
        return;

    PVOID VoiceSE_NotifyFreeChannel_VA = nullptr;

    if (g_iEngineType == ENGINE_SVENGINE)
    {
        VoiceSE_NotifyFreeChannel_VA = Search_Pattern(VOICESE_NOTIFYFREECHANNEL_SIG_SVENGINE, DllInfo);

        if (VoiceSE_NotifyFreeChannel_VA)
        {
            VoiceSE_NotifyFreeChannel_VA = GetCallAddress((PUCHAR)VoiceSE_NotifyFreeChannel_VA + Sig_Length(VOICESE_NOTIFYFREECHANNEL_SIG_SVENGINE) - 1);
        }

    }
    else
    {
        VoiceSE_NotifyFreeChannel_VA = Search_Pattern(VOICESE_NOTIFYFREECHANNEL_SIG_GOLDSRC, DllInfo);

        if (VoiceSE_NotifyFreeChannel_VA)
        {
            VoiceSE_NotifyFreeChannel_VA = GetCallAddress((PUCHAR)VoiceSE_NotifyFreeChannel_VA + Sig_Length(VOICESE_NOTIFYFREECHANNEL_SIG_GOLDSRC) - 1);
        }
    }

    gAudEngine.VoiceSE_NotifyFreeChannel = (decltype(gAudEngine.VoiceSE_NotifyFreeChannel))ConvertDllInfoSpace(VoiceSE_NotifyFreeChannel_VA, DllInfo, RealDllInfo);
    Sig_FuncNotFound(VoiceSE_NotifyFreeChannel);
}

void Engine_FillAddress_CL_ViewEntityVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    /*
        //Global pointers that link into engine vars
        int *cl_viewentity = NULL;
    */

    if (g_iEngineType == ENGINE_SVENGINE)
    {
#define CL_VIEWENTITY_SIG_SVENGINE "\x68\x2A\x2A\x2A\x2A\x50\x6A\x06\xFF\x35\x2A\x2A\x2A\x2A\xE8"
        auto addr = (PUCHAR)Search_Pattern_From_Size((void*)DllInfo.TextBase, DllInfo.TextSize, CL_VIEWENTITY_SIG_SVENGINE);
        Sig_AddrNotFound(cl_viewentity);
        PVOID cl_viewentity_VA = *(PVOID*)(addr + 10);
        cl_viewentity = (decltype(cl_viewentity))ConvertDllInfoSpace(cl_viewentity_VA, DllInfo, RealDllInfo);
    }
    else
    {
#define CL_VIEWENTITY_SIG_GOLDSRC "\xA1\x2A\x2A\x2A\x2A\x48\x3B\x2A"
        auto addr = (PUCHAR)Search_Pattern_From_Size((void*)DllInfo.TextBase, DllInfo.TextSize, CL_VIEWENTITY_SIG_GOLDSRC);
        Sig_AddrNotFound(cl_viewentity);

        typedef struct CL_ViewEntity_SearchContext_s
        {
            const mh_dll_info_t& DllInfo;
            bool found_cmp_200{};
        } CL_ViewEntity_SearchContext;

        CL_ViewEntity_SearchContext ctx = { DllInfo };

        g_pMetaHookAPI->DisasmRanges((void*)addr, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

            auto pinst = (cs_insn*)inst;
            auto ctx = (CL_ViewEntity_SearchContext*)context;

            if (pinst->id == X86_INS_CMP &&
                pinst->detail->x86.op_count == 2 &&
                pinst->detail->x86.operands[0].type == X86_OP_MEM &&
                (PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
                (PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
                pinst->detail->x86.operands[1].type == X86_OP_IMM &&
                pinst->detail->x86.operands[1].imm == 0x200)
            {
                ctx->found_cmp_200 = true;
            }

            if (ctx->found_cmp_200)
                return TRUE;

            if (address[0] == 0xCC)
                return TRUE;

            if (pinst->id == X86_INS_RET)
                return TRUE;

            return FALSE;
            }, 0, &ctx);

        if (ctx.found_cmp_200)
        {
            PVOID cl_viewentity_VA = *(PVOID*)(addr + 1);
            cl_viewentity = (decltype(cl_viewentity))ConvertDllInfoSpace(cl_viewentity_VA, DllInfo, RealDllInfo);
        }
    }

    Sig_VarNotFound(cl_viewentity);
}

void Engine_FillAddress_R_DrawTEntitiesOnList(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    if (gAudEngine.R_DrawTEntitiesOnList)
        return;

    PVOID R_DrawTEntitiesOnList_VA = 0;

    {
        const char sigs[] = "Non-sprite set to glow";
        auto NonSprite_String = Search_Pattern_Data(sigs, DllInfo);
        if (!NonSprite_String)
            NonSprite_String = Search_Pattern_Rdata(sigs, DllInfo);

        if (NonSprite_String)
        {
            char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x8B";
            *(DWORD*)(pattern + 1) = (DWORD)NonSprite_String;
            auto NonSprite_Call = Search_Pattern(pattern, DllInfo);

            if (NonSprite_Call)
            {
                R_DrawTEntitiesOnList_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(NonSprite_Call, 0x500, [](PUCHAR Candidate) {

                    if (Candidate[0] == 0xD9 &&
                        Candidate[1] == 0x05 &&
                        Candidate[6] == 0xD8)
                        return TRUE;

                    if (Candidate[0] == 0x55 &&
                        Candidate[1] == 0x8B &&
                        Candidate[2] == 0xEC)
                        return TRUE;

                    return FALSE;
                    });
            }
        }
    }

    if (!R_DrawTEntitiesOnList_VA)
    {
        if (g_iEngineType == ENGINE_SVENGINE)
        {
            R_DrawTEntitiesOnList_VA = Search_Pattern(R_DRAWTENTITIESONLIST_SIG_SVENGINE, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
        {
            R_DrawTEntitiesOnList_VA = Search_Pattern(R_DRAWTENTITIESONLIST_SIG_HL25, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC)
        {
            R_DrawTEntitiesOnList_VA = Search_Pattern(R_DRAWTENTITIESONLIST_SIG_NEW, DllInfo);

            if (!R_DrawTEntitiesOnList_VA)
                R_DrawTEntitiesOnList_VA = Search_Pattern(R_DRAWTENTITIESONLIST_SIG_NEW2, DllInfo);
        }
        else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
        {
            R_DrawTEntitiesOnList_VA = Search_Pattern(R_DRAWTENTITIESONLIST_SIG_BLOB, DllInfo);
        }
    }

    gAudEngine.R_DrawTEntitiesOnList = (decltype(gAudEngine.R_DrawTEntitiesOnList))ConvertDllInfoSpace(R_DrawTEntitiesOnList_VA, DllInfo, RealDllInfo);

    Sig_FuncNotFound(R_DrawTEntitiesOnList);
}

void Engine_FillAddress_R_DrawTEntitiesOnListVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    /*
        //Global pointers that link into engine vars
        float* r_blend = NULL;
        void *cl_frames = NULL;
        int *cl_parsecount = NULL;

        //Global vars
        int size_of_frame = sizeof(frame_t);
    */
    PVOID R_DrawTEntitiesOnList_VA = ConvertDllInfoSpace(gAudEngine.R_DrawTEntitiesOnList, RealDllInfo, DllInfo);

    if (g_dwEngineBuildnum <= 8684)
    {
        size_of_frame = 0x42B8;
    }

    typedef struct R_DrawTEntitiesOnList_SearchContext_s
    {
        const mh_dll_info_t& DllInfo;
        const mh_dll_info_t& RealDllInfo;
        int disableFog_instcount{};
        int parsemod_instcount{};
        int getskin_instcount{};
        int r_entorigin_candidate_count{};
        int push2300_instcount{};
        int ClientDLL_DrawTransparentTriangles_candidate_instcount{};
        ULONG_PTR r_entorigin_candidateVA[3]{};
    } R_DrawTEntitiesOnList_SearchContext;

    R_DrawTEntitiesOnList_SearchContext ctx = { DllInfo, RealDllInfo };

    g_pMetaHookAPI->DisasmRanges(R_DrawTEntitiesOnList_VA, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

        auto pinst = (cs_insn*)inst;
        auto ctx = (R_DrawTEntitiesOnList_SearchContext*)context;

        if (pinst->id == X86_INS_PUSH &&
            pinst->detail->x86.op_count == 1 &&
            pinst->detail->x86.operands[0].imm == 0xB60)
        {//.text:01D92330 68 60 0B 00 00 push    0B60h

            ctx->disableFog_instcount = instCount;
        }

        if (address[0] == 0x6A && address[1] == 0x00 && address[2] == 0xE8)
        {
            //6A 00 push    0
            //E8 A3 13 05 00                                      call    GL_EnableDisableFog

            auto callTarget = GetCallAddress((address + 2));

            typedef struct GL_EnableDisableFog_SearchContext_s
            {
                bool bFoundGL_FOG{};
            } GL_EnableDisableFog_SearchContext;

            GL_EnableDisableFog_SearchContext ctx2 = { };

            g_pMetaHookAPI->DisasmRanges(callTarget, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

                auto pinst = (cs_insn*)inst;
                auto ctx2 = (GL_EnableDisableFog_SearchContext*)context;

                if (pinst->id == X86_INS_PUSH &&
                    pinst->detail->x86.op_count == 1 &&
                    pinst->detail->x86.operands[0].imm == 0xB60)
                {//.text:01D92330 68 60 0B 00 00 push    0B60h

                    ctx2->bFoundGL_FOG = instCount;
                }

                return FALSE;

                }, 0, &ctx2);

            if (ctx2.bFoundGL_FOG)
            {
                ctx->disableFog_instcount = instCount;
            }
        }

        if (pinst->id == X86_INS_MOV &&
            pinst->detail->x86.op_count == 2 &&
            pinst->detail->x86.operands[0].type == X86_OP_REG &&
            pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
            pinst->detail->x86.operands[1].type == X86_OP_MEM &&
            pinst->detail->x86.operands[1].mem.base == 0 &&
            (PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
            (PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
        {
            //.text:01D923D9 A1 DC 72 ED 01                                      mov     eax, cl_parsemod
            //.text:01D88CBB A1 CC AF E3 01                                      mov     eax, cl_parsemod
            DWORD value = *(DWORD*)pinst->detail->x86.operands[1].mem.disp;
            if (value == 63)
            {
                ctx->parsemod_instcount = instCount;
            }
        }
        else if (!cl_parsecount && ctx->parsemod_instcount &&
            instCount < ctx->parsemod_instcount + 3 &&
            (pinst->id == X86_INS_MOV || pinst->id == X86_INS_AND) &&
            pinst->detail->x86.op_count == 2 &&
            pinst->detail->x86.operands[0].type == X86_OP_REG &&
            pinst->detail->x86.operands[1].type == X86_OP_MEM &&
            pinst->detail->x86.operands[1].mem.base == 0 &&
            (PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
            (PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
        {
            //.text:01D923DE 23 05 AC D2 30 02                                   and     eax, cl_parsecount
            //.text:01D88CC0 8B 0D 04 AE D8 02                                   mov     ecx, cl_parsecount
            cl_parsecount = (decltype(cl_parsecount))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
        }
        else if (!cl_frames && ctx->parsemod_instcount &&
            instCount < ctx->parsemod_instcount + 20 &&
            pinst->id == X86_INS_LEA &&
            pinst->detail->x86.op_count == 2 &&
            pinst->detail->x86.operands[0].type == X86_OP_REG &&
            pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
            pinst->detail->x86.operands[1].type == X86_OP_MEM &&
            pinst->detail->x86.operands[1].mem.base != 0 &&
            (PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
            (PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
        {
            //.text:01D923F0 8D 80 F4 D5 30 02                                   lea     eax, cl_frames[eax]
            //.text:01D88CE8 8D 84 CA 4C B1 D8 02                                lea     eax, cl_frames_1[edx+ecx*8]
            cl_frames = (decltype(cl_frames))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
        }
        else if (ctx->parsemod_instcount &&
            instCount < ctx->parsemod_instcount + 5 &&
            pinst->id == X86_INS_IMUL &&
            pinst->detail->x86.op_count == 3 &&
            pinst->detail->x86.operands[0].type == X86_OP_REG &&
            pinst->detail->x86.operands[1].type == X86_OP_REG &&
            pinst->detail->x86.operands[2].type == X86_OP_IMM &&
            pinst->detail->x86.operands[2].imm > 0x4000 &&
            pinst->detail->x86.operands[2].imm < 0xF000)
        {
            //.text:01D923E4 69 C8 D8 84 00 00                                   imul    ecx, eax, 84D8h
            size_of_frame = pinst->detail->x86.operands[2].imm;
        }
        else if (
            pinst->id == X86_INS_MOVSX &&
            pinst->detail->x86.op_count == 2 &&
            pinst->detail->x86.operands[0].type == X86_OP_REG &&
            pinst->detail->x86.operands[0].size == 4 &&
            pinst->detail->x86.operands[1].type == X86_OP_MEM &&
            pinst->detail->x86.operands[1].size == 2 &&
            pinst->detail->x86.operands[1].mem.base != 0 &&
            pinst->detail->x86.operands[1].mem.disp == 0x2E8)
        {
            //.text:01D924D9 0F BF 83 E8 02 00 00                                movsx   eax, word ptr [ebx+2E8h]
            ctx->getskin_instcount = instCount;
        }

        if (ctx->getskin_instcount &&
            instCount < ctx->getskin_instcount + 20 &&
            pinst->id == X86_INS_MOV &&
            pinst->detail->x86.op_count == 2 &&
            pinst->detail->x86.operands[0].type == X86_OP_MEM &&
            pinst->detail->x86.operands[0].mem.base == 0 &&
            (PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
            (PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
        {
            //.text:01D88C23 89 15 E0 98 BC 02                                   mov     r_entorigin, edx
            auto candidateVA = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
            if (ctx->r_entorigin_candidate_count < 3)
            {
                bool bFound = false;
                for (auto k = 0; k < ctx->r_entorigin_candidate_count; ++k)
                {
                    if (ctx->r_entorigin_candidateVA[k] == candidateVA)
                        bFound = true;
                }
                if (!bFound)
                {
                    ctx->r_entorigin_candidateVA[ctx->r_entorigin_candidate_count] = candidateVA;
                    ctx->r_entorigin_candidate_count++;
                }
            }
        }

        if (ctx->getskin_instcount &&
            instCount < ctx->getskin_instcount + 20 &&
            pinst->id == X86_INS_FST &&
            pinst->detail->x86.op_count == 1 &&
            pinst->detail->x86.operands[0].type == X86_OP_MEM &&
            pinst->detail->x86.operands[0].mem.base == 0 &&
            (PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
            (PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
        {
            //.text:01D88C23 89 15 E0 98 BC 02                                   mov     r_entorigin, edx
            auto candidateVA = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
            if (ctx->r_entorigin_candidate_count < 3)
            {
                bool bFound = false;
                for (auto k = 0; k < ctx->r_entorigin_candidate_count; ++k)
                {
                    if (ctx->r_entorigin_candidateVA[k] == candidateVA)
                        bFound = true;
                }
                if (!bFound)
                {
                    ctx->r_entorigin_candidateVA[ctx->r_entorigin_candidate_count] = candidateVA;
                    ctx->r_entorigin_candidate_count++;
                }
            }
        }

        if (ctx->getskin_instcount &&
            instCount < ctx->getskin_instcount + 20 &&
            pinst->id == X86_INS_MOVSS &&
            pinst->detail->x86.op_count == 2 &&
            pinst->detail->x86.operands[1].type == X86_OP_REG &&
            pinst->detail->x86.operands[0].type == X86_OP_MEM &&
            pinst->detail->x86.operands[0].mem.base == 0 &&
            (PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
            (PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
        {
            //.text:101FA69B F3 0F 10 00                                         movss   xmm0, dword ptr[eax]
            //.text : 101FA69F F3 0F 11 05 E0 02 DC 10                             movss   r_entorigin, xmm0
            auto candidateVA = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
            if (ctx->r_entorigin_candidate_count < 3)
            {
                bool bFound = false;
                for (auto k = 0; k < ctx->r_entorigin_candidate_count; ++k)
                {
                    if (ctx->r_entorigin_candidateVA[k] == candidateVA)
                        bFound = true;
                }
                if (!bFound)
                {
                    ctx->r_entorigin_candidateVA[ctx->r_entorigin_candidate_count] = candidateVA;
                    ctx->r_entorigin_candidate_count++;
                }
            }
        }


        if (cl_parsecount && cl_frames && ctx->r_entorigin_candidate_count >= 3)
            return TRUE;

        if (address[0] == 0xCC)
            return TRUE;

        if (pinst->id == X86_INS_RET)
            return TRUE;

        return FALSE;
        }, 0, &ctx);

    Sig_VarNotFound(cl_frames);
    Sig_VarNotFound(cl_parsecount);
    Sig_VarNotFound(size_of_frame);
}

void Engine_FillAddress_VOX_LookupString(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    /*

        char *(*rgpszrawsentence)[CVOXFILESENTENCEMAX] = NULL;
        int *cszrawsentences = NULL;
    */
#define VOX_LOOKUPSTRING_SIG "\x80\x2A\x23\x2A\x2A\x8D\x2A\x01\x50\xE8"
#define VOX_LOOKUPSTRING_SIG_HL25 "\x80\x3B\x23\x0F\x85\x90\x00\x00\x00"
    if (1)
    {
        PVOID addr = NULL;

        if (g_iEngineType == ENGINE_GOLDSRC_HL25)
            addr = Search_Pattern(VOX_LOOKUPSTRING_SIG_HL25, DllInfo);
        else
            addr = Search_Pattern(VOX_LOOKUPSTRING_SIG, DllInfo);

        if (addr)
        {
            typedef struct VOX_LookupString_SearchContext_s
            {
                const mh_dll_info_t& DllInfo;
                const mh_dll_info_t& RealDllInfo;
            } VOX_LookupString_SearchContext;

            VOX_LookupString_SearchContext ctx = { DllInfo, RealDllInfo };

            g_pMetaHookAPI->DisasmRanges(addr, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
                auto pinst = (cs_insn*)inst;
                auto ctx = (VOX_LookupString_SearchContext*)context;

                if (g_iEngineType == ENGINE_SVENGINE)
                {
                    if (!cszrawsentences &&
                        pinst->id == X86_INS_CMP &&
                        pinst->detail->x86.op_count == 2 &&
                        pinst->detail->x86.operands[0].type == X86_OP_MEM &&
                        pinst->detail->x86.operands[0].mem.base == 0 &&
                        pinst->detail->x86.operands[0].mem.index == 0 &&
                        (PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
                        (PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
                        pinst->detail->x86.operands[1].type == X86_OP_REG &&
                        pinst->detail->x86.operands[1].size == 4)
                    {
                        //.text:01D99D06 39 35 18 A2 E0 08                                            cmp     cszrawsentences, esi
                        cszrawsentences = (decltype(cszrawsentences))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
                    }


                    if (!rgpszrawsentence &&
                        pinst->id == X86_INS_PUSH &&
                        pinst->detail->x86.op_count == 1 &&
                        pinst->detail->x86.operands[0].type == X86_OP_MEM &&
                        pinst->detail->x86.operands[0].mem.base == 0 &&
                        pinst->detail->x86.operands[0].mem.index != 0 &&
                        (PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
                        (PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
                        pinst->detail->x86.operands[0].mem.scale == 4)
                    {
                        //.text:01D99D10 FF 34 B5 18 82 E0 08                                         push    rgpszrawsentence[esi*4]
                        rgpszrawsentence = (decltype(rgpszrawsentence))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
                    }

                }
                else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
                {
                    if (!cszrawsentences &&
                        pinst->id == X86_INS_CMP &&
                        pinst->detail->x86.op_count == 2 &&
                        pinst->detail->x86.operands[0].type == X86_OP_MEM &&
                        pinst->detail->x86.operands[0].mem.base == 0 &&
                        pinst->detail->x86.operands[0].mem.index == 0 &&
                        (PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
                        (PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
                        pinst->detail->x86.operands[1].type == X86_OP_REG &&
                        pinst->detail->x86.operands[1].size == 4)
                    {
                        //.text:1020233E 39 35 9C FC 52 10											cmp     cszrawsentences, esi
                        cszrawsentences = (decltype(cszrawsentences))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
                    }


                    if (!rgpszrawsentence &&
                        pinst->id == X86_INS_PUSH &&
                        pinst->detail->x86.op_count == 1 &&
                        pinst->detail->x86.operands[0].type == X86_OP_MEM &&
                        pinst->detail->x86.operands[0].mem.base == 0 &&
                        pinst->detail->x86.operands[0].mem.index != 0 &&
                        (PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
                        (PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
                        pinst->detail->x86.operands[0].mem.scale == 4)
                    {
                        //.text:01D99D10 FF 34 B5 18 82 E0 08                                         push    rgpszrawsentence[esi*4]
                        rgpszrawsentence = (decltype(rgpszrawsentence))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
                    }

                }
                else
                {
                    if (!cszrawsentences &&
                        pinst->id == X86_INS_MOV &&
                        pinst->detail->x86.op_count == 2 &&
                        pinst->detail->x86.operands[1].type == X86_OP_MEM &&
                        pinst->detail->x86.operands[1].mem.base == 0 &&
                        pinst->detail->x86.operands[1].mem.index == 0 &&
                        (PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
                        (PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
                        pinst->detail->x86.operands[0].type == X86_OP_REG)
                    {
                        //.text:01D90EF9 A1 48 B2 3B 02                                      mov     eax, cszrawsentences
                        cszrawsentences = (decltype(cszrawsentences))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
                    }

                    if (!rgpszrawsentence &&
                        pinst->id == X86_INS_MOV &&
                        pinst->detail->x86.op_count == 2 &&
                        pinst->detail->x86.operands[1].type == X86_OP_MEM &&
                        pinst->detail->x86.operands[1].mem.base == 0 &&
                        pinst->detail->x86.operands[1].mem.index != 0 &&
                        (PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
                        (PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
                        pinst->detail->x86.operands[1].mem.scale == 4 &&
                        pinst->detail->x86.operands[0].type == X86_OP_REG)
                    {
                        //.text:01D90F04 8B 0C B5 00 34 72 02                                mov     ecx, rgpszrawsentence[esi*4]
                        rgpszrawsentence = (decltype(rgpszrawsentence))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
                    }
                }


                if (cszrawsentences && rgpszrawsentence)
                    return TRUE;

                if (address[0] == 0xCC)
                    return TRUE;

                return FALSE;

                }, 0, &ctx);
        }

        Sig_VarNotFound(cszrawsentences);
        Sig_VarNotFound(rgpszrawsentence);
    }
}

void Engine_FillAddress_SX_RoomFX(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    PVOID SX_RoomFX_VA = nullptr;

    char pattern[] = "\x6A\x01\x6A\x01\x68\x00\x02\x00\x00\xE8\x2A\x2A\x2A\x2A\x83\xC4\x0C";

    PUCHAR SX_RoomFX_Call = (PUCHAR)Search_Pattern(pattern, DllInfo);

    Sig_VarNotFound(SX_RoomFX_Call);

    SX_RoomFX_VA = GetCallAddress(SX_RoomFX_Call + 9);

    typedef struct SX_RoomFX_SearchContext_s
    {
        const mh_dll_info_t& DllInfo;
        const mh_dll_info_t& RealDllInfo;

        PVOID cl_waterlevel_candidateVA;
        int cl_waterlevel_reg;
        int cl_waterlevel_instCount;
    }SX_RoomFX_SearchContext;

    SX_RoomFX_SearchContext ctx = { DllInfo, RealDllInfo, nullptr, 0 ,0 };

    g_pMetaHookAPI->DisasmRanges(SX_RoomFX_VA, 0x120, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
        {
            auto pinst = (cs_insn*)inst;
            auto ctx = (SX_RoomFX_SearchContext*)context;
            if (!cl_waterlevel &&
                pinst->id == X86_INS_CMP && pinst->detail->x86.op_count == 2 &&
                pinst->detail->x86.operands[0].type == X86_OP_MEM &&
                (PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
                (PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
                pinst->detail->x86.operands[1].type == X86_OP_IMM &&
                pinst->detail->x86.operands[1].imm == 2)
            {
                cl_waterlevel = (decltype(cl_waterlevel))pinst->detail->x86.operands[0].mem.disp;
            }

            if (!cl_waterlevel &&
                pinst->id == X86_INS_MOV && pinst->detail->x86.op_count == 2 &&
                pinst->detail->x86.operands[0].type == X86_OP_REG &&
                pinst->detail->x86.operands[1].type == X86_OP_MEM &&
                (PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
                (PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
            {
                ctx->cl_waterlevel_candidateVA = (PVOID)pinst->detail->x86.operands[1].mem.disp;
                ctx->cl_waterlevel_instCount = instCount;
                ctx->cl_waterlevel_reg = pinst->detail->x86.operands[0].reg;
            }

            if (!cl_waterlevel &&
               instCount > ctx->cl_waterlevel_instCount &&
                instCount < ctx->cl_waterlevel_instCount + 3 &&
                pinst->id == X86_INS_CMP && pinst->detail->x86.op_count == 2 &&
                pinst->detail->x86.operands[0].type == X86_OP_REG &&
                pinst->detail->x86.operands[0].reg == ctx->cl_waterlevel_reg &&
                pinst->detail->x86.operands[1].type == X86_OP_IMM &&
                pinst->detail->x86.operands[1].imm == 2)
            {
                cl_waterlevel = (decltype(cl_waterlevel))ConvertDllInfoSpace(ctx->cl_waterlevel_candidateVA,ctx->DllInfo, ctx->RealDllInfo);
            }

            if (cl_waterlevel)
                return TRUE;

            if (address[0] == 0xCC)
                return TRUE;

            if (pinst->id == X86_INS_RET)
                return TRUE;

            return FALSE;
        }, 0, &ctx);


    Sig_VarNotFound(cl_servercount);
}

void Engine_FillAddress_GetClientTime(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
    PVOID GetClientTime_VA = ConvertDllInfoSpace(gEngfuncs.GetClientTime, RealDllInfo, DllInfo);

    ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(GetClientTime_VA, 0x20, "\xDD\x05");
    Sig_AddrNotFound("cl_time");

    PVOID cl_time_VA = *(PVOID*)(addr + 2);

    cl_time = (double*)ConvertDllInfoSpace(cl_time_VA, DllInfo, RealDllInfo);
    cl_oldtime = cl_time + 1;
}

void Engine_FillAddress(const mh_dll_info_t & DllInfo, const mh_dll_info_t & RealDllInfo)
{
    Engine_FillAddress_S_Init(DllInfo, RealDllInfo);
    Engine_FillAddress_S_Startup(DllInfo, RealDllInfo);
    Engine_FillAddress_S_Shutdown(DllInfo, RealDllInfo);
    Engine_FillAddress_S_FindName(DllInfo, RealDllInfo);
    Engine_FillAddress_S_PrecacheSound(DllInfo, RealDllInfo);
    Engine_FillAddress_S_LoadSound(DllInfo, RealDllInfo);
    Engine_FillAddress_SND_Spatialize(DllInfo, RealDllInfo);
    Engine_FillAddress_S_StartDynamicSound(DllInfo, RealDllInfo);
    Engine_FillAddress_S_StartStaticSound(DllInfo, RealDllInfo);
    Engine_FillAddress_S_StopSound(DllInfo, RealDllInfo);
    Engine_FillAddress_S_StopAllSounds(DllInfo, RealDllInfo);
    Engine_FillAddress_S_Update(DllInfo, RealDllInfo);
    Engine_FillAddress_SequenceGetSentenceByIndex(DllInfo, RealDllInfo);
    Engine_FillAddress_VoiceSE_Idle(DllInfo, RealDllInfo);
    Engine_FillAddress_VoiceSE_NotifyFreeChannel(DllInfo, RealDllInfo);
    Engine_FillAddress_CL_ViewEntityVars(DllInfo, RealDllInfo);
    Engine_FillAddress_R_DrawTEntitiesOnList(DllInfo, RealDllInfo);
    Engine_FillAddress_R_DrawTEntitiesOnListVars(DllInfo, RealDllInfo);
    Engine_FillAddress_VOX_LookupString(DllInfo, RealDllInfo);
    Engine_FillAddress_SX_RoomFX(DllInfo, RealDllInfo);
    Engine_FillAddress_GetClientTime(DllInfo, RealDllInfo);
}

static MetaAudio::AudioEngine* p_engine = nullptr;
static MetaAudio::SoundLoader* p_loader = nullptr;

//S_Startup has been inlined into S_Init in the new HL25 engine. use SNDDMA_Init instead
//static void S_Startup() { p_engine->S_Startup(); }
static int SNDDMA_Init() { return p_engine->SNDDMA_Init(); }
static void S_Init() { p_engine->S_Init(); }
static void S_Shutdown() { p_engine->S_Shutdown(); }
static sfx_t* S_FindName(char* name, int* pfInCache) { return p_engine->S_FindName(name, pfInCache); }
static void S_StartDynamicSound(int entnum, int entchannel, sfx_t* sfx, float* origin, float fvol, float attenuation, int flags, int pitch) { p_engine->S_StartDynamicSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch); };
static void S_StartStaticSound(int entnum, int entchannel, sfx_t* sfx, float* origin, float fvol, float attenuation, int flags, int pitch) { p_engine->S_StartStaticSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch); };
static void S_StopSound(int entnum, int entchannel) { p_engine->S_StopSound(entnum, entchannel); };
static void S_StopAllSounds(qboolean clear) { p_engine->S_StopAllSounds(clear); }
static void S_Update(float* origin, float* forward, float* right, float* up) { p_engine->S_Update(origin, forward, right, up); }
static aud_sfxcache_t* S_LoadSound(sfx_t* s, aud_channel_t* ch) { return p_loader->S_LoadSound(s, ch); }

static hook_t* g_phook_SNDDMA_Init = NULL;
static hook_t* g_phook_S_Init = NULL;
static hook_t* g_phook_S_Shutdown = NULL;
static hook_t* g_phook_S_FindName = NULL;
static hook_t* g_phook_S_StartDynamicSound = NULL;
static hook_t* g_phook_S_StartStaticSound = NULL;
static hook_t* g_phook_S_StopSound = NULL;
static hook_t* g_phook_S_StopAllSounds = NULL;
static hook_t* g_phook_S_Update = NULL;
static hook_t* g_phook_S_LoadSound = NULL;

void S_InstallHook(MetaAudio::AudioEngine* engine, MetaAudio::SoundLoader* loader)
{
  p_engine = engine;
  p_loader = loader;
  Install_InlineHook(SNDDMA_Init);
  Install_InlineHook(S_Init);
  Install_InlineHook(S_Shutdown);
  Install_InlineHook(S_FindName);
  Install_InlineHook(S_StartDynamicSound);
  Install_InlineHook(S_StartStaticSound);
  Install_InlineHook(S_StopSound);
  Install_InlineHook(S_StopAllSounds);
  Install_InlineHook(S_Update);
  Install_InlineHook(S_LoadSound);
}

void S_UninstallHook()
{
    Uninstall_Hook(SNDDMA_Init);
    Uninstall_Hook(S_Init);
    Uninstall_Hook(S_Shutdown);
    Uninstall_Hook(S_FindName);
    Uninstall_Hook(S_StartDynamicSound);
    Uninstall_Hook(S_StartStaticSound);
    Uninstall_Hook(S_StopSound);
    Uninstall_Hook(S_StopAllSounds);
    Uninstall_Hook(S_Update);
    Uninstall_Hook(S_LoadSound);
}

PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo)
{
    if ((ULONG_PTR)addr > (ULONG_PTR)SrcDllInfo.ImageBase && (ULONG_PTR)addr < (ULONG_PTR)SrcDllInfo.ImageBase + SrcDllInfo.ImageSize)
    {
        auto addr_VA = (ULONG_PTR)addr;
        auto addr_RVA = RVA_from_VA(addr, SrcDllInfo);

        return (PVOID)VA_from_RVA(addr, TargetDllInfo);
    }

    return nullptr;
}

PVOID GetVFunctionFromVFTable(PVOID* vftable, int index, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo, const mh_dll_info_t& OutputDllInfo)
{
    if ((ULONG_PTR)vftable > (ULONG_PTR)RealDllInfo.ImageBase && (ULONG_PTR)vftable < (ULONG_PTR)RealDllInfo.ImageBase + RealDllInfo.ImageSize)
    {
        ULONG_PTR vftable_VA = (ULONG_PTR)vftable;
        ULONG vftable_RVA = RVA_from_VA(vftable, RealDllInfo);
        auto vftable_DllInfo = (decltype(vftable))VA_from_RVA(vftable, DllInfo);

        auto vf_VA = (ULONG_PTR)vftable_DllInfo[index];
        ULONG vf_RVA = RVA_from_VA(vf, DllInfo);

        return (PVOID)VA_from_RVA(vf, OutputDllInfo);
    }
    else if ((ULONG_PTR)vftable > (ULONG_PTR)DllInfo.ImageBase && (ULONG_PTR)vftable < (ULONG_PTR)DllInfo.ImageBase + DllInfo.ImageSize)
    {
        auto vf_VA = (ULONG_PTR)vftable[index];
        ULONG vf_RVA = RVA_from_VA(vf, DllInfo);

        return (PVOID)VA_from_RVA(vf, OutputDllInfo);
    }

    return vftable[index];
}