# NtfsMarkBad

## About

This utility allow to mark clusters as bad without checking on NTFS file system.

Just specify the first and last sectors of the bad area. Use physical sector numbers of the disk storage, not logical volume sector numbers. 

Only unused clusters are processed. 

Compiled to 64-bit (file NTFSMARKBAD.EXE). Also a 32-bit version available (file NTFSMARKBAD32.EXE). 

Tested on Windows 7 SP1 (x32 and x64) and Windows 10 (x32 and x64). 
Can be run from the Windows Setup command line (use the version that exactly matches the Windows version). 

To clear the list of bad clusters reformat volume or run CHKDSK /B to rescan clusters for errors.

## Usage

To mark sectors from 100001 to 100010 (physical sector numbers) on volume D:

1) Open the command prompt as administrator

```
CMD
```

2) Quick format volume to NTFS

```
FORMAT D: /FS:NTFS /Q
```

3) Mark sectors as bad

```
NTFSMARKBAD D: 100001 100010
```

4) Check file system state

```
CHKDSK D: /F
```
