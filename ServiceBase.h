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
