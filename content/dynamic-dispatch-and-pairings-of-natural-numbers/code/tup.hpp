#include <utility>

template <std::size_t MaxCount, typename Is, typename... Ts>
struct tup_impl;

template <>
struct tup_impl<0, std::index_sequence<>> {
    static constexpr std::size_t size = 0;
};

template <std::size_t MaxCount, std::size_t I, typename T>
struct tup_impl<MaxCount, std::index_sequence<I>, T> {
    static constexpr std::size_t size = MaxCount;
    T x;
    constexpr T get(
        std::integral_constant<std::size_t, MaxCount - I - 1>) const {
        return x;
    }
};

template <std::size_t MaxCount, std::size_t I, std::size_t... Is, typename T,
          typename... Ts>
struct tup_impl<MaxCount, std::index_sequence<I, Is...>, T, Ts...>
    : tup_impl<MaxCount, std::index_sequence<Is...>, Ts...> {
    T x;
    using tup_impl<MaxCount, std::index_sequence<Is...>, Ts...>::get;
    constexpr T get(
        std::integral_constant<std::size_t, MaxCount - I - 1>) const {
        return x;
    }
};

template <typename... Ts>
struct tup
    : tup_impl<sizeof...(Ts), std::make_index_sequence<sizeof...(Ts)>, Ts...> {
};

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

static_assert(tup_cat(tup{1}).size == tup{1}.size);
static_assert(p<0>(tup_cat(tup{1})) == p<0>(tup{1}));
static_assert(p<0>(tup{1} + tup{}) == 1);
static_assert((tup{1} + tup{}).size == 1);
static_assert(p<0>(tup{} + tup{1}) == 1);
static_assert((tup{} + tup{1}).size == 1);
static_assert(p<0>(tup{1} + tup{2}) == 1);
static_assert(p<1>(tup{1} + tup{2}) == 2);
static_assert((tup{1} + tup{2}).size == 2);
static_assert(p<0>(tup{1} + tup{2} + tup{3}) == 1);
static_assert(p<1>(tup{1} + tup{2} + tup{3}) == 2);
static_assert(p<2>(tup{1} + tup{2} + tup{3}) == 3);
static_assert((tup{1} + tup{2} + tup{3}).size == 3);
