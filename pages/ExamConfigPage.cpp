#include "ExamConfigPage.h"
#include <QMessageBox>
#include <ranges>

ExamConfigPage::ExamConfigPage(storage_manager & storage, QWidget *parent)
    : QWidget(parent), storage_(storage)
{
    ui.setupUi(this);

    // 触摸滚动支持
    QScroller::grabGesture(ui.scrollArea_ExamConfig, QScroller::LeftMouseButtonGesture);
    ui.scrollArea_ExamConfig->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui.scrollArea_ExamConfig->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 信号连接
    connect(ui.btnBack, &QPushButton::clicked, this, &ExamConfigPage::backClicked);
    
    connect(ui.comboExamConfigs, &QComboBox::currentTextChanged, this, &ExamConfigPage::onConfigChanged);
    connect(ui.btnSaveConfig, &QPushButton::clicked, this, &ExamConfigPage::onSaveConfig);
    connect(ui.btnDeleteConfig, &QPushButton::clicked, this, &ExamConfigPage::onDeleteConfig);
    connect(ui.btnStartExam, &QPushButton::clicked, this, &ExamConfigPage::onStartExamClicked);

    // 初始加载
    refreshConfigList();
}

void ExamConfigPage::refreshConfigList()
{
    ui.comboExamConfigs->blockSignals(true);
    ui.comboExamConfigs->clear();
    for(const auto & n : storage_.get_exam_config_names())
    {
        ui.comboExamConfigs->addItem(QString::fromStdString(n));
    }
    ui.comboExamConfigs->blockSignals(false);
    
    // 如果有当前配置，加载它... 或者什么都不做等待用户选择，或者加载默认
    // 这里简单起见，不自动选，或者选第一个
}

void ExamConfigPage::updateAvailableCounts(const std::vector<int>& counts)
{
    if(counts.size() >= 4)
    {
        ui.lbl_Avail_Single->setText(QString::number(counts[0]));
        ui.lbl_Avail_Multi->setText(QString::number(counts[1]));
        ui.lbl_Avail_Judge->setText(QString::number(counts[2]));
        ui.lbl_Avail_Fill->setText(QString::number(counts[3]));
    }
}

void ExamConfigPage::loadConfigToUi(const exam_config & cfg)
{
    ui.edit_Count_Single->setText(QString::number(cfg.single_count));
    ui.edit_Count_Multi->setText(QString::number(cfg.multi_count));
    ui.edit_Count_Judge->setText(QString::number(cfg.judge_count));
    ui.edit_Count_Fill->setText(QString::number(cfg.fill_count));

    ui.edit_Score_Single->setText(QString::number(cfg.single_score));
    ui.edit_Score_Multi->setText(QString::number(cfg.multi_score));
    ui.edit_Score_Judge->setText(QString::number(cfg.judge_score));
    ui.edit_Score_Fill->setText(QString::number(cfg.fill_score));

    ui.edit_ExamTime->setText(QString::number(cfg.exam_duration));
}

exam_config ExamConfigPage::getConfigFromUi() const
{
    exam_config cfg;
    cfg.single_count = ui.edit_Count_Single->text().toUInt();
    cfg.multi_count = ui.edit_Count_Multi->text().toUInt();
    cfg.judge_count = ui.edit_Count_Judge->text().toUInt();
    cfg.fill_count = ui.edit_Count_Fill->text().toUInt();

    cfg.single_score = ui.edit_Score_Single->text().toDouble();
    cfg.multi_score = ui.edit_Score_Multi->text().toDouble();
    cfg.judge_score = ui.edit_Score_Judge->text().toDouble();
    cfg.fill_score = ui.edit_Score_Fill->text().toDouble();

    cfg.exam_duration = ui.edit_ExamTime->text().toLongLong();
    return cfg;
}

void ExamConfigPage::onConfigChanged(const QString & name)
{
    if(name.isEmpty()) return;
    if(storage_.load_exam_config_by_name(name.toStdString()))
    {
        loadConfigToUi(storage_.get_exam_config());
    }
}

void ExamConfigPage::onSaveConfig()
{
    QString name = ui.editConfigName->text().trimmed();
    if(name.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请输入配置名称！");
        return;
    }
    
    exam_config cfg = getConfigFromUi();
    storage_.save_exam_config_as(name.toStdString(), cfg);
    
    // 刷新下拉框并选中
    QString current = name;
    refreshConfigList();
    ui.comboExamConfigs->setCurrentText(current);
    ui.editConfigName->clear();
    
    QMessageBox::information(this, "成功", "配置已保存：" + name);
}

void ExamConfigPage::onDeleteConfig()
{
    QString name = ui.comboExamConfigs->currentText();
    if(name.isEmpty()) return;
    
    auto reply = QMessageBox::question(this, "确认删除", 
        QString("确定要删除配置 \"%1\" 吗？").arg(name),
        QMessageBox::Yes | QMessageBox::No);
    
    if(reply == QMessageBox::Yes)
    {
        storage_.delete_exam_config(name.toStdString());
        refreshConfigList();
    }
}

void ExamConfigPage::onStartExamClicked()
{
    // 验证 UI 输入是否有效（基本数字转换由 toUInt/toDouble 处理，虽然可能返回 0）
    // 发送信号给 MainWindow
    
    std::array<size_t, 4> counts = {
        (size_t)ui.edit_Count_Single->text().toInt(),
        (size_t)ui.edit_Count_Multi->text().toInt(),
        (size_t)ui.edit_Count_Judge->text().toInt(),
        (size_t)ui.edit_Count_Fill->text().toInt()
    };
    
    std::array<double, 4> scores = {
        ui.edit_Score_Single->text().toDouble(),
        ui.edit_Score_Multi->text().toDouble(),
        ui.edit_Score_Judge->text().toDouble(),
        ui.edit_Score_Fill->text().toDouble()
    };
    
    int duration = ui.edit_ExamTime->text().toInt();
    
    // 检查可用数量逻辑（需要传入可用数量吗？或者由 MainWindow 检查？）
    // 为了解耦，这里只发送请求，MainWindow 收到后检查是否满足条件。
    // 但是用户希望能在这页看到警告。
    // 简单起见，ExamConfigPage 可用 counts 由 MainWindow 通过 updateAvailableCounts 更新。
    // 我们可以直接从 UI 标签读取可用数量来做验证。
    
    std::vector<int> available = {
        ui.lbl_Avail_Single->text().toInt(),
        ui.lbl_Avail_Multi->text().toInt(),
        ui.lbl_Avail_Judge->text().toInt(),
        ui.lbl_Avail_Fill->text().toInt()
    };
    
    std::vector<QString> typeNames = { "单选题", "多选题", "判断题", "填空题" };
    QString wrong_text;
    
    for(size_t i=0; i<4; ++i)
    {
        if(counts[i] > static_cast<size_t>(available[i]))
        {
            if(!wrong_text.isEmpty()) wrong_text += "\n";
            wrong_text += QString("%1 的输入数量 (%2) 超过了可用数量 (%3)！")
                .arg(typeNames[i]).arg(counts[i]).arg(available[i]);
        }
    }
    
    if(!wrong_text.isEmpty())
    {
         QMessageBox::warning(this, "题目数量超限", wrong_text);
         return;
    }
    
    // 保存当前配置到 storage 的 current config
    exam_config cfg = getConfigFromUi();
    storage_.set_current_exam_config(cfg);
    
    emit startExam(counts, scores, duration);
}
