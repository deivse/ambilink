#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <stdexcept>

#include <Exceptions.h>

namespace ambilink::utils::audio {

/**
 * @brief A non-owning view over a juce::AudioBuffer<T>, or some section of the buffer.
 * 
 * @tparam T sample type
 */
template<typename T>
class BufferView
{
    juce::AudioBuffer<T>& _buffer;
    const int _start_sample;
    const int _num_samples;

public:
    BufferView(juce::AudioBuffer<T>& buffer)
      : _buffer(buffer), _start_sample(0),
        _num_samples(buffer.getNumSamples()) {}

    BufferView(juce::AudioBuffer<T>& buffer, int start_sample, int num_samples)
      : _buffer(buffer), _start_sample(start_sample),
        _num_samples(num_samples) {
        const auto samples_to_end = buffer.getNumSamples() - _start_sample;
        if (samples_to_end < _num_samples || samples_to_end < 0)
            throw std::invalid_argument("Can't create AudioBufferView for "
                                        "given start_sample and num_samples.");
    };

    BufferView(juce::AudioBuffer<T>& buffer, int start_sample)
      : BufferView(buffer, start_sample,
                   buffer.getNumSamples() - start_sample) {}

    int getNumSamples() const { return _num_samples; }
    int getNumChannels() const { return _buffer.getNumSamples(); }

    /**
     * @brief Clears all channels after (and including) the specified one. (only the samples this view is referencing.) 
     * 
     * @param first_ch_to_clear 
     */
    void clearChannelsStartingFrom(int first_ch_to_clear) {
        for (auto ch = first_ch_to_clear; ch < _buffer.getNumChannels(); ch++) {
            _buffer.clear(ch, _start_sample, _num_samples);
        }
    }

    void copyFrom(int dest_channel, int dest_start_sample, const T* source,
                  int num_samples) {
        _buffer.copyFrom(dest_channel, _start_sample + dest_start_sample,
                         source, num_samples);
    }
    const T* getReadPointer(int channelNumber) {
        return _buffer.getReadPointer(channelNumber) + _start_sample;
    }
};
} // namespace ambilink::utils::audio
