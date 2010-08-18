/*
 * Copyright 2010 Boris Byk.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

// DevSmtp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Server.h"

typedef struct _app_params
{
	bool use_console;
	bool need_install;
	bool need_uninstall;
	bool need_start;
} app_params;

void show_usage(TCHAR* pAppName)
{
	TCHAR drive[_MAX_DRIVE],
		dir[_MAX_PATH],
		filename[_MAX_FNAME],
		extension[_MAX_EXT];

	_tsplitpath_s(pAppName, drive, sizeof(drive), dir, sizeof(dir), filename, sizeof(filename), extension, sizeof(extension));

	_tprintf(_T("  %s - Development Smtp Server.\n\n"), filename);
	_tprintf(_T("  Version: 1.0\n"));
	_tprintf(_T("  Author: Boris Byk\n\n"));
	_tprintf(_T("Purpose\n"));
	_tprintf(_T("  Saves email to file and writes protocol to OutputDebugString or console."));
	_tprintf(_T("Usage:\n"));
	_tprintf(_T("  %s [options]\n\n"), filename);
	_tprintf(_T("Options:\n"));
	_tprintf(_T("  -i : install as a windows service.\n"));
	_tprintf(_T("  -u : uninstall installed windows service.\n"));
	_tprintf(_T("  -c : run in console mode.\n"));
	_tprintf(_T("  -s : start the application as a windows service.\n"));
}

static void parse_arguments( int argc, _TCHAR* argv[], app_params& param )
{	
	for (int i=1; i<argc; i++)
	{
		if (_tcsicmp(argv[i], _T("-i")) == 0)
		{
			param.need_install = true;
		}
		else if (_tcsicmp(argv[i], _T("-u")) == 0)
		{
			param.need_uninstall = true;
		}
		else if (_tcsicmp(argv[i], _T("-c")) == 0)
		{
			param.use_console = true;
		}
		else if (_tcsicmp(argv[i], _T("-s")) == 0)
		{
			param.need_start = true;
		}
	}
}

static void svcUninstall()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;

	// Get a handle to the SCM database. 
	schSCManager = OpenSCManager( 
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager) 
	{
		_tprintf(_T("OpenSCManager failed (%d)\r\n"), GetLastError());
		return;
	}

	// Get a handle to the service.
	schService = OpenService( 
		schSCManager,            // SCM database 
		APP_NAME,               // name of service 
		DELETE);  // need change config access 

	if (NULL == schService)
	{ 
		_tprintf(_T("Failed open service (%d)\r\n"), GetLastError()); 
		CloseServiceHandle(schSCManager);
		return;
	}

	if ( !DeleteService(schService) )
	{
		_tprintf(_T("Failed delete service (%d).\r\n"), GetLastError());
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

static void svcStart()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;

	// Get a handle to the SCM database. 
	schSCManager = OpenSCManager( 
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager) 
	{
		_tprintf(_T("OpenSCManager failed (%d)\r\n"), GetLastError());
		return;
	}

	// Get a handle to the service.
	schService = OpenService( 
		schSCManager,            // SCM database 
		APP_NAME,               // name of service 
		SERVICE_START);  // need change config access 

	if (NULL == schService)
	{ 
		_tprintf(_T("Failed open service (%d)\r\n"), GetLastError()); 
		CloseServiceHandle(schSCManager);
		return;
	}

	if( !StartService(schService, 0, NULL) )
	{
		printf("StartService failed (%d)\n", GetLastError());
		CloseServiceHandle(schService); 
		CloseServiceHandle(schSCManager);
	}
}

static void svcInstall()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	_TCHAR app_path[_MAX_PATH];

	if( !GetModuleFileName( NULL, app_path, _MAX_PATH ) )
	{
		_tprintf(_T("Cannot install service (%d)\r\n"), GetLastError());
		return;
	}

	// Get a handle to the SCM database. 
	schSCManager = OpenSCManager( 
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager) 
	{
		_tprintf(_T("OpenSCManager failed (%d)\r\n"), GetLastError());
		return;
	}

	// Create the service
	schService = CreateService( 
		schSCManager,              // SCM database 
		APP_NAME,                  // name of service 
		APP_NAME_DISPLAY,          // service name to display 
		SERVICE_ALL_ACCESS,        // desired access 
		SERVICE_WIN32_OWN_PROCESS, // service type 
		SERVICE_AUTO_START,		   // start type 
		SERVICE_ERROR_NORMAL,      // error control type 
		app_path,                  // path to service's binary 
		NULL,                      // no load ordering group 
		NULL,                      // no tag identifier 
		NULL,                      // no dependencies 
		NULL,                      // LocalSystem account 
		NULL);                     // no password 

	if (NULL == schService) 
	{
		_tprintf(_T("Failed to install service (%d)\r\n"), GetLastError()); 
	}
	else
	{
		_tprintf(_T("Service installed successfully\r\n"));
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

void DevSmtp::Log(const _TCHAR* buffer)
{
	Log(NULL, buffer);
}

static app_params g_Params = {0};

void DevSmtp::Log(const _TCHAR* context, const _TCHAR* buffer)
{
	_timeb tnow;
	_ftime_s(&tnow);
	_TCHAR dtbuf[22];
	tm today;
	localtime_s( &today, &(tnow.time) );
	_tcsftime( dtbuf, sizeof(dtbuf) / sizeof(_TCHAR), _T("%Y-%m-%d %H:%M:%S"), &today );

	CString msg;
	if (context == NULL)
		msg.Format(_T("[%s] %s\r\n"), dtbuf, buffer);
	else
		msg.Format(_T("[%s] (%s) %s\r\n"), dtbuf, context, buffer);

	if (g_Params.use_console)
	{
		_putts(msg);
	}
	else
	{
		OutputDebugString(msg);
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	setlocale( LC_ALL, "English" );

	if (argc < 2)
		show_usage(argv[0]);

	parse_arguments(argc, argv, g_Params);

	if (g_Params.need_install)
	{
		svcInstall();

		if (g_Params.need_start)
			svcStart();

		return 0;
	}

	if (g_Params.need_start)
	{
		svcStart();
		return 0;
	}
	
	if (g_Params.need_uninstall)
	{
		svcUninstall();
		return 0;
	}

	DevSmtp::Server srv;

	if (g_Params.use_console)
	{
		srv.IsInConsole = true;
		srv.OnStart();
	}
	else
	{
		srv.Start();
	}

	return 0;
}
