# ESP32 logger
for k-type thermocouple

## Hardware
- MAX31855
- OLED 0.96
- W5500 LAN module
- micro SD card adapter
- ESP32 DEVKITC v.4

## Gzip
```powershell
gzip -k -9 data/index.html
gzip -k -9 data/plotter.html
gzip -k -9 data/explore.html

Get-ChildItem -Recurse data -Include *.js,*.mjs,*.worker.js | ForEach-Object {gzip -k -9 $_.FullName}
```

```powershell
Get-ChildItem -Recurse data -Include *.html,*.js,*.mjs,*.worker.js,*.css,*.json | ForEach-Object { gzip -kf -9 $_.FullName }
```

```bash
find data -name "*.js" -or -name "*.mjs" -or -name "*.worker.js" -print0 | xargs -0 -I{} gzip -k -9 {}
```