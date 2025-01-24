#include "NetworkInterface.h"

#ifdef _NETWORK_INTERFACE_USE_FREERTOS
  #define NetworkInterface_Delay osDelay
#else
  #define NetworkInterface_Delay HAL_Delay
#endif

// Private variables
uint16_t W5500_ReadWriteBuff_For_i;
// Private functions ----------------------------------------------------------------------------
static char * socketErrorString(int32_t sockErrorCode) {
  char * returnValue = NULL;
  switch(sockErrorCode) {
    case SOCK_OK           : returnValue = "[SOCK_OK] => Result is OK about socket process."; break;
    case SOCK_BUSY         : returnValue = "[SOCK_ERROR] or [SOCK_BUSY] => Socket is busy on processing the operation. Valid only Non-block IO Mode."; break;
    case SOCK_FATAL        : returnValue = "[SOCK_FATAL] => Result is fatal error about socket process."; break;
    case SOCKERR_SOCKNUM   : returnValue = "[SOCKERR_SOCKNUM] => Invalid socket number."; break;
    case SOCKERR_SOCKOPT   : returnValue = "[SOCKERR_SOCKOPT] => Invalid socket option."; break;
    case SOCKERR_SOCKINIT  : returnValue = "[SOCKERR_SOCKINIT] => Socket is not initialized or SIPR is Zero IP address when Sn_MR_TCP."; break;
    case SOCKERR_SOCKCLOSED: returnValue = "[SOCKERR_SOCKCLOSED] => Socket unexpectedly closed."; break;
    case SOCKERR_SOCKMODE  : returnValue = "[SOCKERR_SOCKMODE] => Invalid socket mode for socket operation."; break;
    case SOCKERR_SOCKFLAG  : returnValue = "[SOCKERR_SOCKFLAG] => Invalid socket flag."; break;
    case SOCKERR_SOCKSTATUS: returnValue = "[SOCKERR_SOCKSTATUS] => Invalid socket status for socket operation."; break;
    case SOCKERR_ARG       : returnValue = "[SOCKERR_ARG] => Invalid argument."; break;
    case SOCKERR_PORTZERO  : returnValue = "[SOCKERR_PORTZERO] => Port number is zero."; break;
    case SOCKERR_IPINVALID : returnValue = "[SOCKERR_IPINVALID] => Invalid IP address."; break;
    case SOCKERR_TIMEOUT   : returnValue = "[SOCKERR_TIMEOUT] => Timeout occurred."; break;
    case SOCKERR_DATALEN   : returnValue = "[SOCKERR_DATALEN] => Data length is zero or greater than buffer max size."; break;
    case SOCKERR_BUFFER    : returnValue = "[SOCKERR_BUFFER] => Socket buffer is not enough for data communication."; break;
    case SOCKFATAL_PACKLEN : returnValue = "[SOCKFATAL_PACKLEN] => Invalid packet length. Fatal Error."; break;
    default: returnValue = "[Not Defined Error Code]"; break;
  }
  return returnValue;
}

// Low Level Implementation ---------------------------------------------------------------------
void W5500_ResetLow(void) {
  #ifdef _NETWORK_INTERFACE_USE_HAL_SPI_DRIVER
  HAL_GPIO_WritePin(DEFAULT_NETWORK_INTERFACE_RESET_GPIO_Port, DEFAULT_NETWORK_INTERFACE_RESET_Pin, GPIO_PIN_RESET);
  #else
  DEFAULT_NETWORK_INTERFACE_RESET_GPIO_Port->BSRR = (uint32_t)DEFAULT_NETWORK_INTERFACE_RESET_Pin << 16;
  #endif
}
void W5500_ResetHigh(void) {
  #ifdef _NETWORK_INTERFACE_USE_HAL_SPI_DRIVER
  HAL_GPIO_WritePin(DEFAULT_NETWORK_INTERFACE_RESET_GPIO_Port, DEFAULT_NETWORK_INTERFACE_RESET_Pin, GPIO_PIN_SET);
  #else
  DEFAULT_NETWORK_INTERFACE_RESET_GPIO_Port->BSRR = (uint32_t)DEFAULT_NETWORK_INTERFACE_RESET_Pin;
  #endif
}
void W5500_Select(void) {
  #ifdef _NETWORK_INTERFACE_USE_HAL_SPI_DRIVER
  HAL_GPIO_WritePin(DEFAULT_NETWORK_INTERFACE_CS_GPIO_Port, DEFAULT_NETWORK_INTERFACE_CS_Pin, GPIO_PIN_RESET);
  #else
  DEFAULT_NETWORK_INTERFACE_CS_GPIO_Port->BSRR = (uint32_t)DEFAULT_NETWORK_INTERFACE_CS_Pin << 16;
  #endif
}
void W5500_Unselect(void) {
  #ifdef _NETWORK_INTERFACE_USE_HAL_SPI_DRIVER
  HAL_GPIO_WritePin(DEFAULT_NETWORK_INTERFACE_CS_GPIO_Port, DEFAULT_NETWORK_INTERFACE_CS_Pin, GPIO_PIN_SET);
  #else
  DEFAULT_NETWORK_INTERFACE_CS_GPIO_Port->BSRR = (uint32_t)DEFAULT_NETWORK_INTERFACE_CS_Pin;
  #endif
}
void W5500_ReadBuff(uint8_t * buff, uint16_t len) {
  #ifdef _NETWORK_INTERFACE_USE_HAL_SPI_DRIVER
  HAL_SPI_Receive(&DEFAULT_HSPI_NETWORK_INTERFACE, buff, len, HAL_MAX_DELAY);
  #else
  MY_SPI_Receive(DEFAULT_SPI_NETWORK_INTERFACE, buff, len);
  #endif
}
void W5500_WriteBuff(uint8_t * buff, uint16_t len) {
  #ifdef _NETWORK_INTERFACE_USE_HAL_SPI_DRIVER
  HAL_SPI_Transmit(&DEFAULT_HSPI_NETWORK_INTERFACE, buff, len, HAL_MAX_DELAY);
  #else
  MY_SPI_Transmit(DEFAULT_SPI_NETWORK_INTERFACE, buff, len);
  #endif
}
uint8_t W5500_ReadByte(void) {
  #ifdef _NETWORK_INTERFACE_USE_HAL_SPI_DRIVER
  uint8_t receiveSpiData;
  HAL_SPI_Receive(&DEFAULT_HSPI_NETWORK_INTERFACE, &receiveSpiData, 1, HAL_MAX_DELAY);
  return receiveSpiData;
  #else
  return MY_SPI_ReceiveByte(DEFAULT_SPI_NETWORK_INTERFACE);
  #endif
}
void W5500_WriteByte(uint8_t spiData) {
  #ifdef _NETWORK_INTERFACE_USE_HAL_SPI_DRIVER
  HAL_SPI_Transmit(&DEFAULT_HSPI_NETWORK_INTERFACE, &spiData, 1, HAL_MAX_DELAY);
  #else
  MY_SPI_TransmitByte(DEFAULT_SPI_NETWORK_INTERFACE, spiData);
  #endif
}
// ----------------------------------------------------------------------------------------------

// Run Time Variables ---------------------------------------------------------------------------
#define WIZNET_SOCKET_IO_BLOCKING_MODE SF_IO_NONBLOCK

#ifdef _NETWORK_INTERFACE_INCLUDE_DHCP
static uint8_t dhcp_buffer[2048]; // 1K should be enough, see https://forum.wiznet.io/t/topic/1612/2
#endif
//static uint8_t dns_buffer[1024]; // 1K seems to be enough for this buffer as well

static Type_IPAssignMode ipAssignMode = IPAssignMode_Static;

volatile bool ip_assigned = false;
volatile uint32_t ctr = 10000;

static uint16_t runTimeVariable_ReceivedDataSize;
static Type_IP4Address runTimeVariable_SenderIP;
static uint16_t runTimeVariable_SenderPort;

static uint8_t runTimeVariable_SocketCounter2;

static wiz_NetInfo net_info;
static int8_t networkInterfaceResult;
static bool w5500SocketIsBusy[_WIZCHIP_SOCK_NUM_] = { false, false, false, false, false, false, false, false };
static void* usingNetworkClients[_WIZCHIP_SOCK_NUM_] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
wiz_NetTimeout netTimeout = { 10 /* Rerty */, 10000 /* 100 * 100us = 10ms */};
// ----------------------------------------------------------------------------------------------

// Callbacks ------------------------------------------------------------------------------------
void Callback_IPAssigned(void) {
  PROGRAM_LOG("Callback: IP assigned! Leased time:% d sec\r\n", getDHCPLeasetime());
  ip_assigned = true;
}

void Callback_IPConflict(void) {
  PROGRAM_LOG("Callback: IP conflict!\r\n");
}
// ----------------------------------------------------------------------------------------------

// Static Functions -----------------------------------------------------------------------------
static bool W5500SocketManager_ReserveFreeSocket(uint8_t* socketNumber) {
  bool thisFunctionResult = false; 
  for(uint8_t getW5500FreeSockcetCounter = 0; getW5500FreeSockcetCounter < 8; getW5500FreeSockcetCounter++) {
    if(w5500SocketIsBusy[getW5500FreeSockcetCounter] == false) {
      *socketNumber = getW5500FreeSockcetCounter;
      w5500SocketIsBusy[getW5500FreeSockcetCounter] = true;
      thisFunctionResult = true;
      break;
    }
  }
  return thisFunctionResult;
}
static bool W5500SocketManager_ReserveSocket(uint8_t socketNumber) {
  bool thisFunctionResult = false; 
  if(w5500SocketIsBusy[socketNumber] == false) {
    w5500SocketIsBusy[socketNumber] = true;
    thisFunctionResult = true;
  }
  return thisFunctionResult;
}
static void W5500SocketManager_ReleaseSocket(uint8_t socketNumber) {
  w5500SocketIsBusy[socketNumber] = false;
}

// Exported Functions ---------------------------------------------------------------------------
bool TransmitDataIndependentUdpTest(Type_IP4Address ipA4Address) {
  bool thisFunctionResult = true;
  uint8_t selectedSocket = 0;
  networkInterfaceResult = socket(selectedSocket, Sn_MR_UDP, 3000, SF_IO_NONBLOCK); // Success : The socket number 'sn' passed as parameter
  if(networkInterfaceResult == selectedSocket) {
    PROGRAM_LOG("socket=<(%d)>%s\r\n", 0, "OK");
    char * testUdpData = "MohammadSayadi";
    networkInterfaceResult = sendto(selectedSocket, (uint8_t*)testUdpData, strlen(testUdpData), (void*)&ipA4Address, 65001);
    if(networkInterfaceResult > SOCK_ERROR) {
      PROGRAM_LOG("sendto=<(%d Bytes)>%s\r\n", networkInterfaceResult, "OK");
    }
    else {
      PROGRAM_LOG("sendto=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
      thisFunctionResult = false;
    }
    networkInterfaceResult = close(selectedSocket);
    PROGRAM_LOG("close=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
  }
  else {
    PROGRAM_LOG("socket=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
    thisFunctionResult = false;
  }
  return thisFunctionResult;
}
bool TransmitDataIndependentTcpTest(Type_IP4Address ipA4Address) {
  bool thisFunctionResult = true;
  uint8_t selectedSocket = 0;
  networkInterfaceResult = socket(selectedSocket, Sn_MR_TCP, 60001, 0); // Success : The socket number 'sn' passed as parameter
  if(networkInterfaceResult == selectedSocket) {
    PROGRAM_LOG("socket=<(%d)>%s\r\n", 0, "OK");
    networkInterfaceResult = connect(selectedSocket, (void*)&ipA4Address, 60001);
    PROGRAM_LOG("connect=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
    if(networkInterfaceResult == SOCK_OK) {
      char * testUdpData = "MohammadSayadi";
      networkInterfaceResult = send(selectedSocket, (uint8_t*)testUdpData, strlen(testUdpData) - 1);
      if(networkInterfaceResult > SOCK_ERROR) {
        PROGRAM_LOG("send=<(%d Bytes)>%s\r\n", networkInterfaceResult, "OK");
      }
      else {
        PROGRAM_LOG("send=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
        thisFunctionResult = false;
      }
      
      networkInterfaceResult = send(selectedSocket, (uint8_t*)testUdpData, strlen(testUdpData) - 1);
      if(networkInterfaceResult > SOCK_ERROR) {
        PROGRAM_LOG("send=<(%d Bytes)>%s\r\n", networkInterfaceResult, "OK");
      }
      else {
        PROGRAM_LOG("send=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
        thisFunctionResult = false;
      }
      
      networkInterfaceResult = send(selectedSocket, (uint8_t*)testUdpData, strlen(testUdpData) - 1);
      if(networkInterfaceResult > SOCK_ERROR) {
        PROGRAM_LOG("send=<(%d Bytes)>%s\r\n", networkInterfaceResult, "OK");
      }
      else {
        PROGRAM_LOG("send=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
        thisFunctionResult = false;
      }
      
      networkInterfaceResult = close(selectedSocket);
      PROGRAM_LOG("close=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
    }
    else {
      close(selectedSocket);
      thisFunctionResult = false;
    }
  }
  else {
    PROGRAM_LOG("socket=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
    thisFunctionResult = false;
  }
  return thisFunctionResult;
}
void NetworkInterface_Init1(void) {
//  // All capable, Auto-negotiation enabled
//  HAL_GPIO_WritePin(W5500_MODE0_GPIO_Port, W5500_MODE0_Pin, GPIO_PIN_SET);
//  HAL_GPIO_WritePin(W5500_MODE1_GPIO_Port, W5500_MODE1_Pin, GPIO_PIN_SET);
//  HAL_GPIO_WritePin(W5500_MODE2_GPIO_Port, W5500_MODE2_Pin, GPIO_PIN_SET);
//  
//  PROGRAM_LOG("Reset W5500\r\n");
//  W5500_ResetLow();
//  NetworkInterface_Delay(20);
//  W5500_ResetHigh();
//  NetworkInterface_Delay(50);
//  
//  reg_wizchip_cs_cbfunc(W5500_Select, W5500_Unselect);
//  PROGRAM_LOG("reg_wizchip_cs_cbfunc\r\n");
//  reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte);
//  PROGRAM_LOG("reg_wizchip_spi_cbfunc\r\n");
//  reg_wizchip_spiburst_cbfunc(W5500_ReadBuff, W5500_WriteBuff);
//  PROGRAM_LOG("reg_wizchip_spiburst_cbfunc\r\n");

//  uint8_t dummy[50];
//  memset(dummy, 0xFF, 50);
//  W5500_WriteBuff(dummy, 50);
//  W5500_Select();
//  NetworkInterface_Delay(50);
//  W5500_Unselect();
//  
//  wizchip_sw_reset();
//  
//  uint8_t rx_tx_buff_sizes[] = {2, 2, 2, 2, 2, 2, 2, 2};
//  wizchip_init(rx_tx_buff_sizes, rx_tx_buff_sizes);
//  PROGRAM_LOG("wizchip_init\r\n");

//  PROGRAM_LOG("Calling DHCP_init()...\r\n");
//  wiz_NetInfo net_info = {
//    .mac  = { 0xEA, 0x11, 0x22, 0x33, 0x44, 0xEA },
//    .dhcp = NETINFO_DHCP
//  };
//  // set MAC address before using DHCP
//  setSHAR(net_info.mac);
//  DHCP_init(APPLICATION_DHCP_SOCKET, dhcp_buffer);

//  PROGRAM_LOG("Registering DHCP callbacks...\r\n");
//  reg_dhcp_cbfunc(
//    Callback_IPAssigned,
//    Callback_IPAssigned,
//    Callback_IPConflict
//  );

//  PROGRAM_LOG("Calling DHCP_run()...\r\n");
//  // actually should be called in a loop, e.g. by timer
//  uint32_t ctr = 10000;
//  while((!ip_assigned) && (ctr > 0)) {
//    DHCP_run();
//    ctr--;
//    NetworkInterface_Delay(100);
//  }
//  if(!ip_assigned) {
//    PROGRAM_LOG("\r\nIP was not assigned :(\r\n");
//    return;
//  }

//  getIPfromDHCP(net_info.ip);
//  getGWfromDHCP(net_info.gw);
//  getSNfromDHCP(net_info.sn);

//  uint8_t dns[4];
//  getDNSfromDHCP(dns);

//  PROGRAM_LOG("IP:  %d.%d.%d.%d\r\nGW:  %d.%d.%d.%d\r\nNet: %d.%d.%d.%d\r\nDNS: %d.%d.%d.%d\r\n",
//    net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3],
//    net_info.gw[0], net_info.gw[1], net_info.gw[2], net_info.gw[3],
//    net_info.sn[0], net_info.sn[1], net_info.sn[2], net_info.sn[3],
//    dns[0], dns[1], dns[2], dns[3]
//  );

//  PROGRAM_LOG("Calling wizchip_setnetinfo()...\r\n");
//  wizchip_setnetinfo(&net_info);

//  PROGRAM_LOG("Calling DNS_init()...\r\n");
//  DNS_init(DNS_SOCKET, dns_buffer);

//  uint8_t addr[4];
//  {
//    char domain_name[] = "eax.me";
//    PROGRAM_LOG("Resolving domain name \"%s\"...\r\n", domain_name);
//    int8_t res = DNS_run(dns, (uint8_t*)&domain_name, addr);
//    if(res != 1) {
//        PROGRAM_LOG("DNS_run() failed, res = %d", res);
//        return;
//    }
//    PROGRAM_LOG("Result: %d.%d.%d.%d\r\n", addr[0], addr[1], addr[2], addr[3]);
//  }

//  PROGRAM_LOG("Creating socket...\r\n");
//  uint8_t http_socket = HTTP_SOCKET;
//  uint8_t code = socket(http_socket, Sn_MR_TCP, 10888, 0);
//  if(code != http_socket) {
//    PROGRAM_LOG("socket() failed, code = %d\r\n", code);
//    return;
//  }

//  PROGRAM_LOG("Socket created, connecting...\r\n");
//  code = connect(http_socket, addr, 80);
//  if(code != SOCK_OK) {
//    PROGRAM_LOG("connect() failed, code = %d\r\n", code);
//    close(http_socket);
//    return;
//  }

//  PROGRAM_LOG("Connected, sending HTTP request...\r\n");
//  {
//    char req[] = "GET / HTTP/1.0\r\nHost: eax.me\r\n\r\n";
//    uint16_t len = sizeof(req) - 1;
//    uint8_t* buff = (uint8_t*)&req;
//    while(len > 0) {
//      PROGRAM_LOG("Sending %d bytes...\r\n", len);
//      int32_t nbytes = send(http_socket, buff, len);
//      if(nbytes <= 0) {
//        PROGRAM_LOG("send() failed, %d returned\r\n", nbytes);
//        close(http_socket);
//        return;
//      }
//      PROGRAM_LOG("%d bytes sent!\r\n", nbytes);
//      len -= nbytes;
//    }
//  }

//  PROGRAM_LOG("Request sent. Reading response...\r\n");
//  {
//    char buff[32];
//    for(;;) {
//      int32_t nbytes = recv(http_socket, (uint8_t*)&buff, sizeof(buff)-1);
//      if(nbytes == SOCKERR_SOCKSTATUS) {
//          PROGRAM_LOG("\r\nConnection closed.\r\n");
//          break;
//      }

//      if(nbytes <= 0) {
//          PROGRAM_LOG("\r\nrecv() failed, %d returned\r\n", nbytes);
//          break;
//      }

//      buff[nbytes] = '\0';
//      PROGRAM_LOG("%s", buff);
//    }
//  }

//  PROGRAM_LOG("Closing socket.\r\n");
//  close(http_socket);
}
bool NetworkInterface_Init(Type_IPAssignMode ipAssignMode, uint8_t* mac, Type_NetworkProperties* networkProperties) {
  bool operationResult = false;
  
  // W5500_100BT_FullDuplex
  HAL_GPIO_WritePin(W5500_MODE0_GPIO_Port, W5500_MODE0_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(W5500_MODE1_GPIO_Port, W5500_MODE1_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(W5500_MODE2_GPIO_Port, W5500_MODE2_Pin, GPIO_PIN_SET);
  
  #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
  PROGRAM_LOG("Reset W5500\r\n");
  #endif
  W5500_ResetLow();
  NetworkInterface_Delay(20);
  W5500_ResetHigh();
  NetworkInterface_Delay(50);
  
  reg_wizchip_cs_cbfunc(W5500_Select, W5500_Unselect);
  #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
  PROGRAM_LOG("reg_wizchip_cs_cbfunc\r\n");
  #endif
  reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte);
  #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
  PROGRAM_LOG("reg_wizchip_spi_cbfunc\r\n");
  #endif
  reg_wizchip_spiburst_cbfunc(W5500_ReadBuff, W5500_WriteBuff);
  #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
  PROGRAM_LOG("reg_wizchip_spiburst_cbfunc\r\n");
  #endif
  
  uint8_t rx_tx_buff_sizes[] = {8, 2, 1, 1, 1, 1, 1, 1};
  networkInterfaceResult = wizchip_init(rx_tx_buff_sizes, rx_tx_buff_sizes);
  #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
  PROGRAM_LOG("wizchip_init=<(%d)>%s\r\n", 0, "OK");
  #endif
  
  //wizphy_setphypmode(PHYCFGR_OPMDC_10F);
  
  wizchip_settimeout(&netTimeout); // Very Important
  
  switch(ipAssignMode) {
    case IPAssignMode_Static: {
      memcpy(net_info.mac, mac, 6);
      Fill_IPAddressArrayFromStruct(net_info.ip, networkProperties->IPAddress);
      Fill_IPAddressArrayFromStruct(net_info.sn, networkProperties->SubnetMask);
      Fill_IPAddressArrayFromStruct(net_info.gw, networkProperties->DefaultGateway);
      Fill_IPAddressArrayFromStruct(net_info.dns, networkProperties->DnsServer);
      net_info.dhcp = NETINFO_STATIC;
      
      wizchip_setnetinfo(&net_info);
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("wizchip_setnetinfo\r\n");
      #endif
      
      wiz_NetInfo rnet_info;
      wizchip_getnetinfo(&rnet_info);
      
      if(
        rnet_info.mac[0] == net_info.mac[0] &&
        rnet_info.mac[1] == net_info.mac[1] &&
        rnet_info.mac[2] == net_info.mac[2] &&
        rnet_info.mac[3] == net_info.mac[3] &&
        rnet_info.mac[4] == net_info.mac[4] &&
        rnet_info.mac[5] == net_info.mac[5] &&
        rnet_info.ip[0]  == net_info.ip[0] &&
        rnet_info.ip[1]  == net_info.ip[1] &&
        rnet_info.ip[2]  == net_info.ip[2] &&
        rnet_info.ip[3]  == net_info.ip[3] &&
        rnet_info.gw[0] == net_info.gw[0] &&
        rnet_info.gw[1] == net_info.gw[1] &&
        rnet_info.gw[2] == net_info.gw[2] &&
        rnet_info.gw[3] == net_info.gw[3] &&
        rnet_info.sn[0] == net_info.sn[0] &&
        rnet_info.sn[1] == net_info.sn[1] &&
        rnet_info.sn[2] == net_info.sn[2] &&
        rnet_info.sn[3] == net_info.sn[3] &&
        rnet_info.dns[0] == net_info.dns[0] &&
        rnet_info.dns[1] == net_info.dns[1] &&
        rnet_info.dns[2] == net_info.dns[2] &&
        rnet_info.dns[3] == net_info.dns[3]     
      ) {
        operationResult = true;
      }
      
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("wizchip_getnetinfo\r\n");
      PROGRAM_LOG("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", rnet_info.mac[0], rnet_info.mac[1], rnet_info.mac[2], rnet_info.mac[3], rnet_info.mac[4], rnet_info.mac[5]);
      PROGRAM_LOG("Ip: %d.%d.%d.%d\r\n", rnet_info.ip[0] , rnet_info.ip[1] , rnet_info.ip[2] , rnet_info.ip[3] );
      PROGRAM_LOG("Gateway: %d.%d.%d.%d\r\n", rnet_info.gw[0] , rnet_info.gw[1] , rnet_info.gw[2] , rnet_info.gw[3] );
      PROGRAM_LOG("Subnet Mask: %d.%d.%d.%d\r\n", rnet_info.sn[0] , rnet_info.sn[1] , rnet_info.sn[2] , rnet_info.sn[3] );
      PROGRAM_LOG("DNS: %d.%d.%d.%d\r\n", rnet_info.dns[0], rnet_info.dns[1], rnet_info.dns[2], rnet_info.dns[3]);
      #endif
      
      break;
    } // case IPAssignMode_Static
    case IPAssignMode_Dhcp: {
      #ifdef _NETWORK_INTERFACE_INCLUDE_DHCP
      memcpy(net_info.mac, mac, 6);
      Fill_IPAddressArray(net_info.ip, 0, 0, 0, 0);
      Fill_IPAddressArray(net_info.sn, 0, 0, 0, 0);
      Fill_IPAddressArray(net_info.gw, 0, 0, 0, 0);
      Fill_IPAddressArray(net_info.dns, 0, 0, 0, 0);
      net_info.dhcp = NETINFO_DHCP;
      
      wizchip_setnetinfo(&net_info);
      PROGRAM_LOG("wizchip_setnetinfo(Init)\r\n");

      PROGRAM_LOG("DHCP_time_handler timer routine init\r\n");
      if(xTimerIsTimerActive(Timer_DhcpHandle) == pdTRUE ) {
          xTimerStop(Timer_DhcpHandle, (TickType_t)FREERTOS_MS_TO_TICK(2000));
      }
      xTimerChangePeriod(Timer_DhcpHandle, FREERTOS_MS_TO_TICK(1000), (TickType_t)FREERTOS_MS_TO_TICK(2000));
      xTimerStart(Timer_DhcpHandle, (TickType_t)FREERTOS_MS_TO_TICK(2000));
      
      W5500SocketManager_ReserveSocket(APPLICATION_DHCP_SOCKET);
      DHCP_init(APPLICATION_DHCP_SOCKET, dhcp_buffer);

      PROGRAM_LOG("Registering DHCP callbacks...\r\n");
      reg_dhcp_cbfunc(Callback_IPAssigned, Callback_IPAssigned, Callback_IPConflict);

      PROGRAM_LOG("Calling DHCP_run()...\r\n");
      // actually should be called in a loop, e.g. by timer
      volatile uint8_t dhcp_ret;
      while((!ip_assigned) && (ctr > 0)) {
//      while(dhcp_ret != DHCP_IP_ASSIGN && dhcp_ret != DHCP_IP_LEASED) {
        dhcp_ret = DHCP_run();
        ctr--;
        NetworkInterface_Delay(1);
      }
      if(!ip_assigned) {
        PROGRAM_LOG("\r\nIP was not assigned :(\r\n");
        return;
      }

      getIPfromDHCP(net_info.ip);
      getGWfromDHCP(net_info.gw);
      getSNfromDHCP(net_info.sn);
      
      uint8_t dns[4];
      getDNSfromDHCP(dns);
      
      PROGRAM_LOG("IP:  %d.%d.%d.%d\r\nGW:  %d.%d.%d.%d\r\nNet: %d.%d.%d.%d\r\nDNS: %d.%d.%d.%d\r\n",
        net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3],
        net_info.gw[0], net_info.gw[1], net_info.gw[2], net_info.gw[3],
        net_info.sn[0], net_info.sn[1], net_info.sn[2], net_info.sn[3],
        dns[0], dns[1], dns[2], dns[3]
      );
      
      wizchip_setnetinfo(&net_info);
      PROGRAM_LOG("wizchip_setnetinfo(From Dhcp)\r\n");
      #endif /* _NETWORK_INTERFACE_INCLUDE_DHCP */
      break;
    } // case IPAssignMode_Dhcp
  }
  return operationResult;
}
bool NetworkInterface_Udp_Open(Type_NetworkClient* udpClient, uint16_t localPort) {
  bool thisFunctionResult;
  uint8_t selectedSocket;
  if(udpClient) {
    if(W5500SocketManager_ReserveFreeSocket(&selectedSocket)) {
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("socket number = %d\r\n", selectedSocket);
      #endif
      networkInterfaceResult = socket(selectedSocket, Sn_MR_UDP, localPort, SF_IO_NONBLOCK); // Success : The socket number 'sn' passed as parameter
      if(networkInterfaceResult == selectedSocket) {
        udpClient->SocketNumber = selectedSocket;
        udpClient->LocalPort = localPort;
        thisFunctionResult = true;
        udpClient->LastStatus = NetworkStatus_Opened;
        udpClient->ClientType = ClientType_Udp;
        udpClient->IsDataReceived = false;
        udpClient->ReceivedDataSize = false;
        udpClient->ApplyReceiveDataFilterBasedOnIpAndPort = false;
        usingNetworkClients[selectedSocket] = udpClient;
        #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
        PROGRAM_LOG("socket=<(%d)>%s\r\n", 0, "OK");
        #endif
      }
      else {
        thisFunctionResult = false;
        #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
        PROGRAM_LOG("socket=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
        #endif
      }
    }
    else thisFunctionResult = false;
  }
  else {
    thisFunctionResult = false;
    #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
    PROGRAM_LOG("Invalid or NULL network client structure\r\n");
    #endif
  }
  return thisFunctionResult;
}
bool NetworkInterface_Udp_Connect(Type_NetworkClient* udpClient, Type_IP4Address destinationIP, uint16_t destinationPort) {
  bool thisFunctionResult = true;
  if(udpClient) {
    Fill_IP4AddressStructFromStruct(udpClient->RemoteIP, destinationIP);
    udpClient->RemotePort = destinationPort;
    uint8_t tmpip4[4];
    Fill_IPAddressArrayFromStruct(tmpip4, udpClient->RemoteIP);
    setSn_DIPR(udpClient->SocketNumber, tmpip4);
    setSn_DPORT(udpClient->SocketNumber, udpClient->RemotePort);
  }
  else {
    thisFunctionResult = false;
    #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
    PROGRAM_LOG("Invalid or NULL network client structure\r\n");
    #endif
  }
  return thisFunctionResult;
}
bool NetworkInterface_Udp_Send(Type_NetworkClient* udpClient, void* data, uint16_t length) {
  bool thisFunctionResult = true;
  if(udpClient) {
    networkInterfaceResult = sendto(udpClient->SocketNumber, data, length, (uint8_t*)&udpClient->RemoteIP, udpClient->RemotePort);
    if(networkInterfaceResult > SOCK_ERROR) {
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("send=<(%d Bytes)>%s\r\n", networkInterfaceResult, "OK");
      #endif
    }
    else {
      thisFunctionResult = false;
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("send=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
      #endif
    }
  }
  else {
    thisFunctionResult = false;
    #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
    PROGRAM_LOG("Invalid or NULL network client structure\r\n");
    #endif
  }
  return thisFunctionResult;
}
bool NetworkInterface_Udp_SendTo(Type_NetworkClient* udpClient, void* data, uint16_t length, Type_IP4Address destinationIP, uint16_t destinationPort) {
  bool thisFunctionResult = true;
  if(udpClient) {
    networkInterfaceResult = sendto(udpClient->SocketNumber, data, length, (uint8_t*)&destinationIP, destinationPort);
    if(networkInterfaceResult > SOCK_ERROR) {
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("sendto=<(%d Bytes)>%s\r\n", networkInterfaceResult, "OK");
      #endif
    }
    else {
      thisFunctionResult = false;
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("sendto=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
      #endif
    }
  }
  else {
    thisFunctionResult = false;
    #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
    PROGRAM_LOG("Invalid or NULL network client structure\r\n");
    #endif
  }
  return thisFunctionResult;
}
void NetworkInterface_Udp_Send2(Type_NetworkClient* udpClient, void* data, uint16_t length) {
  uint32_t timeout_us = 500000;
//    uint16_t freesize = 0;
//  freesize = getSn_TxMAX(udpClient->SocketNumber);
//  if (length > freesize) length = freesize; // check size not to exceed MAX size.
//  while(timeout_ms) {
//    freesize = getSn_TX_FSR(udpClient->SocketNumber);
//    if(length <= freesize) break;
//    else {
//      timeout_ms--;
//      NetworkInterface_Delay(1);
//    }
//  }
  wiz_send_data(udpClient->SocketNumber, data, length);
  setSn_CR(udpClient->SocketNumber, Sn_CR_SEND);
  while(getSn_CR(udpClient->SocketNumber)) {}
  while(timeout_us) {
    if(getSn_IR(udpClient->SocketNumber) & Sn_IR_SENDOK) {
       setSn_IR(udpClient->SocketNumber, Sn_IR_SENDOK);
       break;
    }
    timeout_us--;
    MicroDelay2_DelayUs(1);
  }
}
bool NetworkInterface_Udp_Receive(Type_NetworkClient* udpClient, uint8_t* buffer, Type_IP4Address* senderIP, uint16_t* senderPort) {
  if(udpClient) {
    uint16_t receivedDataSize;
    getsockopt(udpClient->SocketNumber, SO_RECVBUF, &receivedDataSize);
    networkInterfaceResult = recvfrom(udpClient->SocketNumber, buffer, receivedDataSize, (uint8_t*)senderIP, senderPort);
    if(networkInterfaceResult > 0) {
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("recvfrom = %d bytes\r\n", networkInterfaceResult);
      #endif
      return true;
    }
    else {
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("recvfrom=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
      #endif
      return false;
    }
  }
  return false;
}
bool NetworkInterface_Udp_StartReceive(Type_NetworkClient* udpClient, uint8_t* receiveBuffer) {
  if(udpClient && receiveBuffer) {
    // Enabling interrupt (pin)
    intr_kind int_mask = wizchip_getinterruptmask();
    int_mask |= (intr_kind)(IK_SOCK_0 << udpClient->SocketNumber);
    wizchip_setinterruptmask(int_mask);
    // Set Filter Param
    udpClient->ApplyReceiveDataFilterBasedOnIpAndPort = false;
    udpClient->ReceiveBuffer = receiveBuffer;
    return true;
  }
  return false;
}
bool NetworkInterface_Udp_StopReceive(Type_NetworkClient* udpClient) {
  if(udpClient) {
    // Disabaling interrupt (pin)
    intr_kind int_mask = wizchip_getinterruptmask();
    int_mask &= ~((intr_kind)(IK_SOCK_0 << udpClient->SocketNumber));
    wizchip_setinterruptmask(int_mask);
    // Set Filter Param
    udpClient->ApplyReceiveDataFilterBasedOnIpAndPort = false;
    return true;
  }
  return false;
}
bool NetworkInterface_Udp_StartReceiveFrom(Type_NetworkClient* udpClient, uint8_t* receiveBuffer, Type_IP4Address senderIP, uint16_t senderPort) {
  if(udpClient) {
    // Enabling interrupt (pin)
    intr_kind int_mask = wizchip_getinterruptmask();
    int_mask |= (intr_kind)(IK_SOCK_0 << udpClient->SocketNumber);
    wizchip_setinterruptmask(int_mask);
    // Set Filter Param
    udpClient->ApplyReceiveDataFilterBasedOnIpAndPort = true;
    Fill_IP4AddressStructFromStruct(udpClient->ReceiveFilterRemoteIP, senderIP);
    udpClient->ReceiveFilterRemotePort = senderPort;
    udpClient->ReceiveBuffer = receiveBuffer;
    return true;
  }
  return false;
}
bool NetworkInterface_Udp_StopReceiveFrom(Type_NetworkClient* udpClient) {
  if(udpClient) {
    // Disabaling interrupt (pin)
    intr_kind int_mask = wizchip_getinterruptmask();
    int_mask &= ~((intr_kind)(IK_SOCK_0 << udpClient->SocketNumber));
    wizchip_setinterruptmask(int_mask);
    // Set Filter Param
    udpClient->ApplyReceiveDataFilterBasedOnIpAndPort = false;
    return true;
  }
  return false;
}
uint16_t NetworkInterface_Udp_ReceiveDataAvailableInBuffer(Type_NetworkClient* udpClient) { /* return > 0 if any data available */
  if(udpClient) {
    if(udpClient->IsDataReceived) {
      return udpClient->ReceivedDataSize;
    }
  }
  return 0;
}
uint8_t* NetworkInterface_Udp_GetReceivedData(Type_NetworkClient* udpClient) {
  if(udpClient) {
    if(udpClient->IsDataReceived) {
      udpClient->IsDataReceived = false;
      return udpClient->ReceiveBuffer;
    }
    return NULL;
  }
  return NULL;
}
bool NetworkInterface_Udp_Close(Type_NetworkClient* udpClient) {
  bool thisFunctionResult = true;
  if(udpClient) {
    networkInterfaceResult = close(udpClient->SocketNumber);
    W5500SocketManager_ReleaseSocket(udpClient->SocketNumber);
    udpClient->LastStatus = NetworkStatus_Closed;
    usingNetworkClients[udpClient->SocketNumber] = NULL;
//    memset(&udpClient, 0, sizeof(Type_NetworkClient));
    #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
    PROGRAM_LOG("close=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
    #endif
  }
  else {
    thisFunctionResult = false;
    #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
    PROGRAM_LOG("Invalid or NULL network client structure\r\n");
    #endif
  }
  return thisFunctionResult;
}
bool NetworkInterface_Tcp_Open(Type_NetworkClient* tcpClient, uint16_t localPort) {
  bool thisFunctionResult;
  uint8_t selectedSocket;
  if(tcpClient) {
    if(W5500SocketManager_ReserveFreeSocket(&selectedSocket)) {
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("socket number = %d\r\n", selectedSocket);
      #endif
      networkInterfaceResult = socket(selectedSocket, Sn_MR_TCP, localPort, WIZNET_SOCKET_IO_BLOCKING_MODE); // Success : The socket number 'sn' passed as parameter
      if(networkInterfaceResult == selectedSocket) {
        tcpClient->SocketNumber = selectedSocket;
        tcpClient->LocalPort = localPort;
        thisFunctionResult = true;
        tcpClient->LastStatus = NetworkStatus_Opened;
        tcpClient->ClientType = ClientType_Tcp;
        tcpClient->IsDataReceived = false;
        tcpClient->ReceivedDataSize = false;
        tcpClient->ApplyReceiveDataFilterBasedOnIpAndPort = false;
        usingNetworkClients[selectedSocket] = tcpClient;
        #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
        PROGRAM_LOG("socket=<(%d)>%s\r\n", 0, "OK");
        #endif
      }
      else {
        thisFunctionResult = false;
        #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
        PROGRAM_LOG("socket=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
        #endif
      }
    }
    else thisFunctionResult = false;
  }
  else {
    thisFunctionResult = false;
    #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
    PROGRAM_LOG("Invalid or NULL network client structure\r\n");
    #endif
  }
  return thisFunctionResult;
}
bool NetworkInterface_Tcp_Connect(Type_NetworkClient* tcpClient, Type_IP4Address destinationIP, uint16_t destinationPort, uint32_t timeout_ms) {
  bool thisFunctionResult;
  if(tcpClient) {
    Fill_IP4AddressStructFromStruct(tcpClient->RemoteIP, destinationIP);
    tcpClient->RemotePort = destinationPort;
    networkInterfaceResult = connect(tcpClient->SocketNumber, (uint8_t*)&tcpClient->RemoteIP, tcpClient->RemotePort);
    if(WIZNET_SOCKET_IO_BLOCKING_MODE == SF_IO_NONBLOCK && networkInterfaceResult == SOCK_BUSY) {
      networkInterfaceResult = SOCK_OK;
    }
    #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
    PROGRAM_LOG("connect=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
    #endif
    uint8_t connectionStatus = 255;
    while(timeout_ms) {
      getsockopt(tcpClient->SocketNumber, SO_STATUS, &connectionStatus);
      if(connectionStatus == SOCK_ESTABLISHED) break;
      NetworkInterface_Delay(1);
      timeout_ms--;
    }
    if(connectionStatus == SOCK_ESTABLISHED) {
      thisFunctionResult = true;
      tcpClient->LastStatus = NetworkStatus_Connected;
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("Connected\r\n");
      #endif
    }
    else {
      thisFunctionResult = false;
      tcpClient->LastStatus = NetworkStatus_Disconnected;
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("Timeout Reached\r\n");
      #endif
    }
  }
  else {
    thisFunctionResult = false;
    #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
    PROGRAM_LOG("Invalid or NULL network client structure\r\n");
    #endif
  }
  return thisFunctionResult;
}
bool NetworkInterface_Tcp_Disconnect(Type_NetworkClient* tcpClient, uint32_t timeout_ms) {
  bool thisFunctionResult;
  if(tcpClient) {
    networkInterfaceResult = disconnect(tcpClient->SocketNumber);
    if(WIZNET_SOCKET_IO_BLOCKING_MODE == SF_IO_NONBLOCK && networkInterfaceResult == SOCK_BUSY) {
      networkInterfaceResult = SOCK_OK;
    }
    #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
    PROGRAM_LOG("disconnect=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
    #endif
    uint8_t connectionStatus = 255;
    while(timeout_ms) {
      getsockopt(tcpClient->SocketNumber, SO_STATUS, &connectionStatus);
      if(connectionStatus != SOCK_ESTABLISHED) break;
      timeout_ms--;
    }
    if(connectionStatus != SOCK_ESTABLISHED) {
      thisFunctionResult = true;
      tcpClient->LastStatus = NetworkStatus_Disconnected;
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("Disconnected\r\n");
      #endif
    }
    else {
      thisFunctionResult = false;
      tcpClient->LastStatus = NetworkStatus_Connected;
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("Timeout Reached\r\n");
      #endif
    }
  }
  else {
    thisFunctionResult = false;
    #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
    PROGRAM_LOG("Invalid or NULL network client structure\r\n");
    #endif
  }
  return thisFunctionResult;
}
bool NetworkInterface_Tcp_Send(Type_NetworkClient* tcpClient, void* data, uint16_t length) {
  bool thisFunctionResult;
  if(tcpClient) {
    networkInterfaceResult = send(tcpClient->SocketNumber, data, length);
    if(networkInterfaceResult > SOCK_ERROR) {
      thisFunctionResult = true;
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("send=<(%d Bytes)>%s\r\n", networkInterfaceResult, "OK");
      #endif
    }
    else {
      thisFunctionResult = false;
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("send=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
      #endif
    }
  }
  else {
    thisFunctionResult = false;
    #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
    PROGRAM_LOG("Invalid or NULL network client structure\r\n");
    #endif
  }
  return thisFunctionResult;
}
void NetworkInterface_Tcp_Send2(Type_NetworkClient* tcpClient, void* data, uint16_t length) {
  uint32_t timeout_us = 500000;
  wiz_send_data(tcpClient->SocketNumber, data, length);
  setSn_CR(tcpClient->SocketNumber, Sn_CR_SEND);
  while(getSn_CR(tcpClient->SocketNumber)) {}
  while(timeout_us) {
    if(getSn_IR(tcpClient->SocketNumber) & Sn_IR_SENDOK) {
       setSn_IR(tcpClient->SocketNumber, Sn_IR_SENDOK);
       break;
    }
    timeout_us--;
    MicroDelay2_DelayUs(1);
  }
}
bool NetworkInterface_Tcp_Receive(Type_NetworkClient* tcpClient, uint8_t* buffer, Type_IP4Address* senderIP, uint16_t* senderPort) {
  if(tcpClient) {
    uint16_t receivedDataSize;
    getsockopt(tcpClient->SocketNumber, SO_RECVBUF, &receivedDataSize);
    networkInterfaceResult = recv(tcpClient->SocketNumber, buffer, receivedDataSize);
    if(networkInterfaceResult > 0) {
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("recv = %d bytes\r\n", networkInterfaceResult);
      #endif
      return true;
    }
    else {
      #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
      PROGRAM_LOG("recv=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
      #endif
      return false;
    }
  }
  return false;
}
bool NetworkInterface_Tcp_StartReceive(Type_NetworkClient* tcpClient, uint8_t* receiveBuffer) {
  if(tcpClient && receiveBuffer) {
    // Enabling interrupt (pin)
    intr_kind int_mask = wizchip_getinterruptmask();
    int_mask |= (intr_kind)(IK_SOCK_0 << tcpClient->SocketNumber);
    wizchip_setinterruptmask(int_mask);
    // Set Filter Param
    tcpClient->ApplyReceiveDataFilterBasedOnIpAndPort = false;
    tcpClient->ReceiveBuffer = receiveBuffer;
    return true;
  }
  return false;
}
bool NetworkInterface_Tcp_StopReceive(Type_NetworkClient* tcpClient) {
  if(tcpClient) {
    // Disabaling interrupt (pin)
    intr_kind int_mask = wizchip_getinterruptmask();
    int_mask &= ~((intr_kind)(IK_SOCK_0 << tcpClient->SocketNumber));
    wizchip_setinterruptmask(int_mask);
    // Set Filter Param
    tcpClient->ApplyReceiveDataFilterBasedOnIpAndPort = false;
    return true;
  }
  return false;
}
uint16_t NetworkInterface_Tcp_ReceiveDataAvailableInBuffer(Type_NetworkClient* tcpClient) { /* return > 0 if any data available */
  if(tcpClient) {
    if(tcpClient->IsDataReceived) {
      return tcpClient->ReceivedDataSize;
    }
  }
  return 0;
}
uint8_t* NetworkInterface_Tcp_GetReceivedData(Type_NetworkClient* tcpClient) {
  if(tcpClient) {
    if(tcpClient->IsDataReceived) {
      tcpClient->IsDataReceived = false;
      return tcpClient->ReceiveBuffer;
    }
    return NULL;
  }
  return NULL;
}
bool NetworkInterface_Tcp_Close(Type_NetworkClient* tcpClient) {
  bool thisFunctionResult = true;
  if(tcpClient) {
    networkInterfaceResult = close(tcpClient->SocketNumber);
    W5500SocketManager_ReleaseSocket(tcpClient->SocketNumber);
    tcpClient->LastStatus = NetworkStatus_Closed;
    usingNetworkClients[tcpClient->SocketNumber] = NULL;
//    memset(&tcpClient, 0, sizeof(Type_NetworkClient));
    #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
    PROGRAM_LOG("close=<(%d)>%s\r\n", networkInterfaceResult, socketErrorString(networkInterfaceResult));
    #endif
  }
  else {
    thisFunctionResult = false;
    #ifdef _NETWORK_INTERFACE_INTERNAL_DEBUG
    PROGRAM_LOG("Invalid or NULL network client structure\r\n");
    #endif
  }
  return thisFunctionResult;
}
void NetworkInterface_ReceiveInterruptCallback(void) {
  intr_kind wizchipInterrupts = wizchip_getinterrupt();  // Read last interrupts status
  Type_NetworkClient* networkClient;
  for(runTimeVariable_SocketCounter2 = 0; runTimeVariable_SocketCounter2 < _WIZCHIP_SOCK_NUM_; runTimeVariable_SocketCounter2++) {
    if(((intr_kind)(IK_SOCK_0 << runTimeVariable_SocketCounter2)) & wizchipInterrupts) {
      if(usingNetworkClients[runTimeVariable_SocketCounter2] && w5500SocketIsBusy[runTimeVariable_SocketCounter2]) {
        networkClient = usingNetworkClients[runTimeVariable_SocketCounter2];
        if(networkClient->ReceiveBuffer) {
          switch(networkClient->ClientType) {
            case ClientType_Udp: {
              if(networkClient->ApplyReceiveDataFilterBasedOnIpAndPort) {
                getsockopt(networkClient->SocketNumber, SO_RECVBUF, &runTimeVariable_ReceivedDataSize);
                networkInterfaceResult = recvfrom(networkClient->SocketNumber, networkClient->ReceiveBuffer, runTimeVariable_ReceivedDataSize, (uint8_t*)&runTimeVariable_SenderIP, &runTimeVariable_SenderPort);
                if(IP4Address_Compare_StructVsStruct(runTimeVariable_SenderIP, networkClient->ReceiveFilterRemoteIP) && (runTimeVariable_SenderPort == networkClient->ReceiveFilterRemotePort)) {
                  Fill_IP4AddressStructFromStruct(networkClient->LastReceivedPacketRemoteIP, runTimeVariable_SenderIP);
                  networkClient->LastReceivedPacketRemotePort = runTimeVariable_SenderPort;
                  networkClient->ReceivedDataSize = runTimeVariable_ReceivedDataSize;
                  networkClient->IsDataReceived = true;
                }
              } // if(networkClient->ApplyReceiveDataFilterBasedOnIpAndPort)
              else {
                getsockopt(networkClient->SocketNumber, SO_RECVBUF, &runTimeVariable_ReceivedDataSize);
                networkInterfaceResult = recvfrom(networkClient->SocketNumber, networkClient->ReceiveBuffer, runTimeVariable_ReceivedDataSize, (uint8_t*)&networkClient->LastReceivedPacketRemoteIP, &networkClient->LastReceivedPacketRemotePort);
                networkClient->ReceivedDataSize = runTimeVariable_ReceivedDataSize;
                networkClient->IsDataReceived = true;
              } // else if(networkClient->ApplyReceiveDataFilterBasedOnIpAndPort)
              break;
            } // case ClientType_Udp
            case ClientType_Tcp: {
              getsockopt(networkClient->SocketNumber, SO_RECVBUF, &runTimeVariable_ReceivedDataSize);
              networkInterfaceResult = recv(networkClient->SocketNumber, networkClient->ReceiveBuffer, runTimeVariable_ReceivedDataSize);
              networkClient->ReceivedDataSize = runTimeVariable_ReceivedDataSize;
              networkClient->IsDataReceived = true;
              break;
            } // case ClientType_Tcp
          } // switch(networkClient->ClientType)
        } // if(networkClient->ReceiveBuffer != NULL)
      } // if(usingNetworkClients[socketCounter] && w5500SocketIsBusy[socketCounter])
    } // if(((intr_kind)(IK_SOCK_0 << socketCounter)) & wizchipInterrupts)
  } // for(uint8_t socketCounter = 0; socketCounter < 8; socketCounter++)
  wizchip_clrinterrupt(wizchipInterrupts); // Clear interrupts
}
#ifdef _NETWORK_INTERFACE_INCLUDE_DHCP
//void NetworkInterface_1Second_Routine(void) {
//  if(ipAssignMode == IPAssignMode_Dhcp) DHCP_time_handler();
//}
void Timer_Dhcp_Callback(void *argument) {
  DHCP_time_handler();
}
#endif
