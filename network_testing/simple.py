from scapy.all import IP, UDP, send
import time

target = "10.0.2.15"

pkt = IP(dst=target) / UDP(dport=8080) / b"hello\0"

while True:
    send(pkt, verbose=1)
    time.sleep(1)