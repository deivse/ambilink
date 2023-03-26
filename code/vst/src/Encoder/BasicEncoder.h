#pragma once
#include <cstdint>
#include <saf.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <optional>

#include <DataTypes.h>
#include <Events/Consumers.h>
#include <Utility/AudioBufferView.h>

#include "Constants.h"

namespace ambilink::encoders {

inline constexpr uint16_t shSignalCountFromOrder(uint8_t sh_order) {
    return (sh_order + 1) * (sh_order + 1);
};

/**
 * @brief Calculates the max ambisonics order for a given signal count.
 * If `num_output_channels` directly corresponds to some Ambisonics order, then
 * that order is returned. Otherwise the greatest order where the numbber of
 * required output channels is less then `num_output_channels` is returned.
 */
uint8_t maxOrderFromOutChannelCount(uint16_t num_output_channels);

/**
 * @brief Checks if `channel_count` is a valid channel count corresponding to
 * some SH order.
 */
bool isValidOutputChannelCount(int channel_count);

constexpr uint16_t MAX_FRAME_SIZE = 2048 * 4;
constexpr uint16_t MAX_SH_ORDER
  = 5; // Due to VST3 limitations, see
       // https://forums.steinberg.net/t/vst3-hoa-support-3rd-order/201766
constexpr uint16_t MIN_SH_SIGNALS = shSignalCountFromOrder(1);
constexpr uint16_t MAX_SH_SIGNALS = shSignalCountFromOrder(MAX_SH_ORDER);

// asserts to ensure no atomic values used are implemented using locks.
static_assert(std::atomic<NormalizationType>::is_always_lock_free);
static_assert(std::atomic<uint8_t>::is_always_lock_free);

/**
 * @brief Basic encoder performing ambisonic panning and distance-based gain
 * attenuation.
 */
class BasicEncoder : public juce::ValueTree::Listener
{
    juce::AudioProcessorValueTreeState& _params;
    Direction _src_dir_deg{0, 0}; /**< Source directions, in degrees */
    Distance _src_distance{0};

    /* user parameters */
    std::atomic<float>* _normalisation_type;
    std::atomic<float>* _sh_order; /**< Current SH encoding order */

    std::atomic<float>* _dist_att_max_distance;
    std::atomic<float>* _dist_att_type;

    /* Internal audio buffers */
    float _tmp_frame_prev_coeffs[MAX_SH_SIGNALS][MAX_FRAME_SIZE];
    float _tmp_frame_curr_coeffs[MAX_SH_SIGNALS][MAX_FRAME_SIZE];
    float _tmp_fade_out[MAX_SH_SIGNALS][MAX_FRAME_SIZE];
    float _tmp_out[MAX_SH_SIGNALS][MAX_FRAME_SIZE];
    float _interpolator_fade_out[MAX_FRAME_SIZE];
    float _interpolator_fade_in[MAX_FRAME_SIZE];
    uint16_t _prev_interpolator_frame_size = 0;

    /* Internal variables */
    float _prev_weights[MAX_SH_SIGNALS];
    float _prev_gain{0};

    /**
     * @brief Calculates interpolator buffers for given frame size
     * Basically fills interpolator buffers with `frame_size` values
     * linearly interpolated from 0.0f to 1.0f.
     */
    void recalcInterpolatorBuffers(uint16_t frame_size);

public:
    BasicEncoder(juce::AudioProcessorValueTreeState& audio_params);

    int getCurrOutputChannels() { return shSignalCountFromOrder(*_sh_order); }

    /**
     * @brief Performs ambisonics panning based on last direction set with
     * updateDirAndDistance and the value's of the plugin's audio parms (order,
     * normalisation, channel ordering)
     * @warning buffer must contain a sufficient number of channels for the
     * current ambisonics order.
     *
     * @note Only the first input channel of the buffer is used.
     */
    void process(utils::audio::BufferView<float> buffer);

    /**
     * @brief Sets the direction and distance that will be used by the next
     * `process` call.
     */
    inline void updateDirAndDistance(Direction direction, Distance distance) {
        _src_dir_deg = direction;
        _src_distance = distance;
    }

    /**
     * @brief Sets the direction and distance that will be used by the next
     * `process` call.
     */
    inline void updateDirAndDistance(DirectionWithDistance dir_with_distance) {
        _src_dir_deg = dir_with_distance.direction;
        _src_distance = dir_with_distance.distance;
    }
};

} // namespace ambilink::encoders
