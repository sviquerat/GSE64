# GSE
## Reimplementation of GSEmerge for modern times (formerly by Geo-X, JD)
Supports modern 64 bit machines and provides the following command line arguments:
- *-f* or *--filename* (required) - filename of any of the 4 AudioVOR.exe export files (GPS, EFF, SIG, flt)
- *-i* or *--interval* (optional, defaults to 4) - time interval \[s] used for binning location data

Usage: Combining GPS-, SIG- and EFF data created by AudioVOR.exe

Compiled and tested with: gcc 9.2.0 x64 windows (sample build script included)
