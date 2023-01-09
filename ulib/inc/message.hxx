/*++

Module Name:

    message.hxx

Abstract:

    The MESSAGE class provides a  implementation of a message displayer
    class.  Message displayers are meant to be used by applications to
    relay information to the user.  Many functions will require a 'MESSAGE'
    parameter through which to relay their output.

--*/

#pragma once

#include <string>
#include <iostream>
#include "wstring.hxx"

DECLARE_CLASS( MESSAGE );

class MESSAGE : public OBJECT {

    public:
         
        DECLARE_CONSTRUCTOR(MESSAGE);

        VIRTUAL
        ~MESSAGE(
            );

        void Out(const char* str)
        {
            std::cout << str << "\n";
        }
        void Out(const char* str1, const std::string& str2)
        {
            std::cout << str1 << str2 << "\n";
        }
        void Out(const char* str1, const std::string& str2, const char* str3)
        {
            std::cout << str1 << str2 << str3 << "\n";
        }
        void Out(const char* str, LONGLONG number)
        {
            std::cout << str << number << "\n";
        }
        void Out(const char* str1, LONGLONG number, const char* str2)
        {
            std::cout << str1 << number << str2 << "\n";
        }
        void Out(const char* str1, LONGLONG number1, const char* str2, LONGLONG number2, const char* str3)
        {
            std::cout << str1 << number1 << str2 << number2 << str3 << "\n";
        }
        void Out(const char* str1, LONGLONG number1, const char* str2, LONGLONG number2, const char* str3, LONGLONG number3, const char* str4)
        {
            std::cout << str1 << number1 << str2 << number2 << str3 << number3 << str4 << "\n";
        }
        void Out(const char* str1, int number, const char* str2, const std::string& str3)
        {
            std::cout << str1 << number << str2 << str3 << "\n";
        }

		inline void OutIncorrectStructure()
		{
            Out("Incorrect file system structure found.");
		}

        BOOLEAN
        Initialize(
            );

    private:
         
        VOID
        Construct(
            );

         
        VOID
        Destroy(
            );

};


