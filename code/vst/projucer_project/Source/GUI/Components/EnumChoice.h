#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <optional>

namespace ambilink::gui::components {

/**
 * @brief Dropdown selector for enum values
 * 
 * @tparam EnumT the enum type
 * @tparam UnderlyingT the underlying type of the enum, will be deducted.
 */
template<class EnumT, typename UnderlyingT = std::underlying_type_t<EnumT>>
    requires std::is_convertible_v<UnderlyingT, int>
               && (std::numeric_limits<int>::max()
                   > std::numeric_limits<UnderlyingT>::max())
class EnumChoice : public juce::Component, public juce::ComboBox::Listener
{
    constexpr static int _label_combo_gap{5};
    juce::Label _label;

    void comboBoxChanged(juce::ComboBox* changed_combo) override {
        if (!on_selection_changed) return;
        if (changed_combo == &combo) {
            on_selection_changed(getSelectedValue().value());
        }
    }
public:

    juce::ComboBox combo;
    using EnumType = EnumT;

    std::function<void(EnumT)> on_selection_changed = [](EnumT) {};

    /**
     * @brief Construct a new EnumChoice component.
     * 
     * @param values the possible values of the enum.
     * @param default_value the initial value.
     * @param label label for the selector.
     */
    EnumChoice(const std::vector<std::pair<EnumT, juce::String>>& values,
               EnumT default_value, juce::String label) {
        _label.setText(label, juce::dontSendNotification);
        for (auto&& [enum_val, name] : values) {
            jassert(static_cast<int>(enum_val) >= 0);
            combo.addItem(
              enum_val == default_value ? name + " (default)" : name,
              static_cast<int>(enum_val) + 1); // +1 because id can't be 0
        }
        combo.setSelectedId(static_cast<int>(default_value) + 1);
        combo.addListener(this);

        _label.setJustificationType(juce::Justification::right);
        addAndMakeVisible(_label);
        addAndMakeVisible(combo);
    }

    /**
     * @brief Get the currently selected value
     * 
     * @return std::optional<EnumT> the value, or nullopt if nothing is selected.
     */
    std::optional<EnumT> getSelectedValue() {
        if (const int selected_id = combo.getSelectedId(); selected_id == 0) {
            return std::nullopt;
        } else {
            return EnumT{static_cast<UnderlyingT>(selected_id - 1)};
        }
    }

    /// @brief Selects the item corresponding to provided enum value.
    EnumChoice& operator= (EnumT new_val)  {
        combo.setSelectedId(static_cast<int>(new_val) + 1);
        return *this;
    }

    /// @brief Selects the item corresponding to provided enum value.
    void setValue(EnumT new_val) {
        *this = new_val;
    }

    /// @brief defines the layout of the component. 
    void resized() final {
        juce::FlexBox flex;
        flex.flexDirection = juce::FlexBox::Direction::row;
        flex.items.add(juce::FlexItem{_label}.withFlex(2).withMargin(
          {0, _label_combo_gap, 0, 0}));
        flex.items.add(juce::FlexItem{combo}.withFlex(3));
        flex.performLayout(getLocalBounds());
    }
};

} // namespace ambilink::gui::components
