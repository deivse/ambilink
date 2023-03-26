#include "ErrorScreen.h"

#include "MainScreen.h"
#include <ValueIDs.h>

#include <IPC/Utils.h>
#include <IPC/States/ErrorState.h>

namespace ambilink::gui {

ErrorScreen::ErrorScreen(juce::AudioProcessorValueTreeState& params,
                         juce::ValueTree& other_state,
                         events::EventConsumer& event_target)
  : Screen<ErrorScreen>{params, other_state, event_target} {
    _other_plugin_state.addListener(this);
    _error_text.setText("...", juce::dontSendNotification);
    _error_text.setColour(juce::Label::ColourIds::textColourId,
                          {255, 100, 100});
    _error_text.setJustificationType(juce::Justification::centred);
    _reset_button.setButtonText("Reset");
    _reset_button.onClick = [this]() { sendEvent(ipc::commands::Connect{}); };

    addAndMakeVisible(_error_text);
    addAndMakeVisible(_reset_button);
}

ErrorScreen::~ErrorScreen() { _params.state.removeListener(this); }

void ErrorScreen::visibilityChanged() {
    if (isVisible()) {
        updateErrorTextFromValueTree();
    }
}

void ErrorScreen::resized() {
    auto local_bounds = getLocalBounds();

    _error_text.setBounds(
      local_bounds.removeFromTop(local_bounds.getHeight() / 4 * 3));
    _reset_button.setBounds(local_bounds.reduced(10));
}

void ErrorScreen::valueTreePropertyChanged(juce::ValueTree&,
                                           const juce::Identifier& prop) {
    if (prop == ids::ipc_error) {
        updateErrorTextFromValueTree();
    }
}

void ErrorScreen::updateErrorTextFromValueTree() {
    _error_text.setText(_other_plugin_state[ids::ipc_error],
                        juce::dontSendNotification);
}
} // namespace ambilink::gui
