#include "MainScreen.h"
#include "Helpers/MainContentParameterLayout.h"
#include "LookAndFeel.h"
#include "SettingsScreen.h"
#include "Constants.h"

#include <Encoder/Constants.h>

namespace ambilink::gui {

MainScreen::MainScreen(juce::AudioProcessorValueTreeState& params,
                       juce::ValueTree& other_state,
                       events::EventConsumer& event_target)
  : Screen{params, other_state, event_target} {
    initTopPanel();
    addAndMakeVisible(_top_panel);

    _dist_att_type_picker.setLabelText("Distance Attenuation");
    _dist_att_type_picker.component.addItemList(
      encoders::DistanceAttenuationTypeStrings, 1);
    _dist_att_type_attachment = std::make_unique<ComboBoxAttachment>(
      _params, ids::params::distance_attenuation_type.toString(),
      _dist_att_type_picker);
    addAndMakeVisible(_dist_att_type_picker);

    _dist_att_type_picker.component.onChange =
      [this]() {
          const bool dist_att_enabled
            = _dist_att_type_picker.component.getSelectedId()
              != static_cast<int>(encoders::DistanceAttenuationType::NONE) + 1;
          _dist_att_max_distance.setEnabled(dist_att_enabled);
      };

      _dist_att_max_distance.setLabelText("Dist. Att. Max Distance");
    _dist_att_max_distance.component.setSliderStyle(
      juce::Slider::SliderStyle::LinearBar);
    _dist_att_max_distance_attachment = std::make_unique<SliderAttachment>(
      _params, ids::params::distance_attenuation_max_distance.toString(),
      _dist_att_max_distance);
    addAndMakeVisible(_dist_att_max_distance);
};

void MainScreen::resized() {
    MainContentParameterLayout<350>{}.layout(getLocalBounds(), _top_panel,
                                             _dist_att_type_picker,
                                             _dist_att_max_distance);
}

void MainScreen::initTopPanel() {
    _top_panel.addItemStart(
      std::make_shared<components::SubscribedObjectDisplay>(_other_plugin_state,
                                                            _event_target));
    auto settings_button = std::make_shared<juce::TextButton>();
    settings_button->setButtonText("Settings");
    settings_button->onClick = [this]() {
        sendEvent(ScreenChangeCommand::fromScreenType<SettingsScreen>());
    };
    _top_panel.addItemEnd(std::move(settings_button));
    _top_panel.addGapEnd(1);
    _top_panel.setStackRatio(3, 2);
}

} // namespace ambilink::gui
