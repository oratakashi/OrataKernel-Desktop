#!/usr/bin/env python3
"""
Resolve committed conflict markers in source files.
Strategy: take HEAD (ours/stagging) by default.
For security-sensitive paths, keep BOTH sections merged.
"""

import os
import re
import sys

SECURITY_PATHS = [
    "security/",
    "include/linux/kthread.h",
    "include/net/tcp.h",
    "kernel/fork.c",
    "mm/util.c",
    "io_uring/",
    "fs/smb/",
]

def is_security_file(path):
    for p in SECURITY_PATHS:
        if p in path:
            return True
    return False

def resolve_conflicts(filepath, prefer_theirs_for_security=True):
    with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()

    if '<<<<<<< HEAD' not in content:
        return False

    is_sec = is_security_file(filepath)
    changes = []

    # Pattern handles both 3-way (with |||||||) and 2-way conflicts
    pattern = re.compile(
        r'<<<<<<< HEAD\n(.*?)(?:\|\|\|\|\|\|\| [^\n]*\n(.*?))?=======\n(.*?)>>>>>>> [^\n]*\n',
        re.DOTALL
    )

    def replacer(m):
        ours = m.group(1) or ''
        base = m.group(2) or ''
        theirs = m.group(3) or ''

        if is_sec and prefer_theirs_for_security:
            # Merge both: keep ours additions and theirs additions
            # Simple strategy: if theirs adds something not in ours, append it
            ours_lines = set(ours.splitlines())
            theirs_lines = theirs.splitlines()
            extra = [l for l in theirs_lines if l.strip() and l not in ours_lines]
            if extra:
                return ours + '\n'.join(extra) + '\n'
            return ours
        else:
            return ours

    new_content = pattern.sub(replacer, content)

    if new_content != content:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(new_content)
        return True
    return False

def find_conflicted_files(root):
    conflicted = []
    for dirpath, dirnames, filenames in os.walk(root):
        # Skip .git
        dirnames[:] = [d for d in dirnames if d != '.git']
        for fname in filenames:
            ext = os.path.splitext(fname)[1]
            if ext in ('.c', '.h', '.S', '.rs', '.py') or fname in ('Kconfig', 'Makefile') or fname.endswith('.mk'):
                fpath = os.path.join(dirpath, fname)
                try:
                    with open(fpath, 'r', encoding='utf-8', errors='replace') as f:
                        if '<<<<<<< HEAD' in f.read():
                            conflicted.append(fpath)
                except Exception:
                    pass
    return conflicted

if __name__ == '__main__':
    root = sys.argv[1] if len(sys.argv) > 1 else '.'
    files = find_conflicted_files(root)
    print(f"Found {len(files)} files with conflict markers")
    resolved = 0
    failed = []
    for f in files:
        try:
            if resolve_conflicts(f):
                rel = os.path.relpath(f, root)
                print(f"  RESOLVED: {rel}")
                resolved += 1
        except Exception as e:
            failed.append((f, str(e)))

    print(f"\nResolved: {resolved}/{len(files)}")
    if failed:
        print("Failed:")
        for f, e in failed:
            print(f"  {f}: {e}")

    # Check remaining
    remaining = find_conflicted_files(root)
    if remaining:
        print(f"\nStill has conflicts ({len(remaining)}):")
        for f in remaining:
            print(f"  {os.path.relpath(f, root)}")
    else:
        print("\nAll conflicts resolved!")
