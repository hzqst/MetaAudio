cmake_minimum_required(VERSION 3.27)

project(MetaAudio)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Configure Alure2 the way we need it
option(METAAUDIO_STATIC_ALURE "Link Alure2 statically" ON)
if(METAAUDIO_STATIC_ALURE)
    set(ALURE2_LIBRARY alure2_s)
    option(ALURE_BUILD_SHARED "Shared Alure disabled" OFF)
else()
    set(ALURE2_LIBRARY alure2)
    option(ALURE_BUILD_STATIC "Static Alure disabled" OFF)
endif()
option(ALURE_ENABLE_MINIMP3 "" OFF)
option(ALURE_ENABLE_WAVE "" OFF)
option(ALURE_ENABLE_OPUS "" OFF)
option(ALURE_ENABLE_VORBIS "" OFF)
option(ALURE_ENABLE_FLAC "" OFF)
option(ALURE_INSTALL "" OFF)
option(ALURE_BUILD_EXAMPLES "" OFF)

# cleanup libSndFile testing configuration
option(BUILD_EXAMPLES "" OFF)
option(BUILD_PROGRAMS "" OFF)
option(BUILD_TESTING "" OFF)
option(INSTALL_MANPAGES "" OFF)

# Collect all source files
set(SOURCES
    src/exportfuncs.cpp
    src/metaaudio.cpp
    src/Effects/EnvEffects.cpp
    src/Loaders/SteamAudioMapMeshLoader.cpp
    src/snd_hook.cpp
    src/Voice/VoiceDecoder.cpp
    src/Vox/VoxManager.cpp
    src/Loaders/LocalAudioDecoder.cpp
    src/Utilities/AudioCache.cpp
    src/Config/EfxJsonReader.cpp
    src/Config/SettingsManager.cpp
    src/Effects/GoldSrcOcclusionCalculator.cpp
    src/Effects/SteamAudioOcclusionCalculator.cpp
    src/Utilities/Fade.cpp
    src/Workarounds/NoWorkarounds.cpp
    src/Workarounds/XFiWorkarounds.cpp
    src/Loaders/GoldSrcFileBuf.cpp
    src/Loaders/GoldSrcFileStream.cpp
    src/Loaders/GoldSrcFileFactory.cpp
    src/Loaders/SoundLoader.cpp
    src/AudioEngine.cpp
    src/Utilities/ChannelManager.cpp
    src/snd_local.cpp
    src/SoundSources/SoundSourceFactory.cpp
    src/SoundSources/BaseSoundSource.cpp
    src/SoundSources/StaticSoundSource.cpp
    src/SoundSources/StreamingSoundSource.cpp
    # Use interface.cpp from HLSDK
    ${PROJECT_SOURCE_DIR}/externals/MetaHookSv/include/HLSDK/common/interface.cpp
)

# Collect all header files from include directory
file(GLOB_RECURSE HEADERS 
    "${PROJECT_SOURCE_DIR}/include/*.h"
    "${PROJECT_SOURCE_DIR}/include/*.hpp"
)

add_library(MetaAudio SHARED ${SOURCES} ${HEADERS})

set_property(TARGET MetaAudio PROPERTY CXX_STANDARD 17)

# Copy MSVC configuration files to build directory for Visual Studio projects
if(MSVC)
    set(MSVC_CONFIG_FILES
        "${PROJECT_SOURCE_DIR}/msvc/Directory.build.props"
        "${PROJECT_SOURCE_DIR}/msvc/Directory.build.targets"
        "${PROJECT_SOURCE_DIR}/msvc/MetaHookSv.props"
    )
    foreach(CONFIG_FILE ${MSVC_CONFIG_FILES})
        get_filename_component(FILE_NAME ${CONFIG_FILE} NAME)
        configure_file(${CONFIG_FILE} "${CMAKE_BINARY_DIR}/${FILE_NAME}" COPYONLY)
    endforeach()
endif()

target_compile_definitions(MetaAudio PRIVATE AUDIO_EXPORTS)
add_compile_definitions(NOMINMAX PICOJSON_USE_INT64)

target_include_directories(MetaAudio
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/externals/picojson
        ${PROJECT_SOURCE_DIR}/externals/steamaudio/include
        ${PROJECT_SOURCE_DIR}/externals/alure/include/AL
        ${PROJECT_SOURCE_DIR}/externals/MetaHookSv/include
        ${PROJECT_SOURCE_DIR}/externals/MetaHookSv/include/Interface
        ${PROJECT_SOURCE_DIR}/externals/MetaHookSv/include/HLSDK/common
        ${PROJECT_SOURCE_DIR}/externals/MetaHookSv/include/HLSDK/cl_dll
        ${PROJECT_SOURCE_DIR}/externals/MetaHookSv/include/HLSDK/engine
        ${PROJECT_SOURCE_DIR}/externals/MetaHookSv/include/HLSDK/public
        ${PROJECT_SOURCE_DIR}/externals/MetaHookSv/include/HLSDK/pm_shared
        ${PROJECT_SOURCE_DIR}/externals/MetaHookSv/thirdparty/capstone_fork/include/capstone
)

# Alure requires OpenAL
set(OPENAL_BINARY ${PROJECT_SOURCE_DIR}/externals/OpenAL-SDK/bin/Win32/OpenAL32.dll)
set(OPENAL_LIBRARY ${PROJECT_SOURCE_DIR}/externals/OpenAL-SDK/libs/Win32/OpenAL32.lib)
set(OPENAL_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/externals/OpenAL-SDK/include)

find_package(OpenAL REQUIRED)

# Alure requires libSndFile
# Configure libsndfile to build as shared library
set(BUILD_SHARED_LIBS ON)
add_subdirectory(externals/libsndfile-fork)
# Reset BUILD_SHARED_LIBS to avoid affecting other dependencies
set(BUILD_SHARED_LIBS OFF)

# Set SndFile variables for Alure's find_package to succeed
# libsndfile-fork creates a target 'sndfile' that we can reference
set(SNDFILE_LIBRARY sndfile)
set(SNDFILE_LIBRARIES sndfile)
set(SNDFILE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/externals/libsndfile-fork/include)
set(SNDFILE_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/externals/libsndfile-fork/include)
set(SNDFILE_FOUND TRUE)

# MetaAudio requires Alure
add_subdirectory(externals/alure)

if ((MSVC) AND (MSVC_VERSION GREATER_EQUAL 1914))
    # Force C++17 as mpark/variant doesn't compile with MSVC
    set_property(TARGET ${ALURE2_LIBRARY} PROPERTY CXX_STANDARD 17)
    target_compile_options(${ALURE2_LIBRARY} PUBLIC "/Zc:__cplusplus")
    target_compile_options(MetaAudio PUBLIC "/Zc:__cplusplus")
endif()

# Ensure proper build order dependencies
add_dependencies(${ALURE2_LIBRARY} sndfile)
add_dependencies(MetaAudio ${ALURE2_LIBRARY})

target_link_libraries(MetaAudio PRIVATE ${OPENAL_LIBRARY} ${ALURE2_LIBRARY} sndfile)

# Install MetaAudio.dll, libsndfile.dll and OpenAL32.dll to bin directory
install(TARGETS MetaAudio sndfile
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install OpenAL32.dll
install(FILES ${OPENAL_BINARY}
    DESTINATION bin
)