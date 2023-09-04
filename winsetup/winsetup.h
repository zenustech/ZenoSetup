#ifndef WINSETUP_H
#define WINSETUP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*winsetup_progress_cb)(float p,void *ud);

struct winsetup_app_info {
    const char *display_icon;
    const char *display_name;
    const char *display_version;
    size_t app_size;
    const char *install_date;
    const char *uninstall_path;
};

// 将zip包替换到指定目录，对于无法覆盖的文件会尝试将其改名然后再写入
int winsetup_put_res(const char *zip_path,const char *put_path,winsetup_progress_cb cb,void *ud);
// 删除目录
void winsetup_rm_res(const char *path);
// 写入卸载信息到注册表HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\key
int winsetup_put_app_info(const char *key,const struct winsetup_app_info *info);
// 删除卸载信息
void winsetup_rm_app_info(const char *key);
// 创建桌面快捷方式
int winsetup_create_desktop_link(const char *display_name,const char *exe_path);
// 创建开始菜单栏快捷方式
int winsetup_create_menu_link(const char *name,const char *exe_path,const char *unins_path);
// 创建快捷方式
int winsetup_create_link(const char *display_name,const char *link_dir,const char *target_path);


#ifdef __cplusplus
}
#endif

#endif // WINSETUP_H
