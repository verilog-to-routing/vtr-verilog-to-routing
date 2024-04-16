#include "gateio.h"

#ifndef NO_SERVER

#include "telegramparser.h"
#include "commconstants.h"
#include "convertutils.h"

#include <iostream>

namespace server {

GateIO::GateIO()
{
    m_isRunning.store(false);
}

GateIO::~GateIO()
{
    stop();
}

void GateIO::start(int portNum)
{
    if (!m_isRunning.load()) {
        m_portNum = portNum;
        VTR_LOG("starting server");
        m_isRunning.store(true);
        m_thread = std::thread(&GateIO::startListening, this);
    }
}

void GateIO::stop()
{
    if (m_isRunning.load()) {
        m_isRunning.store(false);
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }
}

void GateIO::takeReceivedTasks(std::vector<TaskPtr>& tasks)
{
    std::unique_lock<std::mutex> lock(m_tasksMutex);
    for (TaskPtr& task: m_receivedTasks) {
        m_logger.queue(LogLevel::Debug, "move task id=", task->jobId(), "for processing");
        tasks.push_back(std::move(task));
    }
    m_receivedTasks.clear();
}

void GateIO::moveTasksToSendQueue(std::vector<TaskPtr>& tasks)
{
    std::unique_lock<std::mutex> lock(m_tasksMutex);
    for (TaskPtr& task: tasks) {
        m_logger.queue(LogLevel::Debug, "move task id=", task->jobId(), "finished", (task->hasError()? "with error": "successfully"), task->error(), "to send queue");
        m_sendTasks.push_back(std::move(task));
    }
    tasks.clear();
}

GateIO::ActivityStatus GateIO::checkClientConnection(sockpp::tcp6_acceptor& tcpServer, std::unique_ptr<ClientAliveTracker>& clientAliveTrackerPtr, std::optional<sockpp::tcp6_socket>& clientOpt) {
    ActivityStatus status = ActivityStatus::WAITING_ACTIVITY;

    sockpp::inet6_address peer;
    sockpp::tcp6_socket client = tcpServer.accept(&peer);
    if (client) {
        m_logger.queue(LogLevel::Info, "client", client.address().to_string() , "connection accepted");
        client.set_non_blocking(true);
        clientOpt = std::move(client);

        status = ActivityStatus::CLIENT_ACTIVITY;
    }

    return status;
}

GateIO::ActivityStatus GateIO::handleSendingData(sockpp::tcp6_socket& client, std::unique_ptr<ClientAliveTracker>& clientAliveTrackerPtr) {
    ActivityStatus status = ActivityStatus::WAITING_ACTIVITY;
    std::unique_lock<std::mutex> lock(m_tasksMutex);

    if (!m_sendTasks.empty()) {
        const TaskPtr& task = m_sendTasks.at(0);
        try {
            std::size_t bytesToSend = std::min(CHUNK_MAX_BYTES_NUM, task->responseBuffer().size());
            std::size_t bytesActuallyWritten = client.write_n(task->responseBuffer().data(), bytesToSend);
            if (bytesActuallyWritten <= task->origReponseBytesNum()) {
                task->chopNumSentBytesFromResponseBuffer(bytesActuallyWritten);
                m_logger.queue(LogLevel::Detail,
                            "sent chunk:", getPrettySizeStrFromBytesNum(bytesActuallyWritten),
                            "from", getPrettySizeStrFromBytesNum(task->origReponseBytesNum()),
                            "left:", getPrettySizeStrFromBytesNum(task->responseBuffer().size()));
                status = ActivityStatus::CLIENT_ACTIVITY;
            }
        } catch(...) {
            m_logger.queue(LogLevel::Detail, "error while writing chunk");
            status = ActivityStatus::COMMUNICATION_PROBLEM;
        }

        if (task->isResponseFullySent()) {
            m_logger.queue(LogLevel::Info, "sent:", task->telegramHeader().info(), task->info());
        }
    }

    // remove reported tasks
    std::size_t tasksBeforeRemoving = m_sendTasks.size();

    auto partitionIter = std::partition(m_sendTasks.begin(), m_sendTasks.end(),
                                        [](const TaskPtr& task) { return !task->isResponseFullySent(); });
    m_sendTasks.erase(partitionIter, m_sendTasks.end());
    bool removingTookPlace = tasksBeforeRemoving != m_sendTasks.size();
    if (!m_sendTasks.empty() && removingTookPlace) {
        m_logger.queue(LogLevel::Detail, "left tasks num to send ", m_sendTasks.size());
    }

    return status;
}

GateIO::ActivityStatus GateIO::handleReceivingData(sockpp::tcp6_socket& client, comm::TelegramBuffer& telegramBuff, std::string& receivedMessage) {
    ActivityStatus status = ActivityStatus::WAITING_ACTIVITY;
    if (receivedMessage.size() != CHUNK_MAX_BYTES_NUM) {
        receivedMessage.resize(CHUNK_MAX_BYTES_NUM);
    }
    std::size_t bytesActuallyReceived{0};
    try {
        bytesActuallyReceived = client.read_n(&receivedMessage[0], CHUNK_MAX_BYTES_NUM);
    } catch(...) {
        m_logger.queue(LogLevel::Error, "fail to receiving");
        status = ActivityStatus::COMMUNICATION_PROBLEM;
    }

    if ((bytesActuallyReceived > 0) && (bytesActuallyReceived <= CHUNK_MAX_BYTES_NUM)) {
        m_logger.queue(LogLevel::Detail, "received chunk:", getPrettySizeStrFromBytesNum(bytesActuallyReceived));
        telegramBuff.append(comm::ByteArray{receivedMessage.c_str(), bytesActuallyReceived});
        status = ActivityStatus::CLIENT_ACTIVITY;
    }

    return status;
}

GateIO::ActivityStatus GateIO::handleTelegrams(std::vector<comm::TelegramFramePtr>& telegramFrames, comm::TelegramBuffer& telegramBuff) {
    ActivityStatus status = ActivityStatus::WAITING_ACTIVITY;
    telegramFrames.clear();
    telegramBuff.takeTelegramFrames(telegramFrames);
    for (const comm::TelegramFramePtr& telegramFrame: telegramFrames) {
        // process received data
        std::string message{telegramFrame->data.to_string()};
        bool isEchoTelegram = false;
        if ((message.size() == comm::ECHO_DATA.size()) && (message == comm::ECHO_DATA)) {
            m_logger.queue(LogLevel::Detail, "received", comm::ECHO_DATA);
            isEchoTelegram = true;
            status = ActivityStatus::CLIENT_ACTIVITY;
        }

        if (!isEchoTelegram) {
            m_logger.queue(LogLevel::Detail, "received composed", getPrettySizeStrFromBytesNum(message.size()), ":", getTruncatedMiddleStr(message));
            std::optional<int> jobIdOpt = comm::TelegramParser::tryExtractFieldJobId(message);
            std::optional<int> cmdOpt = comm::TelegramParser::tryExtractFieldCmd(message);
            std::optional<std::string> optionsOpt;
            comm::TelegramParser::tryExtractFieldOptions(message, optionsOpt);
            if (jobIdOpt && cmdOpt && optionsOpt) {
                TaskPtr task = std::make_unique<Task>(jobIdOpt.value(), cmdOpt.value(), optionsOpt.value());
                const comm::TelegramHeader& header = telegramFrame->header;
                m_logger.queue(LogLevel::Info, "received:", header.info(), task->info(/*skipDuration*/true));
                std::unique_lock<std::mutex> lock(m_tasksMutex);
                m_receivedTasks.push_back(std::move(task));
            } else {
                m_logger.queue(LogLevel::Error, "broken telegram detected, fail extract options from", message);
            }
        }
    }

    return status;
}

GateIO::ActivityStatus GateIO::handleClientAliveTracker(sockpp::tcp6_socket& client, std::unique_ptr<ClientAliveTracker>& clientAliveTrackerPtr)
{
    ActivityStatus status = ActivityStatus::WAITING_ACTIVITY;
    if (clientAliveTrackerPtr) {
        /// handle sending echo to client
        if (clientAliveTrackerPtr->isTimeToSentEcho()) {
            comm::TelegramHeader echoHeader = comm::TelegramHeader::constructFromData(comm::ECHO_DATA);
            std::string message = echoHeader.buffer().to_string();
            message.append(comm::ECHO_DATA);
            try {
                std::size_t bytesActuallySent = client.write(message);
                if (bytesActuallySent == message.size()) {
                    m_logger.queue(LogLevel::Detail, "sent", comm::ECHO_DATA);
                    clientAliveTrackerPtr->onEchoSent();
                }
            } catch(...) {
                m_logger.queue(LogLevel::Debug, "fail to sent", comm::ECHO_DATA);
                status = ActivityStatus::COMMUNICATION_PROBLEM;
            }
        }

        /// handle client timeout
        if (clientAliveTrackerPtr->isClientTimeout()) {
            m_logger.queue(LogLevel::Error, "client didn't respond too long");
            status = ActivityStatus::COMMUNICATION_PROBLEM;
        }
    }

    return status;
}

void GateIO::handleActivityStatus(ActivityStatus status, std::unique_ptr<ClientAliveTracker>& clientAliveTrackerPtr, bool& isCommunicationProblemDetected)
{
    if (status == ActivityStatus::CLIENT_ACTIVITY) {
        if (clientAliveTrackerPtr) {
            clientAliveTrackerPtr->onClientActivity();
        }
    } else if (status == ActivityStatus::COMMUNICATION_PROBLEM) {
        isCommunicationProblemDetected = true;
    }
}

void GateIO::startListening()
{
#ifdef ENABLE_CLIENT_ALIVE_TRACKER
    std::unique_ptr<ClientAliveTracker> clientAliveTrackerPtr =
         std::make_unique<ClientAliveTracker>(std::chrono::milliseconds{5000}, std::chrono::milliseconds{20000});
#else
    std::unique_ptr<ClientAliveTracker> clientAliveTrackerPtr;
#endif

    comm::TelegramBuffer telegramBuff;
    std::vector<comm::TelegramFramePtr> telegramFrames;

    sockpp::initialize();
    sockpp::tcp6_acceptor tcpServer(m_portNum);
    tcpServer.set_non_blocking(true);

    if (tcpServer) {
        m_logger.queue(LogLevel::Info, "open server, port=", m_portNum);
    } else {
        m_logger.queue(LogLevel::Info, "fail to open server, port=", m_portNum);
    }

    std::optional<sockpp::tcp6_socket> clientOpt;

    std::string receivedMessage;

    /// comm event loop
    while(m_isRunning.load()) {
        bool isCommunicationProblemDetected = false;

        if (!clientOpt) {
            ActivityStatus status = checkClientConnection(tcpServer, clientAliveTrackerPtr, clientOpt);
            if (status == ActivityStatus::CLIENT_ACTIVITY) {
                if (clientAliveTrackerPtr) {
                    clientAliveTrackerPtr->reset();
                }
            }
        }

        if (clientOpt) {
            sockpp::tcp6_socket& client = clientOpt.value(); // shortcut

            /// handle sending
            ActivityStatus status = handleSendingData(client, clientAliveTrackerPtr);
            handleActivityStatus(status, clientAliveTrackerPtr, isCommunicationProblemDetected);

            /// handle receiving
            status = handleReceivingData(client, telegramBuff, receivedMessage);
            handleActivityStatus(status, clientAliveTrackerPtr, isCommunicationProblemDetected);

            /// handle telegrams
            status = handleTelegrams(telegramFrames, telegramBuff);
            handleActivityStatus(status, clientAliveTrackerPtr, isCommunicationProblemDetected);

            // forward telegramBuffer errors
            std::vector<std::string> telegramBufferErrors;
            telegramBuff.takeErrors(telegramBufferErrors);
            for (const std::string& error: telegramBufferErrors) {
                m_logger.queue(LogLevel::Info, error);
            }

            /// handle client alive tracker
            status = handleClientAliveTracker(client, clientAliveTrackerPtr);
            handleActivityStatus(status, clientAliveTrackerPtr, isCommunicationProblemDetected);

            /// handle communication problem
            if (isCommunicationProblemDetected) {
                clientOpt = std::nullopt;
                if (!telegramBuff.empty()) {
                    m_logger.queue(LogLevel::Debug, "clear telegramBuff");
                    telegramBuff.clear();
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds{LOOP_INTERVAL_MS});
    }
}

void GateIO::printLogs()
{
    m_logger.flush();
}

} // namespace server

#endif /* NO_SERVER */