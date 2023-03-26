#pragma once
#include "Screen.h"

#include "Components/EnumChoice.h"
#include "Components/TopPanel.h"
#include "Components/LabeledComponent.h"

namespace ambilink::gui {

/**
 * @brief The settings screen, currently includes the ambisonics settings.
 * 
 */
class SettingsScreen : public Screen<SettingsScreen>
{
    using ComboBoxAttachment
      = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment
      = juce::AudioProcessorValueTreeState::SliderAttachment;

    // GUI components controlling the audio parameters
    components::LabeledComponent<juce::ComboBox> _norm_type_picker;
    components::LabeledComponent<juce::Slider> _ambi_order_input{};

    // attachments used to connect GUI components to audio parameters
    std::unique_ptr<ComboBoxAttachment> _norm_type_combo_attachment{};
    std::unique_ptr<SliderAttachment> _ambi_order_slider_attachment{};

    /// @brief contains the back button
    components::TopPanel _top_panel{};

    /// @brief populates the top panel
    void initTopPanel();

public:
    SettingsScreen(juce::AudioProcessorValueTreeState& params,
                   juce::ValueTree& other_state,
                   events::EventConsumer& event_target);
    void resized() override;
};

} // namespace ambilink::gui
