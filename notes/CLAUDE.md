# Vault rules for Claude

This vault (`cinder/notes`) is the long-term memory for cinder work. Read the vault, not old chat history.

## Where things live
- `Systems/Cinder HQ.md` — hub. Start here to find a system.
- `Systems/<System>.md` — system overview (properties: status, layer, source, started, last-update). Sub-notes in `Systems/<System>/`.
- `Notes/` — cross-cutting knowledge: `Conventions`, `Verification recipes`, `Roadmap`, `Timeline`, `Working with Claude`. Decisions in `Notes/Decisions/`.
- `Daily/YYYY-MM-DD.md` — session log (template `Templates/Daily.md`).
- `00 Inbox/` — unsorted captures; file them into the right place when touched.
- `Attachments/` — images (captures, diagrams) only.

## Source of truth
- The repo's `CLAUDE.md` and `TODO.md` are authoritative. Notes here summarize and link; when they disagree, the repo wins — fix the note.
- `source:` in a note's properties is a repo-relative path. Check it still exists before quoting a note as fact.

## How to work
- Before a task: read the hub, then only the notes for the system at hand. Don't scan the whole vault.
- After meaningful work: a daily-note entry, dated lines under `## Notes` in affected system notes, bump `last-update`. A finished roadmap item is ticked in `TODO.md`, not only here.
- A user decision (something they chose or rejected) gets a note in `Notes/Decisions/` with the date and their words.
- New note → right folder, frontmatter properties, `[[wikilinks]]` to its system. One topic per note; prefer updating over duplicating.
- Use Obsidian syntax (skills: obsidian-markdown, obsidian-bases, json-canvas).
- `(?)` marks unverified facts — in note text only, never in properties.
- Never write secrets, keys or passwords into the vault.

## Token efficiency
- Keep notes short and factual; summaries over transcripts.
- Read targeted files/sections (grep, offsets) instead of whole folders.
- Tell the user when a request could be done cheaper, and add standing tips to `Notes/Working with Claude.md`.
