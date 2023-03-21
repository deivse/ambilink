#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <IPC/Commands.h>
#include <IPC/Utils.h>
#include <IPC/States/OfflineRendering.h>
#include <IPC/States/Subscribed.h>
#include <IPC/States/ErrorState.h>

#include <spdlog/spdlog.h>

namespace {
auto createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout out{};
    out.add(std::make_unique<juce::AudioParameterInt>(
      ambilink::ids::params::ambisonics_order.toString(), "Ambisonics: Order",
      1, ambilink::encoders::MAX_SH_ORDER, 1));
    out.add(std::make_unique<juce::AudioParameterChoice>(
      ambilink::ids::params::normalization_type.toString(),
      "Ambisonics: Normalization Convention",
      ambilink::encoders::NormalizationTypeStrings,
      static_cast<int>(ambilink::encoders::NormalizationType::DEFAULT)));

    out.add(std::make_unique<juce::AudioParameterChoice>(
      ambilink::ids::params::distance_attenuation_type.toString(),
      "Distance-Based Volume Attenuation: Curve",
      ambilink::encoders::DistanceAttenuationTypeStrings,
      static_cast<int>(ambilink::encoders::DistanceAttenuationType::DEFAULT)));
    out.add(std::make_unique<juce::AudioParameterFloat>(
      ambilink::ids::params::distance_attenuation_max_distance.toString(),
      "Distance-Based Volume Attenuation: Max Distance", 1, 10000, 500));
    return out;
}
} // namespace

namespace ambilink {

AudioProcessor::AudioProcessor()
  : juce::AudioProcessor(
    BusesProperties()
      .withInput("Mono Input", juce::AudioChannelSet::mono(), true)
      .withOutput("Ambisonics Output (up to 5th order)",
                  juce::AudioChannelSet::ambisonic(5), true)),
    events::EventPropagator(_ipc_client),
    events::EventSource{static_cast<events::EventConsumer&>(*this)},
    _params(*this, nullptr, ids::ambilink_params, createParameterLayout()),
    _ipc_client(_other_state), _encoder(_params) {
#ifdef DEBUG
    spdlog::set_level(spdlog::level::debug);
#else
    spdlog::set_level(spdlog::level::warn);
#endif
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] [thread %t] %v");
}

AudioProcessor::~AudioProcessor() = default;

//////////////////////////////////////////////////////////////////////

void AudioProcessor::prepareToPlay(double /*sample_rate*/,
                                   int /*maxExpectedSamplesPerBlock*/) {
    if (_ipc_client.isInState<ipc::state::Subscribed>() && isNonRealtime()) {
        sendEvent(ipc::commands::EnableRenderingMode{});
        while (_ipc_client.isInState<ipc::state::Subscribed>()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }
    } else if (_ipc_client.isInState<ipc::state::OfflineRendering>()
               && !isNonRealtime()) {
        sendEvent(ipc::commands::DisableRenderingMode{});
        while (_ipc_client.isInState<ipc::state::OfflineRendering>()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }
    }
}

void AudioProcessor::releaseResources() {}

void AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                  juce::MidiBuffer& /*midiMessages*/) {
    juce::ScopedNoDenormals noDenormals;

    // TODO: allow user to use downmix or just the first channel - potential
    // phase issues
    utils::audio::downmixToMono(buffer);
    if (_ipc_client.isInState<ipc::state::OfflineRendering>()) {
        processInRenderingMode(buffer);
    } else {
        // In states other than subscribed this just returns `Direction{0, 0},
        // and Distance{0}`
        _encoder.updateDirAndDistance(_ipc_client.getCurrentDirection_rt(),
                                      _ipc_client.getCurrentDistance_rt());
        _encoder.process(buffer);
    }
}

void AudioProcessor::processInRenderingMode(juce::AudioBuffer<float>& buffer) {
    juce::AudioPlayHead::CurrentPositionInfo position{};
    if (!getPlayHead()->getCurrentPosition(position)) {
        jassertfalse;
        return _encoder.process(buffer);
        // TODO: inform user that this host is unsupported.
    }

    const auto time_secs = position.timeInSeconds;

    DirectionWithDistance dir_with_distance{};

    // State change locked
    if (auto state_access
        = _ipc_client.getCurrentState<ipc::state::OfflineRendering>();
        state_access.has_value()) {
        dir_with_distance
          = state_access.value()->getDirectionAndDistanceAtTime(time_secs);
        // TODO: optimisation opportunity - get whole slice, only lock state
        // change mutex once per slice
    }

    // If IPC client switches to a different state, such as ObjectDeleted,
    // all-zero values are used.
    _encoder.updateDirAndDistance(dir_with_distance);
    _encoder.process(buffer);
}

bool AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    return layouts.getMainInputChannels() <= 2
           && encoders::isValidOutputChannelCount(
             layouts.getMainOutputChannels());
}

//////////////////////////////////////////////////////////////////////

bool AudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* AudioProcessor::createEditor() {
    return new gui::AudioProcessorEditor(*this);
}

//////////////////////////////////////////////////////////////////////

void AudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto combined_state = _params.copyState();
    for (auto id : ids::serialized_non_params) {
        combined_state.setProperty(id, _other_state[id], nullptr);
    }
    juce::MemoryOutputStream out{};
    combined_state.writeToStream(out);
    destData.insert(out.getData(), out.getDataSize(), 0);
}

void AudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto deserialized_combined_state
      = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));

    for (auto id : ids::serialized_non_params) {
        jassert(deserialized_combined_state.hasProperty(id));
        _other_state.setProperty(id, deserialized_combined_state[id], nullptr);
        deserialized_combined_state.removeProperty(id, nullptr);
    }

    jassert(deserialized_combined_state.hasType(_params.state.getType()));
    _params.replaceState(deserialized_combined_state);
}

//////////////////////////////////////////////////////////////////////

const juce::String AudioProcessor::getName() const { return JucePlugin_Name; }

bool AudioProcessor::acceptsMidi() const { return false; }

bool AudioProcessor::producesMidi() const { return false; }

bool AudioProcessor::isMidiEffect() const { return false; }

double AudioProcessor::getTailLengthSeconds() const { return 0.0; }

//////////////////////////////////////////////////////////////////////

int AudioProcessor::getNumPrograms() {
    return 1; // NB: some hosts don't cope very well if you tell them
              // there are 0 programs, so this should be at least 1,
              // even if you're not really implementing programs.
}

int AudioProcessor::getCurrentProgram() { return 0; }

void AudioProcessor::setCurrentProgram(int /*index*/) {}

const juce::String AudioProcessor::getProgramName(int /*index*/) { return {}; }

void AudioProcessor::changeProgramName(int /*index*/,
                                       const juce::String& /*newName*/) {}

} // namespace ambilink

/// @brief creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new ambilink::AudioProcessor();
}
