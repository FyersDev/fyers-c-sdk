/**
 * @file fyers_export.h
 * @brief Symbol visibility macros for Windows DLL export/import
 */

#ifndef FYERS_EXPORT_H
#define FYERS_EXPORT_H

#ifdef _WIN32
#  ifdef FYERS_API_EXPORTS
#    define FYERS_API __declspec(dllexport)
#  else
#    define FYERS_API __declspec(dllimport)
#  endif
#else
#  define FYERS_API
#endif

#endif /* FYERS_EXPORT_H */
