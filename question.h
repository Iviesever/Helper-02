#pragma once
#include <variant>
#include <vector>
#include <format>
#include <string>
#include <algorithm>
#include <ranges>
#include <string_view>

enum class question_type
{
    single,  // 单选
    multi,   // 多选
    judge,   // 判断
    fill,    // 填空
    unknown  
};

enum class answer_state
{
    unanswered,
    correct,
    wrong
};

// FNV-1a 64-bit Hash 算法
// 特点：极快，确定性，纯C++，无依赖，适合作为 ID
constexpr size_t stable_hash(std::string_view s)
{
    size_t hash = 14695981039346656037ull; // FNV 偏移基准
    size_t prime = 1099511628211ull;       // FNV 质数

    for(unsigned char c : s)
    {
        hash ^= c;
        hash *= prime;
    }
    return hash;
}

class question
{
public:

    question_type type = question_type::unknown;
    std::string content;                 // 题目内容 (UTF-8 编码)
    std::vector<std::string> options;    // 选项 A, B, C, D...
    std::string source_file;             // 来源文件名
    std::string correct_answer;          // 正确答案

    size_t get_id() const
    {
        // 组合 Content 和排序后的 Options 生成稳定的 ID
        // 选项先排序，确保选项顺序不同但内容相同的题目生成相同的 ID
        std::string combined = content;
        
        // 复制选项并排序
        std::vector<std::string> sorted_options = options;
        std::ranges::sort(sorted_options);
        
        // 拼接所有选项
        for (const auto& opt : sorted_options)
        {
            combined += "|" + opt;  // 使用分隔符避免歧义
        }
        
        return stable_hash(combined);
    }

    bool operator==(const question & other) const = default;

    static int rank(const question & q)
    {
        switch(q.type)
        {
            case question_type::single: return 0;
            case question_type::multi:  return 1;
            case question_type::judge:  return 2;
            case question_type::fill:   return 3;
            default: return 4;
        }
    }

};

struct mistake
{
    question q;       // 必须存实体！引用会导致崩溃。
    size_t count = 0;
};

struct app_config
{
    int font_size = 12;
    
    int button_size = 12;
    
    bool auto_submit = false;    // 单选/判断题点击选项直接判分
    bool confirm_exit = true;    // 返回主页时显示确认提示
    bool auto_next = false;      // 答对后自动跳转下一题
    size_t file_list_limit = 5;  // 文件列表显示数量限制
    
    std::string custom_repo_path; 
    std::string custom_data_path;

    bool dark_mode = false; // 默认: 浅色模式

    bool operator==(const app_config &) const = default;
};

// 刷题策略 - 按题库保存
struct practice_strategy
{
    bool skip_single_most_common = false;       // 跳过单选最常见答案
    bool skip_judge_most_common = false;        // 跳过判断最常见答案
    std::array<bool, 4> skip_single_options = {};  // 跳过单选 A/B/C/D
    std::array<bool, 2> skip_judge_options = {};   // 跳过判断 A(对)/B(错)
    bool exclude_duplicates = false;            // 排除重复题目
    bool exclude_multi_all = false;             // 排除多选全选题
    
    bool operator==(const practice_strategy&) const = default;
};


struct exam_record
{
    std::string repo_name;       // 题库名
    std::string date;           // 日期 "yyyy-MM-dd HH:mm"
    double score;           // 得分
    double total_score;      // 总分
    int duration_sec;        // 耗时(秒)
    int correct_count;       // 正确题数
    int total_count;         // 总题数
};

struct exam_config
{
    size_t single_count{};
    size_t multi_count{};
    size_t judge_count{};
    size_t fill_count{};

    double single_score{ 1.0 };
    double multi_score{ 2.0 };
    double judge_score{ 1.0 };
    double fill_score{ 1.0 };

    size_t exam_duration{ 60 };
};
