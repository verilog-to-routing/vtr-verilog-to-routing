#ifndef TASK_H
#define TASK_H

#ifndef NO_SERVER

#include <string>
#include <memory>
#include <chrono>

#include "telegramheader.h"
#include "commcmd.h"

namespace server {

/**
 * @brief Implements the server task.
 * 
 * This structure aids in encapsulating the client request, request result, and result status.
 * It generates a JSON data structure to be sent back to the client as a response.
 */
class Task {
public:
    /**
     * @brief Constructs a new Task object.
     * 
     * @param job_id The ID of the job associated with the task.
     * @param cmd The command ID (see @ref comm::CMD) associated with the task.
     * @param options Additional options for the task (default: empty string).
     */
    Task(int job_id, comm::CMD cmd, const std::string& options = "");

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    /**
     * @brief Gets the job ID associated with the task.
     * 
     * @return The job ID.
     */
    int job_id() const { return m_job_id; }

    /**
     * @brief Gets the command ID associated with the task.
     * 
     * @return The command ID (see @ref comm::CMD).
     */
    comm::CMD cmd() const { return m_cmd; }

    /**
     * @brief Removes the specified number of bytes from the response buffer.
     * 
     * This method removes the specified number of bytes from the beginning of the response buffer.
     * It is typically used after sending a response to discard the bytes that have been sent.
     * 
     * @param bytes_sent_num The number of bytes to remove from the response buffer.
     */
    void chop_num_sent_bytes_from_response_buffer(std::size_t bytes_sent_num);

    /**
     * @brief Checks if the options of this task match the options of another task.
     * 
     * This method compares the options of this task with the options of another task
     * and returns true if they match, otherwise returns false.
     * 
     * @param other The other task to compare options with.
     * @return True if the options match, false otherwise.
     */
    bool options_match(const std::unique_ptr<Task>& other);

    /**
     * @brief Retrieves the response buffer.
     * 
     * This method returns a constant reference to the response buffer, which contains the data after task execution.
     * 
     * @return A constant reference to the response buffer.
     */
    const std::string& response_buffer() const { return m_response_buffer; }

    /**
     * @brief Checks if the task has finished execution.
     * 
     * This method returns true if the task has finished execution; otherwise, it returns false.
     * 
     * @return True if the task has finished execution, false otherwise.
     */
    bool is_finished() const { return m_is_finished; }

    /**
     * @brief Checks if the task has encountered an error.
     * 
     * This method returns true if the task has encountered an error; otherwise, it returns false.
     * 
     * @return True if the task has encountered an error, false otherwise.
     */
    bool has_error() const { return !m_error.empty(); }

    /**
     * @brief Retrieves the error message associated with the task.
     * 
     * This method returns the error message associated with the task, if any.
     * 
     * @return A constant reference to the error message string.
     */
    const std::string& error() const { return m_error; }

    /**
     * @brief Retrieves the original number of response bytes.
     * 
     * This method returns the original number of response bytes before any chopping operation.
     * 
     * @return The original number of response bytes.
     */
    std::size_t orig_reponse_bytes_num() const { return m_orig_reponse_bytes_num; }

    /**
     * @brief Checks if the response has been fully sent.
     * 
     * This method returns true if the entire response has been successfully sent, 
     * otherwise it returns false.
     * 
     * @return True if the response has been fully sent, false otherwise.
     */
    bool is_response_fully_sent() const { return m_is_response_fully_sent; }

    /**
     * @brief Marks the task as failed with the specified error message.
     * 
     * This method sets the task's error message to the provided error string, 
     * indicating that the task has failed.
     * 
     * @param error The error message describing the reason for the task's failure.
     */
    void set_fail(const std::string& error);

    /**
     * @brief Marks the task as successfully completed.
     */
    void set_success();

    /**
     * @brief Marks the task as successfully completed with the specified result.
     * 
     * This method marks the task as successfully completed and stores the result.
     * It takes an rvalue reference to a string, allowing for efficient move semantics.
     * 
     * @param result An rvalue reference to a string describing the result of the task execution.
     *               The content of this string will be moved into the task's result storage.
     */
    void set_success(std::string&& result);

    /**
     * @brief Generates a string containing information about the task.
     * 
     * This method generates a string containing information about the task,
     * including its identifier, command, options, and optionally its duration.
     * 
     * @param skip_duration If true, the duration information will be omitted from the string.
     * 
     * @return A string containing information about the task.
     */
    std::string info(bool skip_duration = false) const;

    /**
     * @brief Retrieves the TelegramHeader associated with the task.
     * 
     * This method returns a reference to the TelegramHeader associated with the task.
     * 
     * @return A reference to the TelegramHeader associated with the task.
     */
    const comm::TelegramHeader& telegram_header() const { return m_telegram_header; }

    /**
     * @brief Retrieves the options associated with the task.
     * 
     * This method returns a reference to the options string associated with the task.
     * 
     * @return A reference to the options string associated with the task.
     */
    const std::string& options() const { return m_options; }

private:
    int m_job_id = -1;
    comm::CMD m_cmd = comm::CMD::NONE;
    std::string m_options;
    std::string m_result;
    std::string m_error;
    bool m_is_finished = false;
    comm::TelegramHeader m_telegram_header;
    std::string m_response_buffer;
    std::size_t m_orig_reponse_bytes_num = 0;
    bool m_is_response_fully_sent = false;

    std::chrono::high_resolution_clock::time_point m_creation_time;

    int64_t time_ms_elapsed() const;

    void bake_response();
};
using TaskPtr = std::unique_ptr<Task>;

} // namespace server

#endif /* NO_SERVER */

#endif /* TASK_H */
