#pragma once
#include <inttypes.h>

#define LOAD_STRUCTURE( struc, type, arr )auto loadStructure{ ( char* ) &struc }; 	for ( int i{ }; i < sizeof( type ) - 1; ++i ) { arr.emplace_back( loadStructure[ i ] ); }

enum ERequestType {
	CLIENT_ACKNOWLEDGEMENT = 1,
	CLIENT_ACKNOWLEDGEMENTMODULE,
	LOGIN,
	GET_SUBSCRIPTIONS,
	GET_DECRYPTOR
};

struct LoginMessage_t {
	uint8_t m_iRequestType{ };// login etc

	char		m_szUsername[ 128 ];// self explan
	uint64_t	m_iHardwareId;// self explan
};

// mb make this into an input/output struct? variables are redundant in both.
struct TransImports_t {
	char m_szImportName[ 32 ];// mb longer?
	unsigned char m_szFunctionName[ 32 ];// mb longer?
	int m_pFunction{ }; // pointer relevant to client module
	int m_pFunctionPTR{ };// pointer to function inside image
};