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
void i2c_sequencer() {

    struct StateMachine {
        // Internal SM
        auto operator()() {
            using namespace boost::sml;

            auto start_timer = [] () { Timer::start(); };
            auto reset       = [] () { Datagram::reset(); };
            auto ready_reply = [] () { Datagram::ready_reply(); };
            auto reply       = [] () { Uart::send(Datagram::get_buffer()); };

            auto handle_char = [] (const auto& event) {
                Timer::start(); // Restart the timers (15/35/40)
                Datagram::process_char(event.c);
            };

            return make_transition_table(
            * "cold"_s                + event<can_start_receiving>                    = "initial"_s
            , "initial"_s             + on_entry<_>                     / start_timer
            , "initial"_s             + event<t35_timeout>                            = "idle"_s
            , "initial"_s             + event<char_received>            / start_timer = "initial"_s
            , "idle"_s                + on_entry<_>                     / reset
            , "idle"_s                + event<char_received>            / handle_char = "reception"_s
            , "idle"_s                + event<demand_of_emission>                     = "emission"_s
            , "reception"_s           + event<t15_timeout>                            = "control_and_waiting"_s
            , "reception"_s           + event<char_received>            / handle_char = "reception"_s
            , "control_and_waiting"_s + event<t35_timeout> [must_reply]               = "reply"_s
            , "control_and_waiting"_s + event<char_received>                          = "initial"_s
            , "control_and_waiting"_s + event<t35_timeout>                            = "idle"_s
            , "reply"_s               + on_entry<_>                     / ready_reply
            , "reply"_s               + event<char_received>            / handle_char = "initial"_s // Unlikely - but a possibility
            , "reply"_s               + event<t40_timeout>                            = "emission"_s
            , "emission"_s            + on_entry<_>                     / reply
            , "emission"_s            + event<frame_sent>                             = "initial"_s
            );

    static auto pca1 = PCA1;
    static auto pca2 = PCA2;

    static enum { init_pca1, init_pca2, rw1, rw2 } stage = init_pca1;

    switch ( stage )
    {
    case init_pca1:
        pca1.init(i2c_sequencer);
        break;
    case init_pca2:
        pca2.init(i2c_sequencer);
        break;
    case rw1:
        pca1.readwrite(fb, i2c_on_read1);
        break;
    case rw2:
        pca1.readwrite(fb, i2c_on_read1);
        break;
    default:
        break;
    }


}

void i2c_on_read() {
    // Grab the value
    // process - that's it!
    // Integrator on keys
    // Push on queue for Modbus

}

}