#pragma once
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#define declare_juce_id(name)     \
    const juce::Identifier name { \
#name                     \
    }

/// @brief definitions of `juce::Identifier`s used for plugin parameters.
namespace ambilink::ids {

declare_juce_id(ambilink_params);
/// @brief vst audio params
namespace params {
    declare_juce_id(ambisonics_order);
    declare_juce_id(normalization_type);
    declare_juce_id(distance_attenuation_type);
    declare_juce_id(distance_attenuation_max_distance);
} // namespace params

declare_juce_id(ambilink_other_state);

declare_juce_id(object_name);
declare_juce_id(object_deleted);

declare_juce_id(ipc_client_state);
declare_juce_id(curr_direction_azimuth_deg);
declare_juce_id(curr_direction_elevation_deg);
declare_juce_id(curr_distance);
declare_juce_id(object_list);

/// @brief Value with this ID will be set if an exception
/// occurs during IPC communication.
declare_juce_id(ipc_error);

/// @brief ids for properties that aren't VST audio params, but are serialised.
const std::array serialized_non_params{object_name, object_deleted};

} // namespace ambilink::ids
