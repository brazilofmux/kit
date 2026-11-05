using System;
using System.Globalization;
using System.Linq;
using PrescriptionTracker.Models;

namespace PrescriptionTracker.Services;

public class InventoryCalculator
{
    public InventoryReport CalculateInventory(Drug drug, DateTime asOfDate)
    {
        var timeZone = ResolveTimeZone(drug.TimeZoneId);
        var asOfLocal = DateTime.SpecifyKind(asOfDate.Date.AddDays(1).AddTicks(-1), DateTimeKind.Unspecified);
        var startLocal = DetermineStart(drug, asOfLocal);

        var purchasesTotal = drug.Purchases
            .Where(p => p.Date <= asOfLocal)
            .OrderBy(p => p.Date)
            .Sum(p => p.Amount);

        var adjustmentsTotal = drug.Adjustments
            .Where(a => a.Date <= asOfLocal)
            .OrderBy(a => a.Date)
            .Sum(a => a.Amount);

        var legacyDailyRate = CalculateLegacyDailyRate(drug);
        var legacyUsage = CalculateLegacyUsage(startLocal, asOfLocal, legacyDailyRate);
        var scheduledUsage = CalculateScheduledUsage(drug, startLocal, asOfLocal);

        var currentInventory = drug.InitialAmount + purchasesTotal + adjustmentsTotal - legacyUsage - scheduledUsage;

        var scheduledDailyRate = CalculateScheduledDailyRate(drug);
        var totalDailyRate = legacyDailyRate + scheduledDailyRate;

        var depletionDate = EstimateDepletionDate(drug, asOfLocal, currentInventory, legacyDailyRate);

        return new InventoryReport
        {
            DrugName = drug.Name,
            AsOfDate = asOfDate,
            CurrentInventory = currentInventory,
            DailyUsageRate = totalDailyRate,
            EstimatedDepletionDate = depletionDate,
            Unit = FormatUnit(drug.Unit),
            TimeZoneId = timeZone.Id,
            DeclaredStartDate = drug.StartDate,
            UsageAnchorDate = startLocal,
            LegacyDailyUsageRate = legacyDailyRate,
            ScheduledDailyUsageRate = scheduledDailyRate,
            TotalLegacyUsage = legacyUsage,
            TotalScheduledUsage = scheduledUsage,
            Schedules = CloneSchedules(drug),
            Type = drug.Type
        };
    }

    private static TimeZoneInfo ResolveTimeZone(string? timeZoneId)
    {
        if (string.IsNullOrWhiteSpace(timeZoneId))
        {
            return TimeZoneInfo.Local;
        }

        try
        {
            return TimeZoneInfo.FindSystemTimeZoneById(timeZoneId);
        }
        catch (TimeZoneNotFoundException)
        {
            return ResolveFallbackTimeZone(timeZoneId);
        }
        catch (InvalidTimeZoneException)
        {
            return ResolveFallbackTimeZone(timeZoneId);
        }
    }

    private static TimeZoneInfo ResolveFallbackTimeZone(string timeZoneId)
    {
        if (OperatingSystem.IsWindows() && TimeZoneInfo.TryConvertIanaIdToWindowsId(timeZoneId, out var windowsId))
        {
            return TimeZoneInfo.FindSystemTimeZoneById(windowsId);
        }

        return TimeZoneInfo.Local;
    }

    private static DateTime DetermineStart(Drug drug, DateTime asOfLocal)
    {
        if (drug.StartDate.HasValue)
        {
            return DateTime.SpecifyKind(drug.StartDate.Value, DateTimeKind.Unspecified);
        }

        if (drug.Purchases.Any())
        {
            var earliestPurchase = drug.Purchases.Min(p => p.Date);
            return DateTime.SpecifyKind(earliestPurchase, DateTimeKind.Unspecified);
        }

        // Fallback to previous behavior: assume regimen started 30 days prior if nothing else is known.
        return DateTime.SpecifyKind(asOfLocal.AddDays(-30), DateTimeKind.Unspecified);
    }

    private static double CalculateLegacyUsage(DateTime startLocal, DateTime asOfLocal, double legacyDailyRate)
    {
        if (legacyDailyRate <= 0)
        {
            return 0;
        }

        var totalSpan = asOfLocal - startLocal;
        var daysElapsed = Math.Max(0, totalSpan.TotalDays);
        return legacyDailyRate * daysElapsed;
    }

    private static double CalculateScheduledUsage(Drug drug, DateTime startLocal, DateTime asOfLocal)
    {
        if (!drug.Schedules.Any())
        {
            return 0;
        }

        var startDate = DateOnly.FromDateTime(startLocal);
        var endDate = DateOnly.FromDateTime(asOfLocal);

        if (endDate < startDate)
        {
            return 0;
        }

        double total = 0;
        for (var date = startDate; date <= endDate; date = date.AddDays(1))
        {
            total += CalculateScheduledUsageForDate(drug, date, startLocal, asOfLocal, includeBoundary: true);
        }

        return total;
    }

    private static double CalculateScheduledUsageForDate(
        Drug drug,
        DateOnly date,
        DateTime scheduleAnchor,
        DateTime asOfLocal,
        bool includeBoundary)
    {
        if (!drug.Schedules.Any())
        {
            return 0;
        }

        double total = 0;
        foreach (var schedule in drug.Schedules)
        {
            if (!OccursOn(schedule, date, scheduleAnchor))
            {
                continue;
            }

            var eventTime = schedule.TimeOfDay.HasValue
                ? date.ToDateTime(TimeOnly.FromTimeSpan(schedule.TimeOfDay.Value))
                : date.ToDateTime(TimeOnly.MinValue);

            if (!includeBoundary && eventTime <= asOfLocal)
            {
                continue;
            }

            if (eventTime < scheduleAnchor)
            {
                continue;
            }

            if (includeBoundary && eventTime > asOfLocal)
            {
                continue;
            }

            total += schedule.Amount;
        }

        return total;
    }

    private static bool OccursOn(DoseSchedule schedule, DateOnly date, DateTime anchor)
    {
        return schedule.Period switch
        {
            Period.Day => true,
            Period.Week => OccursOnWeek(schedule, date, anchor),
            Period.Month => OccursOnMonth(schedule, date, anchor),
            _ => false
        };
    }

    private static bool OccursOnWeek(DoseSchedule schedule, DateOnly date, DateTime anchor)
    {
        if (schedule.DaysOfWeek.Count == 0)
        {
            return date.DayOfWeek == anchor.DayOfWeek;
        }

        return schedule.DaysOfWeek.Contains(date.DayOfWeek);
    }

    private static bool OccursOnMonth(DoseSchedule schedule, DateOnly date, DateTime anchor)
    {
        if (schedule.DaysOfMonth.Count == 0)
        {
            var anchorDay = Math.Min(anchor.Day, DateTime.DaysInMonth(date.Year, date.Month));
            return date.Day == anchorDay;
        }

        return schedule.DaysOfMonth.Contains(date.Day);
    }

    private static double CalculateLegacyDailyRate(Drug drug)
    {
        return drug.RatePeriod switch
        {
            Period.Day => drug.RateAmount,
            Period.Week => drug.RateAmount / 7.0,
            Period.Month => drug.RateAmount / 30.0,
            _ => 0
        };
    }

    private static double CalculateScheduledDailyRate(Drug drug)
    {
        if (!drug.Schedules.Any())
        {
            return 0;
        }

        double total = 0;
        foreach (var schedule in drug.Schedules)
        {
            total += schedule.Period switch
            {
                Period.Day => schedule.Amount,
                Period.Week => schedule.Amount * Math.Max(schedule.DaysOfWeek.Count, 1) / 7.0,
                Period.Month => schedule.Amount * Math.Max(schedule.DaysOfMonth.Count, 1) / 30.0,
                _ => 0
            };
        }

        return total;
    }

    private static DateTime? EstimateDepletionDate(
        Drug drug,
        DateTime asOfLocal,
        double currentInventory,
        double legacyDailyRate)
    {
        var scheduledDailyRate = CalculateScheduledDailyRate(drug);
        var averageDailyRate = legacyDailyRate + scheduledDailyRate;

        if (currentInventory <= 0)
        {
            return asOfLocal;
        }

        if (averageDailyRate <= 0)
        {
            return null;
        }

        var projectedInventory = currentInventory;
        var cursorDate = DateOnly.FromDateTime(asOfLocal);
        var anchor = DetermineStart(drug, asOfLocal);
        var safetyCounter = 0;

        while (projectedInventory > 0 && safetyCounter < 366 * 10)
        {
            cursorDate = cursorDate.AddDays(1);
            if (cursorDate.ToDateTime(TimeOnly.MinValue) >= anchor)
            {
                projectedInventory -= legacyDailyRate;
            }
            projectedInventory -= CalculateScheduledUsageForDate(drug, cursorDate, anchor, asOfLocal, includeBoundary: false);
            safetyCounter++;
        }

        if (projectedInventory > 0)
        {
            return null;
        }

        var depletionDate = cursorDate.ToDateTime(TimeOnly.MaxValue);
        return DateTime.SpecifyKind(depletionDate, DateTimeKind.Unspecified);
    }
    private static IReadOnlyList<DoseSchedule> CloneSchedules(Drug drug)
    {
        if (!drug.Schedules.Any())
        {
            return Array.Empty<DoseSchedule>();
        }

        return drug.Schedules
            .Select(schedule => new DoseSchedule
            {
                Amount = schedule.Amount,
                Period = schedule.Period,
                DaysOfWeek = new List<DayOfWeek>(schedule.DaysOfWeek),
                DaysOfMonth = new List<int>(schedule.DaysOfMonth),
                TimeOfDay = schedule.TimeOfDay
            })
            .ToList();
    }

    private static string FormatUnit(UnitType? unit)
    {
        if (!unit.HasValue)
        {
            return "units";
        }

        return unit.Value switch
        {
            UnitType.Mg => "mg",
            UnitType.Ml => "ml",
            UnitType.Pills => "pills",
            UnitType.Units => "units",
            UnitType.Capsules => "capsules",
            UnitType.Drops => "drops",
            UnitType.Mcg => "mcg",
            _ => unit.Value.ToString().ToLowerInvariant()
        };
    }
}

public class InventoryReport
{
    public string DrugName { get; set; } = string.Empty;
    public string? SourceFile { get; set; }
    public DrugType Type { get; set; }
    public DateTime AsOfDate { get; set; }
    public DateTime? DeclaredStartDate { get; set; }
    public DateTime UsageAnchorDate { get; set; }
    public double CurrentInventory { get; set; }
    public double DailyUsageRate { get; set; }
    public double LegacyDailyUsageRate { get; set; }
    public double ScheduledDailyUsageRate { get; set; }
    public double TotalLegacyUsage { get; set; }
    public double TotalScheduledUsage { get; set; }
    public DateTime? EstimatedDepletionDate { get; set; }
    public string Unit { get; set; } = string.Empty;
    public string? TimeZoneId { get; set; }
    public IReadOnlyList<DoseSchedule> Schedules { get; set; } = Array.Empty<DoseSchedule>();

    public override string ToString()
    {
        return $"{DrugName}: {CurrentInventory:F2} {Unit} remaining (daily avg {DailyUsageRate:F2} {Unit}/day)";
    }
}
