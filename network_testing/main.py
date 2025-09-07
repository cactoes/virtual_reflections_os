# ai generated packet generator script

from scapy.all import Ether, IP, UDP, sendp, get_if_list
from time import sleep

# Change this to match the Npcap interface name for tap0
iface = r"\Device\NPF_{DA8915B7-4E0D-415D-98BE-8D7CF3757533}"

# Destination = MAC of the guest's e1000 (net1)
pkt = Ether(dst="52:54:00:12:34:56", src="02:00:00:00:00:01") \
      / IP(dst="10.0.2.15", src="10.0.2.1") \
      / UDP(dport=1234, sport=4321) \
      / b"hello e1000 driver"

# pkt = Ether(dst="ff:ff:ff:ff:ff:ff", src="02:00:00:00:00:01") \
#       / b"e1000 test packet"

print(f"Sending on {iface} ...")
while True:
    sendp(pkt, iface=iface, count=1, verbose=1)
    sleep(0.5);
