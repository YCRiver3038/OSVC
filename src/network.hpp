#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

/*
  network (socket接続周辺処理をまとめたクラス)
  ヘッダーファイル 
*/

#include <string>
#include "stdio.h"
#include "string.h"
#include "stdint.h"

#include "errno.h"

#if defined(_WIN32) || defined(_WIN64)
#include "ws2tcpip.h"
#include "winsock.h"
#define poll(fdArray, fds, timeout) WSAPoll(fdArray, fds, timeout)
#define close(close_fd) closesocket(close_fd)
#else
#include "fcntl.h"
#include "unistd.h"
#include "netdb.h"
#include "sys/poll.h"
#include "sys/types.h"
#include "arpa/inet.h"
#include "netinet/ip.h"
#endif

constexpr uint32_t EM_TIMEOUT_LENGTH = 100; // default: 100[msec]
constexpr uint32_t EM_RECVBUF_LENGTH_DEFAULT = 32767;
constexpr int32_t EM_CONNECTION_CLOSED  = -250;
constexpr int32_t EM_CONNECTION_TIMEDOUT = -240;
constexpr int32_t EM_ERR = -256;
constexpr int32_t EM_SEND_AVAILABLE = 0;

extern volatile bool network_force_return_req;

class network {
  protected:
    struct addrinfo* dest_info = nullptr;
    struct addrinfo addr_info_hint = {};
    struct sockaddr* con_sock_addr = nullptr;
    int con_sock_addr_len = 0;
    int socket_type;
    int common_setup(const std::string& com_ip, const std::string& com_port);
    struct pollfd accept_pollfd;
    ssize_t recvfrom_ovl(int fd, uint8_t* buffer, size_t count, int flags, struct sockaddr* addr, socklen_t* addr_len);
    ssize_t recvfrom_ovl(int fd, uint16_t* buffer, size_t count, int flags, struct sockaddr* addr, socklen_t* addr_len);
    ssize_t recvfrom_ovl(int fd, uint32_t* buffer, size_t count, int flags, struct sockaddr* addr, socklen_t* addr_len);
    bool nw_connected = false;
    bool blocking_io = false;
    std::string ip_addr;
    std::string port_info;

  protected:
    struct pollfd recv_pollfd;
    struct pollfd send_pollfd;
    int socket_fd;
  
  public:
    network();
    network(const std::string& con_ip_addr, const std::string& con_port);
    network(const std::string& con_ip_addr, const std::string& con_port, const int con_sock_type);
    network(const std::string& con_ip_addr, const std::string& con_port, const int con_family, const int con_sock_type);
    ~network();
    int nw_connect(bool is_blocking=false);
    int nw_bind_and_listen(bool is_blocking=false);
    int nw_accept(struct sockaddr* peer_addr=nullptr, socklen_t* peer_addr_len=nullptr);
    void nw_close();
    //size_t resize_buffer(size_t bsize);
    template <typename DTYPE> ssize_t recv_data(DTYPE* data_dest, ssize_t n_elm, struct sockaddr* src_addr_dest=nullptr, socklen_t* src_addr_len_dest=nullptr, bool nopoll=false);

    template<typename SDTYPE> ssize_t send_data_common(SDTYPE* data_arr, const size_t sb_size, const struct sockaddr* dest_addr=nullptr, const socklen_t dest_addr_len=0);
    ssize_t send_data( uint8_t* data_arr, const size_t sb_size, const struct sockaddr* dest_addr=nullptr, const socklen_t dest_addr_len=0);
    ssize_t send_data(uint16_t* data_arr, const size_t sb_size, const struct sockaddr* dest_addr=nullptr, const socklen_t dest_addr_len=0);
    ssize_t send_data(uint32_t* data_arr, const size_t sb_size, const struct sockaddr* dest_addr=nullptr, const socklen_t dest_addr_len=0);
    int set_address(const std::string& dest_ip, const std::string& dest_port, const int con_family, const int con_sock_type);
    int set_address(const std::string& dest_ip, const std::string& dest_port);
    void errno_to_string(int err_num, std::string& dest);
    int get_connection_addr(char* addrDest, int addrDestLen, char* portDest, int portDestLen);
};

template<typename SDTYPE> ssize_t network::send_data_common(SDTYPE* data_arr, const size_t sb_size, const struct sockaddr* dest_addr, const socklen_t dest_addr_len){
  if (data_arr == nullptr) {
    return (ssize_t)EM_ERR;
  }

  int send_errno = 0;
  ssize_t send_head_index= 0;
  ssize_t send_remain = 0;
  ssize_t sent_length = 0;
  int timeout_count = 0;
  int poll_res = 0;
  ssize_t poll_errno = 0;
#if defined(_WIN32) || defined(_WIN64)
  const char* ptr_cast = (const char*)data_arr;
#else
  uint8_t* ptr_cast = (uint8_t*)data_arr;
#endif

  send_head_index = 0;
  send_remain = sb_size*sizeof(SDTYPE);
  while ((send_remain > 0) && !network_force_return_req) {
    poll_res = poll(&send_pollfd, 1, EM_TIMEOUT_LENGTH);
    poll_errno = errno;
    if (poll_res > 0) {
      if (send_head_index >= sb_size) {
        break;
      }
      if (send_pollfd.revents & (0|POLLWRNORM)) {
        sent_length = sendto(socket_fd, &(ptr_cast[send_head_index]), send_remain, 0, dest_addr, dest_addr_len);
        send_errno = errno;
        if (sent_length >= 0) {
          send_head_index += sent_length;
          send_remain -= sent_length;
        } else {
          switch(send_errno){
            case EAGAIN:
              break;
            default:
              return (send_errno * -1);
          }
        }
      } else {
        if (send_pollfd.revents & (0|POLLHUP)) {
          return (ssize_t)EM_CONNECTION_CLOSED;
        }
        if (send_pollfd.revents & (0|POLLERR|POLLNVAL)) {
          return (ssize_t)EM_ERR;
        }
      }
    } else {
      if (poll_res == -1) {
        return (ssize_t)(poll_errno * -1);
      }
      if (poll_res == 0) {
        return (ssize_t)EM_CONNECTION_TIMEDOUT;
      }
    }
  }
  return send_head_index;
}
/*
  recv_data()
    receive data from source host
  arguments:
                n_elm -> number of elements to be received
            data_dest -> destination of the received data
        src_addr_dest -> the address of source host will be stored here
    src_addr_len_dest -> size of src_addr_dest
               nopoll -> with/without polling (default: false)

  Arguments "src_addr_dest" and "src_addr_len_dest" can be omitted.
  Argument "data_dest" must be allocated with enough size 
  to hold at least "n_elm" elements of the data before calling this method,
  or it may lead to fatal error such as segmentation fault.

  return:
                        receive success -> actual size of received elements
  socket is neither bound nor connected -> 0
                       no data received -> 0
                         receive failed -> negated errno
                     connection timeout -> -240(EM_CONNECTION_TIMEDOUT)
                      connection closed -> -250(EM_CONNECTION_CLOSED)
           destination is not allocated -> -256(EM_ERR)
                           receive fail -> -256(EM_ERR)
*/
template <typename DTYPE> ssize_t network::recv_data(DTYPE* data_dest, ssize_t n_elm, struct sockaddr* src_addr_dest, socklen_t* src_addr_len_dest, bool nopoll){
  ssize_t recv_length = 0;
  ssize_t poll_errno = 0;
  int poll_res = 0;
  if (nopoll) {
    int rcv_errno = 0;
    recv_length = recvfrom_ovl(socket_fd, data_dest, n_elm, 0, src_addr_dest, src_addr_len_dest);
    rcv_errno = errno;
    if (recv_length >= 0) {
      return recv_length;
    } else {
      return -1 * rcv_errno;
    }
  } else {
#ifdef _GNU_SOURCE
    const short rd_hup_mask = 0|POLLRDHUP|POLLHUP;
#else
    const short rd_hup_mask = 0|POLLHUP;
#endif
    if (!nw_connected && (socket_type == SOCK_STREAM)) {
      return 0;
    }
    if (data_dest == nullptr) {
      return EM_ERR;
    }

    poll_res = poll(&recv_pollfd, 1, EM_TIMEOUT_LENGTH);
    poll_errno = (ssize_t)errno;
    if (poll_res == -1) {
      return (ssize_t)(poll_errno * -1);
    }
    if (poll_res == 0) {
      return (ssize_t)EM_CONNECTION_TIMEDOUT;
    }
    if (recv_pollfd.revents & rd_hup_mask) {
      return (ssize_t)EM_CONNECTION_CLOSED;
    }
    if (recv_pollfd.revents & (0|POLLERR|POLLNVAL)) {
      return (ssize_t)EM_ERR;
    }

    if (recv_pollfd.revents & 0|POLLRDNORM) {
      recv_length = recvfrom_ovl(socket_fd, data_dest, n_elm, 0, src_addr_dest, src_addr_len_dest);
      return recv_length;
    }
  }
  return EM_ERR;
}

class fd_network : public network {
  public:
    fd_network(int init_fd);
};
#endif
