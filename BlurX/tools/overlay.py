#!/usr/bin/env python3
"""overlay.py for BlurX: the Blur1Multi tool (../Blur1Multi/tools/overlay.py,
commands and manifest format documented there) pointed at this phase, whose
bases are Blur1Multi then Blur1PC. A BlurX copy is recorded against the
highest base that holds the file."""
import os
import sys

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), 'Blur1Multi', 'tools'))
import overlay  # noqa: E402

# Blur1Multi as committed (tools/sync_base.py), when the snapshot exists
snapshot = os.path.join(HERE, '_work', 'base', 'Blur1Multi')
overlay.configure(HERE, ['Blur1Multi', 'Blur1PC'],
                  {'Blur1Multi': snapshot} if os.path.isdir(snapshot) else None)
sys.exit(overlay.main(sys.argv))
