#pragma once
#include <cstdint>
#include <juce_core/juce_core.h>

namespace ambilink::encoders {

/**
 * @brief Ambisonics normalization convention.
 *
 */
enum class NormalizationType : uint8_t
{
    N3D = 0, /** orthonormalised (N3D) */
    SN3D,    /** Schmidt semi-normalisation (SN3D) */
    DEFAULT = SN3D
};
static const juce::StringArray NormalizationTypeStrings{"N3D", "SN3D"};

/**
 * @brief Distance attenuation curve used by the encoder.
 * 
 */
enum class DistanceAttenuationType : uint8_t
{
    NONE = 0,
    LIN = 1,
    LOG = 2,
    DEFAULT = LOG
};
static const juce::StringArray DistanceAttenuationTypeStrings{"None", "Linear", "Logarithmic"};

} // namespace ambilink::encoders
