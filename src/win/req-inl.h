/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef UV_WIN_REQ_INL_H_
#define UV_WIN_REQ_INL_H_

#include <assert.h>

#include "uv.h"
#include "debug.h"
#include "internal.h"


#define SET_REQ_STATUS(req, status)                                     \
   (req)->overlapped.Internal = (ULONG_PTR) (status)

#define SET_REQ_ERROR(req, error)                                       \
  SET_REQ_STATUS((req), NTSTATUS_FROM_WIN32((error)))

#define SET_REQ_SUCCESS(req)                                            \
  SET_REQ_STATUS((req), STATUS_SUCCESS)

#define GET_REQ_STATUS(req)                                             \
  ((NTSTATUS) (req)->overlapped.Internal)

#define REQ_SUCCESS(req)                                                \
  (NT_SUCCESS(GET_REQ_STATUS((req))))

#define GET_REQ_ERROR(req)                                              \
  (pRtlNtStatusToDosError(GET_REQ_STATUS((req))))

#define GET_REQ_SOCK_ERROR(req)                                         \
  (uv_ntstatus_to_winsock_error(GET_REQ_STATUS((req))))


#define REGISTER_HANDLE_REQ(loop, handle, req)                          \
  do {                                                                  \
    INCREASE_ACTIVE_COUNT((loop), (handle));                            \
    uv__req_register((loop), (req));                                    \
  } while (0)

#define UNREGISTER_HANDLE_REQ(loop, handle, req)                        \
  do {                                                                  \
    DECREASE_ACTIVE_COUNT((loop), (handle));                            \
    uv__req_unregister((loop), (req));                                  \
  } while (0)


#define UV_SUCCEEDED_WITHOUT_IOCP(result)                               \
  ((result) && (handle->flags & UV_HANDLE_SYNC_BYPASS_IOCP))

#define UV_SUCCEEDED_WITH_IOCP(result)                                  \
  ((result) || (GetLastError() == ERROR_IO_PENDING))


#define POST_COMPLETION_FOR_REQ(loop, req)                              \
  if (!PostQueuedCompletionStatus((loop)->iocp,                         \
                                  0,                                    \
                                  0,                                    \
                                  &((req)->overlapped))) {              \
    uv_fatal_error(GetLastError(), "PostQueuedCompletionStatus");       \
  }


INLINE static void uv_req_init(uv_loop_t* loop, uv_req_t* req) {
  req->type = UV_UNKNOWN_REQ;
  req->debug_name = "";
  SET_REQ_SUCCESS(req);
}


INLINE static uv_req_t* uv_overlapped_to_req(OVERLAPPED* overlapped) {
  return CONTAINING_RECORD(overlapped, uv_req_t, overlapped);
}


INLINE static void uv_insert_pending_req(uv_loop_t* loop, uv_req_t* req) {
  req->next_req = NULL;
  if (loop->pending_reqs_tail) {
    req->next_req = loop->pending_reqs_tail->next_req;
    loop->pending_reqs_tail->next_req = req;
    loop->pending_reqs_tail = req;
  } else {
    req->next_req = req;
    loop->pending_reqs_tail = req;
  }
}


#define DELEGATE_STREAM_REQ(loop, req, method, handle_at)                     \
  do {                                                                        \
    uv_handle_t* h = ((uv_handle_t*) (req)->handle_at);                       \
    debug_print("DELEGATE_STREAM_REQ: %d %s", h->type, h->debug_name);        \
    switch (((uv_handle_t*) (req)->handle_at)->type) {                        \
      case UV_TCP:                                                            \
        uv_process_tcp_##method##_req(loop,                                   \
                                      (uv_tcp_t*) ((req)->handle_at),         \
                                      req);                                   \
        break;                                                                \
                                                                              \
      case UV_NAMED_PIPE:                                                     \
        uv_process_pipe_##method##_req(loop,                                  \
                                       (uv_pipe_t*) ((req)->handle_at),       \
                                       req);                                  \
        break;                                                                \
                                                                              \
      case UV_TTY:                                                            \
        uv_process_tty_##method##_req(loop,                                   \
                                      (uv_tty_t*) ((req)->handle_at),         \
                                      req);                                   \
        break;                                                                \
                                                                              \
      default:                                                                \
        assert(0);                                                            \
    }                                                                         \
  } while (0)


INLINE static int uv_process_reqs(uv_loop_t* loop) {
  uv_req_t* req;
  uv_req_t* first;
  uv_req_t* next;

  if (loop->pending_reqs_tail == NULL)
    return 0;

  first = loop->pending_reqs_tail->next_req;
  next = first;
  loop->pending_reqs_tail = NULL;

  while (next != NULL) {
    req = next;
    next = req->next_req != first ? req->next_req : NULL;    
    switch (req->type) {
      case UV_READ:
        debug_print("uv_process_reqs: %d %s %s", req->type, "UV_READ", req->debug_name);
        DELEGATE_STREAM_REQ(loop, req, read, data);
        break;

      case UV_WRITE:
        debug_print("uv_process_reqs: %d %s %s", req->type, "UV_WRITE", req->debug_name);
        DELEGATE_STREAM_REQ(loop, (uv_write_t*) req, write, handle);
        break;

      case UV_ACCEPT:
        debug_print("uv_process_reqs: %d %s %s", req->type, "UV_ACCEPT", req->debug_name);
        DELEGATE_STREAM_REQ(loop, req, accept, data);
        break;

      case UV_CONNECT:
        debug_print("uv_process_reqs: %d %s %s", req->type, "UV_CONNECT", req->debug_name);
        DELEGATE_STREAM_REQ(loop, (uv_connect_t*) req, connect, handle);
        break;

      case UV_SHUTDOWN:
        debug_print("uv_process_reqs: %d %s %s", req->type, "UV_SHUTDOWN", req->debug_name);
        /* Tcp shutdown requests don't come here. */
        assert(((uv_shutdown_t*) req)->handle->type == UV_NAMED_PIPE);
        uv_process_pipe_shutdown_req(
            loop,
            (uv_pipe_t*) ((uv_shutdown_t*) req)->handle,
            (uv_shutdown_t*) req);
        break;

      case UV_UDP_RECV:
        debug_print("uv_process_reqs: %d %s %s", req->type, "UV_UDP_RECV", req->debug_name);
        uv_process_udp_recv_req(loop, (uv_udp_t*) req->data, req);
        break;

      case UV_UDP_SEND:
        debug_print("uv_process_reqs: %d %s %s", req->type, "UV_UDP_SEND", req->debug_name);
        uv_process_udp_send_req(loop,
                                ((uv_udp_send_t*) req)->handle,
                                (uv_udp_send_t*) req);
        break;

      case UV_WAKEUP:
        debug_print("uv_process_reqs: %d %s %s", req->type, "UV_WAKEUP", req->debug_name);
        uv_process_async_wakeup_req(loop, (uv_async_t*) req->data, req);
        break;

      case UV_SIGNAL_REQ:
        debug_print("uv_process_reqs: %d %s %s", req->type, "UV_SIGNAL_REQ", req->debug_name);
        uv_process_signal_req(loop, (uv_signal_t*) req->data, req);
        break;

      case UV_POLL_REQ:
        debug_print("uv_process_reqs: %d %s %s", req->type, "UV_POLL_REQ", req->debug_name);
        uv_process_poll_req(loop, (uv_poll_t*) req->data, req);
        break;

      case UV_PROCESS_EXIT:
        debug_print("uv_process_reqs: %d %s %s", req->type, "UV_PROCESS_EXIT", req->debug_name);
        uv_process_proc_exit(loop, (uv_process_t*) req->data);
        break;

      case UV_FS_EVENT_REQ:
        debug_print("uv_process_reqs: %d %s %s", req->type, "UV_FS_EVENT_REQ", req->debug_name);
        uv_process_fs_event_req(loop, req, (uv_fs_event_t*) req->data);
        break;

      default:
        assert(0);
    }
  }

  return 1;
}

#endif /* UV_WIN_REQ_INL_H_ */
