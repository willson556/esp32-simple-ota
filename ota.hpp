#pragma once

#include <functional>
#include <thread>

namespace HttpsOta {

class Ota {
public:
    Ota(const char *feed_url, const char *installed_version, std::function<void()> preUpdateCallback = nullptr)
        : feed_url{feed_url}, installed_version{installed_version}, preUpdateCallback{preUpdateCallback}
    {
    }

    void launchTask() {
        thread = std::thread([this]{ thread_func(); });
        thread.detach();
    }

    ~Ota()
    {
        thread_running = false;
        thread.join();
    }

private:
    const char *feed_url;
    const char *installed_version;
    const std::function<void()> preUpdateCallback;

    std::thread thread;
    void thread_func();
    volatile bool thread_running{true};

    bool update_available(char* url, size_t url_buffer_size);
    void install_update(const char *url);
};

} // namespace HttpsOta