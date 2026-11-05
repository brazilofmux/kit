use std::fs::File;
use std::io::Read;
use std::sync::{Arc, Mutex};
use std::thread;
use memmap2::Mmap;
use clap::{Arg, Command};

const ITT: [usize; 256] = {
    let mut table = [0; 256];
    table[10] = 1;  // LF
    table[13] = 2;  // CR
    table[44] = 3;  // ','
    table[34] = 4;  // '"'
    table
};

const STT_LOOSE: [[usize; 5]; 13] = [
    [6, 10, 3, 8, 1],  // State  0 - F0
    [7, 7, 7, 7, 2],   // State  1 - F2
    [6, 11, 4, 8, 5],  // State  2 - Q2
    [12, 10, 12, 12, 12], // State  3 - CR0
    [12, 11, 12, 12, 12], // State  4 - CR1
    [7, 7, 7, 7, 2],   // State  5 - Save Quote
    [6, 11, 4, 8, 6],  // State  6 - Save Byte
    [7, 7, 7, 7, 2],   // State  7 - Save Byte
    [6, 11, 9, 8, 1],  // State  8 - Save Field
    [12, 10, 12, 12, 12], // State  9 - Save Field
    [6, 10, 3, 8, 1],  // State 10 - Save Record
    [6, 10, 3, 8, 1],  // State 11 - Save Field And Record
    [12, 12, 12, 12, 12], // State 12 - Error
];

const STT_STRICT: [[usize; 5]; 13] = [
    [6, 10, 3, 8, 1],  // State  0 - F0
    [7, 7, 7, 7, 2],   // State  1 - F2
    [12, 11, 4, 8, 5], // State  2 - Q2
    [12, 10, 12, 12, 12], // State  3 - CR0
    [12, 11, 12, 12, 12], // State  4 - CR1
    [7, 7, 7, 7, 2],   // State  5 - Save Quote
    [6, 11, 4, 8, 12], // State  6 - Save Byte
    [7, 7, 7, 7, 2],   // State  7 - Save Byte
    [6, 11, 9, 8, 1],  // State  8 - Save Field
    [12, 10, 12, 12, 12], // State  9 - Save Field
    [6, 10, 3, 8, 1],  // State 10 - Save Record
    [6, 10, 3, 8, 1],  // State 11 - Save Field And Record
    [12, 12, 12, 12, 12], // State 12 - Error
];

struct FileResult {
    filename: String,
    is_valid: bool,
    expected_fields: usize,
    line_number: usize,
    errors: Vec<String>,
}

fn validate_file(filename: &str, strict: bool, use_memory: bool) -> FileResult {
    if use_memory {
        validate_file_memory_mapped(filename, strict)
    } else {
        validate_file_stream(filename, strict)
    }
}

fn validate_file_memory_mapped(filename: &str, strict: bool) -> FileResult {
    let file = match File::open(filename) {
        Ok(file) => file,
        Err(e) => {
            return FileResult {
                filename: filename.to_string(),
                is_valid: false,
                expected_fields: 0,
                line_number: 0,
                errors: vec![format!("Unable to open file: {}", e)],
            };
        }
    };
    
    let mmap = match unsafe { Mmap::map(&file) } {
        Ok(mmap) => mmap,
        Err(e) => {
            return FileResult {
                filename: filename.to_string(),
                is_valid: false,
                expected_fields: 0,
                line_number: 0,
                errors: vec![format!("Failed to memory map file: {}", e)],
            };
        }
    };
    
    let data = &mmap[..];
    validate_data(filename, data, strict)
}

fn validate_file_stream(filename: &str, strict: bool) -> FileResult {
    let mut file = match File::open(filename) {
        Ok(file) => file,
        Err(e) => {
            return FileResult {
                filename: filename.to_string(),
                is_valid: false,
                expected_fields: 0,
                line_number: 0,
                errors: vec![format!("Unable to open file: {}", e)],
            };
        }
    };
    
    let mut buffer = Vec::new();
    if let Err(e) = file.read_to_end(&mut buffer) {
        return FileResult {
            filename: filename.to_string(),
            is_valid: false,
            expected_fields: 0,
            line_number: 0,
            errors: vec![format!("Unable to read file: {}", e)],
        };
    }
    
    validate_data(filename, &buffer, strict)
}

fn validate_data(filename: &str, data: &[u8], strict: bool) -> FileResult {
    let mut result = FileResult {
        filename: filename.to_string(),
        is_valid: true,
        expected_fields: 0,
        line_number: 1,
        errors: Vec::new(),
    };
    
    let mut have_expected_fields = false;
    let mut fields = 0;
    let mut state = 0;
    let transition_table = if strict { STT_STRICT } else { STT_LOOSE };
    
    for &byte in data {
        let column = ITT[byte as usize];
        state = transition_table[state][column];
        
        if state <= 7 {
            continue;
        }
        if (8..=11).contains(&state) {
            if state != 10 {
                fields += 1;
            }
            if state >= 10 {
                if have_expected_fields {
                    if fields != result.expected_fields {
                        result.errors.push(format!(
                            "Line {} contains {} fields instead of the expected {} fields.",
                            result.line_number, fields, result.expected_fields
                        ));
                        result.is_valid = false;
                    }
                } else {
                    result.expected_fields = fields;
                    have_expected_fields = true;
                }
                fields = 0;
                result.line_number += 1;
            }
        } else if state == 12 {
            result.errors.push(format!(
                "Unexpected character '{}' (decimal {}) on line {}.",
                byte as char, byte, result.line_number
            ));
            result.is_valid = false;
            break;
        }
    }

    // End of input ends the last record, newline or not (RFC 4180 2.2).
    match state {
        // after a closing quote, in an unquoted field, or after a comma
        2 | 6 | 8 => {
            fields += 1;
            if have_expected_fields {
                if fields != result.expected_fields {
                    result.errors.push(format!(
                        "Line {} contains {} fields instead of the expected {} fields.",
                        result.line_number, fields, result.expected_fields
                    ));
                    result.is_valid = false;
                }
            } else {
                result.expected_fields = fields;
            }
            result.line_number += 1;
        }
        // nothing read, the record already ended, or already in error
        0 | 10 | 11 | 12 => {}
        // 1, 5, 7: inside an open quote; 3, 4, 9: a CR with no LF
        _ => {
            result.errors.push(format!(
                "Unexpected end of file while parsing line {}.",
                result.line_number
            ));
            result.is_valid = false;
        }
    }

    result
}

fn validate_csv(filenames: Vec<String>, strict: bool, verbose: bool, use_memory: bool, max_threads: usize) -> bool {
    let results = Arc::new(Mutex::new(Vec::new()));
    
    // Process files in batches based on max_threads
    for chunk in filenames.chunks(max_threads) {
        let mut batch_handles: Vec<thread::JoinHandle<()>> = vec![];
        
        for filename in chunk {
            let filename = filename.clone();
            let results = Arc::clone(&results);
            
            let handle = thread::spawn(move || {
                let result = validate_file(&filename, strict, use_memory);
                let mut results = results.lock().unwrap();
                results.push(result);
            });
            
            batch_handles.push(handle);
        }
        
        for handle in batch_handles {
            handle.join().unwrap();
        }
    }

    let results = results.lock().unwrap();
    let mut is_valid = true;

    for result in results.iter() {
        if !result.is_valid {
            is_valid = false;
            if verbose {
                println!(
                    "{}: File is {} invalid with {} columns and {} rows.",
                    result.filename,
                    if strict { "strictly" } else { "loosely" },
                    result.expected_fields,
                    result.line_number
                );
                for error in &result.errors {
                    println!("  {}", error);
                }
            }
        } else if verbose {
            println!(
                "{}: File is {} valid with {} columns and {} rows.",
                result.filename,
                if strict { "strictly" } else { "loosely" },
                result.expected_fields,
                result.line_number
            );
        }
    }

    is_valid
}

fn main() {
    let matches = Command::new("validatecsv")
        .version("1.0")
        .about("CSV file validator")
        .arg(Arg::new("strict")
            .short('s')
            .long("strict")
            .help("Use strict validation mode")
            .action(clap::ArgAction::SetTrue))
        .arg(Arg::new("verbose")
            .short('v')
            .long("verbose")
            .help("Verbose output")
            .action(clap::ArgAction::SetTrue))
        .arg(Arg::new("memory")
            .short('m')
            .long("memory")
            .help("Use memory-mapped I/O")
            .action(clap::ArgAction::SetTrue))
        .arg(Arg::new("threads")
            .short('t')
            .long("threads")
            .value_name("N")
            .help("Number of threads to use"))
        .arg(Arg::new("files")
            .help("CSV files to validate")
            .required(true)
            .num_args(1..)
            .index(1))
        .get_matches();

    let strict = matches.get_flag("strict");
    let verbose = matches.get_flag("verbose");
    let use_memory = matches.get_flag("memory");
    let max_threads: usize = matches.get_one::<String>("threads")
        .and_then(|s| s.parse().ok())
        .unwrap_or_else(|| num_cpus::get());
    
    let filenames: Vec<String> = matches.get_many::<String>("files")
        .unwrap()
        .map(|s| s.to_string())
        .collect();

    let exit_code = if validate_csv(filenames, strict, verbose, use_memory, max_threads) { 0 } else { 1 };
    std::process::exit(exit_code);
}