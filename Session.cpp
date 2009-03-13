#include "StdAfx.h"
#include "Session.h"

#pragma warning(disable:4482)

namespace DevSmtp
{
	Session::Session( Server* r_pServer ) : m_pServer(r_pServer), 
		m_eState(WAIT_ACCEPT), m_eOpState(NONE), m_hFile(0), m_pFo(this)
	{
		ZeroMemory( m_charBuf, DEFAULT_READ_BUFFER_SIZE );
		m_line.reserve( DEFAULT_READ_BUFFER_SIZE );	
		IssueAccept();
	}

	Session::~Session(void)
	{
	}

	void Session::IssueAccept(void)
	{
		Internal = 0;
		InternalHigh = 0;
		Offset = 0;
		OffsetHigh = 0;
		hEvent = 0;
		m_eState = WAIT_ACCEPT;

		ZeroMemory( m_addrBlock, ACCEPT_ADDRESS_LENGTH * 2 );
		m_hSocket = WSASocket( PF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 
			WSA_FLAG_OVERLAPPED );
		assert(m_hSocket != INVALID_SOCKET);

		// Associate the client socket with the I/O Completion Port.
		CreateIoCompletionPort( 
			(HANDLE)m_hSocket,
			m_pServer->m_hIoPort, 
			COMPLETION_KEY_IO, 0 );

		BOOL res = AcceptEx( m_pServer->m_hListener, m_hSocket, m_addrBlock, 0, ACCEPT_ADDRESS_LENGTH, 
			ACCEPT_ADDRESS_LENGTH, NULL, this );
		int err;

		if (!(res == FALSE && (err = WSAGetLastError()) == ERROR_IO_PENDING))
		{
			LOG1(_T("Fail AcceptEx (%d)"), err);
			exit(2);
		}
	}

	void Session::OnIoComplete(DWORD rNumTransferred)
	{	
		switch (m_eState)
		{
		case WAIT_ACCEPT:
			CompleteAccept();
			break;
		case WAIT_REQUEST:
			CompleteReadLine( rNumTransferred );
			break;
		case WAIT_SEND:
			CompleteWrite( rNumTransferred );
			break;
		case WAIT_REQUEST_DATA:
			CompleteReadData( rNumTransferred );
			break;
		case WAIT_WRITE:
			CompleteWriteFile( rNumTransferred );
			break;
		}
	}

	void Session::CompleteReadLine(DWORD rNumTransferred)
	{
		bool shdIssue = true;

		if (rNumTransferred > 0)
		{
			DWORD l_charPos = 0;
			int span;
			DWORD char_pos = l_charPos;

			do 
			{
				char ch = m_charBuf[char_pos];
				if (ch == '\r' || ch == '\n')
				{
					span = char_pos - l_charPos;
					m_line.append( &(m_charBuf[l_charPos]), span );
					l_charPos = char_pos + 1;

					if ((ch == '\r' && l_charPos < rNumTransferred) && (m_charBuf[l_charPos] == '\n'))
					{
						l_charPos++;
					}

					OnReadLine();
					m_line.clear();
					return;
				}
				else
				{
					char_pos++;
				}
			} 
			while (char_pos < rNumTransferred);
			span = rNumTransferred - l_charPos;
			m_line.append( &(m_charBuf[l_charPos]), span );
		}

		if (shdIssue)
			IssueRead();
	}

	void Session::CompleteReadData( DWORD rNumTransferred )
	{
		DWORD l_char_pos = 0;
		int span;
		DWORD char_pos = l_char_pos;
		bool shdIssue = true;
		char sep[] = "\r\n.\r\n";
		int si = 0;

		do 
		{
			char ch = m_charBuf[char_pos];

			if (ch == sep[si])
			{
				si++;
			}
			else if (si > 0)
			{
				si = 0;

				if (ch == sep[si])
				{
					si++;
				}
			}

			if (si == 5) //separator length)
			{
				span = char_pos - l_char_pos;
				m_line.append( &(m_charBuf[l_char_pos]), span - 3 );
				l_char_pos = char_pos + 1;

				OnReadLine();

				m_line.clear();
				return;
			}

			char_pos++;
		} 
		while (char_pos < rNumTransferred);
		span = rNumTransferred - l_char_pos;
		m_line.append( &(m_charBuf[l_char_pos]), span );

		if (shdIssue)
			IssueRead(STATE::WAIT_REQUEST_DATA);
	}

	void Session::CompleteAccept()
	{
		setsockopt( m_hSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
			(char*)&m_pServer->m_hListener, sizeof(SOCKET) );

		// fetch connection context and log it
		SOCKADDR_IN sin;
		int nameLen = sizeof(sin);
		getpeername( m_hSocket, (PSOCKADDR)&sin, &nameLen);
		char* remote_ip = inet_ntoa(sin.sin_addr);
		char buffer[26];
		sprintf_s(buffer, sizeof(buffer), "Connected %s", remote_ip);
		Log(TOLPTCHAR(buffer));

		CompleteOperation();
	}

	void Session::OnReadLine()
	{
		_TCHAR buf[65];
		_itot_s(m_hSocket, buf, 10);
		LOG2(_T("C: %s"), buf, TOLPTCHAR(&m_line[0]));
		
		CompleteOperation();
	}

	void Session::IssueRead()
	{
		IssueRead(STATE::WAIT_REQUEST);
	}

	void Session::IssueRead(STATE rState)
	{
		m_eState = rState;
		
		BOOL res = ReadFile( (HANDLE)m_hSocket, m_charBuf, DEFAULT_READ_BUFFER_SIZE, 
			NULL, (LPOVERLAPPED)this );
 
		if (!res)
		{
			int err = GetLastError();
			if (err == ERROR_IO_PENDING)
			{
				return;
			}
			else if (err == ERROR_NETNAME_DELETED)
			{
				IssueReset();
				return;
			}

			assert(false);
		}
	}

	void Session::IssueWrite(size_t pos)
	{
		m_eState = WAIT_SEND;
		BOOL res = WriteFile( (HANDLE)m_hSocket, &(m_outline[pos]), m_outline.size() - pos, NULL, (LPOVERLAPPED)this );
		int err;

		if (!(res == TRUE || (err = WSAGetLastError()) == ERROR_IO_PENDING))
		{
			LOG1(_T("Fail WriteFile (%d)"), err);
		}
	}

	void Session::CompleteWrite( DWORD rNumTransferred )
	{
		if (rNumTransferred < m_outline.size())
		{
			IssueWrite( rNumTransferred );
			return;
		}

		_TCHAR buf[65];
		_itot_s(m_hSocket, buf, 10);
		LOG2(_T("S: %s"), buf, TOLPTCHAR(&(m_outline.substr(0, m_outline.size() - 2)[0])));
		CompleteOperation();
	}

	void Session::CompleteOperation()
	{
		if (m_eState == STATE::WAIT_REQUEST && 
			strncmp(m_line.c_str(), "QUIT", 4) == 0)
		{
			m_eOpState = OPSTATE::RE_QUIT;
			m_outline = "221 Bye\r\n";
			IssueWrite(0);
			return;
		}

		if (m_eState == STATE::WAIT_REQUEST &&
			strncmp(m_line.c_str(), "RSET", 4) == 0)
		{
			m_eOpState = OPSTATE::RE_DOT;
			m_outline = "250 Ok\r\n";
			IssueWrite(0);
			return;
		}

		if (m_eState == STATE::WAIT_REQUEST &&
			strncmp(m_line.c_str(), "NOOP", 4) == 0)
		{
			// does not change state
			m_outline = "250 Ok\r\n";
			IssueWrite(0);
			return;
		}

		switch (m_eOpState)
		{
		case OPSTATE::NONE:
			m_eOpState = OPSTATE::SEND_WELCOME;
			m_outline = "220 DevSmtp\r\n";
			IssueWrite(0);
			break;
		case OPSTATE::SEND_WELCOME:
			m_eOpState = OPSTATE::RCV_HELLO;
			IssueRead();
			break;
		case OPSTATE::RCV_HELLO:
			if (strncmp(m_line.c_str(), "HELO", 4) != 0)
			{
				m_eOpState = OPSTATE::SEND_WELCOME;
				m_outline = "500 Command not recognized: EHLO\r\n";
				IssueWrite(0);
				return;
			}

			m_eOpState = OPSTATE::RE_RCV_HELLO;
			m_outline = "250 Ok\r\n";
			IssueWrite(0);
			break;
		case OPSTATE::RE_RCV_HELLO:
			m_eOpState = OPSTATE::MAIL;
			IssueRead();
			break;
		case OPSTATE::MAIL:
			if (strncmp(m_line.c_str(), "MAIL", 4) != 0)
			{
				m_eOpState = OPSTATE::RE_RCV_HELLO;
				m_outline = "500 Command not recognized: EHLO\r\n";
				IssueWrite(0);
				return;
			}
			m_eOpState = OPSTATE::RE_MAIL;
			m_outline = "250 Ok\r\n";
			IssueWrite(0);
			break;
		case OPSTATE::RE_MAIL:
			m_eOpState = OPSTATE::RCPT;
			IssueRead();
			break;
		case OPSTATE::RCPT:
			m_eOpState = OPSTATE::RE_RCPT;
			m_outline = "250 Ok\r\n";
			IssueWrite(0);
			break;
		case OPSTATE::RE_RCPT:
			m_eOpState = OPSTATE::RCPT_OR_DATA;
			IssueRead();
			break;
		case OPSTATE::RCPT_OR_DATA:
			if (strncmp(m_line.c_str(), "DATA", 4) == 0)
			{
				m_eOpState = OPSTATE::RE_DATA;
				m_outline = "354 End data with <CR><LF>.<CR><LF>\r\n";
			}
			else
			{			
				m_eOpState = OPSTATE::RE_RCPT;
				m_outline = "250 Ok\r\n";
			}
			
			IssueWrite(0);
			break;
		case OPSTATE::RE_DATA:
			m_eOpState = OPSTATE::DOT;
			IssueRead(STATE::WAIT_REQUEST_DATA);
			break;
		case OPSTATE::DOT:
			m_eOpState = OPSTATE::WRITE_FILE;
			IssueWriteFile(0);
			break;
		case OPSTATE::WRITE_FILE:
			m_eOpState = OPSTATE::RE_DOT;
			m_outline = "250 Ok\r\n";
			IssueWrite(0);
			break;
		case OPSTATE::RE_DOT:
			m_eOpState = OPSTATE::QUIT_OR_MAIL;
			IssueRead();
			break;
		case OPSTATE::QUIT_OR_MAIL:
			if (strncmp(m_line.c_str(), "MAIL", 4) == 0)
			{
				m_eOpState = OPSTATE::RE_MAIL;
				m_outline = "250 Ok\r\n";
				IssueWrite(0);
			}
			break;
		case OPSTATE::RE_QUIT:
			IssueReset();
			break;
		}
	}

	void Session::IssueReset()
	{
		Log(_T("Closed socket."));
		closesocket(m_hSocket);
		m_eOpState = OPSTATE::NONE;
		IssueAccept();
	}

	bool Session::CanProcessZero()
	{
		return m_eState == STATE::WAIT_ACCEPT;
	}

	void Session::IssueWriteFile(size_t pos)
	{
		if (m_hFile == 0)
		{
			byte attempts = 50;
			_TCHAR file_name[_MAX_PATH];
						
			tstring full_path;
			full_path.reserve(_MAX_PATH);
			full_path.append(m_pServer->m_pPath);
			full_path.append("email-");

			time_t tt = time( NULL );
			srand((unsigned)tt);
			_TCHAR tmpbuf[16];
			struct tm today;
			localtime_s( &today, &tt );
			_tcsftime(tmpbuf, sizeof(tmpbuf) / sizeof(_TCHAR), _T("%Y%m%d-%H%M%S"), &today);

			full_path.append(tmpbuf);
		
			while (TRUE)
			{
				_stprintf_s(file_name, sizeof(file_name) / sizeof(_TCHAR), _T("%s-(%d).eml"), full_path.c_str(), rand());

				LOG1(_T("Saving email to '%s'"), file_name);

				m_hFile = CreateFile(file_name, 
					GENERIC_WRITE, 
					0, 
					NULL, 
					CREATE_NEW,
					FILE_FLAG_OVERLAPPED | FILE_FLAG_WRITE_THROUGH,
					NULL);

				if (m_hFile == INVALID_HANDLE_VALUE)
				{
					LOG1(_T("Can't open file to write (%d)."), GetLastError());

					if (--attempts > 0)
						continue;
					else
					{
						IssueReset();
						return;
					}
				}

				break;
			}

			HANDLE hcport = CreateIoCompletionPort( 
				m_hFile,
				m_pServer->m_hIoPort, 
				COMPLETION_FILE_KEY_IO, 0 );

			assert(hcport != NULL);

			m_eState = WAIT_WRITE;

			m_pFo.Internal = 0;
			m_pFo.InternalHigh = 0;
			m_pFo.Offset = 0;
			m_pFo.OffsetHigh = 0;
			m_pFo.hEvent = 0;
		}
		else
		{
			LOG1(_T("Used existing file handler %d"), m_hFile);
		}
		
		BOOL res = WriteFile( m_hFile, &(m_line[pos]), m_line.size() - pos, NULL, &m_pFo );
		int err;

		if (!(res == TRUE || (err = GetLastError()) == ERROR_IO_PENDING))
		{
			LOG1(_T("Fail WriteFile (%d)"), err);
			CloseHandle(m_hFile);
			m_hFile = 0;

			IssueReset();
		}
	}

	void Session::CompleteWriteFile( DWORD rNumTransferred )
	{
		if (rNumTransferred < m_line.size())
		{
			IssueWriteFile( rNumTransferred );
			return;
		}

		CloseHandle(m_hFile);
		m_hFile = 0;

		CompleteOperation();
	}
}
