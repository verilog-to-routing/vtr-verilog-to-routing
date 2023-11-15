#include "server.h"
#include "telegramparser.h"

#include <regex>
#include <sstream>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

// debug
#include <filesystem>
#include <string>
#include <fstream>
#include <iostream>
#include <thread>
// debug

namespace {

void writeFile(const std::filesystem::path& filename)
{
    std::ofstream file(filename);
    file << "hello from thread" << std::this_thread::get_id();
    file.close();

    // Optional: Check if the file was successfully created
    if (std::filesystem::exists(filename)) {
        std::cout << "File created: " << filename << "\n";
    } else {
        std::cerr << "Failed to create file\n";
    }
}
}

Server::Server()
{
    m_isStarted.store(false);
    m_isStopped.store(false);
    // m_isCleaned.store(false);
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
        writeFile("file_starting_server");
        m_isStarted.store(true);
        m_thread = std::thread(&Server::startListening, this);
    }
}

void Server::stop()
{
    if (!m_isStopped.load()) {
        m_isStopped.store(true);
        std::cout << "~~~ th=" << std::this_thread::get_id() << " stopping server, is stopped=" << m_isStopped.load() << std::endl;
        //writeFile("file_stopping_server");

        // // Start a new thread to join the server thread (running this from GUI thread leads to deadlock)
        // std::thread joiner([this]{
        //     if (m_thread.joinable()) {
        //         //std::cout << "~~~ th=" << std::this_thread::get_id() << " join thread START" << std::endl;
        //         writeFile("file_join_START");
        //         m_thread.join();
        //         writeFile("file_join_END");
        //         //std::cout << "~~~ th=" << std::this_thread::get_id() << " join thread FINISHED" << std::endl;
        //     }
        // });

        // // Detach the joiner thread since we don't need to join it
        // joiner.detach();

        //std::this_thread::sleep_for(std::chrono::milliseconds(5000));

        // while(!m_isCleaned.load()) {
        //     std::cout << "~~~ th=" << std::this_thread::get_id() << " waiting thread cleaned" << std::endl;
        //     std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        // }
        std::cout << "~~~ th=" << std::this_thread::get_id() << " Server.stop() finished" << std::endl;
    }                
}

void Server::takeRecievedTasks(std::vector<Task>& tasks)
{
    tasks.clear();
    //std::cout<<"---m_receivedTasksMutex START"<<std::endl;
    std::unique_lock<std::mutex> lock(m_receivedTasksMutex);
    if (m_receivedTasks.size() > 0) {
        std::cout << "take " << m_receivedTasks.size() << " num of received tasks"<< std::endl;
    }
    std::swap(tasks, m_receivedTasks);
    //std::cout<<"---m_receivedTasksMutex END"<<std::endl;
}

void Server::addSendTasks(const std::vector<Task>& tasks)
{
    //std::cout<<"---m_sendTasksMutex START"<<std::endl;
    std::unique_lock<std::mutex> lock(m_sendTasksMutex);
    for (const Task& task: tasks) {
        std::cout << "addSendTasks id=" << task.jobId() << std::endl;
        m_sendTasks.push_back(task);
    }
    //std::cout<<"---m_sendTasksMutex END"<<std::endl;
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
        std::cout << "~~~ th=" << std::this_thread::get_id() << "loop_______0000" << " is stopped=" << m_isStopped.load() << std::endl;
        client_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_socket >= 0) {
            // Handle sending
            {
                //std::cout<<"---m_sendTasksMutex222 START"<<std::endl;
                std::unique_lock<std::mutex> lock(m_sendTasksMutex);
                for (const Task& task: m_sendTasks) {
                    std::string response = task.toJsonStr();
                    response += END_TELEGRAM_SEQUENCE;
                    std::cout << "sending" << response << "to client" << std::endl;
                    send(client_socket, response.c_str(), response.length(), 0);
                }
                m_sendTasks.clear();
                //std::cout<<"---m_sendTasksMutex222 END"<<std::endl;
            }

            // Handle receiving
            char buffer[1024];
            ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes_received > 0) {
                // Process the received data (you can replace this with your own logic)
                std::string message(buffer, bytes_received);
                std::cout << "Received: " << message << std::endl;

                int jobId = telegramparser::extractJobId(message);
                int cmd = telegramparser::extractCmd(message);
                std::string options = telegramparser::extractOptions(message);
                std::cout << "server: jobId=" << jobId << ", cmd=" << cmd << ", options=" << options << std::endl;
                if ((jobId != -1) && (cmd != -1)) {
                    //std::cout<<"---m_receivedTasksMutex222 START"<<std::endl;
                    std::unique_lock<std::mutex> lock(m_receivedTasksMutex);
                    m_receivedTasks.emplace_back(jobId, cmd, options);
                    //std::cout<<"---m_receivedTasksMutex222 END"<<std::endl;
                }
            }

            // Close the client socket
            close(client_socket);
        }

        std::cout << "~~~ th=" << std::this_thread::get_id() << "loop_______111" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(COMM_LOOP_INTERVAL_MS));
        std::cout << "~~~ th=" << std::this_thread::get_id() << "loop_______222" << std::endl;
    }

    // m_isCleaned.store(true);
    std::cout << "~~~ th=" << std::this_thread::get_id() << "exit thread call!!!" << std::endl;

    close(server_socket);
}
