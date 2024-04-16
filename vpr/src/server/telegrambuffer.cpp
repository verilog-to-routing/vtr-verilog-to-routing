#include "telegrambuffer.h"

namespace comm {

void TelegramBuffer::append(const ByteArray& bytes)
{
    m_rawBuffer.append(bytes);
}

bool TelegramBuffer::checkTelegramHeaderPresence()
{
    std::size_t signatureStartIndex = m_rawBuffer.findSequence(TelegramHeader::SIGNATURE, TelegramHeader::SIGNATURE_SIZE);
    if (signatureStartIndex != std::size_t(-1)) {
        if (signatureStartIndex != 0) {
            // discard bytes preceding the header start position.
            m_rawBuffer.erase(m_rawBuffer.begin(), m_rawBuffer.begin() + signatureStartIndex);
        }
        return true;
    }
    return false;
}

void TelegramBuffer::takeTelegramFrames(std::vector<comm::TelegramFramePtr>& result)
{
    if (m_rawBuffer.size() <= TelegramHeader::size()) {
        return;
    }

    bool mayContainFullTelegram = true; 
    while (mayContainFullTelegram) {
        mayContainFullTelegram = false;
        // attempt to extract telegram header
        if (!m_headerOpt) {
            if (checkTelegramHeaderPresence()) {
                TelegramHeader header(m_rawBuffer);
                if (header.isValid()) {
                    m_headerOpt = std::move(header);
                }
            }
        }

        // attempt to extract telegram frame based on the telegram header
        if (m_headerOpt) {
            const TelegramHeader& header = m_headerOpt.value();
            std::size_t wholeTelegramSize = TelegramHeader::size() + header.bodyBytesNum();
            if (m_rawBuffer.size() >= wholeTelegramSize) {
                // checksum validation
                ByteArray data(m_rawBuffer.begin() + TelegramHeader::size(), m_rawBuffer.begin() + wholeTelegramSize);
                uint32_t actualCheckSum = data.calcCheckSum();
                if (actualCheckSum == header.bodyCheckSum()) {
                    // construct telegram frame if checksum matches
                    TelegramFramePtr telegramFramePtr = std::make_shared<TelegramFrame>(TelegramFrame{header, std::move(data)});
                    data.clear(); // post std::move safety step
                    result.push_back(telegramFramePtr);                    
                } else {
                    m_errors.push_back("wrong checkSums " + std::to_string(actualCheckSum) +" for " + header.info() + " , drop this chunk");
                }
                m_rawBuffer.erase(m_rawBuffer.begin(), m_rawBuffer.begin() + wholeTelegramSize);
                m_headerOpt.reset();
                mayContainFullTelegram = true;
            }
        }
    }
}

std::vector<comm::TelegramFramePtr> TelegramBuffer::takeTelegramFrames()
{
    std::vector<comm::TelegramFramePtr> result;
    takeTelegramFrames(result);
    return result;
}

void TelegramBuffer::takeErrors(std::vector<std::string>& errors)
{
    errors.clear();
    std::swap(errors, m_errors);
}

} // namespace comm
