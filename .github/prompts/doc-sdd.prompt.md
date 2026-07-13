---
name: "Doc: SDD Section"
description: "Scaffold or generate a Software Design Description (SDD) LaTeX section under doc/sdd/, matching the project's existing SDD document class and structure. Supports two modes: derive real content from the codebase (existing project) or emit blank placeholder templates (new project)."
argument-hint: "<subsystem-name> <mode: existing|blank> — e.g. 'PowerManagement existing' or 'Humidifier blank'"
agent: "agent"
tools: [codebase, search, editFiles, runCommands, runTasks]
---

Generate or scaffold a **Software Design Description (SDD)** section under [doc/sdd/](../../doc/sdd/), strictly following the conventions of the project's existing SDD LaTeX document class.

## Inputs

- **Subsystem/component name** (required): the design area to document (e.g. `PowerManagement`, `Humidifier`, `FirmwareUpdate`). Derive a `CamelCase` folder name and a human-readable title. For a brand-new project this may be omitted when scaffolding the full skeleton (see Scope below).
- **Mode** (required — do not assume a default): if the caller did not clearly state `existing` or `blank`, ask before writing anything.
  - `existing` — analyze the codebase and write real, accurate content describing the actual design.
  - `blank` — emit a placeholder skeleton (`Blah, Blah, Blah` / `TODO` style) matching sibling stubs, with no invented behavior.

## Scope

- **Add a design section** (default): create/populate one subsystem section under `doc/sdd/design/`.
- **New-project full skeleton**: when the caller asks for a brand-new SDD (no existing `doc/sdd/`), scaffold the complete top-level structure — `sdd.tex`, the front-matter section files (`overview.tex`, `purpose.tex`, `scope.tex`, `glossary.tex`, `references.tex`, `revisions.tex`, `appendix.tex`, `design.tex`), the `architecture/` folder (`hwArchitecture.tex`, `swArchitecture.tex`), and the required `.sty`/`.cls` style files — using blank placeholders. Copy the style/class files from an existing `doc/sdd/` tree rather than authoring them from scratch when one is available.

## Structure to follow (do not deviate)

Reference the existing layout before writing:
- Main document: [doc/sdd/sdd.tex](../../doc/sdd/sdd.tex) (`\documentclass{<sddclass>}`, `\makesdd{}`, `\project{<Project>}`, `\docnumber`, `\docversion`).
- Section aggregator: [doc/sdd/design.tex](../../doc/sdd/design.tex) — imports each subsystem via `\import{design/<Folder>/}{<File>.tex}` in alphabetical order (keep any fixed lead section first).
- Front-matter sections live as sibling `.tex` files: `overview.tex`, `purpose.tex`, `scope.tex`, `glossary.tex`, `references.tex`, `revisions.tex`, `appendix.tex`.
- Architecture: [doc/sdd/architecture/](../../doc/sdd/architecture/) (`hwArchitecture.tex`, `swArchitecture.tex`).
- Example design leaf to mirror: an existing subsystem leaf under [doc/sdd/design/](../../doc/sdd/design/).

## Steps

1. Create the folder `doc/sdd/design/<Folder>/` (add an `assets/` subfolder only if figures are referenced).
2. Create `doc/sdd/design/<Folder>/<File>.tex` that begins with `\headingII{<Title>} \label{sec:<Folder>}` followed by `\headingIII{...}` subsections.
3. Add (or uncomment) the matching `\import{design/<Folder>/}{<File>.tex}` line in [doc/sdd/design.tex](../../doc/sdd/design.tex), keeping the list alphabetical.
4. Use only the existing heading macros (`\headingII`, `\headingIII`), `itemize`/`enumerate`, `align*` for math, and `longtable` for tables — do **not** introduce new packages or classes.

## Content rules

- **existing mode**: For each `\headingIII` sub-block, describe initialization sequences, provided functionality (as an `\itemize` list), timing/threading, error/fault reporting, and any register/config details grounded in the actual source. Do not fabricate values — cite the code you drew them from in your final summary.
- **blank mode**: Emit the same heading skeleton with short placeholder bodies so the section compiles and is ready to fill in.
- Escape LaTeX special characters (`_`, `%`, `&`, `#`) correctly.
- Never modify the `.sty`/`.cls` style files or front-matter numbering.

## Verify

After writing, attempt to compile the document to confirm it builds — prefer an existing build task, otherwise run the available LaTeX toolchain (e.g. `latexmk -pdf sdd.tex` or `pdflatex`) from `doc/sdd/`. If no LaTeX toolchain is installed, skip compilation and say so rather than guessing a command. Report and fix any LaTeX errors your changes introduced.

## Output

After writing, report: the files created/edited, the `\import` line added, the compile result (or why it was skipped), and (existing mode) the source files that informed the content. Note if `sdd.tex` `\docnumber`/`\docversion` still need to be filled in.
