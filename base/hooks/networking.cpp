#include "../core/hooks.h"
#include "../context.h"
#include "../features/visuals/visuals.h"
#include "../features/rage/exploits.h"

void FASTCALL Hooks::hkPacketEnd( void* cl_state, void* EDX ) {
	static auto oPacketEnd = DTR::PacketEnd.GetOriginal<decltype( &Hooks::hkPacketEnd )>( );

	if ( !ctx.m_pLocal
		|| Interfaces::ClientState->iServerTick != Interfaces::ClientState->iDeltaTick )
		return oPacketEnd( cl_state, EDX );

	Features::Visuals.DormantESP.GetActiveSounds( );

	/*if ( ctx.m_iTicksAllowed >= 14
		&& Config::Get<bool>( Vars.ExploitsDoubletapDefensive )
		&& !Features::Exploits.m_iShiftAmount
		&& ctx.m_pWeapon
		&& !ctx.m_pWeapon->IsGrenade( )
		&& ctx.CalcCorrectionTicks( ) != -1 )
		&& Features::Exploits.m_iRechargeCmd != Interfaces::ClientState->iLastOutgoingCommand ) {
		ctx.m_pLocal->m_nTickBase( ) -= ctx.m_iTicksAllowed;
	}*/

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