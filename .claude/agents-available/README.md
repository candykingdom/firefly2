# Claude Code delegation templates (opt-in)

Shared Claude Code/Codex setup, single-source skill layout, and role parity are documented
in [`.agents/README.md`](../../.agents/README.md). This directory contains the Claude-specific
role wrappers.

These agents are opt-in: `.claude/agents/` is gitignored, so the templates do not affect a
developer until linked into that active directory.

| Agent | Model | Effort | Use |
|---|---|---|---|
| `Explore` | Haiku | low | Broad read-only repository exploration |
| `scout` | Haiku | low | One focused lookup or concise summary |
| `mech-executor` | Sonnet | medium | Fully specified mechanical edits |

From the repository root, enable them with:

```sh
mkdir -p .claude/agents
for f in Explore scout mech-executor; do
  ln -sf ../agents-available/$f.md .claude/agents/$f.md
done
```

The agents take effect in new Claude Code sessions. Keep delegation narrow: use a worker
only when bounded exploration, context isolation, or useful parallelism outweighs the extra
worker context and coordination cost. The parent retains architecture, invariants, ambiguous
decisions, and final review.
