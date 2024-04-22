#ifndef TASK_H
#define TASK_H

#ifndef NO_SERVER

#include <string>
#include <memory>
#include <chrono>

#include "telegramheader.h"

namespace server {

/**
 * @brief Implements the server task.
 * 
 * This structure aids in encapsulating the client request, request result, and result status.
 * It generates a JSON data structure to be sent back to the client as a response.
 */
class Task {
public:
    Task(int job_id, int cmd, const std::string& options = "");

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    int job_id() const { return m_job_id; }
    int cmd() const { return m_cmd; }

    void chop_num_sent_bytes_from_response_buffer(std::size_t bytesSentNum);

    bool options_match(const class std::unique_ptr<Task>& other);

    const std::string& response_buffer() const { return m_response_buffer; }

    bool is_finished() const { return m_is_finished; }
    bool has_error() const { return !m_error.empty(); }
    const std::string& error() const { return m_error; }

    std::size_t orig_reponse_bytes_num() const { return m_orig_reponse_bytes_num; }

    bool is_response_fully_sent() const { return m_is_response_fully_sent; }

    void fail(const std::string& error);
    void success(const std::string& result = "");

    std::string info(bool skip_duration = false) const;

    const comm::TelegramHeader& telegram_header() const { return m_telegram_header; }

    const std::string& options() const { return m_options; }

private:
    int m_job_id = -1;
    int m_cmd = -1;
    std::string m_options;
    std::string m_result;
    std::string m_error;
    bool m_is_finished = false;
    comm::TelegramHeader m_telegram_header;
    std::string m_response_buffer;
    std::size_t m_orig_reponse_bytes_num = 0;
    bool m_is_response_fully_sent = false;

    std::chrono::high_resolution_clock::time_point m_creation_time;

    int64_t time_ms_elapsed() const;

    void bake_response();
};
using TaskPtr = std::unique_ptr<Task>;

} // namespace server

#endif /* NO_SERVER */

#endif /* TASK_H */
