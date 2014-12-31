#include "uv.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
//#include <windows.h>
#include "debug.h"


static int g_log_level=30;


void _uv_init_debug_inner(int level){
    g_log_level = level;       

    debug_print(LL_INFO, "_uv_init_debug_inner: %d %s", level, GetCommandLine());
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
    OutputDebugString(buffer);
    free(buffer);
}
