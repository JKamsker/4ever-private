#pragma once

#ifdef _WIN32


class CTNICEventSink : public IWbemObjectSink
{
protected:
	virtual void OnMediaDisconnect( IWbemClassObject *pObj);

public:
	void ReleaseTEVENT();

	BYTE InitTEVENT(
		CString& strIP,
		int nTimer);

	BYTE GetMACAddress(
		LPBYTE pMACAddress,
		CString& strIP);

public:
	STDMETHOD(SetStatus)( long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject *pObjParam);
	STDMETHOD(Indicate)( long lObjectCount, IWbemClassObject **ppObjArray);
	STDMETHOD(QueryInterface)( REFIID riid, LPVOID *ppv);

	STDMETHOD_(ULONG, Release)();
	STDMETHOD_(ULONG, AddRef)();

protected:
	IWbemServices *m_pIWBEM;

private:
	ULONG m_nRef;

public:
	CTNICEventSink();
	~CTNICEventSink();
};

#else

// The network card events come from WMI: on the other systems the sink never fires.
struct IWbemClassObject;

class CTNICEventSink
{
protected:
	virtual void OnMediaDisconnect( IWbemClassObject *pObj) {}

public:
	void ReleaseTEVENT() {}

	BYTE InitTEVENT(
		CString& strIP,
		int nTimer) { return FALSE; }

	BYTE GetMACAddress(
		LPBYTE pMACAddress,
		CString& strIP) { return FALSE; }

public:
	CTNICEventSink() {}
	virtual ~CTNICEventSink() {}
};

#endif
