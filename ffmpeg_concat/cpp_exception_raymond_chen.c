BOOL ComputeChecksum(LPCTSTR pszFile, DWORD* pdwResult)
{
    HANDLE h = CreateFile(pszFile, GENERIC_READ, 
            FILE_SHARE_READ, NULL, OPEN_EXISTING, 
            FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hfm = CreateFileMapping(h, NULL, PAGE_READ, 0, 
            0, NULL);
    void *pv = MapViewOfFile(hfm, FILE_MAP_READ, 0, 0, 0);
    DWORD dwHeaderSum;
    CheckSumMappedFile(pvBase, GetFileSize(h, NULL),
            &dwHeaderSum, pdwResult);
    UnmapViewOfFile(pv);
    CloseHandle(hfm);
    CloseHandle(h);
    return TRUE;
}

