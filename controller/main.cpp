/**
 * @file
 * Main entry point for the pneumatic controller
 * Defines all input and output handling objects and most of the reactors
 * @author gax
 */
#include "board.h"
#include "pca9555.h"
#include "reactor.h"
#include "timer.h"

/************************************************************************/
/* Local functions                                                      */
/************************************************************************/

auto pcaLed = pca9555::pca9555(2);
void config_pca(void *arg);
void write_pca(void *arg);
auto do_config_pca = reactor_register( config_pca, reactor_prio_realtime );
auto do_write_pca = reactor_register( write_pca, reactor_prio_realtime );
auto pattern = uint16_t{1};

void config_pca(void *arg)
{
   pcaLed.write(pca9555::configure, 0);
   timer_arm(do_write_pca, timer_get_count_from_now(0), 50, NULL );
}

void write_pca(void *arg)
{
   pcaLed.write(pca9555::write, pattern);
   pattern <<= 1;
   if ( pattern == 0) pattern = 1;
}

int main(void)
{
   board_init();

   timer_arm(do_config_pca, timer_get_count_from_now(10), 0, NULL );

   // Run the reactor
   reactor_run();
}
