#pragma once

#include <QWidget>
#include <QScroller>
#include <vector>
#include <array>
#include "ui_ExamConfigPage.h"
#include "../storage_manager.h"

class ExamConfigPage : public QWidget
{
    Q_OBJECT

public:
    explicit ExamConfigPage(storage_manager & storage, QWidget *parent = nullptr);
    ~ExamConfigPage() {}

    // 初始化/刷新配置列表
    void refreshConfigList();

    // 更新可用题目数量显示
    void updateAvailableCounts(const std::vector<int>& counts);

signals:
    void backClicked();
    
    // 开始考试信号：传递各题型的题数和分值配置，以及考试时间
    // counts: [single, multi, judge, fill]
    // scores: [single, multi, judge, fill]
    // durationMin: 考试时长(分钟)
    void startExam(const std::array<size_t, 4>& counts, 
                   const std::array<double, 4>& scores, 
                   int durationMin);

private slots:
    void onConfigChanged(const QString & name);
    void onSaveConfig();
    void onDeleteConfig();
    void onStartExamClicked();

private:
    Ui::ExamConfigPage ui;
    storage_manager & storage_; // 引用外部存储管理器

    // 辅助函数
    void loadConfigToUi(const exam_config & cfg);
    exam_config getConfigFromUi() const;
};
