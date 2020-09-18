/**
 * @file reader_writer.h
 * @author matthewv
 * @date April 21, 2011
 * @date Copyright 2011-2012
 *
 * @brief Declarations for event/state machine for file descriptor operations
 */

#ifndef READER_WRITER_H
#define READER_WRITER_H

#include <deque>
#include <sys/uio.h>

#include "meventobj.h"
#include "statemachine.h"

typedef std::shared_ptr<class ReaderWriter> ReaderWriterPtr;
typedef std::shared_ptr<class ReaderWriterBuf> ReaderWriterBufPtr;
typedef std::deque<ReaderWriterBufPtr> ReaderWriterBufDeque_t;

/*! \class ReaderWriter

*/

/**
 * Read / Write to a file descriptor
 *
 * Use event manager to handle operations asynchronously

\dot
digraph ReaderWriter {
a [ label="Exists"];
b [ label="Have fd"];
c [ label="Reading"];
d [ label="Writing"];
f [ label="Read and Write"];

a -> b [label="fd assigned"];
b -> c [label="read request"];
c -> a [label="error"];
c -> a [label="timeout"];
c -> c [label="readable"];
c -> b [label="read done"];
c -> f [label="write request"];
b -> d [label="write request"];
d -> a [label="error"];
d -> a [label="timeout"];
d -> d [label="writable"];
d -> b [label="write done"];
d -> f [label="read request"];
h -> a [label="error"];
h -> a [label="timeout"];
h -> h [label="writable"];
h -> h [label="readable"];
h -> c [label="write done"];
h -> d [label="read done"];
b -> a [label="close request"];

}
\enddot
 */

class ReaderWriter : public MEventObj {
  /****************************************************************
   *  Member objects
   ****************************************************************/
public:
  /// list of state nodes
  enum ReaderWriterNode_e {
    RW_NODE_CLOSED = 100,     //!< beginning state
    RW_NODE_OPEN = 101,       //!< have opened file descriptor for read/write
    RW_NODE_READING = 102,    //!< awaiting read data
    RW_NODE_WRITING = 103,    //!< awaiting write availability
    RW_NODE_READ_WRITE = 104, //!< awaiting both read and write activity
  };

  /// list of state edges
  enum ReaderWriterEdge_e {
    RW_EDGE_FIRST = 100,
    RW_EDGE_FD_ASSIGNED = 101, //!< an open file descriptor is available
    RW_EDGE_ERROR = 102,       //!< error on fd (or other logic)
    RW_EDGE_TIMEOUT = 103,     //!< something took too long
    RW_EDGE_RD_REQUEST = 104,  //!< request for read
    RW_EDGE_READABLE = 105,    //!< data available on fd
    RW_EDGE_READ_DONE = 106,   //!< read request satisfied
    RW_EDGE_WR_REQUEST = 107,  //!< request for write
    RW_EDGE_WRITEABLE = 108,   //!< accepting output on fd
    RW_EDGE_WRITE_DONE = 109,  //!< write request satisfied
    RW_EDGE_CLOSE = 110,       //!< close request

    // externally initiated edge messages
    RW_EDGE_DATAREADY = 112,

    // old edge names ... clean up someday
    RW_EDGE_RECEIVED = RW_EDGE_READ_DONE,
    RW_EDGE_CLOSED = RW_EDGE_CLOSE,
    RW_EDGE_SENT = RW_EDGE_WRITE_DONE,
    RW_EDGE_WRITABLE = RW_EDGE_WRITEABLE,

    RW_EDGE_LAST = 113 // mark end of numeric range
  };

protected:
  ReaderWriterBufPtr m_ReadBuf;  //!< null, or active read buffer
  ReaderWriterBufPtr m_WriteBuf; //!< null, or active write buffer

  ReaderWriterBufDeque_t m_PendingWrite; //!< for delayed sends

  bool m_AutoRead; //!< true (default) to automatically post
                   //!<   additional reads
private:
  /****************************************************************
   *  Member functions
   ****************************************************************/
public:
  ReaderWriter();

  virtual ~ReaderWriter();

  virtual void Close();

  /// debug
  void Dump();

  /// get data from handle
  void Read(ReaderWriterBufPtr &Buffer);

  /// write data to handle
  void Write(ReaderWriterBufPtr &Buffer);

  //
  // statemachine callbacks
  //
  bool EdgeNotification(unsigned int EdgeId, StateMachinePtr &Caller,
                        bool PreNotify) override;

  //
  // meventobj callbacks
  //

  /// External callback used when time value expires
  virtual void TimerCallback();

  /// External callback when handle contains error flag
  virtual bool ErrorCallback();

  /// External callback when handle contains read flag
  virtual bool ReadAvailCallback();

  /// External callback when handle contains write flag
  virtual bool WriteAvailCallback();

  /// External callback when handle contains HUP and/or RDHUP flag
  virtual bool CloseCallback(int);

protected:
private:
  ReaderWriter(const ReaderWriter &); //!< disabled:  copy operator
  ReaderWriter &
  operator=(const ReaderWriter &); //!< disabled:  assignment operator

}; // ReaderWriter

class ReaderWriterBuf : public StateMachine {
  /****************************************************************
   *  Member objects
   ****************************************************************/
public:
protected:
  bool m_DataReady; //!< true if write data ready now

private:
  /****************************************************************
   *  Member functions
   ****************************************************************/
public:
  ReaderWriterBuf() : m_DataReady(true){};

  virtual ~ReaderWriterBuf(){};

  // read operations
  virtual const struct iovec *ReadIovec() { return (NULL); };

  virtual int ReadIovecCnt() { return (0); };

  virtual size_t ReadLen() { return (0); };

  virtual void ReadMarkLen(size_t Read){};

  virtual size_t ReadMinimum() { return (0); };

  // write operations
  virtual const struct iovec *WriteIovec() { return (NULL); };

  virtual int WriteIovecCnt() { return (0); };

  virtual size_t WriteLen() { return (0); };

  virtual void WriteMarkLen(size_t Written){};

  virtual size_t WriteEnd() { return (0); };

  virtual bool IsDataReady() { return (m_DataReady); };

  void SetDataReady(bool Ready = true) {
    if (!m_DataReady && Ready) {
      m_DataReady = Ready;
      SendCompletion(ReaderWriter::RW_EDGE_DATAREADY);
    } // if
    else {
      m_DataReady = Ready;
    } // else
    return;
  };

  /// Public routine to receive Edge notification
  bool EdgeNotification(unsigned int EdgeId, StateMachinePtr &Caller,
                        bool PreNotify) override {
    bool ret_flag;

    ret_flag = false;
    if (ReaderWriter::RW_EDGE_DATAREADY == EdgeId) {
      StateMachinePtr state_ptr = GetStateMachinePtr();
      Caller->RemoveCompletion(state_ptr);
      SetDataReady();
      ret_flag = true;
    } // if

    return (ret_flag);
  };

  /// debug
  virtual void Dump(){};

protected:
  /// helper function to adjust given iovec by number bytes already processed
  size_t AdjustIovec(struct iovec *Vec, int VecCnt, size_t Processed);

private:
  ReaderWriterBuf(const ReaderWriterBuf &); //!< disabled:  copy operator
  ReaderWriterBuf &
  operator=(const ReaderWriterBuf &); //!< disabled:  assignment operator
};                                    // class ReaderWriterBuf

#endif // ifndef READER_WRITER_H
