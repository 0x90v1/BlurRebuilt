# [BlurX] Ghidra script (PyGhidra): decompile the functions that contain each
# address listed in $BLURX_SITES (one hex address per line) and write them, once
# each, to $BLURX_OUT. Used on the Kanzen image to read the archive writers
# (sites that load the 'AR' object tag 0x4152).
#@category BlurX
import os
from ghidra.app.decompiler import DecompInterface

sites = [l.strip() for l in open(os.environ["BLURX_SITES"]) if l.strip()]
space = currentProgram.getAddressFactory().getDefaultAddressSpace()
fm = currentProgram.getFunctionManager()
dec = DecompInterface()
dec.openProgram(currentProgram)
done = set()
with open(os.environ["BLURX_OUT"], "w") as out:
    for s in sites:
        f = fm.getFunctionContaining(space.getAddress(int(s, 16)))
        if f is None:
            out.write("// %s: no function\n\n" % s)
            continue
        key = f.getEntryPoint().getOffset()
        if key in done:
            out.write("// %s: in %s (above)\n\n" % (s, f.getEntryPoint()))
            continue
        done.add(key)
        r = dec.decompileFunction(f, 120, monitor)
        out.write("// site %s: function %s (%s)\n" % (s, f.getEntryPoint(), f.getName()))
        out.write((r.getDecompiledFunction().getC() if r.decompileCompleted() else "// decompile failed\n") + "\n\n")
print("decompiled %d functions" % len(done))
