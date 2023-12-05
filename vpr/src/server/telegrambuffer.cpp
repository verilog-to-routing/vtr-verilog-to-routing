#include "telegrambuffer.h"
#include "serverconsts.h"

void TelegramBuffer::append(const ByteArray& bytes)
{
    m_rawBuffer.append(bytes);
}

std::vector<ByteArray> TelegramBuffer::takeFrames()
{
    std::vector<ByteArray> result;
    ByteArray candidate;
    for (unsigned char b: m_rawBuffer.data()) {
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
