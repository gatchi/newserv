DWORD WSResolveAddress(char* address);
DWORD WSGetLocalAddressList(in_addr* local_addr);
bool WSAddLocalAddress(in_addr* local_addr,in_addr* add);
DWORD WSGetConnectedAddress(int s);
DWORD WSDetermineBestConnectAddress(in_addr* local_addr,DWORD daddr);
bool WSIsLocalAddressType(DWORD daddr);

