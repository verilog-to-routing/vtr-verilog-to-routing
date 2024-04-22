#ifndef TASK_H
#define TASK_H

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
    Task(int jobId, int cmd, const std::string& options = "");

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    int job_id() const { return m_jobId; }
    int cmd() const { return m_cmd; }

    void chopNumSentBytesFromResponseBuffer(std::size_t bytesSentNum);

    bool options_match(const class std::unique_ptr<Task>& other);

    const std::string& responseBuffer() const { return m_responseBuffer; }

    bool isFinished() const { return m_isFinished; }
    bool hasError() const { return !m_error.empty(); }
    const std::string& error() const { return m_error; }

    std::size_t origReponseBytesNum() const { return m_origReponseBytesNum; }

    bool isResponseFullySent() const { return m_isResponseFullySent; }

    void fail(const std::string& error);
    void success(const std::string& result = "");

    std::string info(bool skipDuration = false) const;

    const comm::TelegramHeader& telegramHeader() const { return m_telegramHeader; }

    const std::string& options() const { return m_options; }

private:
    int m_jobId = -1;
    int m_cmd = -1;
    std::string m_options;
    std::string m_result;
    std::string m_error;
    bool m_isFinished = false;
    comm::TelegramHeader m_telegramHeader;
    std::string m_responseBuffer;
    std::size_t m_origReponseBytesNum = 0;
    bool m_isResponseFullySent = false;

    std::chrono::high_resolution_clock::time_point m_creationTime;

    int64_t timeMsElapsed() const;

    void bakeResponse();
};
using TaskPtr = std::unique_ptr<Task>;

} // namespace server

#endif /* TASK_H */
