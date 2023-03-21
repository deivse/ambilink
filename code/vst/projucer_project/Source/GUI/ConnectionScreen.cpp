#include "ConnectionScreen.h"

#include "MainScreen.h"
#include <ValueIDs.h>

#include <IPC/Utils.h>
#include <IPC/States/Disconnected.h>

namespace ambilink::gui {

ConnectionScreen::ConnectionScreen(juce::AudioProcessorValueTreeState& params,
                                   juce::ValueTree& other_state,
                                   events::EventConsumer& event_target)
  : Screen<ConnectionScreen>{params, other_state, event_target} {
    _text.setText("Connecting... Please launch the Blender plugin.",
                  juce::dontSendNotification);
    _text.setColour(juce::Label::ColourIds::textColourId, {255, 255, 255});
    _text.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(_text);
}

ConnectionScreen::~ConnectionScreen() { _params.state.removeListener(this); }

void ConnectionScreen::resized() { _text.setBounds(getLocalBounds()); }
} // namespace ambilink::gui
