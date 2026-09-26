# [BlurX] Ghidra script (PyGhidra): print big-endian words from the program's own
# memory map (the raw image file does not map every section one to one).
#   BLURX_WORDS = "addr:count,addr:count" (hex addresses)
# Each word is printed with the function that contains it when it points at code,
# and with its bytes as text, so vtables and string pools read in one pass.
#@category BlurX
import os

space = currentProgram.getAddressFactory().getDefaultAddressSpace()
mem = currentProgram.getMemory()
fm = currentProgram.getFunctionManager()
for item in os.environ["BLURX_WORDS"].split(","):
    a, n = item.split(":")
    base = int(a, 16)
    for i in range(int(n)):
        addr = space.getAddress(base + 4 * i)
        w = mem.getInt(addr) & 0xFFFFFFFF
        f = fm.getFunctionContaining(space.getAddress(w)) if 0x82000000 <= w < 0x84000000 else None
        text = "".join(chr(b) if 32 <= b < 127 else "." for b in [(w >> s) & 0xFF for s in (24, 16, 8, 0)])
        print("WORD %08x: %08x  %s  %s" % (base + 4 * i, w, text, f.getName() + "@" + str(f.getEntryPoint()) if f else ""))
