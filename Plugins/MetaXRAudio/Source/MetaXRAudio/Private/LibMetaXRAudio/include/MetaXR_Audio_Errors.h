// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/********************************************************************************/ /**
 \file      MetaXR_Audio_Errors.h
 \brief     OVR Audio SDK public header file containing return codes
 ************************************************************************************/

#ifndef OVR_Audio_Errors_h
#define OVR_Audio_Errors_h

#include <stdint.h>

#include "MetaXRAudioTypes.h"

/// Result type used by the Audio SDK
#ifdef OVR_RESULT_DEFINED
#error "duplicate ovrResult definition"
#else
#define OVR_RESULT_DEFINED
typedef MetaXRAudioResult ovrResult;

// Error code constants for backward compatibility
#define ovrSuccess MetaXRAudioSuccess
#define ovrError_AudioUnknown MetaXRAudioError_Unknown
#define ovrError_AudioInvalidParam MetaXRAudioError_InvalidParam
#define ovrError_AudioBadSampleRate MetaXRAudioError_BadSampleRate
#define ovrError_AudioMissingDLL MetaXRAudioError_MissingDLL
#define ovrError_AudioBadAlignment MetaXRAudioError_BadAlignment
#define ovrError_AudioUninitialized MetaXRAudioError_Uninitialized
#define ovrError_AudioHRTFInitFailure MetaXRAudioError_HRTFInitFailure
#define ovrError_AudioBadVersion MetaXRAudioError_BadVersion
#define ovrError_AudioSymbolNotFound MetaXRAudioError_SymbolNotFound
#define ovrError_SharedReverbDisabled MetaXRAudioError_SharedReverbDisabled
#define ovrError_AudioBadAlloc MetaXRAudioError_BadAlloc
#define ovrError_AudioNoAvailableAmbisonicInstance MetaXRAudioError_NoAvailableAmbisonicInstance
#define ovrError_AudioMemoryAllocFailure MetaXRAudioError_MemoryAllocFailure
#define ovrError_AudioUnsupportedFeature MetaXRAudioError_UnsupportedFeature
#define ovrError_AudioInvalidAudioContext MetaXRAudioError_InvalidAudioContext
#define ovrError_AudioBadMesh MetaXRAudioError_BadMesh
#endif

#ifndef OVRA_EXPORT
#define OVRA_EXPORT METAXRAUDIO_EXPORT
#endif

#endif // OVR_Audio_Errors_h
