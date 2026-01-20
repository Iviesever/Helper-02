#pragma once

#include <QWidget>
#include "ui_SettingsPage.h"

struct app_config;

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);
    ~SettingsPage() {}

    // 加载配置到 UI
    void loadConfig(const app_config& cfg);
    
    // 从 UI 获取配置
    int fontTitleSize() const { return ui.sliderFontTitle->value(); }
    int fontBaseSize() const { return ui.sliderFontBase->value(); }
    bool autoSubmit() const { return ui.chkAutoSubmit->isChecked(); }
    bool confirmExit() const { return ui.chkConfirmExit->isChecked(); }
    bool autoNext() const { return ui.chkAutoNext->isChecked(); }
    bool darkMode() const { return ui.chkDarkMode->isChecked(); }
    int fileListLimit() const { return ui.spinBoxFileListLimit->value(); }
    QString repoPath() const { return ui.editRepoPath->text(); }
    QString dataPath() const { return ui.editDataPath->text(); }

    // 设置路径
    void setRepoPath(const QString& path) { ui.editRepoPath->setText(path); }
    void setDataPath(const QString& path) { ui.editDataPath->setText(path); }

signals:
    void backClicked();
    void saveClicked();
    void browseRepoClicked();
    void browseDataClicked();

private:
    Ui::SettingsPage ui;
};
