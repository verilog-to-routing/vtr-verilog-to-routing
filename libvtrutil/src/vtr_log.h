#ifndef VTR_LOG_H
#define VTR_LOG_H

namespace vtr {
typedef void (*PrintHandlerInfo)(const char* pszMessage, ... );
typedef void (*PrintHandlerWarning)(const char* pszFileName, unsigned int lineNum, const char* pszMessage,	... );
typedef void (*PrintHandlerError)(const char* pszFileName, unsigned int lineNum, const char* pszMessage,	... );
typedef void (*PrintHandlerDirect)(const char* pszMessage,	... );

extern PrintHandlerInfo printf_info;
extern PrintHandlerWarning printf_warning;
extern PrintHandlerError printf_error;
extern PrintHandlerDirect printf_direct;

} //namespace


#endif
