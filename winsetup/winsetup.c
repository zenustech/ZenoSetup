#include "winsetup.h"
#include "zip.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <Windows.h>
#include <ShlObj.h>

#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"shell32.lib")

static void local_to_utf8(const char *str,char *buf,int nbuf) {
    wchar_t wbuf[1024] = {0};
    int n = MultiByteToWideChar(CP_ACP,0,str,(int)strlen(str),wbuf,1024);
    WideCharToMultiByte(CP_UTF8,0,wbuf,n,buf,nbuf,NULL,NULL);
}

static void mk_file_dir(const char *path) {
    char buf[1024];
    for (int i = 0;i < 1024 && path[i];i++) {
        if (path[i] == '/' || path[i] == '\\') {
            buf[i] = 0;
            CreateDirectoryA(buf,NULL);
        }
        buf[i] = path[i];
    }
}

int winsetup_put_res(const char *zip_path, const char *put_path,winsetup_progress_cb cb,void *ud) {
    char buf[1024];
    local_to_utf8(zip_path,buf,sizeof(buf));
    struct zip_t *zip = zip_open(buf,ZIP_DEFAULT_COMPRESSION_LEVEL,'r');
    if (!zip) {
        perror("open zip_path fail!");
        return -1;
    }
    int count = (int)zip_entries_total(zip);
    char cwd[1024];
    CreateDirectoryA(put_path,NULL);
    GetCurrentDirectoryA(sizeof(cwd),cwd);
    SetCurrentDirectoryA(put_path);
    for (int i = 0;i < count;i++) {
        zip_entry_openbyindex(zip,i);
        const char *name = zip_entry_name(zip);
        int ret = 0;
        if (zip_entry_isdir(zip)) {
            size_t len = strlen(name);
            memcpy(buf,name,len);
            // last char is '/'
            buf[len] = 0;
            ret = CreateDirectoryA(buf,NULL) ? 0 : -1;
        }
        else {
            char utf8_name[1024] = {0};
            local_to_utf8(name,utf8_name,sizeof(utf8_name));
            snprintf(buf,sizeof(buf),"%s.old",name);
            remove(buf);
            mk_file_dir(name);
            int ret = zip_entry_fread(zip,utf8_name);
            if (ret != 0) {
                rename(name,buf);
                ret = zip_entry_fread(zip,utf8_name);
            }
        }
        printf("put %s\\%s %s\n",put_path,name,(ret == 0 ? "true" : "false"));
        zip_entry_close(zip);
        if (cb) {
            float p = (float)(i + 1) / count;
            cb(p,ud);
        }
    }
    zip_close(zip);
    SetCurrentDirectoryA(cwd);
    return 0;
}

static void safe_remove(const char *path) {
    if (strstr(path,"\\..") || strstr(path,"/..")) {
        fprintf(stderr,"dange path %s !!!\n",path);
        return;
    }
    if(!DeleteFileA(path))
        RemoveDirectoryA(path);
    printf("remove %s\n",path);
}

void winsetup_rm_res(const char *path) {
    char buf[1024];
    snprintf(buf,sizeof(buf),"%s\\*.*",path);
    WIN32_FIND_DATAA wfd;
    HANDLE hd = FindFirstFileA(buf,&wfd);
    if (hd == INVALID_HANDLE_VALUE) return;
    do {
        if (strcmp(wfd.cFileName,".") == 0 || strcmp(wfd.cFileName,"..") == 0)
            continue;
        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            snprintf(buf,sizeof(buf),"%s\\%s",path,wfd.cFileName);
            winsetup_rm_res(buf);
        }
        else {
            snprintf(buf,sizeof(buf),"%s\\%s",path,wfd.cFileName);
            safe_remove(buf);
        }

    } while(FindNextFileA(hd,&wfd));
    FindClose(hd);
    safe_remove(path);
}

int winsetup_put_app_info(const char *key, const struct winsetup_app_info *info) {
    HKEY hkey = 0;
    char buf[1024];
    snprintf(buf,sizeof(buf),"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s",key);
    RegCreateKeyExA(HKEY_CURRENT_USER,buf,0,NULL,REG_OPTION_NON_VOLATILE,NULL,NULL,&hkey,NULL);
    RegCloseKey(hkey);
    RegOpenKeyExA(HKEY_CURRENT_USER,buf,0,KEY_READ | KEY_WRITE | KEY_QUERY_VALUE,&hkey);
    if (!hkey) return -1;
    LSTATUS ret;
    ret = RegSetValueExA(hkey,"DisplayIcon",0,REG_SZ,info->display_icon,(DWORD)strlen(info->display_icon));
    ret = RegSetValueExA(hkey,"DisplayName",0,REG_SZ,info->display_name,(DWORD)strlen(info->display_name));
    ret = RegSetValueExA(hkey,"DisplayVersion",0,REG_SZ,info->display_version,(DWORD)strlen(info->display_version));
    ret = RegSetValueExA(hkey,"InstallDate",0,REG_SZ,info->install_date,(DWORD)strlen(info->install_date));
    ret = RegSetValueExA(hkey,"UninstallString",0,REG_SZ,info->uninstall_path,(DWORD)strlen(info->uninstall_path));
    DWORD size = (DWORD)info->app_size;
    ret = RegSetValueExA(hkey,"EstimatedSize",0,REG_DWORD,&size,sizeof(DWORD));
    RegCloseKey(hkey);
    return ret == ERROR_SUCCESS ? 0 : -1;
}

void winsetup_rm_app_info(const char *key) {
    char buf[1024];
    snprintf(buf,sizeof(buf),"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s",key);
    RegDeleteKeyA(HKEY_CURRENT_USER,buf);
}

static char* expandev(const char *path,char *buf,size_t nbuf) {
    if (!buf) {
        nbuf = 1024;
        buf = malloc(nbuf);
    }
    ExpandEnvironmentStringsA(path,buf,(DWORD)nbuf);
    return buf;
}

int winsetup_create_link(const char *display_name,
                          const char *link_dir,
                          const char *target_path) {
    IShellLinkA *slink = NULL;
    IPersistFile *psf = NULL;
    CoInitialize(0);
    CoCreateInstance(&CLSID_ShellLink,NULL,CLSCTX_ALL,&IID_IShellLinkA,&slink);
    if (!slink) {
        fprintf(stderr,"create IShellLink obj fail\n");
        return -1;
    }
    slink->lpVtbl->SetPath(slink,target_path);
    slink->lpVtbl->SetWorkingDirectory(slink,target_path);
    slink->lpVtbl->QueryInterface(slink,&IID_IPersistFile,&psf);
    if (!psf) {
        fprintf(stderr,"create IPersistFile obj fail\n");
        slink->lpVtbl->Release(slink);
        return -1;

    }
    char buf[1024] = {0};
    strcat(buf,link_dir);
    strcat(buf,"\\");
    strcat(buf,display_name);
    strcat(buf,".lnk");
    char *dest_path = expandev(buf,NULL,0);
    remove(dest_path);
    mk_file_dir(dest_path);
    wchar_t buf2[1024] = {0};
    MultiByteToWideChar(CP_ACP,0,dest_path,(int)strlen(dest_path),buf2,sizeof(buf2));
    free(dest_path);
    psf->lpVtbl->Save(psf,buf2,TRUE);
    psf->lpVtbl->Release(psf);
    slink->lpVtbl->Release(slink);
    CoUninitialize();
    return 0;
}

int winsetup_create_desktop_link(const char *display_name, const char *exe_path) {
    char dir[1024];
    SHGetFolderPathA(NULL,CSIDL_DESKTOP,NULL,SHGFP_TYPE_CURRENT,dir);
    return winsetup_create_link(display_name,dir,exe_path);
}

int winsetup_create_menu_link(const char *name, const char *exe_path, const char *unins_path) {
    char dir[1024];
    SHGetFolderPathA(NULL,CSIDL_PROGRAMS,NULL,SHGFP_TYPE_CURRENT,dir);
    strcat(dir,"\\");
    strcat(dir,name);
    int r = winsetup_create_link(name,dir,exe_path);
    r = winsetup_create_link("卸载",dir,unins_path);
    return r;
}
