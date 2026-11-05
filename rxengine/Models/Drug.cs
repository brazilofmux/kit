namespace PrescriptionTracker.Models;

public enum DrugType
{
    Pills,
    Liquid,
    Injection,
    Capsule,
    Tablet,
    Supplement,
    Drops,
    Gel,
    Powder
}

public enum UnitType
{
    Mg,
    Ml,
    Pills,
    Units,
    Capsules,
    Drops,
    Mcg
}

public enum Period
{
    Day,
    Week,
    Month
}

public class Drug
{
    public string Name { get; set; } = string.Empty;
    public DrugType Type { get; set; }
    public double InitialAmount { get; set; }
    public UnitType? Unit { get; set; }
    public double RateAmount { get; set; }
    public Period RatePeriod { get; set; }
    public List<Purchase> Purchases { get; set; } = new();
    public List<Adjustment> Adjustments { get; set; } = new();
    public DateTime? StartDate { get; set; }
    public string? TimeZoneId { get; set; }
    public List<DoseSchedule> Schedules { get; set; } = new();
}

public class DoseSchedule
{
    public double Amount { get; set; }
    public Period Period { get; set; }
    public List<DayOfWeek> DaysOfWeek { get; set; } = new();
    public List<int> DaysOfMonth { get; set; } = new();
    public TimeSpan? TimeOfDay { get; set; }
}

public class Purchase
{
    public DateTime Date { get; set; }
    public double Amount { get; set; }
}

public class Adjustment
{
    public DateTime Date { get; set; }
    public double Amount { get; set; }
    public string Reason { get; set; } = string.Empty;
}
