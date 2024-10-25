#ifndef TIMER_CPP_API_INCLUDED
#define TIMER_CPP_API_INCLUDED

#include <chrono>
#include <cstdint>
#include <utility>

extern "C" {
    #include "timer.h"  // Including the C timer API
}

namespace timer {

// Custom clock representing the 32-bit monotonic timer counter
class clock {
public:
    using duration = std::chrono::milliseconds<uint32_t>;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<clock, duration>;
    static constexpr bool is_steady = true;  // Monotonic, no adjustments

    // Get the current time based on the timer counter
    static time_point now() noexcept {
        return time_point(duration(timer_get_count()));
    }

    static time_point after(duration d) noexcept {
      return now() + d;
    }
};


// Alias
// Alias for std::chrono::milliseconds for easier usage
using duration = clock::duration;
using time_point = clock::time_point;


// Timer class that wraps timer_instance_t and provides utility methods
class Timer {
public:
    Timer() : instance(TIMER_INVALID_INSTANCE) {}

    Timer(timer_instance_t inst) : instance(inst) {}

    // Move constructor and assignment
    Timer(Timer&& other) noexcept : instance(other.instance) {
        other.instance = TIMER_INVALID_INSTANCE;
    }

    Timer& operator=(Timer&& other) noexcept {
        if (this != &other) {
            cancel();
            instance = other.instance;
            other.instance = TIMER_INVALID_INSTANCE;
        }
        return *this;
    }

    // Destructor
    ~Timer() {
        cancel();
    }

    // Cancel the timer if it's valid
    bool cancel() {
        if (instance != TIMER_INVALID_INSTANCE) {
            if (timer_cancel(instance)) {
                instance = TIMER_INVALID_INSTANCE;
                return true;
            }
        }
        return false;
    }

    // Check if the timer is valid
    bool is_valid() const {
        return instance != TIMER_INVALID_INSTANCE;
    }

private:
    timer_instance_t instance;

    // Disable copy constructor and assignment
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
};

// Add a cast operator to time_point to convert it to timer_count_t
inline constexpr timer_count_t to_timer_count(const time_point& tp) {
    return static_cast<timer_count_t>(tp.time_since_epoch().count());
}

// Add a cast operator to time_point to convert it to timer_count_t
inline constexpr timer_count_t to_timer_count(const duration& d) {
    return static_cast<timer_count_t>(d.count());
}

// C++ wrapper for timer_init
inline void init() {
    timer_init();
}


// Arm a timer and return a Timer object
Timer arm(reactor_handle_t reactor, time_point tp, duration repeat = duration{0}, void* arg = nullptr) {
    timer_instance_t instance = timer_arm(
        reactor,
        to_timer_count(tp),
        to_timer_count(repeat), 
        arg
    );

    return Timer(instance);  // Return the wrapped Timer instance
}

// Arm a timer and return a Timer object
Timer arm(reactor_handle_t reactor, duration delay, duration repeat = duration{0}, void* arg = nullptr) {
    timer_instance_t instance = timer_arm(
        reactor,
        to_timer_count(clock::after(delay)),
        to_timer_count(repeat),
        arg
    );

    return Timer(instance);  // Return the wrapped Timer instance
}

} // namespace timer

#endif // TIMER_CPP_API_INCLUDED
