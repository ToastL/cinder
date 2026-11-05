---
type: meta
last-update: 2026-11-05
---

How to get the most out of Claude on cinder with the least token use. Rules Claude follows are in `CLAUDE.md` at the vault root; repo rules in the repo's `CLAUDE.md`.

## Setup
- 2026-11-05: Vault created at `cinder/notes/`, inside the repo. Start Claude in `cinder/`, not `notes/`: auto-memory, git, the build and `.claude/` settings are keyed to the repo root. `notes/CLAUDE.md` loads when Claude works on files here.
- Claude's auto-memory for this repo lives in `~/.claude/projects/-Users-toaster-Desktop-projects-c-cpp-cinder/memory/`. Its decisions are mirrored in `Notes/Decisions/`.
- Obsidian skills (obsidian-markdown, obsidian-bases, json-canvas, obsidian-cli) are installed user-wide.
- The `/log` command is wired to the Veld vault, not this one.

## Habits that save tokens
- Start a new session (`/clear`) per unrelated task. The vault and the repo's `CLAUDE.md` carry the context.
- Point Claude at a system note ("see [[Physics]]") instead of "look around". The repo `CLAUDE.md` is ~1,400 lines; a system note is the cheap entry point.
- Paste only the relevant part of a log or failing test.
- Use a lower effort or a faster model for mechanical edits (`/model`, `/effort`).

## Improvement ideas
- [ ] A cinder `/log` (or make `/log` pick the vault from the working directory), so sessions here update this vault's daily notes.
- [ ] A one-line pointer to this vault in the repo's `CLAUDE.md`, so every session knows it exists.
- [x] 2026-11-05: `notes/.obsidian/workspace*.json` and `cache` ignored in the repo `.gitignore`.
