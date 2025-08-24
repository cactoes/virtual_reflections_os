from scapy.all import Ether, ARP, sendp
from time import sleep

# Npcap interface for tap0 (check with get_if_list() if unsure)
iface = r"\Device\NPF_{DA8915B7-4E0D-415D-98BE-8D7CF3757533}"

# Parameters for ARP request
src_mac = "02:00:00:00:00:01"   # your host/tap MAC
src_ip  = "10.0.2.1"            # IP of your host on that network
target_ip = "10.0.2.15"         # IP of your guest (you want to resolve this!)

# Build ARP request ("who-has target_ip? tell src_ip")
pkt = Ether(dst="ff:ff:ff:ff:ff:ff", src=src_mac) / \
      ARP(op=1, hwsrc=src_mac, psrc=src_ip, hwdst="00:00:00:00:00:00", pdst=target_ip)

print(f"Sending ARP requests on {iface} for {target_ip} ...")
# while True:
sendp(pkt, iface=iface, count=1, verbose=1)
    # sleep(1)
