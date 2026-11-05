# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

`parsemail` is a .NET Core console application designed to parse email files with `.Kagura` extension from a specified directory. The application:

1. Reads email files using the MimeKit library
2. Extracts email headers and saves them as CSV files
3. Extracts email content and attachments and saves them with appropriate file extensions
4. Organizes extracted content into timestamped directories

## Build and Run Commands

### Building the Project

```bash
# Build the project
dotnet build parsemail/parsemail.csproj

# Build with Release configuration
dotnet build parsemail/parsemail.csproj -c Release
```

### Running the Application

```bash
# Run from project directory
dotnet run --project parsemail/parsemail.csproj

# Run with specific data directory
DATA_DIR=/path/to/data dotnet run --project parsemail/parsemail.csproj
```

### Docker Support

```bash
# Build Docker image
docker build -t parsemail .

# Run Docker container (mounting a data volume)
docker run -v /path/to/local/data:/data parsemail
```

## Code Architecture

The application has a simple architecture with the main components:

1. **Main Method** (`Program.cs:13-71`): Entry point that scans the data directory for `.Kagura` files and processes each one

2. **HandleHeaders** (`Program.cs:82-131`): Extracts email headers and writes them to a CSV file, skipping certain headers like DKIM signatures

3. **HandleMimeEntity** (`Program.cs:135-209`): Recursively processes MIME entities (multipart, message parts, attachments) and saves content to appropriate files

4. **Helper Methods**:
   - `GetNextCount()` (`Program.cs:75-79`): Generates sequential counters for naming
   - `SanitizeFileName()` (`Program.cs:212-218`): Sanitizes filenames to remove invalid characters

## Dependencies

The project uses the following NuGet packages:

- **MimeKit** (v4.11.0): For parsing email messages
- **CsvHelper** (v33.0.1): For creating CSV files from email headers

## Environment Variables

- `DATA_DIR`: Optional environment variable to specify the input directory containing `.Kagura` files (defaults to `/data`)
