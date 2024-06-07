#ifndef NO_SERVER

#include "telegrambuffer.h"

namespace comm {

void TelegramBuffer::append(const ByteArray& bytes) {
    m_raw_buffer.append(bytes);
}

bool TelegramBuffer::check_telegram_header_presence() {
    auto [found, signature_start_index] = m_raw_buffer.find_sequence(TelegramHeader::SIGNATURE, TelegramHeader::SIGNATURE_SIZE);
    if (found) {
        if (signature_start_index != 0) {
            // discard bytes preceding the header start position.
            m_raw_buffer.erase(m_raw_buffer.begin(), m_raw_buffer.begin() + signature_start_index);
        }
        return true;
    }
    return false;
}

void TelegramBuffer::take_telegram_frames(std::vector<comm::TelegramFramePtr>& result) {
    if (m_raw_buffer.size() <= TelegramHeader::size()) {
        return;
    }

    bool may_contain_full_telegram = true;
    while (may_contain_full_telegram) {
        may_contain_full_telegram = false;
        // attempt to extract telegram header
        if (!m_header_opt) {
            if (check_telegram_header_presence()) {
                TelegramHeader header(m_raw_buffer);
                if (header.is_valid()) {
                    m_header_opt = std::move(header);
                }
            }
        }

        // attempt to extract telegram frame based on the telegram header
        if (m_header_opt) {
            const TelegramHeader& header = m_header_opt.value();
            std::size_t expected_telegram_size = TelegramHeader::size() + header.body_bytes_num();
            if (m_raw_buffer.size() >= expected_telegram_size) {
                // checksum validation
                ByteArray body(m_raw_buffer.begin() + TelegramHeader::size(), m_raw_buffer.begin() + expected_telegram_size);
                uint32_t actual_check_sum = body.calc_check_sum();
                if (actual_check_sum == header.body_check_sum()) {
                    // construct telegram frame if checksum matches
                    TelegramFramePtr telegram_frame_ptr = std::make_shared<TelegramFrame>();
                    telegram_frame_ptr->header = header;
                    telegram_frame_ptr->body = std::move(body);
                    body.clear(); // post std::move safety step

                    result.push_back(telegram_frame_ptr);
                } else {
                    m_errors.push_back("wrong checkSums " + std::to_string(actual_check_sum) +" for " + header.info() + " , drop this chunk");
                }
                m_raw_buffer.erase(m_raw_buffer.begin(), m_raw_buffer.begin() + expected_telegram_size);
                m_header_opt.reset();
                may_contain_full_telegram = true;
            }
        }
    }
}

void TelegramBuffer::take_errors(std::vector<std::string>& errors) {
    errors.reserve(errors.size() + m_errors.size());
    std::move(std::begin(m_errors), std::end(m_errors), std::back_inserter(errors));
    m_errors.clear();
}

} // namespace comm

#endif /* NO_SERVER */
