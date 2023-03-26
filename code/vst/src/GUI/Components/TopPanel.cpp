#include "TopPanel.h"
#include <GUI/LookAndFeel.h>

namespace ambilink::gui::components {

TopPanel::TopPanel(Orientation orientation) : _orientation(orientation) {
    const bool horizontal = _orientation == Orientation::HORIZONTAL;
    _flex_start.flexDirection = horizontal ? juce::FlexBox::Direction::row
                                           : juce::FlexBox::Direction::column;
    _flex_start.justifyContent = juce::FlexBox::JustifyContent::flexStart;

    _flex_end.flexDirection = horizontal
                                ? juce::FlexBox::Direction::rowReverse
                                : juce::FlexBox::Direction::columnReverse;
    _flex_end.justifyContent = juce::FlexBox::JustifyContent::flexEnd;
}

void TopPanel::addItemStart(std::shared_ptr<juce::Component> component,
                               uint8_t flex_ratio) {
    auto& item =  _items.emplace_back(std::move(component));
    addAndMakeVisible(*item);
    _flex_start.items.add(
      juce::FlexItem{*item}.withFlex(flex_ratio).withMinWidth(5));
}

void TopPanel::addItemEnd(std::shared_ptr<juce::Component> component,
                             uint8_t flex_ratio) {
    auto& item = _items.emplace_back(std::move(component));
    addAndMakeVisible(*item);
    _flex_end.items.add(
      juce::FlexItem{*item}.withFlex(flex_ratio).withMinWidth(5));
}

void TopPanel::addGapStart(uint8_t flex_ratio) {
    _flex_start.items.add(juce::FlexItem{}.withFlex(flex_ratio));
}

void TopPanel::addGapEnd(uint8_t flex_ratio) {
    _flex_end.items.add(juce::FlexItem{}.withFlex(flex_ratio));
}

void TopPanel::setStackRatio(uint8_t start_stack_ration,
                                uint8_t end_stack_ratio) {
    _start_flex_ratio = start_stack_ration;
    _end_flex_ratio = end_stack_ratio;
}

void TopPanel::paint(juce::Graphics& g) {
    g.fillAll(colors::panel::background);
}

void TopPanel::resized() {
    auto local_bounds = getLocalBounds().reduced(_panel_margin);
    const auto start_stack_fraction = static_cast<float>(_start_flex_ratio)
                                      / (_start_flex_ratio + _end_flex_ratio);

    auto start_flex_bounds
      = _orientation == Orientation::HORIZONTAL
          ? local_bounds.removeFromLeft(local_bounds.toFloat().getWidth()
                                        * start_stack_fraction)
          : local_bounds.removeFromTop(local_bounds.toFloat().getHeight()
                                       * start_stack_fraction);

    _flex_start.performLayout(start_flex_bounds);
    _flex_end.performLayout(local_bounds);
}

} // namespace ambilink::gui::components
