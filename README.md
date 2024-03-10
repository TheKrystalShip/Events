# TKS/Events

A simple Event<T> & EventHandler<T> library implementation in C++

# Install

Copy the `src/Events.hpp` file into `/usr/local/include` or any other `include` directory your compiler will look into.

# Usage

There's a simple example set up in `src/main.cpp` showcasing the basic usage of creating, triggering and subscribing to an event.

## Event provider example

```c++
namespace TKS
{
    // This class periodically checks the connection status of an Arduino::WiFiClient
    // and triggers an event whenever the connection status changes.
    class WiFiGuardian
    {
    public:
        TKS::Events::Event<Arduino::WIFI_STATUS> StatusChange;

        WiFiGuardian(Arduino::WiFiClient *wifi)
        {
            _wifi = wifi;
            _previousStatus = wifi->status();
        }

        ~WiFiGuardian()
        {
            delete _wifi;
        }

        void Start()
        {
            this->Start(DEFAULT_WAIT_MS);
        }

        void Start(unsigned int milisecondInterval)
        {
            this->_milisecondInterval = milisecondInterval;
            this->_isRunning = true;
            _thread = std::thread([this](){ init(); });
        }

        void Stop()
        {
            if (!_isRunning)
                return;

            _isRunning = false;

            if (_thread.joinable())
                _thread.join();
        }
    private:
        Arduino::WiFiClient *_wifi;
        Arduino::WIFI_STATUS _previousStatus;
        bool _isRunning;
        std::thread _thread;
        unsigned int _milisecondInterval;

        void init()
        {
            std::cout << "WiFiGuardian thread started" << std::endl;

            while (_isRunning)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(_milisecondInterval));
                std::cout << "Checking wifi status" << std::endl;

                if (_wifi == nullptr)
                {
                    std::cout << "ERROR: _wifi is nullptr, breaking loop" << std::endl;
                    break;
                }

                Arduino::WIFI_STATUS newStatus = _wifi->status();

                // Trigger the event if there's a status change
                if (newStatus != _previousStatus)
                {
                    StatusChange(newStatus);
                    _previousStatus = newStatus;
                }
            }
        }
    };
}

```

## Event consumer example

```c++
#include <TKS/Events>

#include <mutex>
#include <iostream>

int main(int argc, char *argv[])
{
    // Create an instance of TKS::Events::EventHandler<T> which
    // accepts a lambda function as a parameter.
    // This will be called by the event
    TKS::Events::EventHandler<Arduino::WIFI_STATUS> OnWifiStatusChange(
        [](Arduino::WIFI_STATUS newStatus)
        {
            switch (newStatus) {
            case Arduino::WIFI_STATUS::CONNECTED:
                // Process some data, send some messages
                break;
            case Arduino::WIFI_STATUS::DISCONNECTED:
                // Log some error, queue up data locally and wait for reconnect
                break;
            default:
                break;
            }
        }
    );

    std::cout << "Subscribing to OnWifiStatusChange event" << std::endl;

    Arduino::WiFiClient wifiClient;
    TKS::WiFiGuardian _guardian(&wifiClient);

    // Subscribe to the event
    _guardian.StatusChange += OnWifiStatusChange;
    _guardian.Start();

    // [...]

    // Unsubscribe from the event
    _guardian.StatusChange -= OnWifiStatusChange;
    _guardian.Stop();

    return 0;
}
```
