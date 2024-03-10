#include "TKS/Events.hpp"

#include <string>
#include <iostream>
#include <thread>

class EventProducer
{
public:
    // This is the event that will trigger
    TKS::Events::Event<int> IntIncrementedEvent;

    EventProducer()
    {
        this->_counter = 0;
    }

    void Start()
    {
        this->_internalThread = std::thread([&](){ init(); });
    }

private:
    int _counter;
    std::thread _internalThread;

    void init()
    {
        while (true)
        {
            // Trigger the event, passing it data
            IntIncrementedEvent(_counter++);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};

int main(int argc, char *argv[])
{
    EventProducer eventProducer;

    // Create a handler function that will be subscribed to the event
    TKS::Events::EventHandler<int> handler(
        [](int counter)
        { std::cout << counter << std::endl; }
    );

    // Subscribe to the event
    eventProducer.IntIncrementedEvent += handler;
    eventProducer.Start();

    // There's also the option to unsubscribe if needed.
    // eventProducer.IntIncrementedEvent -= handler;

    while (true)
        std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}
