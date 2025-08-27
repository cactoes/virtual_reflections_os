from scapy.all import Ether, IP, ICMP, sendp, sniff, ARP
from time import sleep

iface = r"\Device\NPF_{467B5C6D-0FFE-4B63-AB44-96C26ECD9D69}"

src_mac = "02:00:00:00:00:01"   # your host/tap MAC
src_ip  = "10.0.2.1"            # IP of your host on that network
target_ip = "10.0.2.15"         # IP of your guest (you want to resolve this!)

# Gratuitous ARP = ARP reply, broadcasted
pkt = Ether(dst="ff:ff:ff:ff:ff:ff", src=src_mac) / \
      ARP(op=2, hwsrc=src_mac, psrc=src_ip,
          hwdst="00:00:00:00:00:00", pdst=src_ip)

sendp(pkt, iface=iface, verbose=1)
sleep(1)

pkt = Ether(dst="52:54:0:12:34:56") / IP(dst=target_ip, src=src_ip) / ICMP(type=8)

print("Sending ICMP Echo Request ...")
while True:
    sendp(pkt, iface=iface, verbose=1)
    sleep(1)
