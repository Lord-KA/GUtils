#ifndef GUTILS_H
#define GUTILS_H

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>


/**
 * @brief check is expression is true, otherwise writes error message to logStream and returnes provided error code
 * @param expr expression to check
 * @param errCode error code to return if false
 * @param errMsg  s-ctyle null-terminated string with an error message to write to log stream
 * @param logStream log stream to write message to
 */
#ifndef NDEBUG
#define ASSERT_LOG(expr, errCode, errMsg, logStream) ({                              \
    if (!(expr)) {                                                                    \
        if (logStream != NULL)                                                         \
            fprintf((logStream),  "%s in %s on line %d!\n", errMsg, __func__, __LINE__);\
        return (errCode);                                                                \
    }                                                                                     \
})
#else
#define ASSERT_LOG(expr, errCode, errMsg, logStream)
#endif


/** 
 * @brief macro that are used in c-style pseudo-templates 
 */
#define CONCATENATE(A, B) A##_##B                   
#define TEMPLATE(func, TYPE) CONCATENATE(func, TYPE)


/**
 * @brief checks if pointer is valid, that uses system-based checks if GUTILS_USE_PTR_SYS_CHECK defined
 * @param ptr pointer to check
 * @return true if valid, false otherwise
 */
static bool gPtrValid(const void* ptr)       
{
    if (ptr == 0) {
        return false;
    }
  
    #ifdef GUTILS_USE_PTR_SYS_CHECK
        #ifdef __unix__
            size_t page_size = sysconf(_SC_PAGESIZE);
            void *base = (void *)((((size_t)ptr) / page_size) * page_size);
            return msync(base, page_size, MS_ASYNC) == 0;
        #else
            #ifdef _WIN32
                MEMORY_BASIC_INFORMATION mbi = {};
                if (!VirtualQuery(ptr, &mbi, sizeof (mbi)))
                    return false;
  
                if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
                    return false;  // Guard page -> bad ptr
    
                DWORD readRights = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY
                    | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
    
                return (mbi.Protect & readRights) != 0;
            #else   
                fprintf(stderr, "WARNING: your OS is unsupported, system pointer checks are diabled!\n");
            #endif  /* _WIN32 */
        #endif  /* __unix__ */
    #endif  /* GUTILS_USE_PTR_SYS_CHECK */
  
    return true;
}

/**
 * @brief finds the first occurrence of needle in haystack with known length
 * @param haystack c-style string to check
 * @param needle char to look for
 * @length length of haystack string
 */
static char* strnchr(const char *haystack, char needle, size_t length) 
{
    return (char*)memchr(haystack, needle, strnlen(haystack, length));

}


/**
 * @brief checks if haystack has any chars from needles
 * @haystack c-style string to check
 * @needles   array of chars to search for
 * @haystackLen length of haystack string
 * @needlesLen  length of needles array
 * @return true if haystack has a char from needle, false otherwise
 */
static bool strnConsistsChrs(const char *haystack, const char* needles, size_t haystackLen, size_t needlesLen)
{
    char *iter = (char*)needles;
    while ((iter - needles) < needlesLen && *iter != '\0') {
        if (strnchr(haystack, *iter, haystackLen))
            return 1;
        ++iter;
    }
    return 0;
}


/**
 * @brief checks if haystack is a correct integer with some surrounding spaces, supports hexadecimal and octal integers
 * @haystack null-terminated c-style string to check
 * @return true if it is a correct integer, false otherwise
 */
static bool isInteger(const char *haystack) 
{
    /* 
     * WARNING: haystack must be a null-terminated string
     */

    bool hexadecimalMode = false;
    char *iter = (char*)haystack;
    while (isspace(*iter))
        ++iter;

    /* for hexadecimal and octal support */
    if (iter - haystack < strlen(haystack) - 1
            && *iter == '0'
            && (*(iter + 1) == 'o' || *(iter + 1) == 'x')) {
        if (*(iter + 1) == 'x')
            hexadecimalMode = true;
        iter += 2;
    }
    if (hexadecimalMode) {
        while (isdigit(*iter) || ('a' <= *iter && *iter <= 'f'))
            ++iter;
    } else {
        while (isdigit(*iter))
            ++iter;
    }

    while (isspace(*iter))
        ++iter;
    if (*iter == '\0')
        return true;
    else 
        return false;
}


/**
 * @brief checks if haystack is a correct double number with some surrounding spaces, supports scientific notation
 * @haystack null-terminated c-style string to check
 * @return true if it is a correct double, false otherwise
 */
static bool isDouble(const char *haystack) {
    /* 
     * WARNING: haystack must be a null-terminated string
     */

    char *iter = (char*)haystack;
    while (isspace(*iter))
        ++iter;

    do {
        if (!isdigit(*iter))
            return false;
        ++iter;
    } while (*iter != '\0' && *iter != '.');

    do {
        ++iter;
    } while (isdigit(*iter));
    if (*iter == '\0')
        return true;

    if (*iter != 'e')
        return false;

    if (iter - haystack >= strlen(haystack) - 2)
        return false;
    ++iter;
    if (*iter != '+' && *iter != '-')
        return false;
    ++iter;
    while (*iter != '\0' && !isspace(*iter)) {
        if (!isdigit(*iter))
            return false;
        ++iter;
    }
    while (isspace(*iter))
        ++iter;
    if (*iter == '\0')
        return true;
    else 
        return false;
}


/**
 * @brief gets line from input stream and writes it in buffer with length bufferLen
 * @brief buffer c-style string to write line to, will add NULL after the lines end
 * @brief bufferLen length of the provided buffer
 * @brief in filestream to read from
 * @return 0 if finished successfully, 1 otherwise
 */
static bool getline(char *buffer, size_t bufferLen, FILE *in) 
{
    assert(gPtrValid(buffer));
    assert(gPtrValid(in));

    char c;
    size_t curLen = 0;
    c = fgetc(in);
    if (ferror(in))
        return 1;
    if (c == '\n' && in == stdin)
        c = fgetc(in);
    while (!feof(in) && c != '\n') {
        if (curLen < bufferLen - 1) 
            buffer[curLen++] = c;
        else 
            goto finish;
        c = fgetc(in);
    }
    if (ferror(in))
        return 1;
finish:
    buffer[curLen++] = '\0';
    return 0;
}


/**
 * @brief compares strings with skipping all space chars
 * @param firstIter  pointer (iterator) to one c-style string 
 * @param secondIter pointer (iterator) to other c-style string 
 * @param direction equals 1 or -1 dependent on the intended direction of iterators 
 * @return -1 if first < second; 1 if first > second; 0 if first == second
 */
int strSkpCmp(char* firstIter, char* secondIter, int direction)
{
    assert(direction == 1 || direction == -1);
    assert(gPtrValid(firstIter));
    assert(gPtrValid(secondIter));

    while (*firstIter != '\0' && *secondIter != '\0') {
        if (!isspace(*(firstIter)))
            firstIter += direction;
        else if (!isspace(*(secondIter)))
            secondIter += direction;
        else {
            if (*(secondIter) < *(firstIter)) 
               return 1;
            if (*(secondIter) > *(firstIter))
               return -1;
            firstIter += direction;
            secondIter += direction;
        }
    }
    while (!isspace(*firstIter) && *firstIter != '\0')
        firstIter += direction;

    while (!isspace(*secondIter) && *secondIter != '\0')
        secondIter += direction;

    if (*firstIter < *secondIter) 
        return -1;
    else if (*firstIter > *secondIter)
       return 1;

    return 0;
}


/**
* @brief compares first N non-space chars in strings
* @param firstIter  pointer (iterator) to one c-style string 
* @param secondIter pointer (iterator) to other c-style string 
* @param length number of non-space chars to compare
* @return -1 if first < second; 1 if first > second; 0 if first == second
*/
int strnSkpCmp(char* firstIter, char* secondIter, size_t length)
{
    assert(gPtrValid(firstIter));
    assert(gPtrValid(secondIter));

    while (*firstIter != '\0' && *secondIter != '\0' && length > 0) {
        if (isspace(*(firstIter)))
            ++firstIter;
        else if (isspace(*(secondIter)))
            ++secondIter;
        else {
            --length;
            if (*(secondIter) < *(firstIter)) 
                return 1;
            if (*(secondIter) > *(firstIter))
                return -1;
            ++firstIter;
            ++secondIter;
        }
    }
    if (length == 0)
        return 0;

    while (!isspace(*firstIter) && *firstIter != '\0')
        ++firstIter;

    while (!isspace(*secondIter) && *secondIter != '\0')
        ++secondIter;

    if (*firstIter < *secondIter) 
        return -1;
    else if (*firstIter > *secondIter)
        return 1;

   return 0;
}


/**
 * @brief chechs if string is some form of YES with skipping spaces
 * @param buffer string to check
 * @return true if str is YES, false otherwise
 */
bool strIsYes(char *buffer) 
{
    assert(gPtrValid(buffer));
    if (!strSkpCmp(buffer, "Yes", 1))
        return 1;
    if (!strSkpCmp(buffer, "YES", 1))
        return 1;
    if (!strSkpCmp(buffer, "yes", 1))
        return 1;
    if (!strSkpCmp(buffer, "Y",   1))
        return 1;
    if (!strSkpCmp(buffer, "y",   1))
        return 1;
    return 0;
}


/**
 * @brief chechs if string is some form of NO with skipping spaces
 * @param buffer string to check
 * @return true if str is NO, false otherwise
 */
bool strIsNo(char *buffer) 
{
    assert(gPtrValid(buffer));
    if (!strSkpCmp(buffer, "No", 1))
        return 1;
    if (!strSkpCmp(buffer, "NO", 1))
        return 1;
    if (!strSkpCmp(buffer, "no", 1))
        return 1;
    if (!strSkpCmp(buffer, "N",  1))
        return 1;
    if (!strSkpCmp(buffer, "n",  1))
        return 1;
    return 0;
}


/**
 * @brief chechs if string is some form of QUIT with skipping spaces
 * @param buffer string to check
 * @return true if str is QUIT, false otherwise
 */
bool strIsQuit(char *buffer) 
{
    assert(gPtrValid(buffer));
    if (!strSkpCmp(buffer, "Quit", 1))
        return 1;
    if (!strSkpCmp(buffer, "quit", 1))
        return 1;
    if (!strSkpCmp(buffer, "QUIT", 1))
        return 1;
    if (!strSkpCmp(buffer, "Q",    1))
        return 1;
    if (!strSkpCmp(buffer, "q",    1))
        return 1;
    return 0;
}

#endif
