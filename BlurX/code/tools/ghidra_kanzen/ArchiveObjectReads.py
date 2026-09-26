# [BlurX] Ghidra script (PyGhidra) for the Kanzen image: find the archive's object
# reads. In the Blur 1 source (misc_shared/archive/binaryarchive.h) reading an
# object is: U16 tag ('AR'), U16 version, then T::Deserialise. In the Speed build
# the tag check is compiled out, leaving two reads of 2 bytes through the archive
# byte reader ($BLURX_READER, the function that reads n bytes and byte-swaps) and
# then the call to the deserialiser. This lists every call to the reader with its
# size argument (li r5,N just before the call), finds calls of size 2 that follow
# another of size 2 in the same function, and writes, per such pair, the next
# call after it (the deserialiser candidate) to $BLURX_OUT; with $BLURX_DECOMPILE
# set it also decompiles each distinct candidate into $BLURX_OUT.c.
#@category BlurX
import os
from ghidra.app.decompiler import DecompInterface

space = currentProgram.getAddressFactory().getDefaultAddressSpace()
listing = currentProgram.getListing()
refs = currentProgram.getReferenceManager()
fm = currentProgram.getFunctionManager()
reader = space.getAddress(int(os.environ["BLURX_READER"], 16))


def size_arg(call):
    """the li r5,N (addi r5,r0,N) within the 8 instructions before a call"""
    ins = listing.getInstructionBefore(call)
    for _ in range(8):
        if ins is None:
            return None
        if ins.getMnemonicString() == "li" and str(ins.getOpObjects(0)[0]) == "r5":
            return ins.getOpObjects(1)[0].getValue()
        if ins.getMnemonicString() in ("bl", "bctrl", "blr"):
            return None
        ins = listing.getInstructionBefore(ins.getAddress())
    return None


calls = []
for r in refs.getReferencesTo(reader):
    if r.getReferenceType().isCall():
        calls.append(r.getFromAddress())
calls.sort()
pairs = []
for a, b in zip(calls, calls[1:]):
    if b.subtract(a) > 64:
        continue
    if size_arg(a) == 2 and size_arg(b) == 2:
        nxt = listing.getInstructionAfter(b)
        target = None
        for _ in range(24):
            if nxt is None:
                break
            if nxt.getMnemonicString() == "bl":
                flows = nxt.getFlows()
                if flows and flows[0] != reader:
                    target = flows[0]
                    break
            nxt = listing.getInstructionAfter(nxt.getAddress())
        pairs.append((a, b, target))

candidates = {}
with open(os.environ["BLURX_OUT"], "w") as out:
    out.write("// reader %s: %d calls, %d tag+version pairs\n" % (reader, len(calls), len(pairs)))
    for a, b, t in pairs:
        out.write("%s %s -> %s\n" % (a, b, t))
        if t is not None:
            candidates.setdefault(t.getOffset(), t)

if os.environ.get("BLURX_DECOMPILE"):
    dec = DecompInterface()
    dec.openProgram(currentProgram)
    with open(os.environ["BLURX_OUT"] + ".c", "w") as out:
        for off in sorted(candidates):
            t = candidates[off]
            f = fm.getFunctionAt(t) or fm.getFunctionContaining(t)
            if f is None:
                out.write("// %s: no function\n\n" % t)
                continue
            r = dec.decompileFunction(f, 120, monitor)
            out.write("// ===== %s (%s)\n" % (f.getEntryPoint(), f.getName()))
            out.write((r.getDecompiledFunction().getC() if r.decompileCompleted() else "// decompile failed\n") + "\n")
print("calls %d, pairs %d, candidates %d" % (len(calls), len(pairs), len(candidates)))
