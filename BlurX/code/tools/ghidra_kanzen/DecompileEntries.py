# [BlurX] Ghidra script (PyGhidra): decompile functions at known entry points
# (typically .pdata starts found off line), creating each function first with the
# length its .pdata entry declares, so an unanalysed program needs no whole-program
# pass.
#   BLURX_ENTRIES = "entry[:length],..." (hex entry, decimal byte length)
#   BLURX_OUT     = output file
#@category BlurX
import os
from ghidra.app.decompiler import DecompInterface
from ghidra.app.cmd.disassemble import DisassembleCommand
from ghidra.app.cmd.function import CreateFunctionCmd
from ghidra.program.model.address import AddressSet

space = currentProgram.getAddressFactory().getDefaultAddressSpace()
fm = currentProgram.getFunctionManager()
dec = DecompInterface()
dec.openProgram(currentProgram)
with open(os.environ["BLURX_OUT"], "w") as out:
    for item in os.environ["BLURX_ENTRIES"].split(","):
        parts = item.split(":")
        entry = space.getAddress(int(parts[0], 16))
        length = int(parts[1]) if len(parts) > 1 else 0
        f = fm.getFunctionAt(entry)
        if f is None:
            body = AddressSet(entry, entry.add(max(length, 4) - 1)) if length else None
            DisassembleCommand(entry, body, True).applyTo(currentProgram, monitor)
            cmd = CreateFunctionCmd(None, entry, body, None) if body else CreateFunctionCmd(entry)
            cmd.applyTo(currentProgram, monitor)
            f = fm.getFunctionAt(entry)
        if f is None:
            out.write("// %s: could not create a function\n\n" % parts[0])
            continue
        r = dec.decompileFunction(f, 300, monitor)
        out.write("// ===== %s (%s), %d bytes\n" % (f.getEntryPoint(), f.getName(), length))
        out.write((r.getDecompiledFunction().getC() if r.decompileCompleted() else "// decompile failed: %s\n" % r.getErrorMessage()) + "\n\n")
print("decompiled entries")
