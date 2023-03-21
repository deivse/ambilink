#include "SubscribedObjectDisplay.h"
#include "../../PluginEditor.h"

#include "../../Utility/Utils.h"
#include "../ObjectSelectionScreen.h"
#include "../LookAndFeel.h"

namespace ambilink::gui::components {

SubscribedObjectDisplay::SubscribedObjectDisplay(
  juce::ValueTree& other_state, events::EventConsumer& event_target)
  : events::EventSource(event_target), _other_state(other_state) {
    addAndMakeVisible(_subbed_object_name);
    addAndMakeVisible(_edit_button);

    _other_state.addListener(this);

    _edit_button.setButtonText("Edit");
    _edit_button.onClick = [this]() {
        sendEvent(ScreenChangeCommand::fromScreenType<ObjectSelectionScreen>());
    };

    updateFromState();
}

SubscribedObjectDisplay::~SubscribedObjectDisplay() {
    _other_state.removeListener(this);
}

void SubscribedObjectDisplay::valueTreePropertyChanged(
  juce::ValueTree&, const juce::Identifier& id) {
    if (id == ids::object_deleted || id == ids::object_name) {
        updateFromState();
    }
}

void SubscribedObjectDisplay::updateFromState() {
    auto object_name = _other_state[ids::object_name];
    _subbed_object_name.setText(object_name.isVoid() ? "Not Subscribed"
                                                     : object_name.toString(),
                                juce::dontSendNotification);

    if (_other_state[ids::object_deleted]) {
        _subbed_object_name.setText(object_name.toString() + " [DELETED]",
                                    juce::dontSendNotification);
        _subbed_object_name.setColour(juce::Label::ColourIds::textColourId, colors::error_red);
    } else {
        _subbed_object_name.setColour(juce::Label::ColourIds::textColourId, colors::white);
    }
}

void SubscribedObjectDisplay::resized() {
    auto local_bounds = getLocalBounds();
    juce::FlexBox flex;
    flex.flexDirection = juce::FlexBox::Direction::row;
    flex.items.add(juce::FlexItem{_subbed_object_name}.withFlex(4));
    flex.items.add(juce::FlexItem{_edit_button}.withMaxWidth(55).withFlex(1));
    flex.performLayout(local_bounds);
}

void SubscribedObjectDisplay::paint(juce::Graphics& g) {
    g.fillAll(colors::panel::component::background);
}

} // namespace ambilink::gui::components
