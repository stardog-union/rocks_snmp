/****************************************************************
*   Module:  logging.h
*  Purpose:  Common logging interface for Windows and Linux
*         :
*   Author:  matthewv
*  Created:  03/23/05
*Revisions:
   $Header: //depot/code/main/porting/logging.h#8 $
****************************************************************/

/****************************************************************
 * Copyright 2005
 * Matthew Von-Maszewski
 * All Rights Reserved
 ****************************************************************/

/****************************************************************
 *  Precompiled Header stuff ends after this
 ****************************************************************/

/****************************************************************
 *  File / Environment #defines
 ****************************************************************/

#ifndef LOGGING_H
#define LOGGING_H

/****************************************************************
 *  Includes
 ****************************************************************/

#include <stdarg.h>
#include <sys/syslog.h>

/****************************************************************
 *  Debug
 ****************************************************************/

/****************************************************************
 *  #defines, typedefs, enum
 ****************************************************************/

// these are matthewv specific values, which just do console routing
#define LOG_TRACE 0x08
#define LOG_VERBOSE 0x10

/****************************************************************
 *  Static data
 ****************************************************************/

class LoggingLinux;
extern LoggingLinux gLogging;

#define LOG_NAME_LENGTH 32
extern char gLogName[LOG_NAME_LENGTH + 1];
extern char gLogProgram[LOG_NAME_LENGTH + 1];
extern unsigned gLogHandle;
extern unsigned gLogLevel;
extern int gLogFacility;
extern bool gLogStderr;

/****************************************************************
 *  Static functions
 ****************************************************************/

/****************************************************************
 *  Classes
 ****************************************************************/

/****************************************************************
 *  LoggingClass
 *    Abstract class for Logging calls (Windows versus Linux)
 ****************************************************************/

class LoggingClass {
  /****************************************************************
   *  Member objects
   ****************************************************************/

public:
  volatile unsigned m_LineCount; // how many lines have gone to the log

protected:
private:
  /****************************************************************
   *  Member functions
   ****************************************************************/
private:
  LoggingClass(const LoggingClass &); /* no copy constructor */

public:
  LoggingClass() : m_LineCount(0){};
  virtual ~LoggingClass(){};

  virtual void Open(const char *LogName, const char *LogProgram, int Facility,
                    bool StderrLog = false) = 0;
  virtual void Close() = 0;

  virtual void Log(unsigned Priority, const char *Format, ...) = 0;
  virtual void vLog(unsigned Priority, const char *Format, va_list Va) = 0;

  virtual void LogBinary(unsigned Priority, const char *Prefix,
                         const void *Data, unsigned DataSize);

  unsigned SetLogLevel(unsigned NewLevel) {
    unsigned OldLevel = gLogLevel;
    gLogLevel = NewLevel;
    return (OldLevel);
  };
  unsigned GetLogLevel(void) { return (gLogLevel); };

  unsigned GetLineCount() const { return (m_LineCount); };

protected:
};

/****************************************************************
 *  LoggingLinux
 *    Linux version of the object
 ****************************************************************/

class LoggingLinux : public LoggingClass {
  /****************************************************************
   *  Member objects
   ****************************************************************/

public:
protected:
private:
  /****************************************************************
   *  Member functions
   ****************************************************************/
private:
  LoggingLinux(const LoggingLinux &); /* no copy constructor */

public:
  LoggingLinux(){};
  virtual ~LoggingLinux() { Close(); };

  virtual void Open(const char *LogName, const char *LogProgram, int Facility,
                    bool StderrLog = false);
  virtual void Close();

  virtual void Log(unsigned /*Priority*/, const char * /*Format*/, ...) {
    return;
  };
  virtual void vLog(unsigned Priority, const char *Format, va_list Va);

protected:
}; /* LoggingLinux */;

// this inline function makes a quick decision as to whether or not
//  the logging object should be called.  This is an attempt to
//  keep the performance up even if bunches of log calls are in
//  the code.

static inline void Logging(unsigned Priority, const char *Format, ...) {
  if (Priority <= gLogLevel) {
    va_list va;
    va_start(va, Format);
    gLogging.vLog(Priority, Format, va);
  } // if
} // Logging

#endif // LOGGING_H
