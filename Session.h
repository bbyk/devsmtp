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

#pragma once

#include "Server.h"
#include "FileOverlapped.h"

namespace DevSmtp
{
	class Session : public OVERLAPPED
	{
		enum STATE
		{
			WAIT_ACCEPT,
			WAIT_REQUEST,
			WAIT_SEND,
			WAIT_REQUEST_DATA,
			WAIT_WRITE,
		};

		enum OPSTATE
		{
			NONE,
			SEND_WELCOME,
			RCV_HELLO,
			RE_RCV_HELLO,
			MAIL,
			RE_MAIL,
			RCPT,
			RE_RCPT,
			RCPT_OR_DATA,
			WRITE_FILE,
			RE_DATA,
			DOT,
			RE_DOT,
			QUIT_OR_MAIL,
			RE_QUIT,
		};

		friend class Server;
		friend class FileOverlapped;
	private:
		FileOverlapped m_pFo;
		Server* m_pServer;
		SOCKET m_hSocket;
		HANDLE m_hFile;
		BYTE m_addrBlock[ ACCEPT_ADDRESS_LENGTH * 2 ];
		char m_charBuf[ DEFAULT_READ_BUFFER_SIZE ];
		std::basic_string<char> m_line, m_outline;
		STATE m_eState;
		OPSTATE m_eOpState;
		void OnReadLine(void);
		void OnIoComplete(DWORD);

		bool CanProcessZero();
		void CompleteReadData(DWORD);

		void CompleteWrite(DWORD);
		void CompleteWriteFile(DWORD);
		void CompleteAccept(void);

		void CompleteOperation();

		void IssueWriteFile(size_t pos);
		void IssueReset();
		void IssueRead(void);
		void IssueRead(STATE);
		void IssueAccept(void);
		void CompleteReadLine(DWORD);
		void IssueWrite(size_t pos);
	public:
		Session( Server* );
		~Session(void);
	};
}
