#ifndef GATEIO_H
#define GATEIO_H

#include "task.h"
#include "telegrambuffer.h"

#include <sys/socket.h>
#include <netinet/in.h>

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

namespace server {

/** 
 * @brief Implements the socket communication layer with the outside world. 
 *        It begins listening on the specified port number for incoming client requests,
 *        collects and encapsulates them into tasks. 
 *        The incoming tasks are extracted and handled by the top-level logic (TaskResolver). 
 *        Once the tasks are resolved by the TaskResolver, they are returned 
 *        to be sent back to the client as a response.
 * 
 * Note: 
 * - gateio is not started automatically upon creation; you have to use the 'start' method with the port number.
 * - The gateio runs in a separate thread to ensure smooth IO behavior.
 * - The socket is initialized in a non-blocking mode to function properly in a multithreaded environment.
*/
class GateIO
{
public:
    explicit GateIO();
    ~GateIO();

    bool isRunning() const { return m_isRunning.load(); }

    void takeRecievedTasks(std::vector<Task>&);
    void addSendTasks(const std::vector<Task>&);

    void start(int portNum);
    void stop();

private:
    int m_portNum = -1;

    std::atomic<bool> m_isRunning; // is true when started

    std::thread m_thread; // thread to execute socket IO work

    std::vector<Task> m_receivedTasks; // tasks from clinet (requests)
    std::mutex m_receivedTasksMutex;

    std::vector<Task> m_sendTasks; // task to client (reponses)
    std::mutex m_sendTasksMutex;

    comm::TelegramBuffer m_telegramBuff;

    void startListening(); // thread worker function
};

} // namespace server

#endif // GATEIO_H

