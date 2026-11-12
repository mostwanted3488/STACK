# STACK

VST3 host plugin: 20 slots inside a single FL Studio mixer slot.

- Loads VST3 plugins in series (FL Studio native plugins are not supported)
- Per-slot bypass, opens each plugin's own window
- Cumulative latency reported to the host, global dry/wet is latency-compensated
- Saves the state of every hosted plugin inside the project

## Build

The GitHub Actions workflow builds the Windows VST3 on every push to `main` and
publishes it as the `latest` release. Copy `STACK.vst3` to
`C:\Program Files\Common Files\VST3`.

Local build: `cmake -B build -A x64 && cmake --build build --config Release`
