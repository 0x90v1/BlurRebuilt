# DX11 decisions

## T187 — stream effect constants and memoize legacy samplers (2026-10-07)

Found in / Belongs to Blur1PCDX11. Ordinary FX11 constant buffers use dynamic storage and WRITE_DISCARD, copying the complete effect backing store whenever it is dirty. Texture buffers retain DEFAULT/UpdateSubresource; externally supplied non-updatable buffers retain their existing behavior. This changes upload mechanics while preserving authored values, pass ordering and simulation cadence. GPU readback verifies partial parameter writes, independent cloned effects and 128 queued alternating draws on hardware and WARP.

Legacy sampler slots memoize their last descriptor and native object. An actual FX Apply advances the binding epoch, requiring the next legacy setter to rebind even if the descriptor is unchanged. Descriptor changes and device changes also force lookup/binding. Keep the existing global state-object cache and multithread protection.

Retain the repaired fullscreen blt presentation path. Temporary GPU timestamp/Present instrumentation is removed from the delivered build. These optimizations improve measured performance; they do not establish DX9 parity or complete graphics parity. Measurements and verification limits are in analysis.md.
