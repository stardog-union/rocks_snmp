/****************************************************************
*   Module:  logging.cpp
*  Purpose:  Common logging interface for Windows and Linux
*         :
*   Author:  matthewv
*  Created:  03/23/05
*Revisions:
   $Header: //depot/code/main/porting/logging.cpp#8 $
****************************************************************/

/****************************************************************
 * Copyright 2005
 * Matthew Von-Maszewski
 * All Rights Reserved
 ****************************************************************/

/****************************************************************
 *  File / Environment #defines
 ****************************************************************/

/****************************************************************
 *  Includes
 ****************************************************************/

#include "logging.h"

#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

/****************************************************************
 *  Debug
 ****************************************************************/

/****************************************************************
 *  #defines, typedefs, enum
 ****************************************************************/

/****************************************************************
 *  Static data
 ****************************************************************/

#ifdef WIN32
HANDLE LoggingWindows::m_EventLog = NULL;
LoggingWindows gLogging;
#else
LoggingLinux gLogging;
#endif

// Use of these constants allows the logging to still work
//  if needed before construction or after destruction of
//  the Logging object
char gLogName[LOG_NAME_LENGTH + 1] = {0};
char gLogProgram[LOG_NAME_LENGTH + 1] = {0};
unsigned gLogHandle = 0;
unsigned gLogLevel = LOG_WARNING;
int gLogFacility = LOG_USER;
bool gLogStderr = false;

/****************************************************************
 *  Classes
 ****************************************************************/

/****************************************************************
 * Function:  LoggingClass::LogBinary
 *  Purpose:  Place an "area" prefix on the Prefix and move on
 *         :
 *Revisions:
 *   Date      By      Description (LoggingClass::LogBinary)
 * -------- -------- -------------------------------------------
 * 07/09/07 matthewv Created
 ****************************************************************/
void LoggingClass::LogBinary(
    unsigned Priority,  // Priority of the message
    const char *Prefix, // String for start of binary dump
    const void *Data,   // pointer to binary data to dump
    unsigned DataSize)  // size of the binary data
{
  char buffer[512], hex[16 * 3 + 1], ascii[16 + 1], *hex_ptr, *ascii_ptr;
  unsigned offset;
  const unsigned char *data;
  va_list va_null;

  data = reinterpret_cast<const unsigned char *>(Data);

  for (offset = 0; offset < DataSize; offset += 16) {
    int loop, value;
    unsigned index;

    for (loop = 0, hex_ptr = hex, ascii_ptr = ascii, index = offset; loop < 16;
         ++loop, hex_ptr += 3, ++ascii_ptr, ++index) {
      if (index < DataSize) {
        value = *(data + index);
        snprintf(hex_ptr, 4, "%02X ", value);

        if (isprint(value))
          *ascii_ptr = (char)value;
        else
          *ascii_ptr = '.';
      } // if
      else {
        strcpy(hex_ptr, "   ");
        *ascii_ptr = ' ';
      } // else
    }   // for

    *hex_ptr = '\0';
    *ascii_ptr = '\0';
    strcpy(buffer, Prefix);
    strcat(buffer, hex);
    strcat(buffer, ascii);

    vLog(Priority, buffer, va_null);
  } // for

  return;

} // LoggingClass::LogBinary

#ifdef WIN32
/****************************************************************
 * Function:  LoggingWindows::Open
 *  Purpose:  Do a formal initialization of the logging stream
 *         :   NOT THREAD SAFE (but threads likely not an issue)
 *Revisions:
 *   Date      By      Description (LoggingWindows::Open)
 * -------- -------- -------------------------------------------
 * 04/07/05 matthewv Created
 ****************************************************************/
void LoggingWindows::Open(
    const char *LogName,    // name of the log to receive entries (WINDOWS)
    const char *LogProgram, // name of program generating the entries
    int LogFacility,        // syslog routing of entries (LINUX)
    bool /*StderrLog*/)     // also log to stderr
{

  // log could be open due to logging during start up, or we could be
  //  dynamically changing the LogName or LogProgram
  if (0 != gLogHandle)
    Close();

  // make a static copy of the names for later use
  strncpy(gLogName, LogName, LOG_NAME_LENGTH);
  gLogName[LOG_NAME_LENGTH] = '\0';

  strncpy(gLogProgram, LogProgram, LOG_NAME_LENGTH);
  gLogProgram[LOG_NAME_LENGTH] = '\0';

  gLogFacility = LogFacility;

  // open the log and mark flag
  //  for now, just mark log as "open"
  gLogHandle = 1;

  // Get NT Event log into play too
  if (NULL != m_EventLog)
    DeregisterEventSource(m_EventLog);

  // NULL says log on local server, returns zero on failure ... what to do?
  m_EventLog = RegisterEventSource(NULL, LogProgram);

  return;

} // LoggingWindows::Open

/****************************************************************
 * Function:  LoggingWindows::Close
 *  Purpose:  Do a formal close of the logging.  Leave global constants
 *         :   alone since a post-destruct call might need them.
 *         :   NOT THREAD SAFE (but threads likely not an issue)
 *Revisions:
 *   Date      By      Description (LoggingWindows::Close)
 * -------- -------- -------------------------------------------
 * 04/07/05 matthewv Created
 ****************************************************************/
void LoggingWindows::Close() {
  if (NULL != m_EventLog)
    DeregisterEventSource(m_EventLog);

  m_EventLog = NULL;

  gLogHandle = 0;

  return;

} // LoggingWindows::Close

/****************************************************************
 * Function:  LoggingWindows::vLog
 *  Purpose:  Send a logging entry to the log, stderr, and/or stdout
 *         :   NOT THREAD SAFE (but threads likely not an issue)
 *Revisions:
 *   Date      By      Description (LoggingWindows::vLog)
 * -------- -------- -------------------------------------------
 * 03/24/05 matthewv Created
 ****************************************************************/
void LoggingWindows::vLog(
    unsigned Priority,  // Priority of the message, and/or trace/verbose mark
    const char *Format, // printf style format string
    va_list Va)         // the "other" args that support the format string
{
  int priority_used;

  priority_used = Priority;

  // verbose stuff goes to stdout
  if (0 != (LOG_VERBOSE | priority_used)) {
    printf("%s: ", gLogProgram);
    vprintf(Format, Va);

    priority_used &= ~LOG_VERBOSE;
  } // if

  // trace stuff goes to stderr & log
  if (0 != (LOG_TRACE | Priority)) {
    fprintf(stderr, "%s: ", gLogProgram);
    vfprintf(stderr, Format, Va);

    priority_used |= LOG_INFO;

  } // if

  if (0 != priority_used && NULL != m_EventLog) {
    BOOL wgood;
    WORD type;
    TCHAR buffer[1024];
    LPCTSTR array[2];

    _vsnprintf(buffer, sizeof(buffer), Format, Va);
    array[0] = buffer;
    array[1] = NULL;

    // translate type
    switch (Priority & LOG_PRIMASK) {
    case LOG_ERR:
      type = EVENTLOG_ERROR_TYPE;
      break;
    case LOG_WARNING:
      type = EVENTLOG_WARNING_TYPE;
      break;
    case LOG_INFO:
      type = EVENTLOG_INFORMATION_TYPE;
      break;
    case LOG_DEBUG:
      type = EVENTLOG_INFORMATION_TYPE;
      break;
    default:
      type = EVENTLOG_INFORMATION_TYPE;
      break;
    } // switch

    wgood = ReportEvent(m_EventLog, type, 0, 0, NULL, 1, 0, array, NULL);
  } // if

  if (0 != priority_used)
    InterlockedIncrement((long *)&m_LineCount);

  return;

} // LoggingWindows::vLog

#else /* end WIN32, begin !WIN32 */

/****************************************************************
 * Function:  LoggingLinux::Open
 *  Purpose:  Do a formal initialization of the logging stream
 *         :   NOT THREAD SAFE (but threads likely not an issue)
 *Revisions:
 *   Date      By      Description (LoggingLinux::Open)
 * -------- -------- -------------------------------------------
 * 03/24/05 matthewv Created
 ****************************************************************/
void LoggingLinux::Open(
    const char *LogName,    // name of the log to receive entries (WINDOWS)
    const char *LogProgram, // name of program generating the entries
    int LogFacility,        // syslog routing of entries (LINUX)
    bool StderrLog)         // also log to stderr
{
  int options;

  // log could be open due to logging during start up, or we could be
  //  dynamically changing the LogName or LogProgram
  if (0 != gLogHandle)
    Close();

  // make a static copy of the names for later use
  if (NULL != LogName) {
    strncpy(gLogName, LogName, LOG_NAME_LENGTH);
    gLogName[LOG_NAME_LENGTH] = '\0';
  } // if
  else {
    *gLogName = '\0';
  } // else

  if (NULL != LogProgram) {
    strncpy(gLogProgram, LogProgram, LOG_NAME_LENGTH);
    gLogProgram[LOG_NAME_LENGTH] = '\0';
  } // if
  else {
    *gLogProgram = '\0';
  } // else;

  gLogFacility = LogFacility;
  gLogStderr = StderrLog;

  options = LOG_PID;
  if (StderrLog)
    options |= LOG_PERROR;

  // open the log and mark flag
  openlog(LogProgram, LOG_PID, LogFacility);

  // mark log as "open"
  gLogHandle = 1;

  return;

} // LoggingLinux::Open

/****************************************************************
 * Function:  LoggingLinux::Close
 *  Purpose:  Do a formal close of the logging.  Leave global constants
 *         :   alone since a post-destruct call might need them.
 *         :   NOT THREAD SAFE (but threads likely not an issue)
 *Revisions:
 *   Date      By      Description (LoggingLinux::Close)
 * -------- -------- -------------------------------------------
 * 03/24/05 matthewv Created
 ****************************************************************/
void LoggingLinux::Close() {
  closelog();

  gLogHandle = 0;

} // LoggingLinux::Close

/****************************************************************
 * Function:  LoggingLinux::vLog
 *  Purpose:  Send a logging entry to the log, stderr, and/or stdout
 *         :   NOT THREAD SAFE (but threads likely not an issue)
 *Revisions:
 *   Date      By      Description (LoggingLinux::vLog)
 * -------- -------- -------------------------------------------
 * 08/25/07 matthewv Removed trace & verbose in favor of StderrLog on open
 * 03/24/05 matthewv Created
 ****************************************************************/
void LoggingLinux::vLog(
    unsigned Priority,  // Priority of the message, and/or trace/verbose mark
    const char *Format, // printf style format string
    va_list Va)         // the "other" args that support the format string
{
  int priority_used;

  priority_used = Priority;
#if 0
    // verbose stuff goes to stdout
    if (0!=(LOG_VERBOSE | priority_used))
    {
        printf("%s: ",gLogProgram);
        vprintf(Format, Va);

        priority_used &= ~LOG_VERBOSE;
    }   // if

    // trace stuff goes to stderr & log
    if (0!=(LOG_TRACE | Priority))
    {
        fprintf(stderr, "%s: ",gLogProgram);
        vfprintf(stderr, Format, Va);

        vsyslog(LOG_INFO, Format, Va);

        priority_used=0;

    }   // if
#endif
  va_list vcopy;
  va_copy(vcopy, Va);

  if (0 != priority_used) {
    vsyslog(priority_used, Format, Va);
  } // if

  if (gLogStderr) {
    printf("%s: ", gLogProgram);
    vprintf(Format, vcopy);
    printf("\n");
  } // if

  return;

} // LoggingLinux::vLog

#endif /* else !WIN32 */
