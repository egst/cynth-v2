#pragma once

#include <utility>
#include <type_traits>

namespace cynth::constexpr_tools {

    /*template <typename T, T...> struct sequence_generator;

    template <typename T, T FROM, T STEP, T... REST>
    struct sequence_generator<T, FROM, STEP, FROM, REST...> {
        using type = std::integer_sequence<T, REST...>;
    };
    template <typename T, T FROM, T STEP, T FIRST, T... REST>
    struct sequence_generator<T, FROM, STEP, FIRST, REST...> {
        using type = std::conditional_t<(FIRST - STEP >= FROM),
            typename sequence_generator<T, FROM, std::min(FIRST - STEP, FROM), std::min(FIRST - STEP, FROM), REST...>::type, void>;
    };

    template <typename T, T FROM, T TO, T STEP>
    using make_sequence = std::conditional_t<(TO - FROM) % STEP == 0,
        typename sequence_generator<T, FROM, STEP, TO>::type, void>;*/

    template <typename T, T VAL>
    struct constexpr_wrapper { constexpr static T value = VAL; };

    template <typename Func, typename T, T... SEQ>
    constexpr void for_constexpr (Func func, std::integer_sequence<T, SEQ...>) {
        (func(constexpr_wrapper<T, SEQ>{}), ...);
    }
    /*template <typename T, T FROM, T TO, T STEP, typename Func>
    constexpr void for_constexpr (Func func) {
        for_constexpr (func, make_sequence<T, FROM, TO, STEP>{});
    }
    template <typename T, T FROM, T TO, typename Func>
    constexpr void for_constexpr (Func func) {
        for_constexpr (func, make_sequence<T, FROM, TO, 1>{});
    }*/
    template <typename T, T TO, typename Func>
    constexpr void for_constexpr (Func func) {
        //for_constexpr (func, make_sequence<T, 0, TO, 1>{});
        for_constexpr (func, std::make_integer_sequence<T, TO>{});
    }

}