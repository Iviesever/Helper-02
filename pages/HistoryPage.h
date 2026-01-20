#pragma once

#include <QWidget>
#include <vector>
#include "ui_HistoryPage.h"
#include "../storage_manager.h" // 为了 exam_record

class HistoryPage : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryPage(QWidget *parent = nullptr);
    ~HistoryPage() {}

    // 加载历史记录到表格
    void loadHistory(const std::vector<exam_record>& records);

    // 清空表格
    void clearHistory();

signals:
    void backClicked();

private:
    Ui::HistoryPage ui;
};
