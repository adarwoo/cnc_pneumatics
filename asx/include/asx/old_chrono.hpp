#include <stdint.h>
#include <etl/ratio.h>

extern "C" volatile uint32_t sysclock_counter;

namespace asx {
    namespace chrono {

        // Simulated system clock function that returns the time in milliseconds
        inline uint32_t sysclock() {
            return sysclock_counter;
        }

        // Duration class that represents time intervals
        template <typename Rep, typename Period>
        class duration {
        public:
            explicit duration(Rep count) : count_(count) {}

            Rep count() const { return count_; }

            // Allow adding durations of the same type
            duration operator+(const duration& other) const {
                return duration(count_ + other.count_);
            }

        private:
            Rep count_;
        };

        // Alias for milliseconds and seconds
        using milliseconds = duration<uint32_t, etl::ratio<1, 1000>>;
        using seconds = duration<uint32_t, etl::ratio<1>>;

        // Time point class representing a specific point in time
        template <typename Clock, typename Duration>
        class time_point {
        public:
            explicit time_point(Duration d) : duration_since_epoch_(d) {}

            Duration time_since_epoch() const { return duration_since_epoch_; }

            // Add durations to a time_point
            time_point& operator+=(const Duration& d) {
                duration_since_epoch_ = duration_since_epoch_ + d;
                return *this;
            }

        private:
            Duration duration_since_epoch_;
        };

        // Alias for time_point based on milliseconds
        using time_point_ms = time_point<struct steady_clock, milliseconds>;

        // Steady clock class representing a clock that cannot be adjusted
        struct steady_clock {
            using duration = milliseconds;
            using time_point = time_point_ms;

            static time_point now() {
               return time_point(duration(static_cast<uint32_t>(sysclock())));
            }
        };

        // User-defined literals for milliseconds
        inline milliseconds operator""_ms(unsigned long long ms) {
            return milliseconds(static_cast<uint32_t>(ms));
        }

        // User-defined literals for seconds
        inline seconds operator""_s(unsigned long long s) {
            return seconds(static_cast<uint32_t>(s));
        }

    } // namespace chrono
} // namespace asx
