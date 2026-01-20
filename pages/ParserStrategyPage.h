#pragma once

#include <QWidget>
#include "ui_ParserStrategyPage.h"
#include "../parser/parser_strategy.h"
#include <QToolButton>

class storage_manager;

class ParserStrategyPage : public QWidget
{
    Q_OBJECT

public:
    explicit ParserStrategyPage(storage_manager* storage, QWidget *parent = nullptr);
    ~ParserStrategyPage() {}

    // 刷新策略列表
    void refreshStrategyList();

    // 获取当前选中的策略名称
    QString currentStrategyName() const;

signals:
    void backClicked();
    void strategyChanged(const QString& name);

private slots:
    void onStrategySelected(int index);
    void onNewClicked();
    void onSaveClicked();
    void onDeleteClicked();
    void onTestClicked();

private:
    void loadStrategyToUI(const parser_strategy& strategy);
    parser_strategy getStrategyFromUI();
    void setEditable(bool editable);
    void setupReferenceSection();
    void addReferenceItem(const QString& title, const QString& desc, const QString& code);

    Ui::ParserStrategyPage ui;
    storage_manager* storage_;
    bool isNewStrategy_ = false;
};
