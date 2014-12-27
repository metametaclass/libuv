#include <stdlib.h>
//#include <varargs.h>
#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

void debug_print(const char *fmt, ...)    
{
    int n, size=100;
    va_list ap;
    char* buffer = NULL;

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
