#include "uv.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
//#include <windows.h>
#include "debug.h"


static int g_log_level=30;
static int g_use_ods=1;
static int g_use_stderr=1;
static int g_show_pid=0;


void _uv_init_debug_inner(int level, int use_ods, int use_stderr, int show_pid){
    debug_print(LL_INFO, "_uv_init_debug_inner: %8d %d ODS:%d stderr:%d %s", GetCurrentProcessId(), level, use_ods, use_stderr, GetCommandLine());

    g_log_level = level;       
    g_use_ods = use_ods;
    g_use_stderr = use_stderr;    
    g_show_pid = show_pid;

    if(!(g_use_ods || g_use_stderr)){
        g_log_level=LL_NO_LOG;
    }
    
}

void debug_print(int level, const char *fmt, ...)    
{
    int n, size=100;
    va_list ap;
    char* buffer = NULL;      

    
    if(level<g_log_level){
        return;
    }
    
    while (1) {
        char* newbuffer = (char*)realloc(buffer, size);
        if(newbuffer==NULL){
            free(buffer);
            return;
        }
        buffer = newbuffer;
        va_start(ap, fmt);
        n = vsnprintf(buffer, size, fmt, ap);
        //n = _vsn
        va_end(ap);
        if (n > -1 && n < size){
            break;
        }                
        if (n > -1)
            size=n+1;
        else
            size*=2;
    }
    if(g_use_ods){
        OutputDebugString(buffer);
    }
    if(g_use_stderr){
        DWORD pid = g_show_pid?GetCurrentProcessId():0;
        fprintf(stderr, "[%8d] %s\n", pid, buffer);
    }
    free(buffer);
}
