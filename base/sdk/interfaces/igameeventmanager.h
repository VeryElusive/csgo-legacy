#pragma once
// @credits: https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/public/igameevents.h

// used: keyvalues class
#include "../datatypes/keyvalues.h"

#define EVENT_DEBUG_ID_INIT			42
#define EVENT_DEBUG_ID_SHUTDOWN		13

class IGameEvent
{
public:
	virtual					~IGameEvent( ) { }
	virtual const char* GetName( ) const = 0;

	virtual bool			IsReliable( ) const = 0;
	virtual bool			IsLocal( ) const = 0;
	virtual bool			IsEmpty( const char* szKeyName = nullptr ) = 0;

	virtual bool			GetBool( const char* szKeyName = nullptr, bool bDefault = false ) = 0;
	virtual int				GetInt( const char* szKeyName = nullptr, int iDefault = 0 ) = 0;
	virtual std::uint64_t	GetUint64( const char* szKeyName = nullptr, std::uint64_t uDefault = 0 ) = 0;
	virtual float			GetFloat( const char* szKeyName = nullptr, float flDefault = 0.f ) = 0;
	virtual const char* GetString( const char* szKeyName = nullptr, const char* szDefault = "" ) = 0;
	virtual const wchar_t* GetWString( const char* szKeyName = nullptr, const wchar_t* szDefault = L"" ) = 0;
	virtual const void* GetPtr( const char* szKeyName = nullptr ) const = 0;

	virtual void			SetBool( const char* szKeyName, bool bValue ) = 0;
	virtual void			SetInt( const char* szKeyName, int iValue ) = 0;
	virtual void			SetUint64( const char* szKeyName, std::uint64_t uValue ) = 0;
	virtual void			SetFloat( const char* szKeyName, float flValue ) = 0;
	virtual void			SetString( const char* szKeyName, const char* szValue ) = 0;
	virtual void			SetWString( const char* szKeyName, const wchar_t* szValue ) = 0;
	virtual void			SetPtr( const char* szKeyName, const void* pValue ) = 0;

	virtual bool			ForEventData( void* pEvent ) const = 0;
};

class IGameEventListener2
{
public:
	virtual ~IGameEventListener2( void ) {}

	virtual void FireGameEvent( IGameEvent* event ) = 0;
	virtual int  GetEventDebugID( void ) = 0;
};

#undef CreateEvent
class CSVCMsg_GameEvent;
class IGameEventManager2
{
public:
	virtual             ~IGameEventManager2( ) = 0;
	virtual int         LoadEventsFromFile( const char* filename ) = 0;
	virtual void        Reset( ) = 0;
	virtual bool        AddListener( IGameEventListener2* listener, const char* name, bool bServerSide ) = 0;
	virtual bool        FindListener( IGameEventListener2* listener, const char* name ) = 0;
	virtual int         RemoveListener( IGameEventListener2* listener ) = 0;
	virtual IGameEvent* CreateEvent( const char* name, bool bForce, unsigned int dwUnknown ) = 0;
	virtual bool        FireEvent( IGameEvent* event, bool bDontBroadcast = false ) = 0;
	virtual bool        FireEventClientSide( IGameEvent* event ) = 0;
	virtual IGameEvent* DuplicateEvent( IGameEvent* event ) = 0;
	virtual void        FreeEvent( IGameEvent* event ) = 0;
	virtual bool        SerializeEvent( IGameEvent* event, bf_write* buf ) = 0;
	virtual IGameEvent* UnserializeEvent( bf_read* buf ) = 0;
};
#define CreateEvent  CreateEventA