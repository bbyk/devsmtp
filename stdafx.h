// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"
#include <sys/timeb.h>
#include <time.h>
#include <winsock2.h>
#include <mswsock.h> 
#include <windows.h>
#include <process.h>
#include <direct.h>
#include <stdio.h>
#include <tchar.h>
#include <cassert>
#include <vector>
#include <string>
#include <sstream>
#include <atlstr.h>
#include "defs.h"

namespace DevSmtp
{
	class Server;
	class Session;
	class ServiceBase;
	class FileOverlapped;

	enum
	{
		ACCEPT_ADDRESS_LENGTH = (sizeof( struct sockaddr_in) + 16),
		COMPLETION_KEY_SHUTDOWN = 1,
		COMPLETION_KEY_IO = 2,
		COMPLETION_FILE_KEY_IO = 3,
		MAX_THREADS_IN_POOL = MAX_THREADS * 2,
	};

	typedef std::basic_string<TCHAR> tstring;
	typedef std::basic_ostringstream<TCHAR> tostringstream;

	void Log(const _TCHAR*);
	void Log(const _TCHAR*, const _TCHAR*);
}

#define LOG1(format, v1) CString __log_msg; __log_msg.Format(format, v1); DevSmtp::Log(__log_msg);
#define LOG2(format, context, v1) CString __log_msg; __log_msg.Format(format, v1); DevSmtp::Log(context, __log_msg);

#ifdef UNICODE
#define TOLPTCHAR(str) CString(str)
#else
#define TOLPTCHAR(str) str
#endif

#include "Server.h"
#include "Session.h"

#pragma comment(lib, "ws2_32")   // Standard socket API.
#pragma comment(lib, "mswsock")  // AcceptEx, TransmitFile, etc,.

// TODO: reference additional headers your program requires here
