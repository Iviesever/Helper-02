#pragma once

#include <QWidget>
#include <QStringList>
#include <QComboBox>
#include <QListWidget>
#include <QScrollArea>
#include "ui_HomePage.h"

class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr);
    ~HomePage() {}

    // 公共接口
    QComboBox* comboRepo() const { return ui.comboRepo; }
    QComboBox* comboParser() const { return ui.comboParser; }
    QListWidget* listWidgetFiles() const { return ui.listWidgetFiles; }
    QScrollArea* scrollArea() const { return ui.scrollArea_Home; }
    
    // 错题过滤设置
    int mistakeOp() const { return ui.comboMistakeOp->currentIndex(); }
    int mistakeCount() const { return ui.comboMistakeCount->currentText().toInt(); }
    
    // 题型过滤
    bool isSingleChecked() const { return ui.chkSingle->isChecked(); }
    bool isMultiChecked() const { return ui.chkMulti->isChecked(); }
    bool isJudgeChecked() const { return ui.chkJudge->isChecked(); }
    bool isFillChecked() const { return ui.chkFill->isChecked(); }
    
    // 获取选中的文件路径
    QStringList selectedFilePaths() const
    {
        QStringList paths;
        for(auto* item : ui.listWidgetFiles->selectedItems()) {
            paths.append(item->data(Qt::UserRole).toString());
        }
        return paths;
    }
    
    // 更新错题次数下拉框
    void updateMistakeCountCombo(size_t maxCount)
    {
        ui.comboMistakeCount->clear();
        for(size_t i = 1; i <= maxCount; ++i) {
            ui.comboMistakeCount->addItem(QString::number(i));
        }
    }

signals:
    // 页面跳转信号
    void openSettings();
    void openHistory();
    void openExamConfig();
    void openParserStrategy();
    void openPracticeStrategy();
    
    // 练习/考试开始信号
    void startSequentialPractice();
    void startRandomPractice();
    void startViewMode();  // 看题模式
    
    // 题库切换信号
    void repoChanged(int index);

private:
    Ui::HomePage ui;
};
