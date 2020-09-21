/**
 * @file request_response_buff.cpp
 * @author matthewv
 * @date June 9, 2011
 * @date Copyright 2011-2012
 *
 * @brief Implementation of a reusable buffer for RequestResponse
 */

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#include "request_response_buf.h"
#include "logging.h"

/**
 * Initialize the data members.
 * @date Created 06/09/11
 * @author matthewv
 */
RequestResponseBuf::RequestResponseBuf() {
  m_ResponseBuf = new char[eResponseSizeIncrement];
  m_ResponseBufLen = (NULL != m_ResponseBuf ? eResponseSizeIncrement : 0);

  if (NULL == m_ResponseBuf) {
    Logging(LOG_ERR, "%s: out of memory error", __func__);
  } // if

  Reset();

  return;

} // RequestResponseBuf::RequestResponseBuf

/**
 * Release resources
 * @date Created 06/09/11
 * @author matthewv
 */
RequestResponseBuf::~RequestResponseBuf() {
  delete[] m_ResponseBuf;

  return;

} // RequestResponseBuf::~RequestResponseBuf

/**
 * Put object into a fresh state (for initial use or reuse)
 * @date Created 06/09/11
 * @author matthewv
 */
void RequestResponseBuf::Reset() {
  m_RequestVec.iov_base = NULL;
  m_RequestVec.iov_len = 0;
  m_RequestSize = 0;
  m_RequestSent = 0;
  m_RequestString.clear();

  m_ResponseVec.iov_base = NULL;
  m_ResponseVec.iov_len = 0;
  m_ResponseIn = 0;

  // leave buffer alone, not reset

} // RequestResponseBuf::Reset

/**
 * Request is a static string, do not need a copy
 * @date Created 06/09/11
 * @author matthewv
 */
void RequestResponseBuf::StaticRequestString(
    const char *RequestString, //!< string to send as the request
    int RequestLength) //!< length of static string (or -1 to calculate it)
{
  m_RequestVec.iov_base = (void *)RequestString;
  if (-1 != RequestLength)
    m_RequestVec.iov_len = RequestLength;
  else
    m_RequestVec.iov_len = (NULL != RequestString ? strlen(RequestString) : 0);

  m_RequestSize = m_RequestVec.iov_len;
  m_RequestSent = 0;

  return;

} // RequestResponseBuf::StaticRequestString

/**
 * Request string must be copied locally
 * @date Created 06/09/11
 * @author matthewv
 */
void RequestResponseBuf::DynamicRequestString(
    const char *RequestString) //!< string to send as the request
{
  if (NULL != RequestString) {
    m_RequestString = RequestString;
    m_RequestVec.iov_base = (void *)m_RequestString.c_str();
    m_RequestVec.iov_len = m_RequestString.length();
  } // if
  else {
    m_RequestVec.iov_base = NULL;
    m_RequestVec.iov_len = 0;
  } // else

  m_RequestSize = m_RequestVec.iov_len;
  m_RequestSent = 0;

  return;

} // RequestResponseBuf::DynamicRequestString

/**
 * Give original of the response information
 * @date Created 06/09/11
 * @author matthewv
 */
const struct iovec *RequestResponseBuf::ReadIovec() {
  m_ResponseVec.iov_base = m_ResponseBuf;
  m_ResponseVec.iov_len = m_ResponseBufLen;

  AdjustIovec(&m_ResponseVec, 1, m_ResponseIn);

  return (&m_ResponseVec);

} // RequestResponseBuf::ReadIovec

/**
 * Test if string end is within buffer, if not should buffer expand
 * @date Created 06/09/11
 * @author matthewv
 */
size_t RequestResponseBuf::ReadMinimum() {
  size_t ret_size;

  // string marking end of buffer present?
  if (0 != m_ResponseEnding.length() &&
      NULL == memmem(m_ResponseBuf, m_ResponseIn, m_ResponseEnding.c_str(),
                     m_ResponseEnding.length())) {
    // not found, keep reading chunks
    ret_size = m_ResponseIn + 1;

    // is there space for more reading?
    if (m_ResponseBufLen < ret_size) {
      char *new_buf;
      size_t new_len;

      new_len = m_ResponseBufLen + eResponseSizeIncrement;
      new_buf = new char[new_len];

      if (NULL != new_buf) {
        memcpy(new_buf, m_ResponseBuf, m_ResponseBufLen);
        m_ResponseBufLen = new_len;
        delete[] m_ResponseBuf;
        m_ResponseBuf = new_buf;
      } // if
      else {
        Logging(LOG_ERR, "%s: out of memory error", __func__);
      } // else
    }   // if
  }     // if
  else {
    ret_size = m_ResponseIn;
  } // else

  return (ret_size);

} // RequestResponseBuf::ReadMinimum
