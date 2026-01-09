# RADStar (rstar) — RADS Plugin Manager

> “pip for RADS”: fetch, install, and run community plugins from a Git-hosted index.

## Purpose
- Single-command install/run of plugins: `rstar install <name>`, `rstar run <name> [args]`.
- Fetch metadata/payloads from a Git-hosted index (your website / Git repo as source of truth).
- Keep RADS ergonomics: minimal manifest, drop-in execution with `rads <entry>`.

## CLI (proposed)
- `rstar install <name>`: resolve in index, download (git/zip), place in plugin store, record in `package.rads`.
- `rstar list`: show installed plugins and versions.
- `rstar run <name> [-- args]`: execute plugin entry using `rads <entry>` inside the plugin directory.
- (later) `update <name>`, `search <query>`.

## Plugin manifest (`plugin.radsmeta`)
INI/TOML-like, checked into the plugin repo:
```
name = "ai-discord-bot"
version = "0.1.0"
entry = "main.rads"            # file to execute
deps = ["net", "http"]         # stdlib or plugins
bin = ["ai-discord-bot"]       # optional CLI entry names
desc = "AI helper that wires to Discord"
repo = "https://your-git-site/ai-discord-bot.git"
```

## Index format (served from website/Git)
`index.json` example:
```json
{
  "plugins": {
    "ai-discord-bot": {
      "name": "ai-discord-bot",
      "version": "0.1.0",
      "repo": "https://your-git-site/ai-discord-bot.git",
      "desc": "AI helper with Discord integration",
      "entry": "main.rads"
    }
  }
}
```

## Install layout (default)
- Plugin store: `~/.rads/plugins/<name>/`
- Manifest cached: `~/.rads/plugins/<name>/plugin.radsmeta`
- Local record: append to `package.rads` or `plugins.lock`.

## Execution model
- `rstar run <name>`: chdir into plugin folder, run `rads <entry> [args]`.
- Future: extend RADS runtime search path to auto-`include` plugins by name.

## Security considerations (MVP)
- Prefer HTTPS for index and repos.
- Verify manifest name matches request.
- (later) Checksums/signatures for archives.

## Initial test plugin (AI Discord bot)
- Repo layout:
  - `plugin.radsmeta`
  - `main.rads` (orchestrator)
  - `discord_client.rads`
  - `ai_bridge.rads` (stub; calls local/remote AI)
- Behavior: respond to `!ping` with `pong`; configurable token via env vars.

## Roadmap
1) Implement `tools/rstar/rstar.c` (commands: install/list/run; HTTP fetch TODO stub).
2) Add build/install hooks in `install.sh`/`Makefile`.
3) Publish sample `index.json` + `plugins/ai-discord-bot` scaffold.
4) Add plugin search path to RADS runtime (optional follow-up).
