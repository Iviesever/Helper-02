#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <QString>

namespace platform_utils
{
    QString to_q_path(const std::filesystem::path & path);

    std::filesystem::path to_fs_path(const QString & qstr);

    std::filesystem::path get_repo_path();

    QString get_repo_q_path();


    // 获取 App 数据存储路径 (用于存放 json 记录)
    std::filesystem::path get_app_data_path();

    // 获取题库搜索路径 (Windows: ./题库, Android: Download/题库)
    std::filesystem::path get_question_bank_path();

    // 扫描题库目录下的所有子文件夹 (Repo)
    std::vector<std::string> get_repo_dir();

    // 扫描指定 Repo 下的 txt 文件
    std::vector<std::string> get_repo_file(const std::string & repo_name);

    void set_repo_path_override(const QString & path);
    // 检查并请求权限 (仅 Android 需要，Windows 空实现)
    void request_android_permissions();
};