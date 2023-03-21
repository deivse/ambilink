#include "ObjectSelectionScreen.h"
#include "Components/ScreenTransitionButton.h"
#include "Constants.h"
#include "MainScreen.h"
#include "LookAndFeel.h"

#include <IPC/Utils.h>
#include <IPC/Commands.h>
#include <IPC/States/Disconnected.h>

namespace ambilink::gui {

ObjectSelectionScreen::ObjectSelectionScreen(
  juce::AudioProcessorValueTreeState& params, juce::ValueTree& other_state,
  events::EventConsumer& event_target)
  : Screen{params, other_state, event_target} {
    setWantsKeyboardFocus(true);

    _object_list.setModel(this);
    _object_list.setRowHeight(object_list_row_height);
    addAndMakeVisible(_object_list);

    initTopPanel();
    addAndMakeVisible(_top_panel);

    updateFromState();
    _other_plugin_state.addListener(this);
    sendEvent(ipc::commands::UpdateObjectList{});
};

ObjectSelectionScreen::~ObjectSelectionScreen() {
    _other_plugin_state.removeListener(this);
}

void ObjectSelectionScreen::initTopPanel() {
    _search_field->addListener(this);
    _search_field->setJustification(juce::Justification::centredLeft);
    _search_field->setTextToShowWhenEmpty("Type here to search objects",
                                          colors::panel::component::hint_text);

    _top_panel.setStackRatio(2, 1);
    _top_panel.addItemStart(_search_field);
    _top_panel.addItemEnd(components::makeBackToMainButton(this), 1);
    _top_panel.addGapEnd(1);
}

void ObjectSelectionScreen::resized() {
    auto local_bounds = getLocalBounds();
    _top_panel.setBounds(local_bounds.removeFromTop(
      local_bounds.getHeight() * constants::top_panel_height_ratio));
    _object_list.setBounds(local_bounds.reduced(5));
}

void ObjectSelectionScreen::visibilityChanged() {
    if (isShowing()) {
        sendEvent(ipc::commands::UpdateObjectList{});
        _search_field->grabKeyboardFocus();
        startTimerHz(update_interval_hz);
    } else {
        _search_field->clear();
        stopTimer();
        updateShownObjects();
    }
}

void ObjectSelectionScreen::timerCallback() {
    sendEvent(ipc::commands::UpdateObjectList{});
}

void ObjectSelectionScreen::valueTreePropertyChanged(
  juce::ValueTree&, const juce::Identifier& property) {
    if (property == ids::object_list) {
        updateFromState();
    } else if (property == ids::ipc_client_state
               && !ipc::isInState<ipc::state::Disconnected>(
                 _other_plugin_state)) {
        sendEvent(ipc::commands::UpdateObjectList{});
    }
}

void ObjectSelectionScreen::updateFromState() {
    if (auto obj_list_value = _other_plugin_state[ids::object_list];
        !obj_list_value.isVoid()) {
        _all_objects.clearQuick();
        _all_objects = *_other_plugin_state[ids::object_list].getArray();
    } else {
        _all_objects = {};
    }
    updateShownObjects();
}

void ObjectSelectionScreen::updateShownObjects() {
    _shown_objects.clear();
    if (_search_field->isEmpty()) {
        std::for_each(_all_objects.begin(), _all_objects.end(),
                      [this](const auto& object_name) {
                          _shown_objects.emplace_back(object_name);
                      });
    } else {
        auto search_string = _search_field->getText().toLowerCase();
        std::map<int, std::vector<juce::StringRef>> tmp{};
        for (const auto& object_name : _all_objects) {
            auto index = object_name.toLowerCase().indexOf(search_string);
            if (index == -1) continue;
            tmp[index].emplace_back(object_name);
        }
        for (auto&& [_, items] : tmp) {
            for (auto&& item : items) {
                _shown_objects.emplace_back(item);
            }
        }
    }
    _object_list.updateContent();
}

int ObjectSelectionScreen::getNumRows() { return _shown_objects.size(); }

void ObjectSelectionScreen::paintListBoxItem(int row_number, juce::Graphics& g,
                                             int width, int height,
                                             bool rowIsSelected) {
    g.setColour(colors::background);
    g.fillRect(getBounds());

    // If searching, the first item will be selected when the Return
    // key is pressed.
    if ((!_search_field->isEmpty() && row_number == 0) || rowIsSelected) {
        g.setColour(colors::orange_highlight);
    } else {
        g.setColour(colors::white);
    }

    g.setFont(font_height);
    g.drawText(_shown_objects[static_cast<size_t>(row_number)], 5, 0, width,
               height, juce::Justification::centredLeft, true);
}

void ObjectSelectionScreen::onSelect(int selected_item) {
    sendEvent(ipc::commands::SubscribeToObject{
      _shown_objects[static_cast<size_t>(selected_item)]});
    _object_list.updateContent();
    _object_list.setSelectedRows({}, juce::dontSendNotification);
    _other_plugin_state.setProperty(ids::object_name, "...", nullptr);
    setNextScreen<MainScreen>();
}

void ObjectSelectionScreen::selectedRowsChanged(int lastRowselected) {
    onSelect(lastRowselected);
}

void ObjectSelectionScreen::textEditorTextChanged(juce::TextEditor&) {
    updateShownObjects();
}

void ObjectSelectionScreen::textEditorReturnKeyPressed(juce::TextEditor&) {
    if (!_search_field->isEmpty() && !_shown_objects.empty())
        onSelect(0); // If enter pressed when searching, the first
                     // item is selected
}

void ObjectSelectionScreen::textEditorEscapeKeyPressed(juce::TextEditor&) {
    setNextScreen<MainScreen>();
}

bool ObjectSelectionScreen::keyPressed(const juce::KeyPress& key) {
    if (juce::KeyPress::escapeKey == key) {
        setNextScreen<MainScreen>();
        return true;
    }
    return false;
}

} // namespace ambilink::gui
