#include "telegrambuffer.h"
#include "commconstants.h"

namespace comm {
    
void TelegramBuffer::append(const ByteArray& bytes)
{
    m_rawBuffer.append(bytes);
}

std::vector<ByteArray> TelegramBuffer::takeFrames()
{
    std::vector<ByteArray> result;
    ByteArray candidate;
    for (unsigned char b: m_rawBuffer) {
        if (b == TELEGRAM_FRAME_DELIMETER) {
            if (!candidate.empty()) {
                result.push_back(candidate);
            }
            candidate = ByteArray();
        } else {
            candidate.append(b);
        }
    }
    std::swap(m_rawBuffer, candidate);
    return result;
}

} // namespace comm
