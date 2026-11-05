# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A C# 12/.NET 8 CLI application that parses prescription tracking files written in a custom DSL, calculates current medication inventory, and predicts refill dates based on usage patterns. Uses ANTLR 4.13.2 for grammar parsing.

## Build and Development Commands

```bash
# Local development
dotnet restore                     # First-time dependency fetch
dotnet build                       # Compile the project
dotnet run -- [options] <file>     # Run with prescription file(s)

# Example runs
dotnet run -- --format detailed ./ibuprofen.rx
dotnet run -- --output json './*.rx'
dotnet run -- --as-of 2025-12-31 './*.rx'  # Simulate future date

# After initial build, run directly without rebuilding
dotnet bin/Debug/net8.0/PrescriptionTracker.dll --format concise './*.rx'

# Docker workflow
docker build -t prescription-tracker .
docker run -v $(pwd):/data prescription-tracker '/data/*.rx'

# Grammar regeneration (requires Java and antlr-4.13.2-complete.jar)
./regenerate-grammar.sh
```

## Architecture and Data Flow

### Core Processing Pipeline

1. **CLI Entry (`Program.cs`)**: Parses command-line arguments (`--as-of`, `--format`, `--output`), expands file globs, and coordinates processing
2. **ANTLR Parsing (`Parser/`)**: Lexer/Parser generated from `Prescription.g4` tokenize and parse `.rx` files
3. **AST Visitor (`Parser/DrugVisitor.cs`)**: Traverses parse tree, builds `Drug` objects with schedules, purchases, adjustments
4. **Inventory Calculation (`Services/InventoryCalculator.cs`)**: Computes current inventory and projects depletion dates
5. **Report Formatting (`Program.cs::ReportFormatter`)**: Outputs concise tables, detailed breakdowns, or JSON

### Key Domain Models (`Models/Drug.cs`)

- **Drug**: Represents a prescription with name, type, initial amount, optional unit, legacy rate (deprecated), timezone, start date, schedules, purchases, and adjustments
- **DoseSchedule**: Encodes periodic dosing with amount, period (Day/Week/Month), optional days-of-week/days-of-month filters, and optional time-of-day
- **Purchase/Adjustment**: Timestamped inventory changes
- **InventoryReport**: Output record with current inventory, daily usage rate, estimated depletion date, usage anchor date, and schedule details

### Inventory Calculation Logic (`Services/InventoryCalculator.cs`)

**Anchor Date Determination (`DetermineStart`)**:

- If `START` declared: use that date
- Else if purchases exist: use earliest purchase date
- Else: fallback to 30 days before report date (rolling window)

**Usage Calculation**:

- **Legacy Rate**: Deprecated `RATE` statement converted to daily equivalent (week÷7, month÷30). Applied uniformly from anchor date to report date.
- **Scheduled Doses**: Each `DOSE` statement generates a `DoseSchedule`. Calculator iterates day-by-day from anchor to report date, checking if each schedule "occurs on" that day based on period/filters, then sums actual doses consumed.

**Current Inventory Formula**:
```
InitialAmount + Purchases (≤ asOfDate) + Adjustments (≤ asOfDate)
  - Legacy Usage (anchor → asOfDate)
  - Scheduled Usage (anchor → asOfDate)
```

**Depletion Projection (`EstimateDepletionDate`)**:

- Simulates forward day-by-day consumption (legacy + scheduled) until inventory ≤ 0
- Safety limit: 3660 iterations (10 years)
- Returns the date inventory reaches zero, or null if never depleted

**Time Zone Handling**:

- `TIMEZONE` accepts IANA IDs (e.g., "America/Chicago")
- On Windows, attempts IANA→Windows conversion via `TimeZoneInfo.TryConvertIanaIdToWindowsId`
- Falls back to `TimeZoneInfo.Local` if unresolvable

### DSL Grammar (`Parser/Prescription.g4`)

**Statement Types**:

- `DRUG "Name" TYPE <type> INITIAL <amount> [UNIT <unit>]` – Starts a new drug block; all subsequent statements apply to this drug until next DRUG
- `RATE <amount> per <period>` – Legacy uniform rate (deprecated in favor of DOSE)
- `DOSE <amount> [UNIT <unit>] per <period> [ON <specifiers>] [AT HH:MM]` – Scheduled dose with optional day filters and time
- `START YYYY-MM-DD [HH:MM]` – Override usage anchor date
- `TIMEZONE "IANA/ID"` – Set timezone for this drug
- `PURCHASE YYYY-MM-DD <amount>` – Add inventory
- `ADJUSTMENT YYYY-MM-DD <amount> REASON "text"` – Manual correction (+ or -)
- `// comment` – Ignored

**Dose Specifiers**:

- Weekly: `ON Monday,Thursday` or `ON <day-name>`
- Monthly: `ON 1,15` (calendar days)
- Omit `ON` to repeat every period at anchor's weekday/day-of-month

**Multi-Drug Files**:

- Each `DRUG` statement switches context; one `.rx` file can define multiple prescriptions

## Grammar Development Workflow

1. Edit `Parser/Prescription.g4`
2. Run `./regenerate-grammar.sh` to regenerate C# parser code (requires Java + `antlr-4.13.2-complete.jar` at repo root)
3. Update `Parser/DrugVisitor.cs` visitor methods if grammar structure changed
4. Rebuild with `dotnet build`

ANTLR artifacts are checked into `Parser/` so contributors without Java can still compile. Only grammar authors need the ANTLR jar.

## Testing and Validation

No automated test suite exists. Manual validation workflow:

1. Create or modify `.rx` fixture files in repo root
2. Run with different formats: `dotnet run -- --format concise './*.rx'` and `--format detailed`
3. Verify depletion dates, status flags (⚠️ URGENT ≤7 days, SOON ≤30 days, DEPLETED at 0)
4. Test edge cases: no `START` (should anchor to first purchase), timezone conversion on Windows, monthly doses on day-31 in short months, weekly schedules with/without day filters
5. Override report date: `--as-of 2026-01-01` to project future scenarios
6. Check JSON output for correctness: `--output json`

When extending DSL, add sample `.rx` files demonstrating new syntax and update README.md examples.

## Code Style

- C# 12 conventions: file-scoped namespaces, 4-space indentation, PascalCase for types/methods, camelCase for locals
- Nullable reference types enabled
- Prefer expression-bodied members only when clearer than block style (current code uses blocks)
- String interpolation for report output; unit formatting via `FormatUnit` (lowercase: "mg", "pills", etc.)
- Use `dotnet format` before commits for consistency

## Important Implementation Details

**DrugVisitor State Machine (`Parser/DrugVisitor.cs`)**:

- Tracks `_currentDrug` field; each `VisitDrugDefinition` call creates a new Drug and switches context
- All other visit methods call `EnsureCurrentDrug()` which throws if no DRUG declared yet
- `GetDrugs()` finalizes and returns all accumulated drugs

**Schedule Occurrence Logic (`InventoryCalculator.cs`)**:

- `OccursOn` checks if a schedule fires on a given date
- Weekly with no days: repeats on same weekday as anchor
- Weekly with days: fires on specified weekdays
- Monthly with no days: fires on anchor's day-of-month (clamped to valid range)
- Monthly with days: fires on specified calendar days

**Report Ordering (`Program.cs::Main`)**:

- Reports sorted by `EstimatedDepletionDate` (nulls last), then by drug name
- Ensures urgent refills appear at top of output

**File Pattern Expansion (`GetMatchingFiles`)**:

- Supports `*` and `?` globs
- Quotes required when passing globs to shell: `dotnet run -- './*.rx'`
- Returns empty array if pattern matches nothing

## Security Considerations

- `.rx` files are plain text; never commit real prescription data—use synthetic fixtures
- No secrets or credentials should appear in DSL comments or REASON strings
- ANTLR error handling throws exceptions with line/column info; sanitize if exposing publicly
