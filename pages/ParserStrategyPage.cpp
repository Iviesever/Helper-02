#include "ParserStrategyPage.h"
#include "../storage_manager.h"
#include "../parser/text_parser.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDialog>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QLabel>
#include <QScroller>
#include <QScrollBar>

ParserStrategyPage::ParserStrategyPage(storage_manager* storage, QWidget *parent)
    : QWidget(parent), storage_(storage)
{
    ui.setupUi(this);

    // 启用惯性滚动，提升手感
    QScroller::grabGesture(ui.scrollArea->viewport(), QScroller::LeftMouseButtonGesture);
    ui.scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 连接信号
    connect(ui.btnBack, &QPushButton::clicked, this, &ParserStrategyPage::backClicked);
    connect(ui.btnNew, &QPushButton::clicked, this, &ParserStrategyPage::onNewClicked);
    connect(ui.btnSave, &QPushButton::clicked, this, &ParserStrategyPage::onSaveClicked);
    connect(ui.btnDelete, &QPushButton::clicked, this, &ParserStrategyPage::onDeleteClicked);
    connect(ui.btnTest, &QPushButton::clicked, this, &ParserStrategyPage::onTestClicked);
    connect(ui.comboStrategies, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ParserStrategyPage::onStrategySelected);

    connect(ui.comboStrategies, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ParserStrategyPage::onStrategySelected);

    setupReferenceSection();
    refreshStrategyList();
}

void ParserStrategyPage::refreshStrategyList()
{
    ui.comboStrategies->blockSignals(true);
    ui.comboStrategies->clear();

    // 添加默认策略
    ui.comboStrategies->addItem("默认策略", "");

    // 添加保存的策略
    auto names = storage_->get_parser_strategy_names();
    for (const auto& name : names)
    {
        ui.comboStrategies->addItem(QString::fromStdString(name), QString::fromStdString(name));
    }

    ui.comboStrategies->blockSignals(false);

    // 选中第一项
    if (ui.comboStrategies->count() > 0)
    {
        ui.comboStrategies->setCurrentIndex(0);
        onStrategySelected(0);
    }
}

QString ParserStrategyPage::currentStrategyName() const
{
    return ui.comboStrategies->currentData().toString();
}

void ParserStrategyPage::onStrategySelected(int index)
{
    isNewStrategy_ = false;
    ui.editName->setEnabled(false);

    if (index <= 0 || ui.comboStrategies->currentData().toString().isEmpty())
    {
        // 默认策略
        loadStrategyToUI(parser_strategy::get_default());
        ui.btnDelete->setEnabled(false);
    }
    else
    {
        QString name = ui.comboStrategies->currentData().toString();
        auto strategy = storage_->get_parser_strategy(name.toStdString());
        if (strategy)
        {
            loadStrategyToUI(*strategy);
            ui.btnDelete->setEnabled(true);
        }
    }
}

void ParserStrategyPage::onNewClicked()
{
    isNewStrategy_ = true;
    
    // 基于当前策略创建新策略
    parser_strategy newStrategy = getStrategyFromUI();
    newStrategy.name = "新策略";
    loadStrategyToUI(newStrategy);
    
    ui.editName->setEnabled(true);
    ui.editName->setFocus();
    ui.editName->selectAll();
    ui.btnDelete->setEnabled(false);
}

void ParserStrategyPage::onSaveClicked()
{
    QString name = ui.editName->text().trimmed();
    if (name.isEmpty())
    {
        QMessageBox::warning(this, "错误", "策略名称不能为空！");
        return;
    }

    if (name == "默认策略")
    {
        QMessageBox::warning(this, "错误", "不能使用\"默认策略\"作为名称！");
        return;
    }

    parser_strategy strategy = getStrategyFromUI();
    strategy.name = name.toStdString();

    // 检查是否存在同名策略
    if (isNewStrategy_)
    {
        auto existing = storage_->get_parser_strategy(name.toStdString());
        if (existing)
        {
            auto reply = QMessageBox::question(this, "确认",
                QString("策略 \"%1\" 已存在，是否覆盖？").arg(name),
                QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes)
                return;
        }
    }

    storage_->save_parser_strategy(strategy);
    
    QMessageBox::information(this, "成功", "策略已保存！");
    
    refreshStrategyList();
    
    // 选中刚保存的策略
    for (int i = 0; i < ui.comboStrategies->count(); ++i)
    {
        if (ui.comboStrategies->itemData(i).toString() == name)
        {
            ui.comboStrategies->setCurrentIndex(i);
            break;
        }
    }

    emit strategyChanged(name);
}

void ParserStrategyPage::onDeleteClicked()
{
    QString name = ui.comboStrategies->currentData().toString();
    if (name.isEmpty()) return;

    auto reply = QMessageBox::question(this, "确认",
        QString("确定要删除策略 \"%1\"？").arg(name),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) return;

    storage_->delete_parser_strategy(name.toStdString());
    
    refreshStrategyList();
    
    emit strategyChanged("");
}

void ParserStrategyPage::onTestClicked()
{
    // 选择文件进行测试
    QString filePath = QFileDialog::getOpenFileName(this, "选择测试文件", "", "Text Files (*.txt)");
    if (filePath.isEmpty()) return;

    // 使用当前编辑的策略进行解析
    parser_strategy strategy = getStrategyFromUI();
    text_parser parser(strategy);

    std::vector<question> questions;
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
         QTextStream in(&file);
         in.setEncoding(QStringConverter::Utf8);
         QString content = in.readAll();
         std::string std_content = content.toStdString();
         
         std::string path_str = filePath.toStdString();
         size_t last_slash = path_str.find_last_of("/\\");
         std::string simple_filename = (last_slash != std::string::npos) ? path_str.substr(last_slash + 1) : path_str;

         questions = parser.parse(std_content, simple_filename);
         file.close();
    }
    else
    {
         QMessageBox::critical(this, "错误", "无法打开测试文件！");
         return;
    }

    // 构建结果文本
    QString result = QString("解析结果：共 %1 题\n\n").arg(questions.size());
    
    for (size_t i = 0; i < questions.size(); ++i)
    {
        const auto& q = questions[i];
        QString typeStr;
        switch (q.type)
        {
            case question_type::single: typeStr = "单选"; break;
            case question_type::multi: typeStr = "多选"; break;
            case question_type::judge: typeStr = "判断"; break;
            case question_type::fill: typeStr = "填空"; break;
            default: typeStr = "未知"; break;
        }
        
        QString contentPreview = QString::fromStdString(q.content);
        if (contentPreview.length() > 80)
            contentPreview = contentPreview.left(80) + "...";
        
        result += QString("%1. [%2] %3\n   答案: %4\n\n")
            .arg(i + 1)
            .arg(typeStr)
            .arg(contentPreview)
            .arg(QString::fromStdString(q.correct_answer));
    }

    // 创建可滚动对话框
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("测试结果");
    dialog->resize(500, 400);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    QTextEdit* textEdit = new QTextEdit();
    textEdit->setPlainText(result);
    textEdit->setReadOnly(true);
    textEdit->setLineWrapMode(QTextEdit::WidgetWidth);
    layout->addWidget(textEdit);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    layout->addWidget(buttonBox);

    dialog->exec();
}

void ParserStrategyPage::loadStrategyToUI(const parser_strategy& strategy)
{
    ui.editName->setText(QString::fromStdString(strategy.name));
    ui.editSingleKeywords->setText(QString::fromStdString(strategy.single_keywords));
    ui.editMultiKeywords->setText(QString::fromStdString(strategy.multi_keywords));
    ui.editJudgeKeywords->setText(QString::fromStdString(strategy.judge_keywords));
    ui.editFillKeywords->setText(QString::fromStdString(strategy.fill_keywords));
    ui.editAnswerKeywords->setText(QString::fromStdString(strategy.answer_keywords));
    ui.editGarbagePatterns->setPlainText(QString::fromStdString(strategy.garbage_patterns));
    ui.editJudgeTrue->setText(QString::fromStdString(strategy.judge_true_values));
    ui.editJudgeFalse->setText(QString::fromStdString(strategy.judge_false_values));
}

parser_strategy ParserStrategyPage::getStrategyFromUI()
{
    parser_strategy strategy;
    strategy.name = ui.editName->text().toStdString();
    strategy.single_keywords = ui.editSingleKeywords->text().toStdString();
    strategy.multi_keywords = ui.editMultiKeywords->text().toStdString();
    strategy.judge_keywords = ui.editJudgeKeywords->text().toStdString();
    strategy.fill_keywords = ui.editFillKeywords->text().toStdString();
    strategy.answer_keywords = ui.editAnswerKeywords->text().toStdString();
    strategy.garbage_patterns = ui.editGarbagePatterns->toPlainText().toStdString();
    strategy.judge_true_values = ui.editJudgeTrue->text().toStdString();
    strategy.judge_false_values = ui.editJudgeFalse->text().toStdString();
    return strategy;
}

void ParserStrategyPage::setEditable(bool editable)
{
    ui.editName->setEnabled(editable);
}

void ParserStrategyPage::setupReferenceSection()
{
    // 1. is_question_start
    addReferenceItem("题目开始检测 (is_question_start)",
        "系统自动检测题目开始。使用 UTF-8 字节处理，支持半角点(.)、全角点(．)和顿号(、)。",
        R"(bool text_parser::is_question_start(std::string_view line) const
{
    auto t = trim(line);
    if (t.empty()) return false;
    
    // 第一个字符必须是数字
    if (t[0] < '0' || t[0] > '9') return false;
    
    // 跳过数字，检查后面是否是点号
    // 支持: . (半角), ．(全角 U+FF0E), 、(顿号 U+3001)
    // ...
    return true;
})");

    // 2. is_garbage_line
    addReferenceItem("垃圾行过滤 (is_garbage_line)",
        "使用【过滤规则】配置。在解析前，这些行会被直接丢弃。支持纯文本关键词匹配和正则表达式。",
        R"(bool text_parser::is_garbage_line(std::string_view line) const
{
    auto t = trim(line);
    if(t.empty()) return true;

    // 1. Regex check (for patterns like "^\s*\d+分\s*$")
    for(const auto & re : re_garbage_list_)
        if(std::regex_match(t.begin(), t.end(), re)) return true;

    // 2. Plain text check (e.g., "AI讲解", "查看作答记录")
    // ...
    return false;
})");

    // 3. detect_type
    addReferenceItem("题型检测 (detect_type)",
        "确定题目类型。优先检查【单选/多选/判断/填空关键词】，如果未匹配则根据选项特征自动推断。",
        R"(question_type text_parser::detect_type(std::string_view block, std::string_view answer_raw) const
{
    // 优先：关键词检测
    if (contains_keyword(block, strategy_.multi_keywords)) return question_type::multi;
    if (contains_keyword(block, strategy_.judge_keywords)) return question_type::judge;
    // ...

    // 推断：根据选项特征 (支持全角/半角点号)
    if (has_option(block, 'A') && has_option(block, 'B'))
        return question_type::single;

    return question_type::unknown;
})");

    // 4. parse_single_block
    addReferenceItem("单题解析 (parse_single_block)",
        "解析独立的题目块。分割题干、选项和答案。\n使用【答案关键词】定位答案，自动支持内联答案 (A) 提取。",
        R"(question text_parser::parse_single_block(std::string_view block, ...) const
{
    // 1. 查找答案关键词 ("正确答案", "答案：" 等)
    // 2. 如未找到，尝试提取内联答案 (A) 或 （A）
    // 3. 手动扫描选项位置 (支持全角/半角点)
    // 4. 清理题号前缀
    return q;
})");
}

void ParserStrategyPage::addReferenceItem(const QString& title, const QString& desc, const QString& code)
{
    QFrame* frame = new QFrame();
    frame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    // 启用背景自动填充
    frame->setAutoFillBackground(true);
    // 关键修正：使用 setBackgroundRole 而不是 setPalette
    // 这样当全局 Palette 改变时，Frame 会自动切换到新的 Base 颜色，而不是固定在创建时的颜色
    frame->setBackgroundRole(QPalette::Base);

    QVBoxLayout* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(5);

    // 标题
    QLabel* lblTitle = new QLabel(title);
    // 设置字体
    QFont font = lblTitle->font();
    font.setBold(true);
    font.setPointSize(11);
    lblTitle->setFont(font);
    // 关键：设置前景色角色为 Text（与 Base 背景配套），确保跟随主题变化
    lblTitle->setForegroundRole(QPalette::Text);
    layout->addWidget(lblTitle);

    // 描述
    QLabel* lblDesc = new QLabel(desc);
    lblDesc->setWordWrap(true);
    // 关键：同样设置前景色角色
    lblDesc->setForegroundRole(QPalette::Text);
    layout->addWidget(lblDesc);

    // 代码容器 (初始隐藏)
    QWidget* codeContainer = new QWidget();
    QVBoxLayout* codeLayout = new QVBoxLayout(codeContainer);
    codeLayout->setContentsMargins(0, 0, 0, 0);

    QTextEdit* textCode = new QTextEdit();
    textCode->setPlainText(code);
    textCode->setReadOnly(true);
    textCode->setFont(QFont("Consolas", 9));
    // 移除 QSS，使用原生外观
    textCode->setFrameShape(QFrame::Box);
    textCode->setFrameShadow(QFrame::Sunken);
    textCode->setFixedHeight(150); // 代码块固定高度
    
    codeLayout->addWidget(textCode);
    codeContainer->setVisible(false);

    // 切换按钮
    QToolButton* btnToggle = new QToolButton();
    btnToggle->setText("▶ 查看源码");
    btnToggle->setToolButtonStyle(Qt::ToolButtonTextOnly); // 只显示文字，不显示额外图标
    btnToggle->setAutoRaise(true);
    btnToggle->setCursor(Qt::PointingHandCursor);

    connect(btnToggle, &QToolButton::clicked, [btnToggle, codeContainer](bool) {
        bool visible = codeContainer->isVisible();
        codeContainer->setVisible(!visible);
        if (!visible) {
             btnToggle->setText("▼ 收起源码");
        } else {
             btnToggle->setText("▶ 查看源码");
        }
    });

    layout->addWidget(btnToggle);
    layout->addWidget(codeContainer);

    ui.layoutReferenceContent->addWidget(frame);
}
