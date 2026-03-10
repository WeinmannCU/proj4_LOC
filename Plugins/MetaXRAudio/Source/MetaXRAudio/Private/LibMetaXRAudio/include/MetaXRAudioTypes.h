// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/********************************************************************************/ /**
  \file      MetaXRAudioTypes.h
  \brief     Meta XR Audio SDK shared types header file
  ************************************************************************************/

#ifndef MetaXRAudioTypes_h
#define MetaXRAudioTypes_h

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define METAXRAUDIO_MAJOR_VERSION 1
#define METAXRAUDIO_MINOR_VERSION 115
#define METAXRAUDIO_PATCH_VERSION 0

#ifndef METAXRAUDIO_EXPORT
#ifdef _WIN32
#define METAXRAUDIO_EXPORT __declspec(dllexport)
#elif defined(__ANDROID__)
#define METAXRAUDIO_EXPORT __attribute__((visibility("default")))
#elif defined __APPLE__
#define METAXRAUDIO_EXPORT __attribute__((visibility("default")))
#elif defined __linux__
#define METAXRAUDIO_EXPORT __attribute__((visibility("default")))
#elif defined(__HEXAGON_ARCH__) || defined(__hexagon__)
#define METAXRAUDIO_EXPORT __attribute__((visibility("default")))
#else
#error not implemented
#endif
#endif

#ifndef FUNC_NAME
#ifdef _WIN32
#define FUNC_NAME __FUNCTION__
#elif defined(__ANDROID__)
#define FUNC_NAME __func__
#elif defined __APPLE__
#define FUNC_NAME __func__
#elif defined __linux__
#define FUNC_NAME __func__
#elif defined(__HEXAGON_ARCH__) || defined(__hexagon__)
#define FUNC_NAME __func__
#else
#error not implemented
#endif
#endif

//-----------------------------------------------------------------------------------
// ***** OVR_ALIGNAS
//
#if !defined(OVR_ALIGNAS)
#if defined(__GNUC__) || defined(__clang__)
#define OVR_ALIGNAS(n) __attribute__((aligned(n)))
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
#define OVR_ALIGNAS(n) __declspec(align(n))
#elif defined(__CC_ARM)
#define OVR_ALIGNAS(n) __align(n)
#else
#error Need to define OVR_ALIGNAS
#endif
#endif

// Error codes
#if defined(__cplusplus) && (__cplusplus >= 201703)
#define METAXRAUDIO_NO_DISARD_RESULT_TYPE
#endif

/// Enumerates error codes that can be returned by Audio SDK
#ifdef METAXRAUDIO_NO_DISARD_RESULT_TYPE
typedef enum [[nodiscard]] MetaXRAudioResult : int32_t {
#else
typedef enum {
#endif
  MetaXRAudioSuccess = 0,
  MetaXRAudioError_Unknown = 2000, ///< An unknown error has occurred.
  MetaXRAudioError_InvalidParam = 2001, ///< An invalid parameter, e.g. NULL pointer or out of range variable, was passed
  MetaXRAudioError_BadSampleRate = 2002, ///< An unsupported sample rate was declared
  MetaXRAudioError_MissingDLL = 2003, ///< The DLL or shared library could not be found
  MetaXRAudioError_BadAlignment = 2004, ///< Buffers did not meet 16b alignment requirements
  MetaXRAudioError_Uninitialized = 2005, ///< audio function called before initialization
  MetaXRAudioError_HRTFInitFailure = 2006, ///< HRTF provider initialization failed
  MetaXRAudioError_BadVersion = 2007, ///< Mismatched versions between header and libs
  MetaXRAudioError_SymbolNotFound = 2008, ///< Couldn't find a symbol in the DLL
  MetaXRAudioError_SharedReverbDisabled = 2009, ///< Late reverberation is disabled
  MetaXRAudioError_BadAlloc = 2016,
  MetaXRAudioError_NoAvailableAmbisonicInstance = 2017, /// < Ran out of ambisonic sources
  MetaXRAudioError_MemoryAllocFailure = 2018, ///< out of memory (fatal)
  MetaXRAudioError_UnsupportedFeature = 2019, ///< Unsupported feature
  MetaXRAudioError_InvalidAudioContext = 2020,
  MetaXRAudioError_BadMesh = 2021,
  MetaXRAudioError_InternalEnd = 2099, ///< Internal errors used by Audio SDK defined down towards public errors
                                       ///< NOTE: Since we do not define a beginning range for Internal codes, make sure
                                       ///< not to hard-code range checks (since that can vary based on build)
} MetaXRAudioResult;

/// A 3D vector with float components.
typedef struct OVR_ALIGNAS(4) MetaXRAudioVector3f_ {
  float x, y, z;
} MetaXRAudioVector3f;

/// 3D Pose structure (position + orientation)
typedef struct {
  MetaXRAudioVector3f position;
  MetaXRAudioVector3f forward;
  MetaXRAudioVector3f up;
} MetaXRAudioPose;

#define METAXRAUDIO_REVERB_BAND_COUNT (4)
#define OVRA_REVERB_BAND_COUNT (4) // For backwards compatibility

const int kReverbBandCount = METAXRAUDIO_REVERB_BAND_COUNT;
const int kReverbShCoefCount = 4;
typedef float MetaXRAudioBands[METAXRAUDIO_REVERB_BAND_COUNT];

typedef void (*METAXRAUDIO_RAYCAST_CALLBACK)(
    MetaXRAudioVector3f origin,
    MetaXRAudioVector3f direction,
    MetaXRAudioVector3f* hit,
    MetaXRAudioVector3f* normal,
    MetaXRAudioBands reflectionBands,
    void* pctx);

#ifndef OVR_CAPI_h
typedef struct ovrPosef_ ovrPosef;
typedef struct ovrPoseStatef_ ovrPoseStatef;
#endif

/// Audio context initialization flags
typedef enum {
  MetaXRAudioInitFlag_Default = 0x0000,
  MetaXRAudioInitFlag_ThreadUnsafe = 0x0010, /// < Skip mutex locks, thread safety becomes caller responsibility
  MetaXRAudioInitFlag_LazyAllocations = 0x0020, /// < Dynamicly allocate memory on demand rather than preallocating
} MetaXRAudioInitFlags;

/// Audio source flags
typedef enum {
  MetaXRAudioSourceFlag_None = 0x0000,

  MetaXRAudioSourceFlag_WideBand_HINT = 0x0010, ///< Wide band signal (music, voice, noise, etc.)
  MetaXRAudioSourceFlag_NarrowBand_HINT = 0x0020, ///< Narrow band signal (pure waveforms, e.g sine)
  MetaXRAudioSourceFlag_BassCompensation_DEPRECATED = 0x0040, ///< Compensate for drop in bass from HRTF (deprecated)
  MetaXRAudioSourceFlag_DirectTimeOfArrival = 0x0080, ///< Time of arrival delay for the direct signal
  MetaXRAudioSourceFlag_ReflectionsDisabled = 0x0100, ///< Disable reflections and reverb for a single AudioSource
  MetaXRAudioSourceFlag_MediumAbsorption = 0x4000, ///< Frequency specific medium absorption over distance
  MetaXRAudioSourceFlag_DisableResampling_RESERVED = 0x8000, ///< Disable resampling IR to output rate, INTERNAL USE ONLY

} MetaXRAudioSourceFlag;

/// Audio source attenuation mode
typedef enum {
  MetaXRAudioSourceAttenuationMode_None = 0, ///< Sound is not attenuated, e.g. middleware handles attenuation
  MetaXRAudioSourceAttenuationMode_Fixed = 1, ///< Sound has fixed attenuation (passed to ovrAudio_SetAudioSourceAttenuationMode)
  MetaXRAudioSourceAttenuationMode_InverseSquare = 2, ///< Sound uses internally calculated attenuation based on inverse square

  MetaXRAudioSourceAttenuationMode_COUNT

} MetaXRAudioSourceAttenuationMode;

/// Global boolean flags
typedef enum {
  MetaXRAudioEnable_None = 0, ///< None
  MetaXRAudioEnable_EarlyReflections = 2, ///< Enable/disable early reflections globally. Default: disabled
  MetaXRAudioEnable_LateReverberation = 3, ///< Late reverbervation, requires simple room modeling enabled. Default: disabled
  MetaXRAudioEnable_RandomizeReverb = 4, ///< Randomize reverbs to diminish artifacts. Default: enabled
  MetaXRAudioEnable_PerformanceCounters = 5, ///< Enable profiling. Default: disabled
  MetaXRAudioEnable_Diffraction = 6, ///< Enable Diffraction. Default: disabled

  MetaXRAudioEnable_COUNT
} MetaXRAudioEnable;

/// Explicit override to select reflection and reverb system
typedef enum {
  MetaXRAudioAcousticModel_Automatic = -1, ///< Automatically select highest quality (if geometry is set the propagation system will be
                                           ///< active, otherwise fallback to the Room Acoustics)
  MetaXRAudioAcousticModel_None = 0, ///< Disable all acoustics features
  MetaXRAudioAcousticModel_Shoebox = 1, ///< Room controlled by ovrAudioBoxRoomParameters
  MetaXRAudioAcousticModel_DynamicRoomModeling = 2, ///< Room automatically fit the Room
  MetaXRAudioAcousticModel_AcousticRayTracing = 3, ///< Sound propgated using game geometry
  MetaXRAudioAcousticModel_COUNT
} MetaXRAudioAcousticModel;

/// Status mask returned by spatializer APIs
typedef enum {
  MetaXRAudioSpatializationStatus_None = 0x00, ///< Nothing to report
  MetaXRAudioSpatializationStatus_Finished = 0x01, ///< Buffer is empty and sound processing is finished
  MetaXRAudioSpatializationStatus_Working = 0x02, ///< Data still remains in buffer (e.g. reverberation tail)

} MetaXRAudioSpatializationStatus;

/// Ambisonic formats
typedef enum {
  MetaXRAudioAmbisonicFormat_FuMa, ///< standard B-Format, channel order = WXYZ (W channel is -3dB)
  MetaXRAudioAmbisonicFormat_AmbiX ///< ACN/SN3D standard, channel order = WYZX
} MetaXRAudioAmbisonicFormat;

/// Ambisonic rendering modes
typedef enum {
  MetaXRAudioAmbisonicRenderMode_SphericalHarmonics = -1, ///< (default) Uses a spherical harmonic representation of HRTF
  MetaXRAudioAmbisonicRenderMode_Mono = -2 ///< Plays the W (omni) channel through left and right with no spatialization
} MetaXRAudioAmbisonicRenderMode;

/// Opaque type definitions for audio source and context
typedef struct MetaXRAudioSource_ MetaXRAudioSource;
typedef struct MetaXRAudioContext_* MetaXRAudioContext;
typedef struct MetaXRAudioAmbisonicStream_* MetaXRAudioAmbisonicStream;
typedef void* MetaXRAudioSpectrumAnalyzer;

/** \brief A function pointer that allocates the specified number of bytes and returns a pointer to the memory. Return null if allocation
 * fails. */
typedef void* (*MetaXRAudioAllocateCallback)(size_t align, size_t size);
/** \brief A function pointer that deallocates a pointer previously returned from the allocation function. */
typedef void (*MetaXRAudioDeallocateCallback)(size_t align, void* data);

/// Audio context configuration structure
typedef struct _MetaXRAudioContextConfig {
  uint32_t acc_Size; ///< set to size of the struct
  uint32_t acc_MaxNumSources; ///< maximum number of audio sources to support
  uint32_t acc_SampleRate; ///< sample rate (16000 to 48000, but 44100 and 48000 are recommended for best quality)
  uint32_t acc_BufferLength; ///< number of samples in mono input buffers passed to spatializer
  uint32_t acc_InitFlags; ///< initialization flags
  uint32_t acc_AmbisonicOrderMax; ///< highest supported ambisonic order (1-8)
} MetaXRAudioContextConfiguration;

/// Box room parameters used by MetaXRAudio_SetSimpleBoxRoomParameters
typedef struct _MetaXRAudioBoxRoomParameters {
  uint32_t brp_Size; ///< Size of struct
  float brp_ReflectLeft, brp_ReflectRight; ///< Reflection values (0 - 0.97)
  float brp_ReflectUp, brp_ReflectDown; ///< Reflection values (0 - 0.97)
  float brp_ReflectBehind, brp_ReflectFront; ///< Reflection values (0 - 0.97)
  float brp_Width, brp_Height, brp_Depth; ///< Size of box in meters
} MetaXRAudioBoxRoomParameters;

/// Box room parameters used by MetaXRAudio_SetAdvancedBoxRoomParameters
typedef struct _MetaXRAudioAdvancedBoxRoomParameters {
  uint32_t abrp_Size; ///< Size of struct
  MetaXRAudioBands abrp_ReflectLeft, abrp_ReflectRight; ///< Reflection bands (0 - 1.0)
  MetaXRAudioBands abrp_ReflectUp, abrp_ReflectDown; ///< Reflection bands (0 - 1.0)
  MetaXRAudioBands abrp_ReflectBehind, abrp_ReflectFront; ///< Reflection bands (0 - 1.0)
  float abrp_Width, abrp_Height, abrp_Depth; ///< Size of box in meters
  int abrp_LockToListenerPosition; ///< Whether box is centered on listener
  MetaXRAudioVector3f abrp_RoomPosition;
} MetaXRAudioAdvancedBoxRoomParameters;

typedef enum {
  MetaXRAudioMaterialPreset_AcousticTile = 0,
  MetaXRAudioMaterialPreset_Brick,
  MetaXRAudioMaterialPreset_BrickPainted,
  MetaXRAudioMaterialPreset_Cardboard,
  MetaXRAudioMaterialPreset_Carpet,
  MetaXRAudioMaterialPreset_CarpetHeavy,
  MetaXRAudioMaterialPreset_CarpetHeavyPadded,
  MetaXRAudioMaterialPreset_CeramicTile,
  MetaXRAudioMaterialPreset_Concrete,
  MetaXRAudioMaterialPreset_ConcreteRough,
  MetaXRAudioMaterialPreset_ConcreteBlock,
  MetaXRAudioMaterialPreset_ConcreteBlockPainted,
  MetaXRAudioMaterialPreset_Curtain,
  MetaXRAudioMaterialPreset_Foliage,
  MetaXRAudioMaterialPreset_Glass,
  MetaXRAudioMaterialPreset_GlassHeavy,
  MetaXRAudioMaterialPreset_Grass,
  MetaXRAudioMaterialPreset_Gravel,
  MetaXRAudioMaterialPreset_GypsumBoard,
  MetaXRAudioMaterialPreset_Marble,
  MetaXRAudioMaterialPreset_Mud,
  MetaXRAudioMaterialPreset_PlasterOnBrick,
  MetaXRAudioMaterialPreset_PlasterOnConcreteBlock,
  MetaXRAudioMaterialPreset_Rubber,
  MetaXRAudioMaterialPreset_Soil,
  MetaXRAudioMaterialPreset_SoundProof,
  MetaXRAudioMaterialPreset_Snow,
  MetaXRAudioMaterialPreset_Steel,
  MetaXRAudioMaterialPreset_Stone,
  MetaXRAudioMaterialPreset_Vent,
  MetaXRAudioMaterialPreset_Water,
  MetaXRAudioMaterialPreset_WoodThin,
  MetaXRAudioMaterialPreset_WoodThick,
  MetaXRAudioMaterialPreset_WoodFloor,
  MetaXRAudioMaterialPreset_WoodOnConcrete,
  MetaXRAudioMaterialPreset_MetaDefault,
  MetaXRAudioMaterialPreset_COUNT
} MetaXRAudioMaterialPreset;

/// Get reflection bands for a material preset
inline MetaXRAudioResult MetaXRAudio_GetReflectionBands(MetaXRAudioMaterialPreset Preset, MetaXRAudioBands Bands) {
  if (Preset >= MetaXRAudioMaterialPreset_COUNT || Bands == NULL) {
    return MetaXRAudioError_InvalidParam;
  }

  switch (Preset) {
    case MetaXRAudioMaterialPreset_AcousticTile:
      Bands[0] = 0.488168418f;
      Bands[1] = 0.361475229f;
      Bands[2] = 0.339595377f;
      Bands[3] = 0.498946249f;
      break;
    case MetaXRAudioMaterialPreset_Brick:
      Bands[0] = 0.975468814f;
      Bands[1] = 0.972064495f;
      Bands[2] = 0.949180186f;
      Bands[3] = 0.930105388f;
      break;
    case MetaXRAudioMaterialPreset_BrickPainted:
      Bands[0] = 0.975710571f;
      Bands[1] = 0.983324170f;
      Bands[2] = 0.978116691f;
      Bands[3] = 0.970052719f;
      break;
    case MetaXRAudioMaterialPreset_Cardboard:
      Bands[0] = 0.590000f;
      Bands[1] = 0.435728f;
      Bands[2] = 0.251650f;
      Bands[3] = 0.208000f;
      break;
    case MetaXRAudioMaterialPreset_Carpet:
      Bands[0] = 0.987633705f;
      Bands[1] = 0.905486643f;
      Bands[2] = 0.583110571f;
      Bands[3] = 0.351053834f;
      break;
    case MetaXRAudioMaterialPreset_CarpetHeavy:
      Bands[0] = 0.977633715f;
      Bands[1] = 0.859082878f;
      Bands[2] = 0.526479602f;
      Bands[3] = 0.370790422f;
      break;
    case MetaXRAudioMaterialPreset_CarpetHeavyPadded:
      Bands[0] = 0.910534739f;
      Bands[1] = 0.530433178f;
      Bands[2] = 0.294055820f;
      Bands[3] = 0.270105422f;
      break;
    case MetaXRAudioMaterialPreset_CeramicTile:
      Bands[0] = 0.990000010f;
      Bands[1] = 0.990000010f;
      Bands[2] = 0.982753932f;
      Bands[3] = 0.980000019f;
      break;
    case MetaXRAudioMaterialPreset_Concrete:
      Bands[0] = 0.990000010f;
      Bands[1] = 0.983324170f;
      Bands[2] = 0.980000019f;
      Bands[3] = 0.980000019f;
      break;
    case MetaXRAudioMaterialPreset_ConcreteRough:
      Bands[0] = 0.989408433f;
      Bands[1] = 0.964494646f;
      Bands[2] = 0.922127008f;
      Bands[3] = 0.900105357f;
      break;
    case MetaXRAudioMaterialPreset_ConcreteBlock:
      Bands[0] = 0.635267377f;
      Bands[1] = 0.652230680f;
      Bands[2] = 0.671053469f;
      Bands[3] = 0.789051592f;
      break;
    case MetaXRAudioMaterialPreset_ConcreteBlockPainted:
      Bands[0] = 0.902957916f;
      Bands[1] = 0.940235913f;
      Bands[2] = 0.917584062f;
      Bands[3] = 0.919947326f;
      break;
    case MetaXRAudioMaterialPreset_Curtain:
      Bands[0] = 0.686494231f;
      Bands[1] = 0.545859993f;
      Bands[2] = 0.310078561f;
      Bands[3] = 0.399473131f;
      break;
    case MetaXRAudioMaterialPreset_Foliage:
      Bands[0] = 0.518259346f;
      Bands[1] = 0.503568292f;
      Bands[2] = 0.578688800f;
      Bands[3] = 0.690210819f;
      break;
    case MetaXRAudioMaterialPreset_Glass:
      Bands[0] = 0.655915797f;
      Bands[1] = 0.800631821f;
      Bands[2] = 0.918839693f;
      Bands[3] = 0.923488140f;
      break;
    case MetaXRAudioMaterialPreset_GlassHeavy:
      Bands[0] = 0.827098966f;
      Bands[1] = 0.950222731f;
      Bands[2] = 0.974604130f;
      Bands[3] = 0.980000019f;
      break;
    case MetaXRAudioMaterialPreset_Grass:
      Bands[0] = 0.881126285f;
      Bands[1] = 0.507170796f;
      Bands[2] = 0.131893098f;
      Bands[3] = 0.0103688836f;
      break;
    case MetaXRAudioMaterialPreset_Gravel:
      Bands[0] = 0.729294717f;
      Bands[1] = 0.373122454f;
      Bands[2] = 0.255317450f;
      Bands[3] = 0.200263441f;
      break;
    case MetaXRAudioMaterialPreset_GypsumBoard:
      Bands[0] = 0.721240044f;
      Bands[1] = 0.927690148f;
      Bands[2] = 0.934302270f;
      Bands[3] = 0.910105407f;
      break;
    case MetaXRAudioMaterialPreset_Marble:
      Bands[0] = 0.990000f;
      Bands[1] = 0.990000f;
      Bands[2] = 0.982754f;
      Bands[3] = 0.980000f;
      break;
    case MetaXRAudioMaterialPreset_Mud:
      Bands[0] = 0.844084f;
      Bands[1] = 0.726577f;
      Bands[2] = 0.794683f;
      Bands[3] = 0.849737f;
      break;
    case MetaXRAudioMaterialPreset_PlasterOnBrick:
      Bands[0] = 0.975696504f;
      Bands[1] = 0.979106009f;
      Bands[2] = 0.961063504f;
      Bands[3] = 0.950052679f;
      break;
    case MetaXRAudioMaterialPreset_PlasterOnConcreteBlock:
      Bands[0] = 0.881774724f;
      Bands[1] = 0.924773932f;
      Bands[2] = 0.951497555f;
      Bands[3] = 0.959947288f;
      break;
    case MetaXRAudioMaterialPreset_Rubber:
      Bands[0] = 0.950000f;
      Bands[1] = 0.916621f;
      Bands[2] = 0.936230f;
      Bands[3] = 0.950000f;
      break;
    case MetaXRAudioMaterialPreset_Soil:
      Bands[0] = 0.844084203f;
      Bands[1] = 0.634624243f;
      Bands[2] = 0.416662872f;
      Bands[3] = 0.400000036f;
      break;
    case MetaXRAudioMaterialPreset_SoundProof:
      Bands[0] = 0.000000000f;
      Bands[1] = 0.000000000f;
      Bands[2] = 0.000000000f;
      Bands[3] = 0.000000000f;
      break;
    case MetaXRAudioMaterialPreset_Snow:
      Bands[0] = 0.532252669f;
      Bands[1] = 0.154535770f;
      Bands[2] = 0.0509644151f;
      Bands[3] = 0.0500000119f;
      break;
    case MetaXRAudioMaterialPreset_Steel:
      Bands[0] = 0.793111682f;
      Bands[1] = 0.840140402f;
      Bands[2] = 0.925591767f;
      Bands[3] = 0.979736567f;
      break;
    case MetaXRAudioMaterialPreset_Stone:
      Bands[0] = 0.980000f;
      Bands[1] = 0.978740f;
      Bands[2] = 0.955701f;
      Bands[3] = 0.950000f;
      break;
    case MetaXRAudioMaterialPreset_Vent:
      Bands[0] = 0.847042f;
      Bands[1] = 0.620450f;
      Bands[2] = 0.702170f;
      Bands[3] = 0.799473f;
      break;
    case MetaXRAudioMaterialPreset_Water:
      Bands[0] = 0.970588267f;
      Bands[1] = 0.971753478f;
      Bands[2] = 0.978309572f;
      Bands[3] = 0.970052719f;
      break;
    case MetaXRAudioMaterialPreset_WoodThin:
      Bands[0] = 0.592423141f;
      Bands[1] = 0.858273327f;
      Bands[2] = 0.917242289f;
      Bands[3] = 0.939999998f;
      break;
    case MetaXRAudioMaterialPreset_WoodThick:
      Bands[0] = 0.812957883f;
      Bands[1] = 0.895329595f;
      Bands[2] = 0.941304684f;
      Bands[3] = 0.949947298f;
      break;
    case MetaXRAudioMaterialPreset_WoodFloor:
      Bands[0] = 0.852366328f;
      Bands[1] = 0.898992121f;
      Bands[2] = 0.934784114f;
      Bands[3] = 0.930052698f;
      break;
    case MetaXRAudioMaterialPreset_WoodOnConcrete:
      Bands[0] = 0.959999979f;
      Bands[1] = 0.941232264f;
      Bands[2] = 0.937923789f;
      Bands[3] = 0.930052698f;
      break;
    case MetaXRAudioMaterialPreset_MetaDefault:
      Bands[0] = 0.9f;
      Bands[1] = 0.9f;
      Bands[2] = 0.9f;
      Bands[3] = 0.9f;
      break;
    case MetaXRAudioMaterialPreset_COUNT:
    default:
      Bands[0] = 0.000000000f;
      Bands[1] = 0.000000000f;
      Bands[2] = 0.000000000f;
      Bands[3] = 0.000000000f;
      return MetaXRAudioError_InvalidParam;
  };

  return MetaXRAudioSuccess;
}

#ifdef __cplusplus
}
#endif

#endif // MetaXRAudioTypes_h
