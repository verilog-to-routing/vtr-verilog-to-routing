#ifndef GATEIO_H
#define GATEIO_H

#include "task.h"

#include <chrono>
#include <sstream>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <utility>

namespace server {

/**
 * @brief Implements the socket communication layer with the outside world.
 *        Operable only with a single client. As soon as client connection is detected
 *        it begins listening on the specified port number for incoming client requests,
 *        collects and encapsulates them into tasks. 
 *        The incoming tasks are extracted and handled by the top-level logic (TaskResolver). 
 *        Once the tasks are resolved by the TaskResolver, they are returned 
 *        to be sent back to the client as a response.
 * 
 * Note: 
 * - gateio is not started automatically upon creation; you have to use the 'start' method with the port number.
 * - The gateio runs in a separate thread to ensure smooth IO behavior.
 * - The socket is initialized in a non-blocking mode to function properly in a multithreaded environment.
*/
class GateIO
{
    class ClientAliveTracker {
    public:
        ClientAliveTracker(const std::chrono::milliseconds& echoIntervalMs, const std::chrono::milliseconds& clientTimeoutMs)
            : m_echoIntervalMs(echoIntervalMs), m_clientTimeoutMs(clientTimeoutMs) {
            reset();
        }
        ClientAliveTracker()=default;

        void onClientActivity() {
            m_lastClientActivityTime = std::chrono::high_resolution_clock::now();
        }

        void onEchoSent() {
            m_lastEchoSentTime = std::chrono::high_resolution_clock::now();
        }

        bool isTimeToSentEcho() const {
            return (durationSinceLastClientActivityMs() > m_echoIntervalMs) && (durationSinceLastEchoSentMs() > m_echoIntervalMs);
        }
        bool isClientTimeout() const  { return durationSinceLastClientActivityMs() > m_clientTimeoutMs; }

        void reset() {
            onClientActivity();
        }

    private:
        std::chrono::high_resolution_clock::time_point m_lastClientActivityTime;
        std::chrono::high_resolution_clock::time_point m_lastEchoSentTime;
        std::chrono::milliseconds m_echoIntervalMs;
        std::chrono::milliseconds m_clientTimeoutMs;

        std::chrono::milliseconds durationSinceLastClientActivityMs() const {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastClientActivityTime);
        }
        std::chrono::milliseconds durationSinceLastEchoSentMs() const {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastEchoSentTime);
        }
    };

    enum class LogLevel: int {
        Error,
        Info,
        Detail,
        Debug
    };

    class TLogger {
    public:
        TLogger() {
            m_logLevel = static_cast<int>(LogLevel::Info);
        }
        ~TLogger() {}

        template<typename... Args>
        void queue(LogLevel logLevel, Args&&... args) {
            if (static_cast<int>(logLevel) <= m_logLevel) {
                std::unique_lock<std::mutex> lock(m_logStreamMutex);
                if (logLevel == LogLevel::Error) {
                    m_logStream << "ERROR:";
                }
                ((m_logStream << ' ' << std::forward<Args>(args)), ...);
                m_logStream << "\n";
            }
        }

        void flush() {
            std::unique_lock<std::mutex> lock(m_logStreamMutex);
            if (!m_logStream.str().empty()) {
                std::cout << m_logStream.str();
                m_logStream.str("");
            }
        }

    private:
        std::stringstream m_logStream;
        std::mutex m_logStreamMutex;
        std::atomic<int> m_logLevel;
    };

public:
    explicit GateIO();
    ~GateIO();

    const int LOOP_INTERVAL_MS = 100;

    bool isRunning() const { return m_isRunning.load(); }

    void takeRecievedTasks(std::vector<TaskPtr>&);
    void moveTasksToSendQueue(std::vector<TaskPtr>&);

    void printLogs(); // called from main thread

    void start(int portNum);
    void stop();

private:
    int m_portNum = -1;

    std::atomic<bool> m_isRunning; // is true when started

    std::thread m_thread; // thread to execute socket IO work

    std::mutex m_tasksMutex;
    std::vector<TaskPtr> m_receivedTasks; // tasks from client (requests)
    std::vector<TaskPtr> m_sendTasks; // task to client (reponses)

    TLogger m_logger;

    void startListening(); // thread worker function
};

} // namespace server

#endif // GATEIO_H

