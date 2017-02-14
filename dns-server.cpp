#include <windows.h>
#include <winsock2.h>
#include <stdio.h>

#include "network-addresses.h"
#include "dns-server.h"
#include "netconfig.h"

extern CFGFile* config;
extern in_addr local_addresses[];
extern in_addr external_connect;
extern in_addr local_connect;

HANDLE dnsthread = NULL;
int dns_socket;

void DNSBuildResponse(char* destbuffer,char* srcbuffer,in_addr address)
{
    DWORD offset,str_len;
    str_len = strlen(&srcbuffer[0x0C]) + 1;
    offset = str_len + 0x0C;
    char* addressbuffer = (char*)(&address);
    destbuffer[0x00] = srcbuffer[0];
    destbuffer[0x01] = srcbuffer[1];
    destbuffer[0x02] = 0x81; // 0x85;
    destbuffer[0x03] = 0x80;
    destbuffer[0x04] = 0x00;
    destbuffer[0x05] = 0x01;
    destbuffer[0x06] = 0x00;
    destbuffer[0x07] = 0x01;
    destbuffer[0x08] = 0x00;
    destbuffer[0x09] = 0x00;
    destbuffer[0x0A] = 0x00;
    destbuffer[0x0B] = 0x00;
    memcpy(&destbuffer[0x0C],&srcbuffer[0x0C],str_len);
    destbuffer[offset + 0x00] = 0x00;
    destbuffer[offset + 0x01] = 0x01;
    destbuffer[offset + 0x02] = 0x00;
    destbuffer[offset + 0x03] = 0x01;
    destbuffer[offset + 0x04] = 0xC0;
    destbuffer[offset + 0x05] = 0x0C;
    destbuffer[offset + 0x06] = 0x00;
    destbuffer[offset + 0x07] = 0x01;
    destbuffer[offset + 0x08] = 0x00;
    destbuffer[offset + 0x09] = 0x01;
    destbuffer[offset + 0x0A] = 0x00;
    destbuffer[offset + 0x0B] = 0x00;
    destbuffer[offset + 0x0C] = 0x00;
    destbuffer[offset + 0x0D] = 0x3C;
    destbuffer[offset + 0x0E] = 0x00;
    destbuffer[offset + 0x0F] = 0x04;
    destbuffer[offset + 0x10] = addressbuffer[0];
    destbuffer[offset + 0x11] = addressbuffer[1];
    destbuffer[offset + 0x12] = addressbuffer[2];
    destbuffer[offset + 0x13] = addressbuffer[3];
}

bool DNSServerThread(void*)
{
    in_addr local;
    sockaddr_in dns_remote;

    int dns_addrsize;
    int dns_bytesrecv;
    char dns_buffer[1024],dns_dataToSend[1024];
    while (dns_socket != SOCKET_ERROR)
    {
        dns_addrsize = sizeof(struct sockaddr_in);
        dns_bytesrecv = SOCKET_ERROR;
        memset(&dns_remote,0,dns_addrsize);
        dns_bytesrecv = recvfrom(dns_socket,(char*)dns_buffer,1024,0,(SOCKADDR*)&dns_remote,&dns_addrsize);
        if (dns_bytesrecv > 0)
        {
            if (WSIsLocalAddressType(dns_remote.sin_addr.s_addr))
            {
                local.s_addr = local_connect.s_addr;
                if (!local.s_addr) local.s_addr = WSDetermineBestConnectAddress(local_addresses,dns_remote.sin_addr.s_addr);
            } else local.s_addr = external_connect.s_addr;
            DNSBuildResponse(dns_dataToSend,dns_buffer,local);
            dns_bytesrecv = sendto(dns_socket,(char*)dns_dataToSend,0x3C,0,(SOCKADDR*)&dns_remote,dns_addrsize);
            ConsolePrintColor("> > DNS server: %s referred to ",inet_ntoa(dns_remote.sin_addr));
            ConsolePrintColor("%s\n",inet_ntoa(local));
        }
    }
    if (dns_socket != SOCKET_ERROR) closesocket(dns_socket);
    return true;
}

bool DNSStartServer()
{
    int dns_errors;
    dns_socket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (dns_socket == (signed)INVALID_SOCKET) return false;

    BYTE yes = true;
    dns_errors = setsockopt(dns_socket,SOL_SOCKET,SO_REUSEADDR,(char*)(&yes),1);
    if (dns_errors != NO_ERROR)
    {
        closesocket(dns_socket);
        return false;
    }

    sockaddr_in dns_service;
    dns_service.sin_family = AF_INET;
    dns_service.sin_addr.s_addr = INADDR_ANY;
    dns_service.sin_port = htons(53);

    dns_errors = bind(dns_socket,(SOCKADDR*)&dns_service,sizeof(dns_service));
    if (dns_errors != NO_ERROR)
    {
        closesocket(dns_socket);
        return false;
    }

    dnsthread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)DNSServerThread,NULL,0,NULL);
    if (!dnsthread)
    {
        closesocket(dns_socket);
        return false;
    }
    return true;
}

void DNSStopServer()
{
    closesocket(dns_socket);
    dns_socket = SOCKET_ERROR;
    if (WaitForSingleObject(dnsthread,100) == WAIT_TIMEOUT) TerminateThread(dnsthread,0);
    CloseHandle(dnsthread);
    dnsthread = NULL;
}

