#pragma once
#include "Screen.h"

namespace ambilink::gui {

/**
 * @brief Shown when an IPC error occurs.
 */
class ErrorScreen : public Screen<ErrorScreen>
{
    juce::Label _error_text;
    juce::TextButton _reset_button;

    void updateErrorTextFromValueTree();
public:
    ErrorScreen(juce::AudioProcessorValueTreeState& params,
                     juce::ValueTree& other_state,
                     events::EventConsumer& event_target);
    ~ErrorScreen();

    void visibilityChanged() final;

    void resized() override;

    void valueTreePropertyChanged(juce::ValueTree&,
                                  const juce::Identifier& prop) final;
};
} // namespace ambilink::gui
