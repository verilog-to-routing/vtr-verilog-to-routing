#include "server.h"
#include "telegramparser.h"

#include <regex>
#include <sstream>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

Server::Server()
{
    m_isStarted.store(false);
    m_isStopped.store(false);
}

Server::~Server()
{
    std::cout << "~~~ th=" << std::this_thread::get_id() << " ~Server()" << std::endl;
    stop();
}

void Server::setPortNum(int portNum)
{
    std::unique_lock<std::mutex> lock(m_portNumMutex);
    m_portNum = portNum;
}

void Server::start()
{
    if (!m_isStarted.load()) {
        std::cout << "~~~ th=" << std::this_thread::get_id() << " starting server" << std::endl;
        m_isStarted.store(true);
        m_thread = std::thread(&Server::startListening, this);
    }
}

void Server::stop()
{
    if (!m_isStopped.load()) {
        m_isStopped.store(true);
        std::cout << "~~~ th=" << std::this_thread::get_id() << " stopping server, is stopped=" << m_isStopped.load() << std::endl;
        if (m_thread.joinable()) {
            std::cout << "~~~ th=" << std::this_thread::get_id() << " join thread START" << std::endl;
            m_thread.join();
            std::cout << "~~~ th=" << std::this_thread::get_id() << " join thread FINISHED" << std::endl;
        }
    }                
}

void Server::takeRecievedTasks(std::vector<Task>& tasks)
{
    tasks.clear();
    std::unique_lock<std::mutex> lock(m_receivedTasksMutex);
    if (m_receivedTasks.size() > 0) {
        std::cout << "take " << m_receivedTasks.size() << " num of received tasks"<< std::endl;
    }
    std::swap(tasks, m_receivedTasks);
}

void Server::addSendTasks(const std::vector<Task>& tasks)
{
    std::unique_lock<std::mutex> lock(m_sendTasksMutex);
    for (const Task& task: tasks) {
        std::cout << "addSendTasks id=" << task.jobId() << std::endl;
        m_sendTasks.push_back(task);
    }
}

void Server::startListening()
{
    int server_socket;
    int client_socket = -1;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(m_portNum);

    // Bind the socket to the network address and port
    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    } else {
        std::cout << "~~~ th=" << std::this_thread::get_id() << " start listening port: " << m_portNum << std::endl;
    }

    // Put the server socket in a passive mode, where it waits for the client to approach the server to make a connection
    if (listen(server_socket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Set the server socket to non-blocking mode
    if (fcntl(server_socket, F_SETFL, fcntl(server_socket, F_GETFL, 0) | O_NONBLOCK) < 0) {
        perror("Error setting socket to non-blocking");
        exit(EXIT_FAILURE);
    }

    bool connectionProblemDetected = false;

    // Event loop
    while(!m_isStopped.load()) {
        if (connectionProblemDetected || client_socket < 0) {
            int flags = fcntl(client_socket, F_GETFL, 0);
            fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);
            client_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (client_socket > 0) {
                std::cout << "accept client" << std::endl;
            }
        }

        if (client_socket >= 0) {
            connectionProblemDetected = false;
            // Handle sending
            {
                std::unique_lock<std::mutex> lock(m_sendTasksMutex);
                for (const Task& task: m_sendTasks) {
                    std::string response = task.toJsonStr();
                    response += static_cast<unsigned char>(TELEGRAM_FRAME_DELIMETER);
                    std::cout << "sending" << response << "to client" << std::endl;
                    send(client_socket, response.c_str(), response.length(), 0);
                }
                m_sendTasks.clear();
            }

            ////////////////////
            // Handle receiving
            fd_set readfds;
            struct timeval tv;

            // Set a timeout
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            // Clear the set ahead of time
            FD_ZERO(&readfds);

            // Add our descriptor to the set
            FD_SET(client_socket, &readfds);

            // Wait until data is available or timeout
            int selectResult = select(client_socket + 1, &readfds, NULL, NULL, &tv);

            if (selectResult == -1) {
                std::cerr << "Error in select()\n";
            } else if (selectResult == 0) {
                std::cout << "Timeout occurred! No data available.\n";
            } else {
                if (FD_ISSET(client_socket, &readfds)) {
                    // Data is available; proceed with recv
                    char data[1024];
                    ssize_t bytes_received = recv(client_socket, data, sizeof(data), 0);
                    if (bytes_received > 0) {
                        m_telegramBuff.append(ByteArray{data});
                    } else if (bytes_received == 0) {
                        std::cout << "Connection closed\n";
                        connectionProblemDetected = true;
                    } else {
                        std::cerr << "Error in recv()\n";
                        connectionProblemDetected = true;
                    }
                }
            }

            auto frames = m_telegramBuff.takeFrames();
            for (const ByteArray& frame: frames) {
                // Process received data
                std::string message{frame.to_string()};
                std::cout << "Received: " << message << std::endl;

                int jobId = telegramparser::extractJobId(message);
                int cmd = telegramparser::extractCmd(message);
                std::string options = telegramparser::extractOptions(message);
                std::cout << "server: jobId=" << jobId << ", cmd=" << cmd << ", options=" << options << std::endl;
                if ((jobId != -1) && (cmd != -1)) {
                    std::unique_lock<std::mutex> lock(m_receivedTasksMutex);
                    m_receivedTasks.emplace_back(jobId, cmd, options);
                }
            }

            if (connectionProblemDetected && (client_socket >= 0)) {
                std::cout << "close client connection" << std::endl;
                close(client_socket);
                m_telegramBuff.clear();
            }
        }
    }

    if (client_socket >= 0) {
        std::cout << "close client socket" << std::endl;
        close(client_socket);
    }

    close(server_socket);
}
