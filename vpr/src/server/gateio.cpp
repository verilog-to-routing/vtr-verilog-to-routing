#include "gateio.h"

#ifndef NO_SERVER

#include "telegramparser.h"
#include "telegrambuffer.h"
#include "commconstants.h"
#include "convertutils.h"

#include "sockpp/tcp6_acceptor.h"

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

void GateIO::startListening()
{
#ifdef ENABLE_CLIENT_ALIVE_TRACKER
    std::unique_ptr<ClientAliveTracker> clientAliveTrackerPtr =
         std::make_unique<ClientAliveTracker>(std::chrono::milliseconds{5000}, std::chrono::milliseconds{20000});
#else
    std::unique_ptr<ClientAliveTracker> clientAliveTrackerPtr;
#endif

    static const std::string echoData{comm::ECHO_DATA};

    comm::TelegramBuffer telegramBuff;
    std::vector<comm::TelegramFramePtr> telegramFrames;

    sockpp::initialize();
    sockpp::tcp6_acceptor tcpServer(m_portNum);
    tcpServer.set_non_blocking(true);

    const std::size_t chunkMaxBytesNum = 2*1024*1024; // 2Mb

    if (tcpServer) {
        m_logger.queue(LogLevel::Info, "open server, port=", m_portNum);
    } else {
        m_logger.queue(LogLevel::Info, "fail to open server, port=", m_portNum);
    }

    std::optional<sockpp::tcp6_socket> clientOpt;

    /// comm event loop
    while(m_isRunning.load()) {
        bool isCommunicationProblemDetected = false;

        /// check for the client connection
        if (!clientOpt) {
            sockpp::inet6_address peer;
            sockpp::tcp6_socket client = tcpServer.accept(&peer);
            if (client) {
                m_logger.queue(LogLevel::Info, "client", client.address().to_string() , "connection accepted");
                client.set_non_blocking(true);
                clientOpt = std::move(client);

                if (clientAliveTrackerPtr) {
                    clientAliveTrackerPtr->reset();
                }
            }
        }

        if (clientOpt) {
            sockpp::tcp6_socket& client = clientOpt.value();

            /// handle sending response
            {
                std::unique_lock<std::mutex> lock(m_tasksMutex);
                
                if (!m_sendTasks.empty()) {
                    const TaskPtr& task = m_sendTasks.at(0);                
                    try {
                        std::size_t bytesToSend = std::min(chunkMaxBytesNum, task->responseBuffer().size());
                        std::size_t bytesActuallyWritten = client.write_n(task->responseBuffer().data(), bytesToSend);
                        if (bytesActuallyWritten <= task->origReponseBytesNum()) {
                            task->chopNumSentBytesFromResponseBuffer(bytesActuallyWritten);
                            m_logger.queue(LogLevel::Detail,
                                        "sent chunk:", getPrettySizeStrFromBytesNum(bytesActuallyWritten),
                                        "from", getPrettySizeStrFromBytesNum(task->origReponseBytesNum()),
                                        "left:", getPrettySizeStrFromBytesNum(task->responseBuffer().size()));
                            if (clientAliveTrackerPtr) {
                                clientAliveTrackerPtr->onClientActivity();
                            }
                        }
                    } catch(...) {
                        m_logger.queue(LogLevel::Detail, "error while writing chunk");
                        isCommunicationProblemDetected = true;
                    }

                    if (task->isResponseFullySent()) {
                        m_logger.queue(LogLevel::Info,
                                       "sent:", task->telegramHeader().info(), task->info());
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
            } // release lock

            /// handle receiving
            std::string receivedMessage;
            receivedMessage.resize(chunkMaxBytesNum);
            std::size_t bytesActuallyReceived{0};
            try {
                bytesActuallyReceived = client.read_n(&receivedMessage[0], chunkMaxBytesNum);
            } catch(...) {
                m_logger.queue(LogLevel::Error, "fail to recieving");
                isCommunicationProblemDetected = true;
            }

            if ((bytesActuallyReceived > 0) && (bytesActuallyReceived <= chunkMaxBytesNum)) {
                m_logger.queue(LogLevel::Detail, "received chunk:", getPrettySizeStrFromBytesNum(bytesActuallyReceived));
                telegramBuff.append(comm::ByteArray{receivedMessage.c_str(), bytesActuallyReceived});
                if (clientAliveTrackerPtr) {
                    clientAliveTrackerPtr->onClientActivity();
                }
            }

            /// handle telegrams
            telegramFrames.clear();
            telegramBuff.takeTelegramFrames(telegramFrames);
            for (const comm::TelegramFramePtr& telegramFrame: telegramFrames) {
                // process received data
                std::string message{telegramFrame->data.to_string()};
                bool isEchoTelegram = false;
                if (clientAliveTrackerPtr) {
                    if ((message.size() == echoData.size()) && (message == echoData)) {
                        m_logger.queue(LogLevel::Detail, "received", echoData);
                        clientAliveTrackerPtr->onClientActivity();
                        isEchoTelegram = true;
                    }
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
                        m_logger.queue(LogLevel::Info,
                                       "received:", header.info(), task->info(/*skipDuration*/true));
                        std::unique_lock<std::mutex> lock(m_tasksMutex);
                        m_receivedTasks.push_back(std::move(task));
                    } else {
                        m_logger.queue(LogLevel::Error, "broken telegram detected, fail extract options from", message);
                    }
                }
            }

            // forward telegramBuffer errors
            std::vector<std::string> telegramBufferErrors;
            telegramBuff.takeErrors(telegramBufferErrors);
            for (const std::string& error: telegramBufferErrors) {
                m_logger.queue(LogLevel::Info, error);
            }

            /// handle client alive tracker
            if (clientAliveTrackerPtr) {
                if (clientAliveTrackerPtr->isTimeToSentEcho()) {
                    comm::TelegramHeader echoHeader = comm::TelegramHeader::constructFromData(echoData);
                    std::string message = echoHeader.buffer().to_string();
                    message.append(echoData);
                    try {
                        std::size_t bytesActuallySent = client.write(message);
                        if (bytesActuallySent == message.size()) {
                            m_logger.queue(LogLevel::Detail, "sent", echoData);
                            clientAliveTrackerPtr->onEchoSent();
                        }
                    } catch(...) {
                        m_logger.queue(LogLevel::Debug, "fail to sent", echoData);
                        isCommunicationProblemDetected = true;
                    }
                }
            }

            /// handle client alive
            if (clientAliveTrackerPtr) {
                if (clientAliveTrackerPtr->isClientTimeout()) {
                    m_logger.queue(LogLevel::Error, "client didn't repond too long");
                    isCommunicationProblemDetected = true;
                }
            }

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