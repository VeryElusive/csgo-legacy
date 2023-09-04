#pragma once
#include <unordered_map>
#include "../../dependencies/LUA/lua/lua.hpp"
#include "../../dependencies/LUA/sol/sol.hpp"
#include "../../features/misc/logger.h"

struct LuaState_t {
	sol::state m_cState;
	std::unordered_map<uint32_t, sol::protected_function> m_cCallbacks;
};

namespace Scripting {
	void Unload( std::string name );
	void AddCallback( std::string name, sol::protected_function callback, sol::this_state state );
	void Load( std::string fileName );

	void Log( std::string text, bool onscreen );

	inline bool m_bInCallback{ };
	inline bool m_bLoadingScript{ };
	inline std::unordered_map<std::string, std::shared_ptr<LuaState_t>> m_mapStates;

	template< typename ...Ts>
	inline void DoCallback( uint32_t hash, Ts... args ) {
		if ( m_bLoadingScript )
			return;

		m_bInCallback = true;

		auto temp = m_mapStates;

		for ( auto& states : temp ) {
			if ( !states.first.empty( ) && states.second != nullptr ) {
				for ( auto callbacks : states.second->m_cCallbacks ) {
					if ( callbacks.first != hash )
						continue;

					sol::safe_function_result result = callbacks.second.call( args... );

					if ( result.valid( ) )
						continue;

					sol::error err = result;
					Features::Logger.Log( std::string( err.what( ) ) + _( "lua" ), true );

					Unload( states.first );
				}
			}
		}

		m_bInCallback = false;
	}
}