#include "network.hpp"

/* force return request variable */
volatile bool network_force_return_req = false;

/* private method */
ssize_t network::recvfrom_ovl(int fd,
                              uint8_t* buffer, size_t count, int flags,
                              struct sockaddr* addr, socklen_t* addr_len) {
#if defined(_WIN32) || defined(_WIN64)
    return recvfrom(fd, (char*)buffer, count, flags, addr, addr_len);
#else
    return recvfrom(fd, buffer, count, flags, addr, addr_len);
#endif
}
ssize_t network::recvfrom_ovl(int fd,
                              uint16_t* buffer, size_t count, int flags,
                              struct sockaddr* addr, socklen_t* addr_len) {
  size_t temp_counter = 0;
  ssize_t rf_retnum = 0;
  int rf_errno = 0;

#if defined(_WIN32) || defined(_WIN64)
  rf_retnum = recvfrom(fd, (char*)buffer, count*sizeof(uint16_t),
                       flags, addr, addr_len);
#else
  rf_retnum = recvfrom(fd, buffer, count*sizeof(uint16_t),
                       flags, addr, addr_len);
#endif

  rf_errno = errno;
  if (rf_retnum < 0) {
    return -1*rf_errno;
  }
  if (rf_retnum == 0) {
    return 0;
  }
  while (temp_counter < count && (!network_force_return_req)) {
    buffer[temp_counter] = ntohs(buffer[temp_counter]);
    temp_counter++;
  }
  return rf_retnum;
}
ssize_t network::recvfrom_ovl(int fd,
                              uint32_t* buffer, size_t count, int flags,
                              struct sockaddr* addr, socklen_t* addr_len) {
  size_t temp_counter = 0;
  ssize_t rf_retnum = 0;
  int rf_errno = 0;

#if defined(_WIN32) || defined(_WIN64)
  rf_retnum = recvfrom(fd, (char*)buffer, count*sizeof(uint32_t),
                       flags, addr, addr_len);
#else
  rf_retnum = recvfrom(fd, buffer, count*sizeof(uint32_t),
                       flags, addr, addr_len);
#endif

  rf_errno = errno;
  if (rf_retnum < 0) {
    return -1*rf_errno;
  }
  if (rf_retnum == 0) {
    return 0;
  }
  while (temp_counter < count && (!network_force_return_req)) {
    buffer[temp_counter] = ntohl(buffer[temp_counter]);
    temp_counter++;
  }
  return rf_retnum;
}

void network::errno_to_string(int err_num, std::string& dest) {
  dest.clear();
  switch (err_num) {
    case EM_ERR:
      dest.assign("Error");
      break;
    case EM_CONNECTION_TIMEDOUT:
      dest.assign("Connection timeout.");
      break;
    case EM_CONNECTION_CLOSED:
      dest.assign("Connection closed.");
      break;
    case -1:
      dest.assign("Error");
      break;
    case 0:
      dest.assign("No error");
      break;
    default:
      if (err_num < 0) { 
        dest.assign(strerror(-1 * err_num));
      } else {
        dest.assign(strerror(err_num));
      }
      break;
  }
}
int network::get_connection_addr(char* addrDest, int addrDestLen, char* portDest, int portDestLen) {
  if (!con_sock_addr) {
    return -1;
  }
  return getnameinfo(con_sock_addr, (socklen_t)con_sock_addr_len,
                     addrDest, (socklen_t)addrDestLen, portDest, (socklen_t)portDestLen,
                     0|NI_NUMERICHOST|NI_NUMERICSERV);
}

int network::common_setup(const std::string& com_ip,
                          const std::string& com_port) {
  if (nw_connected) {
    close(socket_fd);
    nw_connected = false;
  }
  if (con_sock_addr) {
    con_sock_addr = nullptr;
  }
  if (dest_info) {
    freeaddrinfo(dest_info);
  }
#ifdef _GNU_SOURCE
  recv_pollfd.events = POLLRDNORM | POLLRDHUP | POLLHUP | POLLERR | POLLNVAL;
  accept_pollfd.events = POLLRDNORM | POLLRDHUP | POLLHUP
                       | POLLERR | POLLNVAL;
#else
  recv_pollfd.events = POLLRDNORM | POLLHUP | POLLERR | POLLNVAL;
  accept_pollfd.events = POLLRDNORM | POLLHUP | POLLERR | POLLNVAL;
#endif
  send_pollfd.events = POLLWRNORM | POLLHUP | POLLERR | POLLNVAL;
  return getaddrinfo(com_ip.c_str(), com_port.c_str(), &addr_info_hint, &dest_info);
}

/* public method */
network::network(const std::string& con_ip_addr, const std::string& con_port,
                 const int con_family, const int con_sock_type) {
#if defined(_WIN32) || defined(_WIN64)
  wVersionRequested = MAKEWORD(2, 2);
  wInitStatus = WSAStartup(wVersionRequested, &wsaData);
  if (wInitStatus != 0) {
    printf("WSASetup error: %d\n", wInitStatus);
    return;
  }
#endif

  int comset_status = 0;

  ip_addr.clear();
  ip_addr.assign(con_ip_addr);
  port_info.clear();
  port_info.assign(con_port);
  addr_info_hint.ai_family = con_family;
  addr_info_hint.ai_socktype = con_sock_type;
  addr_info_hint.ai_flags = 0;
  if (con_sock_type == SOCK_DGRAM) {
    addr_info_hint.ai_protocol = 0 | IPPROTO_UDP;
  } else if (con_sock_type == SOCK_STREAM) {
    addr_info_hint.ai_protocol = 0 | IPPROTO_TCP;
  } else {
    addr_info_hint.ai_protocol = 0;
  }
  socket_type = con_sock_type;
  comset_status = common_setup(ip_addr, port_info);
  if (comset_status != 0) {
    printf("FATAL: network setup failed ( %d: %s )\n", comset_status, gai_strerror(comset_status));
  }
}

#ifdef __APPLE__
network::network(): network("127.0.0.1", "62341", PF_INET, SOCK_STREAM){}
network::network(const std::string& con_ip_addr, const std::string& con_port):
                 network(con_ip_addr, con_port, PF_UNSPEC, SOCK_STREAM){}
network::network(const std::string& con_ip_addr, const std::string& con_port, const int con_sock_type):
                 network(con_ip_addr, con_port, PF_UNSPEC, con_sock_type){}
#else
network::network(): network("127.0.0.1", "62341", AF_INET, SOCK_STREAM){}
network::network(const std::string& con_ip_addr, const std::string& con_port):
                 network(con_ip_addr, con_port, AF_UNSPEC, SOCK_STREAM){}
network::network(const std::string& con_ip_addr, const std::string& con_port, const int con_sock_type):
                 network(con_ip_addr, con_port, AF_UNSPEC, con_sock_type){}
#endif

network::~network() {
  if (nw_connected) {
    close(socket_fd);
    nw_connected = false;
  }
  con_sock_addr = nullptr;
  freeaddrinfo(dest_info);  
#if defined(_WIN32) || defined(_WIN64)
  if (wInitStatus == 0) {
    WSACleanup();
  }
#endif
}

/*
  set_address()
  return:
        0 -> success
    other -> error(return value of getaddrinfo())
*/
int network::set_address(const std::string& dest_ip,
                         const std::string& dest_port){
  if(nw_connected) {
    close(socket_fd);
    nw_connected = false;
  }
#ifdef __APPLE__
  addr_info_hint.ai_family = PF_UNSPEC;
#else
  addr_info_hint.ai_family = AF_UNSPEC;
#endif
  addr_info_hint.ai_socktype = SOCK_STREAM;
  return common_setup(dest_ip, dest_port);
}

/*
  set_address()
  return:
        0 -> success
    other -> error(return value of getaddrinfo())
*/
int network::set_address(const std::string& dest_ip,
                         const std::string& dest_port,
                         const int con_family, const int con_sock_type) {
  if (nw_connected) {
    close(socket_fd);
    nw_connected = false;
  }
  addr_info_hint.ai_family = con_family;
  addr_info_hint.ai_socktype = con_sock_type;
  return common_setup(dest_ip, dest_port);
}

/*
  nw_connect(bool is_blocking)
    used if the application is intended to behave as client.
  return:
                0 -> success
               -1 -> at least one of the specified IP address and port is not known
    negated errno -> failed
*/
int network::nw_connect(bool is_blocking) {
  int errno_ret = 0;
  int connection_status = 0;

  if (nw_connected) {
    return 0;
  }
  if (dest_info == nullptr) {
    return -9999;
  }
  blocking_io = is_blocking;
  for (struct addrinfo* di_ref = dest_info;
       di_ref != nullptr; di_ref = di_ref->ai_next) {
    if (network_force_return_req) {
      break;
    }
    socket_fd = socket(di_ref->ai_family, di_ref->ai_socktype,
                       di_ref->ai_protocol);
    con_sock_addr = di_ref->ai_addr;
    con_sock_addr_len = di_ref->ai_addrlen;
    errno_ret = errno;
    if (socket_fd != -1) {
      if (!is_blocking) {
#if defined(_WIN32) || defined(_WIN64)
        u_long ioctlret = 1;
        ioctlsocket(socket_fd, FIONBIO, &ioctlret);
#else
        fcntl(socket_fd, F_SETFL, O_NONBLOCK);
#endif
        do {
          connection_status = connect(socket_fd,
                                      di_ref->ai_addr, di_ref->ai_addrlen);
          errno_ret = errno;
          if ((errno_ret == EISCONN) || (connection_status == 0)) {
            break;
          }
        } while ((errno_ret == EINPROGRESS) || (errno_ret == EALREADY)
                  && !network_force_return_req);
        if ((connection_status != -1) || (errno_ret == EISCONN)) {
          recv_pollfd.fd = socket_fd;
          send_pollfd.fd = socket_fd;
          nw_connected = true;
          return 0;
        }
      } else {
          connection_status = connect(socket_fd, di_ref->ai_addr, di_ref->ai_addrlen);
          errno_ret = errno;
          if (connection_status == 0) {
            nw_connected = true;
            break;
          }
      }
    }
  }
  return -1 * errno_ret;
}
/*
  nw_accept(struct sockaddr* peer_addr, socklen_t* peer_addr_len)
    alias of accept()
  return:
    non-negative value -> new connected socket
    negative value -> negated errno
*/
int network::nw_accept(struct sockaddr* peer_addr, socklen_t* peer_addr_len){
  int accepted_fd = 0;
  int accept_errno = 0;
  int poll_res = 0;
  int poll_errno = 0;
  struct sockaddr connected_from;
  socklen_t cf_length = sizeof(connected_from);

#ifdef _GNU_SOURCE
  constexpr short ac_hup_mask = 0|POLLRDHUP|POLLHUP;
#else
  constexpr short ac_hup_mask = 0|POLLHUP;
#endif
  if (!nw_connected) {
    return 0;
  }
  if (nw_connected) {
    poll_res = poll(&accept_pollfd, 1, EM_TIMEOUT_LENGTH);
    poll_errno = (ssize_t)errno;
    if (poll_res == -1) {
      return (ssize_t)(poll_errno * -1);
    }
    if (poll_res == 0) {
      return (ssize_t)EM_CONNECTION_TIMEDOUT;
    }
    if (accept_pollfd.revents & ac_hup_mask) {
      return (ssize_t)EM_CONNECTION_CLOSED;
    }
    if (accept_pollfd.revents & (0|POLLERR|POLLNVAL)) {
      return (ssize_t)EM_ERR;
    }
#ifdef _GNU_SOURCE
    int ac_flag = 0;
    if (!blocking_io) {
      ac_flag = 0|SOCK_NONBLOCK|SOCK_CLOEXEC;
    }
    accepted_fd = accept4(socket_fd, peer_addr, peer_addr_len, ac_flag);
#else      
    accepted_fd = accept(socket_fd, peer_addr, peer_addr_len);
#endif
    accept_errno = errno;      
    if (accepted_fd > 0) {
      return accepted_fd;
    }
  }
  return -1 * accept_errno;
}
/*
  nw_bind_and_listen(bool is_blocking)
    used if the application is intended to behave as server.
  return:
        0 -> success
       -1 -> at least one of the specified IP address and port is not known
    errno -> failed
*/
int network::nw_bind_and_listen(bool is_blocking){
  int errno_ret = 0;
  int bind_status = 0;
  addr_info_hint.ai_flags = addr_info_hint.ai_flags | AI_PASSIVE;
  common_setup(ip_addr, port_info);
  if (nw_connected) {
    return 0;
  }
  if (dest_info == nullptr) {
    return -9999;
  }
  blocking_io = is_blocking;
  for (struct addrinfo* di_ref = dest_info; di_ref != nullptr; di_ref = di_ref->ai_next) {
    if (network_force_return_req) {
      break;
    }
    socket_fd = socket(di_ref->ai_family, di_ref->ai_socktype, di_ref->ai_protocol);
    errno_ret = errno;
    if (socket_fd != -1) {
      bind_status = bind(socket_fd, di_ref->ai_addr, di_ref->ai_addrlen);
      errno_ret = errno;
      if (bind_status == 0) {
        if (socket_type == SOCK_STREAM) {
          int listen_status = 0;
          listen_status = listen(socket_fd, 8);
          errno_ret = errno;
          if (listen_status != 0) {
            return errno_ret;
          } 
        }
        nw_connected = true;
        if (!is_blocking) {
#if defined(_WIN32) || defined(_WIN64)
          u_long ioctlret = 1;
          ioctlsocket(socket_fd, FIONBIO, &ioctlret);
#else
          fcntl(socket_fd, F_SETFL, O_NONBLOCK);
#endif
        }
        recv_pollfd.fd = socket_fd;
        send_pollfd.fd = socket_fd;
        accept_pollfd.fd = socket_fd;
        return 0;
      }
    }
  }
  return errno_ret;
}

/*
  send_data(uint8_t* data_arr, const size_t sb_size, const struct sockaddr* dest_addr, const socklen_t dest_addr_len)
    send data to destination

  arguments:
         data_arr -> an array that contains data
          sb_size -> number of elements
        dest_addr -> pointer to destination address
    dest_addr_len -> size of dest_addr

  arguments "dest_addr" and "dest_addr_len" can be omitted

  return:
                           send success -> sent length (actual size)
  socket is neither bound nor connected -> 0
                              send fail -> negated errno
                      connection closed -> -250(EM_CONNECTION_CLOSED)
                passed array is nullptr -> -256(EM_ERR)
*/
ssize_t network::send_data(uint8_t* data_arr, const size_t sb_size, const struct sockaddr* dest_addr, const socklen_t dest_addr_len) {
  if (data_arr == nullptr) {
    return (ssize_t)EM_ERR;
  }
  if (!nw_connected && (socket_type == SOCK_STREAM)) {
    return 0;
  }
  return send_data_common(data_arr, sb_size, dest_addr, dest_addr_len);
}

/*
  send_data(uint16_t* data_arr, const size_t sb_size, const struct sockaddr* dest_addr, const socklen_t dest_addr_len)
    send data to destination

  arguments:
         data_arr -> an array that contains data
          sb_size -> number of elements (not actual size)
        dest_addr -> pointer to socket address
    dest_addr_len -> size of dest_addr

  arguments "dest_addr" and "dest_addr_len" can be omitted

  return:
                           send success -> sent length (actual size)
    socket neither binded nor connected -> 0
                              send fail -> negated errno
                      connection closed -> -250(EM_CONNECTION_CLOSED)
               passed buffer is nullptr -> -256(EM_ERR)
*/
ssize_t network::send_data(uint16_t* data_arr, const size_t sb_size, const struct sockaddr* dest_addr, const socklen_t dest_addr_len) {
  if (data_arr == nullptr) {
    return (ssize_t)EM_ERR;
  }
  if (!nw_connected && (socket_type == SOCK_STREAM)) {
    return 0;
  }
  size_t temp_counter = 0;

  while(temp_counter < sb_size){
    if (network_force_return_req) {
      break;
    }
    data_arr[temp_counter] = htons(data_arr[temp_counter]);
    temp_counter++;
  }

  return send_data_common(data_arr, sb_size, dest_addr, dest_addr_len);
}

/*
  send_data(uint32_t* data_arr, const size_t sb_size, const struct sockaddr* dest_addr, const socklen_t dest_addr_len)
    send data to destination

  arguments:
         data_arr -> an array that contains data
          sb_size -> number of elements (not actual size)
        dest_addr -> pointer to socket address
    dest_addr_len -> size of dest_addr

  arguments "dest_addr" and "dest_addr_len" can be omitted

  return:
                           send success -> sent length (actual size)
    socket neither binded nor connected -> 0
                              send fail -> negated errno
                      connection closed -> -250(EM_CONNECTION_CLOSED)
               passed buffer is nullptr -> -256(EM_ERR)
*/
ssize_t network::send_data(uint32_t* data_arr, const size_t sb_size, const struct sockaddr* dest_addr, const socklen_t dest_addr_len) {
  if (data_arr == nullptr) {
    return (ssize_t)EM_ERR;
  }
  if (!nw_connected && (socket_type == SOCK_STREAM)) {
    return 0;
  }
  size_t temp_counter = 0;

  while(temp_counter < sb_size){
    if (network_force_return_req) {
      break;
    }
    data_arr[temp_counter] = htonl(data_arr[temp_counter]);
    temp_counter++;
  }
  return send_data_common(data_arr, sb_size, dest_addr, dest_addr_len);
}

void network::nw_close(){
  if (socket_type == SOCK_STREAM) {
#if defined(_WIN32) || defined(_WIN64)
    shutdown(socket_fd, SD_BOTH);
#else
    shutdown(socket_fd, SHUT_RDWR);
#endif
  }
  close(socket_fd);
  nw_connected = false;
}

fd_network::fd_network(int init_fd){
  socket_fd = init_fd;
  recv_pollfd.fd = init_fd;
  send_pollfd.fd = init_fd;

  nw_connected = true;
  addr_info_hint.ai_flags = 0;
  addr_info_hint.ai_protocol = 0;

#ifdef _GNU_SOURCE
  recv_pollfd.events = POLLRDNORM | POLLRDHUP | POLLHUP | POLLERR | POLLNVAL;
  accept_pollfd.events = POLLRDNORM | POLLRDHUP | POLLHUP
                       | POLLERR | POLLNVAL;
#else
  recv_pollfd.events = POLLRDNORM | POLLHUP | POLLERR | POLLNVAL;
  accept_pollfd.events = POLLRDNORM | POLLHUP | POLLERR | POLLNVAL;
#endif
  send_pollfd.events = POLLWRNORM | POLLHUP | POLLERR | POLLNVAL;
}
