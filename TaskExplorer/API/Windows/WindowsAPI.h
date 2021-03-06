#pragma once
#include "../SystemAPI.h"
#include "WinProcess.h"
#include "WinSocket.h"
//#include "Monitors/EventMonitor.h"
#include "Monitors/EtwEventMonitor.h"
#include "Monitors/FwEventMonitor.h"
//#include "Monitors/FirewallMonitor.h"
#include "SidResolver.h"
#include "SymbolProvider.h"
#include "DnsResolver.h"
#include "WinService.h"
#include "WinDriver.h"
#include "WinWnd.h"
#include "WinPoolEntry.h"

enum MISC_WIN_EVENT_TYPE
{
	// ETW Disk events
	EtwDiskReadType = 1,
	EtwDiskWriteType,
	
	// ETW File I/O events
	EtwFileNameType,
	EtwFileCreateType,
	EtwFileDeleteType,
	EtwFileRundownType,
	
	// ETW Network events
	EtwNetworkReceiveType,
	EtwNetworkSendType,

	// EventLog Firewall events
	EvlFirewallAllowed,
	EvlFirewallBlocked,

	EventTypeUnknow = ULONG_MAX
};


class CWindowsAPI : public CSystemAPI
{
	Q_OBJECT

public:
	CWindowsAPI(QObject *parent = nullptr);
	virtual ~CWindowsAPI();

	virtual bool Init();

	virtual QPair<QString, QString> SellectDriver();
	virtual STATUS InitDriver(QString DeviceName, QString FileName, int SecurityLevel = 0);

	virtual bool RootAvaiable();

	virtual CProcessPtr GetProcessByID(quint64 ProcessId, bool bAddIfNew = false);

	virtual bool UpdateSysStats();

	virtual bool UpdateProcessList();

	virtual bool UpdateSocketList();

	virtual CSocketPtr FindSocket(quint64 ProcessId, quint32 ProtocolType, const QHostAddress& LocalAddress, quint16 LocalPort, const QHostAddress& RemoteAddress, quint16 RemotePort, CSocketInfo::EMatchMode Mode);

	virtual bool UpdateOpenFileList();

	virtual bool UpdateServiceList(bool bRefresh = false);

	virtual bool UpdateDriverList();

	virtual CSymbolProvider* GetSymbolProvider()		{ return m_pSymbolProvider; }

	virtual CSidResolver* GetSidResolver()				{ return m_pSidResolver; }

	virtual CDnsResolver* GetDnsResolver()				{ return m_pDnsResolver; }

	virtual bool UpdateDnsCache();
	virtual void FlushDnsCache();

	virtual quint64	GetCpuIdleCycleTime(int index);

	virtual quint32 GetTotalGuiObjects() const			{ QReadLocker Locker(&m_StatsMutex); return m_TotalGuiObjects; }
	virtual quint32 GetTotalUserObjects() const			{ QReadLocker Locker(&m_StatsMutex); return m_TotalUserObjects; }
	virtual quint32 GetTotalWndObjects() const			{ QReadLocker Locker(&m_StatsMutex); return m_TotalWndObjects; }

	virtual QMultiMap<quint64, quint64> GetWindowByPID(quint64 ProcessId) const  { QReadLocker Locker(&m_WindowMutex); return m_WindowMap[ProcessId]; }

	virtual QList<QString> GetServicesByPID(quint64 ProcessId) const			 { QReadLocker Locker(&m_ServiceMutex); return m_ServiceByPID.value(ProcessId); }

	virtual quint64 GetUpTime() const;

	virtual QList<SUser> GetUsers() const;

	virtual bool HasExtProcInfo() const;
	virtual bool IsMonitoringETW() const				{ return m_pEventMonitor != NULL; }
	virtual bool IsMonitoringFW() const					{ return m_pFirewallMonitor != NULL; }

	virtual bool IsTestSigning() const					{ QReadLocker Locker(&m_Mutex); return m_bTestSigning; }
	virtual bool HasDriverFailed() const				{ QReadLocker Locker(&m_Mutex); return m_uDriverStatus != 0; }
	virtual quint32 GetDriverStatus() const				{ QReadLocker Locker(&m_Mutex); return m_uDriverStatus; }
	virtual quint32 GetDriverFeatures() const			{ QReadLocker Locker(&m_Mutex); return m_uDriverFeatures; }
	virtual QString GetDriverFileName() const			{ QReadLocker Locker(&m_Mutex); return m_DriverFileName; }
	virtual QString GetDriverDeviceName() const			{ QReadLocker Locker(&m_Mutex); return m_DriverDeviceName; }

	//__inline bool UseDiskCounters() const				{ return m_UseDiskCounters != eDontUse; }
	__inline bool UseDiskCounters() const				{ return m_UseDiskCounters; }

	virtual QMultiMap<QString, CDnsCacheEntryPtr> GetDnsEntryList() const;

	virtual QMap<quint64, CPoolEntryPtr> GetPoolTableList() const { QReadLocker Locker(&m_PoolTableMutex); return m_PoolTableList; }

	virtual bool UpdateThreads(CWinProcess* pProcess);

signals:
	void		PoolListUpdated(QSet<quint64> Added, QSet<quint64> Changed, QSet<quint64> Removed);

public slots:
	void		MonitorETW(bool bEnable);
	void		MonitorFW(bool bEnable);
	void		OnNetworkEvent(int Type, quint64 ProcessId, quint64 ThreadId, quint32 ProtocolType, quint32 TransferSize,
								QHostAddress LocalAddress, quint16 LocalPort, QHostAddress RemoteAddress, quint16 RemotePort);
	void		OnDnsResEvent(quint64 ProcessId, quint64 ThreadId, const QString& HostName, const QStringList& Result);
	void		OnFileEvent(int Type, quint64 FileId, quint64 ProcessId, quint64 ThreadId, const QString& FileName);
	void		OnDiskEvent(int Type, quint64 FileId, quint64 ProcessId, quint64 ThreadId, quint32 IrpFlags, quint32 TransferSize, quint64 HighResResponseTime);

	bool		UpdatePoolTable();

protected:
	virtual void OnHardwareChanged();

	quint32		EnumWindows();

	void		AddNetworkIO(int Type, quint32 TransferSize);
	void		AddDiskIO(int Type, quint32 TransferSize);

	bool		InitWindowsInfo();

	//CEventMonitor*			m_pEventMonitor;
	CEtwEventMonitor*			m_pEventMonitor;
	CFwEventMonitor*		m_pFirewallMonitor;
	//CFirewallMonitor*		m_pFirewallMonitor;


	//mutable QReadWriteLock	m_FileNameMutex;
	//QMap<quint64, QString>	m_FileNames;


	// Guard it with		m_OpenFilesMutex
	//QMultiMap<quint64, CHandleRef> m_HandleByObject;

	// Guard it with m_ServiceMutex
	QMap<quint64, QList<QString> > m_ServiceByPID;

	CSymbolProvider*		m_pSymbolProvider;

	CSidResolver*			m_pSidResolver;

	CDnsResolver*			m_pDnsResolver;
	

	// Guard it with		m_StatsMutex
	quint32					m_TotalGuiObjects;
	quint32					m_TotalUserObjects;
	quint32					m_TotalWndObjects;


	/*enum EUseDiskCounters
	{
		eDontUse = 0,
		eForProgramsOnly,
		eUseForSystem
	};
	volatile EUseDiskCounters m_UseDiskCounters;*/
	volatile bool			m_UseDiskCounters;

	mutable QReadWriteLock	m_WindowMutex;
	QHash<quint64, QMultiMap<quint64, quint64> > m_WindowMap;
	QHash<quint64, QPair<quint64, quint64> > m_WindowRevMap;

	mutable QReadWriteLock		m_PoolTableMutex;
	QMap<quint64, CPoolEntryPtr>m_PoolTableList;

private:
	void UpdatePerfStats();
	void UpdateSambaStats();
	quint64 UpdateCpuStats(bool SetCpuUsage);
	quint64 UpdateCpuCycleStats();
	void UpdateCPUCycles(quint64 TotalCycleTime, quint64 IdleCycleTime);
	bool InitCpuCount();

	QString GetFileNameByID(quint64 FileId) const;

	bool m_bTestSigning;
	quint32 m_uDriverStatus;
	quint32 m_uDriverFeatures;
	QString m_DriverFileName;
	QString m_DriverDeviceName;

	struct SWindowsAPI* m;
};

extern quint32 g_fileObjectTypeIndex;
extern quint32 g_EtwRegistrationTypeIndex;

#define CPU_TIME_DIVIDER (10 * 1000 * 1000) // the clock resolution is 100ns we need 1sec