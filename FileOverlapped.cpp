#include "StdAfx.h"
#include "FileOverlapped.h"

namespace DevSmtp
{
	FileOverlapped::FileOverlapped(Session* r_pSession)
	{
		m_pSession = r_pSession;
	}

	FileOverlapped::~FileOverlapped(void)
	{
	}
}
