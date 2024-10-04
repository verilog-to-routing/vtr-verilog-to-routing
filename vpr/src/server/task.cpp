#ifndef NO_SERVER

#include "task.h"

#include <sstream>

#include "convertutils.h"
#include "commconstants.h"
#include "zlibutils.h"

namespace server {

Task::Task(int jobId, comm::CMD cmd, const std::string& options)
: m_job_id(jobId), m_cmd(cmd), m_options(options) {
    m_creation_time = std::chrono::high_resolution_clock::now();
}

void Task::chop_num_sent_bytes_from_response_buffer(std::size_t bytes_sent_num) {
    if (m_response_buffer.size() >= bytes_sent_num) {
        m_response_buffer.erase(0, bytes_sent_num);
    } else {
        m_response_buffer.clear();
    }

    if (m_response_buffer.empty()) {
        m_is_response_fully_sent = true;
    }
}

bool Task::options_match(const std::unique_ptr<Task>& other) {
    if (other->options().size() != m_options.size()) {
        return false;
    }
    return other->options() == m_options;
}

void Task::set_fail(const std::string& error) {
    m_is_finished = true;
    m_error = error;
    bake_response();
}

void Task::set_success() {
    m_is_finished = true;
    bake_response();
}

void Task::set_success(std::string&& result) {
    m_result = std::move(result);
    m_is_finished = true;
    bake_response();
}

std::string Task::info(bool skip_duration) const {
    std::stringstream ss;
    ss << "task["
       << "id="   << std::to_string(m_job_id)
       << ",cmd=" << std::to_string(static_cast<int>(m_cmd));
    if (!skip_duration) {
        ss << ",exists=" << get_pretty_duration_str_from_ms(time_ms_elapsed());
    }
    ss << "]";
    return ss.str();
}

int64_t Task::time_ms_elapsed() const {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_creation_time).count();
}

void Task::bake_response() {
    std::stringstream ss;
    ss << "{";

    ss << "\"" << comm::KEY_JOB_ID << "\":\"" << m_job_id << "\",";
    ss << "\"" << comm::KEY_CMD << "\":\"" << static_cast<int>(m_cmd) << "\",";
    ss << "\"" << comm::KEY_OPTIONS << "\":\"" << m_options << "\",";
    if (has_error()) {
        ss << "\"" << comm::KEY_DATA << "\":\"" << m_error << "\",";
    } else {
        ss << "\"" << comm::KEY_DATA << "\":\"" << m_result << "\",";
    }
    int status = has_error() ? 0 : 1;
    ss << "\"" << comm::KEY_STATUS << "\":\"" << status << "\"";

    ss << "}";

    std::optional<std::string> body_opt;
    uint8_t compressor_id = comm::NONE_COMPRESSOR_ID;
#ifndef FORCE_DISABLE_ZLIB_TELEGRAM_COMPRESSION
    body_opt = try_compress(ss.str());
    if (body_opt) {
        compressor_id = comm::ZLIB_COMPRESSOR_ID;
    }
#endif
    if (!body_opt) {
        // fail to compress, use raw
        compressor_id = comm::NONE_COMPRESSOR_ID;
        body_opt = ss.str();
    }

    std::string body{std::move(body_opt.value())};
    m_telegram_header = comm::TelegramHeader::construct_from_body(body, compressor_id);

    m_response_buffer.append(m_telegram_header.buffer().begin(), m_telegram_header.buffer().end());
    m_response_buffer.append(body);

    m_orig_reponse_bytes_num = m_response_buffer.size();
}

} // namespace server

#endif /* NO_SERVER */
