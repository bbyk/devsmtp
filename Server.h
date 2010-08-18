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

#include "Session.h"
#include "ServiceBase.h"

namespace DevSmtp
{
	class Server : public ServiceBase
	{
		friend class Session;
	private:
		std::vector<Session*> m_pSessions;
		HANDLE m_hThreads[MAX_THREADS_IN_POOL];
		HANDLE m_hStopEvent;
		HANDLE m_hIoPort;
		SOCKET m_hListener;
		static unsigned CALLBACK WorkerProcCallback(void*);
		void WorkerProc(void);
		tstring m_pPath;
	public:
		bool IsInConsole;
		Server(void);
		~Server(void);
		virtual bool OnStart();
	protected:
		virtual void OnStop();
	};
}
