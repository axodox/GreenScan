// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once


#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

#include <queue>
#include <unordered_map>

#define LPSTRDelete(str) { if(str) { delete [strlen(str) + 1] str; str=nullptr; } }

