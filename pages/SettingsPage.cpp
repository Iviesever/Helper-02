#include "SettingsPage.h"
#include "../storage_manager.h"

#include <QScrollBar>
#include <QScroller>
#include <QFileDialog>

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    
    // 惯性滚动设置
    QScroller::grabGesture(ui.scrollArea_Settings->viewport(), QScroller::LeftMouseButtonGesture);
    ui.scrollArea_Settings->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui.scrollArea_Settings->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui.scrollArea_Settings->setWidgetResizable(true);
    ui.scrollArea_Settings->setAlignment(Qt::AlignHCenter);
    ui.scrollArea_Settings->horizontalScrollBar()->setRange(0, 0);
    
    // 信号连接
    connect(ui.btnBack, &QPushButton::clicked, this, &SettingsPage::backClicked);
    connect(ui.btnSave, &QPushButton::clicked, this, &SettingsPage::saveClicked);
    connect(ui.btnBrowseRepo, &QAbstractButton::clicked, this, &SettingsPage::browseRepoClicked);
    connect(ui.btnBrowseData, &QAbstractButton::clicked, this, &SettingsPage::browseDataClicked);
    
    // 滑块预览
    connect(ui.sliderFontTitle, &QSlider::valueChanged, this, [this](int value) {
        ui.lbl_FontTitleConfig->setText(QString("题目字体大小 (%1):").arg(value));
        ui.lbl_PreviewTitle->setStyleSheet(QString("font-size: %1px; color: #55aaff;").arg(value));
    });
    
    connect(ui.sliderFontBase, &QSlider::valueChanged, this, [this](int value) {
        ui.lbl_FontBaseConfig->setText(QString("选项/按钮字体大小 (%1):").arg(value));
        ui.lbl_PreviewBase->setStyleSheet(QString("font-size: %1px; color: #55aaff;").arg(value));
    });
}

void SettingsPage::loadConfig(const app_config& cfg)
{
    ui.sliderFontTitle->setValue(cfg.font_size);
    ui.sliderFontBase->setValue(cfg.button_size);
    ui.chkAutoSubmit->setChecked(cfg.auto_submit);
    ui.chkConfirmExit->setChecked(cfg.confirm_exit);
    ui.chkAutoNext->setChecked(cfg.auto_next);
    ui.spinBoxFileListLimit->setValue(static_cast<int>(cfg.file_list_limit));
    ui.chkDarkMode->setChecked(cfg.dark_mode);
    ui.editRepoPath->setText(QString::fromStdString(cfg.custom_repo_path));
    ui.editDataPath->setText(QString::fromStdString(cfg.custom_data_path));
    
    // 初始化预览
    ui.lbl_PreviewTitle->setStyleSheet(QString("font-size: %1px; color: #55aaff;").arg(cfg.font_size));
    ui.lbl_PreviewBase->setStyleSheet(QString("font-size: %1px; color: #55aaff;").arg(cfg.button_size));
}
