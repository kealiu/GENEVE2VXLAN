# Geneve Thread

read geneve packets, store the original header, and then convert to vxlan packets, send it to vxlan target.

in this demo code, target is set in `main.c`, as `127.0.0.1` now.

# Vxlan Thread

read vxlan packets, and found out original geneve header, restore it, and send back to GWLB.