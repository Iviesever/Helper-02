#pragma once

#include "question.h" 
#include "platform_utils.h"
#include "parser/parser_strategy.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <optional> 
#include <functional>
#include <string_view>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

class storage_manager
{
public:

    explicit storage_manager(std::filesystem::path root_path = {})
    {
        // 如果没有传入路径，则为默认路径
        if(root_path.empty())
        {
            root_path_ = platform_utils::get_app_data_path();
        }
        else
        {
            root_path_ = std::move(root_path);
        }
        config_root_path_ = root_path_; // 默认: 配置和数据在同一位置

        load_all();
    }
    ~storage_manager() = default;

    storage_manager(const storage_manager &) = delete;
    storage_manager & operator=(const storage_manager &) = delete;

    storage_manager(storage_manager &&) = default;
    storage_manager & operator=(storage_manager &&) = default;

    // 配置 (Config.json)
    const app_config & config() const { return config_; }
    void update_config(const app_config & new_config);

    // 考试配置 (exam_configs.json) - 支持多配置
    const exam_config & get_exam_config() const { return exam_config_; }
    std::vector<std::string> get_exam_config_names() const;           // 获取所有配置名称
    bool load_exam_config_by_name(const std::string & name);          // 按名称加载
    void save_exam_config_as(const std::string & name, const exam_config & cfg); // 保存为指定名称
    void delete_exam_config(const std::string & name);                // 删除指定配置
    void set_current_exam_config(const exam_config & cfg) { exam_config_ = cfg; }

    // 解析策略管理 (parser_strategies.json)
    std::vector<std::string> get_parser_strategy_names() const;
    std::optional<parser_strategy> get_parser_strategy(const std::string& name) const;
    void save_parser_strategy(const parser_strategy& strategy);
    void delete_parser_strategy(const std::string& name);
    parser_strategy get_default_strategy() const { return parser_strategy::get_default(); }

    // 错题 (Mistakes.json)
    void add_mistake(const question &);
    int get_mistake_count(const question &) const;
    size_t get_max_mistake()const { return max_mistake_; }
    std::vector<std::pair<question, int>> filter_mistakes(const std::vector<question> & all_questions) const;

    // 历史 (History.json)
    void add_exam_record(const exam_record & record);
    std::vector<exam_record> get_history(const std::string & repo_name) const;

    // 刷题策略 (practice_strategies.json) - 按题库保存
    practice_strategy get_practice_strategy(const std::string& repo_name) const;
    void save_practice_strategy(const std::string& repo_name, const practice_strategy& strategy);

private:

    std::filesystem::path get_json_path(std::string_view filename) const
    {
        if(filename == config_file_) return config_root_path_ / filename;
        return root_path_ / filename;
    }

    std::optional<QJsonObject> get_json_object(std::string_view json_file) const
    {
        auto path = get_json_path(json_file);
        QFile file(platform_utils::to_q_path(path));

        if(!file.open(QIODevice::ReadOnly))// 文件不存在或无法打开
        {
            return std::nullopt;
        }

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        if(doc.isNull() || !doc.isObject()) // JSON 格式错误或文件为空
        {
            return std::nullopt;
        }

        return doc.object();
    }


    void modify_json(std::string_view filename, QJsonDocument::JsonFormat format, std::function<void(QJsonObject &)> modifier)
    {
        QJsonObject root_obj;

        modifier(root_obj);

        // 写入
        auto path = get_json_path(filename);
        QFile file(platform_utils::to_q_path(path));

        // WriteOnly 会清空文件内容，准备重写
        if(file.open(QIODevice::WriteOnly))
        {
            file.write(QJsonDocument(root_obj).toJson(format));
        }
    }


    void load_all()
    {
        // 确保根目录存在
        QDir dir(platform_utils::to_q_path(root_path_));
        if(!dir.exists())
        {
            dir.mkpath(".");
        }

        load_config();
        // exam_config 使用默认值，用户可从 UI 加载已保存配置
        exam_config_ = { 10, 5, 5, 5, 2.0, 4.0, 2.0, 2.0, 45 };
        load_mistakes();
    }

    void load_config();
    void save_config();

    void load_mistakes();
    void save_mistakes();


    

    std::filesystem::path root_path_;
    std::filesystem::path config_root_path_; // 配置文件固定路径
    app_config config_;
    exam_config exam_config_;
    std::unordered_map<size_t, size_t> mistakes_;
    size_t max_mistake_{};

    static constexpr std::string_view config_file_ = "config.json";
    static constexpr std::string_view exam_configs_file_ = "exam_configs.json";
    static constexpr std::string_view mistake_file_ = "mistakes.json";
    static constexpr std::string_view history_file_ = "history.json";
    static constexpr std::string_view parser_strategies_file_ = "parser_strategies.json";
    static constexpr std::string_view practice_strategies_file_ = "practice_strategies.json";
};