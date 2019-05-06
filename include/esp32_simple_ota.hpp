#pragma once

#include <functional>
#include <thread>

namespace esp32_simple_ota {

/**
 * @brief The OTA manager periodically checks for updates.
 * 
 * @details If the version number of the installed app doesn't
 * match the version in the feed, the app from the feed will
 * be downloaded and installed.
 * 
 * The ESP32 `pthreads` library is used for the update task. If
 * you experience a stack overflow, try increasing the default
 * stack size in the sdkconfig.
 * 
 */
class OTAManager {
public:

    /**
     * @brief Construct a new OTAManager object.
     * 
     * @param feed_url URL to the JSON feed.
     * @param installed_version Currently installed version of your app.
     * @param updateCheckInterval Interval, in seconds, between update checks.
     * @param preUpdateCallback Function to call before performing an update. Use this to save data if required.
     * @param safeToUpdateCallback Function to call to see if it's an acceptable time to perform an update.
     *                             Return true if it is. Return false to cancel the update.
     */
    OTAManager(const char *feed_url,
        const char *installed_version,
        unsigned updateCheckInterval,
        std::function<void()> preUpdateCallback = nullptr,
        std::function<bool()> safeToUpdateCallback = nullptr)
        : feed_url{feed_url},
          installed_version{installed_version},
          updateCheckInterval{updateCheckInterval},
          preUpdateCallback{preUpdateCallback},
          safeToUpdateCallback{safeToUpdateCallback}
    {
    }

    /**
     * @brief Begin periodically checking for updates.
     * 
     * @details This function should be called once a network connection has been established.
     */
    void launchTask() {
        thread = std::thread([this]{ thread_func(); });
        thread.detach();
    }

    ~OTAManager()
    {
        thread_running = false;
        thread.join();
    }

private:
    const char *feed_url;
    const char *installed_version;
    const unsigned updateCheckInterval;
    const std::function<void()> preUpdateCallback;
    const std::function<bool()> safeToUpdateCallback;

    std::thread thread;
    void thread_func();
    volatile bool thread_running{true};

    bool update_available(char* url, size_t url_buffer_size);
    void install_update(const char *url);
};

} // namespace esp32_simple_ota