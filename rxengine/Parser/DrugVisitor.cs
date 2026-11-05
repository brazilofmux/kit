using System.Globalization;
using Antlr4.Runtime.Misc;
using PrescriptionTracker.Models;

namespace PrescriptionTracker.Parser;

public class DrugVisitor : PrescriptionBaseVisitor<object?>
{
    private readonly List<Drug> _drugs = new();
    private Drug? _currentDrug;

    public IReadOnlyList<Drug> GetDrugs()
    {
        if (_drugs.Count == 0)
            throw new InvalidOperationException("No drug has been defined");
        return _drugs;
    }

    public override object? VisitDrugDefinition([NotNull] PrescriptionParser.DrugDefinitionContext context)
    {
        var name = context.STRING().GetText().Trim('"');
        var typeText = context.drugType().GetText();
        var type = ParseDrugType(typeText);
        var initialAmount = double.Parse(context.NUMBER().GetText(), CultureInfo.InvariantCulture);

        UnitType? unit = null;
        if (context.unit() != null)
        {
            var unitText = context.unit().unitType().GetText();
            unit = ParseUnitType(unitText);
        }

        var drug = new Drug
        {
            Name = name,
            Type = type,
            InitialAmount = initialAmount,
            Unit = unit
        };

        _currentDrug = drug;
        _drugs.Add(drug);

        return null;
    }

    public override object? VisitRateDefinition([NotNull] PrescriptionParser.RateDefinitionContext context)
    {
        var drug = EnsureCurrentDrug();

        var rateAmount = double.Parse(context.NUMBER().GetText(), CultureInfo.InvariantCulture);
        var periodText = context.period().GetText();
        var period = ParsePeriod(periodText);

        drug.RateAmount = rateAmount;
        drug.RatePeriod = period;

        return null;
    }

    public override object? VisitDoseDefinition([NotNull] PrescriptionParser.DoseDefinitionContext context)
    {
        var drug = EnsureCurrentDrug();

        var amount = double.Parse(context.NUMBER().GetText(), CultureInfo.InvariantCulture);
        var period = ParsePeriod(context.period().GetText());

        var schedule = new DoseSchedule
        {
            Amount = amount,
            Period = period
        };

        if (context.unit() != null)
        {
            var unitText = context.unit().unitType().GetText();
            var parsedUnit = ParseUnitType(unitText);
            if (parsedUnit.HasValue)
            {
                // Prefer explicit unit declarations when the drug has none yet.
                drug.Unit ??= parsedUnit;
            }
        }

        var options = context.doseOptions();
        if (options != null)
        {
            var specifiers = options.doseSpecifier();
            if (specifiers != null && specifiers.Length > 0)
            {
                if (period == Period.Week)
                {
                    foreach (var spec in specifiers)
                    {
                        if (spec.DAY_OF_WEEK() != null)
                        {
                            var day = ParseDayOfWeek(spec.DAY_OF_WEEK().GetText());
                            schedule.DaysOfWeek.Add(day);
                        }
                        else if (spec.NUMBER() != null)
                        {
                            var ordinal = int.Parse(spec.NUMBER().GetText(), CultureInfo.InvariantCulture);
                            // Interpret numeric specifiers for weekly periods as ISO weekday numbers (1=Monday).
                            var day = (DayOfWeek)(((ordinal - 1) % 7 + 7) % 7);
                            schedule.DaysOfWeek.Add(day);
                        }
                    }
                }
                else if (period == Period.Month)
                {
                    foreach (var spec in specifiers)
                    {
                        if (spec.NUMBER() == null)
                            throw new InvalidOperationException("Monthly schedules require numeric day specifiers (1-31).");

                        var dayOfMonth = int.Parse(spec.NUMBER().GetText(), CultureInfo.InvariantCulture);
                        schedule.DaysOfMonth.Add(dayOfMonth);
                    }
                }
            }

            var time = options.time();
            if (time != null)
            {
                var hour = int.Parse(time.NUMBER(0).GetText(), CultureInfo.InvariantCulture);
                var minute = int.Parse(time.NUMBER(1).GetText(), CultureInfo.InvariantCulture);
                schedule.TimeOfDay = new TimeSpan(hour, minute, 0);
            }
        }

        drug.Schedules.Add(schedule);
        return null;
    }

    public override object? VisitStartDefinition([NotNull] PrescriptionParser.StartDefinitionContext context)
    {
        var drug = EnsureCurrentDrug();

        var dateContext = context.date();
        var year = int.Parse(dateContext.NUMBER(0).GetText(), CultureInfo.InvariantCulture);
        var month = int.Parse(dateContext.NUMBER(1).GetText(), CultureInfo.InvariantCulture);
        var day = int.Parse(dateContext.NUMBER(2).GetText(), CultureInfo.InvariantCulture);

        if (context.time() != null)
        {
            var timeContext = context.time();
            var hour = int.Parse(timeContext.NUMBER(0).GetText(), CultureInfo.InvariantCulture);
            var minute = int.Parse(timeContext.NUMBER(1).GetText(), CultureInfo.InvariantCulture);
            drug.StartDate = new DateTime(year, month, day, hour, minute, 0);
        }
        else
        {
            drug.StartDate = new DateTime(year, month, day);
        }

        return null;
    }

    public override object? VisitTimezoneDefinition([NotNull] PrescriptionParser.TimezoneDefinitionContext context)
    {
        var drug = EnsureCurrentDrug();
        var timezone = context.STRING().GetText().Trim('"');
        drug.TimeZoneId = timezone;
        return null;
    }

    public override object? VisitPurchase([NotNull] PrescriptionParser.PurchaseContext context)
    {
        var drug = EnsureCurrentDrug();

        var dateContext = context.date();
        var year = int.Parse(dateContext.NUMBER(0).GetText(), CultureInfo.InvariantCulture);
        var month = int.Parse(dateContext.NUMBER(1).GetText(), CultureInfo.InvariantCulture);
        var day = int.Parse(dateContext.NUMBER(2).GetText(), CultureInfo.InvariantCulture);
        var date = new DateTime(year, month, day);

        var amount = double.Parse(context.NUMBER().GetText(), CultureInfo.InvariantCulture);

        drug.Purchases.Add(new Purchase
        {
            Date = date,
            Amount = amount
        });

        return null;
    }

    public override object? VisitAdjustment([NotNull] PrescriptionParser.AdjustmentContext context)
    {
        var drug = EnsureCurrentDrug();

        var dateContext = context.date();
        var year = int.Parse(dateContext.NUMBER(0).GetText(), CultureInfo.InvariantCulture);
        var month = int.Parse(dateContext.NUMBER(1).GetText(), CultureInfo.InvariantCulture);
        var day = int.Parse(dateContext.NUMBER(2).GetText(), CultureInfo.InvariantCulture);
        var date = new DateTime(year, month, day);

        var amount = double.Parse(context.NUMBER().GetText(), CultureInfo.InvariantCulture);
        var reason = context.STRING().GetText().Trim('"');

        drug.Adjustments.Add(new Adjustment
        {
            Date = date,
            Amount = amount,
            Reason = reason
        });

        return null;
    }

    private Drug EnsureCurrentDrug()
    {
        if (_currentDrug == null)
            throw new InvalidOperationException("Statements must follow a DRUG declaration.");
        return _currentDrug;
    }

    private static Period ParsePeriod(string periodText)
    {
        return periodText switch
        {
            "day" => Period.Day,
            "week" => Period.Week,
            "month" => Period.Month,
            _ => throw new InvalidOperationException($"Unknown period '{periodText}'")
        };
    }

    private static DrugType ParseDrugType(string typeText)
    {
        return typeText switch
        {
            "pills" => DrugType.Pills,
            "liquid" => DrugType.Liquid,
            "injection" => DrugType.Injection,
            "capsule" => DrugType.Capsule,
            "tablet" => DrugType.Tablet,
            "supplement" => DrugType.Supplement,
            "drops" => DrugType.Drops,
            "gel" => DrugType.Gel,
            "powder" => DrugType.Powder,
            _ => throw new InvalidOperationException($"Unknown drug type '{typeText}'")
        };
    }

    private static UnitType? ParseUnitType(string unitText)
    {
        return unitText switch
        {
            "mg" => UnitType.Mg,
            "ml" => UnitType.Ml,
            "pills" => UnitType.Pills,
            "units" => UnitType.Units,
            "capsules" => UnitType.Capsules,
            "drops" => UnitType.Drops,
            "mcg" => UnitType.Mcg,
            _ => null
        };
    }

    private static DayOfWeek ParseDayOfWeek(string text)
    {
        if (Enum.TryParse<DayOfWeek>(text, true, out var value))
        {
            return value;
        }

        throw new InvalidOperationException($"Unrecognized day of week '{text}'");
    }
}
