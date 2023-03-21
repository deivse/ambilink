#pragma once

#include <memory>
#include <juce_gui_basics/juce_gui_basics.h>

#include <GUI/Screen.h>
#include <GUI/MainScreen.h>

namespace ambilink::gui::components {

/// @brief makes a button that sends a ScreenChangeCommand to change to TargetScreenT
template<typename TargetScreenT>
std::shared_ptr<juce::TextButton>
  makeScreenTransitionBtn(ScreenBase* current_screen, juce::String button_text) {
    auto btn = std::make_shared<juce::TextButton>();
    btn->setButtonText(std::move(button_text));
    btn->onClick
      = [current_screen]() { current_screen->setNextScreen<TargetScreenT>(); };
    return btn;
}

/// @brief makes a button that sends a ScreenChangeCommand to change to MainScreen
inline auto makeBackToMainButton(ScreenBase* current_screen) {
    return makeScreenTransitionBtn<MainScreen>(current_screen, "< Back");
}

} // namespace ambilink::gui::components
