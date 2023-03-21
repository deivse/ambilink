#pragma once

#include <JuceHeader.h>

#include <DataTypes.h>
#include <ValueIDs.h>
#include <Utility/Utils.h>
#include <IPC/Client.h>
#include <Events/Consumers.h>
#include <Events/Sources.h>

#include <Encoder/BasicEncoder.h>

namespace ambilink {

/**
 * @brief Basically contains the whole plugin, implements audio processing,
 * creates instances of the AudioProcessorEditor when requested by host
 * software, provides information about the plugin, etc.
 *
 */
class AudioProcessor : public juce::AudioProcessor,
                       public ambilink::events::EventPropagator,
                       public ambilink::events::EventSource
{
public:
    AudioProcessor();
    ~AudioProcessor() override;

    ////////////////////////////
    ///// audio processing /////
    ////////////////////////////

    void prepareToPlay(double sample_rate,
                       int max_expected_samples_per_block) final;
    void releaseResources() final;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const final;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) final;
    void processBlock(juce::AudioBuffer<double>&, juce::MidiBuffer&) final {
        // Unsupported, just to silence warning.
    }

    //////////////////////
    ///// gui editor /////
    //////////////////////

    juce::AudioProcessorEditor* createEditor() final;
    bool hasEditor() const final;

    ///////////////////////
    ///// plugin info /////
    ///////////////////////

    const juce::String getName() const final;

    bool acceptsMidi() const final;
    bool producesMidi() const final;
    bool isMidiEffect() const final;
    double getTailLengthSeconds() const final;

    ///////////////////////////////////////////////////
    ///// plugin programs (presets) - unsupported /////
    ///////////////////////////////////////////////////

    int getNumPrograms() final;
    int getCurrentProgram() final;
    void setCurrentProgram(int index) final;
    const juce::String getProgramName(int index) final;
    void changeProgramName(int index, const juce::String& new_name) final;

    ///////////////////////////////
    ///// state serialisation /////
    ///////////////////////////////

    void getStateInformation(juce::MemoryBlock& dest_data) final;
    void setStateInformation(const void* data, int size_in_bytes) final;

    /////////////////////////////
    ///// Ambilink-specific /////
    /////////////////////////////

    juce::AudioProcessorValueTreeState& getParams() { return _params; }
    juce::ValueTree& getOtherState() { return _other_state; }

    /////////////////////////////

private:
    juce::AudioProcessorValueTreeState _params;
    juce::ValueTree _other_state{ids::ambilink_other_state};

    ambilink::ipc::IPCClient _ipc_client;
    ambilink::encoders::BasicEncoder _encoder;

    /**
     * @brief Used to process audio in offline rendering mode.
     * Gets data directly from the ipc::state::OfflineRendering instance
     * held by `_ipc_client`.
     */
    void processInRenderingMode(juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioProcessor)
};

} // namespace ambilink
