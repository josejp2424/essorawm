#!/usr/bin/env python3
"""Small dependency-free PO updater and MO compiler for Puppy build systems."""
from __future__ import annotations

import ast
import os
import re
import struct
import sys
from pathlib import Path


def _unquote(value: str) -> str:
    value = value.strip()
    if not value.startswith('"'):
        return ''
    return ast.literal_eval(value)


def _quote(value: str) -> str:
    value = value.replace('\\', '\\\\').replace('"', '\\"')
    value = value.replace('\t', '\\t').replace('\r', '\\r').replace('\n', '\\n')
    return f'"{value}"'


def split_blocks(text: str) -> list[str]:
    return re.split(r'\n\s*\n', text.rstrip('\n'))


def field_value(block: str, field: str) -> str | None:
    lines = block.splitlines()
    prefix = field + ' '
    for i, line in enumerate(lines):
        if line.startswith(prefix):
            value = _unquote(line[len(prefix):])
            j = i + 1
            while j < len(lines) and lines[j].startswith('"'):
                value += _unquote(lines[j])
                j += 1
            return value
    return None


def entry_key(block: str) -> str | None:
    msgid = field_value(block, 'msgid')
    if msgid is None:
        return None
    context = field_value(block, 'msgctxt')
    return (context + '\x04' if context is not None else '') + msgid


def replace_msgstr(block: str, translation: str) -> str:
    lines = block.splitlines()
    start = None
    end = None
    for i, line in enumerate(lines):
        if line.startswith('msgstr '):
            start = i
            end = i + 1
            while end < len(lines) and lines[end].startswith('"'):
                end += 1
            break
    new_line = 'msgstr ' + _quote(translation)
    if start is None:
        lines.append(new_line)
    else:
        lines[start:end] = [new_line]
    return '\n'.join(lines)


def update_po(path: Path, messages: list[str], translations: dict[str, str],
              source_ref: str = 'src/essoradesktop.c src/desktopicons.c') -> None:
    text = path.read_text(encoding='utf-8') if path.exists() else ''
    blocks = split_blocks(text) if text else []
    by_msgid: dict[str, int] = {}
    for index, block in enumerate(blocks):
        msgid = field_value(block, 'msgid')
        if msgid is not None:
            by_msgid[msgid] = index
    for msgid in messages:
        msgstr = translations.get(msgid, msgid)
        if msgid in by_msgid:
            idx = by_msgid[msgid]
            blocks[idx] = replace_msgstr(blocks[idx], msgstr)
        else:
            flags = '#, c-format\n' if '%s' in msgid else ''
            block = f'#: {source_ref}\n{flags}msgid {_quote(msgid)}\nmsgstr {_quote(msgstr)}'
            by_msgid[msgid] = len(blocks)
            blocks.append(block)
    path.write_text('\n\n'.join(blocks).rstrip() + '\n', encoding='utf-8')


def update_pot(path: Path, messages: list[str],
               source_ref: str = 'src/essoradesktop.c src/desktopicons.c') -> None:
    text = path.read_text(encoding='utf-8') if path.exists() else ''
    blocks = split_blocks(text) if text else []
    existing = {field_value(b, 'msgid') for b in blocks}
    for msgid in messages:
        if msgid in existing:
            continue
        flags = '#, c-format\n' if '%s' in msgid else ''
        blocks.append(f'#: {source_ref}\n{flags}msgid {_quote(msgid)}\nmsgstr ""')
    path.write_text('\n\n'.join(blocks).rstrip() + '\n', encoding='utf-8')


def parse_catalog(path: Path) -> dict[str, str]:
    catalog: dict[str, str] = {}
    for block in split_blocks(path.read_text(encoding='utf-8')):
        if '#, fuzzy' in block:
            continue
        key = entry_key(block)
        msgstr = field_value(block, 'msgstr')
        if key is not None and msgstr is not None:
            catalog[key] = msgstr
    return catalog


def compile_mo(po_path: Path, mo_path: Path) -> None:
    catalog = parse_catalog(po_path)
    keys = sorted(catalog)
    ids = b'\0'.join(k.encode('utf-8') for k in keys) + b'\0'
    strs = b'\0'.join(catalog[k].encode('utf-8') for k in keys) + b'\0'
    n = len(keys)
    keystart = 7 * 4 + n * 8 * 2
    valuestart = keystart + len(ids)
    offsets = []
    offset = 0
    for key in keys:
        data = key.encode('utf-8')
        offsets.append((len(data), keystart + offset))
        offset += len(data) + 1
    value_offsets = []
    offset = 0
    for key in keys:
        data = catalog[key].encode('utf-8')
        value_offsets.append((len(data), valuestart + offset))
        offset += len(data) + 1
    output = [struct.pack('<7I', 0x950412DE, 0, n, 7 * 4,
                          7 * 4 + n * 8, 0, 0)]
    output.extend(struct.pack('<2I', length, pos) for length, pos in offsets)
    output.extend(struct.pack('<2I', length, pos) for length, pos in value_offsets)
    output.extend([ids, strs])
    mo_path.parent.mkdir(parents=True, exist_ok=True)
    mo_path.write_bytes(b''.join(output))


def main(argv: list[str]) -> int:
    if len(argv) == 4 and argv[1] == 'compile':
        compile_mo(Path(argv[2]), Path(argv[3]))
        return 0
    print(f'usage: {argv[0]} compile input.po output.mo', file=sys.stderr)
    return 2


if __name__ == '__main__':
    raise SystemExit(main(sys.argv))
