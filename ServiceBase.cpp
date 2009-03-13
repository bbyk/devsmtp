#include "StdAfx.h"
#include "ServiceBase.h"

namespace DevSmtp
{
	ServiceBase* gInst;

	ServiceBase::ServiceBase(_TCHAR* r_pSrvName) : m_hSvcStopEvent(NULL), m_dwCheckPoint(1)
	{
		m_pSrvName = r_pSrvName;
	}

	ServiceBase::~ServiceBase(void)
	{
	}

	void ServiceBase::Start()
	{
		gInst = this;

		// TO_DO: Add any additional services for the process to this table.
		SERVICE_TABLE_ENTRY dispatch_table[] = 
		{ 
			{ m_pSrvName, (LPSERVICE_MAIN_FUNCTION)SvcMain }, 
			{ NULL, NULL } 
		}; 

		// This call returns when the service has stopped. 
		// The process should simply terminate when the call returns.

		if ( !StartServiceCtrlDispatcher( dispatch_table ) ) 
		{ 
			LOG1(_T("Can't connect to service control manager (%d)."), GetLastError());
		}
	}

	void ServiceBase::OnSvcMain()
	{
		// Register the handler function for the service
		m_hSvcStatus = RegisterServiceCtrlHandler( 
			m_pSrvName, 
			SvcCtrlHandler);

		// These SERVICE_STATUS members remain as set here

		m_svcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
		m_svcStatus.dwServiceSpecificExitCode = 0;    

		// Report initial status to the SCM

		ReportSvcStatus( SERVICE_START_PENDING, NO_ERROR, 3000 );

		// Perform service-specific initialization and work.

		SvcInit();
	}

	void ServiceBase::SvcInit()
	{
		m_hSvcStopEvent = CreateEvent(
			NULL,    // default security attributes
			TRUE,    // manual reset event
			FALSE,   // not signaled
			NULL);   // no name

		if ( m_hSvcStopEvent == NULL)
		{
			ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
			return;
		}

		if ( !OnStart() )
		{
			ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
			return;
		}

		// Report running status when initialization is complete.
		ReportSvcStatus( SERVICE_RUNNING, NO_ERROR, 0 );

		while(TRUE)
		{
			// Check whether to stop the service.
			WaitForSingleObject(m_hSvcStopEvent, INFINITE);

			OnStop();

			ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
			return;
		}
	}

	void CALLBACK ServiceBase::SvcMain( DWORD dwArgc, _TCHAR** lpszArgv )
	{
		gInst->OnSvcMain();
	}

	void ServiceBase::ReportSvcStatus( DWORD dwCurrentState,
		DWORD dwWin32ExitCode,
		DWORD dwWaitHint)
	{
		// Fill in the SERVICE_STATUS structure.
		m_svcStatus.dwCurrentState = dwCurrentState;
		m_svcStatus.dwWin32ExitCode = dwWin32ExitCode;
		m_svcStatus.dwWaitHint = dwWaitHint;

		if (dwCurrentState == SERVICE_START_PENDING)
			m_svcStatus.dwControlsAccepted = 0;
		else m_svcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

		if ( (dwCurrentState == SERVICE_RUNNING) ||
			(dwCurrentState == SERVICE_STOPPED) )
			m_svcStatus.dwCheckPoint = 0;
		else m_svcStatus.dwCheckPoint = m_dwCheckPoint++;

		// Report the status of the service to the SCM.
		SetServiceStatus( m_hSvcStatus, &m_svcStatus );
	}

	void ServiceBase::OnSvcCtrlHandler( DWORD dwCtrl )
	{
		// Handle the requested control code. 

		switch(dwCtrl) 
		{  
		case SERVICE_CONTROL_STOP: 
			ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

			// Signal the service to stop.

			SetEvent(m_hSvcStopEvent);

			return;

		case SERVICE_CONTROL_INTERROGATE: 
			// Fall through to send current status.
			break; 

		default: 
			break;
		} 

		ReportSvcStatus(m_svcStatus.dwCurrentState, NO_ERROR, 0);
	}

	void CALLBACK ServiceBase::SvcCtrlHandler( DWORD dwCtrl )
	{
		gInst->OnSvcCtrlHandler( dwCtrl );
	}

	void ServiceBase::Stop()
	{
		OnSvcCtrlHandler(SERVICE_CONTROL_STOP);
	}
}