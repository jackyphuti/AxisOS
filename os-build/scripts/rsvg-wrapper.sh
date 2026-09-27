#!/bin/bash
IN_FILE=""
OUT_FILE=""
ARGS=()

while [ $# -gt 0 ]; do
  case "$1" in
    --format)
      ARGS+=("-f" "$2")
      shift 2
      ;;
    --height)
      ARGS+=("-h" "$2")
      shift 2
      ;;
    --width)
      ARGS+=("-w" "$2")
      shift 2
      ;;
    *)
      if [ -z "$IN_FILE" ]; then
        IN_FILE="$1"
      else
        OUT_FILE="$1"
      fi
      shift
      ;;
  esac
done

if [ -n "$OUT_FILE" ]; then
  exec rsvg-convert "${ARGS[@]}" -o "$OUT_FILE" "$IN_FILE"
else
  exec rsvg-convert "${ARGS[@]}" "$IN_FILE"
fi
