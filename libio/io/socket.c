#include <stdint.h>
#include <WinSock2.h>
#include <common.h>
#pragma comment(lib, "ws2_32.lib")

enum
{
  SOCKET_ON_STATUS,
  SOCKET_ON_READ,
  SOCKET_CALLBACK,
};

#if 0
  +----+----+ +----+----+ +----+----+ +----+----+
  +----+----+ +----+----+ +----+----+ +----+----+
   ^^   ^^     ^^^^
   ||   ||     ||||
   ||   ||     ||||
   ||   ||     ||||
   ||   ||     ||||
   ||   ||     ||||
   ||   ||     ||++----------------------------------- 11 : BROADCAST, 10 : MULTICAST, 00 : UNICAST, 01 : RAW
   ||   ||     ||
   ||   ||     |+------------------------------------- 1 : UDP,     0 : TCP
   ||   ||     +-------------------------------------- 1 : SERVER,  0 : CLIENT
   ||   ||
   ||   |+-------------------------------------------- 1 : Reader Stop Request
   ||   +--------------------------------------------- 1 : Reader in Thread
   ||
   |+------------------------------------------------- 1 : accepter stop
   +-------------------------------------------------- 1 : accepter in thread
#endif


#if 0
  +----+----+ +----+----+ +----+----+ +----+----+
  +----+----+ +----+----+ +----+----+ +----+----+
   1110 0000   0000 0000    F     D      0    B
                            F     D      0    F
                            F     D      0    A
                            F     D      B    B
                            F     D      B    A
                            1     0      0    B
                            1     0      0    A
                            1     0      1    B
                            1     0      1    A
  0xE000FD0B
  0xE000FD0A
  0xE000100B
  0xE000100A
  0xE000101B
  0xE000101A
#endif


#define MAX_CLIENT       256


#define BUFFER_COUNT     128
#define BUFFER_SIZE      256




#pragma pack(1)
typedef struct
{
  int32_t fd;
  int8_t* p;
  //int8_t _b[BUFFER_COUNT][BUFFER_SIZE];
} fds;
#pragma pack()


#pragma pack(1)
typedef struct
{
  fd_set fdset;
  volatile fds _fds[MAX_CLIENT];
} tagCSocketClient;
#pragma pack()



#pragma pack(1)
typedef struct
{
  int32_t (*callback[SOCKET_CALLBACK])(void* h, int32_t fd, int8_t* b, int32_t sz, int32_t err, void* o);
  void*    o;

  void*    _thr[SOCKET_CALLBACK];
  uint32_t _tid[SOCKET_CALLBACK];
  uint32_t _SR_;
  int32_t  fd;

  CRITICAL_SECTION    _cr;
  struct sockaddr_in  _in;
  tagCSocketClient    _client;
} tagCSocket;
#pragma pack()

void print_client_fd(tagCSocketClient* p)
{
  int32_t i = 0;

  for ( i=0 ; i<MAX_CLIENT ; i++ )
  {
    if ( i && ((i%16)==0) ) printf("\r\n");
    else if ( i && ((i%8)==0) ) printf("  ");
    printf("  %4d", p->_fds[i].fd);
  }
  printf("\r\n");
}


int32_t set_client_fd(tagCSocketClient* p, int32_t fd)
{
  int32_t e = -1;
  int32_t i = 0;

  for ( i=0 ; i<MAX_CLIENT ; i++ )
  {
    if ( p->_fds[i].fd <= 0 )
    {
      p->_fds[i].fd = fd;
      e = i;
      break;
    }
  }
  return e;
}

int32_t clear_client_fd(tagCSocketClient* p, int32_t fd)
{
  int32_t e = -1;
  int32_t i = 0;

  for ( i=0 ; i<MAX_CLIENT ; i++ )
  {
    if ( fd == -1 )
    {
      p->_fds[i].fd = 0;
      e = i;
    }
    else
    {
      if ( p->_fds[i].fd == fd )
      {
        p->_fds[i].fd = 0;
        e = i;
        break;
      }
    }
  }

  return e;
}





__declspec(dllexport)
int32_t socket_read(void* h, int32_t fd, int8_t* b, int32_t sz)
{
  int32_t e = 0;

  e = recv(fd, b, sz, 0);

  return e;
}

__declspec(dllexport)
int32_t socket_write(void* h, int32_t fd, int8_t* b, int32_t sz)
{
  int32_t e = 0;

  e = send(fd, b, sz, 0);

  return e;
}

__declspec(dllexport)
int32_t socket_readfrom(void* h, int32_t fd, int8_t* b, int32_t sz, struct sockaddr_in* addr)
{
  int32_t e = 0;
  int32_t l = sizeof(struct sockaddr_in);

  e = recvfrom(fd, b, sz, 0, (struct sockaddr*)addr,&l);

  return e;
}

__declspec(dllexport)
int32_t socket_writeto(void* h, int32_t fd, int8_t* b, int32_t sz, struct sockaddr_in* addr)
{
  int32_t e = 0;
  int32_t l = sizeof(struct sockaddr_in);

  e = sendto(fd, b, sz, 0, (struct sockaddr*)addr, &l);

  return e;
}

int32_t connection_status(int32_t fd, int32_t timeout)
{
  int32_t        e=0;
  fd_set         rset, wset;
  struct timeval tval={0};
  int32_t        optval;
  uint32_t       optlen;

  FD_ZERO(&rset);
  FD_SET(fd, &rset);
  wset = rset;
  tval.tv_sec = 0;
  tval.tv_usec = timeout*1000;
  
  // wait connect for a specified timeout
  e = select(fd+1,&rset,&wset,0, &tval);
  if ( e )
  {
    optlen = sizeof(optval);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, (int8_t *)&optval, &optlen);
    if ( optval == 0 ) e = 0;
    else e = 0xE000FD0F;
  }
  else
  {
    e = 0xE000FD0F;
  }
  return e;
}

#if 0
  +----+----+ +----+----+ +----+----+ +----+----+
                                                 
  +----+----+ +----+----+ +----+----+ +----+----+
#endif



void* socket_accepter(void* arg)
{
  int32_t idx = 0;
  int32_t e = 0;
  int8_t b[32] = {0};
  struct sockaddr client = {0};
  struct sockaddr_in* pc = &client;
  int32_t csz = sizeof(struct sockaddr_in);
  int32_t cfd = 0;
  int32_t cbinfo[1024] = {0};
  tagCSocket* p = (tagCSocket*)arg;

  xSET_SEMAPHORE(p->_SR_, 0x80000000, 0x80000000);
  while ( xCHECK_SEMAPHORE(p->_SR_,0x00000000, 0x40000000) )
  {
    p->callback[SOCKET_ON_STATUS](p->o, p->fd, 0, 0, 0xE000FDAB, 0);
    cfd = accept(p->fd, &client, &csz);
    if ( cfd > 0 )
    {
      xLOCK(&p->_cr);
      idx = set_client_fd(&p->_client, cfd);
      xUNLOCK(&p->_cr);
      //print_client_fd(&p->_client);
      sprintf(cbinfo, "%d:%d.%d.%d.%d:%d",cfd,
              (pc->sin_addr.s_addr&0x000000FF),
              (pc->sin_addr.s_addr&0x0000FF00)>>8,
              (pc->sin_addr.s_addr&0x00FF0000)>>16,
              (pc->sin_addr.s_addr&0xFF000000)>>24,
              htons(pc->sin_port));
      p->callback[SOCKET_ON_STATUS](p->o, cfd, cfd>0?cbinfo:0, cfd>0?strlen(cbinfo):0, 0xE000FDA0, 0);
      p->callback[SOCKET_ON_STATUS](p->o, cfd, (int8_t*)&p->_client._fds[idx].p, idx, 0xE000FD10, 0);
    }
    p->callback[SOCKET_ON_STATUS](p->o, p->fd, cfd>0?cbinfo:0, cfd>0?strlen(cbinfo):0, 0xE000FDAA, 0);
    zDelay(1);
  }
  zDelay(1000);
  xSET_SEMAPHORE(p->_SR_, 0x00000000, 0xC0000000 );
  //p->_SR_ &= 0x3FFFFFFF;
  CloseHandle(p->_thr[0]);
  return 0;
}


void* __socket_reader_c(tagCSocket* p)
{
  int32_t i = 0;
  int32_t e = 0;
  int8_t b[1024] = {0};


  e = connection_status(p->fd, 4000);
  if ( e < 0 ) return e;
 
  while ( xCHECK_SEMAPHORE(p->_SR_,0x00000000, 0x04000000) )
  {
    p->callback[SOCKET_ON_STATUS](p->o, p->fd, 0, 0, 0xE000101B, 0);
    e = socket_read(p, p->fd, b, 1024);
    if ( e > 0 )
    {
      p->callback[SOCKET_ON_READ](p->o, p->fd, b, e, 0, 0);
    }
    else
    {
      if ( e == 0 )
      {
        p->callback[SOCKET_ON_STATUS](p->o, p->fd, 0, 0, 0xE000101F, 0);
      }
    }
    p->callback[SOCKET_ON_STATUS](p->o, p->fd, 0, 0, 0xE000101A, 0);
    zDelay(1);
  }
}

void* __socket_reader_d(tagCSocket* p)
{
  int32_t i = 0;
  int32_t sz = 0;
  int32_t e = 0;
  int32_t fd = 0;
  int8_t b[1024] = {0};

  while ( xCHECK_SEMAPHORE(p->_SR_,0x00000000, 0x40000000) )
  {
    fd = p->_client._fds[i].fd;
    if ( fd > 0 )
    {
      p->callback[SOCKET_ON_STATUS](p->o, fd, (int8_t*)&p->_client._fds[i].p, i, 0xE000101B, 0);

      sz = 1 << *(p->_client._fds[i].p+0);
      //p->_client._fds[i].p = p->_client._fds[i]._b[idx];
      e = socket_read(p, fd, p->_client._fds[i].p, sz);
      if ( e > 0 )
      {
        p->callback[SOCKET_ON_READ](p->o, fd, p->_client._fds[i].p, e, 0, 0);
        p->callback[SOCKET_ON_STATUS](p->o, fd, (int8_t*)&p->_client._fds[i].p, i, 0xE0001010, 0);

      }
      else
      {
        if ( e == 0 )
        {
          p->callback[SOCKET_ON_STATUS](p->o, fd, 0, 0, 0xE000101F, 0);
          xLOCK(&p->_cr);
          clear_client_fd(&p->_client, fd);
          xUNLOCK(&p->_cr);
          //print_client_fd(&p->_client);
        }
      }
      p->callback[SOCKET_ON_STATUS](p->o, fd, 0, 0, 0xE000101A, 0);
    }
    i = ((++i)%MAX_CLIENT);

    zDelay(1);
  }
}



void* socket_reader(void* arg)
{
  tagCSocket* p = (tagCSocket*)arg;

  xSET_SEMAPHORE(p->_SR_, 0x08000000, 0x08000000);
  if ( xCHECK_SEMAPHORE(p->_SR_,0x00800000, 0x00800000) )
  {
    __socket_reader_d(p);
  }
  else
  {
    __socket_reader_c(p);
  }
  xSET_SEMAPHORE(p->_SR_, 0x00000000, 0x0C000000 );
  CloseHandle(p->_thr[1]);
  return 0;
}


int32_t socket_option(int32_t fd)
{
  uint64_t  opt = 1;
  ioctlsocket(fd, FIONBIO, &opt);
  return 0;
}

__declspec(dllexport)
int32_t socket_open(void** h, int8_t* ip, int8_t* port, int8_t* cstype, int8_t* protocol, int8_t* casttype, int32_t (*callback[])(void*,int32_t,int8_t*,int32_t,int32_t,void*), void* o)
{
  int32_t e = 0;
  uint32_t _ip = inet_addr(ip);
  uint16_t _port = atoi(port);
	WSADATA wsaData = {0};
  tagCSocket* p = 0;


  uint32_t tid;
  uint32_t thr;
  tagCSocket sck = {0};
  uint32_t c = 0;

	if ( WSAStartup(0x202, &wsaData) != 0 ) return 0xE0000001;

  p = *h = (tagCSocket*)calloc(1, sizeof(tagCSocket));

  if ( *cstype=='S' || *cstype=='s' ) xSET_SEMAPHORE(p->_SR_,0x00800000,0x00800000);
  if ( *protocol=='U' || *protocol=='u' ) xSET_SEMAPHORE(p->_SR_,0x00400000,0x00400000);
  if ( *casttype=='B' || *casttype=='b' ) xSET_SEMAPHORE(p->_SR_,0x00300000,0x00300000);
  else if ( *casttype=='M' || *casttype=='m' ) xSET_SEMAPHORE(p->_SR_,0x00200000,0x00200000);
  else if ( *casttype=='R' || *casttype=='r' ) xSET_SEMAPHORE(p->_SR_,0x00100000,0x00100000);

  xLOCK_INIT(&p->_cr);

  p->callback[SOCKET_ON_STATUS] = callback[SOCKET_ON_STATUS];
  p->callback[SOCKET_ON_READ]   = callback[SOCKET_ON_READ];
  p->o = o;

  p->callback[SOCKET_ON_STATUS](p->o, 0, 0, 0, 0xE000FD0B, 0);
  p->fd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  if ( p->fd <= 0 )   p->callback[SOCKET_ON_STATUS](p->o, p->fd, 0, 0, 0xE000FD0F, 0);
  p->callback[SOCKET_ON_STATUS](p->o, p->fd, 0, 0, 0xE000FD0A, 0);

  socket_option(p->fd);

  p->_in.sin_family = AF_INET;
  p->_in.sin_port = htons(_port);

  if ( xCHECK_SEMAPHORE(p->_SR_,0x00800000,0x00800000) )
  {
    p->_in.sin_addr.s_addr = htonl(INADDR_ANY);

    p->callback[SOCKET_ON_STATUS](p->o, p->fd, 0, 0, 0xE000FDBB, 0);
    e = bind(p->fd, (struct sockaddr*)&(p->_in), sizeof(struct sockaddr));
    if ( e < 0 )   p->callback[SOCKET_ON_STATUS](p->o, p->fd, 0, 0, 0xE000FDBF, 0);
    p->callback[SOCKET_ON_STATUS](p->o, p->fd, 0, 0, 0xE000FDBA, 0);

    p->callback[SOCKET_ON_STATUS](p->o, p->fd, 0, 0, 0xE000FD7B, 0);
    listen(p->fd, 5);
    p->callback[SOCKET_ON_STATUS](p->o, p->fd, 0, 0, 0xE000FD7A, 0);

    ////// create thread
    xTHREAD_CREATE(socket_accepter, p, &p->_tid[0], p->_thr[0]);
    xGET_SEMAPHORE(p->_SR_,0x80000000,0x80000000,500);
  }
  else
  {
    p->_in.sin_addr.s_addr = _ip;
    e = connect(p->fd, (struct sockaddr*)&p->_in, sizeof(p->_in));
  }

  xTHREAD_CREATE(socket_reader, p, &p->_tid[1], p->_thr[1]);
  xGET_SEMAPHORE(p->_SR_,0x08000000,0x08000000,500);

  return e<0?e:p->fd;
}



__declspec(dllexport)
int32_t socket_close(void** h)
{
  int32_t e = 0;
  tagCSocket* p = (tagCSocket*)(*h);


  xSET_SEMAPHORE(p->_SR_,0x44000000,0x44000000);
  xGET_SEMAPHORE(p->_SR_,0x00000000,0xCC000000,500);

  p->callback[SOCKET_ON_STATUS](p->o, p->fd, 0, 0, 0xE000FDCB, 0);
  shutdown(p->fd, SD_BOTH);
  closesocket(p->fd);
  p->callback[SOCKET_ON_STATUS](p->o, p->fd, 0, 0, 0xE000FDCA, 0);

  xLOCK_FINAL(&p->_cr);

  if ( *h )
  {
    free(*h);
    *h = 0;
  }


	WSACleanup();

  return e;
}
