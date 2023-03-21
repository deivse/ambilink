#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"

#include <GUI/Screen.h>
#include <GUI/LookAndFeel.h>
#include <Events/Consumers.h>

/**
 * @brief All GUI code.
 * 
 */
namespace ambilink::gui {

/**
 * @brief Implements the plugin's GUI.
 */
class AudioProcessorEditor
  : public juce::AudioProcessorEditor,
    public events::InterceptingPropagator<gui::ScreenChangeCommand>,
    public juce::ValueTree::Listener
{
public:
    AudioProcessorEditor(AudioProcessor&);
    ~AudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /// @brief hides the current screen, and shows the next one.
    void setNextScreen(ScreenID next);

private:
    LookAndFeel _look_and_feel{};

    AudioProcessor& _processor;
    juce::AudioProcessorValueTreeState& _params;
    juce::ValueTree _other_state;

    std::unordered_map<ScreenID, std::unique_ptr<ScreenBase>> _screens{};
    ScreenID _current_screen_id = UndefinedScreenID;

    juce::Component& getCurrentScreen();

    void onInterceptedEvent(events::EventBase& event) final;

    void valueTreePropertyChanged(juce::ValueTree& value_tree,
                                  const juce::Identifier& id) final;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioProcessorEditor)
};

} // namespace ambilink::gui
