#include "BasicEncoder.h"

#include <saf_externals.h>
#include <fmt/format.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <Utility/Utils.h>
#include <Exceptions.h>
#include <ValueIDs.h>

namespace {
constexpr float n3d2sn3d[64] = {
  1.0000000000000000e+00f, 5.7735026918962584e-01f, 5.7735026918962584e-01f,
  5.7735026918962584e-01f, 4.4721359549995793e-01f, 4.4721359549995793e-01f,
  4.4721359549995793e-01f, 4.4721359549995793e-01f, 4.4721359549995793e-01f,
  3.7796447300922720e-01f, 3.7796447300922720e-01f, 3.7796447300922720e-01f,
  3.7796447300922720e-01f, 3.7796447300922720e-01f, 3.7796447300922720e-01f,
  3.7796447300922720e-01f, 3.3333333333333331e-01f, 3.3333333333333331e-01f,
  3.3333333333333331e-01f, 3.3333333333333331e-01f, 3.3333333333333331e-01f,
  3.3333333333333331e-01f, 3.3333333333333331e-01f, 3.3333333333333331e-01f,
  3.3333333333333331e-01f, 3.0151134457776363e-01f, 3.0151134457776363e-01f,
  3.0151134457776363e-01f, 3.0151134457776363e-01f, 3.0151134457776363e-01f,
  3.0151134457776363e-01f, 3.0151134457776363e-01f, 3.0151134457776363e-01f,
  3.0151134457776363e-01f, 3.0151134457776363e-01f, 3.0151134457776363e-01f,
  2.7735009811261457e-01f, 2.7735009811261457e-01f, 2.7735009811261457e-01f,
  2.7735009811261457e-01f, 2.7735009811261457e-01f, 2.7735009811261457e-01f,
  2.7735009811261457e-01f, 2.7735009811261457e-01f, 2.7735009811261457e-01f,
  2.7735009811261457e-01f, 2.7735009811261457e-01f, 2.7735009811261457e-01f,
  2.7735009811261457e-01f, 2.5819888974716110e-01f, 2.5819888974716110e-01f,
  2.5819888974716110e-01f, 2.5819888974716110e-01f, 2.5819888974716110e-01f,
  2.5819888974716110e-01f, 2.5819888974716110e-01f, 2.5819888974716110e-01f,
  2.5819888974716110e-01f, 2.5819888974716110e-01f, 2.5819888974716110e-01f,
  2.5819888974716110e-01f, 2.5819888974716110e-01f, 2.5819888974716110e-01f,
  2.5819888974716110e-01f,
};

float calculateGainFromDistance(
  float distance, float max_distance,
  ambilink::encoders::DistanceAttenuationType att_type) {
    using AttType = ambilink::encoders::DistanceAttenuationType;
    constexpr auto small_number = std::numeric_limits<float>::denorm_min();

    distance = std::max(distance, small_number);
    auto dist_to_max_dist_ratio = distance / max_distance;
    float retval{0};

    switch (att_type) {
        case AttType::NONE:
            return 1.0;
        case AttType::LIN:
            retval = 1.0f - dist_to_max_dist_ratio;
            break;
        case AttType::LOG:
            retval = 0.5f * -log(dist_to_max_dist_ratio);
            break;
    }
    return std::clamp(retval, 0.0f, 1.0f);
};

template<typename EnumT>
auto enumFromAudioParamRawValue(std::atomic<float>* enum_param_raw_val) {
    return static_cast<EnumT>(enum_param_raw_val->load());
}
} // namespace
namespace ambilink::encoders {

uint8_t maxOrderFromOutChannelCount(uint16_t output_channel_count) {
    auto order = std::sqrt(output_channel_count) - 1;
    if (order < 0)
        throw ambilink::InvalidArgumentError("shSignalCount",
                                             output_channel_count);
    if (auto retval = order / 1; retval <= MAX_SH_ORDER) {
        return retval;
    }
    throw ambilink::InvalidArgumentError("shSignalCount", output_channel_count,
                                         "resulting order > MAX_SH_ORDER");
}

bool isValidOutputChannelCount(int channel_count) {
    jassert(channel_count >= 0);
    return channel_count <= MAX_SH_SIGNALS && channel_count >= MIN_SH_SIGNALS
           && (std::fmod(std::sqrt(channel_count), 1) == 0);
}

BasicEncoder::BasicEncoder(juce::AudioProcessorValueTreeState& params) : _params(params) {
    _sh_order
      = _params.getRawParameterValue(ids::params::ambisonics_order.toString());
    _normalisation_type = _params.getRawParameterValue(
      ids::params::normalization_type.toString());
    _dist_att_max_distance = _params.getRawParameterValue(
      ids::params::distance_attenuation_max_distance.toString());
    _dist_att_type = _params.getRawParameterValue(
      ids::params::distance_attenuation_type.toString());

    memset(_prev_weights, 0, sizeof(_prev_weights));
}

void BasicEncoder::recalcInterpolatorBuffers(uint16_t frame_size) {
    if (_prev_interpolator_frame_size == frame_size) return;
    _prev_interpolator_frame_size = frame_size;

    for (uint16_t sample = 0; sample < frame_size; sample++) {
        _interpolator_fade_in[sample]
          = static_cast<float>(sample + 1) / static_cast<float>(frame_size);
        _interpolator_fade_out[sample] = 1.0f - _interpolator_fade_in[sample];
    }
}

void BasicEncoder::process(utils::audio::BufferView<float> buffer) {
    // get weights
    const uint16_t frame_size = buffer.getNumSamples();
    if (frame_size > MAX_FRAME_SIZE) {
        throw; // no can do
    }

    const uint8_t sh_order_local = _sh_order->load();
    const auto num_sh_signals = shSignalCountFromOrder(sh_order_local);

    float weights[MAX_SH_SIGNALS];

    static_assert(sizeof(Direction) == sizeof(float[2]));
    getRSH(sh_order_local, reinterpret_cast<float*>(&_src_dir_deg), 1, weights);

    float gain = calculateGainFromDistance(
      _src_distance, *_dist_att_max_distance,
      enumFromAudioParamRawValue<DistanceAttenuationType>(_dist_att_type));

    recalcInterpolatorBuffers(frame_size);

    /* account for normalisation scheme */
    switch (
      enumFromAudioParamRawValue<NormalizationType>(_normalisation_type)) {
        case NormalizationType::N3D: /* already N3D, do nothing */
            break;
        case NormalizationType::SN3D:
            juce::FloatVectorOperations::multiply(weights, weights, n3d2sn3d,
                                                  num_sh_signals);
            break;
    }

    /**
     * If the ambisonic order changes between calls,
     * some of the _prev_weights may be 0 or values from one of the previous
     * calls, which is fine since the garbage values will only be used for a
     * single sample buffer.
     */
    cblas_sgemm(
      CblasRowMajor, CblasNoTrans, CblasNoTrans, num_sh_signals, frame_size, 1,
      _prev_gain, reinterpret_cast<float*>(_prev_weights), 1,
      buffer.getReadPointer(0), frame_size, 0.0f,
      reinterpret_cast<float*>(_tmp_frame_prev_coeffs), MAX_FRAME_SIZE);
    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, num_sh_signals,
                frame_size, 1, gain, reinterpret_cast<float*>(weights), 1,
                buffer.getReadPointer(0), frame_size, 0.0f,
                reinterpret_cast<float*>(_tmp_frame_curr_coeffs),
                MAX_FRAME_SIZE);

    _prev_gain = gain;
    for (uint16_t ch_ix = 0; ch_ix < num_sh_signals; ch_ix++) {
        _prev_weights[ch_ix] = weights[ch_ix];
    }

    for (uint16_t ch_ix = 0; ch_ix < num_sh_signals; ch_ix++) {
        // write fade out of prev coeffs to tmp fade out
        utility_svvmul(_interpolator_fade_out, _tmp_frame_prev_coeffs[ch_ix],
                       frame_size, _tmp_fade_out[ch_ix]);
        // write fade in of curr coeffs to tmp out
        utility_svvmul(_interpolator_fade_in, _tmp_frame_curr_coeffs[ch_ix],
                       frame_size, _tmp_out[ch_ix]);

        // add curr coeffs (tmp fade out) to prev coeffs (tmp out)
        cblas_saxpy(frame_size, 1.0f, _tmp_fade_out[ch_ix], 1, _tmp_out[ch_ix],
                    1);
    }

    for (int64_t ch_ix = 0; ch_ix < num_sh_signals; ch_ix++) {
        buffer.copyFrom(static_cast<int>(ch_ix), 0,
                        reinterpret_cast<float*>(_tmp_out[ch_ix]), frame_size);
    }
}

} // namespace ambilink::encoders
