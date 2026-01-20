#pragma once

#include <QtWidgets/QMainWindow>
#include <print>
#include <QLabel>
#include <QAbstractButton>
#include <QListWidget>
#include <functional>
#include <QMessageBox> 
#include <QTimer>
#include "ui_MainWindow.h"
#include "parser/text_parser.h"
#include "platform_utils.h"
#include "storage_manager.h"
#include <QFile>
#include <QTextStream>
#include <QStringConverter>

#include "pages/HomePage.h"
#include "pages/SettingsPage.h"
#include "pages/ExamConfigPage.h"
#include "pages/HistoryPage.h"
#include "pages/ParserStrategyPage.h"
#include "pages/PracticeStrategyPage.h"

#include <chrono>
#include <unordered_set>

inline QString to_QString(std::string currentQ)
{
    return QString::fromUtf8(currentQ.c_str());
}

class HistoryPage;
class ExamConfigPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT


public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() {}

protected:

    void applyTheme(bool dark)
    {
        QPalette palette;
        if(dark)
        {
            // 深色主题
            palette.setColor(QPalette::Window, QColor(53, 53, 53));
            palette.setColor(QPalette::WindowText, Qt::white);
            palette.setColor(QPalette::Base, QColor(25, 25, 25));
            palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
            palette.setColor(QPalette::ToolTipBase, Qt::white);
            palette.setColor(QPalette::ToolTipText, Qt::white);
            palette.setColor(QPalette::Text, Qt::white);
            palette.setColor(QPalette::Button, QColor(53, 53, 53));
            palette.setColor(QPalette::ButtonText, Qt::white);
            palette.setColor(QPalette::BrightText, Qt::red);
            palette.setColor(QPalette::Link, QColor(42, 130, 218));
            palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
            palette.setColor(QPalette::HighlightedText, Qt::black);
            palette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
            palette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);
        }
        else
        {
            // 浅色主题
            palette.setColor(QPalette::Window, QColor(240, 240, 240));
            palette.setColor(QPalette::WindowText, Qt::black);
            palette.setColor(QPalette::Base, Qt::white);
            palette.setColor(QPalette::AlternateBase, QColor(233, 233, 233));
            palette.setColor(QPalette::ToolTipBase, Qt::white);
            palette.setColor(QPalette::ToolTipText, Qt::black);
            palette.setColor(QPalette::Text, Qt::black);
            palette.setColor(QPalette::Button, QColor(240, 240, 240));
            palette.setColor(QPalette::ButtonText, Qt::black);
            palette.setColor(QPalette::BrightText, Qt::red);
            palette.setColor(QPalette::Link, QColor(42, 130, 218));
            palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
            palette.setColor(QPalette::HighlightedText, Qt::white);
            palette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
            palette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);
        }
        qApp->setPalette(palette);

        // 恢复 HomePage 列表控件的原生滚动条样式
        if(homePage_) homePage_->listWidgetFiles()->setStyleSheet("");

        // 为 page_Quiz 应用特定样式以确保背景遵循主题
        // (覆盖 .ui 中定义的可能导致问题的静态样式表)
        QString commonStyles =
            "QWidget { font-family: \"Microsoft YaHei\"; font-size: 12pt; }"
            "QRadioButton, QCheckBox { spacing: 10px; padding: 5px; }"
            "QRadioButton::indicator, QCheckBox::indicator { width: 20px; height: 20px; subcontrol-position: top left; subcontrol-origin: padding; margin-top: 3px; }";

        QString quizStyles;
        if(dark)
        {
            quizStyles =
                "QWidget#page_Quiz { background-color: #353535; color: #ffffff; }"
                "QFrame#frame_QuizBottom { background-color: #252525; border-top: 1px solid #505050; }";
        }
        else
        {
            quizStyles =
                "QWidget#page_Quiz { background-color: #f5f5f5; color: #000000; }"
                "QFrame#frame_QuizBottom { background-color: #e0e0e0; border-top: 1px solid #d0d0d0; }";
        }

        ui.page_Quiz->setStyleSheet(commonStyles + quizStyles);
    }
    
    // 更新选项样式
    void updateOptionStyle(QWidget * container, bool checked)
    {
        bool isDark = storage.config().dark_mode;
        
        if(checked)
        {
            if (isDark) {
                container->setStyleSheet(
                    "QWidget#optionContainer {"
                    "  background-color: #0D47A1;" 
                    "  border: 2px solid #2196f3;"
                    "  border-radius: 8px;"
                    "}"
                );
            } else {
                container->setStyleSheet(
                    "QWidget#optionContainer {"
                    "  background-color: #e3f2fd;"
                    "  border: 2px solid #2196f3;"
                    "  border-radius: 8px;"
                    "}"
                );
            }
        }
        else
        {
            if (isDark) {
                container->setStyleSheet(
                    "QWidget#optionContainer {"
                    "  background-color: #424242;"
                    "  border: 2px solid transparent;"
                    "  border-radius: 8px;"
                    "}"
                    "QWidget#optionContainer:hover {"
                    "  background-color: #616161;"
                    "  border-color: #2196f3;"
                    "}"
                );
            } else {
                container->setStyleSheet(
                    "QWidget#optionContainer {"
                    "  background-color: #f5f5f5;"
                    "  border: 2px solid transparent;"
                    "  border-radius: 8px;"
                    "}"
                    "QWidget#optionContainer:hover {"
                    "  background-color: #e8e8e8;"
                    "  border-color: #2196f3;"
                    "}"
                );
            }
        }
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if(event->type() == QEvent::MouseButtonRelease)
        {
            QWidget * container = watched->property("targetContainer").value<QWidget *>();
            if(container && container->isEnabled())
            {
                const question & q = curr_questions_[curr_index_];
                bool isMulti = (q.type == question_type::multi);

                bool currentChecked = container->property("isChecked").toBool();

                if(isMulti)
                {
                    // 多选：切换选中状态
                    container->setProperty("isChecked", !currentChecked);
                    updateOptionStyle(container, !currentChecked);
                }
                else
                {
                    // 单选/判断：取消其他选项，选中当前
                    for(int i = 0; i < ui.layout_Options->count(); ++i)
                    {
                        QLayoutItem * item = ui.layout_Options->itemAt(i);
                        QWidget * opt = item->widget();
                        if(opt && opt->objectName() == "optionContainer")
                        {
                            opt->setProperty("isChecked", false);
                            updateOptionStyle(opt, false);
                        }
                    }
                    container->setProperty("isChecked", true);
                    updateOptionStyle(container, true);

                    // 自动提交（如果启用）
                    if(storage.config().auto_submit)
                    {
                        QTimer::singleShot(100, this, &MainWindow::handleSubmitAnswer);
                    }
                }
                return true;
            }
        }
        return QMainWindow::eventFilter(watched, event);
    }

private slots:

    void handleRepoChanged(int index);       // 题库切换

    void handleOpenExamConfig();             // 考试配置



    void handleSubmitAnswer(); // 提交答案

    void handleToggleProgress();     // 答题卡

    void handlePrevQuestion();        // 上一题
    void handleNextQuestion();        // 下一题

    void handleOpenHistory();              // 查看历史
    void handleExitQuiz();          // 退出答题

private:

    Ui::MainWindowClass ui;

    void finish_exam(); // 结算考试/练习

    storage_manager storage;

    std::vector<question> curr_questions_;   // 当前所有题目
    std::vector<answer_state> curr_results_; // 当前所有题目的回答状态
    std::vector<QString> user_answers_;      // 当前所有题目的用户答案 (为了回显)
    int curr_index_{};                       // 当前第几题
    bool is_exam_mode_{ false };             // 是否处于考试模式
    bool is_view_mode_{ false };             // 是否处于看题模式
    std::chrono::steady_clock::time_point exam_start_time_; // 考试开始时间 (高精度)
    QTimer * exam_timer_ = nullptr;          // 考试计时器

    HomePage * homePage_ = nullptr;          // 主页 Widget
    SettingsPage * settingsPage_ = nullptr;  // 设置页 Widget
    HistoryPage * historyPage_ = nullptr;    // 历史页 Widget
    ExamConfigPage * examConfigPage_ = nullptr; // 考试配置页 Widget
    ParserStrategyPage * parserStrategyPage_ = nullptr; // 解析策略页 Widget
    PracticeStrategyPage * practiceStrategyPage_ = nullptr; // 刷题策略页 Widget

    QButtonGroup * m_btnGroup = nullptr;

    void show_question(int index);

    void process(std::function<std::span<question>(std::span<question>)> processor,
        bool shuffle = false);

    // 考试配置辅助函数


    bool init_start()
    {
        auto selected_items = homePage_->listWidgetFiles()->selectedItems();

        if(selected_items.empty())
        {
            QMessageBox::warning(this, "提示", "请先点击列表选中至少一个文件！");
            return false;
        }

        std::vector<std::string> checked_paths;
        checked_paths.reserve(selected_items.size());

        // 提取路径
        for(auto * item : selected_items)
        {
            checked_paths.push_back(item->data(Qt::UserRole).toString().toStdString());
        }

        // 根据用户选择的策略创建解析器
        QString strategyName = homePage_->comboParser()->currentData().toString();
        text_parser parser;
        if (!strategyName.isEmpty())
        {
            auto strategy = storage.get_parser_strategy(strategyName.toStdString());
            if (strategy)
            {
                parser = text_parser(*strategy);
            }
        }

        std::vector<question> loaded_questions;
        for (const auto& path : checked_paths)
        {
            QFile file(QString::fromStdString(path));
            if (file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QTextStream in(&file);
                in.setEncoding(QStringConverter::Utf8);
                // 先全部读入字符串
                QString content = in.readAll();
                std::string std_content = content.toStdString();
                
                // 手动提取文件名以避免 std::filesystem 系统错误
                std::string filename = path;
                size_t last_slash = filename.find_last_of("/\\");
                if (last_slash != std::string::npos) {
                    filename = filename.substr(last_slash + 1);
                }
                
                auto qs = parser.parse(std_content, filename);
                loaded_questions.insert(loaded_questions.end(), std::make_move_iterator(qs.begin()), std::make_move_iterator(qs.end()));
                file.close();
            }
            else
            {
               qCritical() << "Failed to open file:" << QString::fromStdString(path);
            }
        }
        
        // 去重 (根据设置决定是否去重)
        std::vector<question> questions;
        questions.reserve(loaded_questions.size());
        
        if (practiceStrategyPage_->excludeDuplicates())
        {
            std::unordered_set<size_t> seen_ids;
            for(const auto & q : loaded_questions)
            {
                size_t id = q.get_id();
                if(seen_ids.find(id) == seen_ids.end())
                {
                    questions.push_back(q);
                    seen_ids.insert(id);
                }
            }
        }
        else
        {
            questions = std::move(loaded_questions);
        }

        if(questions.empty())
        {
            QMessageBox::warning(this, "提示", "文件里无题目！");
            return false;
        }

        curr_questions_.clear();
        curr_index_ = 0;
        user_answers_.clear();

        auto option{ homePage_->mistakeOp() };
        auto cnt{ static_cast<size_t>(homePage_->mistakeCount()) };

        auto predicate = [=, this](const question & q)
            {
                if(option == 1) return storage.get_mistake_count(q) >= static_cast<int>(cnt);
                if(option == 2) return storage.get_mistake_count(q) == static_cast<int>(cnt);
                return true;
            };

        for(auto & q : questions)
        {
            bool should_add = false;

            switch(q.type)
            {
                case question_type::single:
                    if(homePage_->isSingleChecked())
                    {
                        should_add = true;
                        // 使用优先级跳过逻辑
                        if (practiceStrategyPage_->shouldSkipSingle(QString::fromStdString(q.correct_answer)))
                        {
                            should_add = false;
                        }
                    }
                    break;
                case question_type::multi:
                    if(homePage_->isMultiChecked())
                    {
                        // 如果启用了排除多选全选题，检查答案是否包含所有选项
                        if (practiceStrategyPage_->excludeMultiAll() && !q.options.empty())
                        {
                            // 检查答案长度是否等于选项数量
                            if (q.correct_answer.length() >= q.options.size())
                            {
                                should_add = false; // 排除全选题
                            }
                            else
                            {
                                should_add = true;
                            }
                        }
                        else
                        {
                            should_add = true;
                        }
                    }
                    break;
                case question_type::judge:
                    if(homePage_->isJudgeChecked())
                    {
                        should_add = true;
                        // 使用优先级跳过逻辑
                        if (practiceStrategyPage_->shouldSkipJudge(QString::fromStdString(q.correct_answer)))
                        {
                            should_add = false;
                        }
                    }
                    break;
                case question_type::fill:
                    if(homePage_->isFillChecked())   should_add = true;
                    break;
                default:
                    break;
            }

            if(should_add && predicate(q))
                curr_questions_.emplace_back(std::move(q));

        }

        if(curr_questions_.empty())
        {
            QMessageBox::warning(this, "提示", "筛选后无题目！");
            return false;
        }

        // 初始化用户答案记录
        user_answers_.resize(curr_questions_.size());

        return true;
    }

    void clear_layout(QLayout * layout) // 清空布局
    {
        if(!layout) return;
        QLayoutItem * item;
        
        while((item = layout->takeAt(0)) != nullptr)
        { // 循环取布局里的第一个元素，直到取空
            if(item->widget())
            {
                delete item->widget();
            }
            delete item;
        }
    }



};

