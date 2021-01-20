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

#include "wstring.hxx"


DECLARE_CLASS( MESSAGE );

class MESSAGE : public OBJECT {

    public:
         
        DECLARE_CONSTRUCTOR(MESSAGE);

        VIRTUAL
        ~MESSAGE(
            );

        void Out(const char* str);
        void Out(const char* str1, const char* str2);
        void Out(const char* str1, const char* str2, const char* str3);
        void Out(const char* str, LONGLONG number);
        void Out(const char* str1, LONGLONG number, const char* str2);
        void Out(const char* str1, LONGLONG number1, const char* str2, LONGLONG number2, const char* str3);
        void Out(const char* str1, LONGLONG number1, const char* str2, LONGLONG number2, const char* str3, LONGLONG number3, const char* str4);

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


