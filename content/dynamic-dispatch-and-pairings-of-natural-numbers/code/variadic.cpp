#include <boost/mp11.hpp>
#include <cassert>  // assert macro
#include <cmath>
#include <concepts>
#include <limits>
#include <magic_enum.hpp>

//////////////////////////////
////////// NTTP TUP //////////
//////////////////////////////

#include <boost/mp11.hpp>
#include <utility>

template <std::size_t MaxCount, typename Is, typename... Ts>
struct tup_impl;

template <>
struct tup_impl<0, std::index_sequence<>, boost::mp11::mp_list<>> {
    static constexpr std::size_t size = 0;
};

template <std::size_t MaxCount, std::size_t I, typename T>
struct tup_impl<MaxCount, std::index_sequence<I>, boost::mp11::mp_list<T>> {
    static constexpr std::size_t size = MaxCount;
    T x;
    constexpr T get(
        std::integral_constant<std::size_t, MaxCount - I - 1>) const {
        return x;
    }
};

template <std::size_t MaxCount, std::size_t I, std::size_t... Is, typename T,
          typename... Ts>
struct tup_impl<MaxCount, std::index_sequence<I, Is...>,
                boost::mp11::mp_list<T, Ts...>>
    : tup_impl<MaxCount, std::index_sequence<Is...>,
               boost::mp11::mp_list<Ts...>> {
    T x;
    using tup_impl<MaxCount, std::index_sequence<Is...>,
                   boost::mp11::mp_list<Ts...>>::get;
    constexpr T get(
        std::integral_constant<std::size_t, MaxCount - I - 1>) const {
        return x;
    }
};

// NOTE: mp11 to reverse the list so that the aggregate initialization order
// corresponds to the tomplate parameter order
template <typename... Ts>
struct tup : tup_impl<sizeof...(Ts), std::make_index_sequence<sizeof...(Ts)>,
                      boost::mp11::mp_reverse<boost::mp11::mp_list<Ts...>>> {};

template <typename... Ts>
tup(Ts...) -> tup<Ts...>;

template <typename... Ts>
struct is_tup_ : std::false_type {};
template <typename... Ts>
struct is_tup_<tup<Ts...>> : std::true_type {};
template <typename... Ts>
concept is_tup = is_tup_<Ts...>::value;

template <std::size_t i>
constexpr auto p(is_tup auto t) {
    return t.get(std::integral_constant<std::size_t, i>{});
}

template <typename... Ts>
constexpr tup<Ts...> tup_cat(tup<Ts...> xs) {
    return xs;
}

template <typename... Ts, typename... Us>
constexpr tup<Ts..., Us...> tup_cat(tup<Ts...> xs, tup<Us...> ys) {
    constexpr auto tup_cat_ =
        []<std::size_t... Is, std::size_t... Js>(
            tup<Ts...> xs, std::index_sequence<Is...>, tup<Us...> ys,
            std::index_sequence<Js...>) -> tup<Ts..., Us...> {
        return {p<Is>(xs)..., p<Js>(ys)...};
    };
    return tup_cat_(xs, std::make_index_sequence<sizeof...(Ts)>(), ys,
                    std::make_index_sequence<sizeof...(Us)>());
}

constexpr auto operator+(is_tup auto xs, is_tup auto ys) {
    return tup_cat(xs, ys);
}

template <is_tup... Ts>
constexpr auto tup_cat(Ts... xss) {
    return (tup{} + ... + xss);
}

/////////////////////////////////////
////////// SZUDZIK PAIRING //////////
/////////////////////////////////////

template <typename... Ts>
struct unsigned_arithmetic_result;

template <typename T>
struct unsigned_arithmetic_result<T> {
    using type =
        std::make_unsigned_t<decltype(std::declval<T>() + std::declval<T>())>;
};

template <typename T1, typename T2>
struct unsigned_arithmetic_result<T1, T2> {
    using type =
        std::make_unsigned_t<decltype(std::declval<T1>() + std::declval<T2>())>;
};

template <typename T1, typename T2, typename... Ts>
struct unsigned_arithmetic_result<T1, T2, Ts...>
    : unsigned_arithmetic_result<
          typename unsigned_arithmetic_result<T1, T2>::type, Ts...> {};

template <typename... Ts>
using unsigned_arithmetic_result_t =
    typename unsigned_arithmetic_result<Ts...>::type;

// Szudzik's Elegant Pairing Function
// http://szudzik.com/ElegantPairing.pdf
template <std::unsigned_integral T, std::unsigned_integral S>
constexpr unsigned_arithmetic_result_t<T, S> szudzik_pair(T x, S y) {
    return y > x ? x + y * y : x * x + x + y;
}

template <std::unsigned_integral T>
constexpr T szudzik_pair(T x) {
    return x;
}

template <std::unsigned_integral T, std::unsigned_integral U,
          std::integral... Ts>
constexpr unsigned_arithmetic_result_t<T, Ts...> szudzik_pair(T x, U y,
                                                              Ts... xs) {
    return szudzik_pair(szudzik_pair(x, y), xs...);
}

// metafunction version of szudzik_pair to use with mp11
template <typename... Ts> /* requires (is_integral_constant<Ts> and ...) */
using mp_szudzik_pair =
    std::integral_constant<decltype(szudzik_pair(Ts::value...)),
                           szudzik_pair(Ts::value...)>;

template <typename... Ts>
using mp_szudzik_pair_over_product =
    boost::mp11::mp_product<mp_szudzik_pair, Ts...>;

// need for constexpr
template <std::unsigned_integral I>
constexpr I isqrt(I x) {
    I out{};
    for (; (out + 1) * (out + 1) <= x; out++);
    return out;
}

template <std::unsigned_integral T, std::unsigned_integral S,
          std::same_as<unsigned_arithmetic_result_t<T, S>> I>
constexpr tup<T, S> szudzik_unpair__(I x) {
    // cannot use variables for common subexpressions due to constexpr
    // constraints:
    //
    // q = isqrt(x)
    // l = x - isqrt(x) * isqrt(x)

    // Casts shouldn't cause a narrowing issue here, as we have
    // already checked I == unsigned_arithmetic_result_t<T, S> and we
    // know the values come from a szudzik_pair function so the
    // decomposition lies in the correct range of values for T and S
    return (x - isqrt(x) * isqrt(x)) < isqrt(x)
               ? tup{static_cast<T>(x - isqrt(x) * isqrt(x)),
                     static_cast<S>(isqrt(x))}
               : tup{static_cast<T>(isqrt(x)),
                     static_cast<S>((x - isqrt(x) * isqrt(x)) - isqrt(x))};
}

template <std::unsigned_integral T>
constexpr T szudzik_unpair__(T i) {
    return i;
}

template <typename ts>
constexpr auto szudzik_unpair_(std::unsigned_integral auto x) {
    constexpr auto N = boost::mp11::mp_size<ts>::value;
    static_assert(N > 0);

    using fully_decoded = boost::mp11::mp_back<ts>;
    if constexpr (N == 1) {
        return tup<fully_decoded>{szudzik_unpair__<fully_decoded>(x)};
    } else if constexpr (N == 2) {
        // This case is needed only to ensure overflow safety. Because it is
        // possible for
        //
        // - fully_decoded == also_fully_decoded, yet
        //
        // - fully_decoded != decltype(x), where
        //
        // - decltype(x) == unsigned_arithmetic_result_t<fully_decoded,
        // also_fully_decoded>
        //
        // If we have solely the unary base case, then we drop information
        // to know if fully_decoded != decltype(x) because we screwed up
        // somewhere or if it is a safe conversion (within the range of the
        // destination type)
        using also_fully_decoded = boost::mp11::mp_front<ts>;
        return szudzik_unpair__<also_fully_decoded, fully_decoded>(x);
    } else /* constexpr */ {
        using ts_tail = boost::mp11::mp_pop_back<ts>;
        using decoded_unsigned_arithmetic_result_t =
            boost::mp11::mp_fold<ts_tail, boost::mp11::mp_back<ts_tail>,
                                 unsigned_arithmetic_result_t>;
        return szudzik_unpair_<ts_tail>(
                   p<0>(szudzik_unpair__<decoded_unsigned_arithmetic_result_t,
                                         fully_decoded>(x))) +
               tup<fully_decoded>{
                   p<1>(szudzik_unpair__<decoded_unsigned_arithmetic_result_t,
                                         fully_decoded>(x))};
    }
}

template <std::unsigned_integral T, std::integral... Ts>
constexpr tup<T, Ts...> szudzik_unpair(std::unsigned_integral auto x) {
    return szudzik_unpair_<boost::mp11::mp_list<T, Ts...>>(x);
}

//////////////////////////////////////////
////////// ENUM VALUES TO TYPES //////////
//////////////////////////////////////////

template <auto Array, std::size_t... I>
constexpr auto lift_array_(std::index_sequence<I...>) -> boost::mp11::mp_list<
    std::integral_constant<typename decltype(Array)::value_type, Array[I]>...>;

template <auto Array>
using array_to_mp_list =
    decltype(lift_array_<Array>(std::make_index_sequence<Array.size()>{}));

template <typename Enum>
using mp_enum_to_enum_values =
    array_to_mp_list<magic_enum::enum_values<Enum>()>;

template <typename T> /* requires is_integral_constant<T> */
using mp_to_underlying = std::integral_constant<
    std::underlying_type_t<typename T::value_type>,
    static_cast<std::underlying_type_t<typename T::value_type>>(T::value)>;

template <typename EnumValues>
using mp_enum_values_to_underlying =
    boost::mp11::mp_transform<mp_to_underlying, EnumValues>;

/////////////////////////////////////////
//////// DISPATCH ENUM FUNCTION /////////
/////////////////////////////////////////

template <typename F, typename... Enums>
    requires(std::is_enum<Enums>::value and ...)
constexpr auto dispatch_enum(F&& f, Enums... enums) {
    using ts = boost::mp11::mp_list<Enums...>;

    using ts_integral_values = boost::mp11::mp_transform_q<
        boost::mp11::mp_compose<mp_enum_to_enum_values,
                                mp_enum_values_to_underlying>,
        ts>;

    using encoded_pairs =
        boost::mp11::mp_apply<mp_szudzik_pair_over_product, ts_integral_values>;

    using max_code =
        boost::mp11::mp_max_element<encoded_pairs, boost::mp11::mp_less>;

    using encoded_type = max_code::value_type;

    encoded_type encoded =
        szudzik_pair(static_cast<std::underlying_type_t<Enums>>(enums)...);

    // Here `encoded` may have been turned into an int due to common_type and
    // -Wsign-conversion will yell because mp_with_index converts to
    // std::size_t. But we know we are operating with unsigned numbers (problem
    // restriction)
    return boost::mp11::mp_with_index<max_code>(
        static_cast<std::size_t>(encoded), [&](auto i) {
            // decltype(i)::value_type == std::size_t, but we know it fits in
            // the range of encoded_type (since it is <= max_code)
            constexpr auto decoded_pair =
                szudzik_unpair<std::underlying_type_t<Enums>...>(
                    static_cast<encoded_type>(decltype(i)::value));
            return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return std::forward<F>(f)(
                    std::integral_constant<Enums,
                                           Enums{p<Is>(decoded_pair)}>{}...);
            }(std::make_index_sequence<sizeof...(Enums)>{});
        });
}

#include <format>
#include <iostream>
#include <stdexcept>

/*
  state machine transition table ('->' indicates initial state, '*' inidicates
  final states)
     |   | a | b | c |
  -> | A | A | B | C |
     | B | B | C | D |
     | C | C | B | D |
   * | D | - | - | - |
 */

enum class states : std::uint16_t { A = 0, B = 1, C = 2, D = 3 };
enum class events : std::uint16_t { a = 0, b = 1, c = 2 };
enum class inputs : std::uint32_t { x = 0, y = 1, z = 2 };

struct fsm {
    void operator()(auto a, auto b) {
        throw std::domain_error{"invalid transition"};
    }

    void operator()(std::integral_constant<states, states::A>,
                    std::integral_constant<events, events::a>) {
        s = states::A;
        std::cout << "(A, a) -> A\n";
    }

    void operator()(std::integral_constant<states, states::A>,
                    std::integral_constant<events, events::b>) {
        s = states::B;
        std::cout << "(A, b) -> B\n";
    }

    void operator()(std::integral_constant<states, states::A>,
                    std::integral_constant<events, events::c>) {
        s = states::C;
        std::cout << "(A, c) -> C\n";
    }

    void operator()(std::integral_constant<states, states::B>,
                    std::integral_constant<events, events::a>) {
        s = states::B;
        std::cout << "(B, a) -> B\n";
    }

    void operator()(std::integral_constant<states, states::B>,
                    std::integral_constant<events, events::b>) {
        s = states::C;
        std::cout << "(B, b) -> C\n";
    }

    void operator()(std::integral_constant<states, states::B>,
                    std::integral_constant<events, events::c>) {
        s = states::D;
        std::cout << "(B, c) -> D\n";
    }

    void operator()(std::integral_constant<states, states::C>,
                    std::integral_constant<events, events::a>) {
        s = states::C;
        std::cout << "(C, a) -> C\n";
    }

    void operator()(std::integral_constant<states, states::C>,
                    std::integral_constant<events, events::b>) {
        s = states::B;
        std::cout << "(C, b) -> B\n";
    }

    void operator()(std::integral_constant<states, states::C>,
                    std::integral_constant<events, events::c>) {
        s = states::D;
        std::cout << "(C, c) -> D\n";
    }

    constexpr void dispatch_event(events e) { dispatch_enum(*this, s, e); }

    [[nodiscard]] constexpr bool done() const { return s == states::D; }

   private:
    states s{states::A};
};

struct single_arg_dispatch {
    void operator()(std::integral_constant<states, states::A>) {
        std::cout << "State A\n";
    }

    void operator()(std::integral_constant<states, states::B>) {
        std::cout << "State B\n";
    }

    void operator()(std::integral_constant<states, states::C>) {
        std::cout << "State C\n";
    }
};

struct triple_arg_dispatch {
    void operator()(auto x, auto y, auto z) {
        std::cout << "Invalid transition ("
                  << static_cast<std::uint16_t>(decltype(x)::value) << ", "
                  << static_cast<std::uint16_t>(decltype(y)::value) << ", "
                  << static_cast<std::uint32_t>(decltype(z)::value) << ")\n";
    }

    void operator()(std::integral_constant<states, states::A>,
                    std::integral_constant<events, events::a>,
                    std::integral_constant<inputs, inputs::x>) {
        std::cout << "(A, a, x)\n";
    }

    void operator()(std::integral_constant<states, states::B>,
                    std::integral_constant<events, events::a>,
                    std::integral_constant<inputs, inputs::x>) {
        std::cout << "(B, a, x)\n";
    }

    void operator()(std::integral_constant<states, states::B>,
                    std::integral_constant<events, events::b>,
                    std::integral_constant<inputs, inputs::y>) {
        std::cout << "(B, b, y)\n";
    }
};

int main() {
    auto f = fsm{};
    f.dispatch_event(events::b);
    f.dispatch_event(events::b);
    f.dispatch_event(events::c);
    assert(f.done());

    dispatch_enum(single_arg_dispatch{}, states::A);
    dispatch_enum(single_arg_dispatch{}, states::B);
    dispatch_enum(single_arg_dispatch{}, states::C);

    dispatch_enum(triple_arg_dispatch{}, states::A, events::a, inputs::x);
    dispatch_enum(triple_arg_dispatch{}, states::B, events::a, inputs::x);
    dispatch_enum(triple_arg_dispatch{}, states::B, events::b, inputs::y);
    dispatch_enum(triple_arg_dispatch{}, states::A, events::b,
                  inputs::z);  // invalid

    for (std::uint16_t i = 0; i < 4; i++)
        for (std::uint16_t j = 0; j < 3; j++)
            for (std::uint32_t k = 0; k < 3; k++) {
                auto s = szudzik_pair(i, j, k);
                auto t =
                    szudzik_unpair<std::uint16_t, std::uint16_t, std::uint32_t>(
                        s);
                assert(i == p<0>(t) and j == p<1>(t) and k == p<2>(t));
            }

    return 0;
}
