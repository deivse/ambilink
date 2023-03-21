#include "SettingsScreen.h"
#include "Helpers/MainContentParameterLayout.h"
#include "MainScreen.h"
#include "Constants.h"
#include "Components/ScreenTransitionButton.h"

#include <Encoder/Constants.h>

#include <ValueIDs.h>

namespace ambilink::gui {

SettingsScreen::SettingsScreen(juce::AudioProcessorValueTreeState& params,
                               juce::ValueTree& other_state,
                               events::EventConsumer& event_target)
  : Screen{params, other_state, event_target} {
    _norm_type_picker.setLabelText("Normalization Type");
    _norm_type_picker.component.addItemList(encoders::NormalizationTypeStrings,
                                            1);

    _norm_type_combo_attachment = std::make_unique<ComboBoxAttachment>(
      _params, ids::params::normalization_type.toString(), _norm_type_picker);


    _ambi_order_input.component.setSliderStyle(
      juce::Slider::SliderStyle::IncDecButtons);
    _ambi_order_input.setLabelText("Ambisonics Order");
    _ambi_order_slider_attachment = std::make_unique<SliderAttachment>(
      _params, ids::params::ambisonics_order.toString(), _ambi_order_input);

    initTopPanel();

    addAndMakeVisible(_top_panel);
    addAndMakeVisible(_norm_type_picker);
    addAndMakeVisible(_ambi_order_input);
};

void SettingsScreen::initTopPanel() {
    _top_panel.addItemEnd(components::makeBackToMainButton(this), 1);
    _top_panel.addGapEnd(2);
}

void SettingsScreen::resized() {
    MainContentParameterLayout{}.layout(
      getLocalBounds(), _top_panel, _norm_type_picker,
      _ambi_order_input);
};

} // namespace ambilink::gui
