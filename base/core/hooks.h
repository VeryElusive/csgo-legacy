#pragma once
// used: winapi, directx, fmt includes
#include "../havoc.h"
// used: hook setup/destroy
#include "../utils/detourhook.h"
// used: recvprop hook setup/destroy, recvproxydata
#include "prop_manager.h"
// used: baseclasses
#include "interfaces.h"

#define FASTCALL __fastcall
#define STDCALL __stdcall

/*
* VTABLE INDEXES
* functions indexes in their virtual tables
*/
namespace VTABLE
{
	enum
	{
		/* directx table */
		RESET = 16,
		PRESENT = 17,
		ENDSCENE = 42,
		RESETEX = 132,

		/* keyvaluessystem table */
		ALLOCKEYVALUESMEMORY = 2,

		/* client table */
		CREATEMOVE = 21,
		FRAMESTAGENOTIFY = 36,
		WRITEUSERCMDDELTATOBUFFER = 23,

		/* panel table */
		PAINTTRAVERSE = 41,

		/* clientmode table */
		OVERRIDEVIEW = 18,
		OVERRIDEMOUSEINPUT = 23,
		GETVIEWMODELFOV = 35,
		DOPOSTSCREENEFFECTS = 44,

		/* modelrender table */
		DRAWMODELEXECUTE = 21,

		/* studiorender table */
		DRAWMODEL = 29,

		/* enginevgui table*/
		VGUI_PAINT = 14,

		/* viewrender table */
		RENDERSMOKEOVERLAY = 41,

		/* engine table */
		ISCONNECTED = 27,
		ISPAUSED = 90,
		ISHLTV = 93,
		GETSCREENASPECTRATIO = 101,

		/* bsp query table */
		LISTLEAVESINBOX = 6,

		/* prediction table */
		RUNCOMMAND = 19,
		INPREDICTION = 14,

		/* steamgamecoordinator table */
		SENDMESSAGE = 0,
		RETRIEVEMESSAGE = 2,

		/* sound table */
		EMITSOUND = 5,

		/* materialsystem table */
		OVERRIDECONFIG = 21,

		/* renderview table */
		SCENEEND = 9,

		/* surface table */
		LOCKCURSOR = 67,
		PLAYSOUND = 82,

		/* gameevent table */
		FIREEVENT = 9,

		/* convar table */
		GETBOOL = 13,

		/* netchannel table */
		SENDNETMSG = 40,
		SENDDATAGRAM = 46,

		/* gamemovement table*/
		PROCESSMOVEMENT = 1
	};
}

/*
 * DETOURS
 * detour hook managers
 */
namespace DTR
{
	inline CDetourHook Reset;
	inline CDetourHook EndScene;
	inline CDetourHook Present;
	inline CDetourHook CreateMoveProxy;
	inline CDetourHook FrameStageNotify;
	inline CDetourHook GetScreenAspectRatio;
	inline CDetourHook IsPaused;
	inline CDetourHook IsHLTV;
	inline CDetourHook OverrideView;
	inline CDetourHook OverrideConfig;
	inline CDetourHook SendNetMsg;
	inline CDetourHook SendDatagram;
	inline CDetourHook GetViewModelFOV;
	inline CDetourHook DoPostScreenEffects;
	inline CDetourHook IsConnected;
	inline CDetourHook RenderSmokeOverlay;
	inline CDetourHook ListLeavesInBox;
	inline CDetourHook PaintTraverse;
	inline CDetourHook DrawModel;
	inline CDetourHook RunCommand;
	inline CDetourHook SendMessageGC;
	inline CDetourHook RetrieveMessage;
	inline CDetourHook LockCursor;
	inline CDetourHook PlaySoundSurface;
	inline CDetourHook SvCheatsGetBool;
	inline CDetourHook PacketEnd;
	inline CDetourHook PacketStart;
	inline CDetourHook ProcessTempEntities;
	inline CDetourHook Paint;
	inline CDetourHook EmitSound;
	inline CDetourHook ProcessMovement;
	inline CDetourHook DoExtraBonesProcessing;
	inline CDetourHook StandardBlendingRules;
	inline CDetourHook UpdateClientsideAnimation;
	inline CDetourHook AccumulateLayers;
	inline CDetourHook PhysicsSimulate;
	inline CDetourHook ModifyEyePosition;
	inline CDetourHook CalcView;
	inline CDetourHook CalcViewmodelBob;
	inline CDetourHook ShouldSkipAnimFrame;
	inline CDetourHook Setupbones;
	inline CDetourHook CL_SendMove;
	inline CDetourHook CMCreateMove;
	inline CDetourHook WriteUserCmdDeltaToBuffer;
	inline CDetourHook ShouldInterpolate;
	inline CDetourHook AddBoxOverlay;
	inline CDetourHook GlowEffectSpectator;
	inline CDetourHook GetColorModulation;
	inline CDetourHook GetAlphaModulation;
	inline CDetourHook OnLatchInterpolatedVariables;
	inline CDetourHook OnNewCollisionBounds;
	inline CDetourHook CHudScopePaint;
	inline CDetourHook BuildTransformations;
	inline CDetourHook UpdatePostProcessingEffects;
	inline CDetourHook FinishTrackPredictionErrors;
	inline CDetourHook GetUserCmd;
	inline CDetourHook SelectItem;
	inline CDetourHook PreThink;
	inline CDetourHook ItemPostFrame;
	inline CDetourHook MoveParentPhysicsSimulate;

#ifdef SERVER_DBGING
	inline CDetourHook ServerSetupBones;
#endif
}

/*
 * HOOKS
 * swap functions with given pointers
 */
namespace Hooks
{
	inline WNDPROC pOldWndProc = nullptr;
	inline HWND hWindow = nullptr;
	inline RecvVarProxyFn m_bClientSideAnimation;
	inline RecvVarProxyFn m_flSimulationTime;
	inline RecvVarProxyFn m_flAbsYaw;

	// Get
	bool	Setup();
	void	Restore();

	// hooks
	void	FASTCALL	hkLockCursor( ISurface* thisptr, int edx );
	LRESULT	CALLBACK	hkWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	void	FASTCALL	hkPaintTraverse( ISurface* thisptr, int edx, unsigned int uPanel, bool bForceRepaint, bool bForce );
	void	FASTCALL	HkPaint( const std::uintptr_t ecx, const std::uintptr_t edx, const int mode );
	void	FASTCALL	hkCreateMoveProxy( IBaseClientDll* thisptr, int edx, int nSequenceNumber, float flInputSampleFrametime, bool bIsActive );
	void	FASTCALL	hkPacketEnd( void* cl_state, void* EDX );
	void	FASTCALL	hkPacketStart( void* ecx, void* edx, int in_seq, int out_acked );
	bool	FASTCALL	hkProcessTempEntities( void* ecx, void* EDX, void* msg );
	void	FASTCALL	hkDrawModel( IStudioRender* thisptr, int edx, DrawModelResults_t* pResults, const DrawModelInfo_t& info, matrix3x4_t* pBoneToWorld, float* flFlexWeights, float* flFlexDelayedWeights, const Vector& vecModelOrigin, int nFlags );
	void	FASTCALL	hkFrameStageNotify( IBaseClientDll* thisptr, int edx, EClientFrameStage stage );
	void	FASTCALL	hkOverrideView( IClientModeShared* thisptr, int edx, CViewSetup* pSetup );
	bool	FASTCALL	hkOverrideConfig( IMaterialSystem* ecx, void* edx, MaterialSystemConfig_t& config, bool bForceUpdate );
	int		FASTCALL	hkListLeavesInBox( void* thisptr, int edx, const Vector& vecMins, const Vector& vecMaxs, unsigned short* puList, int nListMax );
	float	FASTCALL	hkGetScreenAspectRatio( void* ECX, void* EDX, int32_t iWidth, int32_t iHeight );
	bool	FASTCALL	hkIsPaused( void* ecx, void* edx );
	bool	FASTCALL	hkIsHltv( void* ecx, void* EDX );
	int		FASTCALL	hkEmitSound( void* _this, int edx, IRecipientFilter& filter, int iEntIndex, int iChannel, const char* pSoundEntry, unsigned int nSoundEntryHash, const char* pSample, float flVolume, int nSeed, float flAttenuation, int iFlags, int iPitch, const Vector* pOrigin, const Vector* pDirection, void* pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity, int unk );
	bool	FASTCALL	hkSendNetMsg( INetChannel* pNetChan, void* edx, INetMessage& msg, bool bForceReliable, bool bVoice );
	int		FASTCALL	hkSendDatagram( INetChannel* thisptr, int edx, bf_write* pDatagram );
	void	FASTCALL	hkProcessMovement( void* ecx, DWORD edx, CBasePlayer* basePlayer, CMoveData* moveData );
	void**	STDCALL		hkFinishTrackPredictionErrors( CBasePlayer* basePlayer );
	bool	FASTCALL	hkSvCheatsGetBool( CConVar* thisptr, int edx );
	void	FASTCALL	hkDoExtraBonesProcessing( void* ecx, uint32_t ye, CStudioHdr* hdr, Vector* pos, Quaternion* q, const matrix3x4_t& matrix, uint8_t* bone_computed, void* context );
	void	FASTCALL	hkStandardBlendingRules( CBasePlayer* const ent, const std::uintptr_t edx, CStudioHdr* const mdl_data, int a1, int a2, float a3, int mask );
	void	FASTCALL	hkUpdateClientsideAnimation( CBasePlayer* ecx, void* edx );
	bool	FASTCALL	hkShouldSkipAnimFrame( void* ecx, uint32_t ebx );
	QAngle* FASTCALL	hkGetEyeAngles( CBasePlayer* ecx, void* edx );
	void	FASTCALL	hkAccumulateLayers( CBasePlayer* const ecx, const std::uintptr_t edx, int a0, int a1, float a2, int a3 );
	void	FASTCALL	hkPhysicsSimulate( CBasePlayer* player, int time );
	CUserCmd* FASTCALL	hkGetUserCmd( uint8_t* ecx, uint8_t* edx, int slot, int seqnr );
	void	FASTCALL	hkModifyEyePosition( CCSGOPlayerAnimState* ecx, void* edx, Vector& pos );
	void	FASTCALL	hkCalcView( CBasePlayer* pPlayer, void* edx, Vector& vecEyeOrigin, QAngle& angEyeAngles, float& flZNear, float& flZFar, float& flFov );
	float	FASTCALL	hkCalcViewmodelBob( CWeaponCSBase* pWeapon, void* EDX );
	bool	FASTCALL	hkSetupbones( const std::uintptr_t ecx, const std::uintptr_t edx, matrix3x4_t* const bones, int max_bones, int mask, float time );
	bool	STDCALL		hkCMCreateMove( float input_sample_frametime, CUserCmd* cmd );
	bool	FASTCALL	hkWriteUserCmdDeltaToBuffer( void* ecx, void* edx, int slot, bf_write* buf, int from, int to, bool is_new_command );
	bool	FASTCALL	hkShouldInterpolate( CBasePlayer* ecx, const std::uintptr_t edx );
	int		FASTCALL	hkDoPostScreenEffects( IClientModeShared* thisptr, int edx, CViewSetup* pSetup );
	void	FASTCALL	hkAddBoxOverlay( void* ecx, void* edx, const Vector& origin, const Vector& mins, const Vector& max, QAngle const& orientation, int r, int g, int b, int a, float duration );
	bool	CDECL		hkGlowEffectSpectator( CBasePlayer* const player, CBasePlayer* const local, int& style, Vector& clr, float& alpha_from, float& alpha_to, float& time_from, float& time_to, bool& animate );
	void	FASTCALL	hkGetColorModulation( IMaterial* const ecx, const std::uintptr_t edx, float* const r, float* const g, float* const b );
	float	FASTCALL	hkGetAlphaModulation( IMaterial* ecx, uint32_t ebx );
	void	FASTCALL	hkOnLatchInterpolatedVariables( CBasePlayer* const ecx, const std::uintptr_t edx, const int flags );
	void	FASTCALL	hkCHudScopePaint( void* ecx, uint32_t edx );
	void	FASTCALL	hkOnNewCollisionBounds( CBasePlayer* ecx, uint32_t edx, Vector* oldMins, Vector* newMins, Vector* oldMaxs, Vector* new_Maxs );
	bool	FASTCALL	hkNET_ProcessSocket( void* ecx, void* handler );
	void	FASTCALL	hkBuildTransformations( CBasePlayer* ecx, int edx, CStudioHdr* hdr, Vector* pos, Quaternion* q, int cameraTransform, int boneMask, int boneComputed );
	void	FASTCALL	hkUpdatePostProcessingEffects( void* ecx, int edx );
	void	FASTCALL	hkSelectItem( void* ecx, int edx, int a2 );
	bool	FASTCALL	hkPreThink( void* ecx, int edx, int a2 );
	bool	FASTCALL	hkItemPostFrame( void* ecx, int edx );
	bool	FASTCALL	hkMoveParentPhysicsSimulate( void* ecx, int edx );
	void	FASTCALL	hkRunCommand( void* ecx, void* edx, CBasePlayer* player, CUserCmd* ucmd, IMoveHelper* moveHelper );

	void	CDECL		m_bClientSideAnimationHook( CRecvProxyData* data, void* entity, void* output );
	void	CDECL		m_flSimulationTimeHook( CRecvProxyData* data, void* entity, void* output );


#ifdef SERVER_DBGING
	void FASTCALL hkServerSetupBones( CBaseAnimating* ecx, int edx, matrix3x4a_t* pBoneToWorld, int boneMask );
#endif
}