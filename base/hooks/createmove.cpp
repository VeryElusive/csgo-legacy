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
	const auto& netChannel{ Interfaces::ClientState->pNetChannel };

	if ( netChannel ) {
		const auto backupChokedPackets{ netChannel->iChokedPackets };
		netChannel->iChokedPackets = 0;

		netChannel->SendDatagram( 0 );

		--netChannel->iOutSequenceNr;
		netChannel->iChokedPackets = backupChokedPackets;
	}
}

FORCEINLINE void ShouldShift( CUserCmd& cmd ) {

	if ( !ctx.m_pWeapon || !ctx.m_pWeaponData )
		return;

	if ( Features::Exploits.m_iRechargeCmd == Interfaces::ClientState->iLastOutgoingCommand )
		return;

	if ( ctx.m_iTicksAllowed ) {
		if ( ( ( cmd.iButtons & IN_ATTACK || ( cmd.iButtons & IN_ATTACK2 && ctx.m_pWeaponData->nWeaponType == WEAPONTYPE_KNIFE ) ) && ctx.m_bCanShoot /* && ctx.m_iTicksAllowed >= 14*/
			&& ctx.m_pWeaponData->nWeaponType >= WEAPONTYPE_KNIFE && ctx.m_pWeaponData->nWeaponType < WEAPONTYPE_C4 )
			|| ( !ctx.m_bExploitsEnabled || ctx.m_bFakeDucking ) ) {
			const bool isDTEnabled{ ( Config::Get<bool>( Vars.ExploitsDoubletap ) && Config::Get<keybind_t>( Vars.ExploitsDoubletapKey ).enabled ) };

			Features::Exploits.m_bRealCmds = ( !ctx.m_bExploitsEnabled || isDTEnabled || ctx.m_bFakeDucking );
			Features::Exploits.m_iShiftAmount = ctx.m_iTicksAllowed;// Features::Exploits.m_bRealCmds ? ctx.m_iTicksAllowed : 9;

			//**( int** )Offsets::Sigs.numticks += ctx.m_iTicksAllowed; 1 line dt xD
		}
	}
}

void CreateMove( const int nSequenceNumber, const float flInputSampleFrametime, const bool bIsActive ) {
	typedef void( __thiscall* Fn )( void*, const int, const float, const bool );
	static auto oCreateMove = DTR::CreateMoveProxy.GetOriginal<Fn>( );
	oCreateMove( Interfaces::Client, nSequenceNumber, flInputSampleFrametime, bIsActive );

	auto& cmd{ Interfaces::Input->pCommands[ nSequenceNumber % 150 ] };
	auto& verifiedCmd{ Interfaces::Input->pVerifiedCommands[ nSequenceNumber % 150 ] };

	const auto sameFrameCMD{ **( int** )Offsets::Sigs.numticks - **( int** )Offsets::Sigs.host_currentframetick > 0 };

	/*if ( **( int** )Offsets::Sigs.host_currentframetick == 1 ) {
		Interfaces::ClientState->iServerTick += **( int** )Offsets::Sigs.numticks - 1;
		Interfaces::ClientState->iClientTick += **( int** )Offsets::Sigs.numticks - 1;
		Interfaces::Globals->iTickCount += **( int** )Offsets::Sigs.numticks - 1;
	}*/

	ctx.GetLocal( );

	if ( !ctx.m_pLocal ) {
		ctx.m_iLastPeekCmdNum = 0;
		return;
	}

	const auto nci{ Interfaces::Engine->GetNetChannelInfo( ) };
	if ( !nci )
		return;

	if ( Menu::Opened )
		cmd.iButtons &= ~( IN_ATTACK | IN_ATTACK2 );

	cmd.iButtons &= ~( IN_SPEED | IN_WALK );

	ctx.m_flLerpTime = std::max(
		Offsets::Cvars.cl_interp->GetFloat( ),
		Offsets::Cvars.cl_interp_ratio->GetFloat( ) / Offsets::Cvars.cl_updaterate->GetFloat( ) );

	ctx.m_flServerLatency = nci->GetAvgLatency( FLOW_OUTGOING );
	ctx.m_flRealOutLatency = nci->GetLatency( FLOW_OUTGOING );
	ctx.m_flInLatency = nci->GetLatency( FLOW_INCOMING );

	ctx.m_angOriginalViewangles = cmd.viewAngles;

	ctx.m_pWeapon = ctx.m_pLocal->GetWeapon( );
	ctx.m_pWeaponData = ctx.m_pWeapon ? ctx.m_pWeapon->GetCSWeaponData( ) : nullptr;

	if ( Config::Get<bool>( Vars.MiscFakePing ) )
		Features::Misc.UpdateIncomingSequences( Interfaces::ClientState->pNetChannel );
	else
		Features::Misc.ClearIncomingSequences( );

	static bool prev{ };
	auto updateClantag = [ ]( const char* tag ) -> void {
		using Fn = int( __fastcall* )( const char*, const char* );
		static auto fn = reinterpret_cast< Fn >( Offsets::Sigs.ClanTag );

		fn( tag, tag );
	};

	static bool reset{ };
	if ( Config::Get<bool>( Vars.MiscClantag ) ) {
		updateClantag( _( "Wreakin' havoc" ) );

		if ( !reset )
			reset = true;
	}
	else if ( reset ) {
		reset = false;
		updateClantag( _( "" ) );
	}

	if ( ctx.m_pLocal->IsDead( ) ) {
		Interfaces::Input->bCameraInThirdPerson = false;
		ctx.m_iLastPeekCmdNum = 0;
		Features::Exploits.m_iShiftAmount = 0;
		ctx.m_iTicksAllowed = 0;
		return;
	}

	static bool did{ };
	if ( !did )
		did = Features::EnginePrediction.AddToDataMap( );

	static float prevSpawnTime = ctx.m_pLocal->m_flSpawnTime( );

	if ( prevSpawnTime != ctx.m_pLocal->m_flSpawnTime( ) ) {
		//Interfaces::Engine->ClientCmdUnrestricted( _( "cl_fullupdate" ) );
		//ctx.m_pLocal->m_pAnimState( )->Reset( );
		ctx.m_cLocalData = { };

		ctx.m_iLastShotNumber = 0;

		for ( int i{ }; i < 13; ++i ) {
			auto& layer{ ctx.m_pAnimationLayers[i] };
			auto& layer2{ ctx.m_pLocal->m_AnimationLayers( )[i] };
			layer.flCycle = layer.flWeight = layer.flPlaybackRate = layer.nSequence =
				layer2.flCycle = layer2.flWeight = layer2.flPlaybackRate = layer2.nSequence = 0;
		}

		//ctx.m_cLocalAnimData = { };

		Features::EnginePrediction.m_cCompressionVars = { };

		prevSpawnTime = ctx.m_pLocal->m_flSpawnTime( );
	}

	ctx.m_bSendPacket = false;

	if ( Features::Exploits.ShouldRecharge( ) ) {
		cmd.iTickCount = INT_MAX;
		verifiedCmd.userCmd = cmd;
		verifiedCmd.uHashCRC = cmd.GetChecksum( );

		ctx.m_bSendPacket = false;
		KeepCommunication( );
		return;
	}

	ctx.m_bInCreatemove = true;

	if ( ( ctx.m_iTicksAllowed >= 14
		&& Config::Get<bool>( Vars.ExploitsDoubletapDefensive )
		&& ( Config::Get<bool>( Vars.ExploitsDoubletap ) && Config::Get<keybind_t>( Vars.ExploitsDoubletapKey ).enabled ) )
		// fakelag
		|| ( Config::Get<bool>( Vars.AntiaimFakeLagInPeek ) && !ctx.m_iTicksAllowed
			&& cmd.iCommandNumber - ctx.m_iLastPeekCmdNum >= 14 ) ) {
		ctx.m_bInPeek = Features::Misc.InPeek( Interfaces::Input->pCommands[ Interfaces::ClientState->iLastOutgoingCommand % 150 ] );

		if ( ctx.m_bInPeek ) {
			//if ( ctx.m_iTicksAllowed >= 14 )
			//	ctx.m_bInPeek = Features::Misc.IsDefensivePositionHittable( );

			ctx.m_iLastPeekCmdNum = cmd.iCommandNumber;
		}
	}
	else
		ctx.m_bInPeek = false;

	ctx.m_bCanPenetrate = Features::Autowall.CanPenetrate( );

	auto& localData{ ctx.m_cLocalData.at( cmd.iCommandNumber % 150 ) };
	localData.Save( ctx.m_pLocal, cmd, ctx.m_pWeapon );

	// get some sleep and tidy this shit up later
	const auto extraTicks{ **( int** )Offsets::Sigs.numticks - **( int** )Offsets::Sigs.host_currentframetick };

	const auto runningDefensive{ ctx.m_iTicksAllowed >= 14
		&& Config::Get<bool>( Vars.ExploitsDoubletapDefensive )
		&& ( Config::Get<bool>( Vars.ExploitsDoubletap ) && Config::Get<keybind_t>( Vars.ExploitsDoubletapKey ).enabled ) };

	const auto newCmdsWithDefensive{ std::min( Interfaces::ClientState->nChokedCommands + 1 + extraTicks + ctx.m_iTicksAllowed, 16 ) };
	const auto newCmdsReal{ std::min( Interfaces::ClientState->nChokedCommands + 1 + extraTicks, 16 ) };

	const auto tbDefensive{ Features::Exploits.AdjustTickbase( newCmdsWithDefensive ) + Interfaces::ClientState->nChokedCommands };
	const auto tbReal{ Features::Exploits.AdjustTickbase( newCmdsReal ) + Interfaces::ClientState->nChokedCommands };

	const auto backupSFD{ ctx.m_bSafeFromDefensive };

	if ( runningDefensive ) {
		// breaks when we shoot but whatever
		if ( tbDefensive + 2 > ctx.m_iHighestTickbase )// + 1 but 2 for case of 1 tickbase off
			ctx.m_bSafeFromDefensive = false;
		else
			ctx.m_bSafeFromDefensive = true;
	}
	else
		ctx.m_bSafeFromDefensive = false;

	//if ( ctx.m_bSafeFromDefensive )
	//	ctx.m_bSafeFromDefensive = Features::Misc.IsDefensivePositionHittable( );

	const auto actualShiftTicks{ ( ctx.m_pWeaponData && ctx.m_pWeaponData->nWeaponType >= WEAPONTYPE_KNIFE && ctx.m_pWeaponData->nWeaponType < WEAPONTYPE_C4 )
		? ctx.m_iTicksAllowed : 0 };

	const auto newCmds{ std::min( Interfaces::ClientState->nChokedCommands + 1 + extraTicks + actualShiftTicks, 16 ) };

	const auto realTickbase{ ( ctx.m_bSafeFromDefensive && ctx.m_bInPeek ) && runningDefensive
		? tbDefensive : ( Features::Exploits.AdjustTickbase( newCmds ) + Interfaces::ClientState->nChokedCommands ) };

	ctx.m_flFixedCurtime = TICKS_TO_TIME( realTickbase );

	// pred gets fucked bc of restoring entity, which happens when we call cl_readpackets
	Interfaces::Prediction->Update( Interfaces::ClientState->iDeltaTick, 
		Interfaces::ClientState->iDeltaTick > 0, 
		Interfaces::ClientState->iLastCommandAck,
		Interfaces::ClientState->iLastOutgoingCommand + Interfaces::ClientState->nChokedCommands );

	ctx.m_bCanShoot = ctx.m_pLocal->CanShoot( );

	Features::Antiaim.FakeLag( cmd.iCommandNumber );
	Features::Misc.Movement( cmd );

	// doing this before now
	localData.SavePredVars( ctx.m_pLocal, cmd );

	Features::EnginePrediction.RunCommand( cmd );
	{
		Interfaces::Globals->flCurTime = ctx.m_flFixedCurtime;

		ctx.m_vecEyePos = ctx.m_pLocal->GetEyePosition( ctx.m_angOriginalViewangles.y, ctx.m_angOriginalViewangles.x );

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

		//localData.m_bRevolverCock = !ctx.m_bRevolverCanShoot;

		Features::Ragebot.Main( cmd, !sameFrameCMD );

		Features::Misc.AutoPeek( cmd );
	}
	Features::EnginePrediction.Finish( );

	if ( sameFrameCMD && ctx.m_pWeapon && !ctx.m_pWeapon->IsGrenade( ) )
		cmd.iButtons &= ~IN_ATTACK;

	if ( Interfaces::ClientState->nChokedCommands >= 15 - ctx.m_iTicksAllowed )
		ctx.m_bSendPacket = true;

	if ( !ctx.m_bFakeDucking && cmd.iButtons & IN_ATTACK && ctx.m_pWeapon && !ctx.m_pWeapon->IsGrenade( ) && ctx.m_bCanShoot )
		ctx.m_bSendPacket = true;

	if ( ( cmd.iButtons & IN_ATTACK || ( cmd.iButtons & IN_ATTACK2 && ctx.m_pWeaponData->nWeaponType == WEAPONTYPE_KNIFE ) )
		&& ctx.m_bCanShoot ) {
		ctx.m_iLastShotNumber = cmd.iCommandNumber;
		ctx.m_iLastStopTime = Interfaces::Globals->flRealTime;
	}


	ShouldShift( cmd );

	if ( sameFrameCMD )
		ctx.m_bSendPacket = false;

	/*if ( cmd.iCommandNumber % 2 && ctx.m_iTicksAllowed ) {
		Features::Exploits.m_iShiftAmount = ctx.m_iTicksAllowed;
		Features::Exploits.m_bRealCmds = false;
		//localData.PredictedNetvars.m_nTickBase = tbDefensive;
	}*/

	localData.m_bCanAA = Features::Antiaim.Pitch( cmd );
	ctx.m_flFixedCurtime = TICKS_TO_TIME( runningDefensive && ctx.m_bInPeek && ctx.m_bSafeFromDefensive ? tbDefensive : tbReal );
	Features::Antiaim.RunLocalModifications( cmd, ctx.m_bSendPacket );

	const auto resetTHISTick{ Features::Exploits.m_bResetNextTick };
	if ( ctx.m_bSendPacket ) {
		if ( Features::Exploits.m_iRechargeCmd == Interfaces::ClientState->iLastOutgoingCommand ) {
			int ranCommands{ };
			for ( auto i{ 1 }; i <= Interfaces::ClientState->nChokedCommands; ++i ) {
				const auto j{ ( Interfaces::ClientState->iLastOutgoingCommand + i ) % 150 };

				auto& curUserCmd{ Interfaces::Input->pCommands[ j ] };
				if ( curUserCmd.iTickCount != INT_MAX )
					++ranCommands;
			}
			localData.m_bOverrideTickbase = true;

			// we still charge with choked commands if it is less than 15 - commands to charge so sometimes itll be off by 1
			localData.m_iAdjustedTickbase = Features::Exploits.AdjustTickbase( Interfaces::ClientState->nChokedCommands + 1 ) + ranCommands;

			Features::Exploits.m_bResetNextTick = true;
			Features::Exploits.m_bWasDefensiveTick = false;
		}
		else {
			if ( ctx.m_iTicksAllowed >= 14
				&& Config::Get<bool>( Vars.ExploitsDoubletapDefensive )
				&& ( Config::Get<bool>( Vars.ExploitsDoubletap ) && Config::Get<keybind_t>( Vars.ExploitsDoubletapKey ).enabled )
				&& ( !Features::Exploits.m_iShiftAmount || ( Features::Misc.AutoPeeking && ctx.m_bSafeFromDefensive && ctx.m_bInPeek && Config::Get<bool>( Vars.ExploitsDoubletapExtended ) ) ) ) {

				if ( ctx.m_bInPeek && ctx.m_bSafeFromDefensive ) {
					if ( !Features::Exploits.m_bWasDefensiveTick )
						localData.m_bOverrideTickbase = true;

					Features::Exploits.m_bWasDefensiveTick = true;
				}
				else {
					//Features::Visuals.Chams.AddHitmatrix( ctx.m_pLocal, ctx.m_matRealLocalBones );
					Features::Exploits.m_bAlreadyPeeked = true;

					if ( Features::Exploits.m_bWasDefensiveTick ) {
						localData.m_bOverrideTickbase = true;
						localData.m_iAdjustedTickbase = tbReal;
					}

					Features::Exploits.m_bWasDefensiveTick = false;
				}
			}
			else {
				if ( Features::Exploits.m_bWasDefensiveTick ) {
					localData.m_iAdjustedTickbase = tbReal;
					localData.m_bOverrideTickbase = true;
				}

				Features::Exploits.m_bWasDefensiveTick = false;
			}
		}
	}
	else {
		ctx.m_bSafeFromDefensive = backupSFD;
		KeepCommunication( );
	}

	if ( resetTHISTick ) {
		localData.m_bOverrideTickbase = true;
		localData.m_iAdjustedTickbase = Features::Exploits.m_bWasDefensiveTick ? tbDefensive : tbReal;
		Features::Exploits.m_bResetNextTick = false;
	}

	/*if ( ctx.m_iTicksAllowed ) {
		if ( !ctx.m_pLocal->m_vecVelocity( ).Length( ) && ctx.m_bSafeFromDefensive ) {
			Features::Exploits.m_bRealCmds = false;
			Features::Exploits.m_iShiftAmount = ctx.m_iTicksAllowed;
			cmd.viewAngles.y += 180.f;
		}
	}*/

	verifiedCmd.userCmd = cmd;
	verifiedCmd.uHashCRC = cmd.GetChecksum( );
	ctx.m_bInCreatemove = false;
}

void __declspec( naked ) __fastcall Hooks::hkCreateMoveProxy( uint8_t* ecx, uint8_t*, int sequenceNumber, float inputSampleFrametime, bool active ) {
	__asm {
		push ebp
		mov ebp, esp

		push eax
		lea eax, [ ecx ]
		pop eax

		mov ctx.m_bSendPacket, bl

		push ebx
		push esi
		push edi
	}

	CreateMove( sequenceNumber, inputSampleFrametime, active );

	__asm {
		pop edi
		pop esi
		pop ebx

		mov bl, ctx.m_bSendPacket

		mov esp, ebp
		pop ebp

		ret 0xC
	}
}