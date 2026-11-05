#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GRAMMAR_PATH="$ROOT_DIR/Parser/Prescription.g4"
TARGET_DIR="$ROOT_DIR/Parser"
ANTLR_JAR="$ROOT_DIR/antlr-4.13.2-complete.jar"

if [[ ! -f "$GRAMMAR_PATH" ]]; then
  echo "Grammar not found at $GRAMMAR_PATH" >&2
  exit 1
fi

if [[ ! -f "$ANTLR_JAR" ]]; then
  cat >&2 <<MSG
Missing $ANTLR_JAR.
Download it from https://www.antlr.org/download/antlr-4.13.2-complete.jar
and place it at the repository root before rerunning this script.
MSG
  exit 1
fi

java -jar "$ANTLR_JAR" -Dlanguage=CSharp -visitor -o "$TARGET_DIR" "$GRAMMAR_PATH"
echo "Grammar regenerated in $TARGET_DIR"
