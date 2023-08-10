#include "keyvalues.h"
#include "../../core/interfaces.h"

#include <assert.h>

CKeyValues::CKeyValues( const char* szKeyName, void* pUnknown1, HKeySymbol hCaseInsensitiveKeyName )
{
	using CKeyValuesConstructorFn = void( __thiscall* )( void*, const char*, void*, HKeySymbol );
	static CKeyValuesConstructorFn oConstructor = reinterpret_cast< CKeyValuesConstructorFn >( Displacement::Sigs.InitKeyValues ); // @xref: client.dll -> "OldParticleSystem_Destroy"
	oConstructor( this, szKeyName, pUnknown1, hCaseInsensitiveKeyName );
}

CKeyValues::~CKeyValues( )
{
	using CKeyValuesDestructorFn = void( __thiscall* )( void*, int );
	static CKeyValuesDestructorFn oDestructor = reinterpret_cast< CKeyValuesDestructorFn >( Displacement::Sigs.DestructKeyValues );
	oDestructor( this, 1 );
}

void* CKeyValues::operator new(std::size_t nAllocSize)
{
	// manually allocate memory, because game constructor doesn't call it automatically
	return Interfaces::KeyValuesSystem->AllocKeyValuesMemory( nAllocSize );
}

void CKeyValues::operator delete(void* pMemory)
{
	// do nothing, because game destructor will automatically free memory
	// I::KeyValuesSystem->FreeKeyValuesMemory(pMemory);
	( void )pMemory;
}

const char* CKeyValues::GetName()
{
	return Interfaces::KeyValuesSystem->GetStringForSymbol(this->uKeyNameCaseSensitive1 | (this->uKeyNameCaseSensitive2 << 8));
}

CKeyValues* CKeyValues::FromString(const char* szName, const char* szValue)
{
	CKeyValues* pKeyValues = nullptr;

	__asm
	{
		push 0
		mov edx, szValue
		mov ecx, szName
		call Displacement::Sigs.oFromString
		add esp, 4
		mov pKeyValues, eax
	}

	return pKeyValues;
}

void CKeyValues::LoadFromBuffer(char const* szResourceName, const char* szBuffer, void* pFileSystem, const char* szPathID, GetSymbolProcFn pfnEvaluateSymbolProc)
{
	using LoadFromBufferFn = void(__thiscall*)(void*, const char*, const char*, void*, const char*, void*, void*);
	static auto oLoadFromBuffer = reinterpret_cast<LoadFromBufferFn>( Displacement::Sigs.oLoadFromBuffer );
	assert(oLoadFromBuffer != nullptr);

	oLoadFromBuffer(this, szResourceName, szBuffer, pFileSystem, szPathID, pfnEvaluateSymbolProc, nullptr);
}

bool CKeyValues::LoadFromFile(void* pFileSystem, const char* szResourceName, const char* szPathID, GetSymbolProcFn pfnEvaluateSymbolProc)
{
	using LoadFromFileFn = bool(__thiscall*)(void*, void*, const char*, const char*, void*);
	static auto oLoadFromFile = reinterpret_cast<LoadFromFileFn>( Displacement::Sigs.oLoadFromFile );
	assert(oLoadFromFile != nullptr);

	return oLoadFromFile(this, pFileSystem, szResourceName, szPathID, pfnEvaluateSymbolProc);
}

CKeyValues* CKeyValues::FindKey(const char* szKeyName, const bool bCreate)
{
	using FindKeyFn = CKeyValues * (__thiscall*)(void*, const char*, bool);
	static auto oFindKey = reinterpret_cast<FindKeyFn>( Displacement::Sigs.oFindKey );
	assert(oFindKey != nullptr);

	return oFindKey(this, szKeyName, bCreate);
}

int CKeyValues::GetInt( const char* szKeyName, const int iDefaultValue )
{
	CKeyValues* pSubKey = this->FindKey( szKeyName, false );

	if ( pSubKey == nullptr )
		return iDefaultValue;

	switch ( pSubKey->iDataType )
	{
	case TYPE_STRING:
		return std::atoi( pSubKey->szValue );
	case TYPE_WSTRING:
		return _wtoi( pSubKey->wszValue );
	case TYPE_FLOAT:
		return static_cast< int >( pSubKey->flValue );
	case TYPE_UINT64:
		// can't convert, since it would lose data
		assert( false );
		return 0;
	default:
		break;
	}

	return pSubKey->iValue;
}

float CKeyValues::GetFloat( const char* szKeyName, const float flDefaultValue )
{
	CKeyValues* pSubKey = this->FindKey( szKeyName, false );

	if ( pSubKey == nullptr )
		return flDefaultValue;

	switch ( pSubKey->iDataType )
	{
	case TYPE_STRING:
		return static_cast< float >( std::atof( pSubKey->szValue ) );
	case TYPE_WSTRING:
		return std::wcstof( pSubKey->wszValue, nullptr );
	case TYPE_FLOAT:
		return pSubKey->flValue;
	case TYPE_INT:
		return static_cast< float >( pSubKey->iValue );
	case TYPE_UINT64:
		return static_cast< float >( *reinterpret_cast< std::uint64_t* >( pSubKey->szValue ) );
	case TYPE_PTR:
	default:
		return 0.0f;
	}
}

const char* CKeyValues::GetString( const char* szKeyName, const char* szDefaultValue )
{
	using GetStringFn = const char* ( __thiscall* )( void*, const char*, const char* );
	static auto oGetString = reinterpret_cast< GetStringFn >( Displacement::Sigs.oGetString );
	assert( oGetString != nullptr );

	return oGetString( this, szKeyName, szDefaultValue );
}

void CKeyValues::SetString( const char* szKeyName, const char* szStringValue )
{
	CKeyValues* pSubKey = FindKey( szKeyName, true );

	if ( pSubKey == nullptr )
		return;

	using SetStringFn = void( __thiscall* )( void*, const char* );
	static auto oSetString = reinterpret_cast< SetStringFn >( Displacement::Sigs.oSetString );
	assert( oSetString != nullptr );

	oSetString( pSubKey, szStringValue );
}

void CKeyValues::SetInt( const char* szKeyName, const int iValue )
{
	CKeyValues* pSubKey = FindKey( szKeyName, true );

	if ( pSubKey == nullptr )
		return;

	pSubKey->iValue = iValue;
	pSubKey->iDataType = TYPE_INT;
}

void CKeyValues::SetUint64( const char* szKeyName, const int nLowValue, const int nHighValue )
{
	CKeyValues* pSubKey = FindKey( szKeyName, true );

	if ( pSubKey == nullptr )
		return;

	// delete the old value
	delete[ ] pSubKey->szValue;

	// make sure we're not storing the WSTRING - as we're converting over to STRING
	delete[ ] pSubKey->wszValue;
	pSubKey->wszValue = nullptr;

	pSubKey->szValue = new char[ sizeof( std::uint64_t ) ];
	*reinterpret_cast< std::uint64_t* >( pSubKey->szValue ) = static_cast< std::uint64_t >( nHighValue ) << 32ULL | nLowValue;
	pSubKey->iDataType = TYPE_UINT64;
}