#pragma once
#include "../../../core/interfaces.h"

namespace Wrappers::Interface {
	namespace EntityList {
		int GetLocalIndex( ) {
			return Interfaces::Engine->GetLocalPlayer( );
		}

		Wrappers::Entity::CPlayer GetClientEntity( int index ) {
			if ( !Interfaces::Engine->IsInGame( ) )
				return nullptr;

			return Wrappers::Entity::CPlayer( reinterpret_cast< CBasePlayer* >( Interfaces::ClientEntityList->GetClientEntity( index ) ) );
		}

		int GetMaxClients( ) {
			return Interfaces::Globals->nMaxClients;
		}

		int GetHighestEntityIndex( ) {
			return Interfaces::ClientEntityList->GetHighestEntityIndex( );
		}
	}

	namespace Globals {
		float RealTime( ) {
			return Interfaces::Globals->flRealTime;
		}
	}
}