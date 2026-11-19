#!/usr/bin/env python3
"""Structural QA for the public Hony 850 Pro repository; does not validate mains safety."""
from __future__ import annotations
import re
import sys
from pathlib import Path
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[1]
FIRMWARE = ROOT / 'firmware/current/Hony850Pro_Nano_Modular_v2_0_5'
REQUIRED = [
    ROOT / 'README.md', ROOT / 'README.en.md',
    ROOT / 'hardware/Hony850Pro_Esquematico_Conexiones_Completas_V2_0_5.pdf',
    ROOT / 'bom/BOM.csv',
    FIRMWARE / 'Hony850Pro_Nano_Modular_v2_0_5.ino',
    ROOT / 'docs/academic/Carpeta_de_Campo_Hony_850_Pro_2026-11-19.docx',
    ROOT / 'docs/academic/Carpeta_de_Campo_Hony_850_Pro_2026-11-19.pdf',
    ROOT / 'docs/academic/Presentacion_Hony_850_Pro_Fotos_Reales_2026.pptx',
    ROOT / 'docs/academic/Presentacion_Hony_850_Pro_Fotos_Reales_2026.pdf',
]
errors = []
for path in REQUIRED:
    if not path.is_file() or path.stat().st_size == 0:
        errors.append(f'Required file missing or empty: {path.relative_to(ROOT)}')

expected = 25
found = [p for p in FIRMWARE.iterdir() if p.is_file()] if FIRMWARE.exists() else []
if len(found) != expected:
    errors.append(f'Firmware should include {expected} files; found {len(found)}')

n_links = 0
pat = re.compile(r'!?\[[^\]]*\]\(([^)]+)\)')
for path in ROOT.rglob('*.md'):
    contents = path.read_text(encoding='utf-8')
    for raw in pat.findall(contents):
        if raw.startswith(('http://', 'https://', 'mailto:', '#')):
            continue
        rel = raw.split('#', 1)[0]
        if not rel:
            continue
        n_links += 1
        if not (path.parent / rel).exists():
            errors.append(f'Broken local link in {path.relative_to(ROOT)}: {raw}')

# Verify archive readability / signature only, not slides' semantics.
for path in (ROOT / 'docs/academic').glob('*'):
    if path.suffix in ('.docx', '.pptx'):
        try:
            with ZipFile(path) as z:
                bad = z.testzip()
                if bad: errors.append(f'Corrupt office ZIP member: {path.name}: {bad}')
        except Exception as exc:
            errors.append(f'Cannot read office document {path.name}: {exc}')
    if path.suffix == '.pdf' and path.read_bytes()[:5] != b'%PDF-':
        errors.append(f'Bad PDF signature: {path.name}')

# No planned future sensor is permitted; flow descriptions of historic faults are okay.
for name in ('docs/roadmap.md', 'docs/BUILD_ES.md', 'README.md'):
    content = (ROOT / name).read_text(encoding='utf-8').casefold()
    if not any(x in content for x in ('no se', 'no habrá', 'no incorpora')) or 'sensor' not in content:
        errors.append(f'Decision to exclude airflow sensors not clearly documented: {name}')

if errors:
    print('\n'.join('FAIL: ' + err for err in errors))
    sys.exit(1)
print(f'PASS: {len(found)} firmware files, {n_links} checked local Markdown links, document signatures and sensor scope policy.')
print('NOTE: This QA does not compile Arduino firmware or certify physical electrical safety.')
