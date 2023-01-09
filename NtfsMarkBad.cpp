// NtfsMarkBad.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "stdafx.h"

#include <fstream>

#include "common.h"

#include "ulib.hxx"
#include "message.hxx"
#include "system.hxx"
#include "ifssys.hxx"
#include "ntfsvol.hxx"

#include "TextUtils.h"

#define VERSION_TEXT "0.0.2"

BOOLEAN DefineClassDescriptors()
{
    return UlibDefineClassDescriptors() && IfsutilDefineClassDescriptors() && UntfsDefineClassDescriptors();
}

void OutputVersion(MESSAGE& Message)
{
    Message.Out("NTFSMARKBAD " VERSION_TEXT
#if defined(_M_AMD64)        
        " x64"
#else
        " x32"
#endif
        "     https://github.com/jamersonpro/ntfsmarkbad"
    );
    Message.Out("");
}

void OutputAboutBanner(MESSAGE& Message)
{
	Message.Out("Mark clusters as bad on NTFS without checking\n"
		"\n"
		"Basic usage:\n"
		"NTFSMARKBAD <drive>: <first_sector_number> <last_sector_number>\n"
		"Batch mode:\n"
		"NTFSMARKBAD <drive>: /B <sector_numbers_file>\n"
		"Info mode:\n"
		"NTFSMARKBAD <drive>:\n");
}

int ParseSectorsFile(MESSAGE& Message, const std::string& filename, std::vector<sectors_range>& runTargets)
{
    Message.Out("Reading ", filename, "...");
	std::ifstream input_file(filename.c_str());
    if (input_file.is_open())
    {
        std::string line;
        int lineCounter = 0;
        while (std::getline(input_file, line))
        {
            lineCounter++;
            trim(line);
            if (line.empty()) continue;
            std::vector<std::string> parts = split(line, " \t,;");
            if (parts.empty())
            {
                continue;
            }
            else if (parts.size() == 1)
            {
                std::string firstSectorStr = trim(parts[0]);
                __int64 firstSector = parse_int64(firstSectorStr);
                if (firstSector < 0)
                {
                    Message.Out("Invalid sector number in file, line ", lineCounter, " value ", firstSectorStr);
                    return 1;
                }
                if (!runTargets.empty() && runTargets.back().lastSector == firstSector - 1)
                {
                    runTargets.back().lastSector = firstSector;
                }
                else
                {
                    runTargets.push_back(sectors_range(firstSector, firstSector));
                }
            }
            else if (parts.size() == 2)
            {
                std::string firstSectorStr = trim(parts[0]);
                __int64 firstSector = parse_int64(firstSectorStr);
                if (firstSector < 0)
                {
                    Message.Out("Invalid first sector number in file, line ", lineCounter, " value ", firstSectorStr);
                    return 1;
                }
                std::string lastSectorStr = trim(parts[1]);
                __int64 lastSector = parse_int64(lastSectorStr);
                if (lastSector < 0)
                {
                    Message.Out("Invalid last sector number in file with sectors list, line ", lineCounter, " value ", lastSectorStr);
                    return 1;
                }
                if (lastSector < firstSector)
                {
                    Message.Out("Last sector number is less than first number in file, line ", lineCounter);
                    return 1;
                }
                if (!runTargets.empty() && runTargets.back().lastSector == firstSector - 1)
                {
                    runTargets.back().lastSector = lastSector;
                }
                else
                {
                    runTargets.push_back(sectors_range(firstSector, lastSector));
                }
            }
            else
            {
                Message.Out("Wrong line in file, line #", lineCounter, " value ", line);
                return 1;
            }
        }
        input_file.close();
    }
    else
    {
        Message.Out("Failed to open file with sectors list.");
        return 1;
    }
    if (runTargets.empty())
    {
        Message.Out("Empty file with sectors list.");
        return 1;
    }
    return 0;
}

int __cdecl
main(
    ULONG nArgCount,
    PSTR arrArguments[]
)
{
    MESSAGE    Message;
    Message.Initialize();

    OutputVersion(Message);

    DefineClassDescriptors();

    std::vector<sectors_range> runTargets;
    std::string runDrive;

    if (nArgCount != 2 && nArgCount != 4)
    {
        OutputAboutBanner(Message);
        return 1;
    }

    runDrive = str_toupper(arrArguments[1]);

    if (runDrive.length() != 2
        || runDrive[0] < 'A' || runDrive[0] > 'Z'
        || runDrive[1] != ':')
    {
        Message.Out("Invalid drive.");
        return 1;
    }

    if (nArgCount == 4)
    {
        std::string argumentStr2 = arrArguments[2];
        std::string argumentStr3 = arrArguments[3];

        if (str_toupper(argumentStr2) == "/B") //batch mode
        {
            if (ParseSectorsFile(Message, argumentStr3, runTargets))
                return 1;
        }
        else //basic mode
        {
            std::string firstSectorStr = arrArguments[2];
            __int64 firstSector = parse_int64(firstSectorStr);
            if (firstSector < 0)
            {
                Message.Out("Invalid first sector number.");
                return 1;
            }

            std::string lastSectorStr = arrArguments[3];
            __int64 lastSector = parse_int64(lastSectorStr);
            if (lastSector < 0 || lastSector < firstSector)
            {
                Message.Out("Invalid last sector number.");
                return 1;
            }

            runTargets.push_back(sectors_range(firstSector, lastSector));
        }
    }

    //sort and join sectors ranges
    std::sort(runTargets.begin(), runTargets.end());

    std::vector<sectors_range> sortedRunTargets;

    for (std::vector<sectors_range>::iterator current = runTargets.begin(); current != runTargets.end(); ++current)
    {
        if (!sortedRunTargets.empty())
        {
            sectors_range& last = sortedRunTargets.back();
            if (last.contains(current->firstSector) || last.lastSector + 1 == current->firstSector)
            {
                if (!last.contains(current->lastSector))
                {
                    last.lastSector = current->lastSector;
                }
                continue;
            }
        }
        sortedRunTargets.push_back(*current);
    }

    runTargets = sortedRunTargets;

    DSTRING         CurrentDrive;
    if (!SYSTEM::QueryCurrentDosDriveName(&CurrentDrive))
    {
        Message.Out("Error.");
        return 1;
    }

    DSTRING         InputParamDrive;
    InputParamDrive.Initialize(runDrive.c_str());


    DSTRING         NtDriveName;
    NtDriveName.Initialize("\\??\\");
    NtDriveName.Strcat(&InputParamDrive);


    if (CurrentDrive == InputParamDrive)
    {
        Message.Out("Cannot lock current drive. Change current drive and rerun the program.");
        return 1;
    }


    DSTRING ntfs_name;
    if (!ntfs_name.Initialize("NTFS"))
    {
        Message.Out("Error.");
        return 1;
    }


    NTSTATUS            Status;
    DSTRING             drivename;

    BOOL FsNameIsNtfs;

    if (!IFS_SYSTEM::QueryFileSystemNameIsNtfs(&NtDriveName,
        &FsNameIsNtfs,
        &Status))
    {
        if (Status == STATUS_ACCESS_DENIED)
        {
            Message.Out("Access denied. Run the program as administrator.");
        }
        else if (Status != STATUS_SUCCESS)
        {
            Message.Out("Cannot open volume for direct access.");
        }
        else
        {
            Message.Out("Cannot determine file system of drive: ", runDrive);
        }

        return 1;
    }

    if (!FsNameIsNtfs) //NOT NTFS
    {
        Message.Out("Only NTFS file system supported.");
        return 1;
    }


    NTFS_VOL    NtfsVol;
    BOOLEAN     Result;

    Result = NtfsVol.Initialize(&NtDriveName, &Message);
    if (!Result)
    {
        Message.Out("Failed to initialize.");
        return 1;
    }

    Result = NtfsVol.MarkBad(runTargets, &Message);
    if (!Result)
    {
        Message.Out("An error has occurred.");
        return 1;
    }

    Message.Out("Completed.");
    return 0;
}



