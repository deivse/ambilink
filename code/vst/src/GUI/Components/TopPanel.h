#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <list>

namespace ambilink::gui::components {

/**
 * @brief Container that has 2 stacks of items - one at the start of the
 * container, one at the end. Items can be added to each, the stacks grow
 * towards each other. For horizontal orientation, the "start" stack is at the
 * left, and the "end" stack is at the right, for vertical it's top and bottom
 * respectively.
 *
 */
class TopPanel : public juce::Component
{
public:
    enum class Orientation
    {
        HORIZONTAL,
        VERTICAL
    };

    TopPanel(Orientation orientation = Orientation::HORIZONTAL);
    void addItemStart(std::shared_ptr<juce::Component> component,
                      uint8_t flex_ratio = 1);
    void addItemEnd(std::shared_ptr<juce::Component> component,
                    uint8_t flex_ratio = 1);
    void addGapStart(uint8_t flex_ratio = 1);
    void addGapEnd(uint8_t flex_ratio = 1);

    void setStackRatio(uint8_t left_stack_flex, uint8_t right_stack_flex);

    constexpr static int getItemHeight(int panel_height) {
        return panel_height - _panel_margin * 2;
    }

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    /// @brief default gap between items
    constexpr static uint8_t _default_item_gap = 10;
    /// @brief margin around the whole top panel
    constexpr static uint8_t _panel_margin = 8;

    std::list<std::shared_ptr<juce::Component>> _items{};
    juce::FlexBox _flex_start{};
    juce::FlexBox _flex_end{};

    uint8_t _start_flex_ratio = 1;
    uint8_t _end_flex_ratio = 1;

    const Orientation _orientation;
};
} // namespace ambilink::gui::components
