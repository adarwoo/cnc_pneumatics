/******************************************************************************
The MIT License(MIT)
https://github.com/adarwoo/pneumatic_door_controller

Copyright(c) 2021 Guillaume ARRECKX - software@arreckx.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/
#ifndef state_machine_h_included
#define state_machine_h_included

#include <boost/sml.hpp>

namespace sml = boost::sml;

// Create those simple events
struct on_open {};
struct on_close {};
struct door_is_up {};
struct door_is_down {};
struct timeout {};
struct comms_established; {};
struct comms_error {};
   
   
namespace
{
   ///< Dummy event to all processing forks in SM
   struct next
   {};

   ///< Marker to force the SM to evaluate 'next'
   bool controller_process_next{ false };
}  // namespace


struct sm_pdc
{
   auto operator()() const noexcept
   {
      using namespace sml;

      return make_transition_table(
         *"init"_s + event<comms_established> = state<mode_manual>,
         "splash"_s + sml::on_entry<_> / []( UIView &v ) { v.draw_splash(); }

         ,
         state<mode_manual> + sml::on_entry<_> / []( UIView &v ) { v.draw(); },
         state<mode_manual> + event<usb_on> / []( UIView &v ) { v.draw_usb(); } = state<mode_usb>,
         state<mode_usb> + event<usb_off> = state<mode_manual> );
   }
};

#endif  // state_machine_h_included