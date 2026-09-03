#include <boost/mp11.hpp>
#include <cassert>  // assert macro
#include <cmath>
#include <concepts>
#include <limits>
#include <magic_enum.hpp>

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
    using type = std::make_unsigned_t<
        decltype((std::declval<T1>() + std::declval<T1>()) +
                 (std::declval<T2>() + std::declval<T2>()))>;
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

// need for constexpr
template <std::unsigned_integral T>
constexpr T isqrt(T x) {
    T out{};
    for (; (out + 1) * (out + 1) <= x; out++);
    return out;
}

template <std::unsigned_integral T, std::unsigned_integral S,
          std::same_as<unsigned_arithmetic_result_t<T, S>> I>
constexpr std::pair<T, S> szudzik_unpair(I x) {
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
               ? std::pair{static_cast<T>(x - isqrt(x) * isqrt(x)),
                           static_cast<S>(isqrt(x))}
               : std::pair{
                     static_cast<T>(isqrt(x)),
                     static_cast<S>((x - isqrt(x) * isqrt(x)) - isqrt(x))};
}

template <typename T, typename S> /* requires is_integral_constant<T> and
                                     is_integral_constant<S> */
using mp_szudzik_pair =
    std::integral_constant<decltype(szudzik_pair(T::value, S::value)),
                           szudzik_pair(T::value, S::value)>;

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

template <typename StateT, typename InputT, typename F>
    requires std::is_enum_v<StateT> and std::is_enum_v<InputT>
constexpr auto dispatch_enum(F&& f, StateT state, InputT input) {
    using to_integral_values =
        boost::mp11::mp_compose<mp_enum_to_enum_values,
                                mp_enum_values_to_underlying>;

    using integral_states = to_integral_values::fn<StateT>;
    using integral_inputs = to_integral_values::fn<InputT>;

    using encoded_pairs =
        boost::mp11::mp_product<mp_szudzik_pair, integral_states,
                                integral_inputs>;

    using max_code =
        boost::mp11::mp_max_element<encoded_pairs, boost::mp11::mp_less>;

    using encoded_type = max_code::value_type;

    encoded_type encoded =
        szudzik_pair(static_cast<std::underlying_type_t<StateT>>(state),
                     static_cast<std::underlying_type_t<InputT>>(input));

    boost::mp11::mp_with_index<max_code>(encoded, [&](auto i) {
        constexpr auto decoded_pair =
            szudzik_unpair<std::underlying_type_t<StateT>,
                           std::underlying_type_t<InputT>>(
                static_cast<encoded_type>(decltype(i)::value));
        constexpr std::integral_constant<StateT, StateT{decoded_pair.first}>
            state{};
        constexpr std::integral_constant<InputT, InputT{decoded_pair.second}>
            input{};
        std::forward<decltype(f)>(f)(state, input);
    });
}

// example of alternative to magic_enum (definitions)

template <typename E, E... values>
    requires std::is_enum<E>::value
struct enum_value_sequence {
    using type = boost::mp11::mp_list<std::integral_constant<E, values>...>;
};

template <typename>
struct is_enum_value_sequence_ : std::false_type {};

template <typename E, E... values>
struct is_enum_value_sequence_<enum_value_sequence<E, values...>>
    : std::true_type {};

template <typename T>
concept is_enum_value_sequence = is_enum_value_sequence_<T>::value;

template <typename>
struct attach_enum_value_sequence;

template <typename T>
concept has_attached_enum_value_sequence =
    std::is_enum<T>::value and requires() {
        typename attach_enum_value_sequence<T>::type;
        typename attach_enum_value_sequence<T>::underlying_type;
    };

// start of state machine example

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

enum class states : std::uint16_t { A, B, C, D };
enum class inputs : std::uint32_t { a, b, c };

// example of alternative to magic_enum (specializations)

template <>
struct attach_enum_value_sequence<states> {
    using underlying_type = std::underlying_type_t<states>;
    using type =
        enum_value_sequence<states, states::A, states::B, states::C, states::D>;
};

template <>
struct attach_enum_value_sequence<inputs> {
    using underlying_type = std::underlying_type_t<inputs>;
    using type = enum_value_sequence<inputs, inputs::a, inputs::b, inputs::c>;
};

struct fsm {
    void operator()(auto, auto) {
        throw std::domain_error{"invalid transition"};
    }

    void operator()(std::integral_constant<states, states::A>,
                    std::integral_constant<inputs, inputs::a>) {
        s = states::A;
        std::cout << "(A, a) -> A\n";
    }

    void operator()(std::integral_constant<states, states::A>,
                    std::integral_constant<inputs, inputs::b>) {
        s = states::B;
        std::cout << "(A, b) -> B\n";
    }

    void operator()(std::integral_constant<states, states::A>,
                    std::integral_constant<inputs, inputs::c>) {
        s = states::C;
        std::cout << "(A, c) -> C\n";
    }

    void operator()(std::integral_constant<states, states::B>,
                    std::integral_constant<inputs, inputs::a>) {
        s = states::B;
        std::cout << "(B, a) -> B\n";
    }

    void operator()(std::integral_constant<states, states::B>,
                    std::integral_constant<inputs, inputs::b>) {
        s = states::C;
        std::cout << "(B, b) -> C\n";
    }

    void operator()(std::integral_constant<states, states::B>,
                    std::integral_constant<inputs, inputs::c>) {
        s = states::D;
        std::cout << "(B, c) -> D\n";
    }

    void operator()(std::integral_constant<states, states::C>,
                    std::integral_constant<inputs, inputs::a>) {
        s = states::C;
        std::cout << "(C, a) -> C\n";
    }

    void operator()(std::integral_constant<states, states::C>,
                    std::integral_constant<inputs, inputs::b>) {
        s = states::B;
        std::cout << "(C, b) -> B\n";
    }

    void operator()(std::integral_constant<states, states::C>,
                    std::integral_constant<inputs, inputs::c>) {
        s = states::D;
        std::cout << "(C, c) -> D\n";
    }

    [[nodiscard]] constexpr bool done() const { return s == states::D; }

    constexpr void dispatch_input(inputs e) { dispatch_enum(*this, s, e); };
    states s{states::A};
};

int main() {
    auto f = fsm{};
    f.dispatch_input(inputs::b);
    f.dispatch_input(inputs::b);
    f.dispatch_input(inputs::c);
    assert(f.done());
    return 0;
}
