#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace ambilink::gui {

/// @brief Colors used throughout the UI.
namespace colors {
    const juce::Colour background{23, 20, 30};
    const juce::Colour popup_background{background.brighter(0.1f)};
    const juce::Colour selected_background{82, 55, 136};
    const juce::Colour white{233, 233, 230};
    const juce::Colour orange_highlight{255, 193, 130};
    const juce::Colour error_red{255, 101, 74};
    const juce::Colour slider_bar{selected_background};

    /// @brief colors used for the top panel
    namespace panel {
        const juce::Colour background{43, 40, 50};
        /// @brief colors used for components placed on the top panel
        namespace component {
            const juce::Colour background = panel::background.brighter(0.2f);
            const juce::Colour hint_text = component::background.brighter(0.3f);
        } // namespace component
    }     // namespace panel
} // namespace colors

/// @brief defines a custom look for the plugin
class LookAndFeel : public juce::LookAndFeel_V4
{
    using UIColour = juce::LookAndFeel_V4::ColourScheme::UIColour;
    inline static const std::map<UIColour, juce::Colour> _ui_colors{
      {UIColour::menuBackground, colors::panel::background},
      {UIColour::defaultText, colors::white},
      {UIColour::menuText, colors::white},
      {UIColour::highlightedFill, colors::selected_background},
      {UIColour::highlightedText, colors::orange_highlight},
    };

public:
    /// @brief assigns colors to components
    LookAndFeel();
};

} // namespace ambilink::gui
