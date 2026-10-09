#pragma once

#include <mosquitto.h>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

class MqttSubscriber
{
public:
    MqttSubscriber();
    ~MqttSubscriber();

    MqttSubscriber(const MqttSubscriber&) = delete;
    MqttSubscriber& operator=(const MqttSubscriber&) = delete;

    bool start();
    void run();

private:
    mosquitto* _client = nullptr;
    std::atomic<bool> _httpRunning{false};
    int _httpServerSocket = -1;
    std::thread _httpThread;
    std::mutex _latestMessageMutex;
    std::string _latestMessage;

    static void onConnect(mosquitto* client, void* userdata, int result);
    static void onMessage(mosquitto* client, void* userdata,
                          const mosquitto_message* message);

    bool startHttpServer();
    void stopHttpServer();
    void runHttpServer();
    void handleHttpClient(int clientSocket);
};