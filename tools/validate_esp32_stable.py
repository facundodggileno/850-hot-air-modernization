#!/usr/bin/env python3
"""Static, source-based ESP32-S3 release checks. Not a hardware safety test."""
from __future__ import annotations
import csv
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

root = Path(__file__).resolve().parents[1]
folder = root / "firmware/esp32/Hony850Pro_ESP32S3_v3_4_2"
config = folder / "Config.h"
csv_file = root / "hardware/esp32-s3-pinout-stable.csv"
diagram = root / "hardware/esp32-s3-wiring-v3.4.2.svg"
expected = {
 "LCD_SDA":1, "LCD_SCL":2, "ZC":4, "PUMP":5, "HEATER":6,
 "RELAY":7, "BUZZER":8, "STAND":9,
 "TEMP_CLK":10, "TEMP_DT":11, "TEMP_SW":12,
 "AIR_CLK":13, "AIR_DT":14, "AIR_SW":15,
 "MAX_CS":16, "MAX_SO":17, "MAX_SCK":18,
}
errors = []
if not config.is_file():
 errors.append("Missing ESP32 firmware Config.h")
else:
 text = config.read_text("utf-8")
 try:
  declarations = text.split("namespace Cfg {", 1)[1].split("constexpr bool MAINS_OUTPUTS_ENABLED", 1)[0]
  actual = {k:int(v) for k,v in re.findall(r"\b([A-Z][A-Z0-9_]*)\s*=\s*(\d+)\b", declarations)}
  for k,v in expected.items():
   if actual.get(k)!=v: errors.append(f"GPIO mismatch for {k}: expected {v}, found {actual.get(k)}")
  for symbol,val in [("HONY_ENABLE_MAINS_OUTPUTS","1"),("HONY_ENABLE_PUMP_OUTPUT","1"),
                     ("HONY_CHANNEL_COUNT","3"),("HONY_USER_MAX_TEMP_C","480")]:
   if not re.search(r"#define\s+"+symbol+r"\s+"+val+r"\b",text):
    errors.append(f"Unexpected {symbol}, expected {val}")
 except Exception as ex: errors.append(f"Cannot parse Config.h: {ex}")
if csv_file.is_file():
 with csv_file.open(encoding="utf-8",newline="") as f:
  rows=list(csv.DictReader(f))
 csv_vals={r["Constante"]:int(r["GPIO"]) for r in rows}
 if csv_vals!=expected: errors.append("Pinout CSV and firmware definitions differ")
else: errors.append("Missing GPIO CSV")
if diagram.is_file():
 try:
  root_svg=ET.parse(diagram).getroot()
  if not root_svg.tag.endswith("svg"): errors.append("Invalid SVG root")
  xml_text=diagram.read_text("utf-8")
  for pin in sorted(set(expected.values())):
   if f"GPIO{pin}" not in xml_text: errors.append(f"Missing GPIO{pin} in SVG")
 except Exception as ex: errors.append(f"Invalid SVG: {ex}")
else: errors.append("Missing connection diagram SVG")
if folder.is_dir():
 found=[p for p in folder.iterdir() if p.is_file()]
 if len(found)!=25: errors.append(f"Expected 25 firmware files, found {len(found)}")
 if not (folder/(folder.name+".ino")).is_file(): errors.append("Sketch filename/folder mismatch")
else: errors.append("Missing sketch folder")
if errors:
 for e in errors: print("FAIL:",e)
 sys.exit(1)
print("PASS: 17 stable GPIO assignments, CSV/SVG, 25 firmware files, main output flags")
print("Static checks only. Does not compile firmware or assess physical safety.")
