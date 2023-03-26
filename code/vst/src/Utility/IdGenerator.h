#pragma once

#include <cstdint>
#include <limits>

#include <juce_core/juce_core.h>

namespace ambilink::utils {

/**
 * @brief Helper to generate sequential numeric IDs for CRTP classes, so each
 * concrete class/CRTP class instatiation has a unique ID.
 *
 * @tparam BaseClassT the base of the CRTP class from which leaf classes derive
 * @tparam IntegerIdT integer type used for the id (defaults to uint16_t)
 * @tparam null_id_ ID reserved as the null value (defaults to the lowest value
 * representable by IntegerIdT)
 */
template<typename BaseClassT, typename IntegerIdT = uint16_t,
         IntegerIdT null_id_ = std::numeric_limits<IntegerIdT>::min()>
struct SequentialIdGenerator
{
    using IdType = IntegerIdT;
    static_assert(std::numeric_limits<IdType>::lowest() <= null_id_);

    constexpr static IdType null_id = null_id_;
    inline static IdType curr_id = null_id;

    /// @brief returns the next ID in the sequence. 
    static IdType getNextId() {
        jassert(curr_id + 1 <= std::numeric_limits<IntegerIdT>::max());
        return ++curr_id;
    }
};
} // namespace ambilink::utils
