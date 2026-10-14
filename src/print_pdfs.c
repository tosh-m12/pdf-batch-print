#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <shlobj.h>
#include <winspool.h>
#include <wchar.h>
#include <stdlib.h>

static void message(const wchar_t *s) { MessageBoxW(NULL,s,L"PDF一括印刷",MB_OK); }
static int compare(const void *a,const void *b) {
    return CompareStringOrdinal(*(const wchar_t *const *)a,-1,*(const wchar_t *const *)b,-1,TRUE)-CSTR_EQUAL;
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,LPWSTR args,int show) {
    (void)instance; (void)previous; (void)args; (void)show;
    wchar_t reader[MAX_PATH],folder[MAX_PATH],pattern[MAX_PATH+8],printer[1024];
    DWORD printer_size=1024;
    GetModuleFileNameW(NULL,reader,MAX_PATH);
    wchar_t *slash=wcsrchr(reader,L'\\');
    if (!slash) return 1;
    slash[1]=0;
    if (wcslen(reader)+wcslen(L"SumatraPDF.exe")>=MAX_PATH) return 1;
    wcscat(reader,L"SumatraPDF.exe");
    if(GetFileAttributesW(reader)==INVALID_FILE_ATTRIBUTES) { message(L"SumatraPDF.exeが見つかりません。ZIPをすべて展開してください。"); return 1; }
    if(!GetDefaultPrinterW(printer,&printer_size)) { message(L"Windowsで既定のプリンターを設定してください。"); return 1; }
    if(wcsstr(printer,L"Microsoft Print to PDF") || wcsstr(printer,L"Microsoft XPS") || wcsstr(printer,L"OneNote") || wcsstr(printer,L"Fax")) {
        message(L"既定のプリンターがファイル保存用です。紙に印刷するプリンターを選んでください。"); return 1;
    }
    HRESULT hr=CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);
    if(FAILED(hr)) { message(L"フォルダー選択を開始できません。"); return 1; }
    BROWSEINFOW browse={0};
    browse.lpszTitle=L"印刷するPDFが入ったフォルダーを選んでください";
    browse.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;
    PIDLIST_ABSOLUTE item=SHBrowseForFolderW(&browse);
    if(!item) { CoUninitialize(); return 0; }
    BOOL valid=SHGetPathFromIDListW(item,folder);
    CoTaskMemFree(item); CoUninitialize();
    if(!valid) { message(L"このフォルダーは使用できません。"); return 1; }
    swprintf(pattern,MAX_PATH+8,L"%ls\\*.pdf",folder);
    WIN32_FIND_DATAW data;
    HANDLE search=FindFirstFileW(pattern,&data);
    if(search==INVALID_HANDLE_VALUE) { message(L"PDFが見つかりません。フォルダーにアクセスできることを確認してください。"); return 1; }
    wchar_t **files=NULL; size_t count=0;
    do {
        if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        const wchar_t *extension=wcsrchr(data.cFileName,L'.');
        if(!extension || _wcsicmp(extension,L".pdf")!=0) continue;
        wchar_t **next=realloc(files,(count+1)*sizeof(*files));
        if(!next) { FindClose(search); message(L"メモリーが不足しています。"); return 1; }
        files=next; files[count]=_wcsdup(data.cFileName);
        if(!files[count]) { FindClose(search); message(L"メモリーが不足しています。"); return 1; }
        count++;
    } while(FindNextFileW(search,&data));
    DWORD enumeration_error=GetLastError(); FindClose(search);
    if(enumeration_error!=ERROR_NO_MORE_FILES) { message(L"PDF一覧を取得できませんでした。"); return 1; }
    if(!count) { message(L"このフォルダーにはPDFがありません。"); free(files); return 0; }
    qsort(files,count,sizeof(*files),compare);
    size_t capacity=4096;
    for(size_t i=0;i<count;i++) capacity+=wcslen(files[i])+2;
    wchar_t *summary=calloc(capacity,sizeof(wchar_t));
    if(!summary) { message(L"メモリーが不足しています。"); return 1; }
    swprintf(summary,capacity,L"プリンター: %ls\nフォルダー: %ls\n%llu件\n\n",printer,folder,(unsigned long long)count);
    // Keep large selections readable; the count and folder always identify the complete scope.
    size_t shown=count>20?20:count;
    for(size_t i=0;i<shown;i++) { wcscat(summary,files[i]); wcscat(summary,L"\n"); }
    if(shown<count) wcscat(summary,L"…（ほかのPDFもすべて対象）\n");
    wcscat(summary,L"\nまとめて印刷しますか？");
    int answer=MessageBoxW(NULL,summary,L"PDF一括印刷",MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION);
    free(summary);
    if(answer==IDYES) {
        size_t sent=0;
        ULONGLONG begin=GetTickCount64();
        size_t batches=0;
        while(sent<count) {
            // Windows command lines are limited to 32767 UTF-16 code units.
            wchar_t command[32000];
            swprintf(command,32000,L"\"%ls\" -print-to \"%ls\" -silent",reader,printer);
            size_t end=sent;
            while(end<count) {
                size_t needed=wcslen(folder)+wcslen(files[end])+4;
                if(wcslen(command)+needed>=32000) break;
                wcscat(command,L" \""); wcscat(command,folder);
                wcscat(command,L"\\"); wcscat(command,files[end]); wcscat(command,L"\"");
                end++;
            }
            if(end==sent) { message(L"ファイルパスが長すぎます。"); break; }
            STARTUPINFOW startup={0}; startup.cb=sizeof(startup);
            PROCESS_INFORMATION process={0};
            DWORD code=1;
            BOOL started=CreateProcessW(reader,command,NULL,NULL,FALSE,0,NULL,NULL,&startup,&process);
            if(started) {
                WaitForSingleObject(process.hProcess,INFINITE);
                GetExitCodeProcess(process.hProcess,&code);
                CloseHandle(process.hThread); CloseHandle(process.hProcess);
            } else code=GetLastError();
            if(!started || code!=0) {
                wchar_t error[1024];
                swprintf(error,1024,L"印刷処理でエラーが発生しました（コード %lu）。\n正常に送信できた前のまとまり: %llu件。\n今回の対象: %ls ～ %ls\n今回の中にも送信済みがある可能性があります。印刷待ち一覧を確認してから再実行してください。",code,(unsigned long long)sent,files[sent],files[end-1]);
                message(error); break;
            }
            sent=end;
            batches++;
        }
        if(sent==count) {
            wchar_t result[512];
            swprintf(result,512,L"%llu件をプリンターへ送信しました（%.1f秒）。\n印刷ソフトの起動: %llu回\n紙への印刷状況はプリンターの印刷待ち一覧で確認してください。",(unsigned long long)sent,(GetTickCount64()-begin)/1000.0,(unsigned long long)batches);
            message(result);
        }
    }
    for(size_t i=0;i<count;i++) free(files[i]);
    free(files); return 0;
}
