# [BlurX] Ghidra script (PyGhidra): decompile every function reachable by direct
# calls from the roots in $BLURX_ROOTS (comma separated hex), following only
# callees whose entry lies in [$BLURX_LO, $BLURX_HI) (hex), breadth first, up to
# $BLURX_MAX functions, into $BLURX_OUT with a callee list per function. Functions
# are created on demand from the covering .pdata entry (see RefsAndCallers.py).
#@category BlurX
import os
import bisect
from ghidra.app.decompiler import DecompInterface
from ghidra.app.cmd.disassemble import DisassembleCommand
from ghidra.program.model.address import AddressSet
from ghidra.program.model.symbol import SourceType

space = currentProgram.getAddressFactory().getDefaultAddressSpace()
fm = currentProgram.getFunctionManager()
mem = currentProgram.getMemory()
listing = currentProgram.getListing()
lo = int(os.environ.get("BLURX_LO", "823a0000"), 16)
hi = int(os.environ.get("BLURX_HI", "823e8000"), 16)
limit = int(os.environ.get("BLURX_MAX", "400"))
dec = DecompInterface()
dec.openProgram(currentProgram)

pdata = []
blk = mem.getBlock(".pdata")
for i in range(int(blk.getSize()) // 8):
    a = blk.getStart().add(i * 8)
    begin = mem.getInt(a) & 0xFFFFFFFF
    length = ((mem.getInt(a.add(4)) & 0xFFFFFFFF) >> 8 & 0x3FFFFF) * 4
    if begin and length:
        pdata.append((begin, length))
pdata.sort()
starts = [p[0] for p in pdata]


def ensure_function(addr):
    f = fm.getFunctionAt(addr) or fm.getFunctionContaining(addr)
    if f is not None:
        return f
    off = addr.getOffset()
    i = bisect.bisect_right(starts, off) - 1
    if i < 0 or off >= pdata[i][0] + pdata[i][1]:
        return None
    entry = space.getAddress(pdata[i][0])
    body = AddressSet(entry, entry.add(pdata[i][1] - 1))
    DisassembleCommand(body, None, True).applyTo(currentProgram, monitor)
    for g in list(fm.getFunctionsOverlapping(body)):
        fm.removeFunction(g.getEntryPoint())
    try:
        fm.createFunction(None, entry, body, SourceType.ANALYSIS)
    except Exception:
        pass
    return fm.getFunctionContaining(addr)


def callees(f):
    out = []
    for ins in listing.getInstructions(f.getBody(), True):
        if ins.getMnemonicString() in ("bl", "b") and ins.getFlows():
            t = ins.getFlows()[0].getOffset()
            if lo <= t < hi:
                out.append(ins.getFlows()[0])
    return out


queue = [space.getAddress(int(r, 16)) for r in os.environ["BLURX_ROOTS"].split(",")]
seen = set()
with open(os.environ["BLURX_OUT"], "w") as out:
    while queue and len(seen) < limit:
        a = queue.pop(0)
        f = ensure_function(a)
        if f is None:
            out.write("// %s: no function\n\n" % a)
            continue
        key = f.getEntryPoint().getOffset()
        if key in seen:
            continue
        seen.add(key)
        cs = callees(f)
        r = dec.decompileFunction(f, 120, monitor)
        out.write("// ===== %s calls: %s\n" % (f.getEntryPoint(), " ".join(sorted(set(str(c) for c in cs)))))
        out.write((r.getDecompiledFunction().getC() if r.decompileCompleted() else "// decompile failed\n") + "\n")
        queue.extend(cs)
print("decompiled %d functions" % len(seen))
