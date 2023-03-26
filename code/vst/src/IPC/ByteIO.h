#pragma once
#include <nngpp/buffer.h>
#include <span>
#include <stdexcept>
#include <vector>
#include <string>

#include <juce_core/juce_core.h>

namespace ambilink::ipc {

/**
 * @brief Helper for reading IPC message data.
 */
class DataReader
{
    std::span<uint8_t> _data;
    size_t _read_pos = 0;

public:
    explicit DataReader(nng::buffer&& data);
    DataReader(const DataReader& other) = default;
    DataReader(DataReader&& other) = default;
    DataReader& operator=(const DataReader& other) = default;
    DataReader& operator=(DataReader&& other) = default;

    ~DataReader();

    std::span<uint8_t> readBytes(size_t count);

    size_t remaining() { return _data.size() - _read_pos; }

    /**
     * @brief Read sizeof(T) bytes of data, interpreted as T.
     */
    template<typename T>
    T read() {
        if (sizeof(T) > remaining())
            throw std::out_of_range("data_reader_t: attempted read beyond bounds.");
        T retval = *reinterpret_cast<const T*>(_data.data() + _read_pos);
        _read_pos += sizeof(T);
        return retval;
    }

    /**
     * @brief Move the read position forward by `count` bytes.
     */
    void discard(size_t count) { _read_pos += count; }

    /**
     * @brief Move the read position forward by `sizeof(T)` bytes.
     */
    template<typename T>
    void discard() {
        discard(sizeof(T));
    }
};

/**
 * @brief Helper for writing IPC message data.
 */
class DataWriter
{
    std::vector<uint8_t> _data{};

public:
    DataWriter() = default;
    explicit DataWriter(std::span<uint8_t> initial_data);
    explicit DataWriter(std::vector<uint8_t> initial_data);

    /**
     * @brief Returns a copy of the currently written data.
     *
     * @return std::vector<uint8_t>
     */
    std::vector<uint8_t> get_data() { return _data; }

    /**
     * @brief Moves the written data from the internal store. Only callable on an r-value.
     * No copy is performed.
     *
     * @return std::vector<uint8_t>
     */
    std::vector<uint8_t> release_data() && { return std::move(_data); }

    size_t written() { return _data.size(); }
    
    template<typename T>
    void write(T value) {
        // doesn't take endiannes into account because intended for IPC communication on
        // the same machine.

        auto write_pos = _data.size();
        _data.resize(_data.size() + sizeof(T));
        memcpy(_data.data() + write_pos, &value, sizeof(T));
    }

    void write_bytes(const std::vector<uint8_t>& data);
    void write_string(const std::u8string& string);
    void write_string(const juce::String& string);
    void write_bytes(const uint8_t* data, size_t size);
};

} // namespace ambilink::ipc
