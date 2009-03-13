#pragma once

#include "Server.h"
#include "Session.h"

namespace DevSmtp
{
	class FileOverlapped : public OVERLAPPED
	{
		friend class Server;
		Session* m_pSession;
	public:
		FileOverlapped(Session*);
		~FileOverlapped(void);
	};
}
