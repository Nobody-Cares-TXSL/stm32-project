#!/usr/bin/env python3
"""
GB2312 safe editor: replace lines by line number.
All input/output via UTF-8; file I/O via GB2312.

Usage:
  python3 gb_edit.py <file> <line> <new_content_utf8>

  <line>: 1-based line number
  <new_content_utf8>: the replacement line (UTF-8 string)

Examples:
  # Replace line 88 with a C declaration + comment
  python3 gb_edit.py code/src/alarm.c 88 'static u16 mute_remain = 0;    /* 静音剩余 tick（10ms 单位） */'

  # Delete line 23 (pass empty string)
  python3 gb_edit.py code/src/flame_view.c 23 ''

Options:
  --multi <start> <end> <replacement_utf8>
    Replace lines start..end (inclusive) with a single new line.
    Pass empty string to delete those lines entirely.

  --insert <after_line> <content_utf8>
    Insert a new line after the specified line number.
"""

import sys
import os

def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        sys.exit(1)

    filepath = args[0]

    if args[1] == '--multi':
        start = int(args[2])
        end = int(args[3])
        replacement = args[4].encode('gb2312') if len(args) > 4 else b''
        do_multi_replace(filepath, start, end, replacement)
    elif args[1] == '--insert':
        after = int(args[2])
        content = args[3].encode('gb2312') if len(args) > 3 else b''
        do_insert(filepath, after, content)
    elif args[1] == '--show':
        show_only(filepath, args[2:])
    else:
        lineno = int(args[1])
        if len(args) < 3:
            show_only(filepath, [str(lineno)])
            print("\nError: no replacement content given. Use --show to view only.")
            sys.exit(1)
        new_content = args[2].encode('gb2312')
        do_replace(filepath, lineno, new_content)

def show_only(filepath, line_args):
    with open(filepath, 'rb') as f:
        lines = f.read().split(b'\n')
    for arg in line_args:
        if '-' in arg:
            parts = arg.split('-')
            start, end = int(parts[0]), int(parts[1])
            for i in range(start - 1, min(end, len(lines))):
                print(f"{i+1:4d}\t{lines[i].decode('gb2312', errors='replace')}")
        else:
            i = int(arg) - 1
            if 0 <= i < len(lines):
                print(f"{i+1:4d}\t{lines[i].decode('gb2312', errors='replace')}")

def do_replace(filepath, lineno, new_line_bytes):
    with open(filepath, 'rb') as f:
        lines = f.read().split(b'\n')

    if lineno < 1 or lineno > len(lines):
        print(f"Error: line {lineno} out of range (file has {len(lines)} lines)")
        sys.exit(1)

    old = lines[lineno - 1]
    print(f"L{lineno} OLD: {old.decode('gb2312', errors='replace')}")
    print(f"L{lineno} NEW: {new_line_bytes.decode('gb2312', errors='replace')}")

    lines[lineno - 1] = new_line_bytes

    with open(filepath, 'wb') as f:
        f.write(b'\n'.join(lines))
    print("OK")

def do_multi_replace(filepath, start, end, replacement):
    with open(filepath, 'rb') as f:
        lines = f.read().split(b'\n')

    if start < 1 or end > len(lines) or start > end:
        print(f"Error: range {start}-{end} out of range")
        sys.exit(1)

    count = end - start + 1
    removed = lines[start - 1:end]
    for i, r in enumerate(removed):
        print(f"L{start + i} DEL: {r.decode('gb2312', errors='replace')}")

    if replacement:
        print(f"L{start} NEW: {replacement.decode('gb2312', errors='replace')}")

    lines[start - 1:end] = [replacement] if replacement else []

    with open(filepath, 'wb') as f:
        f.write(b'\n'.join(lines))
    print(f"OK: replaced {count} line(s)")

def do_insert(filepath, after, content):
    with open(filepath, 'rb') as f:
        lines = f.read().split(b'\n')

    if after < 0 or after > len(lines):
        print(f"Error: line {after} out of range")
        sys.exit(1)

    print(f"After L{after} INS: {content.decode('gb2312', errors='replace')}")

    lines.insert(after, content)

    with open(filepath, 'wb') as f:
        f.write(b'\n'.join(lines))
    print("OK")

if __name__ == '__main__':
    main()
