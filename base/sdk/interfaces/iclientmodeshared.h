#pragma once
// used: angle
#include "../datatypes/qangle.h"

#pragma region clientmode_definitions
#define SIGNONSTATE_NONE		0	// no state yet, about to connect
#define SIGNONSTATE_CHALLENGE	1	// client challenging server, all OOB packets
#define SIGNONSTATE_CONNECTED	2	// client is connected to server, netchans ready
#define SIGNONSTATE_NEW			3	// just got serverinfo and string tables
#define SIGNONSTATE_PRESPAWN	4	// received signon buffers
#define SIGNONSTATE_SPAWN		5	// ready to receive entity packets
#define SIGNONSTATE_FULL		6	// we are fully connected, first non-delta packet received (in-game check)
#define SIGNONSTATE_CHANGELEVEL	7	// server is changing level, please wait
#pragma endregion

class CViewSetup
{
public:
	int			iX;
	int			iUnscaledX;
	int			iY;
	int			iUnscaledY;
	int			iWidth;
	int			iUnscaledWidth;
	int			iHeight;
	int			iUnscaledHeight;
	bool		bOrtho;
	float		flOrthoLeft;
	float		flOrthoTop;
	float		flOrthoRight;
	float		flOrthoBottom;
	std::byte	pad0[0x7C];
	float		flFOV;
	float		flViewModelFOV;
	Vector		vecOrigin;
	QAngle		angView;
	float		flNearZ;
	float		flFarZ;
	float		flNearViewmodelZ;
	float		flFarViewmodelZ;
	float		flAspectRatio;
	float		flNearBlurDepth;
	float		flNearFocusDepth;
	float		flFarFocusDepth;
	float		flFarBlurDepth;
	float		flNearBlurRadius;
	float		flFarBlurRadius;
	float		flDoFQuality;
	int			nMotionBlurMode;
	float		flShutterTime;
	Vector		vecShutterOpenPosition;
	QAngle		vecShutterOpenAngles;
	Vector		vecShutterClosePosition;
	QAngle		vecShutterCloseAngles;
	float		flOffCenterTop;
	float		flOffCenterBottom;
	float		flOffCenterLeft;
	float		flOffCenterRight;
	bool		bOffCenter : 1;
	bool		bRenderToSubrectOfLargerScreen : 1;
	bool		bDoBloomAndToneMapping : 1;
	bool		bDoDepthOfField : 1;
	bool		bHDRTarget : 1;
	bool		bDrawWorldNormal : 1;
	bool		bCullFontFaces : 1;
	bool		bCacheFullSceneState : 1;
	bool		bCSMView : 1;
};

class IHudChat;
class IClientModeShared
{
public:
	std::byte	pad0[0x1B];
	void*		pViewport;
	IHudChat*	pChatElement;
	HCursor		hCursorNone;
	void*		pWeaponSelection;
	int			nRootSize[2];
};

class IAppSystem
{
private:
	virtual void function0() = 0;
	virtual void function1() = 0;
	virtual void function2() = 0;
	virtual void function3() = 0;
	virtual void function4() = 0;
	virtual void function5() = 0;
	virtual void function6() = 0;
	virtual void function7() = 0;
	virtual void function8() = 0;
};

class CEventInfo
{
public:
	uint16_t iClassID;          //0x0000 0 implies not in use
	char pad_0002[ 2 ];          //0x0002
	float flFireDelay;          //0x0004 If non-zero, the delay time when the event should be fired ( fixed up on the client )
	char pad_0008[ 4 ];          //0x0008
	const CBaseClient* pClientClass; //0x000C
	void* pData;               //0x0010 Raw event data
	char pad_0014[ 36 ];         //0x0014
	CEventInfo* next;          //0x0038
	char pad_003C[ 8 ]; // 0x003C
};

class INetChannel;
class IClientState
{
public:
	std::byte		pad0[0x9C];				// 0x0000
	INetChannel*	pNetChannel;			// 0x009C
	int				iChallengeNr;			// 0x00A0
	std::byte		pad1[0x64];				// 0x00A4
	int				iSignonState;			// 0x0108
	std::byte		pad2[0x8];				// 0x010C
	float			flNextCmdTime;			// 0x0114
	int				nServerCount;			// 0x0118
	int				iCurrentSequence;		// 0x011C
	std::byte		pad3[ 0x4C ];			// 0x0120
	int				iServerTick;			// 0x016C
	int				iClientTick;			// 0x0170
	int				iDeltaTick;				// 0x0174
	std::byte		pad4[ 0x4B30 ];			// 0x0179
	float			flFrameTime;			// 0x4CA8
	int				iLastOutgoingCommand;	// 0x4CAC
	int				nChokedCommands;		// 0x4CB0
	int				iLastCommandAck;		// 0x4CB4
	int				iLastServerTick;		// 0x4CB8
	int				iCommandAck;			// 0x4CBC
	int				iSoundSequence;			// 0x4CC0
	std::byte		pad8[ 0x128 ];			// 0x4CC4
	CEventInfo*		pEvents;				// 0x4DEC
}; // Size: 0x4E70
