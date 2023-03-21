#include "LookAndFeel.h"

namespace ambilink::gui {
LookAndFeel::LookAndFeel() {
    auto color_scheme = getCurrentColourScheme();
    for (auto&& [color_id, color] : _ui_colors) {
        color_scheme.setUIColour(color_id, color);
    }

    setColour(juce::TextButton::ColourIds::buttonColourId, colors::background);
    setColour(juce::TextButton::ColourIds::buttonOnColourId,
              colors::selected_background);
    setColour(juce::TextButton::ColourIds::textColourOffId, colors::white);
    setColour(juce::TextButton::ColourIds::textColourOnId,
              colors::orange_highlight);

    setColour(juce::TextEditor::ColourIds::backgroundColourId,
              colors::panel::component::background);

    setColour(juce::ListBox::ColourIds::backgroundColourId, colors::background);

    setColour(juce::ComboBox::ColourIds::backgroundColourId,
              colors::panel::component::background);
    setColour(juce::PopupMenu::ColourIds::backgroundColourId,
              colors::popup_background);
    setColour(juce::PopupMenu::ColourIds::highlightedBackgroundColourId,
              colors::selected_background);

    setColour(juce::Slider::ColourIds::textBoxBackgroundColourId,
              colors::panel::component::background);
    setColour(juce::Slider::ColourIds::trackColourId,
              colors::slider_bar);
}
} // namespace ambilink::gui
