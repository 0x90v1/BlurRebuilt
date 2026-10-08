# DX11 decisions

## T187 — stream effect constants and memoize legacy samplers (2026-10-07)

Found in / Belongs to Blur1PCDX11. Ordinary FX11 constant buffers use dynamic storage and WRITE_DISCARD, copying the complete effect backing store whenever it is dirty. Texture buffers retain DEFAULT/UpdateSubresource; externally supplied non-updatable buffers retain their existing behavior. This changes upload mechanics while preserving authored values, pass ordering and simulation cadence. GPU readback verifies partial parameter writes, independent cloned effects and 128 queued alternating draws on hardware and WARP.

Legacy sampler slots memoize their last descriptor and native object. An actual FX Apply advances the binding epoch, requiring the next legacy setter to rebind even if the descriptor is unchanged. Descriptor changes and device changes also force lookup/binding. Keep the existing global state-object cache and multithread protection.

Retain the repaired fullscreen blt presentation path. Temporary GPU timestamp/Present instrumentation is removed from the delivered build. These optimizations improve measured performance; they do not establish DX9 parity or complete graphics parity. Measurements and verification limits are in analysis.md.

## T187 — resolve FX bindings once and retain unaffected native bindings (2026-10-07)

Found in / Belongs to Blur1PCDX11. The user authorized this follow-up after the performance assessment. Cache immutable per-pass binding metadata and parameter interfaces, while reading current shared/material/object data on every update. Effect destruction releases the records through the existing NativeFX11ReleaseCache path; rebuilding a function's binding array invalidates its records.

The engine's PS b13 alpha-state bind invalidates only that FX constant-buffer slot. Preserve FX/device buffer restoration and the existing alpha-test values and comparisons. Keep full-stage invalidation for callers that change whole categories of state.

Skip unchanged native vertex/index buffer and input-layout submissions, following DX9's existing binding checks. Compare raw buffers as well as stride/offset/format. Input layouts retain the existing immutable declaration/shader pair key. Device state invalidation and the raw postprocess diagnostic draw invalidate these memos; failed layout creation remains retryable. Preserve multithread protection, constant upload policy and the accepted fullscreen presentation path.
