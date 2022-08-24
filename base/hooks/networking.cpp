#include "../core/hooks.h"
#include "../context.h"
#include "../features/visuals/visuals.h"

void FASTCALL Hooks::hkPacketEnd( void* cl_state, void* EDX ) {
	static auto oPacketEnd = DTR::PacketEnd.GetOriginal<decltype( &Hooks::hkPacketEnd )>( );

	if ( !ctx.m_pLocal
		|| Interfaces::ClientState->iServerTick != Interfaces::ClientState->iDeltaTick )
		return oPacketEnd( cl_state, EDX );

	Features::Visuals.DormantESP.GetActiveSounds( );

	const auto& local_data = ctx.m_cLocalData.at( Interfaces::ClientState->iCommandAck % 150 )	;
	if ( local_data.m_flSpawnTime == ctx.m_pLocal->m_flSpawnTime( )
		&& local_data.m_iShiftAmount > 0
		&& local_data.m_iTickbase > ctx.m_pLocal->m_nTickBase( )
		&& ( local_data.m_iTickbase - ctx.m_pLocal->m_nTickBase( ) ) <= 17 )
		ctx.m_pLocal->m_nTickBase( ) = local_data.m_iTickbase + 1;

	oPacketEnd( cl_state, EDX );
}

void FASTCALL Hooks::hkPacketStart( void* ecx, void* edx, int in_seq, int out_acked ) {
	static auto oPacketStart = DTR::PacketStart.GetOriginal<decltype( &Hooks::hkPacketStart )>( );
	ctx.GetLocal( );
	if ( !ctx.m_pLocal
		|| ctx.m_pLocal->IsDead( ) )
		return oPacketStart( ecx, edx, in_seq, out_acked );

	if ( ctx.m_iSentCmds.empty( )
		|| std::find( ctx.m_iSentCmds.rbegin( ), ctx.m_iSentCmds.rend( ), out_acked ) == ctx.m_iSentCmds.rend( ) )
		return;

	ctx.m_iSentCmds.erase(
		std::remove_if(
			ctx.m_iSentCmds.begin( ), ctx.m_iSentCmds.end( ),
			[ & ]( const int cmd_number ) {
				return cmd_number < out_acked;
			}
		),
		ctx.m_iSentCmds.end( )
				);

	return oPacketStart( ecx, edx, in_seq, out_acked );
}

bool FASTCALL Hooks::hkProcessTempEntities( void* ecx, void* edx, void* msg ) {
	static auto oProcessTempEntities = DTR::ProcessTempEntities.GetOriginal<decltype( &Hooks::hkProcessTempEntities )>( );

	const auto backupMaxClients = Interfaces::ClientState->nMaxClients;

	Interfaces::ClientState->nMaxClients = 1;

	const auto ret = oProcessTempEntities( ecx, edx, msg );

	Interfaces::ClientState->nMaxClients = backupMaxClients;

	/* const auto ret = oProcessTempEntities( ecx, edx, msg );

	ctx.GetLocal( );

	if ( ctx.m_pLocal && !ctx.m_pLocal->IsDead( ) ) {
		for ( auto i = Interfaces::ClientState->pEvents; i; i = Interfaces::ClientState->pEvents->next ) {
			if ( i->iClassID == 0 )
				continue;

			i->flFireDelay = 0.f;
		}
	}*/

	Interfaces::Engine->FireEvents( );

	return ret;
}

bool FASTCALL Hooks::hkSendNetMsg( INetChannel* pNetChan, void* edx, INetMessage& msg, bool bForceReliable, bool bVoice ) {
	static auto oSendNetMsg = DTR::SendNetMsg.GetOriginal<decltype( &Hooks::hkSendNetMsg )>( );

	if ( msg.GetType( ) == 14 ) // Return and don't send messsage if its FileCRCCheck
		return false;

	if ( ctx.m_pLocal && Interfaces::Engine->IsInGame( ) ) {
		if ( msg.GetGroup( ) == 9 ) // Fix lag when transmitting voice and fakelagging
			bVoice = true;

		return oSendNetMsg( pNetChan, edx, msg, bForceReliable, bVoice );
	}

	return oSendNetMsg( pNetChan, edx, msg, bForceReliable, bVoice );
}

void* FASTCALL Hooks::hkAllocKeyValuesMemory( IKeyValuesSystem* thisptr, int edx, int iSize ) {
	static auto oAllocKeyValuesMemory = DTR::AllocKeyValuesMemory.GetOriginal<decltype( &hkAllocKeyValuesMemory )>( );

	if ( const std::uintptr_t uReturnAddress = reinterpret_cast< std::uintptr_t >( _ReturnAddress( ) ); uReturnAddress == Offsets::Sigs.uAllocKeyValuesEngine || uReturnAddress == Offsets::Sigs.uAllocKeyValuesClient )
		return nullptr;

	return oAllocKeyValuesMemory( thisptr, edx, iSize );
}