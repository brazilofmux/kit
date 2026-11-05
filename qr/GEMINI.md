# QR Tape Project

This project implements the "QR Tape" design, a storage format for representing files as a sequence of QR codes, enabling forward/backward scanning, optional compression, encryption, and FEC.

## Project Structure

*   **`docs/`**: Documentation.
    *   `qr_tape_design.md`: The core design specification for the QR Tape format.
*   **`librs/`**: C++ library implementing Reed-Solomon error correction and CRC utilities.
    *   Contains its own `Makefile` to build `librs.a`.
*   **`AGENTS.md`**: Instructions/Conventions for AI agents.

## Building and Running

### Prerequisites

*   C++ Compiler (`g++`)
*   `ar` (for creating static libraries)

### Building `librs`

Navigate to the `librs` directory and run `make`:

```bash
cd librs
make
```

This will produce the `librs.a` static library.




## Development Conventions

*   **Language**: C++ (primary), with Markdown for documentation.
*   **Style**: The code uses standard C++ conventions. `librs` uses a Makefile.
*   **Design**: Follows the specification in `docs/qr_tape_design.md`.

## Next Steps / TODOs

*   Integrate `librs` with the main application logic if intended.

