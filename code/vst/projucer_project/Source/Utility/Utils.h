#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

/**
 * @brief Generic helper (utility) stuff.
 * 
 */
namespace ambilink::utils {

/// @brief audio utilities.
namespace audio {
    /**
     * @brief Simple utility to add up all channels in a buffer to a single mono
     * channel.
     *
     * @tparam SampleT sample
     * @param buffer
     */
    template<typename SampleT>
    void downmixToMono(juce::AudioBuffer<SampleT>& buffer) {
        for (auto ch = 1; ch < buffer.getNumChannels(); ch++) {
            buffer.addFrom(0, 0, buffer.getReadPointer(ch),
                           buffer.getNumSamples());
        }
    }
} // namespace audio

juce::String juceStringFromU8String(const std::u8string& string);

juce::StringArray
  juceStringArrayFromUTF8StringVec(const std::vector<std::u8string>& vec);

std::u8string u8stringFromJuceString(const juce::String& string);

/**
 * @brief Invokes a callable passed in the constructor when it's destructor gets
 * called.
 *
 * @tparam FuncT type of callable.
 */
template<typename FuncT>
class OnScopeExit
{
    FuncT _func;

public:
    /**
     * @param func the callable that will be invoked upon destruction.
     */
    OnScopeExit(FuncT&& func) : _func{std::forward<FuncT>(func)} {};
    ~OnScopeExit() { _func(); }
};

/**
 * @brief Allows to pass a list of types to a consructor, and create using
 * declarations for lists of types.
 */
template<typename... T>
struct TypeList
{};

/**
 * @brief Allows to differentiate values of same type with different meaning.
 *
 * @tparam T underlying type
 * @tparam DerivedT struct that derives from NamedType and implements a
 * specific named type.
 */
template<typename T, typename DerivedT>
struct NamedType
{
    T value;
    NamedType(T&& value_) : value(std::forward(value_)){};

    operator T&() { return value; }

    using WrappedType = T;
    using WrapperType = DerivedT;
};

/// @brief Helper to define new named types
#define ambilink_declareNamedType(name, wrapped_type)                      \
    struct name : public ::ambilink::utils::NamedType<wrapped_type, name>  \
    {                                                                      \
        using ::ambilink::utils::NamedType<wrapped_type, name>::NamedType; \
    }

template<typename T>
concept IsNamedType = std::is_base_of_v<
  NamedType<typename T::WrappedType, typename T::UniqueParam>, T>;


#ifdef DEBUG
#define debug_only //
#else
#define debug_only
#endif

} // namespace ambilink::utils
