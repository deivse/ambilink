#pragma once
#include "States/State.h"
#include <juce_data_structures/juce_data_structures.h>
#include <ValueIDs.h>

namespace ambilink::ipc {

template<typename StateT>
static bool isInState(juce::ValueTree& valtree_with_id_prop) {
    jassert(valtree_with_id_prop.hasProperty(ids::ipc_client_state));
    return static_cast<state::StateID>(
             int(valtree_with_id_prop[ids::ipc_client_state]))
           == StateT::id;
}

} // namespace ambilink::ipc
