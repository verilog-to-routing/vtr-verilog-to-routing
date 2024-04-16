#include "task.h"

#include <sstream>

#include "telegrambuffer.h"

#include "convertutils.h"
#include "commconstants.h"
#include "zlibutils.h"

namespace server {

Task::Task(int jobId, int cmd, const std::string& options)
: m_jobId(jobId), m_cmd(cmd), m_options(options)
{
    m_creationTime = std::chrono::high_resolution_clock::now();
}

void Task::chopNumSentBytesFromResponseBuffer(std::size_t bytesSentNum) {
    if (m_responseBuffer.size() >= bytesSentNum) {
        m_responseBuffer.erase(0, bytesSentNum);
    } else {
        m_responseBuffer.clear();
    }
    if (m_responseBuffer.empty()) {
        m_isResponseFullySent = true;
    }
}

bool Task::optionsMatch(const class std::unique_ptr<Task>& other) {
    if (other->options().size() != m_options.size()) {
        return false;
    }
    return other->options() == m_options;
}

void Task::fail(const std::string& error) {
    m_isFinished = true;
    m_error = error;
    bakeResponse();
}

void Task::success(const std::string& result) {
    m_result = result;
    m_isFinished = true;
    bakeResponse();
}

std::string Task::info(bool skipDuration) const {
    std::stringstream ss;
    ss << "task["
        << "id=" << std::to_string(m_jobId)
        << ",cmd=" << std::to_string(m_cmd);
    if (!skipDuration) {
        ss << ",exists=" << getPrettyDurationStrFromMs(timeMsElapsed());
    }
    ss << "]";
    return ss.str();
}

int64_t Task::timeMsElapsed() const {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_creationTime).count();
}

void Task::bakeResponse() {
    std::stringstream ss;
    ss << "{";

    ss << "\"" << comm::KEY_JOB_ID << "\":\"" << m_jobId << "\",";
    ss << "\"" << comm::KEY_CMD << "\":\"" << m_cmd << "\",";
    ss << "\"" << comm::KEY_OPTIONS << "\":\"" << m_options << "\",";
    if (hasError()) {
        ss << "\"" << comm::KEY_DATA << "\":\"" << m_error << "\",";
    } else {
        ss << "\"" << comm::KEY_DATA << "\":\"" << m_result << "\",";
    }
    int status = hasError() ? 0 : 1;
    ss << "\"" << comm::KEY_STATUS << "\":\"" << status << "\"";

    ss << "}";

    std::optional<std::string> bodyOpt;
    uint8_t compressorId = comm::NONE_COMPRESSOR_ID;
#ifndef FORCE_DISABLE_ZLIB_TELEGRAM_COMPRESSION
    bodyOpt = tryCompress(ss.str());
    if (bodyOpt) {
        compressorId = comm::ZLIB_COMPRESSOR_ID;
    }
#endif
    if (!bodyOpt) {
        // fail to compress, use raw
        compressorId = comm::NONE_COMPRESSOR_ID;
        bodyOpt = std::move(ss.str());
    }

    std::string body = bodyOpt.value();
    m_telegramHeader = comm::TelegramHeader::constructFromData(body, compressorId);

    m_responseBuffer.append(m_telegramHeader.buffer().begin(), m_telegramHeader.buffer().end());
    m_responseBuffer.append(std::move(body));
    body.clear();
    m_origReponseBytesNum = m_responseBuffer.size();
}


} // namespace server

