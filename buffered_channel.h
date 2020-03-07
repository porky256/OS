#ifndef BUFFERED_CHANNEL_H_
#define BUFFERED_CHANNEL_H_

#include <queue>
#include <mutex>
#include <condition_variable>

template<class T>
class BufferedChannel {
public:
    explicit BufferedChannel(int size) : size(size) {}

    void Send(T value) {
        std::unique_lock<std::mutex> locker(lock);
        channel_is_full.wait(locker, [&]() {
            if (is_closed) throw std::runtime_error("Channel is closed!");
            return que.size() < size;
        });
        que.push(value);
        channel_is_full.notify_all();
    }

    std::pair<T, bool> Recv() {
        std::unique_lock<std::mutex> locker(lock);
        channel_is_empty.wait(locker, [&]() {
            return !que.empty() || is_closed;
        });
        if (que.empty()) {
            channel_is_empty.notify_all();
            return {T(), false};
        }
        channel_is_empty.notify_all();
        T ans = que.front();
        que.pop();
        return {ans, true};
    }

    void Close() {
        std::unique_lock<std::mutex>(lock);
        is_closed = true;
        channel_is_empty.notify_all();
        channel_is_full.notify_all();
    }

private:
    std::condition_variable channel_is_full;
    std::condition_variable channel_is_empty;
    std::mutex lock;
    size_t size;
    std::queue<T> que;
    bool is_closed = false;
};

#endif // BUFFERED_CHANNEL_H_

