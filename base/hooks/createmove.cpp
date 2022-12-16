#include "../core/hooks.h"
#include "../context.h"
#include "../features/rage/autowall.h"
#include "../features/rage/antiaim.h"
#include "../features/rage/ragebot.h"
#include "../features/rage/exploits.h"
#include "../features/misc/engine_prediction.h"
#include "../features/misc/misc.h"
#include "../features/visuals/visuals.h"

FORCEINLINE void KeepCommunication( ) {
	auto& NetChannel = Interfaces::ClientState->pNetChannel;

	if ( NetChannel ) {
		const auto backupChokedPackets{ NetChannel->iChokedPackets };
		const auto backupOutSequenceNr{ NetChannel->iOutSequenceNr };

		NetChannel->iChokedPackets = 0;
		NetChannel->SendDatagram( 0 );

		NetChannel->iOutSequenceNr = backupOutSequenceNr;
		NetChannel->iChokedPackets = backupChokedPackets;
	}
}

FORCEINLINE void ShouldShift( CUserCmd& cmd ) {
	Features::Exploits.m_iShiftAmount = 0;

	if ( !ctx.m_pWeapon )
		return;

	if ( ctx.m_iTicksAllowed ) {
		if ( ( cmd.iButtons & IN_ATTACK && ctx.m_bCanShoot /* && ctx.m_iTicksAllowed >= 14*/
			&& ctx.m_pWeaponData->nWeaponType > WEAPONTYPE_KNIFE && ctx.m_pWeaponData->nWeaponType < WEAPONTYPE_C4 )
			|| ( !ctx.m_bExploitsEnabled || ctx.m_bFakeDucking ) ) {
			const bool isDTEnabled{ ( Config::Get<bool>( Vars.ExploitsDoubletap ) && Config::Get<keybind_t>( Vars.ExploitsDoubletapKey ).enabled ) };

			Features::Exploits.m_bRealCmds = ( !ctx.m_bExploitsEnabled || isDTEnabled || ctx.m_bFakeDucking );
			Features::Exploits.m_iShiftAmount = Features::Exploits.m_bRealCmds ? ctx.m_iTicksAllowed : 9;

			//**( int** )Offsets::Sigs.numticks += ctx.m_iTicksAllowed; 1 line dt xD
		}
	}
}

static void STDCALL CreateMove( int nSequenceNumber, float flInputSampleFrametime, bool bIsActive, bool& bSendPacket )
{
	static auto oCreateMove = DTR::CreateMoveProxy.GetOriginal<decltype( &Hooks::hkCreateMoveProxy )>( );
	oCreateMove( Interfaces::Client, 0, nSequenceNumber, flInputSampleFrametime, bIsActive );

	ctx.GetLocal( );

	bSendPacket = true;
	ctx.m_bSendPacket = false;

	auto& cmd{ Interfaces::Input->pCommands[ nSequenceNumber % 150 ] };
	auto& verifiedCmd{ Interfaces::Input->pVerifiedCommands[ nSequenceNumber % 150 ] };

	if ( !cmd.iCommandNumber || !ctx.m_pLocal ) {
		Features::Exploits.m_iShiftAmount = 0;
		ctx.m_bFilledAnims = false;
		return;
	}

	const auto nci{ Interfaces::Engine->GetNetChannelInfo( ) };
	if ( !nci )
		return;

	if ( !nci->IsLoopback( ) && **( int** )Offsets::Sigs.host_currentframetick <= 1 ) {
		const auto backupTickbase{ ctx.m_pLocal->m_nTickBase( ) };

		typedef bool( FASTCALL* hkNET_ProcessSocketFn )( int, void* );
		reinterpret_cast< hkNET_ProcessSocketFn >( Offsets::Sigs.NET_ProcessSocket )( 0, Interfaces::ClientState + 4 );

		if ( !ctx.m_pLocal )
			return;

		ctx.m_pLocal->m_nTickBase( ) = backupTickbase;
	}

	ctx.m_flLerpTime = std::max(
		Offsets::Cvars.cl_interp->GetFloat( ),
		Offsets::Cvars.cl_interp_ratio->GetFloat( ) / Offsets::Cvars.cl_updaterate->GetFloat( ) );

	// should i do fake ping again

	ctx.m_flRealOutLatency = nci->GetLatency( FLOW_OUTGOING );
	ctx.m_flInLatency = nci->GetLatency( FLOW_INCOMING );

	ctx.m_angOriginalViewangles = cmd.viewAngles;

	ctx.m_pWeapon = ctx.m_pLocal->GetWeapon( );
	ctx.m_pWeaponData = ctx.m_pWeapon ? ctx.m_pWeapon->GetCSWeaponData( ) : nullptr;

	if ( Config::Get<bool>( Vars.MiscFakePing ) )
		Features::Misc.UpdateIncomingSequences( Interfaces::ClientState->pNetChannel );
	else
		Features::Misc.ClearIncomingSequences( );

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
			ctx.m_iLastShotNumber = 0;
			ctx.m_iLastShotTime = 0.f;
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

	auto& localData{ ctx.m_cLocalData.at( cmd.iCommandNumber % 150 ) };
	localData.Save( ctx.m_pLocal, cmd, ctx.m_pWeapon );

	if ( Features::Exploits.ShouldRecharge( ) ) {
		bSendPacket = false;
		cmd.iTickCount = 0xFADED;
		cmd.iButtons &= ~( IN_ATTACK | IN_ATTACK2 );
		KeepCommunication( );

		cmd.bHasBeenPredicted = true;

		verifiedCmd.userCmd = cmd;
		verifiedCmd.uHashCRC = cmd.GetChecksum( );

		localData.SavePredVars( ctx.m_pLocal, cmd );
		return;
	}

	Features::EnginePrediction.PreStart( );

	ctx.m_bCanPenetrate = Features::Autowall.CanPenetrate( );
	ctx.m_bCanShoot = ctx.m_pLocal->CanShoot( );

	Features::Antiaim.FakeLag( );
	Features::Misc.Movement( cmd );

	Features::EnginePrediction.RunCommand( cmd );
	{
		ctx.m_vecEyePos = ctx.m_pLocal->GetEyePosition( 0 );

		ctx.m_bRevolverCanShoot = false;
		if ( ctx.m_pWeapon 
			&& ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_REVOLVER ) {
			static float m_flPostPoneFireReadyTime{ };

			if ( ctx.m_bRevolverCanCock && !( cmd.iButtons & IN_RELOAD ) ) {
				if ( Interfaces::Globals->flCurTime < m_flPostPoneFireReadyTime )
					cmd.iButtons |= IN_ATTACK;
				else if ( Interfaces::Globals->flCurTime < ctx.m_pWeapon->m_flNextSecondaryAttack( ) )
					cmd.iButtons |= IN_ATTACK2;
				else
					m_flPostPoneFireReadyTime = Interfaces::Globals->flCurTime + 0.234375f;

				ctx.m_bRevolverCanShoot = Interfaces::Globals->flCurTime > m_flPostPoneFireReadyTime;
			}
			else {
				cmd.iButtons &= ~IN_ATTACK;
				m_flPostPoneFireReadyTime = Interfaces::Globals->flCurTime + 0.234375f;
			}
		}

		localData.m_bRevolverCock = !ctx.m_bRevolverCanShoot;

		Features::Ragebot.Main( cmd );

		Features::Misc.AutoPeek( cmd );

		Features::Antiaim.Pitch( cmd );
		Features::Antiaim.Yaw( cmd, ctx.m_bSendPacket );

		Features::Misc.MoveMINTFix( cmd, ctx.m_angOriginalViewangles, ctx.m_pLocal->m_fFlags( ), ctx.m_pLocal->m_MoveType( ) );

		localData.SavePredVars( ctx.m_pLocal, cmd );
	}
	Features::EnginePrediction.Finish( );

	const auto extraTicks{ **( int** )Offsets::Sigs.numticks - 1 };

	if ( Interfaces::ClientState->nChokedCommands + extraTicks >= 15 - ctx.m_iTicksAllowed )
		ctx.m_bSendPacket = true;

	if ( cmd.iButtons & IN_ATTACK && ctx.m_pWeapon && !ctx.m_pWeapon->IsGrenade( ) && ctx.m_bCanShoot )
		ctx.m_bSendPacket = false;

	if ( cmd.iButtons & IN_ATTACK && ctx.m_bCanShoot ) {
		ctx.m_iLastShotNumber = cmd.iCommandNumber;
		ctx.m_iLastShotTime = Interfaces::Globals->flRealTime;
	}

	ShouldShift( cmd );

	bSendPacket = ctx.m_bSendPacket;

	if ( bSendPacket ) {
		if ( !Features::Exploits.m_iShiftAmount
			|| !Features::Exploits.m_bRealCmds )
		ctx.m_iSentCmds.push_back( cmd.iCommandNumber );
		Features::Exploits.m_bWasDefensiveTick = false;

		static int timer{ };

		if ( Features::Exploits.m_iRechargeCmd == Interfaces::ClientState->iLastOutgoingCommand ) {
			localData.m_bOverrideTickbase = true;

			localData.m_iAdjustedTickbase = Features::Exploits.AdjustTickbase( Interfaces::ClientState->nChokedCommands + 1, 1, -Interfaces::ClientState->nChokedCommands );

			timer = 0;
		}
		else {
			if ( ctx.m_iTicksAllowed >= 14
				&& Config::Get<bool>( Vars.ExploitsDoubletapDefensive )
				&& !Features::Exploits.m_iShiftAmount
				&& ctx.m_pWeapon
				&& !ctx.m_pWeapon->IsGrenade( ) ) {

				++timer;
				timer = std::min( timer, 15 );
				if ( timer >= ctx.m_iTicksAllowed ) {
					Features::Exploits.m_bAlreadyPeeked = true;
					timer = 0;
				}
				else
					Features::Exploits.m_bWasDefensiveTick = true;

				if ( timer > 2 && timer < 12 ) {
					cmd.viewAngles.y += 180.f;
				}
			}
			else
				timer = 0;

			ctx.m_iLastSentCmdNumber = cmd.iCommandNumber;
		}
	}
	else
		KeepCommunication( );

	Features::AnimSys.UpdateLocalFull( cmd, ctx.m_bSendPacket );

	localData.m_angViewAngles = cmd.viewAngles;

	if ( Interfaces::ClientState->pNetChannel ) {
		// TODO: LEGACY!
		//if ( !DTR::SendNetMsg.IsHooked( ) )
		//	DTR::SendNetMsg.Create( MEM::GetVFunc( Interfaces::ClientState->pNetChannel, VTABLE::SENDNETMSG ), &Hooks::hkSendNetMsg );

		//if ( !DTR::SendDatagram.IsHooked( ) )
		//	DTR::SendDatagram.Create( MEM::GetVFunc( Interfaces::ClientState->pNetChannel, VTABLE::SENDDATAGRAM ), &Hooks::hkSendDatagram );
	}

	verifiedCmd.userCmd = cmd;
	verifiedCmd.uHashCRC = cmd.GetChecksum( );
}

__declspec( naked ) void FASTCALL Hooks::hkCreateMoveProxy( [[maybe_unused]] IBaseClientDll* thisptr, [[maybe_unused]] int edx, [[maybe_unused]] int nSequenceNumber, [[maybe_unused]] float flInputSampleFrametime, [[maybe_unused]] bool bIsActive )
{
	__asm
	{
		push	ebp
		mov		ebp, esp; // store the stack
		push	bl; // bSendPacket
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