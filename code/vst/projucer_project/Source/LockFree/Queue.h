#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <numeric>

#include <Utility/Utils.h>

/**
 * @brief Data structures for real-time thread use.
 */
namespace ambilink::lock_free {

/**
 * @brief A fixed-size lock-free FIFO queue designed to be used by a single
 * producer and a single consumer thread.
 *
 * @tparam ItemT type of items stored in the queue.
 * @tparam MaxQueueSize max number of queued items.
 */
template<typename ItemT, size_t MaxQueueSize>
    requires(MaxQueueSize < std::numeric_limits<uint16_t>::max()
             && std::is_default_constructible_v<ItemT>)
class Queue
{
    std::atomic<uint16_t> _write_pos;
    std::atomic<uint16_t> _read_pos;
    static_assert(std::atomic<uint16_t>::is_always_lock_free);

    std::array<ItemT, MaxQueueSize> _items;

    static uint16_t wrapPos(uint16_t pos) { return pos % MaxQueueSize; }

public:
    /**
     * @brief Push an item. Blocks if the queue is full with an
     * optional block time upper bound.
     *
     * @tparam MaxBlockTimeMilliseconds if != 0 then the function will
     * return false after blocking for more than
     * MaxBlockTimeMilliseconds, otherwise it will block until there
     * is a free spot in the queue.
     * @param item the item to push.
     * @return true item pushed.
     * @return false MaxBlockTimeMilliseconds was surpassed.
     */
    template<size_t MaxBlockTimeMilliseconds = 0>
    bool blockingPush(ItemT&& item) {
        if constexpr (MaxBlockTimeMilliseconds == 0) {
            while (wrapPos(_write_pos + 1) == _read_pos) {
            };
        } else {
            const auto start_time = std::chrono::steady_clock::now();
            while (wrapPos(_write_pos + 1) == _read_pos) {
                if (std::chrono::steady_clock::now() - start_time
                    > std::chrono::milliseconds(MaxBlockTimeMilliseconds))
                    return false;
            };
        }

        _items[_write_pos] = std::move(item);
        _write_pos = wrapPos(_write_pos + 1);
        return true;
    }

    /**
     * @brief Push an item to the queue, non-blocking version.
     *
     * @param item the item to push.
     * @return true The queue was not full, item pushed.
     * @return false The queue was full.
     */
    bool pushOrFail(ItemT&& item) {
        if (wrapPos(_write_pos + 1) == _read_pos) return false;
        _items[_write_pos] = std::move(item);
        _write_pos = wrapPos(_write_pos + 1);
        return true;
    }

    /**
     * @brief Checks if the queue is empty.
     */
    bool empty() { return _write_pos == _read_pos; }

    /**
     * @brief Get the oldest item from the queue.
     */
    ItemT pop() {
        utils::OnScopeExit increase_read_pos{
          [this]() { _read_pos = wrapPos(_read_pos + 1); }};
        return std::move(_items[_read_pos]);
    }
};
} // namespace ambilink::lock_free
