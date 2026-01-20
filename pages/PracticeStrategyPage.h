#pragma once

#include <QWidget>
#include <vector>
#include <array>
#include <unordered_map>
#include "ui_PracticeStrategyPage.h"
#include "../question.h"

class PracticeStrategyPage : public QWidget
{
    Q_OBJECT

public:
    explicit PracticeStrategyPage(QWidget *parent = nullptr);
    ~PracticeStrategyPage() = default;

    // 加载题库统计信息
    void loadStatistics(const std::vector<question>& questions);
    
    // 设置题库名称（用于动态标题）
    void setRepoName(const QString& name);
    
    // 清空统计
    void clearStatistics();
    
    // 加载/获取策略
    void loadStrategy(const practice_strategy& strategy);
    practice_strategy getStrategy() const;
    
    // 获取过滤设置
    bool excludeDuplicates() const { return ui.chkExcludeDuplicates->isChecked(); }
    bool excludeMultiAll() const { return ui.chkExcludeMultiAll->isChecked(); }
    bool skipSingleMostCommon() const { return ui.chkSkipSingleMostCommon->isChecked(); }
    bool skipJudgeMostCommon() const { return ui.chkSkipJudgeMostCommon->isChecked(); }
    
    // 获取单选项跳过设置
    std::array<bool, 4> skipSingleOptions() const;
    std::array<bool, 2> skipJudgeOptions() const;
    
    // 获取最常见答案用于过滤
    QString mostCommonSingleAnswer() const { return mostCommonSingle_; }
    QString mostCommonJudgeAnswer() const { return mostCommonJudge_; }
    
    // 检查是否应跳过单选题（考虑优先级）
    bool shouldSkipSingle(const QString& answer) const;
    bool shouldSkipJudge(const QString& answer) const;

signals:
    void backClicked();
    void settingsChanged();

private:
    Ui::PracticeStrategyPage ui;
    
    // 最常见答案
    QString mostCommonSingle_;
    QString mostCommonJudge_;
    
    // 答案分布统计
    std::unordered_map<std::string, int> singleAnswerDist_;
    std::unordered_map<std::string, int> judgeAnswerDist_;
};
