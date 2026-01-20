#include "MainWindow.h"

#include <QApplication>

#include <algorithm> 
#include <ranges>    
#include <random>    
#include <array>
#include <QLabel>
#include <QAbstractButton>
#include <functional>
#include <QMessageBox> 
#include <QDateTime>
#include <QPlainTextEdit>
#include <QButtonGroup>
#include <QPushButton>
#include <QHBoxLayout>
#include <QTimer>
#include <QFileDialog>
#include <QScroller>
#include <QScrollBar>

MainWindow::MainWindow(QWidget * parent)
	: QMainWindow(parent), storage(platform_utils::get_repo_path())
{
	ui.setupUi(this);

	// 立即应用主题
	applyTheme(storage.config().dark_mode);

	// 创建并嵌入 HomePage
	homePage_ = new HomePage(this);
	ui.layout_HomeContainer->addWidget(homePage_);

	// 创建并嵌入 SettingsPage
	settingsPage_ = new SettingsPage(this);
	ui.layout_SettingsContainer->addWidget(settingsPage_);

	// 创建并嵌入 HistoryPage
	historyPage_ = new HistoryPage(this);
	ui.layout_HistoryContainer->addWidget(historyPage_);

	// 创建并嵌入 ExamConfigPage
	examConfigPage_ = new ExamConfigPage(storage, this);
	ui.layout_ExamConfigContainer->addWidget(examConfigPage_);

	// 创建并嵌入 ParserStrategyPage
	parserStrategyPage_ = new ParserStrategyPage(&storage, this);
	ui.layout_ParserStrategyContainer->addWidget(parserStrategyPage_);

	// 创建并嵌入 PracticeStrategyPage
	practiceStrategyPage_ = new PracticeStrategyPage(this);
	ui.layout_PracticeStrategyContainer->addWidget(practiceStrategyPage_);

#if defined(Q_OS_ANDROID)
	platform_utils::request_android_permissions(); // Android 申请权限
#endif

	// 加载题库列表到 HomePage
	auto repos = platform_utils::get_repo_dir();

	std::println("加载题库列表: repos.size() = {}", repos.size());

	for(auto & r : repos)
	{
		std::println("题库列表: {}", r);
		homePage_->comboRepo()->addItem(QString::fromStdString(r));
	}

	// 更新错题次数选择器
	homePage_->updateMistakeCountCombo(storage.get_max_mistake());

	// HomePage 信号连接
	connect(homePage_, &HomePage::repoChanged, this, &MainWindow::handleRepoChanged);
	
	connect(homePage_, &HomePage::openExamConfig, this, &MainWindow::handleOpenExamConfig);
	
	connect(homePage_, &HomePage::openHistory, this, &MainWindow::handleOpenHistory);

	if(homePage_->comboRepo()->count() > 0)
	{
		handleRepoChanged(0);
	}

	// 刷新策略下拉框
	auto refreshParserCombo = [this]() {
		homePage_->comboParser()->blockSignals(true);
		homePage_->comboParser()->clear();
		homePage_->comboParser()->addItem("默认策略", "");
		auto names = storage.get_parser_strategy_names();
		for (const auto& name : names) {
			homePage_->comboParser()->addItem(QString::fromStdString(name), QString::fromStdString(name));
		}
		homePage_->comboParser()->blockSignals(false);
	};
	refreshParserCombo();

	// 初始化考试计时器
	exam_timer_ = new QTimer(this);

    // 设置导航按钮样式
    ui.btnPrevQ->setStyleSheet(
        "QPushButton { background-color: #757575; color: white; border-radius: 6px; border: none; padding: 8px 16px; font-weight: bold; } "
        "QPushButton:hover { background-color: #9E9E9E; } "
        "QPushButton:pressed { background-color: #616161; }"
    );
    ui.btnNextQ->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; border-radius: 6px; border: none; padding: 8px 16px; font-weight: bold; } "
        "QPushButton:hover { background-color: #64B5F6; } "
        "QPushButton:pressed { background-color: #1976D2; }"
    );


	connect(exam_timer_, &QTimer::timeout, this, [this]()
		{
			if(is_exam_mode_)
			{
				qint64 duration_min = storage.get_exam_config().exam_duration;
				qint64 total_sec = duration_min * 60;
				
				auto now = std::chrono::steady_clock::now();
				qint64 elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - exam_start_time_).count();
				
				qint64 remaining = total_sec - elapsed;

				if(remaining <= 0)
				{
					remaining = 0;
					exam_timer_->stop();
					ui.lbl_ExamTimer->setText("⏱ 00:00");
					
					// 自动交卷
					finish_exam();
					
					QMessageBox::information(this, "考试结束", "考试时间到，已自动交卷！");
				}
				else
				{
					int mins = remaining / 60;
					int secs = remaining % 60;
					ui.lbl_ExamTimer->setText(QString("⏱ %1:%2")
						.arg(mins, 2, 10, QChar('0'))
						.arg(secs, 2, 10, QChar('0')));
				}
			}
		});
	//信号连接
	connect(ui.btnSubmitAnswer, &QPushButton::clicked, this, &MainWindow::handleSubmitAnswer);
	connect(ui.btnNextQ, &QPushButton::clicked, this, &MainWindow::handleNextQuestion);
	connect(ui.btnPrevQ, &QPushButton::clicked, this, &MainWindow::handlePrevQuestion);
	connect(ui.btnProgress, &QPushButton::clicked, this, &MainWindow::handleToggleProgress);
	connect(ui.btnExitQuiz, &QPushButton::clicked, this, &MainWindow::handleExitQuiz);

    // 返回按钮 (History -> Home) - 委托给 HistoryPage
    connect(historyPage_, &HistoryPage::backClicked, this, [this]() {
        ui.stackedWidget->setCurrentWidget(ui.page_Home);
    });

    // 返回按钮 (ExamConfig -> Home) - 委托给 ExamConfigPage
    connect(examConfigPage_, &ExamConfigPage::backClicked, this, [this]() {
        ui.stackedWidget->setCurrentWidget(ui.page_Home);
    });

    // 解析策略管理页信号连接
    connect(homePage_, &HomePage::openParserStrategy, this, [this]() {
        parserStrategyPage_->refreshStrategyList();
        ui.stackedWidget->setCurrentWidget(ui.page_ParserStrategy);
    });

    connect(parserStrategyPage_, &ParserStrategyPage::backClicked, this, [this, refreshParserCombo]() {
        refreshParserCombo();
        ui.stackedWidget->setCurrentWidget(ui.page_Home);
    });

    connect(parserStrategyPage_, &ParserStrategyPage::strategyChanged, this, [this, refreshParserCombo](const QString& name) {
        refreshParserCombo();
        // 选中刚保存的策略
        for (int i = 0; i < homePage_->comboParser()->count(); ++i) {
            if (homePage_->comboParser()->itemData(i).toString() == name) {
                homePage_->comboParser()->setCurrentIndex(i);
                break;
            }
        }
    });

    // 刷题策略页信号连接
    connect(homePage_, &HomePage::openPracticeStrategy, this, [this]() {
        // 验证是否选择了文件
        auto selected_items = homePage_->listWidgetFiles()->selectedItems();
        if (selected_items.empty()) {
            QMessageBox::warning(this, "提示", "请先点击列表选中至少一个文件！");
            return;
        }

        // 加载当前题库的策略
        QString repoName = homePage_->comboRepo()->currentText();
        auto strategy = storage.get_practice_strategy(repoName.toStdString());
        practiceStrategyPage_->loadStrategy(strategy);
        practiceStrategyPage_->setRepoName(repoName);

        // 自动扫描并加载统计
        std::vector<std::string> checked_paths;
        for (auto* item : selected_items) {
            checked_paths.push_back(item->data(Qt::UserRole).toString().toStdString());
        }

        // 根据用户选择的策略创建解析器
        QString strategyName = homePage_->comboParser()->currentData().toString();
        text_parser parser;
        if (!strategyName.isEmpty()) {
            auto parserStrategy = storage.get_parser_strategy(strategyName.toStdString());
            if (parserStrategy) {
                parser = text_parser(*parserStrategy);
            }
        }

        std::vector<question> questions;
        for (const auto& path : checked_paths) {
            QFile file(QString::fromStdString(path));
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                in.setEncoding(QStringConverter::Utf8);
                QString content = in.readAll();
                std::string std_content = content.toStdString();
                
                std::string filename = path;
                size_t last_slash = filename.find_last_of("/\\");
                if (last_slash != std::string::npos) {
                    filename = filename.substr(last_slash + 1);
                }
                
                auto qs = parser.parse(std_content, filename);
                questions.insert(questions.end(), std::make_move_iterator(qs.begin()), std::make_move_iterator(qs.end()));
                file.close();
            }
        }

        if (questions.empty()) {
            QMessageBox::warning(this, "提示", "文件里无题目！");
            return;
        }

        practiceStrategyPage_->loadStatistics(questions);
        ui.stackedWidget->setCurrentWidget(ui.page_PracticeStrategy);
    });

    connect(practiceStrategyPage_, &PracticeStrategyPage::backClicked, this, [this]() {
        // 保存策略到当前题库
        QString repoName = homePage_->comboRepo()->currentText();
        storage.save_practice_strategy(repoName.toStdString(), practiceStrategyPage_->getStrategy());
        ui.stackedWidget->setCurrentWidget(ui.page_Home);
    });


    // 考试配置页 -> 开始考试
    connect(examConfigPage_, &ExamConfigPage::startExam, this, [this](const std::array<size_t, 4>& counts, [[maybe_unused]] const std::array<double, 4>& scores, [[maybe_unused]] int duration) {
        
        exam_start_time_ = std::chrono::steady_clock::now();
        
        // 传递处理函数
        process([&](auto group) {
            int type_index = question::rank(group.front());
            size_t targetCount{};
            if(type_index >= 0 && type_index < 4)
                targetCount = counts[type_index];
            return group | std::views::take(targetCount); 
        }, true);

        is_exam_mode_ = true;
        is_view_mode_ = false;
        ui.btnSubmitAnswer->show();
        ui.lbl_ExamTimer->show();
        exam_timer_->start(1000);
        ui.lbl_ExamTimer->setText("⏱ 00:00");
        ui.stackedWidget->setCurrentWidget(ui.page_Quiz);

        show_question(0);
    });

    // 返回按钮 (Card -> Quiz)
    connect(ui.btnBackFromCard, &QPushButton::clicked, this, [this](){
        ui.stackedWidget->setCurrentWidget(ui.page_Quiz);
    });

	// HomePage -> 设置
	connect(homePage_, &HomePage::openSettings, this, [this]()
		{
			settingsPage_->loadConfig(storage.config());
			ui.stackedWidget->setCurrentWidget(ui.page_Settings);
		});

	// SettingsPage 信号连接
	connect(settingsPage_, &SettingsPage::backClicked, this, [this]()
		{
			ui.stackedWidget->setCurrentWidget(ui.page_Home);
		});

	connect(settingsPage_, &SettingsPage::browseRepoClicked, this, [this]()
		{
			QString dir = QFileDialog::getExistingDirectory(this, "选择题库文件夹", "/sdcard");
			if(!dir.isEmpty())
			{
				settingsPage_->setRepoPath(dir);
			}
		});

	connect(settingsPage_, &SettingsPage::browseDataClicked, this, [this]()
		{
			QString dir = QFileDialog::getExistingDirectory(this, "选择数据存储文件夹", "/sdcard");
			if(!dir.isEmpty())
			{
				settingsPage_->setDataPath(dir);
			}
		});

	// 保存设置
	connect(settingsPage_, &SettingsPage::saveClicked, this, [this]()
		{
			app_config new_config =
			{
				settingsPage_->fontTitleSize(),
				settingsPage_->fontBaseSize(),
				settingsPage_->autoSubmit(),
				settingsPage_->confirmExit(),
				settingsPage_->autoNext(),
				static_cast<size_t>(settingsPage_->fileListLimit()),
				settingsPage_->repoPath().toStdString(),
				settingsPage_->dataPath().toStdString(),
				settingsPage_->darkMode()
			};
            
			bool pathChanged = (new_config.custom_repo_path != storage.config().custom_repo_path) || 
			                   (new_config.custom_data_path != storage.config().custom_data_path);

			storage.update_config(new_config);

			// 如果已更改则应用主题
			applyTheme(new_config.dark_mode);

			// 如果路径改变，热重载题库列表
			if(pathChanged)
			{
				platform_utils::set_repo_path_override(QString::fromStdString(new_config.custom_repo_path));
				
				// 刷新题库下拉框
				auto repos = platform_utils::get_repo_dir();
				homePage_->comboRepo()->clear();
				for(const auto & r : repos)
					homePage_->comboRepo()->addItem(QString::fromStdString(r));
			}

			// 刷新文件列表以应用新的限制
			handleRepoChanged(homePage_->comboRepo()->currentIndex());

			ui.stackedWidget->setCurrentWidget(ui.page_Home);
		});


	// HomePage -> 顺序练习
	connect(homePage_, &HomePage::startSequentialPractice, this, [this]()
		{
			if(!init_start()) return;
			is_exam_mode_ = false;
			is_view_mode_ = false;
			exam_timer_->stop();
			ui.lbl_ExamTimer->hide();
			ui.btnSubmitAnswer->show();
			process([](auto group) { return group; });
			ui.stackedWidget->setCurrentWidget(ui.page_Quiz);
			show_question(0);
		});

	// HomePage -> 乱序练习
	connect(homePage_, &HomePage::startRandomPractice, this, [this]()
		{
			if(!init_start()) return;
			is_exam_mode_ = false;
			is_view_mode_ = false;
			exam_timer_->stop();
			ui.lbl_ExamTimer->hide();
			ui.btnSubmitAnswer->show();
			process([](auto group) { return group; }, true);
			ui.stackedWidget->setCurrentWidget(ui.page_Quiz);
			show_question(0);
		});

	// HomePage -> 看题模式
	connect(homePage_, &HomePage::startViewMode, this, [this]()
		{
			if(!init_start()) return;
			is_exam_mode_ = false;
			is_view_mode_ = true;
			exam_timer_->stop();
			ui.lbl_ExamTimer->hide();
			ui.btnSubmitAnswer->hide();  // 隐藏提交按钮
			process([](auto group) { return group; }, true);
			
			// 将所有题目标记为已正确回答
			for (size_t i = 0; i < curr_questions_.size(); ++i)
			{
				curr_results_[i] = answer_state::correct;
				user_answers_[i] = to_QString(curr_questions_[i].correct_answer);
			}
			
			ui.stackedWidget->setCurrentWidget(ui.page_Quiz);
			show_question(0);
		});

	// 返回按钮 
	connect(ui.btnBackFromCard, &QPushButton::clicked, this, [this]()
		{
			ui.stackedWidget->setCurrentWidget(ui.page_Quiz);
		});

}

void MainWindow::show_question(int index)
{
	// 1. 越界检查
	if(index < 0 || static_cast<size_t>(index) >= curr_questions_.size()) return;

	// 根据题目状态设置提交按钮
	if(curr_results_[index] == answer_state::correct)
	{
		ui.btnSubmitAnswer->setEnabled(false);
		ui.btnSubmitAnswer->setStyleSheet("background-color: #4CAF50; color: white; border: none; border-radius: 4px; padding: 6px;");
	}
	else if(curr_results_[index] == answer_state::wrong)
	{
		ui.btnSubmitAnswer->setEnabled(false);
		ui.btnSubmitAnswer->setStyleSheet("background-color: #F44336; color: white; border: none; border-radius: 4px; padding: 6px;");
	}
	else
	{
		ui.btnSubmitAnswer->setEnabled(true);
		// 显式重置为默认蓝色
		ui.btnSubmitAnswer->setStyleSheet("background-color: #1976D2; color: white; border: none; border-radius: 4px; padding: 6px;"); 
	}

	// 2. 获取当前题目数据
	const question & q = curr_questions_[index];
	curr_index_ = index; // 更新当前索引

	// 3. 更新题目文本
	// 加上题型前缀，例如 [单选题] 题目内容
	QString type_str;
	switch(q.type)
	{
		case question_type::single: type_str = "[单选题] "; break;
		case question_type::multi:  type_str = "[多选题] "; break;
		case question_type::judge:  type_str = "[判断题] "; break;
		case question_type::fill:   type_str = "[填空题] "; break;
		default: type_str = "[未知] "; break;
	}
	ui.lbl_QuestionContent->setText(type_str + to_QString(q.content));
	
	// 应用字体设置
	const auto & cfg = storage.config();
	ui.lbl_QuestionContent->setStyleSheet(QString("font-size: %1px;").arg(cfg.font_size));

	// 4. 更新进度条文字 (蓝色)
	ui.btnProgress->setText(QString("答题卡: %1/%2").arg(index + 1).arg(curr_questions_.size()));
	ui.btnProgress->setStyleSheet("color: #2196F3; font-weight: bold;");



	// 显示错误次数和来源文件 (分成两个label)
	QString sourceFile = to_QString(q.source_file);
	sourceFile = QUrl::fromPercentEncoding(sourceFile.toUtf8());
	// 只显示文件名，不显示路径
	if(sourceFile.contains('/')) sourceFile = sourceFile.mid(sourceFile.lastIndexOf('/') + 1);
	if(sourceFile.contains('\\')) sourceFile = sourceFile.mid(sourceFile.lastIndexOf('\\') + 1);
	if(sourceFile.contains(':')) sourceFile = sourceFile.mid(sourceFile.lastIndexOf(':') + 1);
	
	int errorCount = storage.get_mistake_count(q);
	ui.lbl_QuizInfoRight->setText(QString("错误: %1").arg(errorCount));
	// 错误0次灰色，1次以上红色
	if(errorCount > 0) {
		ui.lbl_QuizInfoRight->setStyleSheet("color: #D32F2F; font-weight: bold;");
	} else {
		ui.lbl_QuizInfoRight->setStyleSheet("color: gray;");
	}
	
	QString displaySource = sourceFile.length() > 15 ? sourceFile.left(12) + "..." : sourceFile;
	ui.lbl_QuizSource->setText(QString("来源: %1").arg(displaySource));


	// 5. 核心：动态生成选项

	// A. 清空上一题
	clear_layout(ui.layout_Options);

	// 重置 ButtonGroup (必须保留，用于单选互斥)
	if(m_btnGroup) { delete m_btnGroup; m_btnGroup = nullptr; }
	if(q.type == question_type::single || q.type == question_type::judge)
	{
		m_btnGroup = new QButtonGroup(this);
		m_btnGroup->setExclusive(true);
	}

	// B. 根据题型生成控件
	if(q.type == question_type::single || q.type == question_type::judge || q.type == question_type::multi)
	{

		for(const auto & optStr : q.options)
		{
			// 提取前缀 (A.) 和内容
			QString fullText = to_QString(optStr);
			QString prefix = ""; // "A"
			QString content = fullText;

			// 尝试分离 "A. Content"
			int dotIdx = fullText.indexOf('.');
			if(dotIdx > 0 && dotIdx <= 3)
			{
				prefix = fullText.left(dotIdx); // "A"
				content = fullText.mid(dotIdx + 1).trimmed();
			}

			// 创建选项容器
			QWidget * container = new QWidget();
			container->setProperty("optionKey", prefix);
			container->setProperty("fullText", fullText);
			container->setProperty("isChecked", false);
			container->setCursor(Qt::PointingHandCursor);
			updateOptionStyle(container, false);
			container->setObjectName("optionContainer");
			
			QHBoxLayout * hLayout = new QHBoxLayout(container);
			hLayout->setContentsMargins(10, 12, 10, 12);
			hLayout->setSpacing(8);
			
			// 选项字母标签 (A/B/C/D)
			QLabel * lblPrefix = new QLabel(prefix + ".");
			lblPrefix->setStyleSheet(QString("font-size: %1px; font-weight: bold; background: transparent;").arg(cfg.button_size));
			lblPrefix->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
			lblPrefix->setAlignment(Qt::AlignTop | Qt::AlignLeft);
			
			// 选项内容标签（支持自动换行）
			QLabel * lblContent = new QLabel(content);
			lblContent->setWordWrap(true);
			lblContent->setStyleSheet(QString("font-size: %1px; background: transparent;").arg(cfg.button_size));
			lblContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
			lblContent->setAlignment(Qt::AlignTop | Qt::AlignLeft);
			
			hLayout->addWidget(lblPrefix, 0, Qt::AlignTop);
			hLayout->addWidget(lblContent, 1, Qt::AlignTop);
			
			// 关联容器点击事件
			container->setProperty("targetContainer", QVariant::fromValue<QWidget*>(container));
			container->installEventFilter(this);
			lblPrefix->setProperty("targetContainer", QVariant::fromValue<QWidget*>(container));
			lblPrefix->installEventFilter(this);
			lblContent->setProperty("targetContainer", QVariant::fromValue<QWidget*>(container));
			lblContent->installEventFilter(this);

			ui.layout_Options->addWidget(container);
		}
	}
	else if(q.type == question_type::fill)
	{
		// 填空
		if (is_view_mode_)
		{
			// 看题模式：直接显示正确答案
			QLabel* lblAnswer = new QLabel();
			lblAnswer->setText(QString("%1").arg(to_QString(q.correct_answer)));
			lblAnswer->setStyleSheet(QString("font-size: %1px; color: #4caf50; font-weight: bold; padding: 12px; background-color: #e8f5e9; border-radius: 6px;").arg(cfg.button_size));
			lblAnswer->setWordWrap(true);
			lblAnswer->setObjectName("lbl_FillAnswer");
			ui.layout_Options->addWidget(lblAnswer);
		}
		else
		{
			// 正常模式：显示输入框
			QPlainTextEdit * edit = new QPlainTextEdit();
			edit->setPlaceholderText("请在此输入答案...");
			edit->setMaximumHeight(100); // 别太高
			edit->setStyleSheet(QString("font-size: %1px;").arg(cfg.button_size));
			// 给它起个名字方便后面 findChild 找到它
			edit->setObjectName("editor_Fill");

			ui.layout_Options->addWidget(edit);
		}
	}

	// 重新让滚动区适应内容变化（防止出现显示不全）
	// ui.scrollContent_Quiz->adjustSize(); 

	// 6. 状态恢复 (保存的状态)
	if (curr_results_[index] != answer_state::unanswered)
	{
		ui.btnSubmitAnswer->setEnabled(false);
		
		QString savedAns = user_answers_[index];
		QString correctAns = to_QString(q.correct_answer);



		// 2. 恢复选中状态
		if (q.type == question_type::fill)
		{
			// 通过遍历 layout 找到编辑器
			QPlainTextEdit* edit = nullptr;
			for(int i = 0; i < ui.layout_Options->count(); ++i)
			{
				QLayoutItem * item = ui.layout_Options->itemAt(i);
				if(item && item->widget())
				{
					edit = qobject_cast<QPlainTextEdit*>(item->widget());
					if(edit) break;
				}
			}
			
			if (edit)
			{
				edit->setPlainText(savedAns);
				edit->setReadOnly(true); // 禁止修改
				const auto & cfg = storage.config();
				
				bool wasCorrect = (curr_results_[index] == answer_state::correct);
				if(wasCorrect)
				{
					// 正确：绿色边框、背景和文字
					edit->setStyleSheet(QString(
						"QPlainTextEdit { font-size: %1px; color: #2e7d32; font-weight: bold; border: 3px solid #4caf50; background-color: #c8e6c9; border-radius: 6px; padding: 8px; }"
					).arg(cfg.button_size));
				}
				else
				{
					// 错误：红色边框、背景和文字
					edit->setStyleSheet(QString(
						"QPlainTextEdit { font-size: %1px; color: #c62828; font-weight: bold; border: 3px solid #f44336; background-color: #ffcdd2; border-radius: 6px; padding: 8px; }"
					).arg(cfg.button_size));
					
					// 显示正确答案
					QLabel* lblCorrectAns = new QLabel();
					lblCorrectAns->setObjectName("lbl_CorrectAnswer");
					lblCorrectAns->setText(QString("%1").arg(correctAns));
					lblCorrectAns->setStyleSheet(QString(
						"font-size: %1px; color: #2e7d32; font-weight: bold; padding: 12px; background-color: #c8e6c9; border: 2px solid #4caf50; border-radius: 6px;"
					).arg(cfg.button_size));
					lblCorrectAns->setWordWrap(true);
					lblCorrectAns->setAlignment(Qt::AlignLeft | Qt::AlignTop);
					ui.layout_Options->addWidget(lblCorrectAns);
				}
			}
		}
		else
		{
			// 遍历选项容器，恢复状态并高亮正确/错误选项
			for(int i = 0; i < ui.layout_Options->count(); ++i)
			{
				QLayoutItem * item = ui.layout_Options->itemAt(i);
				QWidget * container = item->widget();
				if(!container || container->objectName() != "optionContainer") continue;

				container->setEnabled(false); // 禁止修改

				QString optKey = container->property("optionKey").toString();
				bool isUserSelected = savedAns.contains(optKey);
				bool isCorrectOption = correctAns.contains(optKey);

				// 恢复选中状态
				if(isUserSelected) container->setProperty("isChecked", true);

				// 高亮选项颜色
				if(isCorrectOption)
				{
					// 正确选项：绿色
					container->setStyleSheet(
						"QWidget#optionContainer { background-color: #c8e6c9; border: 2px solid #4caf50; border-radius: 8px; }"
						"QLabel { color: #2e7d32; background: transparent; }"
					);
				}
				else if(isUserSelected)
				{
					// 用户选错的选项：红色
					container->setStyleSheet(
						"QWidget#optionContainer { background-color: #ffcdd2; border: 2px solid #f44336; border-radius: 8px; }"
						"QLabel { color: #c62828; background: transparent; }"
					);
				}
			}
		}
	}
}

void MainWindow::finish_exam()
{
    // 1. 计算得分
    const auto & cfg = storage.get_exam_config();
    double scores[4] = {
        cfg.single_score,
        cfg.multi_score,
        cfg.judge_score,
        cfg.fill_score
    };
	
	double totalScore = 0;
	int correctCount = 0;
	
	for(size_t i=0; i<curr_questions_.size(); ++i)
	{
		if(i < curr_results_.size() && curr_results_[i] == answer_state::correct)
		{
			 correctCount++;
			 int tIdx = question::rank(curr_questions_[i]);
			 if(tIdx >=0 && tIdx < 4) totalScore += scores[tIdx];
		}
	}
	
	
	// 2. 保存历史记录 (仅限考试模式)
	if(is_exam_mode_)
	{
		exam_timer_->stop(); // 停止计时器
		exam_record record;
		record.date = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString();
		record.repo_name = homePage_->comboRepo()->currentText().toStdString();
		record.score = totalScore;
		record.total_count = static_cast<int>(curr_questions_.size());
		record.correct_count = correctCount;
		
		auto now = std::chrono::steady_clock::now();
		record.duration_sec = std::chrono::duration_cast<std::chrono::seconds>(now - exam_start_time_).count();
		
		storage.add_exam_record(record);
	}
	
	// 3. 显示结果
	QString msg;
	if(is_exam_mode_)
	{
		msg = QString("考试结束！\n\n得分: %1\n正确题数: %2 / %3\n正确率: %4%")
			.arg(totalScore)
			.arg(correctCount).arg(curr_questions_.size())
			.arg(curr_questions_.empty() ? 0 : (correctCount * 100.0 / curr_questions_.size()), 0, 'f', 1);
	}
	else
	{
		msg = QString("练习结束！\n\n正确题数: %1 / %2\n正确率: %3%")
			.arg(correctCount).arg(curr_questions_.size())
			.arg(curr_questions_.empty() ? 0 : (correctCount * 100.0 / curr_questions_.size()), 0, 'f', 1);
	}
		
	QMessageBox::information(this, is_exam_mode_ ? "成绩单" : "练习完成", msg);
	
	// 4. 返回主页
	ui.stackedWidget->setCurrentWidget(ui.page_Home);
}

void MainWindow::process(std::function<std::span<question>(std::span<question>)> processor, bool shuffle)
{
	// 排序
	std::ranges::sort(curr_questions_, {}, question::rank);

	std::vector<question> final_questions;
	final_questions.reserve(curr_questions_.size());

	// 分组
	auto groups = curr_questions_ | std::views::chunk_by([](const question & a, const question & b)
		{
			return a.type == b.type;
		});

	std::mt19937 gen{ std::random_device{}() };

	for(auto group_view : groups)
	{
		if(group_view.empty()) continue;

		if(shuffle) std::ranges::shuffle(group_view, gen);

		auto selected_span = processor(group_view); // 注入

		final_questions.append_range(selected_span);
	}

	curr_questions_ = std::move(final_questions);

	curr_results_.clear();
	curr_results_.resize(curr_questions_.size(), answer_state::unanswered);
}

// 切换题库
void MainWindow::handleRepoChanged(int index)
{
	auto repos = platform_utils::get_repo_dir();
	if(index < 0 || static_cast<size_t>(index) >= repos.size()) return;

	auto txt_files = platform_utils::get_repo_file(repos[index]);

	auto* listWidget = homePage_->listWidgetFiles();
	listWidget->clear();

	// 多选模式
	listWidget->setSelectionMode(QAbstractItemView::MultiSelection);

	size_t limit = storage.config().file_list_limit;

	// 限制控制可见行数
	int rowHeight = 32; 
	
	if(limit > 0)
	{
		listWidget->setFixedHeight(static_cast<int>(limit * rowHeight));
	}

	for(auto & f : txt_files)
	{
		// 直接提取文件名显示
		QString full_path = QString::fromStdString(f);
		QString file_name = QFileInfo(full_path).fileName();

		auto * item = new QListWidgetItem(file_name);

		// 存入全路径数据
		item->setData(Qt::UserRole, full_path);

		listWidget->addItem(item);
	}

}

// 答题卡
void MainWindow::handleToggleProgress()
{
	QGridLayout * layout = ui.gridLayout_CardButtons;

	clear_layout(layout);

	if(curr_questions_.empty()) return;

	int columns = 5; // 每行 5 个按钮
	int currentRow = 0;

	// 题型名称映射
	auto getTypeName = [](question_type t) -> QString {
		switch(t) {
			case question_type::single: return "单选题";
			case question_type::multi:  return "多选题";
			case question_type::judge:  return "判断题";
			case question_type::fill:   return "填空题";
			default: return "其他";
		}
	};

	// 按题型分组生成（题目已按题型排好序）
	question_type lastType = question_type::unknown;
	int colIndex = 0;

	for(size_t i = 0; i < curr_questions_.size(); ++i)
	{
		const question & q = curr_questions_[i];

		// 新题型：添加分组标题
		if(q.type != lastType)
		{
			// 换到新行
			if(colIndex > 0) {
				currentRow++;
				colIndex = 0;
			}

			// 创建题型标题
			QLabel * typeLabel = new QLabel(getTypeName(q.type));
			typeLabel->setStyleSheet(
				"font-size: 14px; font-weight: bold; color: #1976d2; "
				"padding: 8px 4px; margin-top: 8px;"
			);
			layout->addWidget(typeLabel, currentRow, 0, 1, columns); // 占满一行
			currentRow++;
			colIndex = 0;
			lastType = q.type;
		}

		// 创建题目按钮
		QPushButton * btn = new QPushButton(QString::number(i + 1));
		btn->setFixedSize(50, 50);

		// 根据状态设置颜色 (QSS)
		QString colorStyle;
		answer_state state = curr_results_[i];

		if(state == answer_state::correct)
		{
			colorStyle = "background-color: #81c784; color: white;"; // 绿
		}
		else if(state == answer_state::wrong)
		{
			colorStyle = "background-color: #e57373; color: white;"; // 红
		}
		else
		{
			colorStyle = "background-color: #e0e0e0; color: black;"; // 灰
		}

		if(i == static_cast<size_t>(curr_index_))
		{
			colorStyle += " border: 2px solid #2196f3;";
		}
		else
		{
			colorStyle += " border: none;";
		}

		btn->setStyleSheet(colorStyle);

		connect(btn, &QPushButton::clicked, [this, i]()
			{
				ui.stackedWidget->setCurrentWidget(ui.page_Quiz);
				show_question(static_cast<int>(i));
			});

		layout->addWidget(btn, currentRow, colIndex);
		colIndex++;
		if(colIndex >= columns)
		{
			colIndex = 0;
			currentRow++;
		}
	}

	ui.stackedWidget->setCurrentWidget(ui.page_Card);
}

// 上、下一题
void MainWindow::handlePrevQuestion()
{
	if(curr_index_ <= 0)
	{
		return;
	}

	show_question(--curr_index_);
}
void MainWindow::handleNextQuestion()
{
	if(curr_index_ >= curr_questions_.size() - 1)
	{
		// 结算
		auto reply = QMessageBox::question(this, "提交试卷", 
			"已经是最后一题了，确认提交并查看成绩吗？",
			QMessageBox::Yes | QMessageBox::No);

		if(reply == QMessageBox::Yes)
		{
			finish_exam();
		}
		return;
	}


	show_question(++curr_index_);
}

// 提交答案
void MainWindow::handleSubmitAnswer()
{
	if(curr_questions_.empty()) return;

	const question & q = curr_questions_[curr_index_];
	QString userAnswer = "";

	// 1. 根据题型去布局里找控件
	if(q.type == question_type::single || q.type == question_type::judge || q.type == question_type::multi)
	{
		QStringList selectedList;

		// 遍历 layout_Options 的所有选项容器
		for(int i = 0; i < ui.layout_Options->count(); ++i)
		{
			QLayoutItem * item = ui.layout_Options->itemAt(i);
			QWidget * container = item->widget();
			if(container && container->property("isChecked").toBool())
			{
				QString optKey = container->property("optionKey").toString();
				if(!optKey.isEmpty())
				{
					selectedList.append(optKey);
				}
			}
		}

		if(q.type == question_type::multi)
		{
			selectedList.sort();
			userAnswer = selectedList.join("");
		}
		else if(!selectedList.isEmpty())
		{
			userAnswer = selectedList.first();
		}
	}
	else if(q.type == question_type::fill)
	{
		// 通过遍历 layout 找到编辑器
		for(int i = 0; i < ui.layout_Options->count(); ++i)
		{
			QLayoutItem * item = ui.layout_Options->itemAt(i);
			if(item && item->widget())
			{
				auto edit = qobject_cast<QPlainTextEdit*>(item->widget());
				if(edit)
				{
					userAnswer = edit->toPlainText().trimmed();
					break;
				}
			}
		}
	}
	
	// 0. 保存用户答案 (State Preservation)
	if(curr_results_.size() > static_cast<size_t>(curr_index_))
	{
		user_answers_[curr_index_] = userAnswer;
	}

	// 2. 判分逻辑
	QString correctAns = to_QString(q.correct_answer).trimmed().toUpper();
	bool isCorrect = (userAnswer.trimmed().toUpper() == correctAns);

	if(isCorrect)
	{
		curr_results_[curr_index_] = answer_state::correct;

		// 答对后自动跳转下一题（如果启用且不是最后一题）
		if(storage.config().auto_next && static_cast<size_t>(curr_index_) < curr_questions_.size() - 1)
		{
			QTimer::singleShot(500, this, [this]() {
				show_question(++curr_index_);
			});
		}
	}
	else
	{
		curr_results_[curr_index_] = answer_state::wrong;

		storage.add_mistake(q);
	}

	// 高亮选项：正确变绿，错误变红
	if(q.type == question_type::single || q.type == question_type::judge || q.type == question_type::multi)
	{
		QString correctAnsUpper = to_QString(q.correct_answer).trimmed().toUpper();
		for(int i = 0; i < ui.layout_Options->count(); ++i)
		{
			QLayoutItem * item = ui.layout_Options->itemAt(i);
			QWidget * container = item->widget();
			if(!container || container->objectName() != "optionContainer") continue;

			container->setEnabled(false); // 禁止再次点击

			QString optKey = container->property("optionKey").toString();
			bool isUserSelected = container->property("isChecked").toBool();
			bool isCorrectOption = correctAnsUpper.contains(optKey);

			if(isCorrectOption)
			{
				// 正确选项：绿色
				container->setStyleSheet(
					"QWidget#optionContainer { background-color: #c8e6c9; border: 2px solid #4caf50; border-radius: 8px; }"
					"QLabel { color: #2e7d32; background: transparent; }"
				);
			}
			else if(isUserSelected)
			{
				// 用户选错的选项：红色
				container->setStyleSheet(
					"QWidget#optionContainer { background-color: #ffcdd2; border: 2px solid #f44336; border-radius: 8px; }"
					"QLabel { color: #c62828; background: transparent; }"
				);
			}
		}
	}
	else if(q.type == question_type::fill)
	{
		// 填空题：禁止再次编辑
		QPlainTextEdit* edit = nullptr;
		for(int i = 0; i < ui.layout_Options->count(); ++i)
		{
			QLayoutItem * item = ui.layout_Options->itemAt(i);
			if(item && item->widget())
			{
				edit = qobject_cast<QPlainTextEdit*>(item->widget());
				if(edit) break;
			}
		}
		
		if(edit)
		{
			edit->setReadOnly(true);
			const auto & cfg = storage.config();
			
			if(isCorrect)
			{
				// 正确：绿色边框、背景和文字
				edit->setStyleSheet(QString(
					"QPlainTextEdit { font-size: %1px; color: #2e7d32; font-weight: bold; border: 3px solid #4caf50; background-color: #c8e6c9; border-radius: 6px; padding: 8px; }"
				).arg(cfg.button_size));
			}
			else
			{
				// 错误：红色边框、背景和文字
				edit->setStyleSheet(QString(
					"QPlainTextEdit { font-size: %1px; color: #c62828; font-weight: bold; border: 3px solid #f44336; background-color: #ffcdd2; border-radius: 6px; padding: 8px; }"
				).arg(cfg.button_size));
				
				// 显示正确答案
				QLabel* lblCorrectAns = new QLabel();
				lblCorrectAns->setObjectName("lbl_CorrectAnswer");
				lblCorrectAns->setText(QString("%1").arg(correctAns));
				lblCorrectAns->setStyleSheet(QString(
					"font-size: %1px; color: #2e7d32; font-weight: bold; padding: 12px; background-color: #c8e6c9; border: 2px solid #4caf50; border-radius: 6px;"
				).arg(cfg.button_size));
				lblCorrectAns->setWordWrap(true);
				lblCorrectAns->setAlignment(Qt::AlignLeft | Qt::AlignTop);
				ui.layout_Options->addWidget(lblCorrectAns);
			}
		}
	}

	ui.btnSubmitAnswer->setEnabled(false); // 禁止重复提交
	
	// 反馈颜色
	if(isCorrect)
	{
		ui.btnSubmitAnswer->setStyleSheet("background-color: #4CAF50; color: white; border: none; border-radius: 4px; padding: 6px;");
	}
	else
	{
		ui.btnSubmitAnswer->setStyleSheet("background-color: #F44336; color: white; border: none; border-radius: 4px; padding: 6px;");
	}
	
	// 立即更新错误次数显示
	int errorCount = storage.get_mistake_count(q);
	ui.lbl_QuizInfoRight->setText(QString("错误: %1").arg(errorCount));
	if(errorCount > 0) {
		ui.lbl_QuizInfoRight->setStyleSheet("color: #D32F2F; font-weight: bold;");
	} else {
		ui.lbl_QuizInfoRight->setStyleSheet("color: gray;");
	}
}

// 退出练习
void MainWindow::handleExitQuiz()
{
	// 考试模式下强制确认，或者根据设置确认
	if(is_exam_mode_ || storage.config().confirm_exit)
	{
		QString title = is_exam_mode_ ? "退出考试" : "退出练习";
		QString msg = is_exam_mode_ 
			? "确定要退出考试吗？当前进度和成绩将不会保存。" 
			: "确定要退出练习吗？进度将不会保存。";

		auto reply = QMessageBox::question(this, title, msg, QMessageBox::Yes | QMessageBox::No);
		if(reply == QMessageBox::No) return;
	}

	if(is_exam_mode_)
	{
		is_exam_mode_ = false;
		if(exam_timer_) exam_timer_->stop();
	}

	ui.stackedWidget->setCurrentWidget(ui.page_Home);
}

// 考试历史记录
void MainWindow::handleOpenHistory()
{
	auto repo = homePage_->comboRepo()->currentText().toStdString();
	auto records = storage.get_history(repo);
	
	historyPage_->loadHistory(records);
	
	ui.stackedWidget->setCurrentWidget(ui.page_History);
}

// 考试配置
void MainWindow::handleOpenExamConfig()
{
    // 刷题库
    if(!init_start()) return;

    int c_single=0, c_multi=0, c_judge=0, c_fill=0;
    for(const auto & q : curr_questions_)
    {
        if(q.type == question_type::single) c_single++;
        else if(q.type == question_type::multi) c_multi++;
        else if(q.type == question_type::judge) c_judge++;
        else if(q.type == question_type::fill) c_fill++;
    }

    std::vector<int> counts = { c_single, c_multi, c_judge, c_fill };
    
    examConfigPage_->refreshConfigList();
    examConfigPage_->updateAvailableCounts(counts);

    ui.stackedWidget->setCurrentWidget(ui.page_ExamConfig);
}