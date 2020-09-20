/**
 * @file reader_writer.cpp
 * @author matthewv
 * @date May 9, 2011
 * @date Copyright 2011
 *
 * @brief Implementation of event/state machine for file descriptor operations
 */

#include <errno.h>

#include "reader_writer.h"

#include "meventmgr.h"
#include "util/logging.h"

/**
 * Initialize the data members.
 * @date 05/09/11  matthewv  Created
 */
ReaderWriter::ReaderWriter()
    : m_ReadBuf(NULL), m_WriteBuf(NULL), m_AutoRead(true) {
  SetState(RW_EDGE_CLOSED);
} // ReaderWriter::ReaderWriter

/**
 * Release resources
 * @date 05/06/11  matthewv  Created
 */
ReaderWriter::~ReaderWriter() { Close(); } // ReaderWriter::~ReaderWriter

/**
 * A read or write request took too long
 * @date Created 05/13/11
 * @author matthewv
 */
void ReaderWriter::TimerCallback() {
  SendEdge(RW_EDGE_TIMEOUT);

  /** do not know if the derived class wants to continue
      or close.  do nothing **/

} // ReaderWriter::TimerCallback

/**
 * event logic saw an error on the file descriptor
 * @date Created 05/13/11
 * @author matthewv
 * @returns true if simultaneous Read/Write callbacks should proceed
 */
bool ReaderWriter::ErrorCallback() {
  SendEdge(RW_EDGE_ERROR);

  // stop reads / writes that may be pending ... hence "false"

  return (false);

} // ReaderWriter::ErrorCallback

bool ReaderWriter::CloseCallback(int) {
  Close();

  return (true);
} // ReaderWriter::CloseCallback

/**
 * A file descriptor has something to read
 * @date Created 05/13/11
 * @author matthewv
 * @returns true if simultaneous Write callbacks should proceed
 */
bool ReaderWriter::ReadAvailCallback() {
  bool ret_flag;

  ret_flag = true;

  // if a read buffer is sitting here, read into it
  //  only send an edge notification if the amount read
  //  crosses the minimum requested
  if (NULL != m_ReadBuf.get()) {
    bool again, zero_size;
    int ret_val;

    do {
      again = false;
      zero_size = false;

      ret_val =
          readv(m_Handle, m_ReadBuf->ReadIovec(), m_ReadBuf->ReadIovecCnt());

      if (0 <= ret_val) {
        m_ReadBuf->ReadMarkLen(ret_val);
        // loop if have not reached minimum
        again =
            ((m_ReadBuf->ReadLen() < m_ReadBuf->ReadMinimum()) && 0 != ret_val);
        zero_size = (0 == ret_val);
      } // if

      // error:  interrupt or no data?
      else {
        // no data ... yet
        if (EAGAIN == errno)
          again = false;

        // interrupt, repeat
        else if (EINTR == errno)
          again = true;

        // ouch, something bad
        else {
          again = false;
          ret_flag = false;
          m_ReadBuf = NULL;
          Logging(LOG_ERR, "%s: error on readv [errno %d, handle %d]", __func__,
                  errno, m_Handle);
          SendEdge(RW_EDGE_ERROR);
        } // else
      }   // else
    } while (again);

    // update the epoll read monitoring
    if (m_AutoRead && !zero_size) {
      bool read_flag;
      read_flag = RequestRead(ret_flag &&
                              m_ReadBuf->ReadLen() < m_ReadBuf->ReadMinimum());
      if (!read_flag) {
        Logging(LOG_ERR,
                "%s: RequestRead failed (ret_flag=%d, ReadLen=%zd, "
                "ReadMinimum=%zd)",
                __func__, ret_flag, m_ReadBuf->ReadLen(),
                m_ReadBuf->ReadMinimum());
      } // if
    }   // if

    // Send a notification?
    if (ret_flag &&
        (m_ReadBuf->ReadMinimum() <= m_ReadBuf->ReadLen() || (zero_size))) {
      m_ReadBuf = NULL;
      SendEdge(RW_EDGE_RECEIVED);
    } // if
  }   // if

  // no buffer, just send message
  else {
    bool read_flag;

    read_flag = RequestRead(false);
    if (!read_flag) {
      Logging(LOG_ERR, "%s: RequestRead2 failed", __func__);
    } // if

    SendEdge(RW_EDGE_READABLE);
  } // else

  return (ret_flag);

} // ReaderWriter::ReadAvailCallback

/**
 * A file descriptor is now writable
 * @date Created 05/13/11
 * @author matthewv
 * @returns true if simultaneous post-Write callbacks should proceed (none
 * today)
 */
bool ReaderWriter::WriteAvailCallback() {
  bool ret_flag;

  ret_flag = true;

  // if a write buffer is sitting here, read from it
  //  send an edge notification once all data written
  //  (example:  we get a call after socket completes open ... may not
  //             have buffer assigned then ... hence not an error)
  if (NULL != m_WriteBuf.get()) {
    bool again;
    int ret_val;

    do {
      again = false;
      ret_val = writev(m_Handle, m_WriteBuf->WriteIovec(),
                       m_WriteBuf->WriteIovecCnt());

      if (0 <= ret_val) {
        m_WriteBuf->WriteMarkLen(ret_val);

        // this one finished and another waiting
        if (m_WriteBuf->WriteLen() == m_WriteBuf->WriteEnd() &&
            0 != m_PendingWrite.size() &&
            m_PendingWrite.front()->IsDataReady()) {
          m_WriteBuf = m_PendingWrite.front();
          m_PendingWrite.pop_front();
        } // if

        // loop if have not reached minimum
        again =
            ((m_WriteBuf->WriteLen() < m_WriteBuf->WriteEnd()) && 0 != ret_val);

      } // if

      // error:  interrupt or no data?
      else {
        // no data ... yet
        if (EAGAIN == errno)
          again = false;

        // interrupt, repeat
        else if (EINTR == errno)
          again = true;

        // ouch, something bad
        else {
          again = false;
          ret_flag = false;
          m_WriteBuf = NULL;
          Logging(LOG_ERR, "%s: error on writev [errno %d, handle %d]",
                  __func__, errno, m_Handle);
          SendEdge(RW_EDGE_ERROR);
        } // else
      }   // else
    } while (again);

    // update the epoll write monitoring
    RequestWrite(ret_flag && m_WriteBuf->WriteLen() < m_WriteBuf->WriteEnd());

    // Send a notification?
    if (ret_flag && m_WriteBuf->WriteEnd() == m_WriteBuf->WriteLen()) {
      m_WriteBuf = NULL;
      SendEdge(RW_EDGE_SENT);
    } // if
  }   // if

  // no buffer, just send message
  else {
    RequestWrite(false);
    SendEdge(RW_EDGE_WRITEABLE);
  } // else

  return (ret_flag);

} // ReaderWriter::WriteAvailCallback

/**
 * Close file descriptor
 * @date 05/09/11  matthewv  Created
 */
void ReaderWriter::Close() {
  if (-1 != m_Handle) {
    // in theory, the "close" will clear the epoll entry automatically
    //  doing it manually just in case
#if 0
        int ret_val;

        if (NULL!=m_MgrPtr.get())
            m_MgrPtr->UpdateEpoll(this, false, false);

         ret_val=close(m_Handle);

        // this logging might get painful.  not committed to keeping it
        if (0!=ret_val)
        {
            Logging(LOG_ERR, "%s: Error with close() [errno=%d, fd=%d]",
                    __func__, errno, m_Handle);
        }   // if

        //
        m_Handle=-1;
#else
    m_PendingWrite.clear();
    m_WriteBuf.reset();
    m_ReadBuf.reset();

    // let MEventObj do some cleanup too
    Reset();
#endif

    // inform the world
    SetState(RW_NODE_CLOSED);
    // SendEdge(RW_EDGE_CLOSED);  ... // bad idea, shared_from_this is already dead
  } // if

  return;

} // ReaderWriter::Close

/**
 * Fill a buffer with data and send notification
 * @date Create 05/24/11
 * @author matthewv
 */
void ReaderWriter::Read(ReaderWriterBufPtr &Buffer) //!< buffer to receive data
{
  m_ReadBuf = Buffer;
  ReadAvailCallback();

  return;

} // ReaderWriter::Read

/**
 * Write data from buffer until complete
 * @date Create 05/08/11
 * @author matthewv
 */
void ReaderWriter::Write(ReaderWriterBufPtr &Buffer) //!< buffer to transmit
{
  // immediate write?
  if (NULL == m_WriteBuf.get() && 0 == m_PendingWrite.size() &&
      Buffer->IsDataReady()) {
    m_WriteBuf = Buffer;
    WriteAvailCallback();
  } // if

  // delayed write
  else {
    StateMachinePtr shared = GetStateMachinePtr();
    m_PendingWrite.push_back(Buffer);
    Buffer->AddCompletion(shared);
  } // else

  return;

} // ReaderWriter::Write

/**
 * @brief Turn edge notifications into State changes
 *
 * @date Created 05/13/11
 * @author matthewv
 * @returns  true if edge handled to state transition
 */

bool ReaderWriter::EdgeNotification(
    unsigned int EdgeId, //!< what just happened, what graph edge are we walking
    StateMachinePtr &Caller, //!< what state machine object initiated the edge
    bool PreNotify) //!< for watchers, is the before or after owner processes
{
  bool used;

  used = false;

  // only care about our own events
  if (this == Caller.get()) {
    switch (EdgeId) {
    case RW_EDGE_TIMEOUT:
      Close();
      used = true;
      break;

    case RW_EDGE_ERROR:
      Close();
      used = true;
      break;

    case RW_EDGE_CLOSED:
      used = true;
      break;

    case RW_EDGE_READ_DONE:
      SendCompletion(RW_EDGE_READ_DONE);
      used = true;
      break;

    case RW_EDGE_WRITE_DONE:
      SendCompletion(RW_EDGE_WRITE_DONE);
      used = true;
      break;

    default:
      // send down a level.  If not used then it is an error
      used = MEventObj::EdgeNotification(EdgeId, Caller, PreNotify);
      if (!used && RW_EDGE_FIRST < EdgeId && RW_EDGE_LAST > EdgeId) {
        Logging(LOG_ERR, "%s: unknown edge value passed [EdgeId=%u]",
                __PRETTY_FUNCTION__, EdgeId);
        SendEdge(RW_EDGE_ERROR);
      } // if
      break;
    } // switch
  }   // if

  // outside notification
  else {
    switch (EdgeId) {
    case RW_EDGE_DATAREADY:
      if (NULL == m_WriteBuf.get() && 0 != m_PendingWrite.size() &&
          m_PendingWrite.front()->IsDataReady()) {
        StateMachinePtr this_obj = GetStateMachinePtr();

        Caller->RemoveCompletion(this_obj);
        m_WriteBuf = m_PendingWrite.front();
        m_PendingWrite.pop_front();
        WriteAvailCallback();
      } // if
      break;
    } // switch
  }   // else

  return (used);

} // ReaderWriter::EdgeNotifications

/**
 * Update iovec pointers and lengths to account for data already received
 *
 * @date Created 05/24/11
 * @author matthewv
 * @return Remaining total size of buffers
 */
size_t ReaderWriterBuf::AdjustIovec(
    struct iovec *Vec, //!< non-null pointer to array of struct iovec
    int VecCnt,        //!< number of entries in Vec array
    size_t Processed)  //!< count of bytes already valid in array
{
  size_t limit, used;
  int loop;
  char *ptr;
  struct iovec *vec_ptr;

  limit = 0;
  used = Processed;

  for (loop = 0, vec_ptr = Vec; loop < VecCnt; ++loop, ++vec_ptr) {
    limit += vec_ptr->iov_len;

    if (0 != used) {
      // partial, move pointer and reduce len
      if (used <= vec_ptr->iov_len) {
        ptr = (char *)vec_ptr->iov_base;
        ptr += used;
        vec_ptr->iov_base = ptr;
        vec_ptr->iov_len -= used;
        used = 0;
      } // if

      // this buffer used up, disable
      else {
        used -= vec_ptr->iov_len;
        vec_ptr->iov_len = 0;
      } // else
    }   // if
  }     // for

  return (limit - Processed);

} // ReaderWriterBuf::AdjustIovec
