#pragma once
#include <cassert>
#include <juce_gui_basics/juce_gui_basics.h>

#include <Events/Sources.h>
#include <Events/Consumers.h>
#include <Utility/IdGenerator.h>

namespace ambilink::gui {

class ScreenBase;

using ScreenIDGenerator = utils::SequentialIdGenerator<ScreenBase>;
using ScreenID = ScreenIDGenerator::IdType;
using SetScreenCallback = std::function<void(ScreenID)>;

constexpr static ScreenID UndefinedScreenID = ScreenIDGenerator::null_id;

template<typename DerivedT>
class Screen;
template<typename T>
concept IsScreen = std::is_base_of_v<Screen<T>, T>;

/// @brief Event/command used to change the currently shown screen.
struct ScreenChangeCommand : public events::Event<ScreenChangeCommand>
{
    ScreenID next_screen_id;
    ScreenChangeCommand(ScreenID next_screen_id_)
      : next_screen_id(next_screen_id_){};

    template<IsScreen NextScreenT>
    static ScreenChangeCommand fromScreenType() {
        return {NextScreenT::id};
    }
};

/**
 * @brief Base class for all screens. A screen fills the whole Ambilink window,
 * transitions are realised by changing the active screen by sendind
 * ScreenChangeCommand events.
 */
class ScreenBase : public juce::Component,
                   public events::EventSource,
                   public juce::ValueTree::Listener
{
protected:
    juce::AudioProcessorValueTreeState& _params;
    juce::ValueTree _other_plugin_state;

public:
    /**
     * @param params audio params
     * @param other_state other params
     * @param target event consumer (propagator) that events from this screen
     * will be sent to.
     */
    ScreenBase(juce::AudioProcessorValueTreeState& params,
               juce::ValueTree& other_state, events::EventConsumer& target)
      : events::EventSource{target}, _params(params),
        _other_plugin_state(other_state){};
    virtual ~ScreenBase() = default;

    
    /// @brief Convenience wrapper for sending ScreenChangeCommands.
    template<IsScreen NextScreenT>
    void setNextScreen() {
        sendEvent(ScreenChangeCommand::fromScreenType<NextScreenT>());
    }

    /// @brief Get the ID of this screen.
    virtual ScreenID getScreenID() = 0;
};

/**
 * @brief Utilises CRTP to assign a unique identifier to each derived
 * screen class.
 *
 * @tparam DerivedT the concrete screen class.
 */
template<typename DerivedT>
class Screen : public ScreenBase
{
public:
    using ScreenBase::ScreenBase;
    
    static ScreenID getScreenID_static() { return id; }
    ScreenID getScreenID() final { return id; }

    inline static const ScreenID id = ScreenIDGenerator::getNextId();
};

} // namespace ambilink::gui
