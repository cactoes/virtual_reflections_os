from scapy.all import Ether, ARP, sendp, sniff
from time import sleep

# Npcap interface for tap0 (check with get_if_list() if unsure)
iface = r"\Device\NPF_{467B5C6D-0FFE-4B63-AB44-96C26ECD9D69}"

# Parameters for ARP request
src_mac = "02:00:00:00:00:01"   # your host/tap MAC
src_ip  = "10.0.2.1"            # IP of your host on that network
target_ip = "10.0.2.15"         # IP of your guest (you want to resolve this!)

# Build ARP request ("who-has target_ip? tell src_ip")
# pkt = Ether(dst="ff:ff:ff:ff:ff:ff", src=src_mac) / \
#       ARP(op=1, hwsrc=src_mac, psrc=src_ip, hwdst="00:00:00:00:00:00", pdst=target_ip)

# sendp(pkt, iface=iface, count=1, inter=1, verbose=1)

# sleep(1)

# # Gratuitous ARP = ARP reply, broadcasted
# pkt = Ether(dst="ff:ff:ff:ff:ff:ff", src=src_mac) / \
#       ARP(op=2, hwsrc=src_mac, psrc=src_ip,
#           hwdst="00:00:00:00:00:00", pdst=src_ip)

# print(f"Broadcasting gratuitous ARP: {src_ip} is at {src_mac}")
# sendp(pkt, iface=iface, count=1, inter=1, verbose=1)

# sleep(1)

# print(f"Sending ARP requests on {iface} for {target_ip} ...")
# # while True:
# sendp(pkt, iface=iface, count=1, verbose=1)
#     # sleep(1)

# def handle_pkt(pkt):
#     if ARP in pkt and pkt[ARP].op == 2:  # ARP reply
#         print(f"[+] Got ARP reply: {pkt[ARP].psrc} is at {pkt[ARP].hwsrc}")

# print("Waiting for ARP replies ...")
# reply = sniff(
#     iface=iface,
#     filter="arp",
#     prn=handle_pkt,
#     count=1,
#     timeout=99999
# )