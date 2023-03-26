#include "PluginEditor.h"

#include <array>

#include "PluginProcessor.h"
#include "Utility/Utils.h"

#include "GUI/ConnectionScreen.h"
#include "GUI/MainScreen.h"
#include "GUI/ObjectSelectionScreen.h"
#include "GUI/SettingsScreen.h"
#include "GUI/ErrorScreen.h"
#include "GUI/LookAndFeel.h"

#include <IPC/Utils.h>
#include <IPC/States/Disconnected.h>
#include <IPC/States/ErrorState.h>

namespace ambilink::gui {

AudioProcessorEditor::AudioProcessorEditor(AudioProcessor& processor_)
  : juce::AudioProcessorEditor{&processor_},
    events::InterceptingPropagator<gui::ScreenChangeCommand>(processor_),
    _processor{processor_}, _params{_processor.getParams()},
    _other_state(_processor.getOtherState()) {
    // helper to initialise all screens
    const auto add_screens = [this]<typename... ScreenTypes>() {
        const auto add_screen = [this](auto&& screen) {
            addChildComponent(*screen);
            _screens[screen->getScreenID()] = std::move(screen);
        };
        (add_screen(
           std::make_unique<ScreenTypes>(_params, _other_state, *this)),
         ...);
    };

    add_screens.operator()<MainScreen, SettingsScreen, ObjectSelectionScreen,
                           ConnectionScreen, ErrorScreen>();

    const auto ipc_state_id
      = static_cast<ipc::StateID>(int(_other_state[ids::ipc_client_state]));
    if (ipc_state_id == ipc::state::Disconnected::id)
        _current_screen_id = ConnectionScreen::id;
    else if (ipc_state_id == ipc::state::ErrorState::id)
        _current_screen_id = ErrorScreen::id;
    else
        _current_screen_id = MainScreen::id;

    getCurrentScreen().setVisible(true);
    setSize(400, 300);
    setResizable(false, false);
    setLookAndFeel(&_look_and_feel);

    _other_state.addListener(this);
}

AudioProcessorEditor::~AudioProcessorEditor() {
    _other_state.removeListener(this);
    setLookAndFeel(nullptr);
}

void AudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(gui::colors::background);
}

void AudioProcessorEditor::resized() {
    // Current screen always takes up whole window.
    getCurrentScreen().setBounds(getLocalBounds());
}

juce::Component& AudioProcessorEditor::getCurrentScreen() {
    jassert(_current_screen_id != ambilink::gui::UndefinedScreenID);
    return *_screens[_current_screen_id];
}

void AudioProcessorEditor::setNextScreen(ambilink::gui::ScreenID next) {
    jassert(_screens.contains(next));
    getCurrentScreen().setVisible(false);
    _current_screen_id = next;
    getCurrentScreen().setVisible(true);
    resized();
}

void AudioProcessorEditor::onInterceptedEvent(events::EventBase& event) {
    events::Dispatcher dispatcher(event);
    dispatcher.dispatch<ScreenChangeCommand>([this](ScreenChangeCommand& evt) {
        setNextScreen(evt.next_screen_id);
        return true;
    });
    jassert(event.handled);
}

void AudioProcessorEditor::valueTreePropertyChanged(
  juce::ValueTree&, const juce::Identifier& id) {
    if (id == ids::ipc_client_state) {
        if (ipc::isInState<ipc::state::ErrorState>(_other_state)) {
            setNextScreen(ErrorScreen::id);
        } else if (ipc::isInState<ipc::state::Disconnected>(_other_state)) {
            setNextScreen(ConnectionScreen::id);
        } else if (_current_screen_id == ConnectionScreen::id
                   || _current_screen_id == ErrorScreen::id) {
            setNextScreen(MainScreen::id);
        }
    }
}

} // namespace ambilink::gui
