#ifndef MIGRAPH_GUARD_ERRORS_HPP
#define MIGRAPH_GUARD_ERRORS_HPP

#include <exception>
#include <stdexcept>
#include <string>

namespace migraph {

/// Represents exceptions that can be thrown by migraphlib
struct exception : std::runtime_error
{
    exception(std::string msg = "") : std::runtime_error(msg) {}
};

/**
 * @brief Create an exception object
 *
 * @param context A message that says where the exception occurred
 * @param message Custom message for the error
 * @return Exceptions
 */
inline exception make_exception(std::string context, std::string message = "")
{
    return {context + ": " + message};
}

/**
 * @brief Create a message of a file location
 *
 * @param file The filename
 * @param line The line number
 *
 * @return A string that represents the file location
 */
inline std::string make_source_context(const std::string& file, int line)
{
    return file + ":" + std::to_string(line);
}

/**
 * @brief Throw an exception with context information
 */
#define MIGRAPH_THROW(...) \
    throw migraph::make_exception(migraph::make_source_context(__FILE__, __LINE__), __VA_ARGS__)

} // namespace migraph

#endif