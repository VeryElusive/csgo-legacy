#pragma once
#include "../../../sdk/interfaces/igameeventmanager.h"

namespace Wrappers::Events {
	class CEvent {
	public:
		int GetInt( const char* bruh ) {
			if ( !m_pEvent )
				return 0;

			return m_pEvent->GetInt( bruh );
		}

		bool GetBool( const char* bruh ) {
			if ( !m_pEvent )
				return 0;

			return m_pEvent->GetBool( bruh );
		}

		float GetFloat( const char* bruh ) {
			if ( !m_pEvent )
				return 0;

			return m_pEvent->GetFloat( bruh );
		}

		std::string GetString( const char* bruh ) {
			if ( !m_pEvent )
				return "";

			return static_cast< std::string >( m_pEvent->GetString( bruh ) );
		}

		uint64_t GeUint64( const char* bruh ) {
			if ( !m_pEvent )
				return 0;

			return m_pEvent->GetUint64( bruh );
		}

		IGameEvent* m_pEvent{ };
	};
}