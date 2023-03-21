#pragma once
#include <utility>
#include <juce_gui_basics/juce_gui_basics.h>

namespace ambilink::gui::components {

enum class LabelPosition
{
    Left,
    Bottom
};

/**
 * @brief A simple wrapper that adds a label to any component.
 *
 * @tparam ComponentT type of the wrapped component
 */
template<typename ComponentT,
         LabelPosition label_position = LabelPosition::Left>
class LabeledComponent : public juce::Component
{
    juce::Label _label;
    constexpr static uint8_t _gap = 10;

public:
    ComponentT component;

    /**
     * @brief Construct a new LabeledComponent
     * 
     * @tparam ComponentConstructorArgs 
     * @param args arguments for the constructor of ComponentT
     */
    template<typename... ComponentConstructorArgs>
    LabeledComponent(ComponentConstructorArgs&&... args)
      : component{std::forward<ComponentConstructorArgs>(args)...} {
        switch (label_position) {
            case LabelPosition::Left:
                _label.setJustificationType(juce::Justification::right);
                break;
            case LabelPosition::Bottom:
                _label.setJustificationType(juce::Justification::centred);
        }
        addAndMakeVisible(_label);
        addAndMakeVisible(component);
    }
    
    void setLabelText(const juce::String& new_label) {
        _label.setText(new_label, juce::dontSendNotification);
    }

    void resized() final {
        juce::FlexBox flex;
        if constexpr (label_position == LabelPosition::Left) {
            flex.flexDirection = juce::FlexBox::Direction::row;
        } else if constexpr (label_position == LabelPosition::Bottom) {
            flex.flexDirection = juce::FlexBox::Direction::columnReverse;
        }

        flex.items.add(
          juce::FlexItem{_label}.withFlex(2).withMargin({0, _gap, 0, 0}));
        flex.items.add(juce::FlexItem{component}.withFlex(3));
        flex.performLayout(getLocalBounds());
    }

    operator ComponentT&() { return component; }
};

} // namespace ambilink::gui::components
