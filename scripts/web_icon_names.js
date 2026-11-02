#!/usr/bin/env node
"use strict";

const fs = require("node:fs");
const esbuild = require("esbuild");

// Scan the code that can run in the browser. Type declarations, annotations,
// type-only imports and comments cannot render icons. Use the same compiler
// as the bundle rather than maintaining a partial TypeScript parser here.
function runtimeIconNames(sources, knownNames) {
  const known = new Set(knownNames);
  const names = new Set();
  for (const { file, source } of sources) {
    const { code } = esbuild.transformSync(source, {
      loader: "ts",
      sourcefile: file,
      // Erase types without adding downlevel helpers and their own strings
      // (for example "symbol" for private-field property conversion).
      target: "esnext",
      legalComments: "none",
      logLevel: "silent",
    });
    for (const match of code.matchAll(/\bmdi-([a-z0-9-]+)\b/g)) {
      // Explicit classes must be checked even when the name is unknown: a
      // typo or missing local codepoint must not silently remove the glyph.
      names.add(match[1]);
    }
    for (const match of code.matchAll(/["']([a-z][a-z0-9-]*)["']/g)) {
      if (known.has(match[1])) names.add(match[1]);
    }
  }
  return [...names].sort();
}

if (require.main === module) {
  try {
    const { sources, knownNames } = JSON.parse(fs.readFileSync(0, "utf8"));
    process.stdout.write(JSON.stringify(runtimeIconNames(sources, knownNames)));
  } catch (error) {
    process.stderr.write(`${error.message}\n`);
    process.exitCode = 1;
  }
}

module.exports = { runtimeIconNames };
