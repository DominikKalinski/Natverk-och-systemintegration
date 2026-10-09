#include "mqtt-subscriber.h"

#include <exception>
#include <iostream>

int main()
{
    try
    {
        MqttSubscriber subscriber;

        if (!subscriber.start())
        {
            return 1;
        }

        subscriber.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}