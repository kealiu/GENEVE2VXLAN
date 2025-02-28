# GENEVE2VXLAN

Demo C code for converting/bridging from AWS GWLB to VXLAN target

all code could be compiled by `make`. and it is ARM/x86 compatible. (for AWS arm CPU)

## Source Directories

### `vxlan`

include the demo code, which transfer GWLB GENEVE packets to VXLAN packets, and receive VXLAN packets then send back to GWLB.

### `demo`

just a demo to show the contents of GWLB GENEVE. By running it on GWLB target, you could found out that:

1. AWS GWLB include 3 option header
- header1: 8 bytes, it is the `vpce` id
- header2: 8 bytes, all 0, may reserved for further usage.
- header3: 4 bytes, not sure, maybe packet id.
