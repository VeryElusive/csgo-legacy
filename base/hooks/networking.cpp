#include "../core/hooks.h"
#include "../context.h"
#include "../features/visuals/visuals.h"
#include "../features/rage/exploits.h"

void FASTCALL Hooks::hkPacketEnd( void* ecx, void* edx ) {
	static auto oPacketEnd = DTR::PacketEnd.GetOriginal<decltype( &Hooks::hkPacketEnd )>( );

	oPacketEnd( ecx, edx );

	Features::Visuals.DormantESP.GetActiveSounds( );
}

void FASTCALL Hooks::hkPacketStart( void* ecx, void* edx, int in_seq, int out_acked ) {
	static auto oPacketStart = DTR::PacketStart.GetOriginal<decltype( &Hooks::hkPacketStart )>( );
	
	if ( !ctx.m_pLocal
		|| ctx.m_pLocal->IsDead( ) )
		return oPacketStart( ecx, edx, in_seq, out_acked );

	/*auto& localData{ ctx.m_cLocalData.at( Interfaces::ClientState->iLastCommandAck % 150 ) };

	if ( localData.m_flSpawnTime == ctx.m_pLocal->m_flSpawnTime( )
		&& !localData.m_iShiftAmount
		&& !localData.m_bOverrideTickbase
		&& localData.m_iAdjustedTickbase ) {
		localData.m_iAdjustedTickbase = 0;
	}*/

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

	const auto ret = oProcessTempEntities( ecx, edx, msg );
	
	if ( ctx.m_pLocal && !ctx.m_pLocal->IsDead( ) ) {
		for ( auto i = Interfaces::ClientState->pEvents; i; i = Interfaces::ClientState->pEvents->next ) {
			if ( i->iClassID == 0 )
				continue;

			i->flFireDelay = 0.f;
		}
	}

	Interfaces::Engine->FireEvents( );

	return ret;
}

bool FASTCALL Hooks::hkSendNetMsg( INetChannel* pNetChan, void* edx, INetMessage& msg, bool bForceReliable, bool bVoice ) {
	static auto oSendNetMsg = DTR::SendNetMsg.GetOriginal<decltype( &Hooks::hkSendNetMsg )>( );

	if ( msg.GetType( ) == 14 ) // Return and don't send messsage if its FileCRCCheck
		return false;

	if ( ctx.m_pLocal ) {
		if ( msg.GetGroup( ) == 9 ) // Fix lag when transmitting voice and fakelagging
			bVoice = true;

		return oSendNetMsg( pNetChan, edx, msg, bForceReliable, bVoice );
	}

	return oSendNetMsg( pNetChan, edx, msg, bForceReliable, bVoice );
}

void* FASTCALL Hooks::hkAllocKeyValuesMemory( IKeyValuesSystem* thisptr, int edx, int iSize ) {
	static auto oAllocKeyValuesMemory = DTR::AllocKeyValuesMemory.GetOriginal<decltype( &hkAllocKeyValuesMemory )>( );

	if ( const std::uintptr_t uReturnAddress = reinterpret_cast< std::uintptr_t >( _ReturnAddress( ) ); 
		uReturnAddress == Offsets::Sigs.uAllocKeyValuesEngine || uReturnAddress == Offsets::Sigs.uAllocKeyValuesClient )
		return nullptr;

	return oAllocKeyValuesMemory( thisptr, edx, iSize );
}


int FASTCALL Hooks::hkSendDatagram( INetChannel* thisptr, int edx, bf_write* pDatagram ) {
	static auto oSendDatagram = DTR::SendDatagram.GetOriginal<decltype( &hkSendDatagram )>( );

	INetChannelInfo* pNetChannelInfo = Interfaces::Engine->GetNetChannelInfo( );

	if ( !ctx.m_pLocal || pDatagram || !pNetChannelInfo || !Config::Get<bool>( Vars.MiscFakePing ) || ctx.m_pLocal->IsDead( ) )
		return oSendDatagram( thisptr, edx, pDatagram );

	const int iOldInReliableState = thisptr->iInReliableState;
	const int iOldInSequenceNr = thisptr->iInSequenceNr;

	// calculate max available fake latency with our real ping to keep it w/o real lags or delays
	const float flMaxLatency = std::max( 0.f, std::clamp( 0.5f, 0.f, Offsets::Cvars.sv_maxunlag->GetFloat( ) ) - pNetChannelInfo->GetLatency( FLOW_OUTGOING ) );
	Features::Misc.AddLatencyToNetChannel( thisptr, flMaxLatency );

	const int iReturn = oSendDatagram( thisptr, edx, pDatagram );

	thisptr->iInReliableState = iOldInReliableState;
	thisptr->iInSequenceNr = iOldInSequenceNr;

	return iReturn;
}