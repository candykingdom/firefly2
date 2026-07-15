# Claude Code and Codex setup

Firefly2 exposes each tool's native discovery paths while keeping shared guidance and
skills single-source.

| Capability | Source of truth | Compatibility surface |
|---|---|---|
| Project guidance | `CLAUDE.md` | `AGENTS.md` links to it |
| Skills | `.claude/skills/<name>/` | `.agents/skills/<name>` links to it |
| Delegation policy | `CLAUDE.md` | Tool-specific role wrappers below |
| Claude roles | `.claude/agents-available/` | Opt-in links under `.claude/agents/` |
| Codex roles | `.codex/agents/` | Loaded in trusted Codex sessions |

## Skills

Edit skills only under `.claude/skills/`. Codex follows the symlinked skill directories,
so both tools execute the same `SKILL.md` and any adjacent scripts or references.

When adding a skill:

1. Create `.claude/skills/<name>/SKILL.md`.
2. Add `.agents/skills/<name> -> ../../.claude/skills/<name>`.
3. Run `npm run check:agents` and start a new Codex or Claude Code session if discovery is
   stale.

Spec Kit remains registered as the Claude integration because its generated canonical
files live under `.claude/skills/`. Do not install a second Codex Spec Kit integration;
that would duplicate the skill bodies. Invoke a linked skill as `$speckit-<command>` in
Codex or `/speckit-<command>` in Claude Code.

## Delegation roles

| Role | Claude tier | Codex tier | Use |
|---|---|---|---|
| `Explore` / `explore` | Haiku / low | Terra / low | Broad read-only exploration |
| `scout` | Haiku / low | Luna / low | One focused lookup or summary |
| `mech-executor` | Sonnet / medium | Terra / medium | Fully specified mechanical edits |

The tools require different role schemas, so their thin role wrappers remain separate.
The shared routing rules live in `CLAUDE.md` through the `AGENTS.md` compatibility link.
The parent agent owns architecture, invariants, ambiguous decisions, and final review.

Claude templates are opt-in. Enable them per machine from the repository root:

```sh
mkdir -p .claude/agents
for f in Explore scout mech-executor; do
  ln -sf ../agents-available/$f.md .claude/agents/$f.md
done
```

Codex project roles load automatically after the repository is trusted. The parent model
is intentionally not pinned by project config: choose Sol high/xhigh when the task needs
hard judgment, or select a cheaper main-session setting when delegation would add no value.
Project configuration caps delegation at four direct children, and workers cannot recurse.
