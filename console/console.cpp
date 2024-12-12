#include <asx/reactor.hpp>
#include <asx/ioport.hpp>

#include "console.hpp"


namespace console
{
   using namespace asx::ioport;

    using pca_local = PCA9555<1, 0x234, 0x23>;
    using pca_remote = PCA9555<1, 0x234, 0x23>;

   auto constexpr LED_A = PinDef{B, 1};
   auto constexpr LED_B = PinDef{B, 0};
   auto constexpr LED_C = PinDef{A, 2};
   auto constexpr RELAY_A = PinDef{B, 3};
   auto constexpr RELAY_B = PinDef{A, 7};
   auto constexpr RELAY_C = PinDef{A, 6};

   /** When constructed, the LED is ON to test it */
   void init()
   {
      using namespace std::chrono;

      Pin(LED_A).init(value::high, dir::out);
      Pin(LED_B).init(value::high, dir::out);
      Pin(LED_C).init(value::high, dir::out);
      Pin(RELAY_A).init(value::low, dir::out);
      Pin(RELAY_B).init(value::low, dir::out);
      Pin(RELAY_C).init(value::low, dir::out);

      // Reset the LEDs to the actual state after 2seconds
      asx::reactor::bind(clean_leds).delay(2s);
   }

   on_i2c_data_received


Master::init( 
    reactor::bind()

    
)

    static enum {
        set_pca1_dir,
        set_pca1_cfg,
        set_pca2_dir,
        set_pca2_cfg
    } stage;

// Call on start
pca1->init ---> pca1->init ---> pca2->init ---> pca2->init ---> sequencer ---> pca1 RW --> pca2 RW -> sequencer

reactor::bind(i2c_sequencer)();

// Called every 10ms
class Sequencer {
    ///< Debouncer for the inputs
    static inline auto debouncers = std::array<2, Debouncer>{};

    struct StateMachine {
        static inline auto pca1 = PCA1;
        static inline auto pca2 = PCA2;

        struct next {};
        struct input_read { uint16_t value; }
        static inline asx::chrono::time_point t;

        // Internal SM
        auto operator()() {
            using namespace boost::sml;

            auto init_pca1    = [] () { pca1.init(on_next); };
            auto init_pca2    = [] () { pca2.init(on_next); };
            auto process_pca1 = [] (const input_read& v) {   };
            auto process_pca2 = [] () {  };

            return make_transition_table(
            * "initialising_pca1"_s + on_entry<_>    / init_pca1
            , "initialising_pca1"_s + next           / init_pca2   = "initialising_pca2"_s
            , "initialising_pca2"_s + next           / set_timer   = "sampling_pca1"_s
            , "sampling_pca1"_s     + on_entry<_>    / sample_pca1
            , "sampling_pca1"_s     + input_read     / process_in  = "sampling_pca2"_s
            , "sampling_pca2"_s     + on_entry<_>    / sample_pca2
            , "sampling_pca2"_s     + input_read     / process_in  = "debouncing"_s
            , "debouncing"_s        + next                         = "sampling_pca1"_s
            );
        }
    };

    ///< The overall modbus state machine
    inline static auto sm = boost::sml::sm<StateMachine>{};

public:
    static void on_next() {
        sm.process_event(StateMachine::next);
    }

    static void process_in(uint8_t data) {
        using namespace boost::sml;
        uint8_t index = 0;

        if ( sm.is("sampling_pca1"_s) ) {
            index = 1;
        }
        
        //integrator[index].integrate(data);

        //

        sm.process_event(StateMachine::next{read});
    }

    static void set_timer() {
        asx::chrono::clock
        sm.process_event(StateMachine::next{read});
    }


}

void i2c_on_read() {
    // Grab the value
    // process - that's it!
    // Integrator on keys
    // Push on queue for Modbus

}

}