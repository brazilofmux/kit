# Repository Guidelines

This repository is currently a new, empty project (only `.git` is present). The conventions below define the expected structure and workflows as the codebase is created. Update this document once the first modules and tooling are added.

## Project Structure & Module Organization

- Source code should live in `src/` with modules grouped by domain (for example, `src/auth/`, `src/api/`).
- Tests should live in `tests/` and mirror the source layout (`tests/auth/`, `tests/api/`).
- Assets (fixtures, images, data files) should go in `assets/` or `fixtures/` with clear naming.

## Build, Test, and Development Commands

No build or test commands are defined yet. When adding tooling, document the exact commands here. Examples to standardize on:

- `make build` or `npm run build` for production builds.
- `make test` or `npm test` for the full test suite.
- `make lint` or `npm run lint` for linting.

## Coding Style & Naming Conventions

- Use 2 spaces for indentation in configuration files and 4 spaces in application code unless the chosen language style guide requires otherwise.
- Prefer `snake_case` for file names, `PascalCase` for types/classes, and `camelCase` for functions and variables.
- Add formatting and linting tools early (for example, `prettier`, `ruff`, `golangci-lint`) and run them in CI.

## Testing Guidelines

- Pick a single testing framework per language and document it (for example, `pytest`, `jest`, `go test`).
- Name test files with a `_test` suffix (`user_service_test.py`, `user.service.test.ts`).
- Aim for meaningful coverage of core paths; add regression tests for every bug fix.

## Commit & Pull Request Guidelines

- No commit history exists yet; use Conventional Commits (`feat:`, `fix:`, `docs:`) to establish a baseline.
- Pull requests should include a clear summary, testing notes, and any relevant issue links.
- Include screenshots or logs when changes affect user-facing behavior or diagnostics.

## Security & Configuration Tips

- Store secrets in environment variables; never commit them to the repo.
- Document required env vars in a `.env.example` once configuration is introduced.
