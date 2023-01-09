# NtfsMarkBad - fastest way to mark bad clusters *without checking*

## About

This command line utility allow to manually mark clusters as bad without checking on NTFS file system.

Just specify the first and last sectors of the bad area. Or provide a text file with bad sector numbers.
Use physical sector numbers of the disk storage, not logical volume sector numbers.
If you are in doubt about sector numbers, you can run the program in info mode to display volume information.

Only unused clusters are processed. 

Compiled to 64-bit (file NTFSMARKBAD.EXE). Also a 32-bit version available (file NTFSMARKBAD32.EXE). 

Tested on Windows 7 SP1 (x32 and x64) and Windows 10 (x32 and x64). 
Can be run from the Windows Setup command line (use the version that exactly matches the Windows version). 

To clear the list of bad clusters reformat volume or run CHKDSK /B to rescan clusters for errors.

## Changelog

* 0.0.1 - Inital version.
* 0.0.2 - Added batch mode and info mode.

## Basic usage

`NTFSMARKBAD <drive>: <first_sector_number> <last_sector_number>`

### Example

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

## Batch mode

`NTFSMARKBAD <drive>: /B <sector_numbers_file>`

First, create a text file with bad sector numbers (physical sector numbers). 
Each line of the file must contain the bad sector number or the first and last sector numbers of the bad area.
You can use a space, tab, comma, or semicolon as the delimiter. 
Empty lines will be skipped.
Then, run the program with /B command option.

### Example

To mark sectors 100001, 100003, 100006-100010 (physical sector numbers) on volume D:

1) Create text file SECTORS.TXT with sectors numbers

```
100001
100003
100006 100010
```

2) Open the command prompt as administrator

```
CMD
```

3) Quick format volume to NTFS

```
FORMAT D: /FS:NTFS /Q
```

4) Mark sectors as bad

```
NTFSMARKBAD D: /B SECTORS.TXT
```

5) Check file system state

```
CHKDSK D: /F
```

## Info mode

`NTFSMARKBAD <drive>:`

Specify only drive to display volume information. No changes will be made to the file system.

### Example

To display volume D: information

1) Open the command prompt as administrator

```
CMD
```

2) Run program

```
NTFSMARKBAD D: 
```

