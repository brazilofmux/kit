using System.Globalization;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;
using Antlr4.Runtime;
using PrescriptionTracker.Models;
using PrescriptionTracker.Parser;
using PrescriptionTracker.Services;

namespace PrescriptionTracker;

internal static class Program
{
    private static readonly InventoryCalculator Calculator = new();

    private static int Main(string[] args)
    {
        var parseResult = AppOptions.Parse(args);

        if (parseResult.RequestHelp)
        {
            PrintUsage();
            return 0;
        }

        if (!parseResult.IsSuccess)
        {
            if (!string.IsNullOrWhiteSpace(parseResult.Error))
            {
                Console.Error.WriteLine(parseResult.Error);
            }

            if (parseResult.ShowUsage)
            {
                PrintUsage();
            }

            return parseResult.ExitCode;
        }

        var options = parseResult.Options ?? throw new InvalidOperationException("Options missing");
        var allReports = new List<InventoryReport>();
        var exitCode = 0;

        foreach (var pattern in options.Patterns)
        {
            var files = GetMatchingFiles(pattern);
            if (files.Length == 0)
            {
                Console.Error.WriteLine($"No files found matching '{pattern}'.");
                exitCode = 1;
                continue;
            }

            foreach (var file in files)
            {
                try
                {
                    var reports = ProcessPrescriptionFile(file, options.AsOfDate);
                    allReports.AddRange(reports);
                }
                catch (Exception ex)
                {
                    exitCode = 1;
                    Console.Error.WriteLine($"Error in '{file}': {ex.Message}");
                }
            }
        }

        if (allReports.Count == 0)
        {
            if (exitCode == 0)
            {
                Console.Error.WriteLine("No prescriptions processed.");
                exitCode = 1;
            }

            return exitCode;
        }

        var orderedReports = allReports
            .OrderBy(r => r.EstimatedDepletionDate ?? DateTime.MaxValue)
            .ThenBy(r => r.DrugName, StringComparer.OrdinalIgnoreCase)
            .ToList();

        switch (options.Output)
        {
            case OutputKind.Json:
                ReportFormatter.WriteJson(orderedReports, options);
                break;
            case OutputKind.Text:
            default:
                ReportFormatter.WriteText(orderedReports, options);
                break;
        }

        return exitCode;
    }

    private static void PrintUsage()
    {
        Console.WriteLine("Usage: PrescriptionTracker [options] <prescription-file-or-pattern> [...]");
        Console.WriteLine();
        Console.WriteLine("Options:");
        Console.WriteLine("  --as-of YYYY-MM-DD    Override the report date (defaults to today).");
        Console.WriteLine("  --format concise|detailed");
        Console.WriteLine("                        Choose text layout (defaults to concise).");
        Console.WriteLine("  --output text|json    Render as text (default) or JSON.");
        Console.WriteLine("  -h, --help            Show this help.");
    }

    private static string[] GetMatchingFiles(string pattern)
    {
        if (pattern.Contains('*') || pattern.Contains('?'))
        {
            var directory = Path.GetDirectoryName(pattern);
            if (string.IsNullOrEmpty(directory))
            {
                directory = Directory.GetCurrentDirectory();
            }

            var fileName = Path.GetFileName(pattern);
            return Directory.GetFiles(directory, fileName)
                .OrderBy(f => f, StringComparer.OrdinalIgnoreCase)
                .ToArray();
        }

        return File.Exists(pattern) ? new[] { pattern } : Array.Empty<string>();
    }

    private static IEnumerable<InventoryReport> ProcessPrescriptionFile(string filePath, DateTime asOfDate)
    {
        var input = File.ReadAllText(filePath);
        var inputStream = new AntlrInputStream(input);

        var lexer = new PrescriptionLexer(inputStream);
        lexer.RemoveErrorListeners();
        lexer.AddErrorListener(ThrowingErrorListener.Instance);

        var tokenStream = new CommonTokenStream(lexer);
        var parser = new PrescriptionParser(tokenStream);
        parser.RemoveErrorListeners();
        parser.AddErrorListener(ThrowingErrorListener.Instance);

        var programContext = parser.program();

        var visitor = new DrugVisitor();
        visitor.Visit(programContext);

        var drugs = visitor.GetDrugs();
        foreach (var drug in drugs)
        {
            var report = Calculator.CalculateInventory(drug, asOfDate);
            report.SourceFile = filePath;
            yield return report;
        }
    }
}

internal enum TextFormat
{
    Concise,
    Detailed
}

internal enum OutputKind
{
    Text,
    Json
}

internal sealed record AppOptions(
    DateTime AsOfDate,
    TextFormat TextFormat,
    OutputKind Output,
    IReadOnlyList<string> Patterns)
{
    public static ParseResult Parse(string[] args)
    {
        if (args.Length == 0)
        {
            return ParseResult.Fail("Missing prescription file or pattern.", showUsage: true);
        }

        var asOfDate = DateTime.Today;
        var textFormat = TextFormat.Concise;
        var output = OutputKind.Text;
        var patterns = new List<string>();

        for (var index = 0; index < args.Length; index++)
        {
            var arg = args[index];

            if (arg is "--help" or "-h")
            {
                return ParseResult.Help();
            }

            if (arg.StartsWith("--", StringComparison.Ordinal))
            {
                var (name, value) = SplitOption(arg);
                switch (name)
                {
                    case "--as-of":
                        if (value == null)
                        {
                            if (index + 1 >= args.Length)
                            {
                                return ParseResult.Fail("Missing value for --as-of.", showUsage: true);
                            }

                            value = args[++index];
                        }

                        if (!DateOnly.TryParseExact(value, "yyyy-MM-dd", CultureInfo.InvariantCulture, DateTimeStyles.None, out var dateOnly))
                        {
                            return ParseResult.Fail($"Invalid value '{value}' for --as-of. Expected YYYY-MM-DD.", showUsage: true);
                        }

                        asOfDate = dateOnly.ToDateTime(TimeOnly.MinValue);
                        break;

                    case "--format":
                        if (value == null)
                        {
                            if (index + 1 >= args.Length)
                            {
                                return ParseResult.Fail("Missing value for --format.", showUsage: true);
                            }

                            value = args[++index];
                        }

                        switch (value.ToLowerInvariant())
                        {
                            case "concise":
                                textFormat = TextFormat.Concise;
                                break;
                            case "detailed":
                                textFormat = TextFormat.Detailed;
                                break;
                            default:
                                return ParseResult.Fail($"Unknown format '{value}'.", showUsage: true);
                        }

                        break;

                    case "--output":
                        if (value == null)
                        {
                            if (index + 1 >= args.Length)
                            {
                                return ParseResult.Fail("Missing value for --output.", showUsage: true);
                            }

                            value = args[++index];
                        }

                        switch (value.ToLowerInvariant())
                        {
                            case "text":
                                output = OutputKind.Text;
                                break;
                            case "json":
                                output = OutputKind.Json;
                                break;
                            default:
                                return ParseResult.Fail($"Unknown output '{value}'.", showUsage: true);
                        }

                        break;

                    default:
                        return ParseResult.Fail($"Unknown option '{name}'.", showUsage: true);
                }

                continue;
            }

            patterns.Add(arg);
        }

        if (patterns.Count == 0)
        {
            return ParseResult.Fail("Missing prescription file or pattern.", showUsage: true);
        }

        var options = new AppOptions(asOfDate, textFormat, output, patterns);
        return ParseResult.Ok(options);
    }

    private static (string Name, string? Value) SplitOption(string argument)
    {
        var separatorIndex = argument.IndexOf('=');
        if (separatorIndex >= 0)
        {
            var name = argument[..separatorIndex];
            var value = argument[(separatorIndex + 1)..];
            return (name, value);
        }

        return (argument, null);
    }
}

internal sealed record ParseResult(AppOptions? Options, bool IsSuccess, bool RequestHelp, bool ShowUsage, string? Error, int ExitCode)
{
    public static ParseResult Ok(AppOptions options) => new(options, true, false, false, null, 0);

    public static ParseResult Fail(string message, bool showUsage, int exitCode = 1)
        => new(null, false, false, showUsage, message, exitCode);

    public static ParseResult Help() => new(null, false, true, false, null, 0);
}

internal sealed class ThrowingErrorListener : IAntlrErrorListener<int>, IAntlrErrorListener<IToken>
{
    public static readonly ThrowingErrorListener Instance = new();

    public void SyntaxError(TextWriter output, IRecognizer recognizer, IToken offendingSymbol, int line, int charPositionInLine, string msg, RecognitionException e)
    {
        throw new InvalidOperationException($"Parse error at {line}:{charPositionInLine} - {msg}");
    }

    public void SyntaxError(TextWriter output, IRecognizer recognizer, int offendingSymbol, int line, int charPositionInLine, string msg, RecognitionException e)
    {
        throw new InvalidOperationException($"Parse error at {line}:{charPositionInLine} - {msg}");
    }
}

internal static class ReportFormatter
{
    private const string Separator = "------------------------------------------------------------------------------";

    public static void WriteText(IReadOnlyList<InventoryReport> reports, AppOptions options)
    {
        Console.WriteLine($"Report Date: {options.AsOfDate:yyyy-MM-dd}");
        Console.WriteLine($"Prescriptions Tracked: {reports.Count}");
        Console.WriteLine();

        if (options.TextFormat == TextFormat.Concise)
        {
            WriteConcise(reports);
        }
        else
        {
            foreach (var report in reports)
            {
                WriteDetailed(report);
                Console.WriteLine();
            }
        }
    }

    public static void WriteJson(IReadOnlyList<InventoryReport> reports, AppOptions options)
    {
        var serializerOptions = new JsonSerializerOptions
        {
            WriteIndented = true
        };

        serializerOptions.Converters.Add(new JsonStringEnumConverter());
        Console.WriteLine(JsonSerializer.Serialize(reports, serializerOptions));
    }

    private static void WriteConcise(IReadOnlyList<InventoryReport> reports)
    {
        Console.WriteLine("Drug                          Remaining        Daily Avg        Refill By    Status");
        Console.WriteLine(Separator);

        foreach (var report in reports)
        {
            var statusLabel = BuildStatusLabel(report);
            var refillDate = report.EstimatedDepletionDate?.ToString("yyyy-MM-dd") ?? "--";
            var remaining = $"{report.CurrentInventory,8:F1} {report.Unit}";
            var daily = report.DailyUsageRate > 0
                ? $"{report.DailyUsageRate,8:F2} {report.Unit}/d"
                : "    --";

            Console.WriteLine($"{report.DrugName,-30} {remaining,-16} {daily,-16} {refillDate,12} {statusLabel}");
            if (!string.IsNullOrEmpty(report.SourceFile))
            {
                Console.WriteLine($"    {Path.GetFileName(report.SourceFile)}");
            }
        }
    }

    private static void WriteDetailed(InventoryReport report)
    {
        var statusLabel = BuildStatusLabel(report);
        Console.WriteLine($"{report.DrugName} ({report.Type})");
        if (!string.IsNullOrEmpty(report.SourceFile))
        {
            Console.WriteLine($"File: {report.SourceFile}");
        }

        Console.WriteLine($"Remaining: {report.CurrentInventory:F2} {report.Unit}");
        Console.WriteLine($"Daily Average: {report.DailyUsageRate:F2} {report.Unit}/day");

        if (report.EstimatedDepletionDate.HasValue)
        {
            var daysUntil = Math.Max(0, (report.EstimatedDepletionDate.Value.Date - report.AsOfDate.Date).Days);
            Console.WriteLine($"Refill By: {report.EstimatedDepletionDate.Value:yyyy-MM-dd} ({daysUntil} days) {statusLabel}");
        }
        else
        {
            Console.WriteLine("Refill By: --");
        }

        Console.WriteLine($"Usage Anchor: {report.UsageAnchorDate:yyyy-MM-dd}");
        if (report.DeclaredStartDate.HasValue)
        {
            Console.WriteLine($"Declared Start: {report.DeclaredStartDate.Value:yyyy-MM-dd}");
        }

        if (report.LegacyDailyUsageRate > 0)
        {
            Console.WriteLine($"Legacy Rate: {report.LegacyDailyUsageRate:F2} {report.Unit}/day");
        }

        if (report.ScheduledDailyUsageRate > 0)
        {
            Console.WriteLine($"Scheduled Avg: {report.ScheduledDailyUsageRate:F2} {report.Unit}/day");
        }

        if (report.Schedules.Count > 0)
        {
            Console.WriteLine("Schedules:");
            foreach (var schedule in report.Schedules)
            {
                var when = schedule.Period switch
                {
                    Period.Day => "daily",
                    Period.Week => DescribeWeek(schedule),
                    Period.Month => DescribeMonth(schedule),
                    _ => schedule.Period.ToString()
                };

                var time = schedule.TimeOfDay.HasValue
                    ? $" at {schedule.TimeOfDay.Value:hh\\:mm}"
                    : string.Empty;

                Console.WriteLine($"  - {schedule.Amount} {report.Unit} per {schedule.Period}{when}{time}");
            }
        }
    }

    private static string DescribeWeek(DoseSchedule schedule)
    {
        if (schedule.DaysOfWeek.Count == 0)
        {
            return " (weekly)";
        }

        var days = string.Join(", ", schedule.DaysOfWeek.Select(d => d.ToString()));
        return $" on {days}";
    }

    private static string DescribeMonth(DoseSchedule schedule)
    {
        if (schedule.DaysOfMonth.Count == 0)
        {
            return " on start day";
        }

        var days = string.Join(", ", schedule.DaysOfMonth);
        return $" on day {days}";
    }

    private static string BuildStatusLabel(InventoryReport report)
    {
        if (report.CurrentInventory <= 0)
        {
            return "⚠️  DEPLETED";
        }

        if (!report.EstimatedDepletionDate.HasValue)
        {
            return string.Empty;
        }

        var daysUntil = (report.EstimatedDepletionDate.Value.Date - report.AsOfDate.Date).Days;
        if (daysUntil <= 7)
        {
            return "⚠️  URGENT";
        }

        if (daysUntil <= 30)
        {
            return "⚠️  SOON";
        }

        return string.Empty;
    }
}
