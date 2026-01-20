#include "platform_utils.h"

// C++ 标准库
#include <algorithm>
#include <print>
#include <filesystem> // 仅保留用于返回类型的定义

// Qt 依赖
#include <QStandardPaths>
#include <QCoreApplication>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QRegularExpression>
#include <QCollator>

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#endif

namespace fs = std::filesystem;

static QString g_repo_path_override;

// fs::path 转 QString
QString platform_utils::to_q_path(const std::filesystem::path & path)
{
#if defined(Q_OS_WIN)
    return QString::fromStdWString(path.wstring());
#else
    return QString::fromStdString(path.string());
#endif
}

// QString 转 fs::path 
fs::path platform_utils::to_fs_path(const QString & qstr)
{
#if defined(Q_OS_WIN)
    return fs::path(qstr.toStdWString());
#else
    return fs::path(qstr.toStdString());
#endif
}

// 应用程序的可写数据路径
std::filesystem::path platform_utils::get_app_data_path()
{
    // Windows  C:\Users\<用户名>\AppData\Roaming\<应用名>
    // Android  /data/user/0/<包名>/files
    QString q_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return to_fs_path(q_path);
}

// 获取题库路径
fs::path platform_utils::get_repo_path()
{
    return to_fs_path(get_repo_q_path());
}

// 获取题库路径 QString
QString platform_utils::get_repo_q_path()
{
    if(!g_repo_path_override.isEmpty()) return g_repo_path_override;

    QString q_base_path;

#if defined(Q_OS_ANDROID)
    QString download_path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QDir dir(download_path);
    q_base_path = dir.filePath("题库");
#else
    // Windows/Desktop: exe 同级目录下的 "题库" 文件夹
    QString app_dir = QCoreApplication::applicationDirPath();
    QDir dir(app_dir);
    q_base_path = dir.filePath("题库");
#endif

    // QDir 确保目录存在
    QDir target_dir(q_base_path);
    if(!target_dir.exists())
    {
        if(!target_dir.mkpath("."))
        {
            qWarning() << "Failed to create directory:" << q_base_path;
        }
    }

    return q_base_path;
}

void platform_utils::set_repo_path_override(const QString & path)
{
    g_repo_path_override = path;
}

// 获取题库文件夹
std::vector<std::string> platform_utils::get_repo_dir()
{
    std::vector<std::string> repos{};

    QString root_path = get_repo_q_path();
    QDir root_dir(root_path);

    if(!root_dir.exists()) return repos;

    root_dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot); // 只要文件夹，不要 . 和 ..
    root_dir.setSorting(QDir::Name);

    QStringList entry_list = root_dir.entryList(); // 返回符合过滤条件的文件夹名称列表

    for(const QString & entry : entry_list)
    {
        repos.push_back(entry.toStdString());
    }

    return repos;
}

// 获取题库文件
std::vector<std::string> platform_utils::get_repo_file(const std::string & repo_name)
{
    std::vector<std::string> files;

    QDir root_dir(get_repo_q_path());

    // 拼接路径
    QString full_repo_path = root_dir.filePath(QString::fromStdString(repo_name));

    QDir target_dir(full_repo_path);
    if(!target_dir.exists()) return files;

    // 获取文件信息列表
    QFileInfoList info_list = target_dir.entryInfoList({ "*.txt" }, QDir::Files | QDir::NoDotAndDotDot, QDir::NoSort);

    // 自然排序 (1.txt, 2.txt, 10.txt)
    std::sort(info_list.begin(), info_list.end(), [&](const QFileInfo & a, const QFileInfo & b)
        {
             QString na = a.fileName();
             QString nb = b.fileName();
             
             static QRegularExpression re("(\\d+)");
             auto matchA = re.match(na);
             auto matchB = re.match(nb);
             
             if(matchA.hasMatch() && matchB.hasMatch()) {
                 int numA = matchA.captured(1).toInt();
                 int numB = matchB.captured(1).toInt();
                 if(numA != numB) return numA < numB;
             }
             
             return na < nb;
        });


    //  最终转换为 std::vector<std::string>
    files.reserve(info_list.size());
    for(const QFileInfo & info : info_list)
    {
        files.push_back(info.absoluteFilePath().toStdString());
    }

    return files;
}

void platform_utils::request_android_permissions()
{
#if defined(Q_OS_ANDROID)
    // Android 11+ (API 30+) 需要 MANAGE_EXTERNAL_STORAGE 权限以获得广泛的访问权限，
    // 或者特定目录的标准权限。
    // 用户提供的 'isExternalStorageManager' 逻辑。
    
    auto sdkInt = QJniObject::getStaticField<jint>("android/os/Build$VERSION", "SDK_INT");

    if(sdkInt >= 30)
    { 
        auto isManager = QJniObject::callStaticMethod<jboolean>(
            "android/os/Environment", "isExternalStorageManager");

        if(!isManager)
        {
            // 1. 构建 Intent: android.settings.MANAGE_APP_ALL_FILES_ACCESS_PERMISSION
            QJniObject action = QJniObject::fromString("android.settings.MANAGE_APP_ALL_FILES_ACCESS_PERMISSION");
            QJniObject intent("android/content/Intent", "(Ljava/lang/String;)V", action.object());

            // 2. 获取 Context
            QJniObject context = QNativeInterface::QAndroidApplication::context();

            // 3. 获取包名
            QJniObject packageName = context.callObjectMethod("getPackageName", "()Ljava/lang/String;");

            // 4. 构建 URI: package:com.example.xxx
            QJniObject uriPrefix = QJniObject::fromString("package:");
            QJniObject uriString = uriPrefix.callObjectMethod("concat", "(Ljava/lang/String;)Ljava/lang/String;", packageName.object());

            QJniObject uri = QJniObject::callStaticObjectMethod("android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;", uriString.object());

            // 5. 设置 Intent 的 Data
            intent.callObjectMethod("setData", "(Landroid/net/Uri;)Landroid/content/Intent;", uri.object());

            // 6. 启动 Activity
            context.callMethod<void>("startActivity", "(Landroid/content/Intent;)V", intent.object());
        }
    }
#endif
}