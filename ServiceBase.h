#pragma once

namespace DevSmtp
{
	class ServiceBase
	{
	public:
		void Start(void);
		void Stop();
		ServiceBase(_TCHAR* = NULL);
		~ServiceBase(void);
		virtual bool ServiceBase::OnStart() = 0;
	private:
		_TCHAR* m_pSrvName;
		SERVICE_STATUS          m_svcStatus; 
		SERVICE_STATUS_HANDLE   m_hSvcStatus; 
		HANDLE                  m_hSvcStopEvent;
		DWORD					m_dwCheckPoint;
		static void CALLBACK SvcMain( DWORD, _TCHAR** );
		void OnSvcMain();
		void SvcInit();
		void OnSvcCtrlHandler( DWORD dwCtrl );
		static void CALLBACK ServiceBase::SvcCtrlHandler( DWORD dwCtrl );
		void ReportSvcStatus( DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
	protected:
		virtual void ServiceBase::OnStop() = 0;
	};
}
