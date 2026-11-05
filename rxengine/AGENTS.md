# Repository Guidelines

## Project Structure & Module Organization

- `Program.cs` coordinates file discovery, CLI option parsing (`--as-of`, `--format`, `--output`), and reporting.
- `Parser/` houses the ANTLR grammar (`Prescription.g4`) plus generated lexer/parser/visitor classes; prefer `./regenerate-grammar.sh` after grammar edits so contributors without Java can still build.
- `Models/` defines the domain types (`Drug`, `DoseSchedule`, etc.) consumed across the app, while `Services/InventoryCalculator.cs` encapsulates inventory math and reporting.
- `.rx` files in the repo root are DSL examples; keep production inputs alongside the executable or mount them into the container.
- `Dockerfile` embeds the ANTLR runtime and builds a runnable image for distributing the CLI.

## Build, Test, and Development Commands

- `dotnet restore` (once) then `dotnet build` – compile the CLI locally; use `--no-restore` when offline.
- `dotnet run -- --format detailed ./ibuprofen.rx` – inspect a single regimen; quote globs (`dotnet run -- --output json "./*.rx"`) for consolidated JSON exports.
- `docker build -t prescription-tracker .` and `docker run -v $(pwd):/data prescription-tracker '/data/*.rx'` – containerized workflow with host prescriptions mounted under `/data`.
- `./regenerate-grammar.sh` – regenerate parser artifacts when `Prescription.g4` changes.

## Coding Style & Naming Conventions

- Follow standard C# 12 conventions: file-scoped namespaces, 4-space indentation, PascalCase for types/methods, camelCase for locals, and suffix DTO-like records with the domain noun (`InventoryReport`).
- Keep nullable reference types enabled and prefer expression-bodied members only when clearer than the current block style.
- Use string interpolation for composing report output and stay consistent with existing formatting width (`F2`, `F1`). Prefer lower-case units via `FormatUnit` helper.
- Run `dotnet format` before submitting to enforce whitespace and using-order consistency.

## Testing Guidelines

- No automated tests exist yet; add an xUnit project under `tests/` when contributing sizeable logic. Name tests `Method_Scenario_Expected` (e.g., `CalculateInventory_NoPurchases_Assumes30Days`).
- Manual validation: run `dotnet run -- --format concise './*.rx'`, optionally override the date (`--as-of 2025-01-01`) to simulate future depletion, and confirm status flags line up with expectations (⚠️ for ≤30 days, depleted inventories at 0).
- Exercise time-zone and start-date logic by crafting fixtures with `START`, `TIMEZONE`, and multiple `DOSE` entries.
- Include updated sample `.rx` fixtures when extending DSL constructs and document them in `README.md`.

## Commit & Pull Request Guidelines

- Git history favors short, imperative summaries (`Add wildcard support`, `Remove unneeded examples`). Keep messages under 72 characters and elaborate in the body if needed.
- Each PR should describe intent, list test or manual run results, and attach sample CLI output or screenshots for report changes.
- Reference related issues with `Fixes #ID` when applicable, and request review when inventory math or grammar behavior changes to ensure double-checking.

## Security & Data Handling

- Avoid checking in real prescription data; anonymize or use synthetic values in `.rx` fixtures.
- Treat `.rx` files as plain text—no secrets or credentials should appear in comments or reasons.
