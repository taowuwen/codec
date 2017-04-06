
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Psapi.h>
#endif

#include "dt.h"


#ifdef WIN32
#pragma comment(lib, "ws2_32.lib")
#endif


dt_int dt_gethostbyname(dt_cchar *domain, struct sockaddr_in *dst)
{
	struct hostent	*remote;
	struct in_addr   addr;
#ifdef DEBUG
	dt_int i = 0;
	dt_char        **pAlias;
#endif

	dt_assert(domain && dst);

	remote = gethostbyname(domain);
	if (!remote && !isalpha(domain[0])) {
		addr.s_addr = inet_addr(domain);
		if (addr.s_addr != INADDR_NONE)
			remote = gethostbyaddr((char *) &addr, 4, AF_INET);
	}

	if (!remote)
		return ERR;

#ifdef DEBUG
	log_trace("Official name: %s", remote->h_name);
	for (pAlias = remote->h_aliases; *pAlias != 0; pAlias++) {
		log_trace("Alternate name #%d: %s", ++i, *pAlias);
	}

	switch (remote->h_addrtype) {
		case AF_INET:
			log_trace("Address type: AF_INET");
		break;
		case AF_INET6:
			log_trace("Address type: AF_INET6");
		break;
#ifdef WIN32
		case AF_NETBIOS:
			log_trace("Address type: AF_NETBIOS");
		break;
#endif
		default:
			log_trace("Address type:  %d", remote->h_addrtype);
		break;
	}

	log_trace("Address length: %d", remote->h_length);

	if (remote->h_addrtype == AF_INET) {
		i = 0;
		while (remote->h_addr_list[i] != NULL) {
			memset(&addr, 0, sizeof(addr));
			addr.s_addr = *(u_long *) remote->h_addr_list[i];
			log_trace("IPv4 #%d: %s", i, inet_ntoa(addr));
			i++;
		}
	} else if (remote->h_addrtype == AF_INET6)
		log_trace("Remotehost is an IPv6 address");
#endif

	dst->sin_addr.s_addr = *(u_long *) remote->h_addr_list[0];
	memcpy(&dst->sin_addr, remote->h_addr, remote->h_length);

	return OK;
}

#ifdef WIN32
dt_int dt_setnonblocking(dt_int sockfd, dt_int yes)
{
	dt_ulong opts = 1;

	if (!yes) 
		opts = 0;

	if (SOCKET_ERROR == ioctlsocket(sockfd, FIONBIO, (dt_ulong *)&opts))
		return ERR;

	return OK;
}
#else
dt_int dt_setnonblocking(dt_int sockfd, dt_int yes)
{
	int opts;

	opts = fcntl(sockfd, F_GETFL);

	opts = (opts | O_NONBLOCK);
	if ( !yes )
		opts = (opts ^ O_NONBLOCK);

	if (fcntl(sockfd, F_SETFL,opts) < 0)
		return ERR;

	return OK;
}
#endif


/*
 * which gonna cause send or recv return in msecs/1000 seconds.
 * check  whether errno == EAGAIN and try again.
 * WSAETIMEDOUT on windows gonna return : check
 * http://msdn.microsoft.com/en-us/library/windows/desktop/ms740476(v=vs.85).aspx
 * */
dt_int dt_setsock_rw_timeout(dt_int sockfd, dt_int msecs)
{
#ifdef WIN32
	dt_int to = msecs;
	setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (dt_char*)&to, sizeof(to));
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (dt_char*)&to, sizeof(to));
#else
	struct timeval to = {msecs/1000, 0};

	setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &to, sizeof(to));
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to));
#endif

	return OK;
}

dt_char *dt_trim(dt_char *src)
{
	dt_char *s = NULL, *e = NULL;
	dt_char *buf = NULL;
	dt_int l;

	buf = dt_strdup(src);
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
		dt_free(buf);
		return NULL;
	}

	e++;
	*e = '\0';

	snprintf(src, l+1, "%s", s);

	dt_free(buf);

	return src;
}

#ifdef WIN32
#endif

#ifdef WIN32
#pragma comment(lib, "Psapi.lib")
#endif



/*
 * for process control
 * */
#ifdef WIN32
HANDLE threadHandle(dt_threadid tid)
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

HANDLE processHandle(dt_processid pid)
{
	return OpenProcess(	SYNCHRONIZE 
				| PROCESS_QUERY_INFORMATION
				| PROCESS_VM_READ,
			0, pid);
}

dt_processid startProcess(const char *cmd, const char *arg)
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
dt_processid startProcess(const char *cmd, const char *arg)
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


int waitForProcessStop(dt_processid pid)
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

int terminateProcess(dt_processid pid)
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

dt_int dt_threadcreate(dt_threadid *tid, thread_entry func, dt_threadarg arg)
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

dt_int dt_threadjoin(dt_threadid tid)
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


dt_int dt_bin2str(dt_cchar *s, dt_int len, dt_char *d, dt_int d_l)
{
	dt_char *p = d;
	dt_int   ret = 0;
	dt_int   l = 0;

	l = len;

	while (len > 0 && d_l > 0) {
		ret = snprintf(p, d_l, "%x", (*s&0xff));

		s++;
		len--;

		p   += ret;
		d_l -= ret;
	}

	return (l - len);
}

dt_int dt_str2bin(dt_cchar *s, dt_char *d, dt_int d_l)
{
	dt_int  total = 0;
	dt_char tmp[4] = {0};
	dt_int  l = d_l;

	if (!s)
		return -1;

	total = strlen(s) / 2;
	while (total > 0 && d_l > 0) {
		memcpy(tmp, s, 2);

		*d = (dt_char )(strtol(tmp, NULL, 16)&0xff);
		d++;
		total--;
		d_l--;
		s = s + 2;
	}

	return (l-d_l);
}
