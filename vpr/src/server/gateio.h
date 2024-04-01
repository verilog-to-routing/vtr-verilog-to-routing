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
 * 
 * Operable only with a single client. As soon as client connection is detected
 * it begins listening on the specified port number for incoming client requests,
 * collects and encapsulates them into tasks.
 * The incoming tasks are extracted and handled by the top-level logic (TaskResolver). 
 * Once the tasks are resolved by the TaskResolver, they are returned 
 * to be sent back to the client as a response.
 * 
 * @note: 
 * - The GateIO instance should be created and managed from the main thread, while its internal processing 
 *   and IO operations are performed asynchronously in a separate thread.  This separation ensures smooth IO behavior 
 *   and responsiveness of the application.
 * - Gateio is not started automatically upon creation, you have to use the 'start' method with the port number.
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

    const int LOOP_INTERVAL_MS = 100;

public:
    GateIO();
    ~GateIO();

    GateIO(const GateIO&) = delete;
    GateIO& operator=(const GateIO&) = delete;

    GateIO(GateIO&&) = delete;
    GateIO& operator=(GateIO&&) = delete;

    // Check if the port listening process is currently running
    bool isRunning() const { return m_isRunning.load(); }

    /**
     * @brief Transfers ownership of received tasks to the caller.
     * 
     * This method moves all received tasks from the internal storage to the provided vector.
     * After calling this method, the internal list of received tasks will be cleared.
     * 
     * @param tasks A reference to a vector where the received tasks will be moved.
     */
    void takeReceivedTasks(std::vector<TaskPtr>&);

    /**
     * @brief Moves tasks to the send queue.
     * 
     * This method moves the tasks to the send queue.
     * Each task is moved from the input vector to the send queue, and the input vector
     * remains empty after the operation.
     * 
     * @param tasks A reference to a vector containing the tasks to be moved to the send queue.
    */
    void moveTasksToSendQueue(std::vector<TaskPtr>&);

    /**
     * @brief Prints log messages for the GateIO.
     * 
     * @note Must be called from main thread since it's invoke std::cout.
     * Calling this method from other threads may result in unexpected behavior.
     */
    void printLogs(); 

    /**
     * @brief Starts the server on the specified port number.
     * 
     * This method starts the server to listen for incoming connections on the specified port number.
     * Once started,the server will continue running in a separate thread and will accept connection only from a single client
     * attempting to connect to the specified port.
     * 
     * @param portNum The port number on which the server will listen for incoming connection.
     */
    void start(int portNum);

    /**
     * @brief Stops the server and terminates the listening thread.
     * 
     * This method stops the server and terminates the listening thread. After calling this method,
     * the server will no longer accept incoming connections and the listening thread will be terminated.
     * 
     * @note This method should be called when the server needs to be shut down gracefully.
     */
    void stop();

private:
    int m_portNum = -1;

    std::atomic<bool> m_isRunning; // is true when started

    std::thread m_thread; // thread to execute socket IO work

    std::mutex m_tasksMutex; // we used single mutex to guard both vectors m_receivedTasks and m_sendTasks
    std::vector<TaskPtr> m_receivedTasks; // tasks from client (requests)
    std::vector<TaskPtr> m_sendTasks; // tasks to client (responses)

    TLogger m_logger;

    void startListening(); // thread worker function
};

} // namespace server

#endif // GATEIO_H

