#pragma once
#include "Screen.h"
#include "Components/TopPanel.h"

namespace ambilink::gui {

/**
 * @brief Shows a list of objects in the Blender scene, allows user to subscribe
 * to an object.
 */
class ObjectSelectionScreen : public Screen<ObjectSelectionScreen>,
                              public juce::ListBoxModel,
                              public juce::TextEditor::Listener,
                              public juce::Timer

{
    /// @brief when the ObjectSelectionScreen is visible, the object list is
    /// updated at this interval.
    constexpr static uint8_t update_interval_hz = 3;
    constexpr static uint8_t object_list_row_height = 22;
    constexpr static uint8_t font_height = object_list_row_height * 0.7;

    juce::ListBox _object_list;
    /// @brief contains the search field.
    components::TopPanel _top_panel;
    std::shared_ptr<juce::TextEditor> _search_field
      = std::make_shared<juce::TextEditor>();

    juce::StringArray _all_objects;
    std::vector<juce::StringRef> _shown_objects;

    void updateShownObjects();

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width,
                          int height, bool rowIsSelected) final;

    void valueTreePropertyChanged(juce::ValueTree&,
                                  const juce::Identifier& prop) final;

    void onSelect(int selected_item);
    void selectedRowsChanged(int last_selected_row) final;
    void textEditorTextChanged(juce::TextEditor&) final;
    void textEditorReturnKeyPressed(juce::TextEditor&) final;
    void textEditorEscapeKeyPressed(juce::TextEditor&) final;

    void updateFromState();
    void visibilityChanged() final;

    bool keyPressed(const juce::KeyPress& key) final;

    /// @brief populates the top panel and sets the layout.
    void initTopPanel();

    /// @brief sends a request to update objectList to the IPC Client.
    void timerCallback() final;

public:
    ObjectSelectionScreen(juce::AudioProcessorValueTreeState& params,
                          juce::ValueTree& other_state,
                          events::EventConsumer& event_target);
    ~ObjectSelectionScreen();

    void resized() override;
};

} // namespace ambilink::gui
