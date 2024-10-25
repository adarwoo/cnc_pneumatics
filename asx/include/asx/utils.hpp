#pragma once
/**
 * Create constexpr ints for flags
 * Usage:
 *    Define a group

 */

#include <cstddef>
#include <utility>
#include <algorithm>

namespace asx
{
   /**
    * Defines a string litteral type which allow passing a string in a template type
    */
   template <std::size_t N>
   struct string_literal {
      constexpr string_literal(const char (&str)[N]) {
         std::copy_n(str, N, value);
      }

      char value[N];
      constexpr std::string_view view() const { return { value, N - 1 }; }
   };

}