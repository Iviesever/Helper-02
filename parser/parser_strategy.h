#pragma once
#include <string>

struct parser_strategy
{
    std::string name;                       // 策略名称（唯一标识）
    
    // 题型识别关键词 (逗号分隔)
    std::string single_keywords;            // 单选题关键词 (e.g., "单选题,单选")
    std::string multi_keywords;             // 多选题关键词 (e.g., "多选题,多选")
    std::string judge_keywords;             // 判断题关键词 (e.g., "判断题,判断")
    std::string fill_keywords;              // 填空题关键词 (e.g., "填空题,填空")
    
    // 答案识别规则
    std::string answer_keywords;            // 答案关键词 (e.g., "正确答案,答案：,答案:")
    
    // 垃圾行过滤规则 (逗号分隔的正则)
    std::string garbage_patterns;           // 需要过滤的行 (e.g., "^\\s*\\d+\\s*$,AI讲解")

    // 判断题答案映射
    std::string judge_true_values;          // 表示"对"的值 (e.g., "对,T,true,√,正确")
    std::string judge_false_values;         // 表示"错"的值 (e.g., "错,F,false,×,X,错误")

    bool operator==(const parser_strategy&) const = default;

    static parser_strategy get_default()
    {
        parser_strategy s;
        s.name = "默认策略";
        
        // 题型关键词
        s.single_keywords = "单选题,单选";
        s.multi_keywords = "多选题,多选";
        s.judge_keywords = "判断题,判断";
        s.fill_keywords = "填空题,填空";
        
        // 答案关键词
        s.answer_keywords = "正确答案,答案：,答案:";
        
        // 垃圾行过滤
        s.garbage_patterns = R"(^\s*\d+(\.\d+)?分\s*$,^\s*[一二三四五六七八九十]+\s*[\.、]\s*.*题.*分.*,^\s*\d+\s*$,AI讲解,查看作答记录,最终成绩,作答时间)";
        
        // 判断题答案映射
        s.judge_true_values = "对,T,true,True,√,正确";
        s.judge_false_values = "错,F,false,False,×,X,错误";
        
        return s;
    }
};

