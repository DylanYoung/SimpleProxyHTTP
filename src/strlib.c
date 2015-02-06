/*
 =============================================================
 Name        : proxy.c
 Author      : Casey Meijer -- A00337010
 Version     : 0.0.1
 License     : MIT
 Description : String manipulation functions

 =============================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* assert strlen(new) <= strlen(old) */
char * replace_str(char * source, char * old, char * new)
{
    char * start = strstr(source, old);
    if ( start == NULL || strlen(new) > strlen(old) )
        return source;
    size_t difference = strlen(old)-strlen(new);
    char * rest = start+strlen(old);
    memcpy(start, new, strlen(new));
    memmove(start+strlen(new),rest,strlen(rest));
    memset(source+strlen(source)-difference, 0, difference);

    return source;
}

char * repl_tween_str(char * source, char * start,
                        char * end, char * new)
{
    start = strstr(source, start) + strlen(start);
    end = strstr(start, end);
    if(start == NULL || end == NULL)
        return source;
    size_t length = end-start;
    size_t difference = length - strlen(new);
    if (strlen(new) > length)
        return source;
    memcpy(start, new, strlen(new));
    memmove(start+strlen(new), end, strlen(end));
    memset(source+strlen(source)-difference, 0, difference);
    return source;
}

/* remember to free(result)!!! */
char * get_tween_str(char * source, char * start, char * end)
{
    start = strstr(source, start)+strlen(start);
    end = strstr(start, end);
    size_t length = end - start;
    char * result = malloc((length+1)*sizeof(char));
    memcpy(result, start, length);
    memset(result+length+1,0,1);
    return result;
}
