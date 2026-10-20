#pragma once

#ifdef _WIN32

class CTConsole
{
private:
	HANDLE m_hConsole;

public:
	bool Create(LPCSTR szTitle, bool bNoClose = false);
	
	void Color(WORD wColor = NULL);
	void Output(LPCSTR szOutput = NULL, ...);
	
	void SetTitle(LPCSTR szTitle);
	char* GetTitle();

	HWND GetHWND();
	HANDLE GetHandle();

	void Show(bool bShow = true);
	void DisableClose();
	void Clear();

	void Close();

	bool IsValid() const { return m_hConsole? true: false; }

public:
	CTConsole();
	~CTConsole();

	static CTConsole* GetInstance();
};

#ifdef TRACE
#undef TRACE
#endif

void TRACE(LPCSTR szOutput = NULL, ...);
#else

// No console window on the other systems: the output goes to stderr.
class CTConsole
{
public:
	bool Create( LPCSTR szTitle, bool bNoClose = false) { return true; }

	void Color( WORD wColor = 0) {}
	void Output( LPCSTR szOutput = NULL, ...)
	{
		if(!szOutput)
			return;

		va_list args;
		va_start( args, szOutput);
		vfprintf( stderr, szOutput, args);
		va_end(args);
	}

	void SetTitle( LPCSTR szTitle) {}
	char *GetTitle() { return NULL; }

	HWND GetHWND() { return NULL; }
	HANDLE GetHandle() { return NULL; }

	void Show( bool bShow = true) {}
	void DisableClose() {}
	void Clear() {}

	void Close() {}

	bool IsValid() const { return true; }

public:
	static CTConsole *GetInstance()
	{
		static CTConsole vCONSOLE;
		return &vCONSOLE;
	}
};
#endif
