#pragma once

#include <asx/bitstore.hpp>

namespace asx {
    template<size_t N, size_t THR>
    class Debouncer {
        using bitstore_t = BitStore<N>;
        using status_t = bitstore_t::storage_t;

        auto inputs = bitstore_t{};
        auto integrator = std::array<N, uint8_t>{};

    public:
        bitstore_t append(status_t raw_sample) {
            auto previous = inputs;
            auto sample = bitstore_t{raw_sample};

            for (uint8_t i=0; i<sample.size(); ++i) {
                bool level = sample.get(i);
                uint8_t thr = integrator[i];

                if (level) {
                    if ( integrator[i] < THR ) {
                        ++integrator[i];
                    }

                    if ( integrator[i] == THR ) {
                        input.set(i);
                    }
                } else {
                    if ( integrator[i] > 0 ) {
                        --integrator[i];
                    }

                    if ( integrator[i] == 0 ) {
                        input.clear(i);
                    }
                }
            }

            // Return all bits which have changed to become 'true' (key is on)
            return (previous ^ input) & input;
        }

        bitstore_t status() {
            return inputs;
        }
    };

}
