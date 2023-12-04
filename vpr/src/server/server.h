#ifndef SERVER_H
#define SERVER_H

#include "task.h"
#include "telegrambuffer.h"

#include <sys/socket.h>
#include <netinet/in.h>

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

class Server
{
public:
    explicit Server();
    ~Server();

    void setPortNum(int portNum);

    bool isStarted() const { return m_isStarted.load(); }
    bool isStopped() const { return m_isStopped.load(); }

    void takeRecievedTasks(std::vector<Task>&);
    void addSendTasks(const std::vector<Task>&);

    void start();
    void stop();

private:
    int m_portNum = -1;
    std::mutex m_portNumMutex;

    std::string m_pathsStr;
    std::atomic<bool> m_isStarted;
    std::atomic<bool> m_isStopped;

    std::thread m_thread;
    std::mutex m_pathsMutex;

    std::vector<Task> m_receivedTasks;
    std::mutex m_receivedTasksMutex;

    std::vector<Task> m_sendTasks;
    std::mutex m_sendTasksMutex;

    TelegramBuffer m_telegramBuff;

    void startListening();
};

#endif
