#ifndef GATEIO_H
#define GATEIO_H

#ifndef NO_SERVER

#include "task.h"
#include "telegrambuffer.h"

#include "vtr_log.h"

#include <chrono>
#include <sstream>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <utility>
#include <optional>

#include "sockpp/tcp6_acceptor.h"

namespace server {

/**
 * @brief Implements the socket communication layer with the outside world.
 * 
 * Operable only with a single client. As soon as client connection is detected
 * it begins listening on the specified port number for incoming client requests,
 * collects and encapsulates them into tasks (see @ref Task).
 * The incoming tasks are extracted and handled by the top-level logic @ref TaskResolver in the main thread.
 * Once the tasks are resolved by the @ref TaskResolver, they are returned to be sent back to the client app as a response.
 * Moving @ref Task across threads happens in @ref server::update.
 * 
 * @note
 * - The GateIO instance should be created and managed from the main thread, while its internal processing 
 *   and IO operations are performed asynchronously in a separate thread.  This separation ensures smooth IO behavior 
 *   and responsiveness of the application.
 * - GateIO is not started automatically upon creation, you have to use the 'start' method with the port number.
 * - The socket is initialized in a non-blocking mode to function properly in a multithreaded environment.
*/
class GateIO
{
    enum class ActivityStatus : int {
        WAITING_ACTIVITY,
        CLIENT_ACTIVITY,
        COMMUNICATION_PROBLEM
    };

    const std::size_t CHUNK_MAX_BYTES_NUM = 2*1024*1024; // 2Mb

    /**
     * @brief Helper class aimed to help detecting a client offline.
     *
     * The ClientAliveTracker is pinged each time there is some activity from the client side.
     * When the client doesn't show activity for a certain amount of time, the ClientAliveTracker generates 
     * an event for sending an ECHO telegram to the client.
     * If, after sending the ECHO telegram, the client does not respond with an ECHO, it means the client is absent, 
     * and it's time to start accepting new client connections in GateIO.
     */
    class ClientAliveTracker {
    public:
        ClientAliveTracker(const std::chrono::milliseconds& echoIntervalMs, const std::chrono::milliseconds& clientTimeoutMs)
            : m_echo_interval_ms(echoIntervalMs), m_client_timeout_ms(clientTimeoutMs) {
            reset();
        }
        ClientAliveTracker()=default;

        void on_client_activity() {
            m_last_client_activity_time = std::chrono::high_resolution_clock::now();
        }

        void on_echo_sent() {
            m_last_echo_sent_time = std::chrono::high_resolution_clock::now();
        }

        bool is_time_to_sent_echo() const {
            return (duration_since_last_client_activity_ms() > m_echo_interval_ms) && (durationSinceLastEchoSentMs() > m_echo_interval_ms);
        }
        bool is_client_timeout() const  { return duration_since_last_client_activity_ms() > m_client_timeout_ms; }

        void reset() {
            on_client_activity();
        }

    private:
        std::chrono::high_resolution_clock::time_point m_last_client_activity_time;
        std::chrono::high_resolution_clock::time_point m_last_echo_sent_time;
        std::chrono::milliseconds m_echo_interval_ms;
        std::chrono::milliseconds m_client_timeout_ms;

        std::chrono::milliseconds duration_since_last_client_activity_ms() const {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_client_activity_time);
        }
        std::chrono::milliseconds durationSinceLastEchoSentMs() const {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_echo_sent_time);
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
            m_log_level = static_cast<int>(LogLevel::Info);
        }
        ~TLogger() {}

        template<typename... Args>
        void queue(LogLevel logLevel, Args&&... args) {
            if (static_cast<int>(logLevel) <= m_log_level) {
                std::unique_lock<std::mutex> lock(m_log_stream_mutex);
                if (logLevel == LogLevel::Error) {
                    m_log_stream << "ERROR:";
                }
                ((m_log_stream << ' ' << std::forward<Args>(args)), ...);
                m_log_stream << "\n";
            }
        }

        void flush() {
            std::unique_lock<std::mutex> lock(m_log_stream_mutex);
            if (!m_log_stream.str().empty()) {
                VTR_LOG(m_log_stream.str().c_str());
                m_log_stream.str("");
            }
        }

    private:
        std::stringstream m_log_stream;
        std::mutex m_log_stream_mutex;
        std::atomic<int> m_log_level;
    };

    const int LOOP_INTERVAL_MS = 100;

public:
    /**
     * @brief Default constructor for GateIO.
     */
    GateIO();
    ~GateIO();

    GateIO(const GateIO&) = delete;
    GateIO& operator=(const GateIO&) = delete;

    GateIO(GateIO&&) = delete;
    GateIO& operator=(GateIO&&) = delete;

    /**
    * @brief Returns a bool indicating whether or not the port listening process is currently running.
    *
    * @return True if the port listening process is running, false otherwise.
    */
    bool is_running() const { return m_is_running.load(); }

    /**
     * @brief Transfers ownership of received tasks to the caller.
     * 
     * This method moves all received tasks from the internal storage to the provided vector.
     * After calling this method, the internal list of received tasks will be cleared.
     * 
     * @param tasks A reference to a vector where the received tasks will be moved.
     */
    void take_received_tasks(std::vector<TaskPtr>& tasks);

    /**
     * @brief Moves tasks to the send queue.
     * 
     * This method moves the tasks to the send queue.
     * Each task is moved from the input vector to the send queue, and the input vector
     * remains empty after the operation.
     * 
     * @param tasks A reference to a vector containing the tasks to be moved to the send queue.
    */
    void move_tasks_to_send_queue(std::vector<TaskPtr>& tasks);

    /**
     * @brief Prints log messages for the GateIO.
     * 
     * @note Must be called from the main thread since it's invoke std::cout.
     * Calling this method from other threads may result in unexpected behavior.
     */
    void print_logs(); 

    /**
     * @brief Starts the server on the specified port number.
     * 
     * This method starts the server to listen for incoming connections on the specified port number.
     * Once started,the server will continue running in a separate thread and will accept connection only from a single client
     * attempting to connect to the specified port.
     * 
     * @param port_num The port number on which the server will listen for incoming connection.
     */
    void start(int port_num);

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
    int m_port_num = -1;

    std::atomic<bool> m_is_running; // is true when started

    std::thread m_thread; // thread to execute socket IO work

    std::mutex m_tasks_mutex; // we used single mutex to guard both vectors m_received_tasks and m_sendTasks
    std::vector<TaskPtr> m_received_tasks; // tasks from client (requests)
    std::vector<TaskPtr> m_send_tasks; // tasks to client (responses)

    TLogger m_logger;

    void start_listening(); // thread worker function

    /// helper functions to be executed inside startListening
    ActivityStatus check_client_connection(sockpp::tcp6_acceptor& tcp_server, std::optional<sockpp::tcp6_socket>& client_opt);
    ActivityStatus handle_sending_data(sockpp::tcp6_socket& client);
    ActivityStatus handle_receiving_data(sockpp::tcp6_socket& client, comm::TelegramBuffer& telegram_buff, std::string& received_message);
    ActivityStatus handle_telegrams(std::vector<comm::TelegramFramePtr>& telegram_frames, comm::TelegramBuffer& telegram_buff);
    ActivityStatus handle_client_alive_tracker(sockpp::tcp6_socket& client, std::unique_ptr<ClientAliveTracker>& client_alive_tracker_ptr);
    void handle_activity_status(ActivityStatus status, std::unique_ptr<ClientAliveTracker>& client_alive_tracker_ptr, bool& is_communication_problem_detected);
    ///
};

} // namespace server

#endif /* NO_SERVER */

#endif /* GATEIO_H */

