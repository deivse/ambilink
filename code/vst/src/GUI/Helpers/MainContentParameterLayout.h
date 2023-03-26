#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <GUI/Components/TopPanel.h>
#include <GUI/Constants.h>

namespace ambilink::gui {

/**
 * @brief Used to layout items on the MainScreen and SetttingsScreen.
 *
 * @tparam MinWidth min width of parameters
 * @tparam MinHeight min height of parameters
 * @tparam ItemMarginHeight vertical spacing between items
 */
template<int MinWidth = 300, int MinHeight = 26, int ItemMarginHeight = 18>
class MainContentParameterLayout
{
    juce::FlexBox _flex;

public:
    MainContentParameterLayout() {
        _flex.flexDirection = juce::FlexBox::Direction::column;
        _flex.alignItems = juce::FlexBox::AlignItems::stretch;
        _flex.justifyContent = juce::FlexBox::JustifyContent::center;
    }

    /**
     * @brief Performs the layout.
     *
     * @param local_bounds
     * @param top_panel the top panel
     * @param params GUI components that will be positioned in a centered vertical stack.
     */
    template<typename... T>
    void layout(juce::Rectangle<int>&& local_bounds,
                components::TopPanel& top_panel, T&... params) {
        const auto add_item = [&](auto& component) {
            _flex.items.add(juce::FlexItem{component}
                              .withMinHeight(MinHeight)
                              .withMinWidth(MinWidth)
                              .withMargin({0, 0, ItemMarginHeight, 0}));
        };

        (add_item(params), ...);

        top_panel.setBounds(local_bounds.removeFromTop(
          local_bounds.getHeight() * constants::top_panel_height_ratio));

        const auto flex_bounds = local_bounds.reduced(
          local_bounds.getWidth() * 0.05, local_bounds.getHeight() * 0.1);
        _flex.performLayout(flex_bounds);
    }
};

} // namespace ambilink::gui
