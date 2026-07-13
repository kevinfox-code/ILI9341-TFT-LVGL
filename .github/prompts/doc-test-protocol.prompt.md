---
name: "Doc: Engineering Test Protocol"
description: "Scaffold or generate an engineering Test Protocol LaTeX document under doc/eng_test/test_protocol/, matching the project's existing test-protocol document class and structure. Supports two modes: derive real test steps from the codebase (existing project) or emit a blank placeholder template (new project)."
argument-hint: "<component-name> <mode: existing|blank> — e.g. 'ADC_Driver existing' or 'Modem blank'"
agent: "agent"
tools: [codebase, search, editFiles, runCommands, runTasks]
---

Generate or scaffold an **engineering Test Protocol** under [doc/eng_test/test_protocol/](../../doc/eng_test/test_protocol/), strictly following the project's existing test-protocol LaTeX document class.

## Inputs

- **Component/driver name** (required): the item under test (e.g. `ADC_Driver`, `Modem`, `USB_Device`). The file must be named `<Component>_Test_Protocol.tex`.
- **Mode** (required — do not assume a default): if the caller did not clearly state `existing` or `blank`, ask before writing anything.
  - `existing` — analyze the codebase and write concrete, executable test steps with real function names and expected results.
  - `blank` — emit a placeholder skeleton with generic steps, ready to fill in for a new project.

For a brand-new project with no `doc/eng_test/` tree, also copy the test-protocol document class (`.cls`) and the shared `assets/` from an existing tree (or scaffold blank equivalents) so the new protocol compiles.

## Structure to follow (do not deviate)

Mirror an existing protocol exactly — reference an existing `*_Test_Protocol.tex` under [doc/eng_test/test_protocol/](../../doc/eng_test/test_protocol/) before writing.
- Document class: the test-protocol `.cls` (same folder). Shared assets: [doc/eng_test/assets/](../../doc/eng_test/assets/).

The document is standalone and must contain, in order:
1. Copyright header comment + `\documentclass{<testprotocolclass>}` preamble with the same `\usepackage` list.
2. `\project{<Project>}`, `\docnumber{XXXXXX}`, `\docversion{00}`.
3. `\begin{document}`, `\maketestprotocoltitle`, a bold title line, `\maketestprotocoltoc`.
4. `\headingI` sections: **Purpose**, **Scope**, **Terminology & Abbreviations** (bordered `tabular`), **Test Method**, **Test Samples**, **Test Description** (with `\headingII{Test Set-up}` using `testprotocolitemize`, `\headingII{Test Cases}`, `\headingII{Test Instructions}` containing `\headingIII` test cases with a `longtable` of columns: Step / Test Step Execution / Expected Result / Actual Result Pass/Fail), **Test Record**, **References** (`tabular`), **Document Revision History** (`tabular`).
5. `\end{document}`.

## Content rules

- **existing mode**: Ground every test step in the real driver — reference actual functions with `\texttt{}` (e.g. `\texttt{<Module>\_Test()}`), realistic setup (debug adapter, DUT/STB, PC), and precise pass/fail criteria. Do not invent APIs; cite the source you used in your final summary.
- **blank mode**: Keep the same section skeleton and table shape with generic placeholder steps so it compiles and is ready to complete.
- Escape LaTeX special characters (`_`, `%`, `&`, `#`) correctly, especially in code identifiers.
- Do not modify the test-protocol `.cls` or shared assets.

## Verify

After writing, attempt to compile the protocol to confirm it builds — prefer an existing build task, otherwise run the available LaTeX toolchain (e.g. `latexmk -pdf <Component>_Test_Protocol.tex` or `pdflatex`) from `doc/eng_test/test_protocol/`. If no LaTeX toolchain is installed, skip compilation and say so rather than guessing a command. Report and fix any LaTeX errors your changes introduced.

## Output

After writing, report: the file created, the test cases included, the compile result (or why it was skipped), and (existing mode) the source files/functions that informed the steps. Flag that `\docnumber`, `\docversion`, author, and revision date placeholders still need real values.
