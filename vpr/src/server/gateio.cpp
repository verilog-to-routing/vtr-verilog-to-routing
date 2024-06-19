#ifndef NO_SERVER

#include "gateio.h"

#include "telegramparser.h"
#include "commconstants.h"
#include "convertutils.h"

namespace server {

GateIO::GateIO() {
    m_is_running.store(false);
}

GateIO::~GateIO() {
    stop();
}

void GateIO::start(int port_num) {
    if (!m_is_running.load()) {
        m_port_num = port_num;
        VTR_LOG("starting server");
        m_is_running.store(true);
        m_thread = std::thread(&GateIO::start_listening, this);
    }
}

void GateIO::stop() {
    if (m_is_running.load()) {
        m_is_running.store(false);
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }
}

void GateIO::take_received_tasks(std::vector<TaskPtr>& tasks) {
    std::unique_lock<std::mutex> lock(m_tasks_mutex);
    for (TaskPtr& task: m_received_tasks) {
        m_logger.queue(LogLevel::Debug, "move task id=", task->job_id(), "for processing");
        tasks.push_back(std::move(task));
    }
    m_received_tasks.clear();
}

void GateIO::move_tasks_to_send_queue(std::vector<TaskPtr>& tasks) {
    std::unique_lock<std::mutex> lock(m_tasks_mutex);
    for (TaskPtr& task: tasks) {
        m_logger.queue(LogLevel::Debug, "move task id=", task->job_id(), "finished", (task->has_error() ? "with error" : "successfully"), task->error(), "to send queue");
        m_send_tasks.push_back(std::move(task));
    }
    tasks.clear();
}

GateIO::ActivityStatus GateIO::check_client_connection(sockpp::tcp6_acceptor& tcp_server, std::optional<sockpp::tcp6_socket>& client_opt) {
    ActivityStatus status = ActivityStatus::WAITING_ACTIVITY;

    sockpp::inet6_address peer;
    sockpp::tcp6_socket client = tcp_server.accept(&peer);
    if (client) {
        m_logger.queue(LogLevel::Info, "client", client.address().to_string() , "connection accepted");
        client.set_non_blocking(true);
        client_opt = std::move(client);

        status = ActivityStatus::CLIENT_ACTIVITY;
    }

    return status;
}

GateIO::ActivityStatus GateIO::handle_sending_data(sockpp::tcp6_socket& client) {
    ActivityStatus status = ActivityStatus::WAITING_ACTIVITY;
    std::unique_lock<std::mutex> lock(m_tasks_mutex);

    if (!m_send_tasks.empty()) {
        const TaskPtr& task = m_send_tasks.at(0);
        try {
            std::size_t bytes_to_send = std::min(CHUNK_MAX_BYTES_NUM, task->response_buffer().size());
            std::size_t bytes_sent = client.write_n(task->response_buffer().data(), bytes_to_send);
            if (bytes_sent <= task->orig_reponse_bytes_num()) {
                task->chop_num_sent_bytes_from_response_buffer(bytes_sent);
                m_logger.queue(LogLevel::Detail,
                            "sent chunk:", get_pretty_size_str_from_bytes_num(bytes_sent),
                            "from", get_pretty_size_str_from_bytes_num(task->orig_reponse_bytes_num()),
                            "left:", get_pretty_size_str_from_bytes_num(task->response_buffer().size()));
                status = ActivityStatus::CLIENT_ACTIVITY;
            }
        } catch(...) {
            m_logger.queue(LogLevel::Detail, "error while writing chunk");
            status = ActivityStatus::COMMUNICATION_PROBLEM;
        }

        if (task->is_response_fully_sent()) {
            m_logger.queue(LogLevel::Info, "sent:", task->telegram_header().info(), task->info());
        }
    }

    // remove reported tasks
    std::size_t tasks_num_before_removing = m_send_tasks.size();

    auto partition_iter = std::partition(m_send_tasks.begin(), m_send_tasks.end(),
                                        [](const TaskPtr& task) { return !task->is_response_fully_sent(); });
    m_send_tasks.erase(partition_iter, m_send_tasks.end());
    bool is_removing_took_place = tasks_num_before_removing != m_send_tasks.size();
    if (!m_send_tasks.empty() && is_removing_took_place) {
        m_logger.queue(LogLevel::Detail, "left tasks num to send ", m_send_tasks.size());
    }

    return status;
}

GateIO::ActivityStatus GateIO::handle_receiving_data(sockpp::tcp6_socket& client, comm::TelegramBuffer& telegram_buff, std::string& received_message) {
    ActivityStatus status = ActivityStatus::WAITING_ACTIVITY;
    std::size_t bytes_actually_received{0};
    try {
        bytes_actually_received = client.read_n(&received_message[0], CHUNK_MAX_BYTES_NUM);
    } catch(...) {
        m_logger.queue(LogLevel::Error, "fail to receiving");
        status = ActivityStatus::COMMUNICATION_PROBLEM;
    }

    if ((bytes_actually_received > 0) && (bytes_actually_received <= CHUNK_MAX_BYTES_NUM)) {
        m_logger.queue(LogLevel::Detail, "received chunk:", get_pretty_size_str_from_bytes_num(bytes_actually_received));
        telegram_buff.append(comm::ByteArray{received_message.c_str(), bytes_actually_received});
        status = ActivityStatus::CLIENT_ACTIVITY;
    }

    return status;
}

GateIO::ActivityStatus GateIO::handle_telegrams(std::vector<comm::TelegramFramePtr>& telegram_frames, comm::TelegramBuffer& telegram_buff) {
    ActivityStatus status = ActivityStatus::WAITING_ACTIVITY;
    telegram_frames.clear();
    telegram_buff.take_telegram_frames(telegram_frames);
    for (const comm::TelegramFramePtr& telegram_frame: telegram_frames) {
        // process received data
        std::string message{telegram_frame->body};
        bool is_echo_telegram = false;
        if ((message.size() == comm::ECHO_TELEGRAM_BODY.size()) && (message == comm::ECHO_TELEGRAM_BODY)) {
            m_logger.queue(LogLevel::Detail, "received", comm::ECHO_TELEGRAM_BODY);
            is_echo_telegram = true;
            status = ActivityStatus::CLIENT_ACTIVITY;
        }

        if (!is_echo_telegram) {
            m_logger.queue(LogLevel::Detail, "received composed", get_pretty_size_str_from_bytes_num(message.size()), ":", get_truncated_middle_str(message));
            std::optional<int> job_id_opt = comm::TelegramParser::try_extract_field_job_id(message);
            std::optional<int> cmd_opt = comm::TelegramParser::try_extract_field_cmd(message);
            std::optional<std::string> options_opt = comm::TelegramParser::try_extract_field_options(message);
            if (job_id_opt && cmd_opt && options_opt) {
                TaskPtr task = std::make_unique<Task>(job_id_opt.value(), static_cast<comm::CMD>(cmd_opt.value()), options_opt.value());
                const comm::TelegramHeader& header = telegram_frame->header;
                m_logger.queue(LogLevel::Info, "received:", header.info(), task->info(/*skipDuration*/true));
                std::unique_lock<std::mutex> lock(m_tasks_mutex);
                m_received_tasks.push_back(std::move(task));
            } else {
                m_logger.queue(LogLevel::Error, "broken telegram detected, fail extract options from", message);
            }
        }
    }

    return status;
}

GateIO::ActivityStatus GateIO::handle_client_alive_tracker(sockpp::tcp6_socket& client, std::unique_ptr<ClientAliveTracker>& client_alive_tracker_ptr) {
    ActivityStatus status = ActivityStatus::WAITING_ACTIVITY;
    if (client_alive_tracker_ptr) {
        /// handle sending echo to client
        if (client_alive_tracker_ptr->is_time_to_sent_echo()) {
            comm::TelegramHeader echo_header = comm::TelegramHeader::construct_from_body(comm::ECHO_TELEGRAM_BODY);
            std::string message{echo_header.buffer()};
            message.append(comm::ECHO_TELEGRAM_BODY);
            try {
                std::size_t bytes_sent = client.write(message);
                if (bytes_sent == message.size()) {
                    m_logger.queue(LogLevel::Detail, "sent", comm::ECHO_TELEGRAM_BODY);
                    client_alive_tracker_ptr->on_echo_sent();
                }
            } catch(...) {
                m_logger.queue(LogLevel::Debug, "fail to sent", comm::ECHO_TELEGRAM_BODY);
                status = ActivityStatus::COMMUNICATION_PROBLEM;
            }
        }

        /// handle client timeout
        if (client_alive_tracker_ptr->is_client_timeout()) {
            m_logger.queue(LogLevel::Error, "client didn't respond too long");
            status = ActivityStatus::COMMUNICATION_PROBLEM;
        }
    }

    return status;
}

void GateIO::handle_activity_status(ActivityStatus status, std::unique_ptr<ClientAliveTracker>& client_alive_tracker_ptr, bool& is_communication_problem_detected) {
    if (status == ActivityStatus::CLIENT_ACTIVITY) {
        if (client_alive_tracker_ptr) {
            client_alive_tracker_ptr->on_client_activity();
        }
    } else if (status == ActivityStatus::COMMUNICATION_PROBLEM) {
        is_communication_problem_detected = true;
    }
}

void GateIO::start_listening() {
#ifdef ENABLE_CLIENT_ALIVE_TRACKER
    std::unique_ptr<ClientAliveTracker> client_alive_tracker_ptr =
         std::make_unique<ClientAliveTracker>(std::chrono::milliseconds{5000}, std::chrono::milliseconds{20000});
#else
    std::unique_ptr<ClientAliveTracker> client_alive_tracker_ptr;
#endif

    comm::TelegramBuffer telegram_buff;
    std::vector<comm::TelegramFramePtr> telegram_frames;

    sockpp::initialize();
    sockpp::tcp6_acceptor tcp_server(m_port_num);
    tcp_server.set_non_blocking(true);

    if (tcp_server) {
        m_logger.queue(LogLevel::Info, "open server, port=", m_port_num);
    } else {
        m_logger.queue(LogLevel::Info, "fail to open server, port=", m_port_num);
    }

    std::optional<sockpp::tcp6_socket> client_opt;

    std::string received_message;
    received_message.resize(CHUNK_MAX_BYTES_NUM);

    /// comm event loop
    while(m_is_running.load()) {
        bool is_communication_problem_detected = false;

        if (!client_opt) {
            ActivityStatus status = check_client_connection(tcp_server, client_opt);
            if (status == ActivityStatus::CLIENT_ACTIVITY) {
                if (client_alive_tracker_ptr) {
                    client_alive_tracker_ptr->reset();
                }
            }
        }

        if (client_opt) {
            sockpp::tcp6_socket& client = client_opt.value(); // shortcut

            /// handle sending
            ActivityStatus status = handle_sending_data(client);
            handle_activity_status(status, client_alive_tracker_ptr, is_communication_problem_detected);

            /// handle receiving
            status = handle_receiving_data(client, telegram_buff, received_message);
            handle_activity_status(status, client_alive_tracker_ptr, is_communication_problem_detected);

            /// handle telegrams
            status = handle_telegrams(telegram_frames, telegram_buff);
            handle_activity_status(status, client_alive_tracker_ptr, is_communication_problem_detected);

            // forward telegramBuffer errors
            std::vector<std::string> telegram_buffer_errors;
            telegram_buff.take_errors(telegram_buffer_errors);
            for (const std::string& error: telegram_buffer_errors) {
                m_logger.queue(LogLevel::Info, error);
            }

            /// handle client alive tracker
            status = handle_client_alive_tracker(client, client_alive_tracker_ptr);
            handle_activity_status(status, client_alive_tracker_ptr, is_communication_problem_detected);

            /// handle communication problem
            if (is_communication_problem_detected) {
                client_opt = std::nullopt;
                if (!telegram_buff.empty()) {
                    m_logger.queue(LogLevel::Debug, "clear telegramBuff");
                    telegram_buff.clear();
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds{LOOP_INTERVAL_MS});
    }
}

void GateIO::print_logs() {
    m_logger.flush();
}

} // namespace server

#endif /* NO_SERVER */
