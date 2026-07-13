#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDir>
#include <QFileInfo>
#include <QStyle>
#include <QProcess>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QTemporaryDir>
#include <QPixmap>
#include <QIcon>
#include <QLineEdit>
#include <QClipboard>
#include <QMimeData>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QSettings>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDesktopServices>
#include <QDateTime>
#include <QMimeDatabase>
#include <QMimeType>
#include <QRegularExpression>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QFileInfoList>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QStandardPaths>

class Parrot : public QWidget {
public:
    Parrot() {
        currentPath = QDir::homePath();
        historyIndex = -1;
        isDarkMode = false;
        viewMode = 0;
        setupUi();
        loadSettings();
        applyTheme();
        connectSignals();
        refresh();
        setAcceptDrops(true);

        // Fade in on startup
        QTimer::singleShot(0, [this]() { fadeIn(this); });
    }

    void openPath(const QString &path) {
        if (QDir(path).exists()) navigate(path);
    }

protected:
    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_F5) refresh();
        else if (e->key() == Qt::Key_F6) { pathBar->setFocus(); pathBar->selectAll(); }
        else if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_N) newFolder();
        else if (e->key() == Qt::Key_Backspace) goUp();
        else if (e->key() == Qt::Key_Delete) deleteSelected();
        else if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_C) copySelected();
        else if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_V) pasteItems();
        else QWidget::keyPressEvent(e);
    }

    void dragEnterEvent(QDragEnterEvent *e) override {
        if (e->mimeData()->hasUrls()) e->acceptProposedAction();
    }

    void handleDrop(const QPoint &pos, const QList<QUrl> &urls, QListWidget *targetList) {
        QListWidgetItem *targetItem = targetList->itemAt(pos);

        if (targetItem) {
            QString targetPath = targetItem->data(Qt::UserRole).toString();
            bool targetIsDir = targetItem->data(Qt::UserRole + 1).toBool();

            if (targetIsDir) {
                for (const QUrl &url : urls) {
                    QString src = url.toLocalFile();
                    QString dst = targetPath + "/" + QFileInfo(src).fileName();
                    if (QFileInfo(src).isDir()) {
                        moveDir(src, dst);
                    } else {
                        QFile::remove(dst);
                        QFile::rename(src, dst);
                    }
                }
                QWidget *w = (viewMode == 0) ? (QWidget*)list : (QWidget*)table;
                pulseWidget(w, isDarkMode ? QColor(217, 70, 239, 120) : QColor(168, 85, 247, 100));
            } else {
                for (const QUrl &url : urls) {
                    QString droppedPath = url.toLocalFile();
                    runFileCommand(droppedPath);
                }
            }
        } else {
            for (const QUrl &url : urls) {
                QString src = url.toLocalFile();
                QString dst = currentPath + "/" + QFileInfo(src).fileName();
                if (QFileInfo(src).isDir()) {
                    moveDir(src, dst);
                } else {
                    QFile::remove(dst);
                    QFile::rename(src, dst);
                }
            }
        }
        refresh();
    }

    void handleDrop(const QPoint &pos, const QList<QUrl> &urls, QTableWidget *targetTable) {
        QTableWidgetItem *targetItem = targetTable->itemAt(pos);

        if (targetItem) {
            QString targetPath = targetItem->data(Qt::UserRole).toString();
            bool targetIsDir = targetItem->data(Qt::UserRole + 1).toBool();

            if (targetIsDir) {
                for (const QUrl &url : urls) {
                    QString src = url.toLocalFile();
                    QString dst = targetPath + "/" + QFileInfo(src).fileName();
                    if (QFileInfo(src).isDir()) {
                        moveDir(src, dst);
                    } else {
                        QFile::remove(dst);
                        QFile::rename(src, dst);
                    }
                }
                QWidget *w = (viewMode == 0) ? (QWidget*)list : (QWidget*)table;
                pulseWidget(w, isDarkMode ? QColor(217, 70, 239, 120) : QColor(168, 85, 247, 100));
            } else {
                for (const QUrl &url : urls) {
                    QString droppedPath = url.toLocalFile();
                    runFileCommand(droppedPath);
                }
            }
        } else {
            for (const QUrl &url : urls) {
                QString src = url.toLocalFile();
                QString dst = currentPath + "/" + QFileInfo(src).fileName();
                if (QFileInfo(src).isDir()) {
                    moveDir(src, dst);
                } else {
                    QFile::remove(dst);
                    QFile::rename(src, dst);
                }
            }
        }
        refresh();
    }

    bool eventFilter(QObject *obj, QEvent *event) override {
        QPushButton *btn = qobject_cast<QPushButton*>(obj);
        if (btn && btn->objectName() == "navBtn") {
            if (event->type() == QEvent::Enter) {
                auto *shadow = new QGraphicsDropShadowEffect(btn);
                shadow->setBlurRadius(12);
                shadow->setOffset(0);
                shadow->setColor(isDarkMode ? QColor(217, 70, 239, 100) : QColor(168, 85, 247, 80));
                btn->setGraphicsEffect(shadow);
            } else if (event->type() == QEvent::Leave) {
                btn->setGraphicsEffect(nullptr);
            } else if (event->type() == QEvent::MouseButtonPress) {
                bouncePress(btn);
            }
        }
        if (btn && btn->objectName() == "accentBtn") {
            if (event->type() == QEvent::MouseButtonPress) {
                bouncePress(btn);
            }
        }
        if (btn && btn->objectName() == "themeBtn") {
            if (event->type() == QEvent::MouseButtonPress) {
                bouncePress(btn);
            }
        }

        // Sidebar tree hover glow
        QTreeWidget *treeW = qobject_cast<QTreeWidget*>(obj);
        if (treeW && treeW->objectName() == "sidebarTree") {
            if (event->type() == QEvent::Enter) {
                auto *shadow = new QGraphicsDropShadowEffect(treeW);
                shadow->setBlurRadius(15);
                shadow->setOffset(0);
                shadow->setColor(isDarkMode ? QColor(217, 70, 239, 60) : QColor(168, 85, 247, 50));
                treeW->setGraphicsEffect(shadow);
            } else if (event->type() == QEvent::Leave) {
                treeW->setGraphicsEffect(nullptr);
            }
        }

        // Search box expand on focus
        QLineEdit *lineEdit = qobject_cast<QLineEdit*>(obj);
        if (lineEdit && lineEdit->objectName() == "searchBox") {
            if (event->type() == QEvent::FocusIn) {
                auto *anim = new QPropertyAnimation(lineEdit, "maximumWidth", this);
                anim->setDuration(200);
                anim->setStartValue(lineEdit->maximumWidth());
                anim->setEndValue(350);
                anim->setEasingCurve(QEasingCurve::OutCubic);
                anim->start(QAbstractAnimation::DeleteWhenStopped);
            } else if (event->type() == QEvent::FocusOut) {
                auto *anim = new QPropertyAnimation(lineEdit, "maximumWidth", this);
                anim->setDuration(200);
                anim->setStartValue(lineEdit->maximumWidth());
                anim->setEndValue(200);
                anim->setEasingCurve(QEasingCurve::InCubic);
                anim->start(QAbstractAnimation::DeleteWhenStopped);
            }
        }

        // Path bar glow on focus
        if (lineEdit && lineEdit->objectName() == "pathBar") {
            if (event->type() == QEvent::FocusIn) {
                pulseWidget(lineEdit, isDarkMode ? QColor(217, 70, 239, 80) : QColor(168, 85, 247, 60), 400);
            }
        }

        // Handle drops on list widget
        QListWidget *listW = qobject_cast<QListWidget*>(obj);
        if (listW && listW->objectName() == "fileList") {
            if (event->type() == QEvent::DragEnter) {
                auto *de = static_cast<QDragEnterEvent*>(event);
                if (de->mimeData()->hasUrls()) {
                    de->acceptProposedAction();
                    animateDragHighlight(true);
                }
                return true;
            } else if (event->type() == QEvent::DragMove) {
                auto *de = static_cast<QDragMoveEvent*>(event);
                if (de->mimeData()->hasUrls()) {
                    de->acceptProposedAction();
                }
                return true;
            } else if (event->type() == QEvent::Drop) {
                auto *de = static_cast<QDropEvent*>(event);
                if (de->mimeData()->hasUrls()) {
                    handleDrop(de->position().toPoint(), de->mimeData()->urls(), listW);
                    animateDragHighlight(false);
                }
                return true;
            } else if (event->type() == QEvent::DragLeave) {
                animateDragHighlight(false);
            }
        }

        // Handle drops on table widget
        QTableWidget *tableW = qobject_cast<QTableWidget*>(obj);
        if (tableW && tableW->objectName() == "fileTable") {
            if (event->type() == QEvent::DragEnter) {
                auto *de = static_cast<QDragEnterEvent*>(event);
                if (de->mimeData()->hasUrls()) {
                    de->acceptProposedAction();
                    animateDragHighlight(true);
                }
                return true;
            } else if (event->type() == QEvent::DragMove) {
                auto *de = static_cast<QDragMoveEvent*>(event);
                if (de->mimeData()->hasUrls()) {
                    de->acceptProposedAction();
                }
                return true;
            } else if (event->type() == QEvent::Drop) {
                auto *de = static_cast<QDropEvent*>(event);
                if (de->mimeData()->hasUrls()) {
                    handleDrop(de->position().toPoint(), de->mimeData()->urls(), tableW);
                    animateDragHighlight(false);
                }
                return true;
            } else if (event->type() == QEvent::DragLeave) {
                animateDragHighlight(false);
            }
        }

        return QWidget::eventFilter(obj, event);
    }

private:
    QString currentPath;
    bool isDarkMode;
    int viewMode;
    int historyIndex;
    QList<QString> history;

    QPushButton *backBtn, *forwardBtn, *upBtn;
    QLineEdit *pathBar;
    QListWidget *list;
    QTableWidget *table;
    QComboBox *sortBox, *viewBox;
    QLabel *statusLabel;
    QSettings settings{"Parrot", "FileManager"};

    // Animation helpers
    QList<QPropertyAnimation*> activeAnims;

    void cleanAnim(QPropertyAnimation *a) {
        if (a && a->state() == QAbstractAnimation::Running) {
            activeAnims.removeOne(a);
            a->stop();
            a->deleteLater();
        }
    }

    void fadeIn(QWidget *w, int duration = 200) {
        auto *effect = new QGraphicsOpacityEffect(w);
        w->setGraphicsEffect(effect);
        auto *anim = new QPropertyAnimation(effect, "opacity", this);
        anim->setDuration(duration);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(anim, &QPropertyAnimation::finished, [w]() { w->setGraphicsEffect(nullptr); });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void fadeOut(QWidget *w, std::function<void()> onDone = nullptr, int duration = 150) {
        auto *effect = new QGraphicsOpacityEffect(w);
        w->setGraphicsEffect(effect);
        auto *anim = new QPropertyAnimation(effect, "opacity", this);
        anim->setDuration(duration);
        anim->setStartValue(1.0);
        anim->setEndValue(0.0);
        anim->setEasingCurve(QEasingCurve::InCubic);
        connect(anim, &QPropertyAnimation::finished, [w, onDone]() {
            w->setGraphicsEffect(nullptr);
            if (onDone) onDone();
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void bouncePress(QWidget *w) {
        QRect original = w->geometry();
        int dx = 1, dy = 1;
        QRect pressed = original.adjusted(dx, dy, -dx, -dy);
        auto *anim = new QPropertyAnimation(w, "geometry", this);
        anim->setDuration(80);
        anim->setKeyValueAt(0.0, original);
        anim->setKeyValueAt(0.5, pressed);
        anim->setKeyValueAt(1.0, original);
        anim->setEasingCurve(QEasingCurve::InOutSine);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void slideContent(int direction) {
        // direction: -1 = slide left (forward), 1 = slide right (back)
        QWidget *w = (viewMode == 0) ? (QWidget*)list : (QWidget*)table;
        QRect endRect = w->geometry();
        QRect startRect = endRect;
        startRect.moveLeft(endRect.left() + direction * endRect.width() * 0.15);

        w->setGeometry(startRect);
        auto *effect = new QGraphicsOpacityEffect(w);
        w->setGraphicsEffect(effect);
        effect->setOpacity(0.3);

        auto *slideAnim = new QPropertyAnimation(w, "geometry", this);
        slideAnim->setDuration(250);
        slideAnim->setStartValue(startRect);
        slideAnim->setEndValue(endRect);
        slideAnim->setEasingCurve(QEasingCurve::OutCubic);

        auto *fadeAnim = new QPropertyAnimation(effect, "opacity", this);
        fadeAnim->setDuration(250);
        fadeAnim->setStartValue(0.3);
        fadeAnim->setEndValue(1.0);
        fadeAnim->setEasingCurve(QEasingCurve::OutCubic);

        auto *group = new QParallelAnimationGroup(this);
        group->addAnimation(slideAnim);
        group->addAnimation(fadeAnim);
        connect(group, &QParallelAnimationGroup::finished, [w]() { w->setGraphicsEffect(nullptr); });
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void animateDialog(QDialog *dlg) {
        QRect finalRect = dlg->geometry();
        QRect startRect = finalRect;
        startRect.setWidth(finalRect.width() * 0.85);
        startRect.setHeight(finalRect.height() * 0.85);
        startRect.moveCenter(finalRect.center());

        dlg->setGeometry(startRect);
        auto *effect = new QGraphicsOpacityEffect(dlg);
        dlg->setGraphicsEffect(effect);
        effect->setOpacity(0.0);

        auto *sizeAnim = new QPropertyAnimation(dlg, "geometry", this);
        sizeAnim->setDuration(200);
        sizeAnim->setStartValue(startRect);
        sizeAnim->setEndValue(finalRect);
        sizeAnim->setEasingCurve(QEasingCurve::OutCubic);

        auto *fadeAnim = new QPropertyAnimation(effect, "opacity", this);
        fadeAnim->setDuration(200);
        fadeAnim->setStartValue(0.0);
        fadeAnim->setEndValue(1.0);
        fadeAnim->setEasingCurve(QEasingCurve::OutCubic);

        auto *group = new QParallelAnimationGroup(this);
        group->addAnimation(sizeAnim);
        group->addAnimation(fadeAnim);
        connect(group, &QParallelAnimationGroup::finished, [dlg]() { dlg->setGraphicsEffect(nullptr); });
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void pulseWidget(QWidget *w, QColor color, int duration = 300) {
        auto *effect = new QGraphicsDropShadowEffect(w);
        effect->setBlurRadius(0);
        effect->setOffset(0);
        effect->setColor(color);
        w->setGraphicsEffect(effect);

        auto *anim = new QPropertyAnimation(effect, "blurRadius", this);
        anim->setDuration(duration);
        anim->setKeyValueAt(0.0, 0);
        anim->setKeyValueAt(0.5, 15);
        anim->setKeyValueAt(1.0, 0);
        anim->setEasingCurve(QEasingCurve::InOutSine);
        connect(anim, &QPropertyAnimation::finished, [w]() { w->setGraphicsEffect(nullptr); });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void animateRefresh() {
        QWidget *w = (viewMode == 0) ? (QWidget*)list : (QWidget*)table;
        fadeIn(w, 180);
    }

    void animateDragHighlight(bool show) {
        QWidget *w = (viewMode == 0) ? (QWidget*)list : (QWidget*)table;
        if (show) {
            auto *effect = new QGraphicsDropShadowEffect(w);
            effect->setBlurRadius(20);
            effect->setOffset(0);
            effect->setColor(isDarkMode ? QColor(217, 70, 239, 150) : QColor(168, 85, 247, 120));
            w->setGraphicsEffect(effect);
        } else {
            w->setGraphicsEffect(nullptr);
        }
    }

    void animateStatusChange(const QString &newText) {
        fadeOut(statusLabel, [this, newText]() {
            statusLabel->setText(newText);
            fadeIn(statusLabel, 100);
        }, 100);
    }

    void setupUi() {
        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        // Top bar
        auto *topBar = new QWidget();
        topBar->setObjectName("topBar");
        auto *topLayout = new QHBoxLayout(topBar);
        topLayout->setContentsMargins(10, 6, 10, 6);
        topLayout->setSpacing(6);

        backBtn = new QPushButton("<");
        backBtn->setObjectName("navBtn");
        backBtn->setToolTip("Back");
        backBtn->setFixedSize(34, 34);
        backBtn->installEventFilter(this);

        forwardBtn = new QPushButton(">");
        forwardBtn->setObjectName("navBtn");
        forwardBtn->setToolTip("Forward");
        forwardBtn->setFixedSize(34, 34);
        forwardBtn->installEventFilter(this);

        upBtn = new QPushButton("^");
        upBtn->setObjectName("navBtn");
        upBtn->setToolTip("Up");
        upBtn->setFixedSize(34, 34);
        upBtn->installEventFilter(this);

        QPushButton *homeBtn = new QPushButton("Home");
        homeBtn->setObjectName("navBtn");
        homeBtn->setToolTip("Home");
        homeBtn->setFixedSize(50, 34);
        homeBtn->installEventFilter(this);

        QPushButton *refreshBtn = new QPushButton("R");
        refreshBtn->setObjectName("navBtn");
        refreshBtn->setToolTip("Refresh");
        refreshBtn->setFixedSize(34, 34);
        refreshBtn->installEventFilter(this);

        pathBar = new QLineEdit(currentPath);
        pathBar->setObjectName("pathBar");
        pathBar->installEventFilter(this);

        QPushButton *themeBtn = new QPushButton(isDarkMode ? "Sun" : "Moon");
        themeBtn->setObjectName("themeBtn");
        themeBtn->setFixedSize(50, 34);
        themeBtn->installEventFilter(this);

        topLayout->addWidget(backBtn);
        topLayout->addWidget(forwardBtn);
        topLayout->addWidget(upBtn);
        topLayout->addWidget(homeBtn);
        topLayout->addWidget(pathBar, 1);
        topLayout->addWidget(refreshBtn);
        topLayout->addWidget(themeBtn);

        root->addWidget(topBar);

        // Toolbar
        auto *toolbar = new QWidget();
        toolbar->setObjectName("toolbar");
        auto *toolLayout = new QHBoxLayout(toolbar);
        toolLayout->setContentsMargins(10, 4, 10, 4);
        toolLayout->setSpacing(6);

        sortBox = new QComboBox();
        sortBox->addItems({"Name", "Size", "Date"});
        sortBox->setToolTip("Sort by");

        viewBox = new QComboBox();
        viewBox->addItems({"Icons", "Details"});
        viewBox->setToolTip("View mode");

        QPushButton *newBtn = new QPushButton("+ Folder");
        newBtn->setObjectName("accentBtn");
        newBtn->installEventFilter(this);

        toolLayout->addWidget(newBtn);
        toolLayout->addWidget(new QLabel("Sort:"));
        toolLayout->addWidget(sortBox);
        toolLayout->addWidget(new QLabel("View:"));
        toolLayout->addWidget(viewBox);
        toolLayout->addStretch();

        QLineEdit *search = new QLineEdit();
        search->setPlaceholderText("Search...");
        search->setObjectName("searchBox");
        search->setMaximumWidth(200);

        // Search box expand animation
        auto *searchExpandAnim = new QPropertyAnimation(search, "maximumWidth", this);
        search->installEventFilter(this);

        toolLayout->addWidget(search);

        root->addWidget(toolbar);

        // Content
        list = new QListWidget();
        list->setObjectName("fileList");
        list->setViewMode(QListWidget::IconMode);
        list->setIconSize(QSize(56, 56));
        list->setGridSize(QSize(100, 90));
        list->setSpacing(4);
        list->setResizeMode(QListWidget::Adjust);
        list->setContextMenuPolicy(Qt::CustomContextMenu);
        list->setSelectionMode(QAbstractItemView::ExtendedSelection);
        list->setAcceptDrops(true);
        list->setDragEnabled(true);
        list->setDropIndicatorShown(true);
        list->installEventFilter(this);

        table = new QTableWidget();
        table->setObjectName("fileTable");
        table->setColumnCount(3);
        table->setHorizontalHeaderLabels({"Name", "Size", "Modified"});
        table->horizontalHeader()->setStretchLastSection(true);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::ExtendedSelection);
        table->setContextMenuPolicy(Qt::CustomContextMenu);
        table->verticalHeader()->hide();
        table->setShowGrid(false);
        table->setVisible(false);
        table->setAcceptDrops(true);
        table->setDragEnabled(true);
        table->setDropIndicatorShown(true);
        table->installEventFilter(this);

        auto *splitter = new QSplitter(Qt::Horizontal);
        splitter->setObjectName("splitter");

        // Sidebar
        auto *sidebar = new QWidget();
        sidebar->setObjectName("sidebar");
        auto *sbLayout = new QVBoxLayout(sidebar);
        sbLayout->setContentsMargins(0, 0, 0, 0);
        sbLayout->setSpacing(0);

        auto *placesGroup = new QGroupBox("PLACES");
        placesGroup->setObjectName("sidebarGroup");
        auto *plLayout = new QVBoxLayout(placesGroup);
        plLayout->setContentsMargins(8, 18, 8, 6);

        auto *tree = new QTreeWidget();
        tree->setObjectName("sidebarTree");
        tree->setHeaderHidden(true);
        tree->setRootIsDecorated(false);
        tree->installEventFilter(this);

        auto addPlace = [&](const QString &name, const QString &path) {
            auto *item = new QTreeWidgetItem(tree);
            item->setText(0, name);
            item->setData(0, Qt::UserRole, path);
        };

        addPlace("Home", QDir::homePath());
        addPlace("Documents", QDir::homePath() + "/Documents");
        addPlace("Downloads", QDir::homePath() + "/Downloads");
        addPlace("Pictures", QDir::homePath() + "/Pictures");
        addPlace("Music", QDir::homePath() + "/Music");
        addPlace("Videos", QDir::homePath() + "/Videos");
        addPlace("Root", "/");

        plLayout->addWidget(tree);
        sbLayout->addWidget(placesGroup);
        sbLayout->addStretch();
        sidebar->setFixedWidth(180);

        splitter->addWidget(sidebar);
        auto *content = new QWidget();
        auto *cl = new QVBoxLayout(content);
        cl->setContentsMargins(0, 0, 0, 0);
        cl->addWidget(list);
        cl->addWidget(table);
        splitter->addWidget(content);
        splitter->setStretchFactor(0, 0);
        splitter->setStretchFactor(1, 1);

        root->addWidget(splitter, 1);

        // Status bar
        auto *statusBar = new QWidget();
        statusBar->setObjectName("statusBar");
        auto *stLayout = new QHBoxLayout(statusBar);
        stLayout->setContentsMargins(10, 4, 10, 4);
        statusLabel = new QLabel();
        statusLabel->setObjectName("statusLabel");
        stLayout->addWidget(statusLabel);
        stLayout->addStretch();
        root->addWidget(statusBar);

        // Window
        setWindowTitle("Parrot");
        resize(1100, 700);
        setMinimumSize(700, 450);

        // Connections
        connect(backBtn, &QPushButton::clicked, this, &Parrot::goBack);
        connect(forwardBtn, &QPushButton::clicked, this, &Parrot::goForward);
        connect(upBtn, &QPushButton::clicked, this, &Parrot::goUp);
        connect(homeBtn, &QPushButton::clicked, [this]() { navigate(QDir::homePath()); });
        connect(refreshBtn, &QPushButton::clicked, this, &Parrot::refresh);
        connect(pathBar, &QLineEdit::returnPressed, this, &Parrot::navToPath);
        connect(newBtn, &QPushButton::clicked, this, &Parrot::newFolder);
        connect(sortBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() { refresh(); });
        connect(viewBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int v) {
            QWidget *oldView = (viewMode == 0) ? (QWidget*)list : (QWidget*)table;
            fadeOut(oldView, [this, v, oldView]() {
                viewMode = v;
                table->setVisible(v == 1);
                list->setVisible(v == 0);
                QWidget *newView = (viewMode == 0) ? (QWidget*)list : (QWidget*)table;
                refresh();
                fadeIn(newView, 200);
            }, 120);
        });
        connect(search, &QLineEdit::textChanged, this, &Parrot::filter);
        connect(list, &QListWidget::itemDoubleClicked, this, &Parrot::onOpen);
        connect(list, &QListWidget::customContextMenuRequested, this, &Parrot::showMenu);
        connect(table, &QTableWidget::customContextMenuRequested, this, &Parrot::showMenu);
        connect(themeBtn, &QPushButton::clicked, [this]() {
            isDarkMode = !isDarkMode;
            applyTheme();
            settings.setValue("dark", isDarkMode);
            fadeIn(list);
            fadeIn(table);
        });
        connect(tree, &QTreeWidget::itemClicked, [this](QTreeWidgetItem *item, int) {
            QString p = item->data(0, Qt::UserRole).toString();
            if (QDir(p).exists()) navigate(p);
        });
    }

    void connectSignals() {}

    void navigate(const QString &path) {
        currentPath = path;
        if (historyIndex < history.size() - 1)
            history.erase(history.begin() + historyIndex + 1, history.end());
        history.append(path);
        historyIndex = history.size() - 1;
        refresh();
        backBtn->setEnabled(historyIndex > 0);
        forwardBtn->setEnabled(historyIndex < history.size() - 1);
    }

    void goBack() {
        if (historyIndex > 0) {
            historyIndex--;
            currentPath = history[historyIndex];
            slideContent(1); // slide right = back
            refresh();
        }
    }

    void goForward() {
        if (historyIndex < history.size() - 1) {
            historyIndex++;
            currentPath = history[historyIndex];
            slideContent(-1); // slide left = forward
            refresh();
        }
    }

    void goUp() {
        QDir d(currentPath);
        if (d.cdUp()) navigate(d.absolutePath());
    }

    void navToPath() {
        QString p = pathBar->text();
        if (QDir(p).exists()) navigate(p);
        else QMessageBox::warning(this, "Error", "Path not found");
    }

    static bool has(const QString &ext, const QStringList &list) { return list.contains(ext); }

    QString parseDesktopExec(const QString &desktopPath) {
        QFile file(desktopPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return "";
        while (!file.atEnd()) {
            QString line = QString::fromLocal8Bit(file.readLine()).trimmed();
            if (line.startsWith("Exec=")) {
                QString exec = line.mid(5);
                // Remove field codes like %f, %F, %u, %U, %d, %D, %n, %N, etc.
                exec.remove(QRegularExpression(" %[fFdDnNmuvw]"));
                return exec;
            }
        }
        return "";
    }

    QString parseDesktopName(const QString &desktopPath) {
        QFile file(desktopPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return QFileInfo(desktopPath).baseName();
        while (!file.atEnd()) {
            QString line = QString::fromLocal8Bit(file.readLine()).trimmed();
            if (line.startsWith("Name=") && !line.mid(5).isEmpty())
                return line.mid(5);
        }
        return QFileInfo(desktopPath).baseName();
    }

    QIcon fileIcon(const QFileInfo &f) {
        if (f.isDir()) {
            QIcon icon = QIcon::fromTheme("folder");
            if (!icon.isNull()) return icon;
            return style()->standardIcon(QStyle::SP_DirIcon);
        }

        QString ext = f.suffix().toLower();
        QIcon fallback = style()->standardIcon(QStyle::SP_FileIcon);

        if (f.fileName().endsWith(".desktop")) {
            // Try to get icon from the desktop file's Icon= line
            QFile file(f.absoluteFilePath());
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                while (!file.atEnd()) {
                    QString line = QString::fromLocal8Bit(file.readLine()).trimmed();
                    if (line.startsWith("Icon=")) {
                        QString iconName = line.mid(5);
                        QIcon icon = QIcon::fromTheme(iconName);
                        if (!icon.isNull()) return icon;
                        // Try with path
                        if (QFileInfo(iconName).exists()) {
                            icon = QIcon(iconName);
                            if (!icon.isNull()) return icon;
                        }
                        break;
                    }
                }
            }
            return QIcon::fromTheme("application-x-desktop", fallback);
        }

        if (f.fileName().endsWith(".app") || f.isExecutable())
            return QIcon::fromTheme("application-x-executable", fallback);

        if (has(ext, {"png","jpg","jpeg","gif","bmp","svg","webp","ico","tiff","tga","xpm","psd","ai","eps"}))
            return QIcon::fromTheme("image-x-generic", fallback);

        if (has(ext, {"mp4","mkv","avi","mov","wmv","flv","webm","m4v","mpg","mpeg","3gp","ogv","ts"}))
            return QIcon::fromTheme("video-x-generic", fallback);

        if (has(ext, {"mp3","wav","ogg","flac","aac","m4a","wma","opus","aiff","mid","midi"}))
            return QIcon::fromTheme("audio-x-generic", fallback);

        if (ext == "pdf")
            return QIcon::fromTheme("application-pdf", fallback);

        if (has(ext, {"zip","tar","gz","bz2","xz","7z","rar","deb","rpm","zst","lz4","lzma"}))
            return QIcon::fromTheme("application-x-archive", fallback);

        if (has(ext, {"doc","docx","odt","rtf","tex","wpd"}))
            return QIcon::fromTheme("x-office-document", fallback);
        if (has(ext, {"xls","xlsx","ods","csv","tsv"}))
            return QIcon::fromTheme("x-office-spreadsheet", fallback);
        if (has(ext, {"ppt","pptx","odp"}))
            return QIcon::fromTheme("x-office-presentation", fallback);

        if (has(ext, {"cpp","c","h","hpp","cc","cxx","hxx","hh"}))
            return QIcon::fromTheme("text-x-c++src", fallback);
        if (has(ext, {"py","pyw","pyi"}))
            return QIcon::fromTheme("text-x-python", fallback);
        if (has(ext, {"js","jsx","ts","tsx","mjs","cjs"}))
            return QIcon::fromTheme("text-x-script", fallback);
        if (has(ext, {"java","kt","kts"}))
            return QIcon::fromTheme("text-x-java", fallback);
        if (ext == "rs")
            return QIcon::fromTheme("text-x-rust", QIcon::fromTheme("text-x-script", fallback));
        if (ext == "go")
            return QIcon::fromTheme("text-x-go", fallback);
        if (has(ext, {"rb","erb"}))
            return QIcon::fromTheme("text-x-ruby", fallback);
        if (ext == "php")
            return QIcon::fromTheme("text-x-php", fallback);
        if (ext == "swift")
            return QIcon::fromTheme("text-x-swift", fallback);

        if (has(ext, {"html","htm","xhtml","xml"}))
            return QIcon::fromTheme("text-html", fallback);
        if (has(ext, {"css","scss","sass","less"}))
            return QIcon::fromTheme("text-x-css", fallback);

        if (has(ext, {"json","yaml","yml","toml","ini","cfg","conf","env","properties"}))
            return QIcon::fromTheme("text-x-generic", fallback);
        if (has(ext, {"md","markdown","rst","txt","log"}))
            return QIcon::fromTheme("text-x-generic", fallback);

        if (has(ext, {"sh","bash","zsh","fish","ps1","bat","cmd","awk","sed","lua","pl","pm"}))
            return QIcon::fromTheme("text-x-script", fallback);

        if (has(ext, {"sql","sqlite","db","sqlite3"}))
            return QIcon::fromTheme("office-database", fallback);

        if (has(ext, {"iso","img","vdi","vmdk","qcow2","raw","bin","rom"}))
            return QIcon::fromTheme("drive-removable-media", fallback);

        if (has(ext, {"ttf","otf","woff","woff2","eot"}))
            return QIcon::fromTheme("font-x-generic", fallback);

        if (has(ext, {"stl","obj","fbx","blend","3ds","dae","gltf","glb"}))
            return QIcon::fromTheme("model-x-generic", fallback);

        if (has(ext, {"iso","img","vdi","vmdk","qcow2","raw","bin","rom"}))
            return QIcon::fromTheme("drive-removable-media", fallback);

        if (has(ext, {"ttf","otf","woff","woff2","eot"}))
            return QIcon::fromTheme("font-x-generic", fallback);

        if (has(ext, {"stl","obj","fbx","blend","3ds","dae","gltf","glb"}))
            return QIcon::fromTheme("model-x-generic", fallback);

        // Try MIME-based icon as last resort
        QMimeType mt = QMimeDatabase().mimeTypeForFile(f);
        QIcon themeIcon = QIcon::fromTheme(mt.iconName());
        if (!themeIcon.isNull()) return themeIcon;

        themeIcon = QIcon::fromTheme(mt.genericIconName());
        if (!themeIcon.isNull()) return themeIcon;

        return style()->standardIcon(QStyle::SP_FileIcon);
    }

    void refresh() {
        pathBar->setText(currentPath);
        QDir dir(currentPath);
        QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

        QString sort = sortBox->currentText();
        if (sort == "Size")
            std::sort(entries.begin(), entries.end(), [](const QFileInfo &a, const QFileInfo &b) { return a.size() > b.size(); });
        else if (sort == "Date")
            std::sort(entries.begin(), entries.end(), [](const QFileInfo &a, const QFileInfo &b) { return a.lastModified() > b.lastModified(); });
        else
            std::sort(entries.begin(), entries.end(), [](const QFileInfo &a, const QFileInfo &b) { return a.fileName().toLower() < b.fileName().toLower(); });

        if (viewMode == 0) {
            list->clear();
            for (const QFileInfo &f : entries) {
                auto *item = new QListWidgetItem(f.fileName());
                item->setIcon(fileIcon(f));
                item->setData(Qt::UserRole, f.absoluteFilePath());
                item->setData(Qt::UserRole + 1, f.isDir());
                list->addItem(item);
            }
        } else {
            table->setSortingEnabled(false);
            table->setRowCount(0);
            table->setRowCount(entries.size());
            for (int i = 0; i < entries.size(); i++) {
                const QFileInfo &f = entries[i];
                auto *name = new QTableWidgetItem(f.fileName());
                name->setIcon(fileIcon(f));
                name->setData(Qt::UserRole, f.absoluteFilePath());
                name->setData(Qt::UserRole + 1, f.isDir());
                table->setItem(i, 0, name);
                table->setItem(i, 1, new QTableWidgetItem(f.isDir() ? "--" : fmtSize(f.size())));
                table->setItem(i, 2, new QTableWidgetItem(f.lastModified().toString("yyyy-MM-dd hh:mm")));
            }
            table->setSortingEnabled(true);
        }

        int dirs = 0, files = 0;
        for (const QFileInfo &f : entries) f.isDir() ? dirs++ : files++;
        animateStatusChange(QString("%1 items  |  %2 folders, %3 files").arg(entries.size()).arg(dirs).arg(files));
        animateRefresh();
    }

    static bool isScript(const QString &path) {
        return path.endsWith(".sh") || path.endsWith(".bash") || path.endsWith(".zsh") ||
               path.endsWith(".fish") || path.endsWith(".pl") || path.endsWith(".py") ||
               path.endsWith(".rb");
    }

    void openScriptWithChoice(const QString &path) {
        QDialog dlg(this);
        dlg.setWindowTitle("Open Script");
        dlg.setMinimumWidth(350);
        auto *layout = new QVBoxLayout(&dlg);

        QLabel *info = new QLabel("How would you like to open\n" + QFileInfo(path).fileName() + "?");
        layout->addWidget(info);

        auto *btnLayout = new QHBoxLayout();
        QPushButton *editBtn = new QPushButton("Edit");
        QPushButton *runBtn = new QPushButton("Run in Terminal");
        QPushButton *cancelBtn = new QPushButton("Cancel");
        btnLayout->addWidget(editBtn);
        btnLayout->addWidget(runBtn);
        btnLayout->addWidget(cancelBtn);
        layout->addLayout(btnLayout);

        connect(editBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
        connect(runBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
        connect(cancelBtn, &QPushButton::clicked, [&dlg]() { dlg.done(-1); });

        runBtn->setObjectName("accentBtn");

        animateDialog(&dlg);

        int result = dlg.exec();

        if (result == QDialog::Accepted) {
            // Edit
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        } else if (result == QDialog::Rejected) {
            // Run in terminal
            runInTerminal(path);
        }
        // -1 = cancel, do nothing
    }

    void runInTerminal(const QString &path) {
        // Try common terminals
        QStringList terminals = {
            "xdg-terminal", "gnome-terminal", "konsole", "kitty",
            "alacritty", "foot", "xterm", "lxterminal", "tilix"
        };

        for (const QString &term : terminals) {
            if (!QStandardPaths::findExecutable(term).isEmpty()) {
                if (term == "gnome-terminal") {
                    QProcess::startDetached(term, {"--", "bash", "-c", "cd \"" + QFileInfo(path).absolutePath() + "\" && bash \"" + path + "\"; echo; echo 'Press enter to close...'; read"});
                } else if (term == "konsole") {
                    QProcess::startDetached(term, {"-e", "bash", "-c", "cd \"" + QFileInfo(path).absolutePath() + "\" && bash \"" + path + "\"; echo; echo 'Press enter to close...'; read"});
                } else if (term == "kitty" || term == "alacritty") {
                    QProcess::startDetached(term, {"bash", "-c", "cd \"" + QFileInfo(path).absolutePath() + "\" && bash \"" + path + "\"; echo; echo 'Press enter to close...'; read"});
                } else {
                    QProcess::startDetached(term, {"-e", "bash", "-c", "cd \"" + QFileInfo(path).absolutePath() + "\" && bash \"" + path + "\"; echo; echo 'Press enter to close...'; read"});
                }
                return;
            }
        }
        QMessageBox::warning(this, "Error", "No terminal emulator found");
    }

    void onOpen(QListWidgetItem *item) {
        QString path = item->data(Qt::UserRole).toString();
        bool isDir = item->data(Qt::UserRole + 1).toBool();
        if (isDir) {
            navigate(path);
        } else if (path.endsWith(".desktop")) {
            QString exec = parseDesktopExec(path);
            if (!exec.isEmpty()) {
                QStringList parts = exec.split(' ', Qt::SkipEmptyParts);
                if (!parts.isEmpty()) {
                    QString cmd = parts.takeFirst();
                    QProcess::startDetached(cmd, parts);
                }
            } else {
                QMessageBox::warning(this, "Error", "No Exec= line found in this .desktop file");
            }
        } else if (path.endsWith(".app")) {
            QProcess::startDetached("pkgr", {"run", path});
        } else if (isScript(path)) {
            openScriptWithChoice(path);
        } else {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    }

    void filter(const QString &q) {
        for (int i = 0; i < list->count(); i++)
            list->item(i)->setHidden(!list->item(i)->text().toLower().contains(q.toLower()));
    }

    void newFolder() {
        bool ok;
        QString name = QInputDialog::getText(this, "New Folder", "Name:", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty()) {
            QDir(currentPath).mkdir(name);
            QWidget *w = (viewMode == 0) ? (QWidget*)list : (QWidget*)table;
            pulseWidget(w, isDarkMode ? QColor(217, 70, 239, 120) : QColor(168, 85, 247, 100));
            refresh();
        }
    }

    void showMenu(const QPoint &pos) {
        QListWidgetItem *item = list->itemAt(pos);
        QMenu menu;
        if (item) {
            QString path = item->data(Qt::UserRole).toString();
            menu.addAction("Open", [this, item]() { onOpen(item); });
            menu.addAction("Open With...", [this, item]() { openWithDialog(item->data(Qt::UserRole).toString()); });
            if (isScript(path)) {
                menu.addAction("Run in Terminal", [this, path]() { runInTerminal(path); });
            }
            if (path.endsWith(".desktop")) {
                menu.addAction("Edit Desktop File", [this, path]() {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
                });
            }
            menu.addSeparator();
            menu.addAction("Copy", this, &Parrot::copySelected);
            menu.addAction("Paste", this, &Parrot::pasteItems);
            menu.addSeparator();
            menu.addAction("Rename", [this, item]() {
                QString old = item->data(Qt::UserRole).toString();
                bool ok;
                QString n = QInputDialog::getText(this, "Rename", "Name:", QLineEdit::Normal, QFileInfo(old).fileName(), &ok);
                if (ok && !n.isEmpty()) { QFile::rename(old, QFileInfo(old).absolutePath() + "/" + n); refresh(); }
            });
            menu.addAction("Delete", this, &Parrot::deleteSelected);
            menu.addSeparator();
            menu.addAction("Properties", [this, item]() {
                QFileInfo info(item->data(Qt::UserRole).toString());
                QMessageBox::information(this, info.fileName(),
                    "Type: " + QString(info.isDir() ? "Folder" : "File") + "\n"
                    "Size: " + fmtSize(info.size()) + "\n"
                    "Modified: " + info.lastModified().toString("yyyy-MM-dd hh:mm"));
            });
        } else {
            menu.addAction("New Folder", this, &Parrot::newFolder);
            menu.addAction("Paste", this, &Parrot::pasteItems);
            menu.addAction("Refresh", this, &Parrot::refresh);
        }
        menu.exec(list->viewport()->mapToGlobal(pos));
    }

    struct AppInfo {
        QString name;
        QString exec;
        QString desktopFile;
    };

    QList<AppInfo> findAppsForFile(const QString &filePath) {
        QList<AppInfo> apps;
        QMimeType mt = QMimeDatabase().mimeTypeForFile(filePath);
        QString mime = mt.name();

        // Also check generic icon name and parent MIME types to find more apps
        QStringList mimeTypes = {mime};
        if (mt.isValid()) {
            QString parent = mt.parentMimeTypes().isEmpty() ? "" : mt.parentMimeTypes().first();
            if (!parent.isEmpty()) mimeTypes.append(parent);
        }

        // Scan .desktop files in standard locations
        QStringList dirs = {
            "/usr/share/applications",
            QDir::homePath() + "/.local/share/applications",
            "/usr/local/share/applications",
            "/var/lib/flatpak/exports/share/applications",
            QDir::homePath() + "/.local/share/flatpak/exports/share/applications"
        };

        // Also scan flatpak per-app dirs
        QDir flatpakDir("/var/lib/flatpak/exports/share/applications");
        if (flatpakDir.exists()) {
            for (const QString &sub : flatpakDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
                dirs.append(flatpakDir.absoluteFilePath(sub));
        }
        QDir homeFlatpak(QDir::homePath() + "/.local/share/flatpak/exports/share/applications");
        if (homeFlatpak.exists()) {
            for (const QString &sub : homeFlatpak.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
                dirs.append(homeFlatpak.absoluteFilePath(sub));
        }

        QSet<QString> seen;

        for (const QString &dirPath : dirs) {
            QDir dir(dirPath);
            if (!dir.exists()) continue;

            for (const QString &entry : dir.entryList({"*.desktop"}, QDir::Files)) {
                QString desktopPath = dir.absoluteFilePath(entry);
                QFile file(desktopPath);
                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

                QString name, exec, mimeTypeStr;
                bool noDisplay = false;
                while (!file.atEnd()) {
                    QString line = QString::fromLocal8Bit(file.readLine()).trimmed();
                    if (line.startsWith("Name=") && name.isEmpty())
                        name = line.mid(5);
                    else if (line.startsWith("Exec="))
                        exec = line.mid(5).section(' ', 0, 0);
                    else if (line.startsWith("NoDisplay=true"))
                        noDisplay = true;
                    else if (line.startsWith("MimeType="))
                        mimeTypeStr = line.mid(9);
                }
                file.close();

                if (noDisplay || name.isEmpty() || exec.isEmpty()) continue;
                if (seen.contains(exec)) continue;

                // Check if this app handles any of our MIME types
                bool handles = false;
                for (const QString &m : mimeTypes) {
                    if (mimeTypeStr.contains(m)) { handles = true; break; }
                }

                if (handles) {
                    seen.insert(exec);
                    apps.append({name, exec, desktopPath});
                }
            }
        }

        // Sort by name
        std::sort(apps.begin(), apps.end(), [](const AppInfo &a, const AppInfo &b) {
            return a.name.toLower() < b.name.toLower();
        });

        return apps;
    }

    void openWithDialog(const QString &filePath) {
        if (QFileInfo(filePath).isDir()) return;

        QList<AppInfo> apps = findAppsForFile(filePath);

        QDialog dialog(this);
        dialog.setWindowTitle("Open With");
        dialog.setMinimumWidth(400);
        dialog.setMinimumHeight(300);

        auto *layout = new QVBoxLayout(&dialog);

        QLabel *label = new QLabel("Choose an application to open:");
        layout->addWidget(label);

        QListWidget *appList = new QListWidget();
        appList->setObjectName("fileList");

        for (const AppInfo &app : apps) {
            auto *item = new QListWidgetItem(app.name);
            item->setData(Qt::UserRole, app.exec);
            item->setData(Qt::UserRole + 1, app.desktopFile);
            appList->addItem(item);
        }

        if (appList->count() == 0) {
            auto *item = new QListWidgetItem("(No applications found)");
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
            appList->addItem(item);
        }

        layout->addWidget(appList);

        auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Cancel);
        QPushButton *openBtn = btnBox->button(QDialogButtonBox::Open);
        openBtn->setText("Open");
        QPushButton *cancelBtn = btnBox->button(QDialogButtonBox::Cancel);
        cancelBtn->setText("Cancel");
        layout->addWidget(btnBox);

        connect(btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        animateDialog(&dialog);

        if (dialog.exec() == QDialog::Accepted) {
            QListWidgetItem *sel = appList->currentItem();
            if (sel && sel->data(Qt::UserRole).isValid()) {
                QString exec = sel->data(Qt::UserRole).toString();
                QProcess::startDetached(exec, {filePath});
            }
        }
    }

    void copySelected() {
        QList<QUrl> urls;
        for (auto *i : list->selectedItems())
            urls.append(QUrl::fromLocalFile(i->data(Qt::UserRole).toString()));
        auto *data = new QMimeData();
        data->setUrls(urls);
        QApplication::clipboard()->setMimeData(data);
    }

    void pasteItems() {
        const QMimeData *d = QApplication::clipboard()->mimeData();
        if (!d || !d->hasUrls()) return;
        for (const QUrl &u : d->urls()) {
            QString s = u.toLocalFile();
            QString dst = currentPath + "/" + QFileInfo(s).fileName();
            if (QFileInfo(s).isDir()) copyDir(s, dst);
            else QFile::copy(s, dst);
        }
        QWidget *w = (viewMode == 0) ? (QWidget*)list : (QWidget*)table;
        pulseWidget(w, isDarkMode ? QColor(217, 70, 239, 100) : QColor(168, 85, 247, 80));
        refresh();
    }

    bool copyDir(const QString &src, const QString &dst) {
        QDir().mkpath(dst);
        for (const QFileInfo &e : QDir(src).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot))
            e.isDir() ? copyDir(e.filePath(), dst + "/" + e.fileName()) : QFile::copy(e.filePath(), dst + "/" + e.fileName());
        return true;
    }

    void moveDir(const QString &src, const QString &dst) {
        QDir().mkpath(dst);
        for (const QFileInfo &e : QDir(src).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
            QString target = dst + "/" + e.fileName();
            if (e.isDir()) {
                moveDir(e.filePath(), target);
            } else {
                QFile::remove(target);
                QFile::rename(e.filePath(), target);
            }
        }
        QDir().rmdir(src);
    }

    void runFileCommand(const QString &path) {
        QStringList terminals = {
            "xdg-terminal", "gnome-terminal", "konsole", "kitty",
            "alacritty", "foot", "xterm", "lxterminal", "tilix"
        };
        QString fileCmd = "file \"" + path + "\"";

        for (const QString &term : terminals) {
            if (!QStandardPaths::findExecutable(term).isEmpty()) {
                if (term == "gnome-terminal") {
                    QProcess::startDetached(term, {"--", "bash", "-c", fileCmd + "; echo; echo 'Press enter to close...'; read"});
                } else if (term == "konsole") {
                    QProcess::startDetached(term, {"-e", "bash", "-c", fileCmd + "; echo; echo 'Press enter to close...'; read"});
                } else if (term == "kitty" || term == "alacritty") {
                    QProcess::startDetached(term, {"bash", "-c", fileCmd + "; echo; echo 'Press enter to close...'; read"});
                } else {
                    QProcess::startDetached(term, {"-e", "bash", "-c", fileCmd + "; echo; echo 'Press enter to close...'; read"});
                }
                return;
            }
        }
        QMessageBox::warning(this, "Error", "No terminal emulator found");
    }

    void deleteSelected() {
        auto items = list->selectedItems();
        if (items.isEmpty()) return;
        if (QMessageBox::question(this, "Delete", QString("Delete %1 item(s)?").arg(items.size())) == QMessageBox::Yes) {
            for (auto *i : items) {
                QString p = i->data(Qt::UserRole).toString();
                QFileInfo(p).isDir() ? QDir(p).removeRecursively() : QFile::remove(p);
            }
            QWidget *w = (viewMode == 0) ? (QWidget*)list : (QWidget*)table;
            pulseWidget(w, QColor(239, 68, 68, 100)); // red pulse for delete
            refresh();
        }
    }

    QString fmtSize(qint64 b) {
        if (b < 1024) return QString::number(b) + " B";
        if (b < 1048576) return QString::number(b / 1024.0, 'f', 1) + " KB";
        if (b < 1073741824) return QString::number(b / 1048576.0, 'f', 1) + " MB";
        return QString::number(b / 1073741824.0, 'f', 2) + " GB";
    }

    void loadSettings() {
        isDarkMode = settings.value("dark", false).toBool();
    }

    void applyTheme() {
        QString bg, surface, sidebar, hover, text, text2, border, accent, accent2;

        if (isDarkMode) {
            bg = "#111111";
            surface = "#1A1A1A";
            sidebar = "#151515";
            hover = "#2A2A2A";
            text = "#EEEEEE";
            text2 = "#888888";
            border = "#333333";
            accent = "#D946EF";
            accent2 = "#C026D3";
        } else {
            bg = "#F5F5F5";
            surface = "#FFFFFF";
            sidebar = "#F0F0F0";
            hover = "#EAEAEA";
            text = "#222222";
            text2 = "#777777";
            border = "#DDDDDD";
            accent = "#A855F7";
            accent2 = "#9333EA";
        }

        setStyleSheet(
            "QWidget { background:" + bg + "; color:" + text + "; font: 12px 'Segoe UI', 'Noto Sans', sans-serif; }"
            "QPushButton { background:" + accent + "; color:white; border:none; border-radius:8px; padding:8px 16px; font-weight:bold; }"
            "QPushButton:hover { background:" + accent2 + "; }"
            "#topBar { background:" + surface + "; border-bottom: 1px solid " + border + "; }"
            "#toolbar { background:" + surface + "; border-bottom: 1px solid " + border + "; }"
            "#sidebar { background:" + sidebar + "; border-right: 1px solid " + border + "; }"
            "#sidebarGroup { color:" + accent + "; font-weight:bold; font-size:11px; padding-top:12px; }"
            "#sidebarTree { background:" + sidebar + "; color:" + text + "; border:none; font-size:12px; }"
            "#sidebarTree::item { padding:6px 8px; border-radius:6px; margin:1px 4px; }"
            "#sidebarTree::item:hover { background:" + hover + "; }"
            "#sidebarTree::item:selected { background:" + accent + "; color:white; }"
            "#navBtn { background:" + surface + "; color:" + text + "; border:1px solid " + border + "; border-radius:8px; font-size:14px; font-weight:normal; padding:4px; }"
            "#navBtn:hover { background:" + hover + "; border-color:" + accent + "; color:" + accent + "; }"
            "#navBtn:pressed { background:" + accent + "; color:white; }"
            "#themeBtn { background:" + surface + "; color:" + accent + "; border:1px solid " + border + "; border-radius:8px; font-size:14px; font-weight:bold; padding:4px; }"
            "#themeBtn:hover { background:" + hover + "; }"
            "#pathBar { background:" + bg + "; color:" + text + "; border:1px solid " + border + "; border-radius:8px; padding:7px 12px; font-size:13px; }"
            "#pathBar:focus { border:2px solid " + accent + "; }"
            "#accentBtn { background:" + accent + "; color:white; border:none; border-radius:8px; padding:6px 14px; font-weight:bold; }"
            "#accentBtn:hover { background:" + accent2 + "; }"
            "QComboBox { background:" + bg + "; color:" + text + "; border:1px solid " + border + "; border-radius:6px; padding:4px 10px; min-width:70px; }"
            "QComboBox:hover { border-color:" + accent + "; }"
            "QComboBox QAbstractItemView { background:" + surface + "; color:" + text + "; border:1px solid " + border + "; selection-background-color:" + accent + "; selection-color:white; }"
            "#fileList { background:" + surface + "; border:none; }"
            "#fileList::item { border-radius:8px; padding:8px; }"
            "#fileList::item:hover { background:" + hover + "; }"
            "#fileList::item:selected { background:" + accent + "; color:white; }"
            "#fileTable { background:" + surface + "; border:none; }"
            "#fileTable::item:selected { background:" + accent + "; color:white; }"
            "#fileTable QHeaderView::section { background:" + sidebar + "; color:" + text2 + "; border:none; border-bottom:2px solid " + accent + "; padding:8px; font-weight:bold; }"
            "#statusBar { background:" + surface + "; border-top:1px solid " + border + "; }"
            "#statusLabel { color:" + text2 + "; }"
            "QMenu { background:" + surface + "; color:" + text + "; border:1px solid " + border + "; border-radius:8px; padding:4px; }"
            "QMenu::item { padding:8px 20px; border-radius:4px; margin:2px 4px; }"
            "QMenu::item:selected { background:" + accent + "; color:white; }"
            "QMenu::separator { height:1px; background:" + border + "; margin:4px 8px; }"
            "QCheckBox { spacing:6px; color:" + text + "; }"
            "QCheckBox::indicator { width:16px; height:16px; border-radius:4px; border:1px solid " + border + "; background:" + surface + "; }"
            "QCheckBox::indicator:checked { background:" + accent + "; border-color:" + accent + "; }"
            "QLineEdit { background:" + bg + "; color:" + text + "; border:1px solid " + border + "; border-radius:8px; padding:6px 10px; }"
            "QLineEdit:focus { border:2px solid " + accent + "; }"
            "QSplitter::handle { background:" + border + "; width:1px; }"
            "QScrollBar:vertical { background:" + bg + "; width:10px; }"
            "QScrollBar::handle:vertical { background:" + border + "; border-radius:5px; min-height:30px; }"
            "QScrollBar::handle:vertical:hover { background:" + accent + "; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }"
        );
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Parrot");
    Parrot w;
    if (argc > 1 && QDir(argv[1]).exists())
        w.openPath(argv[1]);
    w.show();
    return app.exec();
}
