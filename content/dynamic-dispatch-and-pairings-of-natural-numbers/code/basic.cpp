#include <boost/mp11.hpp>
#include <cassert>  // assert macro
#include <cmath>
#include <concepts>
#include <limits>
#include <magic_enum.hpp>

/////////////////////////////////////
////////// SZUDZIK PAIRING //////////
/////////////////////////////////////

// Szudzik's Elegant Pairing Function
// http://szudzik.com/ElegantPairing.pdf
template <std::integral T, std::integral S>
constexpr std::common_type_t<T, S> szudzik_pair(T x, S y) {
    return y > x ? (y * y + x) : (x * x + x + y);
}

// need for constexpr
template <std::integral T>
constexpr T isqrt(T x) {
    T out{};
    for (; (out + 1) * (out + 1) <= x; out++);
    return out;
}

template <std::integral T, std::integral S, std::integral I>
constexpr std::pair<T, S> szudzik_unpair(I x) {
    // cannot use variables for common subexpressions due to constexpr
    // constraints:
    //
    // q = isqrt(x)
    // l = x - isqrt(x) * isqrt(x)
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
    // 1. get the types associated to states and inputs (this requires
    // an existing table that maps each entry in the enum to a type
    // representation)
    //
    // 2. map each state to a different integer
    //
    // 3. map each input to a different integer
    //
    // 4. get the cartesian product of these integers
    //
    // 5. map each pair to a single integer using some pairing
    // function (the number of cases depends on the distribution of
    // the range of the pairing function
    // https://en.wikipedia.org/wiki/Pairing_function . Szudik's
    // pairing has a rather compact distribution)
    //
    // 6. map the (possibly runtime) values `state` and `input` to an
    // integer using the pairing function (now possibly executing at
    // runtime)
    //
    // 7. invoke mp_with_index on a function that converts the runtime
    // value into a compile time constant (here is where the switch is
    // hidden)
    //
    // 8. use the inverse of the pairing function on the integer
    // number to get the pair of integers
    //
    // 9. call the argument function with the specific types as
    // arguments

    using to_integral_values =
        boost::mp11::mp_compose<mp_enum_to_enum_values,
                                mp_enum_values_to_underlying>;

    using integral_states = to_integral_values::fn<StateT>;
    using integral_inputs = to_integral_values::fn<InputT>;

    using encoded_pairs =
        boost::mp11::mp_product<mp_szudzik_pair, integral_states,
                                integral_inputs>;

    using max_encoding =
        boost::mp11::mp_max_element<encoded_pairs, boost::mp11::mp_less>;

    auto encoded =
        szudzik_pair(static_cast<std::underlying_type_t<StateT>>(state),
                     static_cast<std::underlying_type_t<InputT>>(input));

    // Here encoded may have been turned into an int due to common_type, but we
    // know we are operating with unsigned numbers (problem restriction)
    boost::mp11::mp_with_index<
        max_encoding>(static_cast<std::size_t>(encoded), [&](auto i) {
        constexpr auto decoded_pair =
            szudzik_unpair<std::underlying_type_t<StateT>,
                           std::underlying_type_t<InputT>>(decltype(i)::value);
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
enum class inputs : std::uint8_t { a, b, c };

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
