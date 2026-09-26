# [BlurX] Ghidra script (PyGhidra) for the flat Kanzen_*.xex image
# (Blur2Test _tools/decode_xbox_xex_image.py output, imported with the PE loader
# as PowerPC:BE:64:A2ALT-32addr). Ghidra's PE loader does not read the Xbox 360
# exception directory (machine 0x1F2), whose .pdata entries give every function's
# start and length: IMAGE_CE_RUNTIME_FUNCTION_ENTRY = u32 begin address, u32
# (prolog length:8, function length in words:22, 32-bit flag:1, exception flag:1).
# This creates a function at each entry that has none (run before or after
# auto-analysis; reports counts and the first failures).
#@category BlurX
from ghidra.program.model.address import AddressSet
from ghidra.program.model.symbol import SourceType
from ghidra.app.cmd.disassemble import DisassembleCommand
from ghidra.app.plugin.core.analysis import AutoAnalysisManager

# create functions only; auto-analysis reacting to each one made a run take hours
AutoAnalysisManager.getAnalysisManager(currentProgram).setIgnoreChanges(True)

mem = currentProgram.getMemory()
fm = currentProgram.getFunctionManager()
print("functions before: %d" % fm.getFunctionCount())
block = mem.getBlock(".pdata")
if block is None:
    print("no .pdata block")
else:
    space = currentProgram.getAddressFactory().getDefaultAddressSpace()
    start = block.getStart()
    n = int(block.getSize()) // 8
    made = failed = 0
    for i in range(n):
        a = start.add(i * 8)
        begin = mem.getInt(a) & 0xFFFFFFFF
        info = mem.getInt(a.add(4)) & 0xFFFFFFFF
        length = ((info >> 8) & 0x3FFFFF) * 4
        if begin == 0 or length == 0:
            continue
        entry = space.getAddress(begin)
        if fm.getFunctionAt(entry) is not None:
            continue
        try:
            body = AddressSet(entry, entry.add(length - 1))
            DisassembleCommand(body, None, False).applyTo(currentProgram, monitor)
            fm.createFunction(None, entry, body, SourceType.ANALYSIS)
            made += 1
        except Exception as e:
            failed += 1
            if failed <= 5:
                print("fail at %s: %s" % (entry, e))
    print("pdata entries: %d, functions created: %d, failed: %d" % (n, made, failed))
print("functions after: %d" % fm.getFunctionCount())
