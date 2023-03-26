#pragma once

#include "../../IPC/Client.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Screen.h"

class AmbilinkAudioProcessorEditor;

namespace ambilink::gui::components {

/**
 * @brief Displays the name of the subscribed object, and allows to switch to
 * ObjectSelectionScreen.
 */
class SubscribedObjectDisplay : public juce::Component,
                                public juce::ValueTree::Listener,
                                public events::EventSource
{
    juce::Label _subbed_object_name{};
    juce::TextButton _edit_button{};

    juce::ValueTree _other_state;
    void updateFromState();

public:
    SubscribedObjectDisplay(juce::ValueTree& other_state,
                            events::EventConsumer& event_target);
    ~SubscribedObjectDisplay();

    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                  const juce::Identifier& property) override;
    void resized() override;
    void paint(juce::Graphics& g) override;
};
} // namespace ambilink::gui::components
