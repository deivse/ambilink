#pragma once
#include "Screen.h"

namespace ambilink::gui {

/**
 * @brief Shown when the IPCClient is in Disconnected state.
 */
class ConnectionScreen : public Screen<ConnectionScreen>
{
    juce::Label _text;

public:
    ConnectionScreen(juce::AudioProcessorValueTreeState& params,
                     juce::ValueTree& other_state,
                     events::EventConsumer& event_target);
    ~ConnectionScreen();

    void resized() override;
};
} // namespace ambilink::gui
