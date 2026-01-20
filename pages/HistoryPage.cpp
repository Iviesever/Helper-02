#include "HistoryPage.h"

HistoryPage::HistoryPage(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    
    connect(ui.btnBack, &QPushButton::clicked, this, &HistoryPage::backClicked);
    
    // 初始化表头
    ui.tableHistory->setColumnCount(4);
    ui.tableHistory->setHorizontalHeaderLabels({ "时间", "得分", "正确率", "耗时" });
}

void HistoryPage::clearHistory()
{
    ui.tableHistory->setRowCount(0);
}

void HistoryPage::loadHistory(const std::vector<exam_record>& records)
{
    ui.tableHistory->setRowCount(0);
    ui.tableHistory->setRowCount(static_cast<int>(records.size()));

    for(size_t i = 0; i < records.size(); ++i)
    {
        const auto & r = records[i];
        
        // 时间
        ui.tableHistory->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(r.date)));
        
        // 得分
        ui.tableHistory->setItem(i, 1, new QTableWidgetItem(QString::number(r.score)));
        
        // 正确率
        double accuracy = (r.total_count > 0) ? (r.correct_count * 100.0 / r.total_count) : 0.0;
        ui.tableHistory->setItem(i, 2, new QTableWidgetItem(
            QString("%1 / %2 = %3%")
            .arg(r.correct_count)
            .arg(r.total_count)
            .arg(accuracy, 0, 'f', 1)
        ));
        
        // 耗时
        int min = r.duration_sec / 60;
        int sec = r.duration_sec % 60;
        ui.tableHistory->setItem(i, 3, new QTableWidgetItem(QString("%1分%2秒").arg(min).arg(sec)));
    }
}
