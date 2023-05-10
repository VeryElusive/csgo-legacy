/*
* created by slazyy on 15.10.2020
*/

#pragma once

namespace sdk::detail {
	template < typename _fn_t, typename... _args_t > requires std::is_invocable_v< _fn_t, _args_t... >
	using invoke_t = std::invoke_result_t< std::decay_t< _fn_t >, std::decay_t< _args_t >... >;
}