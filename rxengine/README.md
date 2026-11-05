# Prescription Tracker

A C# application with an ANTLR-based DSL for tracking prescription medications, calculating current inventory, and predicting when you'll need to refill.

## Features

- **Custom DSL**: Define prescriptions using a simple, human-readable language with per-dose schedules
- **Multi-Drug Workflows**: Describe several prescriptions inside a single `.rx` file and consolidate the output automatically
- **Inventory Tracking**: Track initial amounts, purchases, and adjustments with optional start dates and time zones
- **Scheduling**: Mix daily rates with day-of-week or day-of-month dosing blocks to model real-world routines
- **Flexible Reporting**: Choose concise tables, detailed breakdowns, or JSON with `--format`/`--output`
- **Docker Support**: Fully containerized application with ANTLR jar included

## DSL Syntax

The prescription language supports the following statements:

### Drug Definition
```
DRUG "Medication Name" TYPE pills|tablet|capsule|liquid|injection|supplement INITIAL amount [UNIT mg|ml|pills|capsules|units|drops|mcg]
```

### Usage Rate
```
RATE amount per day|week|month
```

### Scheduled Dose
```
DOSE amount [UNIT ...] per day|week|month [ON Monday,Thursday|15] [AT HH:MM]
```

Use `ON` with day names for weekly schedules or calendar numbers for monthly dosing. Omit `ON` to repeat every period. `AT` stamps a local time for each dose.

### Start Date
```
START YYYY-MM-DD [HH:MM]
```

### Time Zone
```
TIMEZONE "America/Chicago"
```

### Purchase
```
PURCHASE YYYY-MM-DD amount
```

### Adjustment
```
ADJUSTMENT YYYY-MM-DD amount REASON "explanation"
```

### Comments
```
// This is a comment
```

## Example Prescription File

```
// Weekly injection split across two mornings
DRUG "Saline Solution" TYPE injection INITIAL 0 UNIT ml
START 2025-06-01 07:00
TIMEZONE "America/Chicago"
DOSE 0.5 UNIT ml per week ON Monday,Thursday AT 07:00
PURCHASE 2025-10-05 12

// Tablet taken once each week in the evening
DRUG "Ibuprofen" TYPE tablet INITIAL 0 UNIT pills
DOSE 1 per week ON Thursday AT 20:00
PURCHASE 2025-10-05 12
```

## Building and Running with Docker

### Build the Docker image
```bash
docker build -t prescription-tracker .
```

### Run with a prescription file
```bash
docker run -v $(pwd):/data prescription-tracker /data/example.rx
```

### Run with all prescription files (consolidated report)
```bash
docker run -v $(pwd):/data prescription-tracker '/data/*.rx'
```

Or on Windows (PowerShell):
```powershell
docker run -v ${PWD}:/data prescription-tracker /data/example.rx
```

## Running Locally

```bash
dotnet build
dotnet run -- --format detailed ./ibuprofen.rx
dotnet run -- --output json './*.rx'
# After building once, rerun without rebuilding:
dotnet bin/Debug/net8.0/PrescriptionTracker.dll --format concise './*.rx'
```

Use `--format concise|detailed` to switch layouts and `--output json` to capture machine-readable summaries. The default report date is `DateTime.Today`; override it with `--as-of` to simulate future scenarios.

## Example Output

```
Report Date: 2026-01-15
Prescriptions Tracked: 4

Drug                          Remaining    Daily Avg    Refill By    Status
------------------------------------------------------------------
Ibuprofen                          30.0 pills     1.00 pills/d   2026-02-14 
Multivitamin                       60.0 pills     1.00 pills/d   2026-03-16 
Saline Solution                    10.0 ml        0.14 ml/d      2026-03-26 
1ml Syringes                       12.0 units     0.29 units/d   2026-02-25 
```

*(Output truncated for brevity; run with `--format detailed` to see schedules, anchors, and warning flags for every entry.)*

## Project Structure

```
.
├── Dockerfile              # Docker container configuration
├── Prescription.g4         # ANTLR grammar definition
├── PrescriptionTracker.csproj
├── Models/
│   └── Drug.cs            # Data models
├── Services/
│   └── InventoryCalculator.cs  # Inventory calculation logic
├── Parser/
│   └── PrescriptionVisitor.cs  # ANTLR visitor implementation
├── Program.cs             # Main application entry point and CLI
├── regenerate-grammar.sh  # Helper script to rebuild parser artifacts
├── example.rx             # Example prescription (pills)
└── example_liquid.rx      # Example prescription (liquid)
```

## How It Works

1. **Grammar Definition**: The `Prescription.g4` file defines the DSL grammar
2. **Parser Generation**: During Docker build, ANTLR generates C# parser code from the grammar
3. **Parsing**: The application reads your prescription file and parses it into a data model
4. **Calculation**: The inventory calculator:
   - Starts with the initial amount
   - Adds purchases and adjustments up to the report date
   - Applies scheduled doses and legacy rates from the declared start (or a 30-day rolling default)
   - Simulates future usage to project the depletion date
5. **Reporting**: Generates a report showing current inventory and when you'll need to refill

## Notes

- Dates use ISO `YYYY-MM-DD` and optional `HH:MM` times in local time
- `TIMEZONE` accepts IANA IDs (e.g., `America/Chicago`) and converts automatically on Windows
- Adjustments can be positive (found extra pills) or negative (lost pills)
- If no `START` is provided, the calculator anchors on the first purchase or assumes 30 days of prior usage
- Weekly doses distribute per specified weekdays; monthly doses accept calendar days (e.g., `ON 1,15`)
- Warnings highlight depleted prescriptions or refills due within 30/7 days

## License

This is a sample project for prescription tracking. Use at your own discretion.
