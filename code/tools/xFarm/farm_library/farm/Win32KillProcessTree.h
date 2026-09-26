#ifndef __WIN32_KILLPROCESSTREE_H__
#define __WIN32_KILLPROCESSTREE_H__

#ifdef WIN32
/// kill a process, with the optional ability to kill the entire process tree (all child processes)
BOOL KillProcessEx(IN DWORD dwProcessId, IN BOOL bTree);
#endif

#endif
