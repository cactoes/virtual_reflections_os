from scapy.all import Ether, IP, UDP, sendp
import time

target_ip = "10.0.2.15"
# This matches the MAC address in your vrtlkt.conf
target_mac = "52:54:00:12:34:56" 

# Notice the addition of Ether() to build the Layer 2 frame
pkt = Ether(dst=target_mac) / IP(dst=target_ip) / UDP(dport=8080) / b"hello\0"

while True:
    # sendp operates at layer 2. 
    # NOTE: You MUST replace "tap0" with the actual exact 
    # human-readable name of the TAP adapter in your Windows Network Connections.
    sendp(pkt, iface="tap0", verbose=1)
    time.sleep(1)