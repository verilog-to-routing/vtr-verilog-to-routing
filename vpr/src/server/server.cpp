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
    int server_socket, client_socket;
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
    address.sin_port = htons(PORT_NUM);

    // Bind the socket to the network address and port
    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    } else {
        std::cout << "~~~ th=" << std::this_thread::get_id() << " start listening port: " << PORT_NUM << std::endl;
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

    // Event loop
    while(!m_isStopped.load()) {
        //std::cout << "~~~ th=" << std::this_thread::get_id() << " loop_000" << " is stopped=" << m_isStopped.load() << std::endl;
        int flags = fcntl(client_socket, F_GETFL, 0);
        fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);
        client_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen);

        //std::cout << "~~~ th=" << std::this_thread::get_id() << " loop_111" << std::endl;
        if (client_socket >= 0) {
            // Handle sending
            {
                std::unique_lock<std::mutex> lock(m_sendTasksMutex);
                for (const Task& task: m_sendTasks) {
                    std::string response = task.toJsonStr();
                    response += END_TELEGRAM_SEQUENCE;
                    std::cout << "sending" << response << "to client" << std::endl;
                    send(client_socket, response.c_str(), response.length(), 0);
                    std::cout << "sent" << std::endl;
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
            int result = select(client_socket + 1, &readfds, NULL, NULL, &tv);

            if (result == -1) {
                std::cerr << "Error in select()\n";
            } else if (result == 0) {
                std::cout << "Timeout occurred! No data available.\n";
            } else {
                if (FD_ISSET(client_socket, &readfds)) {
                    // Data is available; proceed with recv
                    char buffer[1024];

                    std::cout << "before receive" << std::endl;
                    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
                    std::cout << "after receive" << std::endl;
                    if (bytes_received > 0) {
                        // Process received data
                        std::string message(buffer, bytes_received);
                        std::cout << "Received: " << message << std::endl;

                        int jobId = telegramparser::extractJobId(message);
                        int cmd = telegramparser::extractCmd(message);
                        std::string options = telegramparser::extractOptions(message);
                        std::cout << "server: jobId=" << jobId << ", cmd=" << cmd << ", options=" << options << std::endl;
                        if ((jobId != -1) && (cmd != -1)) {
                            std::unique_lock<std::mutex> lock(m_receivedTasksMutex);
                            m_receivedTasks.emplace_back(jobId, cmd, options);
                        }
                    } else if (bytes_received == 0) {
                        std::cout << "Connection closed\n";
                    } else {
                        std::cerr << "Error in recv()\n";
                    }
                }
            }

            // Close the client socket
            close(client_socket);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(COMM_LOOP_INTERVAL_MS));
        //std::cout << "~~~ th=" << std::this_thread::get_id() << " loop_222" << std::endl << std::endl;
    }

    close(server_socket);
    //std::cout << "~~~ th=" << std::this_thread::get_id() << " exit thread call!!!" << std::endl;
}
