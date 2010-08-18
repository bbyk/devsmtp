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

#include "StdAfx.h"
#include "Server.h"

namespace DevSmtp
{
	Server::Server(void) : m_pSessions(), m_hStopEvent(0), 
		m_hIoPort(0), m_hListener(0), IsInConsole(false), ServiceBase(APP_NAME)
	{
		m_pSessions.reserve(MAX_SESSIONS);

		// getting current execution directory path.
		_TCHAR szFileName[_MAX_PATH];
		GetModuleFileName(NULL, szFileName, sizeof(szFileName) / sizeof(_TCHAR));

		_TCHAR szDrive[_MAX_DRIVE],
			szPath[_MAX_PATH],
			szFilename[_MAX_FNAME],
			szExtension[_MAX_EXT];

		_tsplitpath_s(szFileName, szDrive, szPath, szFilename, szExtension);
		m_pPath = szDrive;
		m_pPath += szPath;
	}

	Server::~Server(void)
	{
	}

	bool Server::OnStart()
	{
		Log(_T("Starting DevSmtp server"));

		// Initialize the Microsoft Windows Sockets Library
		WSADATA wsa = {0};
		int err = WSAStartup( MAKEWORD(2,2), &wsa );
		if (err != 0)
		{
			Log(_T("Can not find a usable Winsock DLL\r\n"));
			return false;
		}		
		
		m_hStopEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
		m_hIoPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE, 0, 0, MAX_THREADS );
		if (NULL == m_hIoPort)
		{
			LOG1(_T("Cannot create completion port (%d)\r\n"), GetLastError());
			return false;
		}

		// Set up a socket on which to listen for new connections.
		m_hListener = WSASocket( PF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 
			WSA_FLAG_OVERLAPPED );
		if( INVALID_SOCKET == m_hListener )
		{
			LOG1(_T("Cannot create listening socket (%d)\r\n"), WSAGetLastError());
			return false;
		}

		SOCKADDR_IN addr = {0};
		addr.sin_family = AF_INET;
		addr.sin_addr.S_un.S_addr = INADDR_ANY;
		addr.sin_port = htons( DEFAULT_PORT );

		// Bind the listener to the local interface and set to listening state.
		err = bind( m_hListener, (PSOCKADDR)&addr, sizeof(SOCKADDR_IN) );
		if (err != S_OK)
		{
			closesocket(m_hListener);
			LOG1(_T("Fail bind to port (%d)\r\n"), WSAGetLastError());
			return false;
		}

		if (listen( m_hListener, DEFAULT_LISTEN_QUEUE_SIZE ) == SOCKET_ERROR)
		{
			closesocket(m_hListener);
			LOG1(_T("Fail establish listening (%d)\r\n"), WSAGetLastError());
			return false;
		}

		// Associate the Listener socket with the I/O Completion Port.
		CreateIoCompletionPort( (HANDLE)m_hListener, m_hIoPort, COMPLETION_KEY_IO, 0 );

		// Allocate an array of connections; constructor binds them to the port.
		for (int i = 0; i<MAX_SESSIONS; i++)
		{
			m_pSessions.push_back( new Session( this ) );
		}

		// Create worker threads
		for (int i = 0; i<MAX_THREADS_IN_POOL; i++)
		{
			m_hThreads[i] = (HANDLE)_beginthreadex( 0, 0, WorkerProcCallback, (void*)this, 0, 
				NULL );
		}

		Log(_T("Started DevSmtp server"));

		if (IsInConsole)
		{
			// Wait for the user to press CTRL-C...
			WaitForSingleObject( m_hStopEvent, INFINITE );
		}

		return true;
	}

	unsigned CALLBACK Server::WorkerProcCallback(void* thisContext)
	{
		Server* srv = reinterpret_cast<Server*>(thisContext);
		srv->WorkerProc();

		return 0;
	}

	void Server::WorkerProc()
	{
		BOOL status;
		DWORD num_transferred = 0;
		ULONG_PTR key;
		LPOVERLAPPED pOver = NULL;

		while(TRUE)
		{
			status = GetQueuedCompletionStatus(
				m_hIoPort, 
				&num_transferred, &key, &pOver,INFINITE );

			Session* pSession = reinterpret_cast<Session*>(pOver);
			if ( status == FALSE || (num_transferred == 0 && pSession != NULL && !pSession->CanProcessZero()) )
			{
				// An error occurred; reset to a known state.
				if ( pSession != NULL )
				{
					Log(_T("Remote host closed connection."));
					pSession->IssueReset();
				}
			}
			else
			{			
				switch (key)
				{
				case COMPLETION_KEY_IO:
					pSession->OnIoComplete(num_transferred);
					break;
				case COMPLETION_KEY_SHUTDOWN:
					return;
				case COMPLETION_FILE_KEY_IO:
					FileOverlapped* fo = reinterpret_cast<FileOverlapped*>(pOver);
					fo->m_pSession->OnIoComplete(num_transferred);
					break;
				}
			}
		}
	}

	void Server::OnStop()
	{
		Log(_T("Stopping DevStmp server."));

		SetEvent(m_hStopEvent);

		// Post a quit completion message, one per worker thread.
		for (size_t i=0; i<MAX_THREADS_IN_POOL; i++)
		{
			PostQueuedCompletionStatus( m_hIoPort, 0, COMPLETION_KEY_SHUTDOWN, 0 );
		}

		// Wait for all of the worker threads to terminate...
		WaitForMultipleObjects( MAX_THREADS_IN_POOL, m_hThreads, TRUE, INFINITE );

		// Close worker thread handles.
		for (size_t i=0; i<MAX_THREADS_IN_POOL; i++)
		{
			CloseHandle( m_hThreads[i] );
		}

		// Close stop event.
		CloseHandle( m_hStopEvent );
		m_hStopEvent = 0;

		// stop using Winsock2 
		WSACleanup();

		Log(_T("Stopped DevSmtp server."));
	}
}
