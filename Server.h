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