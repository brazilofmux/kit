using CsvHelper;
using MimeKit;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;

namespace parsemail
{
    class Program
    {
        static void Main(string[] args)
        {
            // Make data directory configurable (optional but good practice)
            var dataDirectory = Environment.GetEnvironmentVariable("DATA_DIR") ?? "/data";
            if (!Directory.Exists(dataDirectory))
            {
                Console.Error.WriteLine($"Error: Data directory '{dataDirectory}' not found.");
                return; // Or Environment.Exit(1);
            }
            Console.WriteLine($"Scanning directory: {dataDirectory}");

            // Use try-catch for robustness around file enumeration/processing
            try
            {
                foreach (var fn in Directory.EnumerateFiles(dataDirectory, "*.Kagura"))
                {
                    Console.WriteLine($"Processing file: {fn}");
                    try // Add try-catch for individual file processing
                    {
                        MimeMessage message;
                        // Use using for the file stream
                        using (var stream = File.OpenRead(fn))
                        {
                            // Consider using MimeParser for more control or async loading if needed
                            message = MimeMessage.Load(stream);
                        }

                        // Use UTC time for unambiguous naming
                        var timestamp = message.Date.UtcDateTime.ToString("yyyyMMddTHHmmssZ");
                        var counter = GetNextCount();
                        var basename = Path.Combine(dataDirectory, $"{timestamp}-{counter}"); // Output to same dir for simplicity, or configure output dir

                        // Ensure directory exists or handle creation failure
                        Directory.CreateDirectory(basename);
                        Console.WriteLine($"Created output directory: {basename}");

                        HandleHeaders(basename, message.Headers);
                        HandleMimeEntity(basename, message.Body);

                        Console.WriteLine($"Finished processing: {fn}");
                    }
                    catch (Exception fileEx)
                    {
                        Console.Error.WriteLine($"Error processing file '{fn}': {fileEx.Message}");
                        // Log the full exception details if needed for debugging:
                        // Console.Error.WriteLine(fileEx.ToString());
                        // Continue to the next file
                    }
                }
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"Error enumerating files in '{dataDirectory}': {ex.Message}");
                // Log the full exception details if needed:
                // Console.Error.WriteLine(ex.ToString());
            }

            Console.WriteLine("Processing complete.");
        }

        private static int _counter = 0;

        private static int GetNextCount()
        {
            // Consider Interlocked.Increment if threading is ever introduced
            return _counter++;
        }

        // --- Updated HandleHeaders using CsvHelper ---
        private static void HandleHeaders(string basename, HeaderList hl)
        {
            var fn = Path.Combine(basename, "headers.csv"); // Use standard extension

            // Define headers to skip (using case-insensitive comparison)
            var headersToSkip = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
            {
                "DKIM-Signature",
                "X-Google-DKIM-Signature",
                "X-Gm-Message-State",
                "X-Google-Smtp-Source"
                // Add other headers to skip here if needed
            };

            try
            {
                // Use StreamWriter with UTF8 encoding and CsvWriter
                using (var writer = new StreamWriter(fn, false, Encoding.UTF8)) // Use UTF-8!
                // Use InvariantCulture for consistency
                using (var csv = new CsvWriter(writer, CultureInfo.InvariantCulture))
                {
                    // Write the header row manually
                    csv.WriteField("Field");
                    csv.WriteField("Value");
                    csv.NextRecord(); // Move to the next line

                    // Iterate through MimeKit headers
                    foreach (var h in hl)
                    {
                        // Skip specified headers (case-insensitive)
                        if (headersToSkip.Contains(h.Field))
                        {
                            continue;
                        }

                        // Write the Field and Value for the current header
                        csv.WriteField(h.Field); // Keep original Field name (Unicode preserved)
                        csv.WriteField(h.Value); // Keep original Value (Unicode preserved)
                        csv.NextRecord(); // Move to the next line for the next header
                    }
                } // CsvWriter and StreamWriter are automatically flushed and disposed here
                Console.WriteLine($"Successfully wrote headers to: {fn}");
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"Error writing headers CSV file '{fn}': {ex.Message}");
                // Optional: Attempt to clean up partially written file
                try { if (File.Exists(fn)) File.Delete(fn); } catch { /* Ignore delete error */ }
            }
        }


        // --- HandleMimeEntity - unchanged from original, but added error handling ---
        private static void HandleMimeEntity(string basename, MimeEntity entity)
        {
            // Basic recursive structure - consider adding depth limit for safety?
            switch (entity)
            {
                case Multipart multipart:
                    Console.WriteLine($"Handling Multipart ({multipart.ContentType.MimeType}), Count: {multipart.Count}");
                    foreach (var subEntity in multipart)
                    {
                        // Recursive call
                        HandleMimeEntity(basename, subEntity);
                    }
                    break;

                case MessagePart rfc822:
                    Console.WriteLine("Handling MessagePart (RFC822 embedded message)");
                    // Note: Embedded messages might have their own headers you want to capture separately?
                    // Current logic just processes the body of the embedded message.
                    if (rfc822.Message != null && rfc822.Message.Body != null)
                    {
                        HandleMimeEntity(basename, rfc822.Message.Body);
                    }
                    else
                    {
                        Console.WriteLine("Skipping empty or invalid MessagePart.");
                    }
                    break;

                case MimePart part: // Handles TextPart, ApplicationPart, etc.
                    try
                    {
                        // Sanitize filename - remove invalid path characters
                        var rawFileName = part.FileName;
                        var sanitizedFileName = string.IsNullOrWhiteSpace(rawFileName)
                            ? $"{GetNextCount()}" // Default name if FileName is missing
                            : SanitizeFileName(rawFileName); // Sanitize provided name

                        var path = Path.Combine(basename, sanitizedFileName);

                        // Try to get a better extension
                        if (MimeTypes.TryGetExtension(part.ContentType.MimeType, out string extension))
                        {
                            // Ensure extension starts with a dot if not already present
                            if (!extension.StartsWith(".")) extension = "." + extension;
                            // Avoid double dots if filename already has one
                            if (!Path.HasExtension(path) || Path.GetExtension(path).ToLowerInvariant() != extension.ToLowerInvariant())
                            {
                                path = Path.ChangeExtension(path, extension);
                            }
                        }
                        else if (!Path.HasExtension(path)) // Add a default if none found
                        {
                            path = Path.ChangeExtension(path, ".dat"); // Or .bin, .unknown
                        }

                        // Handle filename conflicts by adding a numeric suffix
                        path = GetUniqueFilePath(path);

                        Console.WriteLine($"Saving part: Type={part.ContentType.MimeType}, Output={path}");

                        using (var stream = File.Create(path))
                        {
                            part.Content.DecodeTo(stream);
                        }
                    }
                    catch (Exception ex)
                    {
                        Console.Error.WriteLine($"Error saving MimePart (FileName: {part.FileName ?? "N/A"}): {ex.Message}");
                        // Log full ex if needed
                    }
                    break;

                default:
                    Console.WriteLine($"Skipping unknown MimeEntity type: {entity.GetType().Name}");
                    break;
            }
        }

        // Helper function to remove invalid characters from potential filenames
        private static string SanitizeFileName(string fileName)
        {
            var invalidChars = Path.GetInvalidFileNameChars();
            // Replace invalid chars with an underscore, or simply remove them
            // Using string.Join avoids repeated allocations of StringBuilder
            return string.Join("_", fileName.Split(invalidChars, StringSplitOptions.RemoveEmptyEntries)).Trim();
        }

        // Helper function to ensure unique filenames by adding a numeric suffix if needed
        private static string GetUniqueFilePath(string filePath)
        {
            if (!File.Exists(filePath))
            {
                return filePath; // File doesn't exist, no need for a unique name
            }

            // Get directory and file info
            string directory = Path.GetDirectoryName(filePath);
            string filenameWithoutExtension = Path.GetFileNameWithoutExtension(filePath);
            string extension = Path.GetExtension(filePath);

            int counter = 1;
            string newFilePath;

            // Keep incrementing counter until we find a unique filename
            do
            {
                newFilePath = Path.Combine(directory, $"{filenameWithoutExtension}_{counter}{extension}");
                counter++;
            } while (File.Exists(newFilePath));

            return newFilePath;
        }
    }
}
