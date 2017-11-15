#include "ems.h"
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>

#ifdef USE_MULTI_THREAD

/*
 * for process control
 * */
#ifdef WIN32
#pragma comment(lib, "Psapi.lib")
HANDLE threadHandle(ems_threadid tid)
{
	HANDLE h = NULL;
	typedef HANDLE (WINAPI *pfnOpenThread)(DWORD, BOOL, DWORD);
	pfnOpenThread fnOpenThread = NULL;

	fnOpenThread = (pfnOpenThread)
		GetProcAddress(LoadLibrary("kernel32.dll"), "OpenThread");
	if ( !fnOpenThread)
		return NULL;

	h = fnOpenThread(SYNCHRONIZE, 0, tid);
	return h;
}

HANDLE processHandle(ems_processid pid)
{
	return OpenProcess(	SYNCHRONIZE 
				| PROCESS_QUERY_INFORMATION
				| PROCESS_VM_READ,
			0, pid);
}

ems_processid startProcess(const char *cmd, const char *arg)
{
	DWORD ret = 0;
	char buf[1024] = {0};
	PROCESS_INFORMATION pinfo;
	STARTUPINFO si;
	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pinfo, sizeof(pinfo) );

	snprintf(buf, 1024, "%s %s", cmd, arg ? arg: "");
	ret = CreateProcess(NULL, buf, 
				NULL,
				NULL,
				FALSE,
				0,
				NULL,
				NULL,
				&si,
				&pinfo);
	if (!ret)
		return -1;

	return pinfo.dwProcessId;
}
#else
ems_processid startProcess(const char *cmd, const char *arg)
{
	int	pid;
	char	*args[10];

	args[0] = (char *)cmd;
	args[1] = (char *)arg;

	pid = vfork();
	if (pid == 0) {
		setsid();
		if ( execvp(cmd, args) == -1) {
			exit(-1);
		}

		exit(0);
	}

	return pid;
}
#endif


int waitForProcessStop(ems_processid pid)
{
#ifdef WIN32
	HANDLE hClnt = NULL;
	hClnt = OpenProcess(SYNCHRONIZE 
			    | PROCESS_QUERY_INFORMATION 
			    | PROCESS_VM_READ, 0, pid);
	if ( !hClnt)
		return ERR;

	WaitForSingleObject(hClnt, -1);
#else
	waitpid(pid, NULL, 0);
#endif
	return OK;
}

int terminateProcess(ems_processid pid)
{
#ifdef WIN32
	HANDLE hProcess;
	hProcess = OpenProcess(PROCESS_TERMINATE,0, pid);
	TerminateProcess(hProcess,0);
	CloseHandle(hProcess);
#else
	kill(pid, 9);
#endif
	return OK;
}

#ifdef WIN32

static BOOL uQueryFullProcessImageName(
		HANDLE hProcess,
		DWORD flg, 
		LPTSTR lpExeName,
		PDWORD lpdwSize)
{
	BOOL brtn = 0;
	char proc[MAX_PATH] = {0};
	char *pch = NULL;
	DWORD sz = 0;

typedef BOOL (WINAPI *funcQueryFullProcessImageName)(HANDLE,DWORD, LPTSTR, PDWORD);

	funcQueryFullProcessImageName QueryFullProcessImageName 
			= (funcQueryFullProcessImageName)
				GetProcAddress(GetModuleHandleA("kernel32"),
						"QueryFullProcessImageNameA");

	brtn = 0;
	sz = *lpdwSize;
	if ( QueryFullProcessImageName)
		brtn = QueryFullProcessImageName(hProcess, flg, proc, &sz);
 
	if ( brtn ) {
		pch = strrchr(proc, '\\');
		if ( pch)
			*lpdwSize = snprintf(lpExeName, *lpdwSize, "%s", pch+1);
		else
			*lpdwSize = snprintf(lpExeName, *lpdwSize, "%s", proc);
	}

	return brtn;
}

int getProcessName(HANDLE hProcess, char *szProcessName, int l_dst)
{
	HMODULE hMod;
	DWORD sz = 0;

	sz = l_dst;
	if ( !uQueryFullProcessImageName(
				hProcess, 
				0,
				szProcessName,
				&sz)) {
		if (EnumProcessModules( hProcess, &hMod, sizeof(hMod), &sz)) {
			if (!GetModuleBaseName(hProcess, hMod, szProcessName, sz))
				return ERR;
		} else
			return ERR;
	}

	return OK;
}

int processNameByPid(DWORD pid, char *dst, int l_dst)
{
	HANDLE hProcess;
	int rtn = ERR;

	hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
				   PROCESS_VM_READ,
				   FALSE, pid );
	if (NULL != hProcess ) {
		rtn = getProcessName(hProcess, dst, l_dst);
		CloseHandle( hProcess );
	}

	return rtn;
}


static int ProcessIs(const char *procName, DWORD processID)
{
	int rtn = 0;
	TCHAR szProcessName[MAX_PATH] ={0};
	DWORD sz;

	sz = sizeof(szProcessName) / sizeof(TCHAR);
	if (processNameByPid(processID, szProcessName, sz) != OK) {
		return rtn;
	}

	if (_stricmp(procName, szProcessName)) {
		rtn = 1;
	}

	return rtn;
}

void terminateProcessByName(char *proc)
{
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) ) {
		return;
	}

	cProcesses = cbNeeded / sizeof(DWORD);

	for ( i = 0; i < cProcesses; i++ ) {
		if( aProcesses[i] != 0 ) {
			if ( ProcessIs(proc, aProcesses[i])) {
				terminateProcess(aProcesses[i]);
			}
		}
	}
}

void terminateProcessByNameAndExceptPid(char *proc, DWORD except_pid)
{
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) ) {
		return;
	}

	cProcesses = cbNeeded / sizeof(DWORD);

	for ( i = 0; i < cProcesses; i++ ) {
		if( aProcesses[i] != 0 && except_pid != aProcesses[i] ) {
			if ( ProcessIs(proc, aProcesses[i])) {
				terminateProcess(aProcesses[i]);
			}
		}
	}
}

int processID(char *proc)
{
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) ) {
		return -1;
	}

	cProcesses = cbNeeded / sizeof(DWORD);
	for ( i = 0; i < cProcesses; i++ ) {
		if( aProcesses[i] != 0 ) {
			if ( ProcessIs(proc, aProcesses[i])) {
				return aProcesses[i];
			}
		}
	}

	return -1;
}

DWORD shellExecuteWith(const char *_app, const char *_param, const char *_dir, int wait)
{
	SHELLEXECUTEINFO sinfo;

	memset(&sinfo, 0, sizeof(SHELLEXECUTEINFO));
	sinfo.cbSize = sizeof(SHELLEXECUTEINFO);
        sinfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        sinfo.hwnd = NULL;
        sinfo.lpVerb = NULL;
        sinfo.lpFile = _app;
        sinfo.lpParameters = _param;
        sinfo.lpDirectory = _dir;
        sinfo.nShow = SW_SHOW;
        sinfo.hInstApp = NULL;

	if (!ShellExecuteEx(&sinfo)) {
		return ERR;
	}

	if (!sinfo.hProcess) return ERR;
	if ( wait){
		WaitForSingleObject(sinfo.hProcess, -1);
		CloseHandle(sinfo.hProcess);
		return OK;
	}

	return GetProcessId(sinfo.hProcess);
}

DWORD shellExecuteRunAsWith(const char *_app, const char *_param, const char *_dir, int wait)
{
	SHELLEXECUTEINFO sinfo;

	memset(&sinfo, 0, sizeof(SHELLEXECUTEINFO));
	sinfo.cbSize = sizeof(SHELLEXECUTEINFO);
        sinfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        sinfo.hwnd = NULL;
        sinfo.lpVerb = "runas";
        sinfo.lpFile = _app;
        sinfo.lpParameters = _param;
        sinfo.lpDirectory = _dir;
        sinfo.nShow = SW_SHOW;
        sinfo.hInstApp = NULL;

	if (!ShellExecuteEx(&sinfo)) {
		return ERR;
	}

	if (!sinfo.hProcess) return ERR;
	if ( wait){
		WaitForSingleObject(sinfo.hProcess, -1);
		CloseHandle(sinfo.hProcess);
		return OK;
	}

	return GetProcessId(sinfo.hProcess);
}

int startProcessAs(const char *_app, const char *_params)
{
	SHELLEXECUTEINFO sinfo;

	memset(&sinfo, 0, sizeof(SHELLEXECUTEINFO));
	sinfo.cbSize = sizeof(SHELLEXECUTEINFO);
        sinfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        sinfo.hwnd = NULL;
        sinfo.lpVerb = "runas";
        sinfo.lpFile = _app;
        sinfo.lpParameters = _params;
        sinfo.lpDirectory = NULL;
        sinfo.nShow = SW_SHOW;
        sinfo.hInstApp = NULL;

	if (!ShellExecuteEx(&sinfo)) {
		return ERR;
	}

	if (!sinfo.hProcess) return ERR;
	{
		WaitForSingleObject(sinfo.hProcess, -1);
		CloseHandle(sinfo.hProcess);
	}

	return OK;
}


DWORD 
startProcessWith(const char *_app, const char *_path, const char *desktop)
{
	STARTUPINFO         si  = {0};
	PROCESS_INFORMATION pi  = {0};

	si.cb           = sizeof(si);
	si.lpTitle   = (char *)desktop;
	si.lpDesktop = (char *)desktop;
	if (!CreateProcess(
				NULL,
				(char *)_app,
				NULL,
				NULL,
				FALSE,  
				0,       
				NULL,    
				_path,
				&si,     
				&pi))
		return ERR;

	return pi.dwProcessId;
}


#endif

ems_int ems_threadcreate(ems_threadid *tid, thread_entry func, ems_threadarg arg)
{
#ifdef WIN32
	if (CreateThread(NULL, 0, 
		(LPTHREAD_START_ROUTINE)func, arg, 0, tid) == NULL)
		return -1;

	return 0;
#else
	return pthread_create(tid, NULL, func, arg);
#endif
}

ems_int ems_threadjoin(ems_threadid tid)
{
#ifdef WIN32
	HANDLE h = NULL;
	h = threadHandle(tid);
	if ( !h)
		return ERR;
	WaitForSingleObject(h, -1);
	CloseHandle(h);
#else
	pthread_join(tid, NULL);
#endif

	return OK;
}
#endif


ems_int ems_bin2str(ems_cchar *s, ems_int len, ems_char *d, ems_int d_l)
{
	ems_char *p = d;
	ems_int   ret = 0;
	ems_int   l = 0;
	ems_char  ch;

	l = len;

	while (len > 0 && d_l > 0) {
		ch = *s++;
		ret = snprintf(p, d_l, "%02x", (ch&0xff));

		len--;

		p   += ret;
		d_l -= ret;
	}

	return (l - len);
}

ems_int ems_str2bin(ems_cchar *s, ems_char *d, ems_int d_l)
{
	ems_int  total = 0;
	ems_char tmp[4] = {0};
	ems_int  l = d_l;

	if (!s)
		return -1;

	total = strlen(s) / 2;
	while (total > 0 && d_l > 0) {
		memcpy(tmp, s, 2);

		*d = (ems_char )(strtol(tmp, NULL, 16)&0xff);
		d++;
		total--;
		d_l--;
		s = s + 2;
	}

	return (l-d_l);
}


ems_int ems_cpucore()
{
	return (ems_int) sysconf(_SC_NPROCESSORS_ONLN);
}

ems_int ems_pagesize()
{
	return (ems_int) sysconf(_SC_PAGESIZE);
}

ems_char *ems_trim(ems_char *src)
{
	ems_char *s = NULL, *e = NULL;
	ems_char *buf = NULL;
	ems_int l;

	buf = ems_strdup(src);
	if (!buf)
		return src;

	l = strlen(src);
	s = buf;
	e = s + l - 1;
	while (s < e) {
		if ((*s == ' ') || (*s=='\t') || (*s =='\n') || (*s =='\r'))
			s++;
		else if ((*e==' ') || (*e =='\t') || (*e=='\n') || (*e=='\r'))
			e--;
		else
			break;
	}

	if ((s >= e) && (*s == ' ')) {
		*src = '\0';
		ems_free(buf);
		return NULL;
	}

	e++;
	*e = '\0';

	snprintf(src, l+1, "%s", s);

	ems_free(buf);

	return src;
}

ems_cchar *ems_geterrmsg(ems_int err)
{
	static ems_char buf_err[128] = {0};

	snprintf(buf_err, 128, "(%d): %s", err, strerror(err));

	return buf_err;
}


ems_int ems_cpuusage()
{
	FILE* file;
	double percent;
	static unsigned long long l_user, l_userlow, l_sys, l_idle;
	unsigned long long user, userlow, sys, idle;
	unsigned long long diff_total, diff_used;

	file = fopen("/proc/stat", "r");
	fscanf(file, "cpu %Ld %Ld %Ld %Ld", &user, &userlow, &sys, &idle);
	fclose(file);

	diff_used  = user - l_user + userlow - l_userlow + sys - l_sys;
	diff_total = diff_used + idle - l_idle; 

	percent = -1;
	if (diff_total > 0) {
		percent = 100 * diff_used / diff_total;
	}

	if (percent < 0 ) {
		percent = 1.0;
	}

	l_user    = user;
	l_userlow = userlow;
	l_sys     = sys;
	l_idle    = idle;

	return (ems_int)percent;
}

unsigned long gettotal(ems_cchar *cmd)
{
	char buf[512] = {0};
	unsigned long total;
	FILE  *fp;

	fp = popen(cmd, "r");
	if (!fp)
		return 0;

	total = 0;
	while (fgets(buf, sizeof(buf), fp)) 
	{
		total += strtol(buf, NULL, 10);
	}

	pclose(fp);

	return total;
}


ems_int ems_memusage()
{
	unsigned long total, used;

	total = gettotal("grep -E \"MemTotal|SwapTotal\" /proc/meminfo | awk '{print $2}'");

	used  = total - 
		gettotal("grep -E \"MemFree|Buffers|Cached|SwapCached|SwapFree\" /proc/meminfo | awk '{print $2}'");

	return (int) ((double)used * 100 / total);
}

ems_int ems_reset_rlimit()
{
	struct rlimit rl, old; 

	if (getrlimit(RLIMIT_CORE, &rl) == 0) {
		rl.rlim_cur = rl.rlim_max;
		if (setrlimit(RLIMIT_CORE, &rl))
			printf("set RLIMIT_CORE failed, try run this by root: %s\n", strerror(errno));
	}

	getrlimit(RLIMIT_NOFILE, &old);

	rl.rlim_cur = rl.rlim_max = 65000;
	if (setrlimit(RLIMIT_NOFILE, &rl)) {
		if (errno == EPERM) {
			rl.rlim_cur = rl.rlim_max = old.rlim_max;
			if (setrlimit(RLIMIT_NOFILE, &rl))
				printf("sysrlimit RLIMIT_NOFILE failed: %s\n", strerror(errno));

		} else
			printf("sysrlimit RLIMIT_NOFILE failed: %s\n", strerror(errno));
	}

	return 0;
}

ems_cchar *ems_itoa(ems_int i)
{
	static ems_char buf[64];
	snprintf(buf, 64, "%d", i);
	return buf;
}

ems_int  ems_atoi(ems_cchar *str)
{
	if (str)
		return strtol(str, NULL, 10);

	return 0;
}


