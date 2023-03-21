#include "Utils.h"

namespace ambilink::utils {
juce::String juceStringFromU8String(const std::u8string& string) {
    return juce::String::fromUTF8(reinterpret_cast<const char*>(string.data()),
                                  string.size());
}

juce::StringArray
  juceStringArrayFromUTF8StringVec(const std::vector<std::u8string>& vec) {
    juce::StringArray retval{};
    for (auto&& str: vec) {
        retval.add(juceStringFromU8String(str));
    }
    return retval;
  }

  std::u8string u8stringFromJuceString(const juce::String &string) {
    return {reinterpret_cast<const char8_t*>(string.toRawUTF8())};
  }
} // namespace ambilink::utils
