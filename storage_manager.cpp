#include "storage_manager.h"



#include <QJsonArray>
#include <QDebug>


// config.json
void storage_manager::load_config()
{
    if(auto obj_opt = get_json_object(config_file_))
    {
        const auto & obj = *obj_opt;

        // 读取所有配置项，使用默认值
        config_.font_size = obj.value("font_size").toInt(12);
        config_.button_size = obj.value("button_size").toInt(12);
        config_.auto_submit = obj.value("auto_submit").toBool(false);
        config_.confirm_exit = obj.value("confirm_exit").toBool(true);
        config_.auto_next = obj.value("auto_next").toBool(false);
        config_.file_list_limit = (size_t)obj.value("file_list_limit").toInt(5);
        config_.custom_repo_path = obj.value("custom_repo_path").toString().toStdString();
        config_.custom_data_path = obj.value("custom_data_path").toString().toStdString();
        config_.dark_mode = obj.value("dark_mode").toBool(false);

        // 应用自定义数据路径
        if(!config_.custom_data_path.empty())
        {
             QString qpath = QString::fromStdString(config_.custom_data_path);
             root_path_ = platform_utils::to_fs_path(qpath);
             QDir dir(qpath);
             if(!dir.exists()) dir.mkpath(".");
        }

        // 应用自定义题库路径
        if(!config_.custom_repo_path.empty())
        {
             platform_utils::set_repo_path_override(QString::fromStdString(config_.custom_repo_path));
        }
    }
}

void storage_manager::save_config()
{
    modify_json(config_file_, QJsonDocument::Indented, [this](QJsonObject & obj)
        {
            // 先读取现有配置，避免覆盖考试配置等
            obj = get_json_object(config_file_).value_or(QJsonObject{});
            
            obj["font_size"] = config_.font_size;
            obj["button_size"] = config_.button_size;
            obj["auto_submit"] = config_.auto_submit;
            obj["confirm_exit"] = config_.confirm_exit;
            obj["auto_next"] = config_.auto_next;
            obj["file_list_limit"] = (int)config_.file_list_limit;
            obj["custom_repo_path"] = QString::fromStdString(config_.custom_repo_path);
            obj["custom_data_path"] = QString::fromStdString(config_.custom_data_path);
            obj["dark_mode"] = config_.dark_mode;
        });
}

void storage_manager::update_config(const app_config & new_config)
{
    if(config_ == new_config) return;

    config_ = new_config;
    save_config();
}

// exam_configs.json - 多配置管理

std::vector<std::string> storage_manager::get_exam_config_names() const
{
    std::vector<std::string> names;
    
    if(auto obj_opt = get_json_object(exam_configs_file_))
    {
        const auto & obj = *obj_opt;
        for(auto it = obj.begin(); it != obj.end(); ++it)
        {
            names.push_back(it.key().toStdString());
        }
    }
    
    return names;
}

bool storage_manager::load_exam_config_by_name(const std::string & name)
{
    if(auto obj_opt = get_json_object(exam_configs_file_))
    {
        const auto & obj = *obj_opt;
        QString qname = QString::fromStdString(name);
        
        if(obj.contains(qname))
        {
            QJsonObject ex = obj[qname].toObject();
            exam_config_ = 
            {
                (size_t)ex.value("cnt_single").toInt(10),
                (size_t)ex.value("cnt_multi").toInt(5),
                (size_t)ex.value("cnt_judge").toInt(5),
                (size_t)ex.value("cnt_fill").toInt(5),
                ex.value("sc_single").toDouble(2.0),
                ex.value("sc_multi").toDouble(4.0),
                ex.value("sc_judge").toDouble(2.0),
                ex.value("sc_fill").toDouble(2.0),
                (size_t)ex.value("duration").toInt(45)
            };
            return true;
        }
    }
    return false;
}

void storage_manager::save_exam_config_as(const std::string & name, const exam_config & cfg)
{
    modify_json(exam_configs_file_, QJsonDocument::Indented, [&](QJsonObject & obj)
    {
        // 先读取现有配置
        obj = get_json_object(exam_configs_file_).value_or(QJsonObject{});
        
        QJsonObject ex;
        ex["cnt_single"] = (qint64)cfg.single_count;
        ex["cnt_multi"] = (qint64)cfg.multi_count;
        ex["cnt_judge"] = (qint64)cfg.judge_count;
        ex["cnt_fill"] = (qint64)cfg.fill_count;
        ex["sc_single"] = cfg.single_score;
        ex["sc_multi"] = cfg.multi_score;
        ex["sc_judge"] = cfg.judge_score;
        ex["sc_fill"] = cfg.fill_score;
        ex["duration"] = (qint64)cfg.exam_duration;

        obj[QString::fromStdString(name)] = ex;
    });
    
    exam_config_ = cfg;
}

void storage_manager::delete_exam_config(const std::string & name)
{
    modify_json(exam_configs_file_, QJsonDocument::Indented, [&](QJsonObject & obj)
    {
        obj = get_json_object(exam_configs_file_).value_or(QJsonObject{});
        obj.remove(QString::fromStdString(name));
    });
}

// mistakes.json

void storage_manager::load_mistakes()
{
    mistakes_.clear();

    max_mistake_ = 0;

    if(auto obj_opt = get_json_object(mistake_file_))
    {
        const QJsonObject & obj = *obj_opt;

        for(auto it = obj.begin(); it != obj.end(); ++it)
        {
            size_t id = it.key().toULongLong();
            int count = it.value().toInt();
            mistakes_[id] = count;

            if(static_cast<size_t>(count) > max_mistake_) max_mistake_ = static_cast<size_t>(count);
        }
    }
}

void storage_manager::save_mistakes()
{
    modify_json(mistake_file_, QJsonDocument::Compact, [this](QJsonObject & obj)
        {
            for(const auto & [id, count] : mistakes_)
            {
                obj[QString::number(id)] = static_cast<qint64>(count);
            }
        });
}

void storage_manager::add_mistake(const question & q)
{
    if(auto current_count = ++mistakes_[q.get_id()]; current_count > max_mistake_)
    {
        max_mistake_ = current_count;
    }

    save_mistakes();
}

int storage_manager::get_mistake_count(const question & q) const
{
    size_t id = q.get_id();
    if(mistakes_.contains(id))
        return static_cast<int>(mistakes_.at(id));
    return 0;
}



std::vector<std::pair<question, int>> storage_manager::filter_mistakes(const std::vector<question> & all_questions) const
{
    std::vector<std::pair<question, int>> result;

    result.reserve(mistakes_.size());

    for(const auto & q : all_questions)
    {
        if(size_t id = q.get_id(); mistakes_.contains(id))
        {
            int count = static_cast<int>(mistakes_.at(id));
            result.push_back({ q, count });
        }
    }

    return result;
}


// history.json

void storage_manager::add_exam_record(const exam_record & record)
{
    modify_json(history_file_, QJsonDocument::Indented, [&](QJsonObject & obj)
        {
            obj = get_json_object(history_file_).value_or(QJsonObject{});

            QJsonObject new_rec;
            new_rec["date"] = QString::fromStdString(record.date);
            new_rec["score"] = record.score;
            new_rec["total_score"] = record.total_score;
            new_rec["duration"] = record.duration_sec;
            new_rec["correct"] = record.correct_count;
            new_rec["total"] = record.total_count;

            QString q_repo_name = QString::fromStdString(record.repo_name);
            QJsonArray arr = obj.value(q_repo_name).toArray(); // 根据题库名查找对应的数组
            arr.prepend(new_rec);                              // 新记录放在最前
            obj[q_repo_name] = arr;                            // 更新后的数组放回对象
        });

}

std::vector<exam_record> storage_manager::get_history(const std::string & repo_name) const
{
    std::vector<exam_record> list;

    if(auto obj_opt = get_json_object(history_file_))
    {
        const auto & obj = obj_opt.value();

        QString q_repo_name = QString::fromStdString(repo_name);

        if(obj.contains(q_repo_name))
        {
            QJsonArray arr = obj.value(q_repo_name).toArray();
            list.reserve(arr.size());

            for(const auto & val : arr)
            {
                QJsonObject o = val.toObject();
                exam_record r;
                r.repo_name = repo_name;
                r.date = o["date"].toString().toStdString();
                r.score = o["score"].toDouble();
                r.total_score = o["total_score"].toDouble();
                r.duration_sec = o["duration"].toInt();
                r.correct_count = o["correct"].toInt();
                r.total_count = o["total"].toInt();
                list.push_back(std::move(r));
            }
        }
    }
    return list;
}

// parser_strategies.json - 解析策略管理

std::vector<std::string> storage_manager::get_parser_strategy_names() const
{
    std::vector<std::string> names;
    
    if(auto obj_opt = get_json_object(parser_strategies_file_))
    {
        const auto& obj = *obj_opt;
        for(auto it = obj.begin(); it != obj.end(); ++it)
        {
            names.push_back(it.key().toStdString());
        }
    }
    
    return names;
}

std::optional<parser_strategy> storage_manager::get_parser_strategy(const std::string& name) const
{
    if(auto obj_opt = get_json_object(parser_strategies_file_))
    {
        const auto& obj = *obj_opt;
        QString qname = QString::fromStdString(name);
        
        if(obj.contains(qname))
        {
            QJsonObject s = obj[qname].toObject();
            parser_strategy strategy;
            strategy.name = name;
            strategy.single_keywords = s.value("single_keywords").toString().toStdString();
            strategy.multi_keywords = s.value("multi_keywords").toString().toStdString();
            strategy.judge_keywords = s.value("judge_keywords").toString().toStdString();
            strategy.fill_keywords = s.value("fill_keywords").toString().toStdString();
            strategy.answer_keywords = s.value("answer_keywords").toString().toStdString();
            strategy.garbage_patterns = s.value("garbage_patterns").toString().toStdString();
            strategy.judge_true_values = s.value("judge_true_values").toString().toStdString();
            strategy.judge_false_values = s.value("judge_false_values").toString().toStdString();
            return strategy;
        }
    }
    return std::nullopt;
}

void storage_manager::save_parser_strategy(const parser_strategy& strategy)
{
    modify_json(parser_strategies_file_, QJsonDocument::Indented, [&](QJsonObject& obj)
    {
        // 先读取现有配置
        obj = get_json_object(parser_strategies_file_).value_or(QJsonObject{});
        
        QJsonObject s;
        s["single_keywords"] = QString::fromStdString(strategy.single_keywords);
        s["multi_keywords"] = QString::fromStdString(strategy.multi_keywords);
        s["judge_keywords"] = QString::fromStdString(strategy.judge_keywords);
        s["fill_keywords"] = QString::fromStdString(strategy.fill_keywords);
        s["answer_keywords"] = QString::fromStdString(strategy.answer_keywords);
        s["garbage_patterns"] = QString::fromStdString(strategy.garbage_patterns);
        s["judge_true_values"] = QString::fromStdString(strategy.judge_true_values);
        s["judge_false_values"] = QString::fromStdString(strategy.judge_false_values);

        obj[QString::fromStdString(strategy.name)] = s;
    });
}

void storage_manager::delete_parser_strategy(const std::string& name)
{
    modify_json(parser_strategies_file_, QJsonDocument::Indented, [&](QJsonObject& obj)
    {
        obj = get_json_object(parser_strategies_file_).value_or(QJsonObject{});
        obj.remove(QString::fromStdString(name));
    });
}

// practice_strategies.json - 刷题策略管理（按题库）

practice_strategy storage_manager::get_practice_strategy(const std::string& repo_name) const
{
    practice_strategy result;
    
    if(auto obj_opt = get_json_object(practice_strategies_file_))
    {
        const auto& obj = *obj_opt;
        QString qname = QString::fromStdString(repo_name);
        
        if(obj.contains(qname))
        {
            QJsonObject s = obj[qname].toObject();
            result.skip_single_most_common = s.value("skip_single_most_common").toBool(false);
            result.skip_judge_most_common = s.value("skip_judge_most_common").toBool(false);
            result.exclude_duplicates = s.value("exclude_duplicates").toBool(false);
            result.exclude_multi_all = s.value("exclude_multi_all").toBool(false);
            
            // 单选选项跳过
            QJsonArray singleOpts = s.value("skip_single_options").toArray();
            for(int i = 0; i < std::min((int)singleOpts.size(), 4); ++i) {
                result.skip_single_options[i] = singleOpts[i].toBool(false);
            }
            
            // 判断选项跳过
            QJsonArray judgeOpts = s.value("skip_judge_options").toArray();
            for(int i = 0; i < std::min((int)judgeOpts.size(), 2); ++i) {
                result.skip_judge_options[i] = judgeOpts[i].toBool(false);
            }
        }
    }
    return result;
}

void storage_manager::save_practice_strategy(const std::string& repo_name, const practice_strategy& strategy)
{
    modify_json(practice_strategies_file_, QJsonDocument::Indented, [&](QJsonObject& obj)
    {
        obj = get_json_object(practice_strategies_file_).value_or(QJsonObject{});
        
        QJsonObject s;
        s["skip_single_most_common"] = strategy.skip_single_most_common;
        s["skip_judge_most_common"] = strategy.skip_judge_most_common;
        s["exclude_duplicates"] = strategy.exclude_duplicates;
        s["exclude_multi_all"] = strategy.exclude_multi_all;
        
        // 单选选项跳过
        QJsonArray singleOpts;
        for(int i = 0; i < 4; ++i) {
            singleOpts.append(strategy.skip_single_options[i]);
        }
        s["skip_single_options"] = singleOpts;
        
        // 判断选项跳过
        QJsonArray judgeOpts;
        for(int i = 0; i < 2; ++i) {
            judgeOpts.append(strategy.skip_judge_options[i]);
        }
        s["skip_judge_options"] = judgeOpts;
        
        obj[QString::fromStdString(repo_name)] = s;
    });
}
