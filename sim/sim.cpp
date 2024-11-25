#include <unistd.h>
#include <avr/io.h>

#include <cstddef>
#include <vector>
#include <functional>
#include <initializer_list>

extern "C" void sysclk_init() {}

extern "C" void interrupt_USART1_RXC_vect();
extern "C" void interrupt_TCA0_CMP0_vect();
extern "C" void interrupt_TCA0_CMP1_vect();
extern "C" void interrupt_TCA0_OVF_vect();
extern "C" void interrupt_USART1_TXC_vect();

// Holds all the MCA registers here
char sim_registers[0x1301];

using action = std::function<void()>;
using test = std::initializer_list<action>;

test TEST_SEND_RECEIVE = {
    [](){ interrupt_TCA0_CMP1_vect(); },
    [](){ USART1.RXDATAL = 44;   interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x01; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x00; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0xff; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x00; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x01; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0xCB; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x87; interrupt_USART1_RXC_vect(); },
    [](){ interrupt_TCA0_CMP0_vect(); },
    [](){ interrupt_TCA0_CMP1_vect(); },
    [](){ interrupt_TCA0_OVF_vect(); /* Reply should be sent */ },
    [](){ interrupt_USART1_TXC_vect(); /* Buffer sent! */ },
    [](){ interrupt_TCA0_CMP1_vect(); /* 35 elapsed */ }
};

std::vector<test> ALL_TESTS = { TEST_SEND_RECEIVE };
auto itTest = ALL_TESTS.begin();
auto itAction = itTest->begin();

extern "C" void sleep_cpu()
{
    (*itAction++)();

    if ( itAction == itTest->end() ) {
        itTest++;
        if ( itTest == ALL_TESTS.end() ) _exit(0);
    }
}