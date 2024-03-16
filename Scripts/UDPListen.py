#!/usr/bin/env python3


from socket import *
import sys

host = '255.255.255.255'
port = 5000
addr = (host,port)

udp_socket = socket(AF_INET, SOCK_DGRAM)


#data = input('write to server: ')
#if not data : 
#    udp_socket.close() 
#    sys.exit(1)

#encode - перекодирует введенные данные в байты, decode - обратно
#data = str.encode(data)
#udp_socket.sendto(data, addr)
#data = bytes.decode(data)

udp_socket.bind(addr)
data, addr = udp_socket.recvfrom(1024)
print("Got: ", len(data))

udp_socket.close()

