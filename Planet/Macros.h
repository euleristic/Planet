#pragma once

// The application is essentially written in two versions, depending on whether the _DEBUG flag is defined.
// The release version can therefore be mostly noexcept and iostream free.
#ifdef _DEBUG

// Includes
#include <iostream>
#include <stdexcept>
#include <array>
#include <chrono>

// Starts a try block. I don't like this because it is a macro that hides the fact you are opening a scope.
// However, I want it only to be the case if _DEBUG, so it's this or wrapping it in #ifdef, #endif in situ.
#define TRY_IF_DEBUG try {

// Catches any exception since TRY_IF_DEBUG and prints it.
#define CATCH_IF_DEBUG } catch (std::exception e) { std::cerr << e.what() << '\n'; }

// Throws and prints message along with the latest SDL error.
#define THROW_SDL_ERROR_IF_DEBUG(message) throw std::runtime_error((std::string(message) + ": " + SDL_GetError()).c_str())

// Throws and prints message
#define THROW_IF_DEBUG(message) throw std::runtime_error(message)

// Marks a function as noexcept, if not in debug
#define NOEXCEPT_IF_NOT_DEBUG

// Replace execution policy argument with "policy" (i.e. "COMPARE_EXECUTION_POLICIES(reduce(policy, {begin}, {end}));")
#define COMPARE_EXECUTION_POLICIES(AlgorithmCall)                             \
static std::array<size_t, 4> record{};										   \
std::array<std::chrono::nanoseconds, 4> execution_times;                        \
{																		         \
auto policy = std::execution::seq;                                                \
auto start = std::chrono::high_resolution_clock::now();                            \
AlgorithmCall;                                                                      \
execution_times[0] = std::chrono::high_resolution_clock::now() - start;              \
}{																					  \
auto policy = std::execution::par;                                                     \
auto start = std::chrono::high_resolution_clock::now();                                 \
AlgorithmCall;                                                                           \
execution_times[1] = std::chrono::high_resolution_clock::now() - start;                   \
}{																						   \
auto policy = std::execution::par_unseq;                                                    \
auto start = std::chrono::high_resolution_clock::now();                                      \
AlgorithmCall;                                                                                \
execution_times[2] = std::chrono::high_resolution_clock::now() - start;                        \
}{																							    \
auto policy = std::execution::unseq;                                                             \
auto start = std::chrono::high_resolution_clock::now();                                           \
AlgorithmCall;                                                                                     \
execution_times[3] = std::chrono::high_resolution_clock::now() - start;                             \
}																									 \
++record[std::min_element(execution_times.begin(), execution_times.end()) - execution_times.begin()]; \
std::cout << "seq: " << record[0] << ". par:" << record[1] << ". par_unseq: " << record[2] << ". unseq: " << record[3] << '\n'



#else // !_DEBUG
#define TRY_IF_DEBUG
#define CATCH_IF_DEBUG
#define THROW_SDL_ERROR_IF_DEBUG(message)
#define THROW_IF_DEBUG(message)
#define NOEXCEPT_IF_NOT_DEBUG noexcept
#define COMPARE_EXECUTION_POLICIES(AlgorithmCall) Not"defined"if'not'debug"!"Remove'macro'!
#endif // _DEBUG

