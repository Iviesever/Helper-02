#include "text_parser.h"
#include <ranges>
#include <algorithm>
#include <charconv>
#include <iostream>

// 辅助函数：拆分 string_view 而不使用 ranges::views::split
std::vector<std::string_view> split_view(std::string_view str, char delim) {
    std::vector<std::string_view> result;
    size_t start = 0;
    while (start < str.size()) {
        size_t end = str.find(delim, start);
        if (end == std::string_view::npos) {
            std::string_view part = str.substr(start);
            if (!part.empty() || start < str.size()) result.push_back(part);
            break;
        }
        result.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    return result;
}

text_parser::text_parser(parser_strategy strategy) 
    : strategy_(std::move(strategy))
{
    compile_patterns();
}

void text_parser::compile_patterns()
{
    // 只编译垃圾行过滤的正则表达式
    // 题目识别和选项解析已改为手动 UTF-8 字节处理，不再使用正则表达式
    re_garbage_list_.clear();
    for (auto pattern_sv : split_view(strategy_.garbage_patterns, ','))
    {
        std::string pattern(pattern_sv);
        // 最小化启发式检测正则表达式语法
        if (pattern.find_first_of("^$*+\\") != std::string::npos)
        {
            try {
                re_garbage_list_.emplace_back(pattern, std::regex::optimize);
            } catch (...) {}
        }
    }
}

std::string_view text_parser::trim(std::string_view sv) const
{
    auto first = sv.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    auto last = sv.find_last_not_of(" \t\r\n");
    return sv.substr(first, last - first + 1);
}

// 检测逻辑

bool text_parser::is_garbage_line(std::string_view line) const
{
    auto t = trim(line);
    if (t.empty()) return true;

    // 正则表达式检查 (regex_match 需要遗留转换)
    // 注意: regex_match 需要 std::string 或迭代器。string_view 上的迭代器可以使用。
    for (const auto& re : re_garbage_list_) {
        if (std::regex_match(t.begin(), t.end(), re)) return true;
    }

    // Plain text check
    for (auto pattern_sv : split_view(strategy_.garbage_patterns, ',')) {
        std::string_view pattern = trim(pattern_sv);
        if (pattern.empty()) continue;
        
        // 跳过正则模式
        if (pattern.find_first_of("^$*+\\") != std::string_view::npos) continue;

        if (t.find(pattern) != std::string_view::npos) return true;
    }
    return false;
}

bool text_parser::is_question_start(std::string_view line) const
{
    auto t = trim(line);
    if (t.empty()) return false;
    
    // 第一个字符必须是数字
    if (t[0] < '0' || t[0] > '9') return false;
    
    // 跳过所有数字
    size_t i = 0;
    while (i < t.size() && t[i] >= '0' && t[i] <= '9') i++;
    
    // 必须有数字，且后面还有字符
    if (i == 0 || i >= t.size()) return false;
    
    // 检查数字后面是否是点号（半角或全角）或顿号
    // 半角点: '.' (0x2E)
    if (t[i] == '.') return true;
    
    // 检查 UTF-8 多字节字符
    if (i + 2 < t.size()) {
        unsigned char c1 = static_cast<unsigned char>(t[i]);
        unsigned char c2 = static_cast<unsigned char>(t[i+1]);
        unsigned char c3 = static_cast<unsigned char>(t[i+2]);
        
        // 全角点 ．(U+FF0E) = EF BC 8E
        if (c1 == 0xEF && c2 == 0xBC && c3 == 0x8E) return true;
        
        // 中文顿号 、(U+3001) = E3 80 81
        if (c1 == 0xE3 && c2 == 0x80 && c3 == 0x81) return true;
    }
    
    return false;
}

bool text_parser::contains_keyword(std::string_view text, std::string_view keywords_csv) const
{
    for (auto kw : split_view(keywords_csv, ',')) {
        if (text.find(trim(kw)) != std::string_view::npos) return true;
    }
    return false;
}

bool text_parser::is_judge_true(std::string_view answer) const {
    return contains_keyword(answer, strategy_.judge_true_values);
}

bool text_parser::is_judge_false(std::string_view answer) const {
    return contains_keyword(answer, strategy_.judge_false_values);
}

question_type text_parser::detect_type(std::string_view block, std::string_view answer_raw) const
{
    // 关键词优先级
    if (contains_keyword(block, strategy_.multi_keywords)) return question_type::multi;
    if (contains_keyword(block, strategy_.judge_keywords)) return question_type::judge;
    if (contains_keyword(block, strategy_.fill_keywords)) return question_type::fill;
    if (contains_keyword(block, strategy_.single_keywords)) return question_type::single;

    // 辅助函数：检测选项是否存在（支持半角和全角点）
    auto has_option = [](std::string_view text, char letter) -> bool {
        // 检查半角: "A." 或 "A、"
        for (size_t i = 0; i < text.size(); i++) {
            if (text[i] == letter) {
                if (i + 1 < text.size() && (text[i+1] == '.')) return true;
                // 检查全角点 ．(EF BC 8E) 或 顿号 、(E3 80 81)
                if (i + 3 < text.size()) {
                    unsigned char c1 = static_cast<unsigned char>(text[i+1]);
                    unsigned char c2 = static_cast<unsigned char>(text[i+2]);
                    unsigned char c3 = static_cast<unsigned char>(text[i+3]);
                    if (c1 == 0xEF && c2 == 0xBC && c3 == 0x8E) return true;  // ．
                    if (c1 == 0xE3 && c2 == 0x80 && c3 == 0x81) return true;  // 、
                }
            }
        }
        return false;
    };

    // 答案推断
    if (is_judge_true(answer_raw) || is_judge_false(answer_raw)) {
        if (!has_option(block, 'C') && !has_option(block, 'D'))
            return question_type::judge;
        return question_type::single; // A/B options might be single choice
    }

    // 如果有 A 和 B 选项，就是选择题
    if (has_option(block, 'A') && has_option(block, 'B')) {
        // 多选题通常有多个正确答案
        if (answer_raw.size() > 1) {
            size_t letter_count = 0;
            for (char c : answer_raw) {
                if (c >= 'A' && c <= 'Z') letter_count++;
            }
            if (letter_count > 1) return question_type::multi;
        }
        return question_type::single;
    }

    return question_type::unknown;
}

// 核心解析

std::vector<question> text_parser::parse(std::string_view content, std::string_view file_name) const
{
    std::vector<question> results;
    
    // 我们需要将分配的块字符串保存在某处，才能对其进行修改，
    // 但这里我们只是对原始视图进行切片。
    // 但是，原来的逻辑是"累积"行。如果行在内存中不连续
    // (在一个文件读取中它们确实是连续的)，我们可以只使用 start/end 指针。
    
    const char* file_start = content.data();
    const char* file_end = content.data() + content.size();
    const char* cur = file_start;
    const char* block_start = nullptr;

    auto flush_block = [&](const char* end_ptr) {
        if (block_start && end_ptr > block_start) {
            std::string_view block_sv(block_start, end_ptr - block_start);
            question q = parse_single_block(block_sv, file_name);
            if (q.type != question_type::unknown) {
                results.push_back(std::move(q));
            }
        }
    };

    // 逐行迭代
    while (cur < file_end) {
        const char* line_end = static_cast<const char*>(memchr(cur, '\n', file_end - cur));
        if (!line_end) line_end = file_end;
        
        std::string_view line_full(cur, line_end - cur);
        // 处理 CR
        if (!line_full.empty() && line_full.back() == '\r') line_full.remove_suffix(1);

        if (!is_garbage_line(line_full)) {
            if (is_question_start(line_full)) {
                // 新题目开始，刷新前一个
                flush_block(cur); // flush up to start of this line
                block_start = cur; // Mark start of new block
            } else {
                // 当前块的延续。
                // 如果这是第一行有效行且不是问题开始，
                // 我们是否只在块开始后才将其视为块的一部分？
                // 或者文件的第一行是垃圾或标题。
                if (!block_start) {
                    // 尝试宽容处理：如果我们还没有找到数字，如果它看起来像文本，也许这*就是*问题开始。
                    // 但是依靠 "1." 更安全。让我们坚持逻辑：丢弃第一个问题编号之前的文本。
                }
            }
        }

        cur = line_end + 1; // 移动到下一行
    }

    // 刷新最后一个块
    flush_block(file_end);

    return results;
}

question text_parser::parse_single_block(std::string_view block, std::string_view file_name) const
{
    question q;
    q.source_file = std::string(file_name);

    // 1. 查找答案分割点
    size_t split_index = block.size();
    
    // "我的答案" 检查
    size_t pos_my = block.find("我的答案");
    
    size_t pos_cor = std::string_view::npos;
    for (auto kw_sv : split_view(strategy_.answer_keywords, ',')) {
        size_t pos = block.find(trim(kw_sv));
        if (pos != std::string_view::npos && (pos_cor == std::string_view::npos || pos < pos_cor)) {
            pos_cor = pos;
        }
    }

    if (pos_my != std::string_view::npos) split_index = pos_my;
    else if (pos_cor != std::string_view::npos) split_index = pos_cor;

    std::string_view content_part = block.substr(0, split_index);
    std::string_view answer_raw;

    // 2. 提取答案
    // 我们需要特别在块中寻找"正确答案"关键词。
    // 之前的逻辑假设分割点是答案的开始，但"我的答案"可能会先出现。
    
    std::string_view real_answer_part;
    bool found_correct_kw = false;

    for (auto kw_sv : split_view(strategy_.answer_keywords, ',')) {
        auto kw = trim(kw_sv);
        size_t pos = block.find(kw);
        // 确保此关键词不是"我的答案" (My Answer) 的一部分
        // 例如，如果 kw 是 "答案"，我们需要检查它前面不是 "我的"
        // 但是 "正确答案" 足够独特。
        // 我们遍历关键词；如果我们找到一个，我们验证它不是 "我的答案"，除非关键词本身就是 "我的答案" (这通常不应该在策略中)
        
        if (pos != std::string_view::npos) {
             // 启发式：检查预防措施。
             // 如果关键词很短（如 "答案"），检查先行词。
             if (pos >= 3 && block.substr(pos - 3, 3) == "的" && kw.find("正确") == std::string_view::npos) {
                 // 可能是 "我的答案"，跳过此命中。
                 // 我们应该从 pos+1 再次搜索？为了简单起见，我们假设只有一次出现或通过第一次有效命中。
                 // Let's try finding the next one.
                 size_t next_pos = block.find(kw, pos + 1);
                 if (next_pos != std::string_view::npos) {
                     pos = next_pos;
                 } else {
                     continue;
                 }
             }

             real_answer_part = block.substr(pos + kw.size());
             found_correct_kw = true;
             break;
        }
    }

    // 2. 提取答案
    // ... (现有的关键词逻辑) ...
    // 如果我们没有找到明确的"正确答案"关键词，但我们有 "我的答案"，
    // 我们不应该使用 "我的答案" 作为正确答案。保持为空？
    
    if (found_correct_kw && !real_answer_part.empty()) {
        // ... (Existing cleanup of answer_raw) ...
         size_t real_start = 0;
         while (real_start < real_answer_part.size()) {
             char c = real_answer_part[real_start];
             if (c == ':' || c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                 real_start++;
                 continue;
             }
             // Check for Chinese colon "：" (EF BC 9A)
             if (static_cast<unsigned char>(c) == 0xEF && 
                 real_start + 2 < real_answer_part.size() &&
                 static_cast<unsigned char>(real_answer_part[real_start+1]) == 0xBC && 
                 static_cast<unsigned char>(real_answer_part[real_start+2]) == 0x9A) {
                 real_start += 3;
                 continue;
             }
             if (static_cast<unsigned char>(c) == 0xEF) {
                 break; 
             }
             break;
         }
         
         answer_raw = trim(real_answer_part.substr(real_start));
         // 只取答案的第一行
         auto first_nl = answer_raw.find('\n');
         if (first_nl != std::string_view::npos) {
             answer_raw = answer_raw.substr(0, first_nl);
         }
         answer_raw = trim(answer_raw);
    }
    
    // 2b. 内联答案提取 (如果没有找到关键词答案)
    std::string patched_content; // If we modify content
    bool content_modified = false;
    std::string inline_answer_storage; // 用于存储内联提取的答案
    
    if (answer_raw.empty()) {
        // 手动查找内联答案 (A), （A）, (AB), （AB）等
        std::string search_str(content_part);
        
        // 查找所有匹配项，取最后一个
        size_t best_start = std::string::npos;
        size_t best_end = std::string::npos;
        std::string best_answer;
        
        for (size_t i = 0; i < search_str.size(); i++) {
            bool is_open = false;
            size_t open_len = 0;
            
            // 检查半角开括号 (
            if (search_str[i] == '(') {
                is_open = true;
                open_len = 1;
            }
            // 检查全角开括号 （(EF BC 88)
            else if (i + 2 < search_str.size()) {
                unsigned char c1 = static_cast<unsigned char>(search_str[i]);
                unsigned char c2 = static_cast<unsigned char>(search_str[i+1]);
                unsigned char c3 = static_cast<unsigned char>(search_str[i+2]);
                if (c1 == 0xEF && c2 == 0xBC && c3 == 0x88) {
                    is_open = true;
                    open_len = 3;
                }
            }
            
            if (is_open) {
                size_t content_start = i + open_len;
                // 跳过空白
                while (content_start < search_str.size() && 
                       (search_str[content_start] == ' ' || search_str[content_start] == '\t')) {
                    content_start++;
                }
                
                // 检查是否是 A-Z
                std::string letters;
                size_t j = content_start;
                while (j < search_str.size() && search_str[j] >= 'A' && search_str[j] <= 'Z') {
                    letters += search_str[j];
                    j++;
                }
                
                if (!letters.empty()) {
                    // 跳过空白
                    while (j < search_str.size() && (search_str[j] == ' ' || search_str[j] == '\t')) j++;
                    
                    // 检查关闭括号
                    bool is_close = false;
                    size_t close_len = 0;
                    if (j < search_str.size() && search_str[j] == ')') {
                        is_close = true;
                        close_len = 1;
                    } else if (j + 2 < search_str.size()) {
                        unsigned char c1 = static_cast<unsigned char>(search_str[j]);
                        unsigned char c2 = static_cast<unsigned char>(search_str[j+1]);
                        unsigned char c3 = static_cast<unsigned char>(search_str[j+2]);
                        if (c1 == 0xEF && c2 == 0xBC && c3 == 0x89) { // ）
                            is_close = true;
                            close_len = 3;
                        }
                    }
                    
                    if (is_close) {
                        // 找到一个匹配！保存（我们取最后一个）
                        best_start = i;
                        best_end = j + close_len;
                        best_answer = letters;
                    }
                }
            }
        }
        
        if (best_start != std::string::npos) {
            inline_answer_storage = best_answer;
            answer_raw = std::string_view(inline_answer_storage); // 现在指向持久的存储
            
            // 在内容中屏蔽答案
            patched_content = search_str;
            patched_content.replace(best_start, best_end - best_start, "( )");
            content_modified = true;
        }
    }

    // 3. 检测类型
    q.type = detect_type(content_part, answer_raw);

    // 4. 清理内容 (移除 "1. " 或 "1．" 或 "1、" 和可选的 "[单选题]")
    std::string content_str;
    if (content_modified) {
        content_str = patched_content;
    } else {
        content_str = std::string(content_part);
    }
    
    // 手动清理题号前缀，支持 UTF-8 全角标点
    {
        size_t i = 0;
        // 跳过开头空白
        while (i < content_str.size() && (content_str[i] == ' ' || content_str[i] == '\t' || content_str[i] == '\r' || content_str[i] == '\n')) i++;
        
        // 跳过数字
        size_t num_start = i;
        while (i < content_str.size() && content_str[i] >= '0' && content_str[i] <= '9') i++;
        
        if (i > num_start && i < content_str.size()) {
            // 跳过点号（半角或全角）
            if (content_str[i] == '.') {
                i++;
            } else if (i + 2 < content_str.size()) {
                unsigned char c1 = static_cast<unsigned char>(content_str[i]);
                unsigned char c2 = static_cast<unsigned char>(content_str[i+1]);
                unsigned char c3 = static_cast<unsigned char>(content_str[i+2]);
                if ((c1 == 0xEF && c2 == 0xBC && c3 == 0x8E) ||  // ．
                    (c1 == 0xE3 && c2 == 0x80 && c3 == 0x81)) {  // 、
                    i += 3;
                }
            }
            
            // 跳过空白
            while (i < content_str.size() && (content_str[i] == ' ' || content_str[i] == '\t')) i++;
            
            // 检查并跳过可选的题型标签 (单选题) 或 [单选题] 或 （单选题）
            if (i < content_str.size()) {
                char open_bracket = content_str[i];
                bool has_bracket = (open_bracket == '(' || open_bracket == '[');
                // 检查全角括号 （ = E3 80 88? 不对，（ = EF BC 88
                if (!has_bracket && i + 2 < content_str.size()) {
                    unsigned char c1 = static_cast<unsigned char>(content_str[i]);
                    unsigned char c2 = static_cast<unsigned char>(content_str[i+1]);
                    unsigned char c3 = static_cast<unsigned char>(content_str[i+2]);
                    if (c1 == 0xEF && c2 == 0xBC && c3 == 0x88) { // （
                        has_bracket = true;
                        i += 3;
                    }
                } else if (has_bracket) {
                    i++;
                }
                
                if (has_bracket) {
                    // 查找关闭括号
                    size_t close_pos = content_str.find_first_of(")]}）", i);
                    // 检查 ） = EF BC 89
                    size_t j = i;
                    while (j < content_str.size()) {
                        if (content_str[j] == ')' || content_str[j] == ']') {
                            close_pos = j;
                            break;
                        }
                        if (j + 2 < content_str.size()) {
                            unsigned char c1 = static_cast<unsigned char>(content_str[j]);
                            unsigned char c2 = static_cast<unsigned char>(content_str[j+1]);
                            unsigned char c3 = static_cast<unsigned char>(content_str[j+2]);
                            if (c1 == 0xEF && c2 == 0xBC && c3 == 0x89) { // ）
                                close_pos = j;
                                break;
                            }
                        }
                        j++;
                    }
                    if (close_pos != std::string::npos) {
                        // 检查括号内是否包含题型关键词
                        std::string_view inside = std::string_view(content_str).substr(i, close_pos - i);
                        if (inside.find("单选") != std::string_view::npos ||
                            inside.find("多选") != std::string_view::npos ||
                            inside.find("判断") != std::string_view::npos ||
                            inside.find("填空") != std::string_view::npos) {
                            // 跳过关闭括号
                            i = close_pos + 1;
                            // 检查是否是 UTF-8 关闭括号
                            if (close_pos + 2 < content_str.size()) {
                                unsigned char c1 = static_cast<unsigned char>(content_str[close_pos]);
                                if (c1 == 0xEF) i = close_pos + 3;
                            }
                        }
                    }
                }
            }
            
            // 移除前缀
            content_str = content_str.substr(i);
        }
    }
    
    // 5. 解析选项
    if (q.type == question_type::single || q.type == question_type::multi || q.type == question_type::judge) {
        std::vector<std::pair<std::string, size_t>> opt_indices;
        
        // 自定义选项查找，支持 UTF-8 全角标点
        // 查找模式: 换行或行首 + 可选空白 + A-Z + 点号(半角/全角/顿号)
        size_t pos = 0;
        while (pos < content_str.size()) {
            // 跳到行首或换行后
            bool at_line_start = (pos == 0);
            if (!at_line_start && pos > 0) {
                // 检查前一个字符是否是换行
                if (content_str[pos-1] == '\n') at_line_start = true;
            }
            
            if (at_line_start) {
                // 跳过空白
                size_t start = pos;
                while (start < content_str.size() && (content_str[start] == ' ' || content_str[start] == '\t')) start++;
                
                // 检查是否是 A-Z
                if (start < content_str.size() && content_str[start] >= 'A' && content_str[start] <= 'Z') {
                    char opt_letter = content_str[start];
                    size_t after_letter = start + 1;
                    
                    bool is_option = false;
                    // 检查后面是否是点号
                    if (after_letter < content_str.size()) {
                        if (content_str[after_letter] == '.') {
                            is_option = true;
                        } else if (after_letter + 2 < content_str.size()) {
                            unsigned char c1 = static_cast<unsigned char>(content_str[after_letter]);
                            unsigned char c2 = static_cast<unsigned char>(content_str[after_letter+1]);
                            unsigned char c3 = static_cast<unsigned char>(content_str[after_letter+2]);
                            // 全角点 ．(U+FF0E) = EF BC 8E
                            if (c1 == 0xEF && c2 == 0xBC && c3 == 0x8E) is_option = true;
                            // 中文顿号 、(U+3001) = E3 80 81
                            if (c1 == 0xE3 && c2 == 0x80 && c3 == 0x81) is_option = true;
                        }
                    }
                    
                    if (is_option) {
                        opt_indices.push_back({ std::string(1, opt_letter), start });
                    }
                }
            }
            
            // 移动到下一个换行符后
            size_t next_nl = content_str.find('\n', pos);
            if (next_nl == std::string::npos) break;
            pos = next_nl + 1;
        }
        
        size_t first_opt_pos = opt_indices.empty() ? std::string::npos : opt_indices[0].second;

        if (first_opt_pos != std::string::npos) {
            q.content = trim(std::string_view(content_str).substr(0, first_opt_pos));
            
            for (size_t i = 0; i < opt_indices.size(); ++i) {
                size_t start = opt_indices[i].second;
                size_t next_start = (i == opt_indices.size() - 1) ? content_str.length() : opt_indices[i+1].second;
                
                std::string_view raw_opt = std::string_view(content_str).substr(start, next_start - start);
                
                // 跳过 "A." 或 "A．" 或 "A、" 部分
                size_t skip = 1; // 跳过字母
                if (skip < raw_opt.size()) {
                    if (raw_opt[skip] == '.') {
                        skip += 1;
                    } else if (skip + 2 < raw_opt.size()) {
                        unsigned char c1 = static_cast<unsigned char>(raw_opt[skip]);
                        if (c1 == 0xEF || c1 == 0xE3) skip += 3; // 跳过 UTF-8 多字节标点
                    }
                }
                raw_opt = raw_opt.substr(skip);
                
                q.options.push_back(opt_indices[i].first + ". " + std::string(trim(raw_opt)));
            }
        } else {
            q.content = trim(content_str);
        }

        // 判断题的回退 (如果解析不到选项，例如只有 "True/False" 文本而不是 A/B)
        if (q.type == question_type::judge && q.options.empty()) {
             q.options = { "A. 对", "B. 错" };
        }
    } else {
        q.content = trim(content_str);
    }

    // 6. 最终确定答案
    if (!answer_raw.empty()) {
        if (q.type == question_type::judge) {
            if (is_judge_true(answer_raw)) q.correct_answer = "A";
            else if (is_judge_false(answer_raw)) q.correct_answer = "B";
            else q.correct_answer = std::string(answer_raw);
        } else if (q.type == question_type::fill) {
             static const std::regex re_idx(R"(\(\d+\)\s*)");
             q.correct_answer = std::regex_replace(std::string(answer_raw), re_idx, "");
        } else {
             // 过滤 A-Z
             std::string final_ans;
             for (char c : answer_raw) {
                 if (c >= 'A' && c <= 'Z') final_ans += c;
                 if (c == ';' || (unsigned char)c > 127) break;
             }
             q.correct_answer = final_ans;
        }
    }

    return q;
}
