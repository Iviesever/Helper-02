#include "HomePage.h"

#include <QScrollBar>
#include <QScroller>
#include <QMessageBox>
#include <QPushButton>

HomePage::HomePage(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    
    // 惯性滚动设置
    QScroller::grabGesture(ui.scrollArea_Home->viewport(), QScroller::LeftMouseButtonGesture);
    ui.scrollArea_Home->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui.scrollArea_Home->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui.scrollArea_Home->setWidgetResizable(true);
    ui.scrollArea_Home->setAlignment(Qt::AlignHCenter);
    ui.scrollArea_Home->horizontalScrollBar()->setRange(0, 0);
    
    // 列表多选模式
    ui.listWidgetFiles->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui.listWidgetFiles->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui.listWidgetFiles->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui.listWidgetFiles->setSelectionMode(QAbstractItemView::MultiSelection);
    
    // 连接内部信号
    connect(ui.btnToSettings, &QPushButton::clicked, this, &HomePage::openSettings);
    connect(ui.btnToHistory, &QPushButton::clicked, this, &HomePage::openHistory);
    connect(ui.btnToExamConfig, &QPushButton::clicked, this, &HomePage::openExamConfig);
    connect(ui.btnStartSeq, &QPushButton::clicked, this, &HomePage::startSequentialPractice);
    connect(ui.btnStartRand, &QPushButton::clicked, this, &HomePage::startRandomPractice);
    connect(ui.btnViewMode, &QPushButton::clicked, this, &HomePage::startViewMode);
    connect(ui.comboRepo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &HomePage::repoChanged);
    connect(ui.btnManageParser, &QPushButton::clicked, this, &HomePage::openParserStrategy);
    connect(ui.btnPracticeStrategy, &QPushButton::clicked, this, &HomePage::openPracticeStrategy);
    
    // 全选按钮
    connect(ui.btnSelectAll, &QPushButton::clicked, this, [this]() {
        bool isAllSelected = (ui.listWidgetFiles->selectedItems().count() == ui.listWidgetFiles->count());
        if(isAllSelected) {
            ui.listWidgetFiles->clearSelection();
        } else {
            ui.listWidgetFiles->selectAll();
        }
    });

    // 关于按钮
    QPushButton* btnAbout = new QPushButton("关于", this);
    btnAbout->setFlat(true);
    ui.verticalLayout_HomeContent->addWidget(btnAbout);

    connect(btnAbout, &QPushButton::clicked, this, [this]() {
        QMessageBox::about(this, "关于",
            "<h3>助手2号 (helper 02)</h3>"
            "<p>作者: Iviesever</p>"
            "<p>GitHub: <a href='https://github.com/Iviesever/Helper-02'>https://github.com/Iviesever/Helper-02</a></p>");
    });
}
