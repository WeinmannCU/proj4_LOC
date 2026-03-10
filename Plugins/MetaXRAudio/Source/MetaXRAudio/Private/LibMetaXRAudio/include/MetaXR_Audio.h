// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/********************************************************************************/ /**
 \file      MetaXR_Audio.h
 \brief     OVR Audio SDK public header file
 ************************************************************************************/

#ifndef OVR_Audio_h
#define OVR_Audio_h

#include "MetaXRAudioTypes.h"
#include "MetaXR_Audio_Defaults.h"
#include "MetaXR_Audio_Errors.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OVR_AUDIO_MAJOR_VERSION 1
#define OVR_AUDIO_MINOR_VERSION 115
#define OVR_AUDIO_PATCH_VERSION 0

// Typedefs for backwards compatibility with OVR naming
typedef MetaXRAudioInitFlags ovrAudioInitFlags;
typedef MetaXRAudioSourceFlag ovrAudioSourceFlag;
typedef MetaXRAudioSourceAttenuationMode ovrAudioSourceAttenuationMode;
typedef MetaXRAudioEnable ovrAudioEnable;
typedef MetaXRAudioAcousticModel ovrAudioAcousticModel;
typedef MetaXRAudioSpatializationStatus ovrAudioSpatializationStatus;
typedef MetaXRAudioAmbisonicFormat ovrAudioAmbisonicFormat;
typedef MetaXRAudioAmbisonicRenderMode ovrAudioAmbisonicRenderMode;
typedef MetaXRAudioMaterialPreset ovrAudioMaterialPreset;
typedef MetaXRAudioVector3f ovrAudioVector3f;
typedef MetaXRAudioBands ovrAudioBands;
typedef MetaXRAudioSource ovrAudioSource;
#ifndef OVR_CONTEXT_DEFINED
#define OVR_CONTEXT_DEFINED
typedef struct MetaXRAudioContext_ ovrAudioContext_;
typedef ovrAudioContext_* ovrAudioContext;
#endif
typedef MetaXRAudioAmbisonicStream ovrAudioAmbisonicStream;
typedef MetaXRAudioSpectrumAnalyzer ovrAudioSpectrumAnalyzer;
typedef MetaXRAudioAllocateCallback ovrAudioAllocateCallback;
typedef MetaXRAudioDeallocateCallback ovrAudioDeallocateCallback;
typedef MetaXRAudioContextConfiguration ovrAudioContextConfiguration;
typedef MetaXRAudioBoxRoomParameters ovrAudioBoxRoomParameters;
typedef MetaXRAudioAdvancedBoxRoomParameters ovrAudioAdvancedBoxRoomParameters;
typedef METAXRAUDIO_RAYCAST_CALLBACK OVRA_RAYCAST_CALLBACK;

// Constants for backward compatibility
#define ovrAudioEnable_COUNT MetaXRAudioEnable_COUNT
#define ovrAudioAcousticModel_COUNT MetaXRAudioAcousticModel_COUNT
#define ovrAudioMaterialPreset_COUNT MetaXRAudioMaterialPreset_COUNT

// Individual enum constants for backward compatibility
#define ovrAudioInitFlag_Default MetaXRAudioInitFlag_Default
#define ovrAudioInitFlag_ThreadUnsafe MetaXRAudioInitFlag_ThreadUnsafe
#define ovrAudioInitFlag_LazyAllocations MetaXRAudioInitFlag_LazyAllocations

#define ovrAudioSourceFlag_None MetaXRAudioSourceFlag_None
#define ovrAudioSourceFlag_WideBand_HINT MetaXRAudioSourceFlag_WideBand_HINT
#define ovrAudioSourceFlag_NarrowBand_HINT MetaXRAudioSourceFlag_NarrowBand_HINT
#define ovrAudioSourceFlag_BassCompensation_DEPRECATED MetaXRAudioSourceFlag_BassCompensation_DEPRECATED
#define ovrAudioSourceFlag_DirectTimeOfArrival MetaXRAudioSourceFlag_DirectTimeOfArrival
#define ovrAudioSourceFlag_ReflectionsDisabled MetaXRAudioSourceFlag_ReflectionsDisabled
#define ovrAudioSourceFlag_MediumAbsorption MetaXRAudioSourceFlag_MediumAbsorption
#define ovrAudioSourceFlag_DisableResampling_RESERVED MetaXRAudioSourceFlag_DisableResampling_RESERVED

#define ovrAudioSourceAttenuationMode_None MetaXRAudioSourceAttenuationMode_None
#define ovrAudioSourceAttenuationMode_Fixed MetaXRAudioSourceAttenuationMode_Fixed
#define ovrAudioSourceAttenuationMode_InverseSquare MetaXRAudioSourceAttenuationMode_InverseSquare
#define ovrAudioSourceAttenuationMode_COUNT MetaXRAudioSourceAttenuationMode_COUNT

#define ovrAudioEnable_None MetaXRAudioEnable_None
#define ovrAudioEnable_EarlyReflections MetaXRAudioEnable_EarlyReflections
#define ovrAudioEnable_LateReverberation MetaXRAudioEnable_LateReverberation
#define ovrAudioEnable_RandomizeReverb MetaXRAudioEnable_RandomizeReverb
#define ovrAudioEnable_PerformanceCounters MetaXRAudioEnable_PerformanceCounters
#define ovrAudioEnable_Diffraction MetaXRAudioEnable_Diffraction

#define ovrAudioAcousticModel_Automatic MetaXRAudioAcousticModel_Automatic
#define ovrAudioAcousticModel_None MetaXRAudioAcousticModel_None
#define ovrAudioAcousticModel_Shoebox MetaXRAudioAcousticModel_Shoebox
#define ovrAudioAcousticModel_DynamicRoomModeling MetaXRAudioAcousticModel_DynamicRoomModeling
#define ovrAudioAcousticModel_AcousticRayTracing MetaXRAudioAcousticModel_AcousticRayTracing

#define ovrAudioSpatializationStatus_None MetaXRAudioSpatializationStatus_None
#define ovrAudioSpatializationStatus_Finished MetaXRAudioSpatializationStatus_Finished
#define ovrAudioSpatializationStatus_Working MetaXRAudioSpatializationStatus_Working

#define ovrAudioAmbisonicFormat_FuMa MetaXRAudioAmbisonicFormat_FuMa
#define ovrAudioAmbisonicFormat_AmbiX MetaXRAudioAmbisonicFormat_AmbiX

#define ovrAudioAmbisonicRenderMode_SphericalHarmonics MetaXRAudioAmbisonicRenderMode_SphericalHarmonics
#define ovrAudioAmbisonicRenderMode_Mono MetaXRAudioAmbisonicRenderMode_Mono

// Error code mappings
#define ovrError_AudioInvalidParam MetaXRAudioError_InvalidParam
#define ovrError_AudioUninitialized MetaXRAudioError_Uninitialized
#define ovrError_AudioBadSampleRate MetaXRAudioError_BadSampleRate
#define ovrError_AudioMissingDLL MetaXRAudioError_MissingDLL
#define ovrError_AudioBadAlignment MetaXRAudioError_BadAlignment
#define ovrError_AudioHRTFInitFailure MetaXRAudioError_HRTFInitFailure
#define ovrError_AudioBadVersion MetaXRAudioError_BadVersion
#define ovrError_AudioSymbolNotFound MetaXRAudioError_SymbolNotFound
#define ovrError_AudioSharedReverbDisabled MetaXRAudioError_SharedReverbDisabled
#define ovrError_AudioBadAlloc MetaXRAudioError_BadAlloc
#define ovrError_AudioNoAvailableAmbisonicInstance MetaXRAudioError_NoAvailableAmbisonicInstance
#define ovrError_AudioMemoryAllocFailure MetaXRAudioError_MemoryAllocFailure
#define ovrError_AudioUnsupportedFeature MetaXRAudioError_UnsupportedFeature
#define ovrError_AudioInvalidAudioContext MetaXRAudioError_InvalidAudioContext
#define ovrError_AudioBadMesh MetaXRAudioError_BadMesh

// Material preset mappings
#define ovrAudioMaterialPreset_AcousticTile MetaXRAudioMaterialPreset_AcousticTile
#define ovrAudioMaterialPreset_Brick MetaXRAudioMaterialPreset_Brick
#define ovrAudioMaterialPreset_BrickPainted MetaXRAudioMaterialPreset_BrickPainted
#define ovrAudioMaterialPreset_Cardboard MetaXRAudioMaterialPreset_Cardboard
#define ovrAudioMaterialPreset_Carpet MetaXRAudioMaterialPreset_Carpet
#define ovrAudioMaterialPreset_CarpetHeavy MetaXRAudioMaterialPreset_CarpetHeavy
#define ovrAudioMaterialPreset_CarpetHeavyPadded MetaXRAudioMaterialPreset_CarpetHeavyPadded
#define ovrAudioMaterialPreset_CeramicTile MetaXRAudioMaterialPreset_CeramicTile
#define ovrAudioMaterialPreset_Concrete MetaXRAudioMaterialPreset_Concrete
#define ovrAudioMaterialPreset_ConcreteRough MetaXRAudioMaterialPreset_ConcreteRough
#define ovrAudioMaterialPreset_ConcreteBlock MetaXRAudioMaterialPreset_ConcreteBlock
#define ovrAudioMaterialPreset_ConcreteBlockPainted MetaXRAudioMaterialPreset_ConcreteBlockPainted
#define ovrAudioMaterialPreset_Curtain MetaXRAudioMaterialPreset_Curtain
#define ovrAudioMaterialPreset_Foliage MetaXRAudioMaterialPreset_Foliage
#define ovrAudioMaterialPreset_Glass MetaXRAudioMaterialPreset_Glass
#define ovrAudioMaterialPreset_GlassHeavy MetaXRAudioMaterialPreset_GlassHeavy
#define ovrAudioMaterialPreset_Grass MetaXRAudioMaterialPreset_Grass
#define ovrAudioMaterialPreset_Gravel MetaXRAudioMaterialPreset_Gravel
#define ovrAudioMaterialPreset_GypsumBoard MetaXRAudioMaterialPreset_GypsumBoard
#define ovrAudioMaterialPreset_Marble MetaXRAudioMaterialPreset_Marble
#define ovrAudioMaterialPreset_Mud MetaXRAudioMaterialPreset_Mud
#define ovrAudioMaterialPreset_PlasterOnBrick MetaXRAudioMaterialPreset_PlasterOnBrick
#define ovrAudioMaterialPreset_PlasterOnConcreteBlock MetaXRAudioMaterialPreset_PlasterOnConcreteBlock
#define ovrAudioMaterialPreset_Rubber MetaXRAudioMaterialPreset_Rubber
#define ovrAudioMaterialPreset_Soil MetaXRAudioMaterialPreset_Soil
#define ovrAudioMaterialPreset_SoundProof MetaXRAudioMaterialPreset_SoundProof
#define ovrAudioMaterialPreset_Snow MetaXRAudioMaterialPreset_Snow
#define ovrAudioMaterialPreset_Steel MetaXRAudioMaterialPreset_Steel
#define ovrAudioMaterialPreset_Stone MetaXRAudioMaterialPreset_Stone
#define ovrAudioMaterialPreset_Vent MetaXRAudioMaterialPreset_Vent
#define ovrAudioMaterialPreset_Water MetaXRAudioMaterialPreset_Water
#define ovrAudioMaterialPreset_WoodThin MetaXRAudioMaterialPreset_WoodThin
#define ovrAudioMaterialPreset_WoodThick MetaXRAudioMaterialPreset_WoodThick
#define ovrAudioMaterialPreset_WoodFloor MetaXRAudioMaterialPreset_WoodFloor
#define ovrAudioMaterialPreset_WoodOnConcrete MetaXRAudioMaterialPreset_WoodOnConcrete
#define ovrAudioMaterialPreset_MetaDefault MetaXRAudioMaterialPreset_MetaDefault

#ifdef __cplusplus
#define OVR_AUDIO_DEPRECATED [[deprecated]]
#else
#define OVR_AUDIO_DEPRECATED
#endif

/// DEPRECATED Initialize OVRAudio
OVR_AUDIO_DEPRECATED inline ovrResult ovrAudio_Initialize(void) {
  return MetaXRAudioSuccess;
}

/// DEPRECATED Shutdown OVRAudio
OVR_AUDIO_DEPRECATED inline void ovrAudio_Shutdown(void) {}

/** \brief A function pointer that allocates the specified number of bytes and returns a pointer to the memory. Return null if allocation
 * fails. */
typedef void* (*ovrAudioAllocateCallback)(size_t align, size_t size);
/** \brief A function pointer that deallocates a pointer previously returned from the allocation function. */
typedef void (*ovrAudioDeallocateCallback)(size_t align, void* data);

OVRA_EXPORT ovrResult ovrAudio_SetMemoryAllocatorCallbacks(ovrAudioAllocateCallback alloc, ovrAudioDeallocateCallback dealloc);

/// Return library's built version information.
///
/// Can be called any time.
/// \param[out] Major pointer to integer that accepts major version number
/// \param[out] Minor pointer to integer that accepts minor version number
/// \param[out] Patch pointer to integer that accepts patch version number
///
/// \return Returns a string with human readable build information
///
OVRA_EXPORT const char* ovrAudio_GetVersion(int* Major, int* Minor, int* Patch);

/// Allocate properly aligned buffer to store samples.
///
/// Helper function that allocates 16-byte aligned sample data sufficient
/// for passing to the spatialization APIs.
///
/// \param NumSamples number of samples to allocate
/// \return Returns pointer to 16-byte aligned float buffer, or NULL on failure
/// \see ovrAudio_FreeSamples
///
OVRA_EXPORT float* ovrAudio_AllocSamples(int NumSamples);

/// Free previously allocated buffer
///
/// Helper function that frees 16-byte aligned sample data previously
/// allocated by ovrAudio_AllocSamples.
///
/// \param Samples pointer to buffer previously allocated by ovrAudio_AllocSamples
/// \see ovrAudio_AllocSamples
///
OVRA_EXPORT void ovrAudio_FreeSamples(float* Samples);

/// Create an audio context for spatializing incoming sounds.
///
/// Creates an audio context with the given configuration.
///
/// \param pContext[out] pointer to store address of context.  NOTE: pointer must be pointing to NULL!
/// \param pConfig[in] pointer to configuration struct describing the desired context attributes
/// \return Returns an ovrResult indicating success or failure
/// \see ovrAudio_DestroyContext
/// \see ovrAudioContextConfiguration
///
OVRA_EXPORT ovrResult ovrAudio_CreateContext(ovrAudioContext* pContext, const ovrAudioContextConfiguration* pConfig);

OVRA_EXPORT ovrResult ovrAudio_InitializeContext(ovrAudioContext Context, const ovrAudioContextConfiguration* pConfig);

OVRA_EXPORT ovrResult ovrAudio_ShutdownContext(ovrAudioContext Context);

/// Destroy a previously created audio context.
///
/// \param[in] Context a valid audio context
/// \see ovrAudio_CreateContext
///
OVRA_EXPORT void ovrAudio_DestroyContext(ovrAudioContext Context);

/// Enable/disable options in the audio context. Can be called on uninitialized context (before ovrAudio_InitializeContext or after
/// ovrAudio_ShutdownContext)
///
/// \param Context context to use
/// \param What specific property to enable/disable
/// \param Enable 0 to disable, 1 to enable
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult ovrAudio_Enable(ovrAudioContext Context, ovrAudioEnable What, int Enable);

/// Query option status in the audio context. Can be called on uninitialized context (before ovrAudio_InitializeContext or after
/// ovrAudio_ShutdownContext)
///
/// \param Context context to use
/// \param What specific property to query
/// \param pEnabled addr of variable to receive the queried property status
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult ovrAudio_IsEnabled(ovrAudioContext Context, ovrAudioEnable What, int* pEnabled);

/// Given an ovrAudio context, get the maximum number of sources allowed.
///
/// \param Context context to use
/// \param pCount addr of variable to receive the queried source count
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult ovrAudio_GetMaxAudioSourceCount(ovrAudioContext Context, int* pCount);

/// Set the unit scale of game units relative to meters. (e.g. for centimeters set UnitScale = 0.01)
///
/// \param UnitScale[in] unit scale value relative to meters
///
OVRA_EXPORT ovrResult ovrAudio_SetUnitScale(ovrAudioContext Context, float UnitScale);

/// Get the unit scale of game units relative to meters.
///
/// \param UnitScale[out] unit scale value value relative to meters
///
OVRA_EXPORT ovrResult ovrAudio_GetUnitScale(ovrAudioContext Context, float* UnitScale);

/// Set box room parameters for reverberation.
///
/// These parameters are used for reverberation/early reflections if
/// ovrAudioEnable_EarlyReflections is enabled.
///
/// \param Context[in] context to use
/// \param Parameters[in] pointer to ovrAudioBoxRoomParameters describing box
/// \return Returns an ovrResult indicating success or failure
/// \see ovrAudioBoxRoomParameters
/// \see ovrAudio_Enable
///
OVR_AUDIO_DEPRECATED OVRA_EXPORT ovrResult
ovrAudio_SetSimpleBoxRoomParameters(ovrAudioContext Context, const ovrAudioBoxRoomParameters* Parameters);

/// Get box room parameters for current reverberation.
///
/// \param Context[in] context to use
/// \param Parameters[out] pointer to returned ovrAudioBoxRoomParameters box description
/// \return Returns an ovrResult indicating success or failure
/// \see ovrAudioBoxRoomParameters
/// \see ovrAudio_Enable
///
OVR_AUDIO_DEPRECATED OVRA_EXPORT ovrResult
ovrAudio_GetSimpleBoxRoomParameters(ovrAudioContext Context, ovrAudioBoxRoomParameters* Parameters);

/// Set advanced box room parameters for reverberation.
///
/// These parameters are used for reverberation/early reflections if
/// ovrAudioEnable_EarlyReflections is enabled.
///
/// \param Context[in] context to use
/// \param Parameters[in] const pointer to parameters
/// \return Returns an ovrResult indicating success or failure
/// \see ovrAudio_SetSimpleBoxRoomParameters
/// \see ovrAudio_Enable
///
OVRA_EXPORT ovrResult ovrAudio_SetAdvancedBoxRoomParameters(ovrAudioContext Context, const ovrAudioAdvancedBoxRoomParameters* Parameters);

/// Get advanced box room parameters for reverberation.
///
/// These parameters are used for reverberation/early reflections if
/// ovrAudioEnable_EarlyReflections is enabled.
///
/// \param Context[in] context to use
/// \param LockToListenerPosition[out] 1 - room is centered on listener, 0 - room center is specified by RoomPosition coordinates
/// \param RoomPositionX[out] desired X coordinate of room (if not locked to listener)
/// \param RoomPositionY[out] desired Y coordinate of room (if not locked to listener)
/// \param RoomPositionZ[out] desired Z coordinate of room (if not locked to listener)
/// \param WallMaterials[out] absorption coefficients for room materials
/// \return Returns an ovrResult indicating success or failure
/// \see ovrAudio_SetAdvancedBoxRoomParameters
/// \see ovrAudio_Enable
///
OVRA_EXPORT ovrResult ovrAudio_GetAdvancedBoxRoomParameters(ovrAudioContext Context, ovrAudioAdvancedBoxRoomParameters* Parameters);

/// Set clutter factors per band which simulates the addition of "clutter" (i.e. furniture,
/// windows blinds, etc.) in a room that is would otherwise be empty. For each band,
/// clutter factors range from 0 to 1 inclusive. A clutter factor of 0 represents a completely
/// empty room (and hence the largest T60 values for a given geometry) and a value of 1.0
/// represents the most clutter (and hence smallest T60 time).
/// \param Context[in] context to use
/// \param clutterFactor An AudioBands array of clutter factor coefficients per band.
/// \return ovrError_AudioInvalidParam if the context or clutterFactor arguments are null,
/// ovrSuccess if the clutterFactor output argument was updated.
/// \see ovrAudio_SetAdvancedBoxRoomParameters
/// \see ovrAudio_GetRoomClutterFactor
OVRA_EXPORT ovrResult ovrAudio_SetRoomClutterFactor(ovrAudioContext Context, ovrAudioBands clutterFactor);

/// Gets clutter factors per band which simulates the addition of "clutter" (i.e. furniture,
/// windows blinds, etc.) in a room that is would otherwise be empty. For each band,
/// clutter factors range from 0 to 1 inclusive. A clutter factor of 0 represents a completely
/// empty room (and hence the largest T60 values for a given geometry) and a value of 1.0
/// represents the most clutter (and hence smallest T60 time).
/// \param Context[in] context to use
/// \param clutterFactor An AudioBands array of clutter factor coefficients per band.
/// \return ovrError_AudioInvalidParam if the context or clutterFactor arguments are null,
/// set, ovrSuccess if the clutterFactor output argument was updated.
/// \see ovrAudio_SetAdvancedBoxRoomParameters
///
OVRA_EXPORT ovrResult ovrAudio_GetRoomClutterFactor(ovrAudioContext Context, ovrAudioBands clutterFactor);

/// Sets the listener's pose state as vectors, position is in game units (unit scale will be applied)
///
/// If this is not set then the listener is always assumed to be facing into
/// the screen (0,0,-1) at location (0,0,0) and that all spatialized sounds
/// are in listener-relative coordinates.
///
/// \param Context[in] context to use
/// \param PositionX[in] X position of listener on X axis
/// \param PositionY[in] Y position of listener on X axis
/// \param PositionZ[in] Z position of listener on X axis
/// \param ForwardX[in] X component of listener forward vector
/// \param ForwardY[in] Y component of listener forward vector
/// \param ForwardZ[in] Z component of listener forward vector
/// \param UpX[in] X component of listener up vector
/// \param UpY[in] Y component of listener up vector
/// \param UpZ[in] Z component of listener up vector
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult ovrAudio_SetListenerVectors(
    ovrAudioContext Context,
    float PositionX,
    float PositionY,
    float PositionZ,
    float ForwardX,
    float ForwardY,
    float ForwardZ,
    float UpX,
    float UpY,
    float UpZ);

/// Gets the listener's pose state as vectors
///
/// \param Context[in] context to use
/// \param pPositionX[in]: addr of X position of listener on X axis
/// \param pPositionY[in]: addr of Y position of listener on X axis
/// \param pPositionZ[in]: addr of Z position of listener on X axis
/// \param pForwardX[in]: addr of X component of listener forward vector
/// \param pForwardY[in]: addr of Y component of listener forward vector
/// \param pForwardZ[in]: addr of Z component of listener forward vector
/// \param pUpX[in]: addr of X component of listener up vector
/// \param pUpY[in]: addr of Y component of listener up vector
/// \param pUpZ[in]: addr of Z component of listener up vector
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult ovrAudio_GetListenerVectors(
    ovrAudioContext Context,
    float* pPositionX,
    float* pPositionY,
    float* pPositionZ,
    float* pForwardX,
    float* pForwardY,
    float* pForwardZ,
    float* pUpX,
    float* pUpY,
    float* pUpZ);

/// Reset an audio source's state.
///
/// Sometimes you need to reset an audio source's internal state due to a change
/// in the incoming sound or parameters.  For example, removing any reverb
/// tail since the incoming waveform has been swapped.
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult ovrAudio_ResetAudioSource(ovrAudioContext Context, int Sound);

/// Sets the position of an audio source in game units (unit scale will be applied).  Use "OVR" coordinate system (same as pose).
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param X position of sound on X axis
/// \param Y position of sound on Y axis
/// \param Z position of sound on Z axis
/// \return Returns an ovrResult indicating success or failure
/// \see ovrAudio_SetListenerVectors
/// \see ovrAudio_SetAudioSourceRange
///
OVRA_EXPORT ovrResult ovrAudio_SetAudioSourcePos(ovrAudioContext Context, int Sound, float X, float Y, float Z);

/// Gets the position of an audio source.  Use "OVR" coordinate system (same as pose).
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param pX address of position of sound on X axis
/// \param pY address of position of sound on Y axis
/// \param pZ address of position of sound on Z axis
/// \return Returns an ovrResult indicating success or failure
/// \see ovrAudio_SetListenerVectors
/// \see ovrAudio_SetAudioSourceRange
///
OVRA_EXPORT ovrResult ovrAudio_GetAudioSourcePos(ovrAudioContext Context, int Sound, float* pX, float* pY, float* pZ);

/// Sets the position of an audio source in game units (unit scale will be applied) and
/// the orientation.  Use "OVR" coordinate system (same as pose).
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param PositionX position of sound on X axis
/// \param PositionY position of sound on Y axis
/// \param PositionZ position of sound on Z axis
/// \param ForwardX Forward vector X component of sound.
/// \param ForwardY Forward vector Y component of sound.
/// \param ForwardZ Forward vector Z  componentof sound.
/// \param UpX Up vector X component of sound.
/// \param UpY position of sound on Y axis
/// \param UpZ position of sound on Z axis
/// \return ovrError_AudioInvalidParam if the context is null or the specified sound index
/// is < 0 or greater than the number of available sources or the positions specified are
/// not finite. ovrError_AudioUninitialized if the context has yet to be initialized.
/// ovrSuccess if the update was successful.
/// \see ovrAudio_SetListenerVectors
/// \see ovrAudio_SetAudioSourceRange
///
OVRA_EXPORT ovrResult ovrAudio_SetAudioSourceVectors(
    ovrAudioContext Context,
    int Sound,
    float PositionX,
    float PositionY,
    float PositionZ,
    float ForwardX,
    float ForwardY,
    float ForwardZ,
    float UpX,
    float UpY,
    float UpZ);

/// Enables directivity for a particular source. If enabled, objects facing away from the
/// listener are low-passed to simulate how directional sources behave. If disabled, the
/// source is considered a omnidirectional radiator.
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param Enabled Non-zero value to enable directivity for the specified source. Zero
/// value disables directivity.
/// \return ovrError_AudioInvalidParam if the context is null or the specified sound index
/// is < 0 or greater than the number of available sources. ovrError_AudioUninitialized if
/// the context has yet to be initialized. ovrSuccess if the update was successful.
/// \see ovrAudio_SetListenerVectors
/// \see ovrAudio_SetAudioSourceVectors
///
OVRA_EXPORT ovrResult ovrAudio_SetSourceDirectivityEnabled(ovrAudioContext Context, int Sound, int Enabled);

/// Gets the value of the enable flag for source directivity for a particular source. I
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param Enabled 1 if directivity is enabled for the source, 0 if not.
/// \return ovrError_AudioInvalidParam if the context is null or the specified sound index
/// is < 0 or greater than the number of available sources. ovrError_AudioUninitialized if
/// the context has yet to be initialized. ovrSuccess if the update was successful.
/// \see ovrAudio_SetListenerVectors
/// \see ovrAudio_SetAudioSourceVectors
///
OVRA_EXPORT ovrResult ovrAudio_GetSourceDirectivityEnabled(ovrAudioContext Context, int Sound, int* Enabled);

/// Sets the value of the enable flag for source direct sound enabled. "Direct sound" is in contrast to reflections and reverb.
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param Enabled 1 if direct sound is enabled for the source, 0 if not.
/// \return ovrError_AudioInvalidParam if the context is null or the specified sound index
/// is < 0 or greater than the number of available sources. ovrError_AudioUninitialized if
/// the context has yet to be initialized. ovrSuccess if the update was successful.
/// \see ovrAudio_SetListenerVectors
/// \see ovrAudio_SetAudioSourceVectors
///
OVRA_EXPORT ovrResult ovrAudio_SetSourceDirectEnabled(ovrAudioContext Context, int Sound, int Enabled);

/// Gets the value of the enable flag for source direct sound enabled. "Direct sound" is in contrast to reflections and reverb.
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param Enabled 1 if direct sound is enabled for the source, 0 if not.
/// \return ovrError_AudioInvalidParam if the context is null or the specified sound index
/// is < 0 or greater than the number of available sources. ovrError_AudioUninitialized if
/// the context has yet to be initialized. ovrSuccess if the update was successful.
/// \see ovrAudio_SetListenerVectors
/// \see ovrAudio_SetAudioSourceVectors
///
OVRA_EXPORT ovrResult ovrAudio_GetSourceDirectEnabled(ovrAudioContext Context, int Sound, int* Enabled);

/// Sets the Directivity intensity of a particular source. A value of 0 will make the source omnidirectional
/// and 1 will make it fully directional with the intended effect as described in the documentation for
/// ovrAudio_SetAudioSourceDirectivityEnabled

/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param intensity 1 for full directional source, 0 for omnidirectional
/// \return ovrError_AudioInvalidParam if the context is null or the specified sound index
/// is < 0 or greater than the number of available sources. ovrError_AudioUninitialized if
/// the context has yet to be initialized. ovrSuccess if the update was successful.
/// \see ovrAudio_SetSourceDirectivityEnabled
///
OVRA_EXPORT ovrResult ovrAudio_SetSourceDirectivityIntensity(ovrAudioContext Context, int Sound, float intensity);

/// Gets the Directivity intensity of a particular source.
/// \see ovrAudio_SetAudioSourceDirectivityIntenstiy
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param intensity 1 for full directional source, 0 for omnidirectional
/// \return ovrError_AudioInvalidParam if the context is null or the specified sound index
/// is < 0 or greater than the number of available sources. ovrError_AudioUninitialized if
/// the context has yet to be initialized. ovrSuccess if the update was successful.
/// \see ovrAudio_SetSourceDirectivityEnabled
///
OVRA_EXPORT ovrResult ovrAudio_GetSourceDirectivityIntensity(ovrAudioContext Context, int Sound, float* intensity);

/// Sets the min and max range of the audio source.
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param RangeMin min range in game units (full gain)
/// \param RangeMax max range in game units
/// \return Returns an ovrResult indicating success or failure
/// \see ovrAudio_SetListenerVectors
/// \see ovrAudio_SetAudioSourcePos
///
OVRA_EXPORT ovrResult ovrAudio_SetAudioSourceRange(ovrAudioContext Context, int Sound, float RangeMin, float RangeMax);

/// Gets the min and max range of the audio source.
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param pRangeMin addr of variable to receive the returned min range parameter (in meters).
/// \param pRangeMax addr of variable to receive the returned max range parameter (in meters).
/// \return Returns an ovrResult indicating success or failure
/// \see ovrAudio_SetListenerVectors
/// \see ovrAudio_SetAudioSourcePos
/// \see ovrAudio_SetAudioSourceRange
///
OVRA_EXPORT ovrResult ovrAudio_GetAudioSourceRange(ovrAudioContext Context, int Sound, float* pRangeMin, float* pRangeMax);

/// Sets the increase of the source's reverb with respect to distance from the source.
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param Reach a unitless value in the range [0,1] controlling how quickly reverb attenuates with distance.
/// A value of 0 causes reverb to attenuate with the direct sound (constant direct-to-reverberant ratio).
/// A value of 1 increases reverb level linearly with distance from the source, to counteract direct sound attenuation.
/// \return Returns an ovrResult indicating success or failure
/// \see ovrAudio_SetAudioSourceRange
///
OVRA_EXPORT ovrResult ovrAudio_SetAudioSourceReverbReach(ovrAudioContext Context, int Sound, float Reach);

/// Gets the increase of the source's reverb with respect to distance from the source.
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param pReach addr of variable to receive the returned reverb reach parameter in the range [0,1].
/// \return Returns an ovrResult indicating success or failure
/// \see ovrAudio_SetAudioSourceRange
///
OVRA_EXPORT ovrResult ovrAudio_GetAudioSourceReverbReach(ovrAudioContext Context, int Sound, float* pReach);

/// Sets the radius of the audio source for volumetric sound sources. Set a radius of 0 to make it a point source.
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param Radius source radius in game units (as determined by unit scale)
/// \return Returns an ovrResult indicating success or failure
/// \see ovrAudio_SetListenerVectors
/// \see ovrAudio_SetAudioSourcePos
///
OVRA_EXPORT ovrResult ovrAudio_SetAudioSourceRadius(ovrAudioContext Context, int Sound, float Radius);

/// Gets the radius of the audio source for volumetric sound sources.
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param pRadius addr of variable to receive the returned radius parameter in game units (as determined by unit scale)
/// \return Returns an ovrResult indicating success or failure
/// \see ovrAudio_SetListenerVectors
/// \see ovrAudio_SetAudioSourcePos
/// \see ovrAudio_SetAudioSourceRadius
///
OVRA_EXPORT ovrResult ovrAudio_GetAudioSourceRadius(ovrAudioContext Context, int Sound, float* pRadius);

/// Sets HRTF intensity for audio source
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param intensity in linear scale (0.0f to 1.0f)
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult ovrAudio_SetAudioHrtfIntensity(ovrAudioContext Context, int Sound, float Intensity);

/// Gets HRTF intensity for audio source
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param pIntensity addr of variable to receive the currently set intensity
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult ovrAudio_GetAudioHrtfIntensity(ovrAudioContext Context, int Sound, float* pIntensity);

/// Sets the reflections send level for audio source
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param Level send level in linear scale (0.0f to 1.0f)
/// \return Returns an ovrResult indicating success or failure
/// \see ovrAudio_SetListenerVectors
/// \see ovrAudio_SetAudioSourcePos
///
OVRA_EXPORT ovrResult ovrAudio_SetAudioReflectionsSendLevel(ovrAudioContext Context, int Sound, float Level);

/// Gets the the reflections send level for audio source
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param pLevel addr of variable to receive the currently set send level
/// \return Returns an ovrResult indicating success or failure
/// \see ovrAudio_SetListenerVectors
/// \see ovrAudio_SetAudioSourcePos
/// \see ovrAudio_SetAudioSourceRadius
///
OVRA_EXPORT ovrResult ovrAudio_GetAudioReflectionsSendLevel(ovrAudioContext Context, int Sound, float* pLevel);

/// Sets the reverb send level for audio source
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param Level send level in linear scale (0.0f to 1.0f)
/// \return Returns an ovrResult indicating success or failure
/// \see ovrAudio_SetListenerVectors
/// \see ovrAudio_SetAudioSourcePos
///
OVRA_EXPORT ovrResult ovrAudio_SetAudioReverbSendLevel(ovrAudioContext Context, int Sound, float Level);

/// Gets the the reverb send level for audio source
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param pLevel addr of variable to receive the currently set send level
/// \return Returns an ovrResult indicating success or failure
/// \see ovrAudio_SetListenerVectors
/// \see ovrAudio_SetAudioSourcePos
/// \see ovrAudio_SetAudioSourceRadius
///
OVRA_EXPORT ovrResult ovrAudio_GetAudioReverbSendLevel(ovrAudioContext Context, int Sound, float* pLevel);

/// Sets an audio source's flags.
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param Flags a logical OR of ovrAudioSourceFlag enumerants
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult ovrAudio_SetAudioSourceFlags(ovrAudioContext Context, int Sound, uint32_t Flags);

/// Gets an audio source's flags.
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param pFlags addr of returned flags (a logical OR of ovrAudioSourceFlag enumerants)
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult ovrAudio_GetAudioSourceFlags(ovrAudioContext Context, int Sound, uint32_t* pFlags);

/// Sets a single audio source flag.
///
/// This function allows toggling an individual flag without needing to first retrieve
/// all flags, modify them, and then set them back. More convenient than using
/// ovrAudio_SetAudioSourceFlags for single flag changes.
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param Flag the specific ovrAudioSourceFlag to modify
/// \param Enable 0 to disable the flag, 1 to enable it
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult ovrAudio_SetAudioSourceFlag(ovrAudioContext Context, int Sound, ovrAudioSourceFlag Flag, int Enable);

/// Gets a single audio source flag.
///
/// This function allows querying the state of an individual flag without needing to
/// retrieve all flags and manually check. Complements ovrAudio_SetAudioSourceFlag
/// for individual flag management.
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param Flag the specific ovrAudioSourceFlag to query
/// \param pEnabled pointer to receive flag state: 1 if enabled, 0 if disabled
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult ovrAudio_GetAudioSourceFlag(ovrAudioContext Context, int Sound, ovrAudioSourceFlag Flag, int* pEnabled);

/// Set the attenuation mode for a sound source.
///
/// Sounds can have their volume attenuated by distance based on different methods.
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param Mode attenuation mode to use
/// \param SourceGain attenuation constant used for fixed attenuation mode
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult
ovrAudio_SetAudioSourceAttenuationMode(ovrAudioContext Context, int Sound, ovrAudioSourceAttenuationMode Mode, float SourceGain);

/// Get the attenuation mode for a sound source.
///
/// Sounds can have their volume attenuated by distance based on different methods.
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param pMode addr of returned attenuation mode in use
/// \param pFixedScale addr of returned attenuation constant used for fixed attenuation mode
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult
ovrAudio_GetAudioSourceAttenuationMode(ovrAudioContext Context, int Sound, ovrAudioSourceAttenuationMode* pMode, float* pSourceGain);

/// Get the overall gain for a sound source.
///
/// The gain after all attenatuation is applied, this can be used for voice prioritization and virtualization
///
/// \param Context context to use
/// \param Sound index of sound (0..NumSources-1)
/// \param pMode addr of returned attenuation mode in use
/// \param pFixedScale addr of returned attenuation constant used for fixed attenuation mode
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult ovrAudio_GetAudioSourceOverallGain(ovrAudioContext Context, int Sound, float* Gain);

/// Spatialize a mono audio source to interleaved stereo output.
///
/// \param Context[in] context to use
/// \param Sound[in] index of sound (0..NumSources-1)
/// \param OutStatus[out] bitwise OR of flags indicating status of currently playing sound
/// \param Dst[out] pointer to stereo interleaved floating point destination buffer
/// \param Src[in] pointer to mono floating point buffer to spatialize
/// \return Returns an ovrResult indicating success or failure
///
/// \see ovrAudio_SpatializeMonoSourceLR
///
OVRA_EXPORT ovrResult
ovrAudio_SpatializeMonoSourceInterleaved(ovrAudioContext Context, int Sound, uint32_t* OutStatus, float* Dst, const float* Src);

/// Spatialize a mono audio source to separate left and right output buffers.
///
/// \param Context[in] context to use
/// \param Sound[in] index of sound (0..NumSources-1)
/// \param OutStatus[out] bitwise OR of flags indicating status of currently playing sound
/// \param DstLeft[out]  pointer to floating point left channel buffer
/// \param DstRight[out] pointer to floating point right channel buffer
/// \param Src[in] pointer to mono floating point buffer to spatialize
/// \return Returns an ovrResult indicating success or failure
///
/// \see ovrAudio_SpatializeMonoSourceInterleaved
///
OVRA_EXPORT ovrResult
ovrAudio_SpatializeMonoSourceLR(ovrAudioContext Context, int Sound, uint32_t* OutStatus, float* DstLeft, float* DstRight, const float* Src);

/// Mix shared reverb into buffer
///
/// \param Context[in] context to use
/// \param OutStatus[out] bitwise OR of flags indicating status of currently playing sound
/// \param OutLeft[out] pointer to floating point left channel buffer to mix into (MUST CONTAIN VALID AUDIO OR SILENCE)
/// \param OutRight[out] pointer to floating point right channel buffer to mix into (MUST CONTAIN VALID AUDIO OR SILENCE)
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult ovrAudio_MixInSharedReverbLR(ovrAudioContext Context, uint32_t* OutStatus, float* DstLeft, float* DstRight);

/// Mix shared reverb into interleaved buffer
///
/// \param Context[in] context to use
/// \param OutStatus[out] bitwise OR of flags indicating status of currently playing sound
/// \param DstInterleaved[out] pointer to interleaved floating point left&right channels buffer to mix into (MUST CONTAIN VALID AUDIO OR
/// SILENCE) \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult ovrAudio_MixInSharedReverbInterleaved(ovrAudioContext Context, uint32_t* OutStatus, float* DstInterleaved);

/// Reset shared reverb
///
/// Use this to cut off reverb tails when entering a pause menu or teleporting to a new location.
///
///\return Returns an ovrResult indicating success or failure
///

OVRA_EXPORT ovrResult ovrAudio_ResetSharedReverb(ovrAudioContext Context);

/// Set shared reverb wet level
///
/// \param Context[in] context to use
/// \param Level[out] linear value to scale global reverb level by
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult ovrAudio_SetSharedReverbWetLevel(ovrAudioContext Context, const float Level);

/// Get shared reverb wet level
///
/// \param Context[in] context to use
/// \param Level[out] linear value currently set to scale global reverb level by
/// \return Returns an ovrResult indicating success or failure
///
OVRA_EXPORT ovrResult ovrAudio_GetSharedReverbWetLevel(ovrAudioContext Context, float* Level);

/// Set user headRadius.
///
/// NOTE: This API is intended to let you set user configuration parameters that
/// may assist with spatialization.
///
/// \param Context[in] context to use
/// \param Config[in] configuration state
OVRA_EXPORT ovrResult ovrAudio_SetHeadRadius(ovrAudioContext Context, float HeadRadius);

/// Set user configuration.
///
/// NOTE: This API is intended to let you set user configuration parameters that
/// may assist with spatialization.
///
/// \param Context[in] context to use
/// \param Config[in] configuration state
OVRA_EXPORT ovrResult ovrAudio_GetHeadRadius(ovrAudioContext Context, float* HeadRadius);

/// Quad-binaural spatialization
///
/// \param ForwardLR[in] pointer to stereo interleaved floating point binaural audio for the forward direction (0 degrees)
/// \param RightLR[in] pointer to stereo interleaved floating point binaural audio for the right direction (90 degrees)
/// \param BackLR[in] pointer to stereo interleaved floating point binaural audio for the backward direction (180 degrees)
/// \param LeftLR[in] pointer to stereo interleaved floating point binaural audio for the left direction (270 degrees)
/// \param LookDirectionX[in] X component of the listener direction vector
/// \param LookDirectionY[in] Y component of the listener direction vector
/// \param LookDirectionZ[in] Z component of the listener direction vector
/// \param NumSamples[in] size of audio buffers (in samples)
/// \param Dst[out] pointer to stereo interleaved floating point destination buffer
///
OVRA_EXPORT ovrResult ovrAudio_ProcessQuadBinaural(
    const float* ForwardLR,
    const float* RightLR,
    const float* BackLR,
    const float* LeftLR,
    float LookDirectionX,
    float LookDirectionY,
    float LookDirectionZ,
    int NumSamples,
    float* Dst);

/// Create an ambisonic stream instance for spatializing B-format ambisonic audio
///
/// \param SampleRate[in] sample rate of B-format signal (16000 to 48000, but 44100 and 48000 are recommended for best quality)
/// \param AudioBufferLength[in] size of audio buffers
/// \param pContext[out] pointer to store address of stream.
///
OVRA_EXPORT ovrResult ovrAudio_CreateAmbisonicStream(
    ovrAudioContext Context,
    int SampleRate,
    int AudioBufferLength,
    ovrAudioAmbisonicFormat format,
    int ambisonicOrder,
    ovrAudioAmbisonicStream* pAmbisonicStream);

/// Reset a previously created ambisonic stream for re-use
///
/// \param[in] Context a valid ambisonic stream
///
OVRA_EXPORT ovrResult ovrAudio_ResetAmbisonicStream(ovrAudioAmbisonicStream AmbisonicStream);

/// Destroy a previously created ambisonic stream.
///
/// \param[in] Context a valid ambisonic stream
/// \see ovrAudio_CreateAmbisonicStream
///
OVRA_EXPORT ovrResult ovrAudio_DestroyAmbisonicStream(ovrAudioAmbisonicStream AmbisonicStream);

/// Sets the render mode for the ambisonic stream.
///
/// \param[in] Context a valid ambisonic stream
/// \see ovrAudioAmbisonicRenderMode
///
OVRA_EXPORT ovrResult ovrAudio_SetAmbisonicRenderMode(ovrAudioAmbisonicStream AmbisonicStream, ovrAudioAmbisonicRenderMode Mode);

/// Sets the render mode for the ambisonic stream.
///
/// \param[in] Context a valid ambisonic stream
/// \see ovrAudioAmbisonicRenderMode
///
OVRA_EXPORT ovrResult ovrAudio_GetAmbisonicRenderMode(ovrAudioAmbisonicStream AmbisonicStream, ovrAudioAmbisonicRenderMode* Mode);

/// Spatialize a mono in ambisonics
///
/// \param InMono[in] Mono audio buffer to spatialize
/// \param DirectionX[in] X component of the direction vector
/// \param DirectionY[in] Y component of the direction vector
/// \param DirectionZ[in] Z component of the direction vector
/// \param Format[in] ambisonic format (AmbiX or FuMa)
/// \param AmbisonicOrder[in] order of ambisonics (1 or 2)
/// \param OutAmbisonic[out] Buffer to write interleaved ambisonics to (4 channels for 1st order, 9 channels for second order)
/// \param NumSamples[in] Length of the buffer in frames (InMono is this length, OutAmbisonic is either 4 or 9 times this length depending
/// on 1st or 2nd order)
///
OVRA_EXPORT ovrResult ovrAudio_MonoToAmbisonic(
    const float* InMono,
    float DirectionX,
    float DirectionY,
    float DirectionZ,
    ovrAudioAmbisonicFormat Format,
    int AmbisonicOrder,
    float* OutAmbisonic,
    int NumSamples);

/// Spatialize ambisonic stream
///
/// \param Src[in] pointer to interleaved floating point ambisonic buffer to spatialize
/// \param Dst[out] pointer to stereo interleaved floating point destination buffer
/// \param NumSamples[in] number of samples per channel
///
OVRA_EXPORT ovrResult ovrAudio_ProcessAmbisonicStreamInterleaved(
    ovrAudioContext Context,
    ovrAudioAmbisonicStream AmbisonicStream,
    const float* Src,
    float* Dst,
    int NumSamples);

/// Set orientation for ambisonic stream
///
/// \param LookDirectionX[in] X component of the source direction vector
/// \param LookDirectionY[in] Y component of the source direction vector
/// \param LookDirectionZ[in] Z component of the source direction vector
/// \param UpDirectionX[in] X component of the source up vector
/// \param UpDirectionY[in] Y component of the source up vector
/// \param UpDirectionZ[in] Z component of the source up vector
///
OVRA_EXPORT ovrResult ovrAudio_SetAmbisonicOrientation(
    ovrAudioAmbisonicStream AmbisonicStream,
    float LookDirectionX,
    float LookDirectionY,
    float LookDirectionZ,
    float UpDirectionX,
    float UpDirectionY,
    float UpDirectionZ);

/// Get orientation for ambisonic stream
///
/// \param pLookDirectionX[in] address of the X component of the source direction vector
/// \param pLookDirectionY[in] address of the Y component of the source direction vector
/// \param pLookDirectionZ[in] address of the Z component of the source direction vector
/// \param pUpDirectionX[in] address of the X component of the source up vector
/// \param pUpDirectionY[in] address of the Y component of the source up vector
/// \param pUpDirectionZ[in] address of the Z component of the source up vector
///
OVRA_EXPORT ovrResult ovrAudio_GetAmbisonicOrientation(
    ovrAudioAmbisonicStream AmbisonicStream,
    float* pLookDirectionX,
    float* pLookDirectionY,
    float* pLookDirectionZ,
    float* pUpDirectionX,
    float* pUpDirectionY,
    float* pUpDirectionZ);

/// Explicitly set the reflection model, this can be used to A/B test the algorithms
///
/// \param Context[in] context to use
/// \param Model[in] The reflection model to use (default is Automatic)
///
/// \see ovrAudioAcousticModel
OVRA_EXPORT ovrResult ovrAudio_SetAcousticModel(ovrAudioContext Context, ovrAudioAcousticModel Model);

/// get the reflection model currently in use
///
/// \param Context[in] context to use
/// \param Model[in] The reflection model currently in use
///
/// \see ovrAudioAcousticModel
OVRA_EXPORT ovrResult ovrAudio_GetAcousticModel(ovrAudioContext Context, ovrAudioAcousticModel* Model);

/// Assign a callback for raycasting into the game geometry
///
/// \param Context[in] context to use
/// \param Callback[in] pointer to an implementation of OVRA_RAYCAST_CALLBACK
/// \param pctx[in] address of user data pointer to be passed into the callback
///
OVR_AUDIO_DEPRECATED OVRA_EXPORT ovrResult
ovrAudio_AssignRaycastCallback(ovrAudioContext Context, OVRA_RAYCAST_CALLBACK Callback, void* pctx);

/// Set the number of ray casts per second are used for dynamic modeling, more rays mean more accurate and responsive modelling but will
/// reduce performance
///
/// \param Context[in] context to use
/// \param RaysPerSecond[in] number of ray casts per second, default = 256
///
OVR_AUDIO_DEPRECATED OVRA_EXPORT ovrResult ovrAudio_SetDynamicRoomRaysPerSecond(ovrAudioContext Context, int RaysPerSecond);

/// Set the speed which the dynamic room interpolates, higher values will update more quickly but less smooth
///
/// \param Context[in] context to use
/// \param InterpSpeed[in] speed which it interpolates (0.0 - 1.0) default = 0.9
///
OVR_AUDIO_DEPRECATED OVRA_EXPORT ovrResult ovrAudio_SetDynamicRoomInterpSpeed(ovrAudioContext Context, float InterpSpeed);

/// Set the maximum distance a cast ray can travel.
///
/// This is useful if you know the bounds of your scene as smaller distances can reduce CPU cost.
///
/// \param Context[in] context to use
/// \param MaxWallDistance[in] distance to wall in meters, default = 50
///
OVR_AUDIO_DEPRECATED OVRA_EXPORT ovrResult ovrAudio_SetDynamicRoomMaxWallDistance(ovrAudioContext Context, float MaxWallDistance);

/// Set the size of the cache which holds a history of the rays cast, a larger value will have more points making it more stable but less
/// responsive
///
/// \param Context[in] context to use
/// \param RayCacheSize[in] number of rays to cache, default = 512
///
OVR_AUDIO_DEPRECATED OVRA_EXPORT ovrResult ovrAudio_SetDynamicRoomRaysRayCacheSize(ovrAudioContext Context, int RayCacheSize);

/// Retrieves the dimensions of the dynamic room moel
///
/// \param Context[in] context to use
/// \param RoomDimensions[out] X, Y, and Z dimensions of the room
/// \param ReflectionsCoefs[out] the reflection coefficients of the walls
/// \param Position[out] the world position of the center of the room
///
OVR_AUDIO_DEPRECATED OVRA_EXPORT ovrResult
ovrAudio_GetRoomDimensions(ovrAudioContext Context, float RoomDimensions[], float ReflectionsCoefs[], ovrAudioVector3f* Position);

/// Retrieves the cache of ray cast hits that are being used to estimate the room, this is useful for debugging rays hitting the wrong
/// objects
///
/// \param Context[in] context to use
/// \param Points[out] array of points where the rays hit geometry
/// \param Normals[out] array of normals
/// \param Length[int] the length of the points and normals array (both should be the same length)
///
OVR_AUDIO_DEPRECATED OVRA_EXPORT ovrResult
ovrAudio_GetRaycastHits(ovrAudioContext Context, ovrAudioVector3f Points[], ovrAudioVector3f Normals[], int Length);

/// Get reflection bands for a material preset
inline ovrResult ovrAudio_GetReflectionBands(ovrAudioMaterialPreset Preset, ovrAudioBands Bands) {
  return MetaXRAudio_GetReflectionBands(Preset, Bands);
}

#ifdef __cplusplus
}
#endif

/*!

\section intro_sec Introduction

The OVRAudio API is a C/C++ interface that implements HRTF-based spatialization
and optional room effects. Your application can directly use this API, though
most developers will access it indirectly via one of our plugins for popular
middleware such as FMOD, Wwise, and Unity.  Starting with Unreal Engine 4.8 it
will also be available natively.

OVRAudio is a low-level API, and as such, it does not buffer or manage sound
state for applications. It positions sounds by filtering incoming monophonic
audio buffers and generating floating point stereo output buffers. Your
application must then mix, convert, and feed this signal to the appropriate
audio output device.

OVRAudio does not handle audio subsystem configuration and output. It is up to
developers to implement this using either a low-level system interface
(e.g., DirectSound, WASAPI, CoreAudio, ALSA) or a high-level middleware
package (e.g,  FMOD, Wwise, Unity).

If you are unfamiliar with the concepts behind audio and virtual reality, we
strongly recommend beginning with the companion guide
*Introduction to Virtual Reality Audio*.

\section sysreq System Requirements

-Windows 7 and 8.x (32 and 64-bit)
-Android
-Mac OS X 10.9+

\section installation Installation

OVRAudio is distributed as a compressed archive. To install, unarchive it in
your development tree and update your compiler include and lib paths
appropriately.

When deploying an application on systems that support shared libraries, you
must ensure that the appropriate DLL/shared library is in the same directory
as your application (Android uses static libraries).

\section multithreading Multithreading

OVRAudio does not create multiple threads  and uses a per-context mutex for
safe read/write access via the API functions from different threads.  It is the
application's responsibility to coordinate context management between different
threads.

\section using Using OVRAudio

This section covers the basics of using OVRAudio in your game or application.

\subsection Initialization

The following code sample illusrtates how to initialize OVRAudio.

Contexts contain the state for a specific spatializer instance.  In most cases
you will only need a single context.

\code{.cpp}
// Make sure to #include "MetaXR_Audio.h"

ovrAudioContext context;

void setup()
{
    // Version checking is not strictly necessary but it's a good idea!
    int major, minor, patch;
    const char *VERSION_STRING;

    VERSION_STRING = ovrAudio_GetVersion( &major, &minor, &patch );
    printf( "Using OVRAudio: %s\n", VERSION_STRING );

    if ( major != OVR_AUDIO_MAJOR_VERSION ||
         minor != OVR_AUDIO_MINOR_VERSION )
    {
      printf( "Mismatched Audio SDK version!\n" );
    }

    ovrAudioContextConfiguration config = {};

    config.acc_Size = sizeof( config );
    config.acc_SampleRate = 48000;
    config.acc_BufferLength = 512;
    config.acc_MaxNumSources = 16;

    if ( ovrAudio_CreateContext( &context, &config ) != ovrSuccess )
    {
      printf( "WARNING: Could not create context!\n" );
      return;
    }
}

\endcode

\subsection gflags Global Flags

A few global flags control OVRAudio's implementation.  These are managed with
ovrAudio_Enable and the appropriate flag:
- ovrAudioEnable_EarlyReflections: Enables box room modeling of
reverberations and reflections
- ovrAudioEnable_LateReverberation: (Requires ovrAudioEnable_EarlyReflections)
Splits room modeling into two components: early reflections (echoes) and late
reverberations.  Late reverberation may be independently disabled.
- ovrAudioEnable_RandomizeReverb: (requires ovrAudioEnable_EarlyReflections
and ovrAudioEnable_LateReverberation) Randomizes reverberation tiles, creating
a more natural sound.

\subsection sourcemanagement Audio Source Management

OVRAudio maintains a set of N audio sources, where N is determined by the value
specified in ovrAudioContextConfiguration::acc_MaxNumSources passed to
ovrAudio_CreateContext.

Each source is associated with a set of parameter, such as:
- position (ovrAudio_SetAudioSourcePosition)
- attenuation range (ovrAudio_SetAudioSourceRange)
- flags (ovrAudio_SetAudioSourceFlags)
- attenuation mode (ovrAudio_SetAudioSourceAttenuationMode)

These may be changed at any time prior to a call to the spatialization APIs.  The
source index (0..N-1) is a parameter to the above functions.

Note: Supplying position values (such as nan, inf) will return an ovrError,
while denormals will be flushed to zero.

Note: Some lingering states such as late reverberation tails may carry over between
calls to the spatializer.  If you dynamically change source sounds bound to an audio
source (for example, you have a pool of OVRAudio sources), you will need to call
ovrAudio_ResetAudioSource to avoid any artifacts.

\subsection attenuation Attenuation

The volume of a sound is attenuated over distance, and this can be modeled in
different ways.  By default, OVRAudio does not perform any attenuation, as the most
common use case is an application or middleware defined attenuation curve.

If you want OVRAudio to attenuate volume based on distance between the sound
source and listener, call ovrAudio_SetAudioSourceAttenuationMode with the
the appropriate mode.  OVRAudio can also scale volume by a fixed value using
ovrAudioSourceAttenuationMode_Fixed.  If, for example, you have computed the
attenuation factor and would like OVRAudio to apply it during spatialization.

\subsection sourceflags Audio Source Flags

You may define properties of specific audio sources by setting appropriate flags
using the ovrAudio_SetAudioSourceFlags aPI.  These flags include:
- ovrAudioSourceFlag_WideBand_HINT: Set this to help mask certain artifacts for
wideband audio sources with a lot of spectral content, such as music, voice and
noise.
- ovrAudioSourceFlag_NarrowBand_HINT: Set this for narrowband audio sources that
lack broad spectral content such as pure tones (sine waves, whistles).
- ovrAudioSourceFlag_DirectTimeOfArrival: Simulate travel time for a sound.  This
is physicaly correct, but may be perceived as less realistic, as games and media
commonly represent sound travel as instantaneous.

\subsection sourcesize Audio Source Size

OVRAudio treats sound sources as infinitely small point sources by default.
This works in most cases, but when a source approaches the listener it may sound
incorret, as if the sound were coming from between the listener's ears.  You may
set a virtual dieamater for a sound source using ovrAudio_SetAudioSourcePropertyf
with the flag ovrAudioSourceProperty_Diameter.  As the listener enters the
sphere, the sounds seems to come from a wider area surrounding the listener's
head.

\subsection envparm Set Environmental Parameters

As the listener transitions into different environments, you can reconfigure the
environment effects parameters.  You may begin by simply inheriting the default
values or setting the values globally a single time.

NOTE: Reflections/reverberation must be enabled

\code
void enterRoom( float w, float h, float d, float r )
{
     ovrAudioBoxRoomParameters brp = {};

     brp.brp_Size = sizeof( brp );
     brp.brp_ReflectLeft = brp.brp_ReflectRight =
     brp.brp_ReflectUp = brp.brp_ReflectDown =
     brp.brp_ReflectFront = brp.brp_ReflectBehind = r;

     brp.brp_Width = w;
     brp.brp_Height = h;
     brp.brp_Depth = d;

     (void)ovrAudio_SetSimpleBoxRoomParameters( AudioContext, &brp );
}
\endcode

\subsection Setting listener Pose (Optional)

You may specify the listener's pose state using values retrieved directly from
the HMD using LibOVR:
\code
void setListenerPose( float PositionX,
    float PositionY,
    float PositionZ,
    float ForwardX,
    float ForwardY,
    float ForwardZ,
    float UpX,
    float UpY,
    float UpZ )
{
   (void)ovrAudio_SetListenerVectors( AudioContext,
    PositionX, PositionY, PositionZ,
    ForwardX, ForwardY, ForwardZ,
    UpX, UpY, UpZ);
}
\endcode

All sound sources are transformed with reference to the specified pose so that
they remain positioned correctly relative to the listener. If you do not call
this function, the listener is assumed to be at (0,0,0) and looking forward
(down the -Z axis), and that all sounds are in listener-relative coordinates.

\subsection spatialization Applying 3D Spatialization

Applying 3D spatialiazation consists of looping over all of your sounds,
copying their data into intermediate buffers, and passing them to the
positional audio engine. It will in turn process the sounds with the
appropriate HRTFs and effects and return a floating point stereo buffer:
\code
void processSounds( Sound *sounds, int NumSounds, float *MixBuffer )
{
   // This assumes that all sounds want to be spatialized!
   // NOTE: In practice these should be 16-byte aligned, but for brevity
   // we're just showing them declared like this
   uint32_t Status = 0;

   float outbuffer[ INPUT_BUFFER_SIZE * 2 ];
   float inbuffer[ INPUT_BUFFER_SIZE ];

   for ( int i = 0; i < NumSounds; i++ )
   {
      // Set the sound's position in space (using OVR coordinates)
      // NOTE: if a pose state has been specified by a previous call to
      // ovrAudio_ListenerPoseStatef then it will be transformed
      // by that as well
      (void)ovrAudio_SetAudioSourcePos( AudioContext, i,
         sounds[ i ].X, sounds[ i ].Y, sounds[ i ].Z );

      // This sets the attenuation range from max volume to silent
      // NOTE: attenuation can be disabled or enabled
      (void)ovrAudio_SetAudioSourceRange( AudioContext, i,
         sounds[ i ].RangeMin, sounds[ i ].RangeMax );

      // Grabs the next chunk of data from the sound, looping etc.
      // as necessary.  This is application specific code.
      sounds[ i ].GetSoundData( inbuffer );

      // Spatialize the sound into the output buffer.  Note that there
      // are two APIs, one for interleaved sample data and another for
      // separate left/right sample data
      (void)ovrAudio_SpatializeMonoSourceInterleaved( AudioContext,
          i, &Status,
         outbuffer, inbuffer );

      // Do some mixing
      for ( int j = 0; j < INPUT_BUFFER_SIZE; j++ )
      {
         if ( i == 0 )
         {
            MixBuffer[ j ] = outbuffer[ j ];
         }
         else
         {
            MixBuffer[ j ] += outbuffer[ j ];
         }
      }
   }

   // From here we'd send the MixBuffer on for more processing or
   // final device output

   PlayMixBuffer(MixBuffer);
}
\endcode

At that point we have spatialized sound mixed into our output buffer.

\subsection reverbtails Finishing Reverb Tails

If late reverberation and simple box room modeling are enabled then the there
will be more output for the reverberation tail after the input sound has finished.
To ensure that the reverberation tail is not cut off, you can continue to feed the
spatialization functions with silence (e.g. NULL source data) after all of the
input data has been processed to get the rest of the output. When the tail has
finished the OutStatus will contain ovrAudioSpatializationStatus_Finished flag.

This correction should occur at the final output stage. In other words, it should be
applied directly on the stereo outputs and not on each sound.

\subsection shutdown Shutdown

When you are no longer using OVRAudio, shut it down by destroying any contexts,
and then calling ovrAudio_Shutdown.

\code
void shutdownOvrAudio()
{
   ovrAudio_DestroyContext( AudioContext );
   ovrAudio_Shutdown();
}
\endcode

*/

#endif // OVR_Audio_h
