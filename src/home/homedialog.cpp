#include "homedialog.h"
#include "ui_homedialog.h"

#include <QStandardItemModel>
#include <QTimer>
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QLayoutItem>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QList>
#include <QMap>
#include <QScrollArea>
#include "assemblydialog.h"
#include "stepshowform.h"
#include "fileprocessing.h"
#include "database_manager.h"
#include <QFileInfo>
#include <QDir>
#include <QScreen>
#include <QScroller>
#include <QInputDialog> // 顶部加入，用于弹出输入框

HomeDialog::HomeDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::HomeDialog)
    , m_selectedDeviceType("")
    , m_scrollAreaWidget(nullptr)
    , m_tutorialLayout(nullptr)
{
    ui->setupUi(this);
    InitUI();
}

HomeDialog::~HomeDialog()
{
    // 清理所有教程控件
    for (auto it = m_deviceTutorials.begin(); it != m_deviceTutorials.end(); ++it) {
        for (StepShowForm* form : it.value()) {
            if (form) {
                form->deleteLater();
            }
        }
    }
    delete ui;
}

void HomeDialog::InitUI()
{
    QStandardItemModel *model = new QStandardItemModel(0, 1, this);
    model->setHorizontalHeaderLabels(QStringList() << tr("操作名称"));
    ui->tableView_step->setModel(model);

    ui->tableView_step->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView_step->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView_step->verticalHeader()->setVisible(false);

    QHeaderView *header = ui->tableView_step->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Stretch);

    model->setHeaderData(0, Qt::Horizontal, Qt::AlignCenter, Qt::TextAlignmentRole);
    int rowHeight = 40; // 可以根据需要调整行高
    ui->tableView_step->verticalHeader()->setDefaultSectionSize(rowHeight);

    ui->tableView_step->setAlternatingRowColors(true);

    // 【修改点 2】：动态注入左侧的“目录控制面板”（由于没有改 .ui 文件，我们在代码里实现注入）
    QLayout* frameLayout = ui->frame_step->layout();
    if (QHBoxLayout* hLayout = qobject_cast<QHBoxLayout*>(frameLayout)) {
        hLayout->removeWidget(ui->tableView_step); // 先把表格抠出来

        QWidget* leftWidget = new QWidget(ui->frame_step);
        QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
        leftLayout->setContentsMargins(0, 0, 0, 0);
        leftLayout->setSpacing(6);

        // 创建控制面板
        m_categoryControlWidget = new QWidget(leftWidget);
        QHBoxLayout* catLayout = new QHBoxLayout(m_categoryControlWidget);
        catLayout->setContentsMargins(0, 0, 0, 0);
        m_btnBackCategory = new QPushButton("⬅ 返回", m_categoryControlWidget);
        m_btnAddSub = new QPushButton("➕ 新增", m_categoryControlWidget);
        m_btnDelSub = new QPushButton("✖ 删除", m_categoryControlWidget);

        m_btnBackCategory->setStyleSheet("background:#909399;color:white;border-radius:4px;padding:4px 8px;font-weight:bold;");
        m_btnAddSub->setStyleSheet("background:#67C23A;color:white;border-radius:4px;padding:4px 8px;font-weight:bold;");
        m_btnDelSub->setStyleSheet("background:#F56C6C;color:white;border-radius:4px;padding:4px 8px;font-weight:bold;");

        catLayout->addWidget(m_btnBackCategory);
        catLayout->addWidget(m_btnAddSub);
        catLayout->addWidget(m_btnDelSub);

        leftLayout->addWidget(m_categoryControlWidget);
        leftLayout->addWidget(ui->tableView_step); // 把表格放回去

        hLayout->insertWidget(0, leftWidget, 1); // 重新塞入原布局，拉伸系数给 1

        m_categoryControlWidget->hide(); // 默认主目录时隐藏
        // 绑定按钮信号
        connect(m_btnBackCategory, &QPushButton::clicked, this, &HomeDialog::on_btnBackCategory_clicked);
        connect(m_btnAddSub, &QPushButton::clicked, this, &HomeDialog::on_btnAddSub_clicked);
        connect(m_btnDelSub, &QPushButton::clicked, this, &HomeDialog::on_btnDelSub_clicked);
    }

    // 调用新的加载函数加载主目录
    loadMainCategories();

    // 连接选择变化信号
    connect(ui->tableView_step->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &HomeDialog::onDeviceTypeSelected);


    // 创建滚动区域的widget和布局
    m_scrollAreaWidget = new QWidget();
    m_tutorialLayout = new QVBoxLayout(m_scrollAreaWidget);
    m_tutorialLayout->setAlignment(Qt::AlignTop);
    m_tutorialLayout->setContentsMargins(10, 10, 10, 10);
    m_tutorialLayout->setSpacing(10);
    // ====== 顶部操作按钮（动态创建）======

    m_topButtonLayout = new QHBoxLayout();

    m_btnSelectAll = new QPushButton("全选");
    m_btnUnselectAll = new QPushButton("取消选择");
    m_btnDeleteSelected = new QPushButton("批量删除");
    m_btnSave = new QPushButton(tr("保存"));

    // 样式（加分）
    m_btnDeleteSelected->setStyleSheet("background:red;color:white;border-radius:6px;");
    m_btnSelectAll->setStyleSheet("background:#409EFF;color:white;border-radius:6px;");
    m_btnUnselectAll->setStyleSheet("background:#909399;color:white;border-radius:6px;");
    m_btnSave->setStyleSheet("background:black;color:white;border-radius:6px;");

    // 加入布局
    m_topButtonLayout->addWidget(m_btnSelectAll);
    m_topButtonLayout->addWidget(m_btnUnselectAll);
    m_topButtonLayout->addWidget(m_btnDeleteSelected);
    m_topButtonLayout->addWidget(m_btnSave);
    m_topButtonLayout->addStretch();

    // 插入到最上面
    m_tutorialLayout->addLayout(m_topButtonLayout);

    // ====== 绑定信号 ======
    connect(m_btnSelectAll, &QPushButton::clicked,
            this, &HomeDialog::on_btnSelectAll_clicked);

    connect(m_btnUnselectAll, &QPushButton::clicked,
            this, &HomeDialog::on_btnUnselectAll_clicked);

    connect(m_btnDeleteSelected, &QPushButton::clicked,
            this, &HomeDialog::on_btnDeleteSelected_clicked);

    connect(m_btnSave, &QPushButton::clicked,
            this, &HomeDialog::on_btnSave_clicked);

    // 设置滚动区域的widget - 使用正确的名称 scrollAreaStep
    ui->scrollAreaStep->setWidget(m_scrollAreaWidget);
    ui->scrollAreaStep->setWidgetResizable(true);

    // 初始隐藏管理员按钮
    ui->btnUserManager->setVisible(false);
    ui->btnImport->setVisible(false);
    // 开启触控屏手指滑动支持 (iPad 体验)
    QScroller::grabGesture(ui->tableView_step->viewport(), QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->scrollAreaStep->viewport(), QScroller::LeftMouseButtonGesture);

    loadMainCategories();
}


// 核心：实现按钮显隐
void HomeDialog::setUserManagerBtnVisible(bool visible)
{
    ui->btnUserManager->setVisible(visible);
    ui->btnImport->setVisible(visible);

    // 重建布局
    QTimer::singleShot(0, this, &HomeDialog::rebuildButtonLayout);
}

// 新增：完整的权限控制方法
void HomeDialog::setUserPermissions(int roleType)
{
    bool isAdmin = (roleType == 0);

    // 管理员专属按钮
    ui->btnUserManager->setVisible(isAdmin);
    ui->btnImport->setVisible(isAdmin);

    // 普通用户和管理员共用按钮
    ui->btnPersonalCenter->setVisible(true);
    ui->btnAssembly->setVisible(true);
    ui->btnStorageRack->setVisible(true);
    ui->btnLogout->setVisible(true);

    if (isAdmin) {
        // 获取horizontalLayout布局对象
        QHBoxLayout* hLayout = qobject_cast<QHBoxLayout*>(ui->horizontalLayout);
        if (hLayout) {
            hLayout->setSpacing(40); // 设置按钮间距为50
        }
    }
    // 重建布局
    // QTimer::singleShot(0, this, &HomeDialog::rebuildButtonLayout);
}

void HomeDialog::analysisStep(QMap<int, QStringList> stepPath, QString deviceTypeName, QString rackdata)
{
    auto setStep = [this](QString step, QString image)->StepShowForm*{
        StepShowForm *stepForm = new StepShowForm(m_scrollAreaWidget);
        stepForm->setStepValue(step);
        stepForm->setImageValue(image);
        return stepForm;
    };

    QString step = tr("%1 - 读取失败").arg(deviceTypeName);
    QString image = ":/image/imageError.png";
    // 开始遍历导入的文件夹数据

    auto setStepList = [this, stepPath, setStep, step, image, deviceTypeName, rackdata](QString itemname = QString()){
        for (QMap<int, QStringList>::const_iterator it = stepPath.constBegin();
             it != stepPath.constEnd(); ++it)
        {
            const QStringList& value = it.value();

            QString stepName = value.at(0);
            QString imagePath = value.at(1); // 这是第一张图（封面图）

            if(!itemname.isEmpty() && (stepName != itemname)){
                continue;
            }

            // 获取当前步骤所在的文件夹路径
            QString folderPath = QFileInfo(imagePath).absolutePath();
            QDir dir(folderPath);

            // --- 1. 查找视频（修改后逻辑，支持双视频分发） ---
            QStringList videoFilters;
            videoFilters << "*.mp4" << "*.avi" << "*.mkv";
            // 使用 QDir::Name 强制按文件名排序，保证 1_xx.mp4 在前，2_xx.mp4 在后
            QFileInfoList videoList = dir.entryInfoList(videoFilters, QDir::Files, QDir::Name);

            QString videoPath = "";
            QString projectorVideoPath = ""; // 新增投影仪专用视频路径

            if (videoList.size() > 0) {
                videoPath = videoList.at(0).absoluteFilePath(); // 第1个视频分配给主屏幕
            }
            if (videoList.size() > 1) {
                projectorVideoPath = videoList.at(1).absoluteFilePath(); // 第2个视频分配给投影仪
            }

            // --- 2. 查找第二张图片用于投影（新增逻辑） ---
            QStringList imageFilters;
            imageFilters << "*.jpg" << "*.png" << "*.jpeg" << "*.bmp";
            // 按名称排序获取文件夹下所有图片
            QFileInfoList allImages = dir.entryInfoList(imageFilters, QDir::Files, QDir::Name);

            QString projectorImagePath;
            for (const QFileInfo& info : std::as_const(allImages)) {
                // 如果这张图片的路径和封面图(imagePath)不一样，它就是我们要找的第二张图
                if (info.absoluteFilePath() != imagePath) {
                    projectorImagePath = info.absoluteFilePath();
                    break; // 找到第一张不一样的就退出循环
                }
            }

            // --- 3. 查找 SOP 说明文本 (新增逻辑) ---
            QStringList txtFilters;
            txtFilters << "*.txt";
            QFileInfoList txtList = dir.entryInfoList(txtFilters, QDir::Files);
            QString sopText = "";
            if (!txtList.isEmpty()) {
                QFile file(txtList.first().absoluteFilePath());
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&file);
                    // 兼容Qt5和Qt6的UTF-8读取，防止中文乱码
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                    in.setEncoding(QStringConverter::Utf8);
#else
                    in.setCodec("UTF-8");
#endif
                    sopText = in.readAll();
                    file.close();
                }
            }

            // 创建卡片
            StepShowForm* stepForm = setStep(
                stepName.isEmpty() ? step : QString("%1:%2").arg(deviceTypeName, stepName),
                imagePath.isEmpty() ? image : imagePath
                );

            // 设置视频路径
            stepForm->setVideoPath(videoPath);
            // ⭐ 把第二个视频的路径也存进卡片
            stepForm->setProjectorVideoPath(projectorVideoPath);

            // ⭐ 设置投影图片路径（确保你已经在 StepShowForm 类中添加了 setProjectorImagePath 方法）
            stepForm->setProjectorImagePath(projectorImagePath);

            // ⭐ 把读到的文本塞进卡片里！
            stepForm->setSopText(sopText);

            // 设置货物信息
            stepForm->setRackDataValue(rackdata);

            qDebug() << "步骤:" << stepName;
            qDebug() << "封面路径:" << imagePath;
            qDebug() << "视频路径:" << videoPath;
            qDebug() << "投影路径:" << projectorImagePath; // 如果为空说明文件夹里只有一张图

            // 删除功能连接
                connect(stepForm, &StepShowForm::signalDelete, this, [this, deviceTypeName](StepShowForm *form) {
                // 1. 停止界面刷新
                m_scrollAreaWidget->setUpdatesEnabled(false);

                // 2. 数据结构清理
                m_deviceTutorials[deviceTypeName].removeOne(form);
                m_tutorialLayout->removeWidget(form);

                form->setParent(nullptr);
                form->hide();
                form->deleteLater();

                // 3. 恢复界面刷新
                m_scrollAreaWidget->setUpdatesEnabled(true);

                // 4. 【关键】：使用单次定时器异步触发保存，避免在信号槽嵌套中执行重负载 IO
                QTimer::singleShot(100, this, [this]() {
                    this->on_btnSave_clicked();
                });
             });

            // 上移功能连接
            connect(stepForm, &StepShowForm::signalMoveUp, this,
                    [this, deviceTypeName](StepShowForm* form)
                    {
                        int index = m_tutorialLayout->indexOf(form);
                        if (index <= 1) return;
                        m_tutorialLayout->removeWidget(form);
                        m_tutorialLayout->insertWidget(index - 1, form);
                        int dataIndex = index - 1;
                        m_deviceTutorials[deviceTypeName].move(dataIndex, dataIndex - 1);
                    });

            // 下移功能连接
            connect(stepForm, &StepShowForm::signalMoveDown, this,
                    [this, deviceTypeName](StepShowForm* form)
                    {
                        int index = m_tutorialLayout->indexOf(form);
                        int count = m_tutorialLayout->count();
                        if (index >= count - 1) return;
                        m_tutorialLayout->removeWidget(form);
                        m_tutorialLayout->insertWidget(index + 1, form);
                        int dataIndex = index - 1;
                        m_deviceTutorials[deviceTypeName].move(dataIndex, dataIndex + 1);
                    });

            // 存储到数据结构
            m_deviceTutorials[deviceTypeName].append(stepForm);
        }
    };

    // 如果有货架信息，按照数据存的顺序展示。
    if(!rackdata.isEmpty()){
        const QStringList records = rackdata.split("^", Qt::SkipEmptyParts);
        for (const QString& record : records) {
            const QStringList fields = record.split("|");
            if (!fields.isEmpty()) {
                setStepList(fields.first());
            }
        }
    }
    else{
        setStepList();
    }



    // 显示当前设备类型的教程
    displayDeviceTutorials(deviceTypeName);
}

// 重建按钮布局 - 关键方法（居中等间距）
void HomeDialog::rebuildButtonLayout()
{
    // 获取底部按钮区域的父控件
    QWidget *buttonParent = ui->btnPersonalCenter->parentWidget();
    QLayout *oldLayout = buttonParent->layout();

    // 收集当前可见的按钮
    QList<QPushButton*> visibleButtons;

    if (ui->btnPersonalCenter->isVisible())
        visibleButtons << ui->btnPersonalCenter;
    if (ui->btnUserManager->isVisible())
        visibleButtons << ui->btnUserManager;
    if (ui->btnImport->isVisible())
        visibleButtons << ui->btnImport;
    if (ui->btnAssembly->isVisible())
        visibleButtons << ui->btnAssembly;
    if (ui->btnStorageRack->isVisible())
        visibleButtons << ui->btnStorageRack;
    if (ui->btnLogout->isVisible())
        visibleButtons << ui->btnLogout;

    // 创建一个新的居中容器
    QHBoxLayout *newLayout = new QHBoxLayout();
    newLayout->setSpacing(50);  // 设置按钮间的间距
    newLayout->setContentsMargins(0, 0, 0, 0);  // 设置边距

    // 创建一个居中容器来包装按钮
    QWidget *centerContainer = new QWidget(buttonParent);
    centerContainer->setObjectName("centerButtonContainer");  // 设置对象名便于调试

    QHBoxLayout *centerLayout = new QHBoxLayout(centerContainer);
    centerLayout->setSpacing(10);
    centerLayout->setContentsMargins(0, 0, 0, 0);

    // 添加可见按钮
    for (QPushButton *btn : visibleButtons) {
        centerLayout->addWidget(btn);
    }

    // 将居中容器添加到主布局中，使用stretch使其居中
    newLayout->addStretch();  // 左侧弹性空间
    newLayout->addWidget(centerContainer);  // 中间按钮容器
    newLayout->addStretch();  // 右侧弹性空间

    // 替换旧布局
    if (oldLayout) {
        delete oldLayout;
    }

    buttonParent->setLayout(newLayout);

    // 强制重新布局
    buttonParent->updateGeometry();
    buttonParent->adjustSize();

    // 更新整个对话框
    updateGeometry();
    adjustSize();

    // 确保界面立即刷新
    QApplication::processEvents();
}

void HomeDialog::on_btnPersonalCenter_clicked()
{
    emit signalOpenPersonalCenter();
}

void HomeDialog::on_btnUserManager_clicked()
{
    emit signalOpenUserManager();
}

void HomeDialog::on_btnLogout_clicked()
{
    emit signalLogout();
}

void HomeDialog::on_btnImport_clicked()
{
    if (m_selectedDeviceType.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择一个设备类型！");
        return;
    }

    QString rootPath = FileProcessing::GetFilePath();

    if (rootPath.isEmpty()) return;

    m_rootPath[m_selectedDeviceType] = rootPath;

    QString deviceTypeName = m_selectedDeviceType;
    QMap<int, QStringList> stepPath = FileProcessing::GetStepPath();

    // 导入教程判断（覆盖或追加）
    if (m_deviceTutorials.contains(deviceTypeName) &&
        !m_deviceTutorials[deviceTypeName].isEmpty())
    {
        QMessageBox::StandardButton reply =
            QMessageBox::question(this, "导入方式",
                                  "已有教程，是否覆盖？\n选择“否”则追加",
                                  QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            // 覆盖
            for (StepShowForm* form : m_deviceTutorials[deviceTypeName]) {
                if (form) {
                    m_tutorialLayout->removeWidget(form);
                    form->setParent(nullptr); // 【终极防闪退】
                    form->hide();
                    form->deleteLater();
                }
            }
            m_deviceTutorials[deviceTypeName].clear();
        }
    }

    analysisStep(stepPath, deviceTypeName);
}

void HomeDialog::on_btnAssembly_clicked()
{
    // 1. 记录当前主界面正在显示的设备类型，防止后台加载时导致界面画面乱跳
    QString currentVisibleDevice = m_selectedDeviceType;

    // 2. 遍历左侧列表中的所有设备类型，检查是否有还没加载进内存的数据
    QAbstractItemModel *model = ui->tableView_step->model();
    if (model) {
        for (int i = 0; i < model->rowCount(); ++i) {
            QString deviceName = model->data(model->index(i, 0)).toString();

            // 如果缓存字典中没有该设备，说明还没被点击加载过
            if (!m_deviceTutorials.contains(deviceName)) {
                StepInfo info;
                info.stepname = deviceName;

                // 从数据库查询，如果存在有效路径，则立刻在后台加载它
                if (DatabaseManager::instance().queryStepInfo(info) && !info.steppath.isEmpty()) {
                    int key = 0;
                    FileProcessing::StepPathClear();
                    FileProcessing::FindLeafFolders(info.steppath, key);
                    QMap<int, QStringList> stepPath = FileProcessing::GetStepPath();

                    // 借用现有的 analysisStep 加载数据并生成控件卡片
                    analysisStep(stepPath, deviceName, info.rackdata);
                    m_rootPath[deviceName] = info.steppath; // 记录根路径
                }
            }
        }
    }

    // 3. 恢复主界面原本应该显示的设备画面（隐藏掉刚才后台悄悄加载的那些设备控件）
    displayDeviceTutorials(currentVisibleDevice);

    // 4. 此时 m_deviceTutorials 已经装满了所有你导入过的教程，放心地传给组装界面
    emit signalJumpAssembly(m_deviceTutorials);
}

void HomeDialog::on_btnStorageRack_clicked()
{
    // 货架信息功能实现
    // 这里可以打开货架信息界面
    emit signalJumpShelf();
}

void HomeDialog::onDeviceTypeSelected()
{
    QModelIndexList selectedIndexes = ui->tableView_step->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty()) return;

    int selectedIndex = selectedIndexes.first().row();
    QAbstractItemModel *model = ui->tableView_step->model();
    QString selectedText = model->data(model->index(selectedIndex, 0)).toString();

    // 状态机判断：当前点的是主目录还是子目录？
    if (!m_isInSubCategoryView) {
        // [阶段 1] 点击了主目录：进入子目录列表
        loadSubCategories(selectedText);
        ui->tableView_step->clearSelection();
    } else {
        // [阶段 2] 点击了子目录：组合唯一标识加载右侧右侧数据
        // 使用 "主目录_子目录" 作为唯一 Key 存入数据库，例如："PCR安装_N1"
        m_selectedDeviceType = m_currentMainCategory + "_" + selectedText;
        qDebug() << "进入具体任务，设备标识为:" << m_selectedDeviceType;

        if(!m_deviceTutorials.contains(m_selectedDeviceType)){
            StepInfo info;
            info.stepname = m_selectedDeviceType;

            if (DatabaseManager::instance().queryStepInfo(info) && !info.steppath.isEmpty()) {
                int key = 0;
                FileProcessing::StepPathClear();
                FileProcessing::FindLeafFolders(info.steppath, key);
                QMap<int, QStringList> stepPath = FileProcessing::GetStepPath();
                analysisStep(stepPath, m_selectedDeviceType, info.rackdata);
                m_rootPath[m_selectedDeviceType] = info.steppath;
            } else {
                displayDeviceTutorials(m_selectedDeviceType);
            }
        } else {
            displayDeviceTutorials(m_selectedDeviceType);
        }
    }
}


void HomeDialog::loadMainCategories()
{
    m_isInSubCategoryView = false;
    m_currentMainCategory = "";

    // 1. 安全隐藏控制面板
    if (m_categoryControlWidget) {
        m_categoryControlWidget->hide();
    }

    // 2. 安全获取并清空列表
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->tableView_step->model());
    if (model) {
        model->removeRows(0, model->rowCount());
        QStringList items = {"PCR安装", "大型一体机安装", "桌面一体机安装", "手持设备安装", "其他设备安装"};
        for(int i = 0; i < items.count(); i++) {
            QStandardItem *idItem = new QStandardItem(items.at(i));
            idItem->setTextAlignment(Qt::AlignCenter);
            model->setItem(i, idItem);
        }
    }

    // 3. 调用我们优化后的清空函数
    clearTutorialDisplay();
}

void HomeDialog::loadSubCategories(const QString& mainCategory)
{
    m_isInSubCategoryView = true;
    m_currentMainCategory = mainCategory;

    // 显示控制面板。并且根据权限判断是否显示新增和删除按钮
    m_categoryControlWidget->show();
    bool isAdmin = ui->btnUserManager->isVisible(); // 借助旧逻辑判断权限
    m_btnAddSub->setVisible(isAdmin);
    m_btnDelSub->setVisible(isAdmin);

    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->tableView_step->model());
    model->removeRows(0, model->rowCount());

    QStringList subCats;
    DatabaseManager::instance().GetSubCategories(mainCategory, subCats);

    for(int i = 0; i < subCats.count(); i++) {
        QStandardItem *idItem = new QStandardItem(subCats.at(i));
        idItem->setTextAlignment(Qt::AlignCenter);
        model->setItem(i, idItem);
    }
    clearTutorialDisplay(); // 清空右侧画面
}

void HomeDialog::on_btnBackCategory_clicked()
{
    loadMainCategories(); // 回退到主目录
}

void HomeDialog::on_btnAddSub_clicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, "新增子目录", QString("正在为【%1】新增型号/编号：").arg(m_currentMainCategory), QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        DatabaseManager::instance().AddSubCategory(m_currentMainCategory, text);
        loadSubCategories(m_currentMainCategory); // 刷新
    }
}

void HomeDialog::on_btnDelSub_clicked()
{
    QModelIndexList selectedIndexes = ui->tableView_step->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先在列表中选择要删除的子目录！");
        return;
    }
    QString selectedText = ui->tableView_step->model()->data(selectedIndexes.first()).toString();

    if (QMessageBox::question(this, "确认", QString("确定要永久删除子目录【%1】吗？这会使其绑定的教程不可见！").arg(selectedText)) == QMessageBox::Yes) {
        DatabaseManager::instance().DelSubCategory(m_currentMainCategory, selectedText);
        loadSubCategories(m_currentMainCategory); // 刷新
    }
}

void HomeDialog::clearTutorialDisplay()
{
    // 安全检查
    if (!m_tutorialLayout) return;

    // 从索引 1 开始删除（保留索引 0 的按钮栏）
    while (m_tutorialLayout->count() > 1)
    {
        QLayoutItem *item = m_tutorialLayout->takeAt(1);
        if (QWidget *widget = item->widget()) {
            widget->hide();
        }
        delete item; // 仅仅删除布局项包装，保留实际的 widget 卡片
    }
}

void HomeDialog::displayDeviceTutorials(const QString& deviceType)
{
    // 清空当前显示的教程
    clearTutorialDisplay();

    // 如果该设备类型有教程，则显示
    if (m_deviceTutorials.contains(deviceType)) {
        for (StepShowForm* form : m_deviceTutorials[deviceType]) {
            if (form) {
                form->show();
                m_tutorialLayout->addWidget(form);
            }
        }
    }
}

void HomeDialog::on_btnDeleteSelected_clicked()
{
    if (m_selectedDeviceType.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先在左侧选择设备！");
        return;
    }

    if (!m_deviceTutorials.contains(m_selectedDeviceType) || m_deviceTutorials[m_selectedDeviceType].isEmpty()) {
        return;
    }

    if (QMessageBox::question(this, "确认", "确定删除所有选中的步骤？") != QMessageBox::Yes)
        return;

    // 1. 【性能优化】暂时阻塞当前界面的重绘，防止删除过程中 UI 引擎疯狂计算导致卡死
    m_scrollAreaWidget->setUpdatesEnabled(false);

    QString device = m_selectedDeviceType;
    QList<StepShowForm*> &list = m_deviceTutorials[device];
    bool hasDeleted = false;

    // 2. 【安全删除】倒序遍历
    for (int i = list.size() - 1; i >= 0; --i) {
        StepShowForm* form = list.at(i);
        if (form && form->isChecked()) {
            // 从布局中彻底移除
            m_tutorialLayout->removeWidget(form);
            form->setParent(nullptr); // 断开父子关系，确保不在渲染树中
            form->hide();

            list.removeAt(i);
            form->deleteLater();
            hasDeleted = true;
        }
    }

    // 3. 【恢复重绘】
    m_scrollAreaWidget->setUpdatesEnabled(true);
    m_scrollAreaWidget->update();

    // 4. 【数据库同步】只有在确实删除了东西时，才统一调用一次保存
    if (hasDeleted) {
        // 使用单次定时器触发保存，给 Qt 一个呼吸的机会去处理 deleteLater，防止逻辑冲突
        QTimer::singleShot(100, this, [this](){
            this->on_btnSave_clicked();
        });
    }
}

void HomeDialog::on_btnSelectAll_clicked()
{
    for (auto form : m_deviceTutorials[m_selectedDeviceType])
        form->setChecked(true);
}
void HomeDialog::on_btnUnselectAll_clicked()
{
    for (auto form : m_deviceTutorials[m_selectedDeviceType])
        form->setChecked(false);
}

void HomeDialog::on_btnSave_clicked()
{
    if(m_deviceTutorials.isEmpty()) return;

    QStringList rackdataList;
    bool ret = true;
    DatabaseManager& db = DatabaseManager::instance();
    db.transaction();

    foreach (QString var, m_deviceTutorials.keys()) {
        rackdataList.clear();
        foreach (StepShowForm* stepItem, m_deviceTutorials[var]) {
            QStringList stepData;
            // 1. 放入步骤名称
            stepData << stepItem->getStepText().split(":").last();

            // 2. 循环提取该步骤下【所有】货位信息
            QVector<QSharedPointer<RackCFGForm>> racks = stepItem->getRackForm();
            foreach (QSharedPointer<RackCFGForm> rackItem, racks) {
                // 依次存入 位置 和 数量
                stepData << rackItem->getRackNumber();
                stepData << rackItem->getQuantityGoods();
            }

            // 结果格式示例： "步骤一|A-1-1|2|A-1-2|3"
            rackdataList << stepData.join("|");
        }

        StepInfo info;
        info.stepname = var;
        info.steppath = rackdataList.isEmpty() ? "" : m_rootPath.value(var, "");
        // 不同步骤之间用 ^ 分隔
        info.rackdata = rackdataList.join("^");

        qDebug() << ">>> 正在保存" << var << "的多零件配置:" << info.rackdata;
        // 🚩 【核心修复】：判断数据库中是否已经有这条记录
        StepInfo checkInfo;
        checkInfo.stepname = var;
        if (db.queryStepInfo(checkInfo)) {
            // 如果存在，更新数据
            ret = db.UpdateStepInfo(info);
        } else {
            // 如果不存在（比如刚建的子目录），必须新增数据！
            ret = db.AddStepInfo(info);
        }

        if(!ret) break;
    }

    if (ret) {
        db.commit();
        QMessageBox::information(this, "保存成功", "多零件配置已同步至数据库！");
    } else {
        db.rollback();
        QMessageBox::critical(this, "错误", "数据库写入失败！");
    }
}

void HomeDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
}