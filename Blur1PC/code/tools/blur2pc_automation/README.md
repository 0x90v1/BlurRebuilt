# Automation scripts

Unattended test runs for every phase. The game reads a script named by the environment variable
`BLUR2PC_AUTOMATION`; `<Phase>/_work/run/run_script.ps1 -Script <file>` sets it, starts the game
minimised, waits for it to exit and prints `automation.log`. The script format is documented at the
top of `code/mercury/library/common/system/include/system/debug/Blur2PCAutomation.h`.

Commands: `tap`, `down`, `up` (keys), `shot` (back-buffer PNG), `quit`, `lua <code>` (runs in the
UI Lua state) and `wait <timeout> <Lua expression>`. A `wait` blocks the lines below it until the
expression is true; their times count from that moment. A wait that times out saves `timeout.png`
and exits with code 3, so a broken run fails fast and says where it stopped.

`automation.log` lists every screen change in every UI context, every key, wait, Lua line and
screenshot, with seconds since the window opened.

Lua helpers available to scripts while automation is active:

| Helper | Returns |
|---|---|
| `Blur2PC.RaceState()` | race logic state: `None`, `Loading`, `Intro`, `Starting` (countdown), `Racing`, `Finished`, `Outro`, `Quitting`, `Restarting`, `GoingToNextEvent` |
| `Blur2PC.Log(text)` | writes a line to `automation.log` |
| `Blur2PC.Run(file)` | runs a Lua file (absolute, or relative to the script) |
| `UIScreen.GetCurrentScreen([context])` | the game's own: current screen, lower case, e.g. `intro\startscreen.lua` |

Write Lua strings that contain backslashes as long strings (`[[intro\startscreen.lua]]`): in a
quoted Lua string `\s` is not a backslash.

## Files

- `quickrace.lua`: `Blur2PC_QuickRace(tier, event[, car])` starts a stock career event from the
  start screen with the calls the career menus make (listed in the file), skipping the menus.
  `Blur2PC_QuickRaceReady()` is true once the profile has loaded.
- `quick_race.txt`: boot, start screen, career tier 1 event 1, vista skip, throttle from the green
  light, three driving screenshots, quit.

The full menu run (`race_long.txt` in `Blur1PC/_work/run/scripts`) still exercises the front end;
it is the T061 regression run.
