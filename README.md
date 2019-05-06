# ESP32-Simple-OTA

The goal of this project is to provide a simple, lightweight way of performing OTA updates.

## Table of Contents

- [ESP-IDF Support](#esp-idf-support)
- [Getting Started](#getting-started)
  - [Client Side](#client-side)
    - [Code](#code)
    - [Certificate](#certificate)
    - [SDK Configuration](#sdk-configuration)
  - [Server Side](#server-side)

## ESP-IDF Support

This library has been tested with ESP-IDF v3.1. Other versions may work but _at least_ v3.0 is known to be required for [libstdc++ threading support](https://github.com/espressif/esp-idf/issues/690#issuecomment-359146044).

## Getting Started

### Client Side

#### Code

Add this repository as a submodule in your `components` folder:

```sh
git submodule add https://github.com/willson556/esp32-simple-ota.git components/ota
```

In your `main.cpp` file, instantiate an `OTAManager`:

```cpp
static esp32_simple_ota::OTAManager ota {
    MAIN_UPDATE_FEED_URL, // This should be a URL beginning with https:// that points to a JSON feed.
    CURRENT_VERSION, // This is the version of your app that is running.
    60, // Check for updates every 60 seconds
    [](){ ovenController.turnOff(); }, // Pre-update callback
    [](){ return ovenController.getCurrentState() == OvenController::State::Off; } // Safe to update callback
};
```

Then, once your device has a network connection, start the OTA Update monitoring task:

```cpp
ota.launchTask();
```

#### Certificate

You will also need to place a copy of your Certificate Authority's `.pem` file at `server_certs/ca_cert.pem` (relative to project root). You are looking for the certificate in the chain of trust _immediately preceding_ the certificate used on your HTTPS server.

If your CA is Let's Encrypt, you can find the appropriate certificate [here](https://letsencrypt.org/certificates/#intermediate-certificates).

#### SDK Configuration

The project needs to be configured to use the Two OTA partition layout:

```
CONFIG_PARTITION_TABLE_TWO_OTA=y
```
This library uses [ESP HTTPS OTA](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/esp_https_ota.html) under the hood.
It's recommended that [Signed App Verification](https://docs.espressif.com/projects/esp-idf/en/latest/security/secure-boot.html#signed-app-verify) be enabled.

### Server Side

The server side requirements are very simple -- any HTTPS server that can host a couple of files will do.

Two files need to be placed on the server:

- [JSON Feed](https://github.com/willson556/esp32-simple-ota/blob/master/sample_feed.json):
    ```jsonc
    {
        "version": "1.0.0",
        "update-url": "https://myserver.com/updates/firmware-1.0.0.bin" // This URL must be < 256 characters.
    }
    ```

- Update File:

    This is the `.bin` file that is the output of the build. The `update-url` in the JSON feed should point to this file.
