import console from "node:console";
import { lstat, readFile, readdir, readlink, realpath } from "node:fs/promises";
import path from "node:path";
import process from "node:process";

const root = process.cwd();
const failures = [];
let skillCount = 0;
const fail = (message) => failures.push(message);
const resolve = (relative) => path.join(root, relative);
const read = (relative) => readFile(resolve(relative), "utf8");

async function expectText(relative, pattern, message) {
  try {
    const content = await read(relative);
    if (!pattern.test(content)) fail(message);
  } catch (error) {
    fail(`${relative}: ${error.message}`);
  }
}

try {
  const [stat, target, resolved] = await Promise.all([
    lstat(resolve("AGENTS.md")),
    readlink(resolve("AGENTS.md")),
    realpath(resolve("AGENTS.md")),
  ]);
  if (!stat.isSymbolicLink()) fail("AGENTS.md must be a symlink to canonical CLAUDE.md");
  if (target !== "CLAUDE.md") fail(`AGENTS.md has unexpected target ${target}`);
  if (resolved !== resolve("CLAUDE.md")) fail("AGENTS.md must resolve to canonical CLAUDE.md");
} catch (error) {
  fail(`AGENTS.md: ${error.message}`);
}

await expectText(
  "CLAUDE.md",
  /\.claude\/skills\/[\s\S]*?\.agents\/skills/,
  "CLAUDE.md must document the canonical skill tree and Codex bridge",
);
await expectText(
  "CLAUDE.md",
  /Use direct children only and at most four active\s+threads/,
  "CLAUDE.md must keep delegation shallow and bounded",
);
await expectText(
  ".gitignore",
  /^\.claude\/agents\/$/m,
  "active per-machine Claude agents must stay ignored",
);
await expectText(
  ".codex/config.toml",
  /\[agents\][\s\S]*?^max_threads\s*=\s*4$[\s\S]*?^max_depth\s*=\s*1$/m,
  "Codex delegation must stay capped at four direct children",
);

for (const [relative, model, effort] of [
  [".codex/agents/explore.toml", "gpt-5.6-terra", "low"],
  [".codex/agents/scout.toml", "gpt-5.6-luna", "low"],
  [".codex/agents/mech-executor.toml", "gpt-5.6-terra", "medium"],
]) {
  await expectText(
    relative,
    new RegExp(`^model\\s*=\\s*"${model}"$`, "m"),
    `${relative} must pin ${model}`,
  );
  await expectText(
    relative,
    new RegExp(`^model_reasoning_effort\\s*=\\s*"${effort}"$`, "m"),
    `${relative} must use ${effort} reasoning effort`,
  );
}

const canonicalRoot = resolve(".claude/skills");
const bridgeRoot = resolve(".agents/skills");

try {
  const skillNames = (await readdir(canonicalRoot, { withFileTypes: true }))
    .filter((entry) => entry.isDirectory())
    .map((entry) => entry.name)
    .sort();
  const bridgeEntries = await readdir(bridgeRoot, { withFileTypes: true });

  for (const entry of bridgeEntries) {
    if (!entry.isSymbolicLink()) {
      fail(`.agents/skills/${entry.name} must be a symlink to .claude/skills/${entry.name}`);
    }
  }

  const bridgeNames = bridgeEntries.map((entry) => entry.name).sort();
  if (skillNames.join("\n") !== bridgeNames.join("\n")) {
    fail(".agents/skills links must exactly match canonical .claude/skills directories");
  }

  for (const name of skillNames) {
    const skillFile = path.join(canonicalRoot, name, "SKILL.md");
    const bridge = path.join(bridgeRoot, name);
    try {
      const [stat, target, resolved, skill] = await Promise.all([
        lstat(bridge),
        readlink(bridge),
        realpath(bridge),
        readFile(skillFile, "utf8"),
      ]);
      if (!stat.isSymbolicLink()) fail(`${path.relative(root, bridge)} must be a symlink`);
      if (target !== `../../.claude/skills/${name}`) {
        fail(`${path.relative(root, bridge)} has unexpected target ${target}`);
      }
      if (resolved !== path.join(canonicalRoot, name)) {
        fail(`${path.relative(root, bridge)} resolves outside the canonical skill directory`);
      }
      if (
        !/^---\n[\s\S]*?\n---\n/.test(skill) ||
        !/^name:\s*["']?[^\n"']+/m.test(skill) ||
        !/^description:\s*["']?[^\n"']+/m.test(skill)
      ) {
        fail(`${path.relative(root, skillFile)} has invalid or missing frontmatter`);
      }
    } catch (error) {
      fail(`${path.relative(root, bridge)}: ${error.message}`);
    }
  }

  skillCount = skillNames.length;
} catch (error) {
  fail(`skill bridge: ${error.message}`);
}

for (const relative of [
  ".claude/agents-available/Explore.md",
  ".claude/agents-available/scout.md",
  ".claude/agents-available/mech-executor.md",
  ".codex/agents/explore.toml",
  ".codex/agents/scout.toml",
  ".codex/agents/mech-executor.toml",
]) {
  try {
    await lstat(resolve(relative));
  } catch (error) {
    fail(`${relative}: ${error.message}`);
  }
}

if (failures.length) {
  console.error(failures.map((message) => `- ${message}`).join("\n"));
  process.exit(1);
}

console.log(
  `Agent setup OK: ${skillCount} canonical skills bridged to Codex.`,
);
