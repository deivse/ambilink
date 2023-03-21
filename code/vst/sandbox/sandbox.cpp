#include <iostream>
#include <span>
#include <stdexcept>

#include <nngpp/socket.h>
#include <nngpp/protocol/req0.h>
#include <nngpp/protocol/sub0.h>
#include <spdlog/spdlog.h>
#include <fstream>

#include <saf.h>
#include <thread>

void fatal(const char* str, int retval) {
    throw std::runtime_error(fmt::format("{} ({})", str, retval));
}

struct vec3_t
{
    float x;
    float y;
    float z;
} __attribute__((packed));

class ByteReader
{
    nng::buffer _nng_buffer;
    std::span<uint8_t> _data;
    size_t _read_pos = 0;

public:
    ByteReader(nng::buffer&& data)
      : _nng_buffer(std::move(data)), _data{static_cast<uint8_t*>(_nng_buffer.data()),
                                            _nng_buffer.size()} {}

    std::span<uint8_t> getBytes(size_t count) {
        if (count > remaining())
            throw std::out_of_range("data_reader_t: attempted read beyond bounds.");
        auto retval = _data.subspan(_read_pos, count);
        _read_pos += count;
        return retval;
    }

    size_t remaining() { return _data.size() - _read_pos; }

    template<typename T>
    T read() {
        if (sizeof(T) > remaining())
            throw std::out_of_range("data_reader_t: attempted read beyond bounds.");
        T retval = *reinterpret_cast<const T*>(_data.data() + _read_pos);
        _read_pos += sizeof(T);
        return retval;
    }
};

int main() {
    try {
        // std::cout << "SAF Version: " << SAF_VERSION_STRING << std::endl;
        // std::cout << "NNG Version: " << nng_version() << std::endl;
        // auto sock = nng::req::open();
        // sock.dial("ipc:///tmp/ipc2");
        // sock.send("TEST");
        // auto buff = sock.recv();
        // std::cout << buff.data<char>() << std::endl;

        auto sock = nng::sub::open();
        sock.set_opt(NNG_OPT_SUB_SUBSCRIBE, nng::buffer{});
        sock.set_opt_ms(NNG_OPT_RECVTIMEO, 0);
        try {
        sock.dial("ipc:///tmp/ambilink_pubsub"); // THE SUB DIALS!;
        } catch (nng::exception& e) {
            std::cout << e.who() << e.what() << std::endl;
            return 1;
        }

        while (1) {
            try {
                auto msg = sock.recv();
                ByteReader reader(std::move(msg));
                auto ambilink_id = reader.read<uint16_t>();
                auto command = reader.read<uint8_t>();
                std::cout << "ambilink_id: " << ambilink_id << std::endl;
                std::cout << "command: " << command << std::endl;

                if (command == 0x00) {
                    auto pos = reader.read<vec3_t>();
                    std::cout << "pos: " << pos.x << ", " << pos.y << ", " << pos.z
                              << std::endl;
                } else {
                    std::cout << "other command" << std::endl;
                }
            } catch (const nng::exception& e) {
                continue;
            }
        }

    } catch (std::exception& e) {
        spdlog::error(e.what());
    }
}
