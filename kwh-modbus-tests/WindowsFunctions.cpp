#include "pch.h"
#include "WindowsFunctions.h"
#include <Windows.h>

WindowsFunctions::WindowsFunctions()
{
}

void WindowsFunctions::Windows_Sleep(int millis)
{
	Sleep(millis);
}
