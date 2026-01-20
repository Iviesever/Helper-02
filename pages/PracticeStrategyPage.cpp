#include "PracticeStrategyPage.h"
#include <QScroller>
#include <algorithm>

PracticeStrategyPage::PracticeStrategyPage(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    
    // 设置滚动手势
    QScroller::grabGesture(ui.scrollArea->viewport(), QScroller::LeftMouseButtonGesture);
    ui.scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui.scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // 连接信号
    connect(ui.btnBack, &QPushButton::clicked, this, &PracticeStrategyPage::backClicked);
    
    // 设置变更信号
    connect(ui.chkExcludeDuplicates, &QCheckBox::toggled, this, &PracticeStrategyPage::settingsChanged);
    connect(ui.chkExcludeMultiAll, &QCheckBox::toggled, this, &PracticeStrategyPage::settingsChanged);
    connect(ui.chkSkipSingleMostCommon, &QCheckBox::toggled, this, &PracticeStrategyPage::settingsChanged);
    connect(ui.chkSkipJudgeMostCommon, &QCheckBox::toggled, this, &PracticeStrategyPage::settingsChanged);
    connect(ui.chkSkipSingleA, &QCheckBox::toggled, this, &PracticeStrategyPage::settingsChanged);
    connect(ui.chkSkipSingleB, &QCheckBox::toggled, this, &PracticeStrategyPage::settingsChanged);
    connect(ui.chkSkipSingleC, &QCheckBox::toggled, this, &PracticeStrategyPage::settingsChanged);
    connect(ui.chkSkipSingleD, &QCheckBox::toggled, this, &PracticeStrategyPage::settingsChanged);
    connect(ui.chkSkipJudgeA, &QCheckBox::toggled, this, &PracticeStrategyPage::settingsChanged);
    connect(ui.chkSkipJudgeB, &QCheckBox::toggled, this, &PracticeStrategyPage::settingsChanged);
}

void PracticeStrategyPage::loadStatistics(const std::vector<question>& questions)
{
    // 统计各类型数量
    int singleCount = 0, multiCount = 0, judgeCount = 0, fillCount = 0;
    
    // 清空并重新统计答案分布
    singleAnswerDist_.clear();
    judgeAnswerDist_.clear();
    
    for (const auto& q : questions)
    {
        switch (q.type)
        {
        case question_type::single:
            singleCount++;
            if (!q.correct_answer.empty())
                singleAnswerDist_[q.correct_answer]++;
            break;
        case question_type::multi:
            multiCount++;
            break;
        case question_type::judge:
            judgeCount++;
            if (!q.correct_answer.empty())
                judgeAnswerDist_[q.correct_answer]++;
            break;
        case question_type::fill:
            fillCount++;
            break;
        default:
            break;
        }
    }
    
    int total = singleCount + multiCount + judgeCount + fillCount;
    
    // 更新统计UI
    ui.label_SingleCount->setText(QString::number(singleCount));
    ui.label_MultiCount->setText(QString::number(multiCount));
    ui.label_JudgeCount->setText(QString::number(judgeCount));
    ui.label_FillCount->setText(QString::number(fillCount));
    ui.label_TotalCount->setText(QString("总计: %1 题").arg(total));
    
    // 显示单选题各选项分布
    auto getCount = [](const std::unordered_map<std::string, int>& map, const std::string& key) {
        auto it = map.find(key);
        return it != map.end() ? it->second : 0;
    };
    
    int countA = getCount(singleAnswerDist_, "A");
    int countB = getCount(singleAnswerDist_, "B");
    int countC = getCount(singleAnswerDist_, "C");
    int countD = getCount(singleAnswerDist_, "D");
    
    // 找出最常见单选答案（可能有多个）
    int maxSingle = std::max({countA, countB, countC, countD});
    mostCommonSingle_.clear();
    if (maxSingle > 0) {
        if (countA == maxSingle) mostCommonSingle_ += "A";
        if (countB == maxSingle) mostCommonSingle_ += "B";
        if (countC == maxSingle) mostCommonSingle_ += "C";
        if (countD == maxSingle) mostCommonSingle_ += "D";
    }
    
    auto formatDist = [singleCount, maxSingle](const QString& opt, int count) {
        double percent = singleCount > 0 ? (100.0 * count / singleCount) : 0;
        QString label = (count == maxSingle && maxSingle > 0) ? " [最常见]" : "";
        return QString("  %1: %2 (%3%)%4").arg(opt).arg(count).arg(percent, 0, 'f', 1).arg(label);
    };
    
    ui.label_SingleDistA->setText(formatDist("A", countA));
    ui.label_SingleDistB->setText(formatDist("B", countB));
    ui.label_SingleDistC->setText(formatDist("C", countC));
    ui.label_SingleDistD->setText(formatDist("D", countD));
    
    // 更新单选最常见答案跳过按钮
    if (!mostCommonSingle_.isEmpty()) {
        ui.chkSkipSingleMostCommon->setText(QString("跳过最常见答案 (%1)").arg(mostCommonSingle_));
    } else {
        ui.chkSkipSingleMostCommon->setText("跳过最常见答案 (无数据)");
    }
    
    // 显示判断题各选项分布
    int judgeA = getCount(judgeAnswerDist_, "A");
    int judgeB = getCount(judgeAnswerDist_, "B");
    
    // 找出最常见判断答案（可能有多个）
    int maxJudge = std::max(judgeA, judgeB);
    mostCommonJudge_.clear();
    if (maxJudge > 0) {
        if (judgeA == maxJudge) mostCommonJudge_ += "A";
        if (judgeB == maxJudge) mostCommonJudge_ += "B";
    }
    
    auto formatJudge = [judgeCount, maxJudge](const QString& opt, int count) {
        double percent = judgeCount > 0 ? (100.0 * count / judgeCount) : 0;
        QString label = (count == maxJudge && maxJudge > 0) ? " [最常见]" : "";
        return QString("  %1: %2 (%3%)%4").arg(opt).arg(count).arg(percent, 0, 'f', 1).arg(label);
    };
    
    ui.label_JudgeDistA->setText(formatJudge("A(对)", judgeA));
    ui.label_JudgeDistB->setText(formatJudge("B(错)", judgeB));
    
    // 更新判断最常见答案跳过按钮
    if (!mostCommonJudge_.isEmpty()) {
        QString display = mostCommonJudge_;
        if (display == "A") display = "A/对";
        else if (display == "B") display = "B/错";
        else if (display == "AB") display = "AB/都常见";
        ui.chkSkipJudgeMostCommon->setText(QString("跳过最常见答案 (%1)").arg(display));
    } else {
        ui.chkSkipJudgeMostCommon->setText("跳过最常见答案 (无数据)");
    }
}

void PracticeStrategyPage::clearStatistics()
{
    ui.label_SingleCount->setText("0");
    ui.label_MultiCount->setText("0");
    ui.label_JudgeCount->setText("0");
    ui.label_FillCount->setText("0");
    ui.label_TotalCount->setText("总计: 0 题");
    
    ui.label_SingleDistA->setText("  A: 0 (0%)");
    ui.label_SingleDistB->setText("  B: 0 (0%)");
    ui.label_SingleDistC->setText("  C: 0 (0%)");
    ui.label_SingleDistD->setText("  D: 0 (0%)");
    
    ui.label_JudgeDistA->setText("  A(对): 0 (0%)");
    ui.label_JudgeDistB->setText("  B(错): 0 (0%)");
    
    ui.chkSkipSingleMostCommon->setText("跳过最常见答案 (无数据)");
    ui.chkSkipJudgeMostCommon->setText("跳过最常见答案 (无数据)");
    
    mostCommonSingle_.clear();
    mostCommonJudge_.clear();
    singleAnswerDist_.clear();
    judgeAnswerDist_.clear();
}

void PracticeStrategyPage::loadStrategy(const practice_strategy& strategy)
{
    ui.chkSkipSingleMostCommon->setChecked(strategy.skip_single_most_common);
    ui.chkSkipJudgeMostCommon->setChecked(strategy.skip_judge_most_common);
    ui.chkExcludeDuplicates->setChecked(strategy.exclude_duplicates);
    ui.chkExcludeMultiAll->setChecked(strategy.exclude_multi_all);
    
    ui.chkSkipSingleA->setChecked(strategy.skip_single_options[0]);
    ui.chkSkipSingleB->setChecked(strategy.skip_single_options[1]);
    ui.chkSkipSingleC->setChecked(strategy.skip_single_options[2]);
    ui.chkSkipSingleD->setChecked(strategy.skip_single_options[3]);
    
    ui.chkSkipJudgeA->setChecked(strategy.skip_judge_options[0]);
    ui.chkSkipJudgeB->setChecked(strategy.skip_judge_options[1]);
}

practice_strategy PracticeStrategyPage::getStrategy() const
{
    practice_strategy s;
    s.skip_single_most_common = ui.chkSkipSingleMostCommon->isChecked();
    s.skip_judge_most_common = ui.chkSkipJudgeMostCommon->isChecked();
    s.exclude_duplicates = ui.chkExcludeDuplicates->isChecked();
    s.exclude_multi_all = ui.chkExcludeMultiAll->isChecked();
    
    s.skip_single_options[0] = ui.chkSkipSingleA->isChecked();
    s.skip_single_options[1] = ui.chkSkipSingleB->isChecked();
    s.skip_single_options[2] = ui.chkSkipSingleC->isChecked();
    s.skip_single_options[3] = ui.chkSkipSingleD->isChecked();
    
    s.skip_judge_options[0] = ui.chkSkipJudgeA->isChecked();
    s.skip_judge_options[1] = ui.chkSkipJudgeB->isChecked();
    
    return s;
}

std::array<bool, 4> PracticeStrategyPage::skipSingleOptions() const
{
    return {
        ui.chkSkipSingleA->isChecked(),
        ui.chkSkipSingleB->isChecked(),
        ui.chkSkipSingleC->isChecked(),
        ui.chkSkipSingleD->isChecked()
    };
}

std::array<bool, 2> PracticeStrategyPage::skipJudgeOptions() const
{
    return {
        ui.chkSkipJudgeA->isChecked(),
        ui.chkSkipJudgeB->isChecked()
    };
}

bool PracticeStrategyPage::shouldSkipSingle(const QString& answer) const
{
    // 最常见答案跳过优先（可能有多个，如AB）
    if (ui.chkSkipSingleMostCommon->isChecked() && !mostCommonSingle_.isEmpty()) {
        if (mostCommonSingle_.contains(answer)) return true;
    }
    
    // 单独选项跳过
    if (answer == "A" && ui.chkSkipSingleA->isChecked()) return true;
    if (answer == "B" && ui.chkSkipSingleB->isChecked()) return true;
    if (answer == "C" && ui.chkSkipSingleC->isChecked()) return true;
    if (answer == "D" && ui.chkSkipSingleD->isChecked()) return true;
    
    return false;
}

bool PracticeStrategyPage::shouldSkipJudge(const QString& answer) const
{
    // 最常见答案跳过优先（可能有多个，如AB都常见）
    if (ui.chkSkipJudgeMostCommon->isChecked() && !mostCommonJudge_.isEmpty()) {
        if (mostCommonJudge_.contains(answer)) return true;
    }
    
    // 单独选项跳过
    if (answer == "A" && ui.chkSkipJudgeA->isChecked()) return true;
    if (answer == "B" && ui.chkSkipJudgeB->isChecked()) return true;
    
    return false;
}

void PracticeStrategyPage::setRepoName(const QString& name)
{
    ui.label_Title->setText(name + " 刷题策略");
}
