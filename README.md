# GSE
## Reimplementation of GSEmerge for modern computers (formerly by Geo-X, JD)
Supports 64 bit machines and provides the additional following command line arguments:
- *-f* or *--filename* (required) - filename of any of the 4 AudioVOR.exe export files (GPS, EFF, SIG, flt)
- *-i* or *--interval* (optional, defaults to 4) - time interval \[s] used for binning location data

more options coming over the next months

Usage: Command line tool to combine GPS-, SIG- and EFF data created by AudioVOR.exe

Compiled and tested with: gcc 9.2.0 x64 windows (sample build script included)

/bin contains precompiled binary created by the build.bat
