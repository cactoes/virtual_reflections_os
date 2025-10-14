from scapy.all import *
from time import sleep

# ====== CONFIG ======
# iface = r"\Device\NPF_{467B5C6D-0FFE-4B63-AB44-96C26ECD9D69}"
iface = r"\Device\NPF_{DA8915B7-4E0D-415D-98BE-8D7CF3757533}"
src_mac = "02:00:00:00:00:01"
src_ip  = "10.0.2.15"
src_port = 4321
dst_ip = "10.0.2.2"
dst_port = 1234
conf.verb = 0
# ====================

def arp_resolve(ip, iface, timeout=2):
    # send an ARP who-has and wait for a reply (srp1 works L2)
    arp_req = Ether(dst="ff:ff:ff:ff:ff:ff", src=src_mac) / ARP(op=1, hwsrc=src_mac, psrc=src_ip, pdst=ip)
    reply = srp1(arp_req, iface=iface, timeout=timeout)
    if reply and ARP in reply:
        return reply[ARP].hwsrc
    return None

def do_handshake(dst_mac, iface, timeout=3):
    # craft L2 SYN
    eth = Ether(dst=dst_mac, src=src_mac)
    ip = IP(dst=dst_ip, src=src_ip)
    syn = TCP(sport=src_port, dport=dst_port, flags="S", seq=1000)
    syn_frame = eth/ip/syn

    # send SYN and wait for SYN-ACK (srp1 at L2)
    synack_frame = srp1(syn_frame, iface=iface, timeout=timeout)
    if synack_frame is None or not synack_frame.haslayer(TCP):
        raise RuntimeError("No SYN-ACK received")

    tcp_layer = synack_frame[TCP]
    if not (tcp_layer.flags & 0x12):  # SYN+ACK bits
        raise RuntimeError("SYN-ACK not seen")

    # craft ACK to complete handshake
    my_seq = syn.seq + 1
    their_seq = tcp_layer.seq
    ack = TCP(sport=src_port, dport=dst_port, flags="A", seq=my_seq, ack=their_seq + 1)
    ack_frame = eth/ip/ack
    sendp(ack_frame, iface=iface)

    state = {
        "eth": eth,
        "ip": ip,
        "sport": src_port,
        "dport": dst_port,
        "seq": my_seq,
        "ack": their_seq + 1,
        "iface": iface
    }
    return state

def send_data(state, data):
    tcp = TCP(sport=state['sport'], dport=state['dport'],
              flags="PA", seq=state['seq'], ack=state['ack'])
    frame = state['eth']/state['ip']/tcp/data
    sendp(frame, iface=state['iface'])
    state['seq'] += len(data)

def sniff_responses(state, timeout=2):
    # filter to traffic between our host and dst_ip/dst_port
    bpf = f"tcp and host {dst_ip} and port {state['dport']}"
    pkts = sniff(filter=bpf, iface=state['iface'], timeout=timeout)
    for p in pkts:
        if TCP in p and len(bytes(p[TCP].payload)) > 0:
            payload = bytes(p[TCP].payload)
            # update ack to remote seq + len(payload)
            state['ack'] = p[TCP].seq + len(payload)
            # reply with ACK for received data
            ack = TCP(sport=state['sport'], dport=state['dport'],
                      flags="A", seq=state['seq'], ack=state['ack'])
            sendp(state['eth']/state['ip']/ack, iface=state['iface'])
            print("Received:", payload)

# ---- main flow ----

# pkt = Ether(dst="ff:ff:ff:ff:ff:ff", src=src_mac) / \
#       ARP(op=2, hwsrc=src_mac, psrc=src_ip,
#           hwdst="00:00:00:00:00:00", pdst=src_ip)

# sendp(pkt, iface=iface, verbose=1)
# sleep(1)

print("Resolving MAC for", dst_ip)
dst_mac = arp_resolve(dst_ip, iface)
if dst_mac is None:
    print("ARP failed; guest did not reply. Check guest networking/firewall.")
else:
    print("Resolved MAC:", dst_mac)
    state = do_handshake(dst_mac, iface)
    print("Handshake done. seq=", state['seq'], "ack=", state['ack'])

    send_data(state, b"hello from scapy\r\n")
    sniff_responses(state, timeout=4)
