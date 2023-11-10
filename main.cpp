#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>

#include <windows.h>

#include <detours/detours.h>

using namespace std;

CHAR szDllPath[1024];
string szCommand;
CHAR szPath[1024];

static BOOL CALLBACK ListBywayCallback(
        PVOID pContext,
        LPCSTR pszFile,
        LPCSTR *ppszOutFile
) {
    (void) pContext;

    *ppszOutFile = pszFile;
    if (pszFile) {
        printf("    %s\n", pszFile);
    }
    return TRUE;
}

static BOOL CALLBACK ListFileCallback(
        PVOID pContext,
        LPCSTR pszOrigFile,
        LPCSTR pszFile,
        LPCSTR *ppszOutFile
) {
    (void) pContext;

    *ppszOutFile = pszFile;
    printf("    %s -> %s\n", pszOrigFile, pszFile);
    return TRUE;
}

static BOOL CALLBACK AddBywayCallback(
        PVOID pContext,
        LPCSTR pszFile,
        LPCSTR *ppszOutFile
) {
    auto pbAddedDll = (PBOOL) pContext;
    if (!pszFile && !*pbAddedDll) {                     // Add new byway.
        *pbAddedDll = TRUE;
        *ppszOutFile = szDllPath;
    }
    return TRUE;
}

BOOL SetFile(PCHAR pszPath, BOOL s_fRemove) {
    BOOL bGood = TRUE;
    HANDLE hOld = INVALID_HANDLE_VALUE;
    HANDLE hNew = INVALID_HANDLE_VALUE;
    PDETOUR_BINARY pBinary = nullptr;

    CHAR szOrg[MAX_PATH];
    CHAR szNew[MAX_PATH];
    CHAR szOld[MAX_PATH];

    szOld[0] = '\0';
    szNew[0] = '\0';

#ifdef _CRT_INSECURE_DEPRECATE
    strcpy_s(szOrg, sizeof(szOrg), pszPath);
    strcpy_s(szNew, sizeof(szNew), szOrg);
    strcat_s(szNew, sizeof(szNew), "#");
    strcpy_s(szOld, sizeof(szOld), szOrg);
    strcat_s(szOld, sizeof(szOld), "~");
#else
    strcpy(szOrg, pszPath);
    strcpy(szNew, szOrg);
    strcat(szNew, "#");
    strcpy(szOld, szOrg);
    strcat(szOld, "~");
#endif
    printf("  %s:\n", pszPath);

    hOld = CreateFile(szOrg,
                      GENERIC_READ,
                      FILE_SHARE_READ,
                      nullptr,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      nullptr);

    if (hOld == INVALID_HANDLE_VALUE) {
        printf("Couldn't open input file: %s, error: %lu\n",
               szOrg, GetLastError());
        bGood = FALSE;
        goto end;
    }

    hNew = CreateFile(szNew,
                      GENERIC_WRITE | GENERIC_READ, 0, nullptr, CREATE_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (hNew == INVALID_HANDLE_VALUE) {
        printf("Couldn't open output file: %s, error: %lu\n",
               szNew, GetLastError());
        bGood = FALSE;
        goto end;
    }

    if ((pBinary = DetourBinaryOpen(hOld)) == nullptr) {
        printf("DetourBinaryOpen failed: %lu\n", GetLastError());
        goto end;
    }

    if (hOld != INVALID_HANDLE_VALUE) {
        CloseHandle(hOld);
        hOld = INVALID_HANDLE_VALUE;
    }

    {
        BOOL bAddedDll = FALSE;

        DetourBinaryResetImports(pBinary);

        if (!s_fRemove) {
            if (!DetourBinaryEditImports(pBinary,
                                         &bAddedDll,
                                         AddBywayCallback, nullptr, nullptr, nullptr)) {
                printf("DetourBinaryEditImports failed: %lu\n", GetLastError());
            }
        }

        if (!DetourBinaryEditImports(pBinary, nullptr,
                                     ListBywayCallback, ListFileCallback,
                                     nullptr, nullptr)) {

            printf("DetourBinaryEditImports failed: %lu\n", GetLastError());
        }

        if (!DetourBinaryWrite(pBinary, hNew)) {
            printf("DetourBinaryWrite failed: %lu\n", GetLastError());
            bGood = FALSE;
        }

        DetourBinaryClose(pBinary);
        pBinary = nullptr;

        if (hNew != INVALID_HANDLE_VALUE) {
            CloseHandle(hNew);
            hNew = INVALID_HANDLE_VALUE;
        }

        if (bGood) {
            if (!DeleteFile(szOld)) {
                DWORD dwError = GetLastError();
                if (dwError != ERROR_FILE_NOT_FOUND) {
                    printf("Warning: Couldn't delete %s: %lu\n", szOld, dwError);
                    bGood = FALSE;
                }
            }
            if (!MoveFile(szOrg, szOld)) {
                printf("Error: Couldn't back up %s to %s: %lu\n",
                       szOrg, szOld, GetLastError());
                bGood = FALSE;
            }
            if (!MoveFile(szNew, szOrg)) {
                printf("Error: Couldn't install %s as %s: %lu\n",
                       szNew, szOrg, GetLastError());
                bGood = FALSE;
            }
        }

        DeleteFile(szNew);
    }


    end:
    if (pBinary) {
        DetourBinaryClose(pBinary);
        pBinary = nullptr;
    }
    if (hNew != INVALID_HANDLE_VALUE) {
        CloseHandle(hNew);
        hNew = INVALID_HANDLE_VALUE;
    }
    if (hOld != INVALID_HANDLE_VALUE) {
        CloseHandle(hOld);
        hOld = INVALID_HANDLE_VALUE;
    }
    return bGood;
}

int CDECL main(int argc, char **argv) {
    memset(szPath, 0, sizeof(szPath));
    GetModuleFileName(nullptr, szPath, sizeof(szPath));
    auto idx = strlen(szPath) - 1;
    while (idx >= 0 && szPath[idx] != '\\') {
        szPath[idx--] = '\0';
    }

    sprintf_s(szDllPath, ARRAYSIZE(szDllPath), "%s%s", szPath, "loaderdll.dll");
    szCommand = szPath + "Insight3.Exe"s;

    if (!filesystem::exists(szCommand)) {
        szCommand = szPath + "sourceinsight4.exe"s;
    }

    if (!filesystem::exists(szCommand)) {
        cout << "Source Insight 3.5 / 4.0 not found!" << endl;
    }


    if (argc == 2) {
        BOOL f_install = FALSE;
        if (strcmp(argv[1], "/install") == 0) {
            f_install = SetFile(szCommand.data(), FALSE);
            if (f_install == TRUE) {
                printf("Mod ExE Success!\n");
            } else {
                printf("Mod ExE Failed!\n");
            }
            return 1;
        } else if (strcmp(argv[1], "/uninstall") == 0) {
            f_install = SetFile(szCommand.data(), TRUE);
            if (f_install == TRUE) {
                printf("UnMod ExE Success!\n");
            } else {
                printf("UnMod ExE Failed!\n");
            }
            return 1;
        }
            //case args
        else {
            szCommand = szPath + "Insight3.Exe -p "s + argv[1];
        }
    }


    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    DWORD dwFlags = CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED;

    SetLastError(0);

    if (!DetourCreateProcessWithDll(nullptr, szCommand.data(),
                                    nullptr, nullptr, TRUE, dwFlags, nullptr, nullptr,
                                    &si, &pi, szDllPath, nullptr)) {
        DWORD dwError = GetLastError();
        printf("loaderex.exe: DetourCreateProcessWithDll failed: %lu\n", dwError);
        if (dwError == ERROR_INVALID_HANDLE) {
#if DETOURS_64BIT
            printf("loaderex.exe: Can't detour a 32-bit target process from a 64-bit parent process.\n");
#else
            printf("loaderex.exe: Can't detour a 64-bit target process from a 32-bit parent process.\n");
#endif
        }
        ExitProcess(9009);
    }

    ResumeThread(pi.hThread);
    return 0;
}