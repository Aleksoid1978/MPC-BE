/* MPC-BE comment out
// Force C locale to avoid this warning:
//
// mmreg.h : warning C4819: The file contains a character that cannot be represented in the current code page (932). Save the file in Unicode format to prevent data loss
#pragma setlocale("C")

// Detect the Windows SDK in use and select Windows 2000 baseline
// if the Vista SDK, else Windows 98 baseline.

#ifdef _MSC_VER
#include <ntverp.h>
#else
#define VER_PRODUCTBUILD 6001
#endif
#if VER_PRODUCTBUILD > 6000
#define _WIN32_WINNT 0x0500
#else
#define _WIN32_WINNT 0x0410
#endif
*/

#include <vd2/system/vdtypes.h>
#include <math.h>
