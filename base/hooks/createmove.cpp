#include "../core/hooks.h"
#include "../context.h"
#include "../features/rage/autowall.h"
#include "../features/rage/antiaim.h"
#include "../features/rage/ragebot.h"
#include "../features/rage/exploits.h"
#include "../features/misc/engine_prediction.h"
#include "../features/misc/misc.h"

void KeepCommunication( ) {
	const auto NetChannel = Interfaces::ClientState->pNetChannel;

	if ( NetChannel && !ctx.m_pLocal->IsDead( ) ) {
		const auto backup_choked_packets = NetChannel->iChokedPackets;

		NetChannel->iChokedPackets = 0;
		NetChannel->SendDatagram( 0 );
		--NetChannel->iOutSequenceNr;

		NetChannel->iChokedPackets = backup_choked_packets;
	}
}

FORCEINLINE void ShouldShift( CUserCmd& cmd ) {
	Features::Exploits.m_iShiftAmount = 0;

	if ( !ctx.m_pWeapon )
		return;

	if ( ctx.m_iTicksAllowed ) {
		if ( ( cmd.iButtons & IN_ATTACK && ctx.m_bCanShoot && ctx.m_iTicksAllowed >= 14
			&& ctx.m_pWeaponData->nWeaponType > WEAPONTYPE_KNIFE && ctx.m_pWeaponData->nWeaponType < WEAPONTYPE_C4 )
			|| ( !ctx.m_bExploitsEnabled || ctx.m_bFakeDucking ) ) {
			const bool isDTEnabled{ ( Config::Get<bool>( Vars.ExploitsDoubletap ) && Config::Get<keybind_t>( Vars.ExploitsDoubletapKey ).enabled ) };

			Features::Exploits.m_bRealCmds = ( !ctx.m_bExploitsEnabled || isDTEnabled || ctx.m_bFakeDucking );
			Features::Exploits.m_iShiftAmount = ctx.m_iTicksAllowed;// everyone does 9 but it actually does not matter at all as long as it's above 6 u all g
		}
	}
}

static void STDCALL CreateMove( int nSequenceNumber, float flInputSampleFrametime, bool bIsActive, bool& bSendPacket )
{
	static auto oCreateMove = DTR::CreateMoveProxy.GetOriginal<decltype( &Hooks::hkCreateMoveProxy )>( );
	oCreateMove( Interfaces::Client, 0, nSequenceNumber, flInputSampleFrametime, bIsActive );

	ctx.GetLocal( );

	bSendPacket = ctx.m_bSendPacket = true;

	auto& cmd{ Interfaces::Input->pCommands[ nSequenceNumber % 150 ] };
	auto& VerifiedCmd{ Interfaces::Input->pVerifiedCommands[ nSequenceNumber % 150 ] };

	if ( !cmd.iCommandNumber || !ctx.m_pLocal ) {
		Features::Exploits.m_iShiftAmount = 0;
		ctx.m_bFilledAnims = false;
		return;
	}

	ctx.m_angOriginalViewangles = cmd.viewAngles;

	ctx.m_pWeapon = ctx.m_pLocal->GetWeapon( );
	ctx.m_pWeaponData = ctx.m_pWeapon ? ctx.m_pWeapon->GetCSWeaponData( ) : nullptr;

	const auto nci = Interfaces::Engine->GetNetChannelInfo( );
	if ( !nci )
		return;

	{
		ctx.m_flLerpTime = std::max(
			Offsets::Cvars.cl_interp->GetFloat( ),
			Offsets::Cvars.cl_interp_ratio->GetFloat( ) / Offsets::Cvars.cl_updaterate->GetFloat( ) );

		ctx.m_flOutLatency = nci->GetLatency( FLOW_OUTGOING );
		ctx.m_flInLatency = nci->GetLatency( FLOW_INCOMING );
	}

	if ( ctx.m_pLocal->IsDead( ) ) {
		Features::Exploits.m_iShiftAmount = 0;
		ctx.m_bFilledAnims = false;
		return;
	}

	{
		static float prevSpawnTime = ctx.m_pLocal->m_flSpawnTime( );
		auto updateClantag = [ ]( const char* tag ) -> void {
			using Fn = int( __fastcall* )( const char*, const char* );
			static auto fn = reinterpret_cast< Fn >( Offsets::Sigs.ClanTag );

			fn( tag, tag );
		};

		static bool prev{ };

		if ( !Config::Get<bool>( Vars.MiscClantag ) ) {
			if ( prev ) {
				updateClantag( _( "" ) );
				prev = false;
			}
		}
		else if ( !prev ) {
			updateClantag( _( "Wreakin' havoc" ) );
			prev = true;
		}
		else if ( prevSpawnTime != ctx.m_pLocal->m_flSpawnTime( ) ) {
			ctx.m_cLocalData = { };
			
			ctx.m_iTicksAllowed = 0;
			ctx.m_bFilledAnims = false;

			//ctx.m_cLocalAnimData = { };

			for ( auto& compressionVar : Features::EnginePrediction.m_cCompressionVars )
				compressionVar = { };

			if ( !Config::Get<bool>( Vars.MiscClantag ) )
				updateClantag( _( "" ) );
			else
				updateClantag( _( "Wreakin' havoc" ) );

			prevSpawnTime = ctx.m_pLocal->m_flSpawnTime( );
		}
	}
	
	// arm jitter bug caused by this ?
	
	auto& LocalData{ ctx.m_cLocalData.at( cmd.iCommandNumber % 150 ) };
	LocalData.Save( ctx.m_pLocal, cmd, ctx.m_pWeapon );

	if ( Features::Exploits.ShouldRecharge( ) ) {
		bSendPacket = false;
		cmd.iTickCount = INT_MAX;
		cmd.iButtons &= ~( IN_ATTACK | IN_ATTACK2 );
		KeepCommunication( );

		VerifiedCmd.userCmd = cmd;
		VerifiedCmd.uHashCRC = cmd.GetChecksum( );

		LocalData.SavePredVars( ctx.m_pLocal, cmd );
		//LocalData.StoreAnims( ctx.m_pLocal );
		return;
	}


	ctx.m_bCanPenetrate = Features::Autowall.CanPenetrate( );
	ctx.m_bCanShoot = ctx.m_pLocal->CanShoot( );

	Features::EnginePrediction.PreStart( );

	Features::Antiaim.FakeLag( );
	Features::Misc.Movement( cmd );

	Features::EnginePrediction.RunCommand( cmd );
	{
		{
			const auto backupPoseParam = ctx.m_pLocal->m_flPoseParameter( ).at( 12u );

			ctx.m_pLocal->m_flPoseParameter( ).at( 12u ) = ( std::clamp( std::remainder(
				0
				- ctx.m_pLocal->m_aimPunchAngle( ).x * Offsets::Cvars.weapon_recoil_scale->GetFloat( ), 
				360.f
			), -90.f, 90.f ) + 90.f ) / 180.f;

			ctx.m_vecEyePos = ctx.m_pLocal->GetEyePosition( );

			ctx.m_pLocal->m_flPoseParameter( ).at( 12u ) = backupPoseParam;
		}

		Features::Ragebot.Main( cmd );

		if ( cmd.iButtons & IN_ATTACK )
			cmd.viewAngles -= ctx.m_pLocal->m_aimPunchAngle( ) * Offsets::Cvars.weapon_recoil_scale->GetFloat( );

		Features::Misc.AutoPeek( cmd );

		Features::Antiaim.Pitch( cmd );

		Features::Misc.MoveMINTFix( cmd, ctx.m_angOriginalViewangles, ctx.m_pLocal->m_fFlags( ), ctx.m_pLocal->m_MoveType( ) );
	}
	Features::EnginePrediction.Finish( );

	LocalData.SavePredVars( ctx.m_pLocal, cmd );

	if ( Interfaces::ClientState->nChokedCommands >= 15 - ctx.m_iTicksAllowed )
		ctx.m_bSendPacket = true;

	if ( !ctx.m_bFakeDucking && cmd.iButtons & IN_ATTACK && ctx.m_pWeapon && !ctx.m_pWeapon->IsGrenade( ) && ctx.m_bCanShoot )
		ctx.m_bSendPacket = true;

	if ( cmd.iButtons & IN_ATTACK && ctx.m_bCanShoot ) {
		ctx.m_iLastShotNumber = cmd.iCommandNumber;
		ctx.m_iLastShotTime = Interfaces::Globals->flRealTime;
	}


	ShouldShift( cmd );

	Features::Antiaim.RunLocalModifications( cmd, ctx.m_bSendPacket );

	bSendPacket = ctx.m_bSendPacket;

	if ( bSendPacket ) {
		if ( !Features::Exploits.m_iShiftAmount
			|| !Features::Exploits.m_bRealCmds )
		ctx.m_iSentCmds.push_back( cmd.iCommandNumber );

		if ( Features::Exploits.m_iRechargeCmd == Interfaces::ClientState->iLastOutgoingCommand ) {
			LocalData.m_bOverrideTickbase = true;

			LocalData.m_iAdjustedTickbase = Features::Exploits.AdjustTickbase(
				Interfaces::ClientState->nChokedCommands + 1, 1, -Interfaces::ClientState->nChokedCommands
			);
		}
		else
			ctx.m_iLastSentCmdNumber = cmd.iCommandNumber;
	}
	else
		KeepCommunication( );

	LocalData.m_angViewAngles = cmd.viewAngles;

	if ( Interfaces::ClientState->pNetChannel ) {
		if ( !DTR::SendNetMsg.IsHooked( ) )
			DTR::SendNetMsg.Create( MEM::GetVFunc( Interfaces::ClientState->pNetChannel, VTABLE::SENDNETMSG ), &Hooks::hkSendNetMsg );
	}

	VerifiedCmd.userCmd = cmd;
	VerifiedCmd.uHashCRC = cmd.GetChecksum( );
}

void CDECL Hooks::hkCL_Move( float accumulated_extra_samples, bool bFinalTick ) {
	static auto oCL_Move = DTR::CL_Move.GetOriginal<decltype( &hkCL_Move )>( );

	ctx.GetLocal( );

	if ( !ctx.m_pLocal || ctx.m_pLocal->IsDead( ) )
		return oCL_Move( accumulated_extra_samples, bFinalTick );

	//readpacket fix
	if ( const auto net{ Interfaces::Engine->GetNetChannelInfo( ) }; !net || !net->IsLoopback( ) ) {
		const auto backupClockCorrection{ Offsets::Cvars.cl_clock_correction->GetBool( ) };
		Offsets::Cvars.cl_clock_correction->SetValue( 0 );

		const auto backupTickbase{ ctx.m_pLocal->m_nTickBase( ) };
		const auto backupFrametime{ Interfaces::Globals->flFrameTime };
		const auto backupCurtime{ Interfaces::Globals->flCurTime };
		const auto backupTickcount{ Interfaces::Globals->iTickCount };

		const auto backupCSFrametime{ Interfaces::ClientState->flFrameTime };
		//const auto backupChokedCommands{ Interfaces::ClientState->nChokedCommands };
		//const auto backupServerTick{ Interfaces::ClientState->iServerTick };
		const auto backupClientTick{ Interfaces::ClientState->iClientTick };

		//const auto backupChokedPackets{ Interfaces::ClientState->pNetChannel->iChokedPackets };
		//const auto backupOutSeqNum{ Interfaces::ClientState->pNetChannel->iOutSequenceNr };

		typedef void( __fastcall* CL_ReadPacketsFn )( bool );
		( ( CL_ReadPacketsFn )Offsets::Sigs.CL_ReadPackets )( bFinalTick );

		ctx.m_pLocal->m_nTickBase( ) = backupTickbase;

		Offsets::Cvars.cl_clock_correction->SetValue( backupClockCorrection );

		Interfaces::Globals->flFrameTime = backupFrametime;
		Interfaces::Globals->flCurTime = backupCurtime;
		Interfaces::Globals->iTickCount = backupTickcount;

		Interfaces::ClientState->flFrameTime = backupCSFrametime;
		//Interfaces::ClientState->nChokedCommands = backupChokedCommands;
		//Interfaces::ClientState->iServerTick = backupServerTick;
		Interfaces::ClientState->iClientTick = backupClientTick;

		//Interfaces::ClientState->pNetChannel->iChokedPackets = backupChokedPackets;
		//Interfaces::ClientState->pNetChannel->iOutSequenceNr = backupOutSeqNum;
	}

	oCL_Move( accumulated_extra_samples, bFinalTick );

}

__declspec( naked ) void FASTCALL Hooks::hkCreateMoveProxy( [[maybe_unused]] IBaseClientDll* thisptr, [[maybe_unused]] int edx, [[maybe_unused]] int nSequenceNumber, [[maybe_unused]] float flInputSampleFrametime, [[maybe_unused]] bool bIsActive )
{
	__asm
	{
		push	ebp
		mov		ebp, esp; // store the stack
		push	ebx; // bSendPacket
		push	esp; // restore the stack
		push	dword ptr[ bIsActive ]; // ebp + 16
		push	dword ptr[ flInputSampleFrametime ]; // ebp + 12
		push	dword ptr[ nSequenceNumber ]; // ebp + 8
		call	CreateMove
			pop		ebx
			pop		ebp
			retn	0Ch
	}
}