# QI RPG/Roguelike Branch

This folder preserves the RPG-oriented QI prototype split from the original
turn-based duel core.

Focus:

- Roguelike run progression between battles.
- Three-choice post-battle rewards.
- Skill pool, equipped skill slots, prerequisites, and bypassed insight.
- Artifacts, elixirs, encounters, school tags, and build identity.

Current structure:

- `QI.h` keeps shared enums, public structs, global declarations, and prototypes.
- `QI.c` still owns the executable entry point, combat loop, AI logic, data
  initialization entry, combat loop, AI logic, and legacy compatibility glue.
- `rpg_data.inc` contains skill, artifact, and elixir database initialization.
- `rpg_rewards.inc` contains reward pools, reward presentation, reward
  application, and post-battle reward flow.
- `rpg_encounters.inc` contains between-battle encounter resolution.
- `rpg_ui_json.inc` contains the non-blocking Web UI JSON mode and UI-side run
  state machine.
- `rpg_build.inc` is the first RPG-side split. It contains build profiling,
  school/archetype naming, build summary printing, and auto-reward scoring while
  preserving the existing `gcc QI.c -lm` build command.
- `web_server.py` runs a local standard-library HTTP server and bridges the Web
  UI to `./qi_rpg --ui-json`.
- `ui/` contains the dependency-free browser interface.

RPG roadmap:

- Continue thinning `QI.c` by moving inventory helpers and elixir use flow into
  focused RPG modules while keeping single-file inclusion until a Makefile lands.
- Add richer character growth fields such as cultivation resources, reputation,
  body/spirit stats, and route-specific breakthrough pressure.
- Improve manual run ergonomics: inspect build, compare replacement skills,
  manage artifacts/elixirs, and explain why a reward strengthens a route.
- Preserve `learned_skills` as the compatibility layer for the old combat and AI
  code while `unlocked_skills` plus `equipped_skills` become the RPG source of
  truth.

Build:

```sh
gcc -Wall -Wextra -o qi_rpg QI.c -lm
```

Run from this folder so `config.txt` is loaded:

```sh
./qi_rpg
```

Web UI:

```sh
gcc -Wall -Wextra -o qi_rpg QI.c -lm
python3 web_server.py
```

Then open:

```text
http://localhost:8000
```

The Web path keeps the CLI path intact. It starts the C core as
`./qi_rpg --ui-json`, reads `##UI_JSON##:` snapshots, and exposes local
`/api/*` endpoints for actions, elixir use, reward choice, continue, and reset.

Notes:

- Root `QI.c` / `QI.h` can be restored to the original duel-focused version
  without losing this RPG prototype.
- This branch intentionally does not carry the animation bridge API.
