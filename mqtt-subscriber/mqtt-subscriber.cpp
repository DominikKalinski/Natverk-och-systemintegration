#include "mqtt-subscriber.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <nlohmann/json.hpp>

namespace
{
constexpr int API_PORT = 8080;
constexpr std::size_t MAX_REQUEST_SIZE = 4096;

void sendResponse(int socket, int status, const char* reason,
                  const std::string& body)
{
    const std::string response =
        "HTTP/1.1 " + std::to_string(status) + " " + reason + "\r\n" +
        "Content-Type: application/json\r\n" +
        "Content-Length: " + std::to_string(body.size()) + "\r\n" +
        "Connection: close\r\n\r\n" + body;

    const char* data = response.c_str();
    std::size_t remaining = response.size();

    while (remaining > 0)
    {
        const ssize_t sent = ::send(socket, data, remaining, MSG_NOSIGNAL);
        if (sent <= 0)
        {
            return;
        }

        data += sent;
        remaining -= static_cast<std::size_t>(sent);
    }
}
} // namespace

MqttSubscriber::MqttSubscriber()
{
    int result = mosquitto_lib_init();
    if (result != MOSQ_ERR_SUCCESS)
    {
        throw std::runtime_error(mosquitto_strerror(result));
    }

    _client = mosquitto_new(nullptr, true, this);
    if (_client == nullptr)
    {
        mosquitto_lib_cleanup();
        throw std::runtime_error("Failed to create MQTT client");
    }

    mosquitto_connect_callback_set(_client, onConnect);
    mosquitto_message_callback_set(_client, onMessage);
}

MqttSubscriber::~MqttSubscriber()
{
    stopHttpServer();

    if (_client != nullptr)
    {
        mosquitto_disconnect(_client);
        mosquitto_destroy(_client);
    }

    mosquitto_lib_cleanup();
}

bool MqttSubscriber::start()
{
    int result = mosquitto_tls_set(
        _client,
        "server.crt",
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );

    if (result != MOSQ_ERR_SUCCESS)
    {
        std::cerr << "TLS setup failed: "
                  << mosquitto_strerror(result) << '\n';
        return false;
    }

    result = mosquitto_connect(_client, "192.168.1.115", 8883, 60);

    if (result != MOSQ_ERR_SUCCESS)
    {
        std::cerr << "Connection failed: "
                  << mosquitto_strerror(result) << '\n';
        return false;
    }

    if (!startHttpServer())
    {
        return false;
    }

    std::cout << "HTTP API listening on port " << API_PORT << '\n'
              << "Try: curl http://localhost:" << API_PORT
              << "/api/readings/latest\n";

    return true;
}

void MqttSubscriber::run()
{
    int result = mosquitto_loop_forever(_client, -1, 1);

    if (result != MOSQ_ERR_SUCCESS)
    {
        std::cerr << "MQTT loop failed: "
                  << mosquitto_strerror(result) << '\n';
    }
}

void MqttSubscriber::onConnect(
    mosquitto* client, void*, int result)
{
    if (result != 0)
    {
        std::cerr << "Broker rejected connection: "
                  << mosquitto_connack_string(result) << '\n';
        return;
    }

    std::cout << "Connected to MQTT broker with TLS" << std::endl;

    int subscribeResult = mosquitto_subscribe(
        client, nullptr, "sensor/distance", 1
    );

    if (subscribeResult != MOSQ_ERR_SUCCESS)
    {
        std::cerr << "Subscribe failed: "
                  << mosquitto_strerror(subscribeResult) << '\n';
    }
}

void MqttSubscriber::onMessage(
    mosquitto*, void* userdata, const mosquitto_message* message)
{
    auto* subscriber = static_cast<MqttSubscriber*>(userdata);

    constexpr int MAX_PAYLOAD_SIZE = 1024;

    if (message->payload == nullptr ||
        message->payloadlen <= 0 ||
        message->payloadlen > MAX_PAYLOAD_SIZE)
    {
        std::cerr << "Rejected MQTT message: invalid payload size\n";
        return;
    }

    const std::string payload(
        static_cast<const char*>(message->payload),
        static_cast<std::size_t>(message->payloadlen)
    );

    try
    {
        const auto data = nlohmann::json::parse(payload);

        const bool valid =
            data.is_object() &&
            data.contains("sensorId") &&
            data["sensorId"].is_number_integer() &&
            data.contains("unit") &&
            data["unit"].is_string() &&
            data["unit"] == "cm" &&
            data.contains("value") &&
            data["value"].is_number() &&
            data["value"].get<double>() >= 0.0;

        if (!valid)
        {
            std::cerr << "Rejected MQTT message: invalid fields or values\n";
            return;
        }
    }
    catch (const nlohmann::json::exception& error)
    {
        std::cerr << "Rejected invalid JSON: " << error.what() << '\n';
        return;
    }

    {
        std::lock_guard<std::mutex> lock(subscriber->_latestMessageMutex);
        subscriber->_latestMessage = payload;
    }

    std::cout << "Topic: " << message->topic
              << " | Data: " << payload << '\n';
}

bool MqttSubscriber::startHttpServer()
{
    _httpServerSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (_httpServerSocket < 0)
    {
        std::cerr << "Could not create HTTP socket: "
                  << std::strerror(errno) << '\n';
        return false;
    }

    int reuseAddress = 1;
    setsockopt(
        _httpServerSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuseAddress,
        sizeof(reuseAddress)
    );

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(API_PORT);

    if (::bind(
            _httpServerSocket,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)
        ) < 0)
    {
        std::cerr << "Could not bind HTTP API to port "
                  << API_PORT << ": " << std::strerror(errno) << '\n';
        ::close(_httpServerSocket);
        _httpServerSocket = -1;
        return false;
    }

    if (::listen(_httpServerSocket, 5) < 0)
    {
        std::cerr << "Could not listen for HTTP requests: "
                  << std::strerror(errno) << '\n';
        ::close(_httpServerSocket);
        _httpServerSocket = -1;
        return false;
    }

    _httpRunning = true;
    _httpThread = std::thread(&MqttSubscriber::runHttpServer, this);
    return true;
}

void MqttSubscriber::stopHttpServer()
{
    _httpRunning = false;

    if (_httpServerSocket >= 0)
    {
        ::shutdown(_httpServerSocket, SHUT_RDWR);
    }

    if (_httpThread.joinable())
    {
        _httpThread.join();
    }

    if (_httpServerSocket >= 0)
    {
        ::close(_httpServerSocket);
        _httpServerSocket = -1;
    }
}

void MqttSubscriber::runHttpServer()
{
    while (_httpRunning)
    {
        sockaddr_in clientAddress{};
        socklen_t addressLength = sizeof(clientAddress);

        const int clientSocket = ::accept(
            _httpServerSocket,
            reinterpret_cast<sockaddr*>(&clientAddress),
            &addressLength
        );

        if (clientSocket < 0)
        {
            if (_httpRunning)
            {
                std::cerr << "HTTP accept failed: "
                          << std::strerror(errno) << '\n';
            }
            continue;
        }

        handleHttpClient(clientSocket);
        ::close(clientSocket);
    }
}

void MqttSubscriber::handleHttpClient(int clientSocket)
{
    char request[MAX_REQUEST_SIZE + 1]{};
    const ssize_t received = ::recv(
        clientSocket, request, MAX_REQUEST_SIZE, 0
    );

    if (received <= 0)
    {
        return;
    }

    request[received] = '\0';

    const std::string requestLine(
        request, strnlen(request, MAX_REQUEST_SIZE)
    );

    if (requestLine.rfind("GET /api/readings/latest ", 0) != 0)
    {
        sendResponse(
            clientSocket,
            404,
            "Not Found",
            "{\"error\":\"Endpoint not found\"}"
        );
        return;
    }

    std::string latestMessage;
    {
        std::lock_guard<std::mutex> lock(_latestMessageMutex);
        latestMessage = _latestMessage;
    }

    if (latestMessage.empty())
    {
        sendResponse(
            clientSocket,
            503,
            "Service Unavailable",
            "{\"error\":\"No sensor reading received yet\"}"
        );
        return;
    }

    // Publishern förväntas skicka giltig JSON, som skickas vidare oförändrad.
    sendResponse(clientSocket, 200, "OK", latestMessage);
}