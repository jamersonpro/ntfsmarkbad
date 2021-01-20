// NtfsMarkBad.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "stdafx.h"

#include "common.h"

#include "ulib.hxx"
#include "message.hxx"
#include "system.hxx"
#include "ifssys.hxx"
#include "ntfsvol.hxx"

#include <iostream>   // std::cout
#include <string>     // std::string, std::stoll
#include <algorithm>
#include <stdlib.h>
#include <errno.h>

BOOLEAN DefineClassDescriptors()
{
    return UlibDefineClassDescriptors() && IfsutilDefineClassDescriptors() && UntfsDefineClassDescriptors();
}

unsigned char char_toupper(unsigned char c)
{
	return toupper(c);
}

std::string str_toupper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), char_toupper);
    return s;
}

void About()
{
    std::cout << "Mark clusters as bad on NTFS\n\nUsage:\nNTFSMARKBAD drive: first_drive_sector_to_mark last_drive_sector_to_mark\n";
};

// convert string to long long
__int64 stoll(const std::string& str) 
{
    const char* ptr = str.c_str();
    char* error_ptr;
    __int64 result = _strtoi64(ptr, &error_ptr, 10);

    if (ptr == error_ptr) 
	{
        throw std::out_of_range("invalid stoll argument");
    }

    if (errno == EINVAL) 
	{
        throw std::out_of_range("stoll argument out of range");
    }

    return result;
}

int __cdecl
main(
    ULONG nArgCount,
    PSTR arrArguments[]
)
{
    MESSAGE    Message;
    Message.Initialize();

    Message.Out("NTFSMARKBAD ", "0.0.1",
#if defined(_M_AMD64)        
        " x64"
#else
        " x32"
#endif
    );
    Message.Out("");

    DefineClassDescriptors();

    if (nArgCount != 4)
    {
        About();
        return 1;
    }

    std::string drive = arrArguments[1];
    drive = str_toupper(drive);

    if (drive.length() != 2
        || drive[0] < 'A' || drive[0]>'Z'
        || drive[1] != ':')
    {
        Message.Out("Invalid drive.");
    }

    std::string firstSectorStr = arrArguments[2];
    __int64 firstSector;
    try
    {
        firstSector = ::stoll(firstSectorStr);
    }
    catch (const std::out_of_range&)
    {
        firstSector = -1;
    }
    if (firstSector < 0)
    {
        Message.Out("Invalid first sector number.");
        return 1;
    }

    std::string lastSectorStr = arrArguments[3];
    __int64 lastSector;
    try
    {
        lastSector = ::stoll(lastSectorStr);
    }
    catch (const std::out_of_range&)
    {
        lastSector = -1;
    }
    if (lastSector < 0 || lastSector < firstSector)
    {
        Message.Out("Invalid last sector number.");
        return 1;
    }



    DSTRING         CurrentDrive;
    if (!SYSTEM::QueryCurrentDosDriveName(&CurrentDrive))
    {
        Message.Out("Error.");
        return 1;
    }

    DSTRING         InputParamDrive;
    InputParamDrive.Initialize(drive.c_str());


    DSTRING         NtDriveName;
    NtDriveName.Initialize("\\??\\");
    NtDriveName.Strcat(&InputParamDrive);


    if (CurrentDrive == InputParamDrive)
    {
        Message.Out("Cannot lock current drive.");
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
            Message.Out("Access denied.");
        }
        else if (Status != STATUS_SUCCESS)
        {
            Message.Out("Cannot open volume for direct access.");
        }
        else
        {
            Message.Out("Cannot determine file system of drive: ", drive.c_str());
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

	Result = NtfsVol.MarkBad(firstSector, lastSector, &Message);
    if (!Result)
    {
        Message.Out("An error has occurred.");
        return 1;
    }

    Message.Out("Completed.");
    return 0;
}


