---
name: handoff
description: Prepare a durable handoff before the user clears context or switches coding agents. Capture what is done, next, blocked, and learned in the active spec or repository docs; verify and commit the handoff so a fresh Claude Code or Codex session can resume with zero prior context. Use when the user says they are about to clear, wants a checkpoint, asks to save context, or requests a handoff.
---

# Prepare a context handoff

Write everything needed by a fresh coding agent with no conversation history to a durable,
committed repository location. Be concrete about paths, facts, failures, and unverified claims.

## Procedure

1. **Take stock.** Determine the current branch, active `specs/<NNN>-*/` feature if any,
   recent commits, working-tree state, completed work, next action, blockers, decisions,
   corrections, and dead ends.

2. **Protect unrelated work.** Inspect `git status` and distinguish changes from this task
   from pre-existing user changes. Do not sweep unrelated changes into the handoff commit.
   If important work remains uncommitted, either commit it when authorized or identify it
   explicitly in the handoff. Never imply that working-tree state exists in a fresh checkout.

3. **Write the primary handoff.**

   - With an active spec feature, prepend a dated handoff to its `plan.md` and mark the prior
     handoff `SUPERSEDED` without deleting history.
   - Without an active feature, update or create `docs/session-handoff.md`.

   Write for zero prior context and include:

   - **START HERE:** exact files to read first.
   - **Purpose:** one line describing the work.
   - **Done and committed:** commit subjects/hashes and observed verification results.
   - **Not done or not proven:** distinguish host tests, simulator tests, and hardware tests.
   - **Next step:** a concrete action, not a vague direction.
   - **Blockers/open questions:** including anything requiring the user, hardware, or radio
     testing.
   - **Key facts:** commands, paths, identifiers, environment names, upload methods, test
     filters, pin/flash quirks, and other facts the next session would otherwise re-derive.

4. **Update durable project knowledge.** Fold non-obvious behavior learned this session into
   the relevant `docs/*.md` file. Keep `CLAUDE.md` accurate if an invariant, build command,
   workflow, or compatibility rule changed. Do not rely on tool-private memory as the only
   copy of a fact needed by the next agent.

5. **Verify proportionally.** If code changed, run the relevant tests and `./lint.sh check`.
   Record actual results and failures. Never label hardware behavior verified from host or
   simulator tests alone.

6. **Commit the handoff.** Stage only the task's handoff, documentation, and authorized work.
   Use a clear subject such as `handoff: <feature> — <state>`. Confirm the handoff is present
   in `HEAD`; do not claim a fresh checkout can resume from uncommitted files.

7. **Respond briefly.** In three to five lines, tell the user the current state, most important
   next step, exact start-here file, commit hash, and whether it is safe to clear.

8. **Emit a next-session prompt last.** Provide one fenced, copyable prompt for a fresh Claude
   Code or Codex session. Include:

   - project and branch plus the fact that the new agent has zero prior context;
   - exact files to read first, in order;
   - a two-to-four-line current-state summary, including what is unproven;
   - concrete next tasks;
   - durable working rules and relevant `CLAUDE.md` invariants; and
   - an instruction to read first, then plan or proceed.

   If a clipboard command is available and permitted, copy that exact prompt and say so.
   Otherwise show the fenced prompt without claiming clipboard success.

## Guardrails

- Put more detail in the committed handoff than in the chat response.
- State corrected or retracted claims plainly; a confident but wrong handoff is worse than
  an incomplete one.
- Do not begin new implementation work while preparing the handoff.
- Do not commit, overwrite, or discard unrelated user changes.
