#include "ByteIO.h"

namespace {
auto spanFromNngBuffer(nng::buffer&& buffer) {
    auto size = buffer.size();
    return std::span<uint8_t>{static_cast<uint8_t*>(buffer.release()), size};
}
} // namespace
namespace ambilink::ipc {
DataReader::DataReader(nng::buffer&& data) : _data{spanFromNngBuffer(std::move(data))} {}

DataReader::~DataReader() { nng_free(_data.data(), _data.size()); }

std::span<uint8_t> DataReader::readBytes(size_t count) {
    if (count > remaining())
        throw std::out_of_range("data_reader_t: attempted read beyond bounds.");
    auto retval = _data.subspan(_read_pos, count);
    _read_pos += count;
    return retval;
}

DataWriter::DataWriter(std::span<uint8_t> initial_data)
  : _data{initial_data.begin(), initial_data.end()} {}
DataWriter::DataWriter(std::vector<uint8_t> initial_data)
  : _data{std::move(initial_data)} {}

void DataWriter::write_bytes(const std::vector<uint8_t>& data) {
    _data.insert(_data.end(), data.begin(), data.end());
}

void DataWriter::write_bytes(const uint8_t* data, size_t size) {
    _data.insert(_data.end(), data, data + size);
}

void DataWriter::write_string(const std::u8string& string) {
    _data.insert(_data.end(), string.begin(), string.end());
}

void DataWriter::write_string(const juce::String& string) {
    _data.insert(_data.end(), string.toRawUTF8(),
                 string.toRawUTF8() + string.getNumBytesAsUTF8());
}

} // namespace ambilink::ipc
