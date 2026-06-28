#!/usr/bin/env python3
"""
GB2312 regex replace (avoids re.sub replacement escape pitfall).

Why this exists:
  re.sub(pattern, repl, text) interprets backslashes in `repl` when it is a
  bytes/str — a literal \\r (0x5c 0x72) becomes a real CR (0x0d), \\n becomes LF.
  Writing a C-source "\\r\\n" literal via re.sub corrupts it into real newlines,
  breaking the string literal across lines (compile error).

  This tool uses a FUNCTION replacement (lambda m: repl) whose return value is
  NOT escape-interpreted, so literal backslashes are written verbatim.

  Prefer gb_edit.py (line-number edits, pure bytes ops, no such pitfall) when
  you know the line numbers. Use this for pattern-matched / unknown-line edits.

Usage (replacement is read from stdin as raw bytes):
  printf '%s' 'printf("[ALARM] led=%d\\r\\n", x);' | \\
      python3 gb_regex_replace.py code/src/alarm.c 'printf\\("\\[ALARM\\].*?\\);'

  - printf '%s' does NOT interpret \\r\\n (keeps them literal). Do NOT use %b.
  - Quote the regex with single quotes; escape ( ) [ ] . * as needed.

Validation: re-decodes the file as GB2312 after writing; corrupt edits raise.
"""

import sys
import re


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    filepath = sys.argv[1]
    pattern = sys.argv[2].encode('latin-1')   # regex as bytes
    repl = sys.stdin.buffer.read()            # replacement: raw bytes from stdin

    with open(filepath, 'rb') as f:
        raw = f.read()

    pat = re.compile(pattern, re.DOTALL)
    n = len(pat.findall(raw))
    if n == 0:
        print("Error: pattern not found in %s" % filepath)
        sys.exit(1)

    new = pat.sub(lambda m: repl, raw)        # function replacement: no escape interp
    with open(filepath, 'wb') as f:
        f.write(new)

    with open(filepath, 'rb') as f:
        f.read().decode('gb2312')             # encoding round-trip check
    print("OK: replaced %d match(es) in %s" % (n, filepath))


if __name__ == '__main__':
    main()
