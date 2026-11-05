using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

public class Validater
{
    private static readonly int[] itt =
    {
    //  0   1   2   3   4   5   6   7    8   9   A   B   C   D   E   F
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  1,  0,  0,  2,  0,  0,  // 0
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  // 1
        0,  0,  4,  0,  0,  0,  0,  0,   0,  0,  0,  0,  3,  0,  0,  0,  // 2
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  // 3
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  // 4
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  // 5
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  // 6
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  // 7

        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  // 8
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  // 9
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  // A
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  // B
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  // C
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  // D
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  // E
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  // F
    };

    private static readonly int[,] stt_loose =
    {
    //   Any  LF  CR   ,   "
        {  6, 10,  3,  8,  1 },  // State  0 - F0
        {  7,  7,  7,  7,  2 },  // State  1 - F2
        {  6, 11,  4,  8,  5 },  // State  2 - Q2
        { 12, 10, 12, 12, 12 },  // State  3 - CR0
        { 12, 11, 12, 12, 12 },  // State  4 - CR1
        {  7,  7,  7,  7,  2 },  // State  5 - Save Quote
        {  6, 11,  4,  8,  6 },  // State  6 - Save Byte
        {  7,  7,  7,  7,  2 },  // State  7 - Save Byte
        {  6, 11,  9,  8,  1 },  // State  8 - Save Field
        { 12, 10, 12, 12, 12 },  // State  9 - Save Field
        {  6, 10,  3,  8,  1 },  // State 10 - Save Record
        {  6, 10,  3,  8,  1 },  // State 11 - Save Field And Record
        { 12, 12, 12, 12, 12 },  // State 12 - Error
    };

    private static readonly int[,] stt_strict =
    {
    //   Any  LF  CR   ,   "
        {  6, 10,  3,  8,  1 },  // State  0 - F0
        {  7,  7,  7,  7,  2 },  // State  1 - F2
        { 12, 11,  4,  8,  5 },  // State  2 - Q2
        { 12, 10, 12, 12, 12 },  // State  3 - CR0
        { 12, 11, 12, 12, 12 },  // State  4 - CR1
        {  7,  7,  7,  7,  2 },  // State  5 - Save Quote
        {  6, 11,  4,  8, 12 },  // State  6 - Save Byte
        {  7,  7,  7,  7,  2 },  // State  7 - Save Byte
        {  6, 11,  9,  8,  1 },  // State  8 - Save Field
        { 12, 10, 12, 12, 12 },  // State  9 - Save Field
        {  6, 10,  3,  8,  1 },  // State 10 - Save Record
        {  6, 10,  3,  8,  1 },  // State 11 - Save Field And Record
        { 12, 12, 12, 12, 12 },  // State 12 - Error
    };

    private class FileResult
    {
        public string Filename { get; set; }
        public bool IsValid { get; set; }
        public int FieldsExpected { get; set; }
        public int LineNumber { get; set; }
        public List<string> Errors { get; set; } = new List<string>();
    }

    static async Task<int> Main(string[] args)
    {
        bool strict = false;
        bool verbose = false;
        int maxThreads = Environment.ProcessorCount;
        var filenames = new List<string>();

        for (int i = 0; i < args.Length; i++)
        {
            switch (args[i])
            {
                case "-s":
                case "--strict":
                    strict = true;
                    break;
                case "-v":
                case "--verbose":
                    verbose = true;
                    break;
                case "-t":
                case "--threads":
                    if (i + 1 < args.Length && int.TryParse(args[++i], out int threads))
                    {
                        maxThreads = threads;
                    }
                    break;
                case "-m":
                case "--memory":
                    // C# doesn't support memory mapping in the same way, ignored
                    break;
                default:
                    if (!args[i].StartsWith("-"))
                    {
                        filenames.Add(args[i]);
                    }
                    break;
            }
        }

        if (filenames.Count == 0)
        {
            Console.WriteLine("Usage: validatecsv [-s|--strict] [-v|--verbose] [-t|--threads N] <filename1> [filename2] ...");
            return 1;
        }

        var semaphore = new SemaphoreSlim(maxThreads, maxThreads);
        var tasks = new List<Task<FileResult>>();

        foreach (var filename in filenames)
        {
            tasks.Add(ValidateFileAsync(filename, strict, semaphore));
        }

        var results = await Task.WhenAll(tasks);

        bool allValid = true;
        foreach (var result in results)
        {
            if (!result.IsValid)
            {
                allValid = false;
                if (verbose)
                {
                    Console.WriteLine($"{result.Filename}: File is {(strict ? "strictly" : "loosely")} invalid with {result.FieldsExpected} columns and {result.LineNumber} rows.");
                    foreach (var error in result.Errors)
                    {
                        Console.WriteLine($"  {error}");
                    }
                }
            }
            else if (verbose)
            {
                Console.WriteLine($"{result.Filename}: File is {(strict ? "strictly" : "loosely")} valid with {result.FieldsExpected} columns and {result.LineNumber} rows.");
            }
        }

        return allValid ? 0 : 1;
    }

    private static async Task<FileResult> ValidateFileAsync(string filename, bool strict, SemaphoreSlim semaphore)
    {
        await semaphore.WaitAsync();
        try
        {
            return await Task.Run(() => ValidateFile(filename, strict));
        }
        finally
        {
            semaphore.Release();
        }
    }

    private static FileResult ValidateFile(string filename, bool strict)
    {
        var result = new FileResult
        {
            Filename = filename,
            IsValid = true,
            FieldsExpected = 0,
            LineNumber = 1
        };

        try
        {
            using (var fs = File.OpenRead(filename))
            using (var br = new BinaryReader(fs))
            {
                bool haveExpectedFields = false;
                int fields = 0;
                int[,] stt = strict ? stt_strict : stt_loose;
                int state = 0;
                long length = br.BaseStream.Length;
                long pos = 0;

                while (pos < length)
                {
                    byte b = br.ReadByte();
                    pos++;
                    int column = itt[b];
                    state = stt[state, column];
                    
                    if (state <= 7) continue;

                    if (8 <= state && state <= 11)
                    {
                        if (10 != state)
                        {
                            fields++;
                        }

                        if (10 <= state)
                        {
                            if (haveExpectedFields)
                            {
                                if (fields != result.FieldsExpected)
                                {
                                    result.IsValid = false;
                                    result.Errors.Add($"Line {result.LineNumber} contains {fields} fields instead of the expected {result.FieldsExpected} fields.");
                                }
                            }
                            else
                            {
                                result.FieldsExpected = fields;
                                haveExpectedFields = true;
                            }
                            fields = 0;
                            result.LineNumber++;
                        }
                    }
                    else if (12 == state)
                    {
                        result.IsValid = false;
                        result.Errors.Add($"Unexpected character '{(char)b}' (decimal {b}) on line {result.LineNumber}.");
                        break;
                    }
                }

                // End of input ends the last record, newline or not (RFC 4180 2.2).
                switch (state)
                {
                    case 2:   // after a closing quote: the field is complete
                    case 6:   // in an unquoted field
                    case 8:   // after a comma: an empty last field
                        fields++;
                        if (haveExpectedFields)
                        {
                            if (fields != result.FieldsExpected)
                            {
                                result.IsValid = false;
                                result.Errors.Add($"Line {result.LineNumber} contains {fields} fields instead of the expected {result.FieldsExpected} fields.");
                            }
                        }
                        else
                        {
                            result.FieldsExpected = fields;
                        }
                        result.LineNumber++;
                        break;
                    case 0:   // nothing read
                    case 10:  // the record already ended
                    case 11:
                    case 12:  // already in error
                        break;
                    default:  // 1, 5, 7: inside an open quote; 3, 4, 9: a CR with no LF
                        result.IsValid = false;
                        result.Errors.Add($"Unexpected end of file while parsing line {result.LineNumber}.");
                        break;
                }
            }
        }
        catch (Exception ex)
        {
            result.IsValid = false;
            result.Errors.Add($"Unable to open file: {ex.Message}");
        }

        return result;
    }
}