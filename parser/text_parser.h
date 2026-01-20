#pragma once

#include "../question.h"
#include "parser_strategy.h"
#include <string_view>
#include <vector>
#include <regex>
#include <optional>

// 文本解析器
class text_parser
{
public:
    explicit text_parser(parser_strategy strategy = parser_strategy::get_default());

    // 核心接口: 解析内存块 -> 题目列表
    [[nodiscard]] std::vector<question> parse(std::string_view content, std::string_view file_name = "") const;

    const parser_strategy& strategy() const { return strategy_; }

private:
    parser_strategy strategy_;

    // 编译的正则表达式用于复杂匹配 (垃圾行过滤)
    std::vector<std::regex> re_garbage_list_;
    
    // 内部编译
    void compile_patterns();

    // 辅助函数 (基于 string_view)
    std::vector<std::string_view> split_lines(std::string_view text) const;
    std::string_view trim(std::string_view sv) const;
    bool is_garbage_line(std::string_view line) const;
    bool is_question_start(std::string_view line) const;
    
    // 解析逻辑
    struct block_info {
        std::string_view content;
        size_t start_line; // 可选的调试信息
    };
    
    question parse_single_block(std::string_view block, std::string_view file_name) const;

    // 检测辅助函数
    bool contains_keyword(std::string_view text, std::string_view keywords_csv) const;
    question_type detect_type(std::string_view block, std::string_view answer_raw) const;
    bool is_judge_true(std::string_view answer) const;
    bool is_judge_false(std::string_view answer) const;
};
