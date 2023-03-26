#pragma once
#include "Screen.h"
#include "Components/SubscribedObjectDisplay.h"
#include "Components/TopPanel.h"
#include "Components/LabeledComponent.h"

class AmbilinkAudioProcessor;

namespace ambilink::gui {

/**
 * @brief Shows the distance attenuation parameters, and the current object.
 * Allows to transition to ObjectSelectionScreen or SettingsScreen.
 *
 */
class MainScreen : public Screen<MainScreen>
{
    using ComboBoxAttachment
      = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment
      = juce::AudioProcessorValueTreeState::SliderAttachment;
    components::TopPanel _top_panel;

    components::LabeledComponent<juce::ComboBox> _dist_att_type_picker;
    components::LabeledComponent<juce::Slider> _dist_att_max_distance;

    std::unique_ptr<ComboBoxAttachment> _dist_att_type_attachment{};
    std::unique_ptr<SliderAttachment> _dist_att_max_distance_attachment{};

    /// @brief populates top panel with the object display component 
    /// and the settings button.
    void initTopPanel();

public:
    MainScreen(juce::AudioProcessorValueTreeState& params,
               juce::ValueTree& other_state,
               events::EventConsumer& event_target);
    void resized() override;
};

} // namespace ambilink::gui
