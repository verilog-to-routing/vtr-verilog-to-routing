#ifndef TASK_H
#define TASK_H

#include <string>
#include <sstream>
#include <iostream>

#include "commconstants.h"

namespace server {

/**
 * @brief Implements the server task.
 * 
 * This structure aids in encapsulating the client request, request result, and result status.
 * It generates a JSON data structure to be sent back to the client as a response.
 */
class Task {
public:
    Task(int jobId, int cmd, const std::string& options = ""):
        m_jobId(jobId), m_cmd(cmd), m_options(options) {}

    int jobId() const { return m_jobId; }
    int cmd() const { return m_cmd; }
    const std::string& options() const { return m_options; }
    const std::string& result() const { return m_result; }

    std::string info() const {
        std::stringstream result;
        result << "id=" << std::to_string(m_jobId) << ",";
        result << "cmd=" << std::to_string(m_cmd) << ",";
        result << "opt=" << m_options;
        return result.str();
    }

    bool isFinished() const { return m_isFinished; }
    bool hasError() const { return m_hasError; }

    void fail(const std::string& error) {
        std::cout << "task " << info() << " finished with error " << error << std::endl;
        m_result = error;
        m_isFinished = true;
        m_hasError = true;
    }
    void success(const std::string& result = "") {
        std::cout << "task " << info() << " finished with success " << result << std::endl;
        m_result = result;
        m_isFinished = true;
    }

    std::string toJsonStr() const
    {
        std::stringstream ss;
        ss << "{";

        ss << "\"" << comm::KEY_JOB_ID << "\":\"" << m_jobId << "\",";
        ss << "\"" << comm::KEY_CMD << "\":\"" << m_cmd << "\",";
        ss << "\"" << comm::KEY_OPTIONS << "\":\"" << m_options << "\",";
        ss << "\"" << comm::KEY_DATA << "\":\"" << m_result << "\",";
        int status = m_hasError ? 0 : 1;
        ss << "\"" << comm::KEY_STATUS << "\":\"" << status << "\"";

        ss << "}";

        return ss.str();
    }

private:
    int m_jobId = -1;
    int m_cmd = -1;
    std::string m_options;
    std::string m_result;
    bool m_isFinished = false;
    bool m_hasError = false;
};

} // namespace server

#endif // TASK_H
