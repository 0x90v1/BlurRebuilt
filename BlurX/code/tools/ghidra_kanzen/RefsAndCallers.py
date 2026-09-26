# [BlurX] Ghidra script (PyGhidra): for each address in $BLURX_ADDRS (comma
# separated hex; data such as a string, or code), find the instructions that
# reference it (Ghidra references, plus lis/addi pairs that build the address),
# then decompile the functions holding them and their callers up to $BLURX_DEPTH
# levels (default 2) into $BLURX_OUT. A code address with no function gets the
# function its .pdata entry declares (IMAGE_CE_RUNTIME_FUNCTION_ENTRY: start,
# length in words at bits 8..29), created on demand, so the image needs no
# whole-program function pass. Callers are found from branch references to the
# entry, and get their functions the same way.
#@category BlurX
import os
from ghidra.app.decompiler import DecompInterface
from ghidra.app.cmd.disassemble import DisassembleCommand
from ghidra.program.model.address import AddressSet
from ghidra.program.model.symbol import SourceType

space = currentProgram.getAddressFactory().getDefaultAddressSpace()
fm = currentProgram.getFunctionManager()
refs = currentProgram.getReferenceManager()
mem = currentProgram.getMemory()
depth = int(os.environ.get("BLURX_DEPTH", "2"))
dec = DecompInterface()
dec.openProgram(currentProgram)

# .pdata: sorted (start, length) pairs
pdata = []
blk = mem.getBlock(".pdata")
for i in range(int(blk.getSize()) // 8):
    a = blk.getStart().add(i * 8)
    begin = mem.getInt(a) & 0xFFFFFFFF
    length = ((mem.getInt(a.add(4)) & 0xFFFFFFFF) >> 8 & 0x3FFFFF) * 4
    if begin and length:
        pdata.append((begin, length))
pdata.sort()
import bisect
starts = [p[0] for p in pdata]


def ensure_function(addr):
    f = fm.getFunctionContaining(addr)
    if f is not None:
        return f
    off = addr.getOffset()
    i = bisect.bisect_right(starts, off) - 1
    if i < 0 or off >= pdata[i][0] + pdata[i][1]:
        return None
    entry = space.getAddress(pdata[i][0])
    body = AddressSet(entry, entry.add(pdata[i][1] - 1))
    DisassembleCommand(body, None, True).applyTo(currentProgram, monitor)
    # auto-analysis sometimes made a function over part of this range (a tail split
    # off at a branch); .pdata is the compiler's own statement of the bounds
    for g in list(fm.getFunctionsOverlapping(body)):
        fm.removeFunction(g.getEntryPoint())
    try:
        fm.createFunction(None, entry, body, SourceType.ANALYSIS)
    except Exception as e:
        print("cannot create %s: %s" % (entry, e))
    return fm.getFunctionContaining(addr)


def lis_addi_sites(target):
    """instructions 'addi rX,rY,lo' after 'lis rY,hi' that form target"""
    hi = ((target + 0x8000) >> 16) & 0xFFFF
    lo = target & 0xFFFF
    out = []
    for block in mem.getBlocks():
        if not block.isExecute():
            continue
        a, end, prev = block.getStart(), block.getEnd(), {}
        while a.compareTo(end) < 0:
            w = mem.getInt(a) & 0xFFFFFFFF
            op = w >> 26
            if op == 15 and (w & 0xFFFF) == hi and ((w >> 16) & 0x1F) == 0:
                prev[(w >> 21) & 0x1F] = a
            elif op == 14 and (w & 0xFFFF) == lo and ((w >> 16) & 0x1F) in prev:
                out.append(a)
            a = a.add(4)
    return out


def callers(f):
    out = []
    for r in refs.getReferencesTo(f.getEntryPoint()):
        if r.getReferenceType().isCall() or r.getReferenceType().isJump():
            g = ensure_function(r.getFromAddress())
            if g is not None:
                out.append(g)
    return out


seen = set()
with open(os.environ["BLURX_OUT"], "w") as out:
    frontier = []
    for s in os.environ["BLURX_ADDRS"].split(","):
        target = int(s, 16)
        taddr = space.getAddress(target)
        if mem.getBlock(taddr) is not None and mem.getBlock(taddr).isExecute():
            sites = [taddr]
        else:
            sites = [r.getFromAddress() for r in refs.getReferencesTo(taddr)] + lis_addi_sites(target)
        out.write("// %08x: sites %s\n" % (target, ", ".join(str(x) for x in sites)))
        for site in sites:
            f = ensure_function(site)
            if f is None:
                out.write("//   %s: no .pdata function covers it\n" % site)
            else:
                frontier.append((f, 0, "holds %s" % site))
    while frontier:
        f, level, why = frontier.pop(0)
        key = f.getEntryPoint().getOffset()
        if key in seen:
            continue
        seen.add(key)
        r = dec.decompileFunction(f, 120, monitor)
        out.write("\n// ===== %s (%s), level %d: %s\n" % (f.getEntryPoint(), f.getName(), level, why))
        out.write(r.getDecompiledFunction().getC() if r.decompileCompleted() else "// decompile failed\n")
        if level < depth:
            for g in callers(f):
                frontier.append((g, level + 1, "calls %s" % f.getEntryPoint()))
print("decompiled %d functions" % len(seen))
