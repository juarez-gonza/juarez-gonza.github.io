template <typename StateT, typename InputT, typename F>
    requires std::is_enum_v<StateT> and std::is_enum_v<InputT>
constexpr auto dispatch_enum(F&& f, StateT state, InputT input)
    -> decltype(std::forward<F>(f)(enum_values<StateT{}, InputT{}>{})) {
    // Alias to a metafunction for steps 1.1 and 1.2
    using to_integral_values =
        boost::mp11::mp_compose<mp_enum_to_enum_values,
                                mp_enum_values_to_underlying>;

    // Steps 1.1 and 1.2
    using integral_states = to_integral_values::fn<StateT>;
    using integral_inputs = to_integral_values::fn<InputT>;

    // Steps 1.3 and 1.4 with metafunction for encoding `e` as
    // mp_e_???.
    using encoded_pairs =
        boost::mp11::mp_product<mp_e_???, integral_states,
                                integral_inputs>;

    // Helper for step 1.5
    using max_code =
        boost::mp11::mp_max_element<encoded_pairs, boost::mp11::mp_less>;

    using encoded_type = max_code::value_type;

    // Step 2.1 with runtime encoding `e` as e_???
    encoded_type encoded =
        e_???(static_cast<std::underlying_type_t<StateT>>(state),
                     static_cast<std::underlying_type_t<InputT>>(input));

    // Step 1.5 at compile time with mp_with_index
    // Step 2.2 at runtime
    boost::mp11::mp_with_index<max_code>(encoded, [&](auto i) {
        // Step 1.6 with compile time inverse of encoding `e` as
        // e_???_inv
        constexpr auto decoded_pair =
	  e_???_inv<std::underlying_type_t<StateT>,
		    std::underlying_type_t<InputT>>(static_cast<encoded_type>(decltype(i)::value));
        std::forward<decltype(f)>(f)(
            enum_values<StateT{decoded_pair.first},
                        InputT{decoded_pair.second}>{});
    });
}
