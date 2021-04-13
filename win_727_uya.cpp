#include <iostream>
#include <stdio.h>
#include <windows.h>
#include <list>
#include <string>
#include <chrono>

HANDLE mutex = NULL;

DWORD WINAPI thread_copy_file(PVOID filenames)
{
    if (!mutex)
    {
        wprintf(L"Mutex uninitialized!\n");
        return -1;
    }

    DWORD wait_code = WaitForSingleObject(mutex, INFINITE);

    if (wait_code == WAIT_OBJECT_0)
    {
        std::wstring* names = (std::wstring*)filenames;

        Sleep(1000);
        wprintf(L"Copy from %s to %s\n", names[0].c_str(), names[1].c_str());
        BOOL result = CopyFile(names[0].c_str(), names[1].c_str(), FALSE);

        ReleaseMutex(mutex);

        if (result)
            return 0;
        else
            return -1;
    }
    else
    {
        std::wstring* names = (std::wstring*)filenames;
        wprintf(L"Failed to start thread %s - code %x\n", names[0].c_str(), wait_code);
        return -1;
    }
}

int main()
{
    std::wstring work_dir = L"work_dir";
    std::wstring dest = L"dest";

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    mutex = CreateMutex(NULL, FALSE, L"Mutex");

    std::list<std::wstring> files;
    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile((work_dir+L"/*").c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                files.push_back(fd.cFileName);
            }
        } while (FindNextFile(hFind, &fd));
        FindClose(hFind);
    }

    size_t count = files.size();
    DWORD* dwThreadID = new DWORD[count];
    HANDLE* hThread = new HANDLE[count];

    auto it = files.begin();
    std::wstring** params = new std::wstring*[count];
    for (int i=0; it != files.end(); it++, i++)
    {
        std::wstring file = *it;
        wprintf(L"%s - Create thread\n", file.c_str());
        params[i] = new std::wstring[2];
        params[i][0] = (work_dir + L"/" + file);
        params[i][1] = (dest + L"/" + file);

        hThread[i] = CreateThread(NULL, 0, thread_copy_file, (PVOID)params[i], 0, &dwThreadID[i]);

        if (!hThread)
            wprintf(L"Couldn't create thread\n");
    }

    WaitForMultipleObjects(count, hThread, TRUE, INFINITE);

    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    wprintf(L"Execution time = %d ms\n", delta.count());
    printf("Success!\n");
}