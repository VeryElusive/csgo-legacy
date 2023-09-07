#pragma once
#include "../../../core/interfaces.h"

namespace Wrappers::Interface {
	namespace EntityList {
		int GetLocalIndex( ) {
			return Interfaces::Engine->GetLocalPlayer( );
		}

		IClientEntity* GetClientEntity( int index ) {
			return Interfaces::ClientEntityList->GetClientEntity( index );
		}
	}
}