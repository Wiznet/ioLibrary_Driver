#ifndef __NETWORK_INTERFACE_H
#define __NETWORK_INTERFACE_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "cmsis_os.h"
#include "timers.h"

#include "main.h"

#include "stm32f4xx_my_spi.h"

#include "MicroDelay.h"

#include "debug.h"

#include "socket.h"
#include "dhcp.h"
#include "dns.h"

#define _NETWORK_INTERFACE_USE_FREERTOS
//#define _NETWORK_INTERFACE_INTERNAL_DEBUG
//#define _NETWORK_INTERFACE_INCLUDE_DHCP
//#define _NETWORK_INTERFACE_USE_HAL_SPI_DRIVER

#ifdef _NETWORK_INTERFACE_USE_FREERTOS
  #include "cmsis_os.h"
#endif

#define APPLICATION_DHCP_SOCKET 7
#define APPLICATION_DNS_SOCKET 6
#define APPLICATION_HTTP_SOCKET 5
#define APPLICATION_DATA_SOCKET 0

//                                        PMODE   2 1 0
#define W5500_10BT_HalfDuplex                  // 0 0 0
#define W5500_10BT_FullDuplex                  // 0 0 1
#define W5500_100BT_HalfDuplex                 // 0 1 0
#define W5500_100BT_FullDuplex                 // 0 1 1
#define W5500_100BT_HalfDuplex_AutoNegotiation // 1 0 0
#define W5500_NotUsed1                         // 1 0 1
#define W5500_NotUsed2                         // 1 1 0
#define W5500_AllCapable_AutoNegotiation       // 1 1 1

#define DEFAULT_SPI_NETWORK_INTERFACE SPI2
#define DEFAULT_HSPI_NETWORK_INTERFACE hspi2
#define DEFAULT_NETWORK_INTERFACE_RESET_GPIO_Port W5500_RESET_GPIO_Port
#define DEFAULT_NETWORK_INTERFACE_RESET_Pin W5500_RESET_Pin
#define DEFAULT_NETWORK_INTERFACE_CS_GPIO_Port W5500_CS_GPIO_Port
#define DEFAULT_NETWORK_INTERFACE_CS_Pin W5500_CS_Pin

extern SPI_HandleTypeDef hspi2;

extern osTimerId_t Timer_DhcpHandle;

//Exported Macros -------------------------------------------------------------------------------
#define Fill_IP4AddressFromArray(_IP4AddressStruct,_IP4AddressArray) \
  _IP4AddressStruct.Part1 = _IP4AddressArray[0]; \
  _IP4AddressStruct.Part2 = _IP4AddressArray[1]; \
  _IP4AddressStruct.Part3 = _IP4AddressArray[2]; \
  _IP4AddressStruct.Part4 = _IP4AddressArray[3]

#define Fill_IP4AddressStruct(_IP4AddressStruct,_ip1,_ip2,_ip3,_ip4) \
  _IP4AddressStruct.Part1 = _ip1; \
  _IP4AddressStruct.Part2 = _ip2; \
  _IP4AddressStruct.Part3 = _ip3; \
  _IP4AddressStruct.Part4 = _ip4

#define Fill_IP4AddressStructFromStruct(_IP4AddressStruct_Dst,_IP4AddressStruct_Src) \
  _IP4AddressStruct_Dst.Part1 = _IP4AddressStruct_Src.Part1; \
  _IP4AddressStruct_Dst.Part2 = _IP4AddressStruct_Src.Part2; \
  _IP4AddressStruct_Dst.Part3 = _IP4AddressStruct_Src.Part3; \
  _IP4AddressStruct_Dst.Part4 = _IP4AddressStruct_Src.Part4

#define FillInit_IP4Address(ip1,ip2,ip3,ip4) { ip1,ip2,ip3,ip4 }

#define Fill_IPAddressArrayFromStruct(_IP4AddressArray,_IP4AddressStruct) \
  _IP4AddressArray[0] = _IP4AddressStruct.Part1; \
  _IP4AddressArray[1] = _IP4AddressStruct.Part2; \
  _IP4AddressArray[2] = _IP4AddressStruct.Part3; \
  _IP4AddressArray[3] = _IP4AddressStruct.Part4

#define Fill_IPAddressArray(_IP4AddressArray,_ip1,_ip2,_ip3,_ip4) \
  _IP4AddressArray[0] = _ip1; \
  _IP4AddressArray[1] = _ip2; \
  _IP4AddressArray[2] = _ip3; \
  _IP4AddressArray[3] = _ip4

#define Fill_IPAddress_Uint8ToUint32(ip8,ip32) \
  ip32 = ((uint32_t)ip8[0]) & 0x000000FF; \
  ip32 = (ip32 << 8) + ((uint32_t)ip8[1] & 0x000000FF); \
  ip32 = (ip32 << 8) + ((uint32_t)ip8[2] & 0x000000FF); \
  ip32 = (ip32 << 8) + ((uint32_t)ip8[3] & 0x000000FF)

#define IP4Address_Compare_StructVsStruct(_IP4AddressStruct_1,_IP4AddressStruct_2) ((_IP4AddressStruct_1.Part1 == _IP4AddressStruct_2.Part1) ? (((_IP4AddressStruct_1.Part2 == _IP4AddressStruct_2.Part2) ? (((_IP4AddressStruct_1.Part3 == _IP4AddressStruct_2.Part3) ? (((_IP4AddressStruct_1.Part4 == _IP4AddressStruct_2.Part4) ? (1) : (0))) : (0))) : (0))) : (0))

#define Fill_MAC(_mac,_mac_p1,_mac_p2,_mac_p3,_mac_p4,_mac_p5,_mac_p6) \
  _mac[0] = _mac_p1; \
  _mac[1] = _mac_p2; \
  _mac[2] = _mac_p3; \
  _mac[3] = _mac_p4; \
  _mac[4] = _mac_p5; \
  _mac[5] = _mac_p6

// Data Types -----------------------------------------------------------------------------------
typedef enum Enum_IPAssignMode {
  IPAssignMode_Static,
  IPAssignMode_Dhcp
} Type_IPAssignMode;

typedef enum Enum_NetworkStatus {
  NetworkStatus_Closed,
  NetworkStatus_Opened,
  NetworkStatus_Connected,
  NetworkStatus_Disconnected
} Type_NetworkStatus;

typedef enum Enum_ClientType {
  ClientType_Udp,
  ClientType_Tcp
} Type_ClientType;

typedef struct Struct_IP4Address {
  uint8_t Part1;
  uint8_t Part2;
  uint8_t Part3;
  uint8_t Part4;
} Type_IP4Address;

typedef struct Struct_NetworkProperties {
  Type_IP4Address IPAddress;
  Type_IP4Address SubnetMask;
  Type_IP4Address DefaultGateway;
  Type_IP4Address DnsServer;
} Type_NetworkProperties;

typedef struct Struct_UdpClient {
  Type_ClientType ClientType;
  uint8_t SocketNumber;
  Type_IP4Address RemoteIP;
  uint16_t RemotePort;
  uint16_t LocalPort;
  Type_NetworkStatus LastStatus;
  // /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
  uint8_t * ReceiveBuffer;
  uint16_t ReceivedDataSize;
  bool IsDataReceived; /* Act as Flag */
  // /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
  Type_IP4Address LastReceivedPacketRemoteIP;
  uint16_t LastReceivedPacketRemotePort;
  // /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
  bool ApplyReceiveDataFilterBasedOnIpAndPort;
  Type_IP4Address ReceiveFilterRemoteIP;
  uint16_t ReceiveFilterRemotePort;
} Type_UdpClient;

typedef struct Struct_TcpClient {
  Type_ClientType ClientType;
  uint8_t SocketNumber;
  Type_IP4Address RemoteIP;
  uint16_t RemotePort;
  uint16_t LocalPort;
  Type_NetworkStatus LastStatus;
  // /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
  uint8_t * ReceiveBuffer;
  uint16_t ReceivedDataSize;
  bool IsDataReceived; /* Act as Flag */
  // /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
  Type_IP4Address LastReceivedPacketRemoteIP;
  uint16_t LastReceivedPacketRemotePort;
  // /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
  bool ApplyReceiveDataFilterBasedOnIpAndPort;
  Type_IP4Address ReceiveFilterRemoteIP;
  uint16_t ReceiveFilterRemotePort;
} Type_TcpClient;

typedef struct Struct_NetworkClient {
  Type_ClientType ClientType;
  uint8_t SocketNumber;
  Type_IP4Address RemoteIP;
  uint16_t RemotePort;
  uint16_t LocalPort;
  Type_NetworkStatus LastStatus;
  // /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
  uint8_t * ReceiveBuffer;
  uint16_t ReceivedDataSize;
  bool IsDataReceived; /* Act as Flag */
  // /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
  Type_IP4Address LastReceivedPacketRemoteIP;
  uint16_t LastReceivedPacketRemotePort;
  // /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
  bool ApplyReceiveDataFilterBasedOnIpAndPort;
  Type_IP4Address ReceiveFilterRemoteIP;
  uint16_t ReceiveFilterRemotePort;
} Type_NetworkClient;
// Exported Functions ---------------------------------------------------------------------------
bool NetworkInterface_Init(Type_IPAssignMode ipAssignMode, uint8_t* mac, Type_NetworkProperties* networkProperties);
bool TransmitDataIndependentUdpTest(Type_IP4Address ipA4Address);
bool TransmitDataIndependentTcpTest(Type_IP4Address ipA4Address);

bool NetworkInterface_Udp_Open(Type_NetworkClient* udpClient, uint16_t localPort);
bool NetworkInterface_Udp_Connect(Type_NetworkClient* udpClient, Type_IP4Address destinationIP, uint16_t destinationPort);
bool NetworkInterface_Udp_Send(Type_NetworkClient* udpClient, void* data, uint16_t length); // Normal Send
bool NetworkInterface_Udp_SendTo(Type_NetworkClient* udpClient, void* data, uint16_t length, Type_IP4Address destinationIP, uint16_t destinationPort);
void NetworkInterface_Udp_Send2(Type_NetworkClient* udpClient, void* data, uint16_t length); // Fast implementation
bool NetworkInterface_Udp_Receive(Type_NetworkClient* udpClient, uint8_t* buffer, Type_IP4Address* senderIP, uint16_t* senderPort); // Normal Receive
bool NetworkInterface_Udp_StartReceive(Type_NetworkClient* udpClient, uint8_t* receiveBuffer); // Start interrupt receiving in background
bool NetworkInterface_Udp_StopReceive(Type_NetworkClient* udpClient); // Stop interrupt receiving in background
bool NetworkInterface_Udp_StartReceiveFrom(Type_NetworkClient* udpClient, uint8_t* receiveBuffer, Type_IP4Address senderIP, uint16_t senderPort); // Start interrupt receiving from specific Ip and Port in background
bool NetworkInterface_Udp_StopReceiveFrom(Type_NetworkClient* udpClient); // Stop interrupt receiving from specific Ip and Port in background
uint16_t NetworkInterface_Udp_ReceiveDataAvailableInBuffer(Type_NetworkClient* udpClient); /* return > 0 if any data available */
uint8_t* NetworkInterface_Udp_GetReceivedData(Type_NetworkClient* udpClient);
bool NetworkInterface_Udp_Close(Type_NetworkClient* udpClient);

bool NetworkInterface_Tcp_Open(Type_NetworkClient* tcpClient, uint16_t localPort);
bool NetworkInterface_Tcp_Connect(Type_NetworkClient* tcpClient, Type_IP4Address destinationIP, uint16_t destinationPort, uint32_t timeout_ms);
bool NetworkInterface_Tcp_Disconnect(Type_NetworkClient* tcpClient, uint32_t timeout_ms);
bool NetworkInterface_Tcp_Send(Type_NetworkClient* tcpClient, void* data, uint16_t length); // Normal Send
void NetworkInterface_Tcp_Send2(Type_NetworkClient* tcpClient, void* data, uint16_t length); // Fast Implementation
bool NetworkInterface_Tcp_Receive(Type_NetworkClient* tcpClient, uint8_t* buffer, Type_IP4Address* senderIP, uint16_t* senderPort); // Normal Receive
bool NetworkInterface_Tcp_StartReceive(Type_NetworkClient* tcpClient, uint8_t* receiveBuffer); // Start interrupt receiving in background
bool NetworkInterface_Tcp_StopReceive(Type_NetworkClient* tcpClient); // Stop interrupt receiving in background
uint16_t NetworkInterface_Tcp_ReceiveDataAvailableInBuffer(Type_NetworkClient* tcpClient); /* return > 0 if any data available */
uint8_t* NetworkInterface_Tcp_GetReceivedData(Type_NetworkClient* tcpClient);
bool NetworkInterface_Tcp_Close(Type_NetworkClient* tcpClient);

void NetworkInterface_ReceiveInterruptCallback(void);

#ifdef _NETWORK_INTERFACE_INCLUDE_DHCP
void NetworkInterface_1Second_Routine(void);
#endif
#endif /* __NETWORK_INTERFACE_H */
