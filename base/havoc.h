#pragma once

// credits to qo0, as i used his base as a base for my sdk.

/* winapi */
#include <windows.h>

/* directx includes */
#include <d3d9.h>
//#include <d3dx9.h>

// used: time_t, tm, std::time
#include <chrono>

// used: ns,ms,s,m,h time literals
using namespace std::chrono_literals;

/* crypt */
#include "utils/xorstr.h"
// @note: also u can try lazy importer for some anti-reverse safety. documentation is available at https://github.com/JustasMasiulis/lazy_importer

/* other */
#include "utils/memory.h"

#include "sdk/datatypes/vector.h"