#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTreeView>
#include <QListView>
#include <QFileSystemModel>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <QProcess>
#include <QLineEdit>
#include <QHeaderView>
#include <QDateTime>
#include <QDesktopServices>
#include <QStyle>
#include <QStyleFactory>
#include <QFont>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QPalette>
#include <QScreen>
#include <QAbstractItemView>
#include <QFrame>
#include <QScrollBar>
#include <QPlainTextEdit>
#include <QGroupBox>
#include <QTextCursor>
#include <QSysInfo>
#include <QTextStream>
#include <QFileIconProvider>
#include <QMap>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSettings>
#include <QEasingCurve>
#include <QPropertyAnimation>

class ManagedWindow;

// ============================================================
// ManagedWindow
// ============================================================
class ManagedWindow : public QWidget {
public:
    QString winTitle;
    QWidget *contentWidget;
    bool minimized;
    bool maximized;
    QRect prevGeometry;
    std::function<void()> onCloseCallback;

    ManagedWindow(const QString &title, QWidget *content, QWidget *parent)
        : QWidget(parent), winTitle(title), contentWidget(content),
          minimized(false), maximized(false)
    {
        setWindowFlags(Qt::FramelessWindowHint);
        setMinimumSize(400, 300);
        setStyleSheet("ManagedWindow { background: rgba(40,40,40,230); border: 1px solid rgba(255,255,255,15); border-radius: 10px; }");

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        titleBar = new QWidget(this);
        titleBar->setFixedHeight(38);
        titleBar->setStyleSheet("QWidget#titlebar { background: transparent; }");
        titleBar->setObjectName("titlebar");

        QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
        titleLayout->setContentsMargins(12, 0, 12, 0);
        titleLayout->setSpacing(0);

        // Traffic light buttons (left side, macOS style)
        QWidget *trafficLights = new QWidget(titleBar);
        trafficLights->setFixedSize(56, 24);
        QHBoxLayout *tlLayout = new QHBoxLayout(trafficLights);
        tlLayout->setContentsMargins(0, 0, 0, 0);
        tlLayout->setSpacing(8);

        closeBtn = new QToolButton(trafficLights);
        closeBtn->setFixedSize(12, 12);
        closeBtn->setStyleSheet("QToolButton { background: #ff5f57; border: none; border-radius: 6px; } QToolButton:hover { background: #ff4040; }");
        connect(closeBtn, &QToolButton::clicked, this, &ManagedWindow::closeWindow);
        tlLayout->addWidget(closeBtn);

        minimizeBtn = new QToolButton(trafficLights);
        minimizeBtn->setFixedSize(12, 12);
        minimizeBtn->setStyleSheet("QToolButton { background: #febc2e; border: none; border-radius: 6px; } QToolButton:hover { background: #ffa500; }");
        connect(minimizeBtn, &QToolButton::clicked, this, &ManagedWindow::minimizeWindow);
        tlLayout->addWidget(minimizeBtn);

        maximizeBtn = new QToolButton(trafficLights);
        maximizeBtn->setFixedSize(12, 12);
        maximizeBtn->setStyleSheet("QToolButton { background: #28c840; border: none; border-radius: 6px; } QToolButton:hover { background: #20a830; }");
        connect(maximizeBtn, &QToolButton::clicked, this, &ManagedWindow::toggleMaximize);
        tlLayout->addWidget(maximizeBtn);

        titleLayout->addWidget(trafficLights);

        // Centered title
        titleLayout->addStretch();
        titleLabel = new QLabel(winTitle, titleBar);
        titleLabel->setStyleSheet("color: #e0e0e0; font-weight: 600; font-size: 13px; background: transparent;");
        titleLayout->addWidget(titleLabel);
        titleLayout->addStretch();
        // Dummy spacer to balance the traffic lights so title is truly centered
        titleLayout->addSpacing(56);

        layout->addWidget(titleBar);
        layout->addWidget(content);
        content->setStyleSheet("background: transparent; color: #e0e0e0;");
    }

    void setTitle(const QString &t) { winTitle = t; titleLabel->setText(t); }

    void minimizeWindow() {
        minimized = true;
        // macOS minimize animation: shrink to dock
        QPropertyAnimation *anim = new QPropertyAnimation(this, "geometry");
        anim->setDuration(250);
        anim->setEasingCurve(QEasingCurve::InBack);
        anim->setStartValue(geometry());
        anim->setEndValue(QRect(parentWidget()->width() / 2 - 50, parentWidget()->height() - 40, 100, 10));
        connect(anim, &QPropertyAnimation::finished, this, [this, anim]() { hide(); anim->deleteLater(); });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void toggleMaximize() {
        if (maximized) {
            QPropertyAnimation *anim = new QPropertyAnimation(this, "geometry");
            anim->setDuration(200);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->setStartValue(geometry());
            anim->setEndValue(prevGeometry);
            connect(anim, &QPropertyAnimation::finished, this, [anim]() { anim->deleteLater(); });
            anim->start(QAbstractAnimation::DeleteWhenStopped);
            maximized = false;
        } else {
            prevGeometry = geometry();
            QWidget *p = parentWidget();
            if (p) {
                QPropertyAnimation *anim = new QPropertyAnimation(this, "geometry");
                anim->setDuration(200);
                anim->setEasingCurve(QEasingCurve::OutCubic);
                anim->setStartValue(geometry());
                anim->setEndValue(QRect(0, 28, p->width(), p->height() - 28 - 80));
            connect(anim, &QPropertyAnimation::finished, this, [anim]() { anim->deleteLater(); });
                anim->start(QAbstractAnimation::DeleteWhenStopped);
            }
            maximized = true;
        }
    }

    void closeWindow() { hide(); minimized = true; if (onCloseCallback) onCloseCallback(); }

protected:
    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton && e->position().y() < 38) {
            dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
            dragging = true;
        }
        raise();
        QWidget::mousePressEvent(e);
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (dragging && (e->buttons() & Qt::LeftButton)) {
            if (maximized) { maximized = false; setGeometry(prevGeometry); dragPos = QPoint(width()/2, e->position().y()); }
            move(e->globalPosition().toPoint() - dragPos);
        }
        QWidget::mouseMoveEvent(e);
    }
    void mouseReleaseEvent(QMouseEvent *) override { dragging = false; }
    void mouseDoubleClickEvent(QMouseEvent *e) override {
        if (e->position().y() < 38) toggleMaximize();
        QWidget::mouseDoubleClickEvent(e);
    }

private:
    QWidget *titleBar;
    QLabel *titleLabel;
    QToolButton *minimizeBtn;
    QToolButton *maximizeBtn;
    QToolButton *closeBtn;
    QPoint dragPos;
    bool dragging = false;
};

// ============================================================
// TopBar - macOS-style menu bar at top
// ============================================================
class TopBar : public QWidget {
public:
    QLabel *clockLabel;
    QWidget *menuArea;
    QHBoxLayout *menuLayout;
    QToolButton *logoBtn;

    TopBar(QWidget *parent) : QWidget(parent) {
        setFixedHeight(28);
        setStyleSheet("TopBar { background: rgba(30,30,30,220); border-bottom: 1px solid rgba(255,255,255,30); }");

        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(12, 0, 12, 0);
        layout->setSpacing(0);

        // Left: Logo + app menus
        logoBtn = new QToolButton(this);
        logoBtn->setFixedSize(28, 28);
        logoBtn->setStyleSheet("QToolButton { background: transparent; color: #8ab4f8; border: none; font-size: 16px; font-weight: bold; border-radius: 14px; } QToolButton:hover { background: rgba(255,255,255,15); }");
        logoBtn->setText("\xf0\x9f\xa6\x86");
        layout->addWidget(logoBtn);

        menuArea = new QWidget(this);
        menuArea->setStyleSheet("background: transparent;");
        menuLayout = new QHBoxLayout(menuArea);
        menuLayout->setContentsMargins(8, 0, 0, 0);
        menuLayout->setSpacing(0);
        layout->addWidget(menuArea);

        layout->addStretch();

        // Center: Clock
        clockLabel = new QLabel(this);
        clockLabel->setFixedWidth(200);
        clockLabel->setAlignment(Qt::AlignCenter);
        clockLabel->setStyleSheet("color: #e0e0e0; font-size: 13px; font-weight: 500; background: transparent;");
        layout->addWidget(clockLabel);

        layout->addStretch();

        // Right: System indicators
        QWidget *rightArea = new QWidget(this);
        rightArea->setStyleSheet("background: transparent;");
        QHBoxLayout *rightLayout = new QHBoxLayout(rightArea);
        rightLayout->setContentsMargins(0, 0, 0, 0);
        rightLayout->setSpacing(6);

        volBtn = new QToolButton(rightArea);
        volBtn->setText("\xe2\x99\xab");
        volBtn->setFixedSize(28, 28);
        volBtn->setStyleSheet("QToolButton { background: transparent; color: #aaa; border: none; font-size: 14px; border-radius: 14px; } QToolButton:hover { background: rgba(255,255,255,15); }");
        rightLayout->addWidget(volBtn);

        wifiBtn = new QToolButton(rightArea);
        wifiBtn->setText("\xf0\x9f\x93\xbb");
        wifiBtn->setFixedSize(28, 28);
        wifiBtn->setStyleSheet("QToolButton { background: transparent; color: #aaa; border: none; font-size: 14px; border-radius: 14px; } QToolButton:hover { background: rgba(255,255,255,15); }");
        rightLayout->addWidget(wifiBtn);

        powerBtn = new QToolButton(rightArea);
        powerBtn->setText("\xe2\x89\x8b");
        powerBtn->setFixedSize(28, 28);
        powerBtn->setStyleSheet("QToolButton { background: transparent; color: #aaa; border: none; font-size: 14px; border-radius: 14px; } QToolButton:hover { background: rgba(255,255,255,15); }");
        rightLayout->addWidget(powerBtn);

        layout->addWidget(rightArea);

        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &TopBar::updateClock);
        timer->start(1000);
        updateClock();
    }

    void addMenu(const QString &text, std::function<void()> onClick) {
        QToolButton *btn = new QToolButton(this);
        btn->setText(text);
        btn->setStyleSheet("QToolButton { background: transparent; color: #ccc; border: none; font-size: 12px; padding: 4px 10px; border-radius: 4px; } QToolButton:hover { background: rgba(255,255,255,15); color: white; }");
        connect(btn, &QToolButton::clicked, this, [onClick]() { onClick(); });
        menuLayout->addWidget(btn);
    }

private:
    void updateClock() {
        clockLabel->setText(QDateTime::currentDateTime().toString("ddd MMM dd   hh:mm:ss"));
    }
    QToolButton *volBtn;
    QToolButton *wifiBtn;
    QToolButton *powerBtn;
};

// ============================================================
// DockButton - icon in the dock
// ============================================================
class DockButton : public QWidget {
public:
    QLabel *iconLabel;
    QLabel *dotLabel;
    bool isRunning;
    QString appName;
    std::function<void()> launcher;

    DockButton(const QString &name, const QIcon &icon, std::function<void()> launch, QWidget *parent)
        : QWidget(parent), appName(name), launcher(launch), isRunning(false)
    {
        setFixedSize(64, 80);
        setCursor(Qt::PointingHandCursor);

        QVBoxLayout *lay = new QVBoxLayout(this);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(2);

        iconLabel = new QLabel(this);
        iconLabel->setFixedSize(52, 52);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setPixmap(icon.pixmap(48, 48));
        iconLabel->setStyleSheet("background: rgba(255,255,255,20); border-radius: 12px; padding: 2px;");
        lay->addWidget(iconLabel, 0, Qt::AlignHCenter);

        dotLabel = new QLabel(this);
        dotLabel->setFixedSize(6, 6);
        dotLabel->setStyleSheet("background: transparent; border-radius: 3px;");
        lay->addWidget(dotLabel, 0, Qt::AlignHCenter);
    }

    void setRunning(bool running) {
        isRunning = running;
        dotLabel->setStyleSheet(running ? "background: #8ab4f8; border-radius: 3px;" : "background: transparent; border-radius: 3px;");
    }

protected:
    void enterEvent(QEnterEvent *) override {
        QPropertyAnimation *anim = new QPropertyAnimation(iconLabel, "geometry");
        anim->setDuration(150);
        anim->setEasingCurve(QEasingCurve::OutBack);
        QRect current = iconLabel->geometry();
        QRect target(current.x() - 4, current.y() - 4, 60, 60);
        anim->setStartValue(current);
        anim->setEndValue(target);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
        iconLabel->setStyleSheet("background: rgba(255,255,255,35); border-radius: 14px; padding: 2px;");
    }

    void leaveEvent(QEvent *) override {
        QPropertyAnimation *anim = new QPropertyAnimation(iconLabel, "geometry");
        anim->setDuration(150);
        anim->setEasingCurve(QEasingCurve::InBack);
        QRect current = iconLabel->geometry();
        QRect target(current.x() + 4, current.y() + 4, 52, 52);
        anim->setStartValue(current);
        anim->setEndValue(target);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
        iconLabel->setStyleSheet("background: rgba(255,255,255,20); border-radius: 12px; padding: 2px;");
    }

    void mousePressEvent(QMouseEvent *) override {
        if (launcher) launcher();
    }
};

// ============================================================
// Dock - macOS-style dock at bottom
// ============================================================
class Dock : public QWidget {
public:
    QList<DockButton*> dockButtons;

    Dock(QWidget *parent) : QWidget(parent) {
        setFixedHeight(80);

        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(12, 0, 12, 0);
        layout->setSpacing(4);
        layout->addStretch();
    }

    DockButton *addApp(const QString &name, const QIcon &icon, std::function<void()> launcher) {
        DockButton *btn = new DockButton(name, icon, launcher, this);
        dockButtons.append(btn);
        int insertIdx = layout()->count() - 1;
        dynamic_cast<QHBoxLayout*>(layout())->insertWidget(insertIdx, btn);
        return btn;
    }

    void setRunning(const QString &name, bool running) {
        for (DockButton *btn : dockButtons) {
            if (btn->appName == name) btn->setRunning(running);
        }
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        // Glass background
        p.setBrush(QColor(30, 30, 30, 180));
        p.setPen(QPen(QColor(255, 255, 255, 40), 1.5));
        p.drawRoundedRect(rect().adjusted(12, 6, -12, -6), 18, 18);
        // Subtle inner glow
        p.setPen(QPen(QColor(255, 255, 255, 10), 1));
        p.drawRoundedRect(rect().adjusted(13, 7, -13, -7), 17, 17);
    }
};

// ============================================================
// StartMenu
// ============================================================
class StartMenu : public QWidget {
public:
    QWidget *overlay;
    StartMenu(QWidget *overlayWidget, QWidget *parent) : QWidget(parent), overlay(overlayWidget) {
        setFixedWidth(260);
        setStyleSheet("StartMenu { background: rgba(30,30,30,230); border: 1px solid rgba(255,255,255,30); border-radius: 10px; }");
        hide();
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(2);
        QLabel *header = new QLabel("  Parrot Desktop", this);
        header->setFixedHeight(36);
        header->setStyleSheet("color: #8ab4f8; font-size: 14px; font-weight: bold; background: transparent; padding: 4px;");
        layout->addWidget(header);
        QFrame *line = new QFrame(this);
        line->setFrameShape(QFrame::HLine);
        line->setStyleSheet("color: rgba(255,255,255,30);");
        layout->addWidget(line);
        appsLayout = layout;
    }

    void addApp(const QString &name, const QString &iconText, std::function<void()> launcher) {
        QToolButton *btn = new QToolButton(this);
        btn->setText("  " + iconText + "    " + name);
        btn->setFixedHeight(36);
        btn->setStyleSheet("QToolButton { background: transparent; color: #e0e0e0; border: none; text-align: left; font-size: 12px; padding: 4px 8px; border-radius: 6px; } QToolButton:hover { background: rgba(138,180,248,30); }");
        appsLayout->addWidget(btn);
        connect(btn, &QToolButton::clicked, this, [this, launcher]() { hide(); overlay->hide(); launcher(); });
    }

    void toggle() {
        if (isVisible()) { hide(); overlay->hide(); }
        else {
            QWidget *p = parentWidget();
            if (p) move(8, 32);
            show(); raise();
            overlay->setGeometry(0, 0, parentWidget()->width(), parentWidget()->height());
            overlay->show(); overlay->raise();
        }
    }

private:
    QLayout *appsLayout;
};

// ============================================================
// DesktopSurface
// ============================================================
class DesktopSurface : public QWidget {
public:
    QPixmap wallpaper;
    std::function<void()> onNewFolder;
    std::function<void()> onOpenFileManager;
    std::function<void()> onSetWallpaper;
    std::function<void()> onOpenTerminal;
    std::function<void()> onAbout;
    std::function<void()> onOpenTextEditor;
    std::function<void()> onRefreshDesktop;

    DesktopSurface(QWidget *parent) : QWidget(parent) {
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QWidget::customContextMenuRequested, this, &DesktopSurface::showContextMenu);
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        if (!wallpaper.isNull()) {
            QPixmap scaled = wallpaper.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            p.drawPixmap((width() - scaled.width()) / 2, (height() - scaled.height()) / 2, scaled);
        } else {
            // macOS Sonoma-style gradient
            QLinearGradient grad(0, 0, width(), height());
            grad.setColorAt(0, QColor(20, 30, 60));
            grad.setColorAt(0.3, QColor(30, 50, 90));
            grad.setColorAt(0.6, QColor(20, 40, 80));
            grad.setColorAt(1, QColor(10, 20, 40));
            p.fillRect(rect(), grad);
        }
    }

private:
    void showContextMenu(QPoint pos) {
        QMenu menu(this);
        menu.setStyleSheet("QMenu { background: #3c3c3c; color: #e0e0e0; border: 1px solid #555; padding: 4px; } QMenu::item { padding: 6px 24px; border-radius: 4px; } QMenu::item:selected { background: #4a6fa5; }");
        menu.addAction(style()->standardIcon(QStyle::SP_DirHomeIcon), "Open File Manager", onOpenFileManager);
        menu.addAction(style()->standardIcon(QStyle::SP_ComputerIcon), "Open Terminal", onOpenTerminal);
        menu.addAction(style()->standardIcon(QStyle::SP_FileIcon), "Open Text Editor", onOpenTextEditor);
        menu.addSeparator();
        menu.addAction(style()->standardIcon(QStyle::SP_DirIcon), "New Folder", onNewFolder);
        menu.addAction(style()->standardIcon(QStyle::SP_DesktopIcon), "Set Wallpaper...", onSetWallpaper);
        menu.addSeparator();
        menu.addAction(style()->standardIcon(QStyle::SP_BrowserReload), "Refresh Desktop", onRefreshDesktop);
        menu.addAction(style()->standardIcon(QStyle::SP_MessageBoxInformation), "About Parrot", onAbout);
        menu.exec(mapToGlobal(pos));
    }
};

// ============================================================
// FileManagerWidget
// ============================================================
class FileManagerWidget : public QWidget {
public:
    FileManagerWidget(QWidget *parent = nullptr) : QWidget(parent), clipCut(false) {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        QToolBar *toolbar = new QToolBar(this);
        toolbar->setMovable(false);
        toolbar->setIconSize(QSize(18, 18));
        toolbar->setStyleSheet("QToolBar { background: #333; border: none; spacing: 2px; padding: 2px; } QToolButton { background: #444; border: 1px solid #555; border-radius: 3px; padding: 3px 6px; color: #ccc; } QToolButton:hover { background: #555; }");

        QAction *actUp = toolbar->addAction(style()->standardIcon(QStyle::SP_ArrowUp), "Up");
        QAction *actHome = toolbar->addAction(style()->standardIcon(QStyle::SP_DirHomeIcon), "Home");
        QAction *actBack = toolbar->addAction(style()->standardIcon(QStyle::SP_ArrowBack), "Back");
        QAction *actFwd = toolbar->addAction(style()->standardIcon(QStyle::SP_ArrowForward), "Forward");
        toolbar->addSeparator();
        QAction *actNewF = toolbar->addAction(style()->standardIcon(QStyle::SP_DirIcon), "New Folder");
        QAction *actNewFile = toolbar->addAction(style()->standardIcon(QStyle::SP_FileIcon), "New File");
        toolbar->addSeparator();
        QAction *actCopy = toolbar->addAction(style()->standardIcon(QStyle::SP_FileDialogContentsView), "Copy");
        QAction *actCut = toolbar->addAction(style()->standardIcon(QStyle::SP_FileDialogDetailedView), "Cut");
        QAction *actPaste = toolbar->addAction(style()->standardIcon(QStyle::SP_FileDialogInfoView), "Paste");
        toolbar->addSeparator();
        QAction *actDel = toolbar->addAction(style()->standardIcon(QStyle::SP_TrashIcon), "Delete");
        QAction *actRefr = toolbar->addAction(style()->standardIcon(QStyle::SP_BrowserReload), "Refresh");

        mainLayout->addWidget(toolbar);

        QHBoxLayout *pathLayout = new QHBoxLayout();
        pathLayout->setContentsMargins(4, 2, 4, 2);
        QLabel *pathLabel = new QLabel(" Path:", this);
        pathLabel->setStyleSheet("color: #aaa; background: transparent;");
        pathEdit = new QLineEdit(this);
        pathEdit->setStyleSheet("background: #2a2a2a; color: #e0e0e0; border: 1px solid #444; border-radius: 3px; padding: 3px;");
        pathLayout->addWidget(pathLabel);
        pathLayout->addWidget(pathEdit);
        mainLayout->addLayout(pathLayout);

        QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
        splitter->setStyleSheet("QSplitter::handle { background: #444; width: 3px; }");

        fsModel = new QFileSystemModel(this);
        fsModel->setRootPath(QDir::homePath());
        fsModel->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
        treeView = new QTreeView(this);
        treeView->setModel(fsModel);
        treeView->setRootIndex(fsModel->index(QDir::homePath()));
        treeView->setHeaderHidden(true);
        treeView->setColumnHidden(1, true); treeView->setColumnHidden(2, true); treeView->setColumnHidden(3, true);
        treeView->setMinimumWidth(180); treeView->setAnimated(true);
        treeView->setStyleSheet("QTreeView { background: #252525; color: #e0e0e0; border: none; } QTreeView::item:hover { background: #3a3a4a; } QTreeView::item:selected { background: #4a6fa5; }");

        detailModel = new QFileSystemModel(this);
        detailModel->setRootPath(QDir::homePath());
        detailModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
        listView = new QListView(this);
        listView->setModel(detailModel);
        listView->setRootIndex(detailModel->index(QDir::homePath()));
        listView->setViewMode(QListView::IconMode);
        listView->setIconSize(QSize(40, 40));
        listView->setGridSize(QSize(80, 80));
        listView->setWrapping(true);
        listView->setResizeMode(QListView::Adjust);
        listView->setMovement(QListView::Snap);
        listView->setSpacing(6);
        listView->setContextMenuPolicy(Qt::CustomContextMenu);
        listView->setStyleSheet("QListView { background: #252525; color: #e0e0e0; border: none; } QListView::item:hover { background: #3a3a4a; } QListView::item:selected { background: #4a6fa5; }");

        splitter->addWidget(treeView); splitter->addWidget(listView);
        splitter->setStretchFactor(0, 0); splitter->setStretchFactor(1, 1);
        splitter->setSizes({180, 500});
        mainLayout->addWidget(splitter, 1);

        statusLabel = new QLabel(this);
        statusLabel->setFixedHeight(22);
        statusLabel->setStyleSheet("color: #888; background: #333; border-top: 1px solid #444; padding: 2px 8px; font-size: 11px;");
        mainLayout->addWidget(statusLabel);

        connect(treeView, &QTreeView::clicked, this, [this](const QModelIndex &i) { navigateTo(fsModel->filePath(i)); });
        connect(listView, &QListView::doubleClicked, this, [this](const QModelIndex &i) {
            QString p = detailModel->filePath(i);
            if (QFileInfo(p).isDir()) navigateTo(p);
            else QDesktopServices::openUrl(QUrl::fromLocalFile(p));
        });
        connect(listView, &QWidget::customContextMenuRequested, this, &FileManagerWidget::showContextMenu);
        connect(pathEdit, &QLineEdit::returnPressed, this, [this]() {
            QString p = pathEdit->text().trimmed();
            if (QDir(p).exists()) navigateTo(p);
            else QMessageBox::warning(this, "Error", "Path not found: " + p);
        });
        connect(actUp, &QAction::triggered, this, [this]() { QDir d(currentDir); if (d.cdUp()) navigateTo(d.absolutePath()); });
        connect(actHome, &QAction::triggered, this, [this]() { navigateTo(QDir::homePath()); });
        connect(actBack, &QAction::triggered, this, &FileManagerWidget::goBack);
        connect(actFwd, &QAction::triggered, this, &FileManagerWidget::goForward);
        connect(actNewF, &QAction::triggered, this, &FileManagerWidget::newFolder);
        connect(actNewFile, &QAction::triggered, this, &FileManagerWidget::newFile);
        connect(actCopy, &QAction::triggered, this, &FileManagerWidget::copySel);
        connect(actCut, &QAction::triggered, this, &FileManagerWidget::cutSel);
        connect(actPaste, &QAction::triggered, this, &FileManagerWidget::paste);
        connect(actDel, &QAction::triggered, this, &FileManagerWidget::deleteSel);
        connect(actRefr, &QAction::triggered, this, &FileManagerWidget::refresh);

        currentDir = QDir::homePath();
        updateStatus();
    }

    void navigateTo(const QString &path) {
        if (!QDir(path).exists()) return;
        history.append(currentDir); forwardHistory.clear();
        currentDir = path;
        detailModel->setRootPath(path);
        listView->setRootIndex(detailModel->index(path));
        treeView->setCurrentIndex(fsModel->index(path));
        treeView->expand(fsModel->index(path));
        pathEdit->setText(path);
        updateStatus();
    }

private:
    void goBack() {
        if (history.isEmpty()) return;
        forwardHistory.append(currentDir); currentDir = history.takeLast();
        detailModel->setRootPath(currentDir); listView->setRootIndex(detailModel->index(currentDir));
        pathEdit->setText(currentDir); updateStatus();
    }
    void goForward() {
        if (forwardHistory.isEmpty()) return;
        history.append(currentDir); currentDir = forwardHistory.takeLast();
        detailModel->setRootPath(currentDir); listView->setRootIndex(detailModel->index(currentDir));
        pathEdit->setText(currentDir); updateStatus();
    }
    void newFolder() {
        bool ok; QString n = QInputDialog::getText(this, "New Folder", "Name:", QLineEdit::Normal, "New Folder", &ok);
        if (ok && !n.isEmpty()) { QDir(currentDir).mkdir(n); refresh(); }
    }
    void newFile() {
        bool ok; QString n = QInputDialog::getText(this, "New File", "Name:", QLineEdit::Normal, "file.txt", &ok);
        if (ok && !n.isEmpty()) { QFile f(currentDir + "/" + n); if (f.open(QIODevice::WriteOnly)) f.close(); refresh(); }
    }
    void copySel() {
        clipPaths.clear(); clipCut = false;
        for (auto &i : listView->selectionModel()->selectedIndexes()) clipPaths.append(detailModel->filePath(i));
    }
    void cutSel() {
        clipPaths.clear(); clipCut = true;
        for (auto &i : listView->selectionModel()->selectedIndexes()) clipPaths.append(detailModel->filePath(i));
    }
    void paste() {
        for (auto &src : clipPaths) {
            QFileInfo info(src); QString dest = currentDir + "/" + info.fileName();
            if (clipCut) QFile::rename(src, dest);
            else if (info.isDir()) copyDir(src, dest);
            else QFile::copy(src, dest);
        }
        if (clipCut) clipPaths.clear(); refresh();
    }
    void deleteSel() {
        auto sel = listView->selectionModel()->selectedIndexes();
        if (sel.isEmpty()) return;
        if (QMessageBox::question(this, "Delete", "Delete " + QString::number(sel.size()) + " item(s)?") == QMessageBox::Yes) {
            for (auto &i : sel) { QString p = detailModel->filePath(i); if (QFileInfo(p).isDir()) QDir(p).removeRecursively(); else QFile::remove(p); }
            refresh();
        }
    }
    void copyDir(const QString &src, const QString &dest) {
        QDir(dest).mkpath(".");
        for (auto &e : QDir(src).entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (e.isDir()) copyDir(e.absoluteFilePath(), dest + "/" + e.fileName());
            else QFile::copy(e.absoluteFilePath(), dest + "/" + e.fileName());
        }
    }
    void refresh() {
        detailModel->setRootPath(currentDir); listView->setRootIndex(detailModel->index(currentDir)); updateStatus();
    }
    void showContextMenu(QPoint pos) {
        auto sel = listView->selectionModel()->selectedIndexes();
        QMenu menu(this);
        menu.setStyleSheet("QMenu { background: #3c3c3c; color: #e0e0e0; border: 1px solid #555; } QMenu::item { padding: 5px 20px; } QMenu::item:selected { background: #4a6fa5; }");
        if (sel.isEmpty()) {
            menu.addAction("New Folder", this, &FileManagerWidget::newFolder);
            menu.addAction("New File", this, &FileManagerWidget::newFile);
            menu.addAction("Refresh", this, &FileManagerWidget::refresh);
        } else {
            menu.addAction("Open", this, [this, sel]() {
                QString p = detailModel->filePath(sel.first());
                if (QFileInfo(p).isDir()) navigateTo(p); else QDesktopServices::openUrl(QUrl::fromLocalFile(p));
            });
            menu.addSeparator();
            menu.addAction("Copy", this, &FileManagerWidget::copySel);
            menu.addAction("Cut", this, &FileManagerWidget::cutSel);
            menu.addAction("Paste", this, &FileManagerWidget::paste);
            menu.addSeparator();
            menu.addAction("Delete", this, &FileManagerWidget::deleteSel);
            menu.addSeparator();
            menu.addAction("Properties", this, [this, sel]() {
                QFileInfo info(detailModel->filePath(sel.first()));
                QMessageBox::information(this, "Properties", "Name: " + info.fileName() + "\nPath: " + info.absoluteFilePath() + "\nSize: " + QString::number(info.size()) + " bytes\nType: " + QString(info.isDir() ? "Directory" : "File") + "\nModified: " + info.lastModified().toString());
            });
        }
        menu.exec(listView->viewport()->mapToGlobal(pos));
    }
    void updateStatus() {
        int count = QDir(currentDir).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).count();
        statusLabel->setText("  " + currentDir + "  |  " + QString::number(count) + " items");
    }

    QFileSystemModel *fsModel, *detailModel;
    QTreeView *treeView; QListView *listView;
    QLineEdit *pathEdit; QLabel *statusLabel;
    QString currentDir;
    QStringList history, forwardHistory, clipPaths;
    bool clipCut;
};

// ============================================================
// TerminalWidget
// ============================================================
class TerminalWidget : public QWidget {
public:
    TerminalWidget(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(0);
        output = new QPlainTextEdit(this);
        output->setReadOnly(true);
        output->setStyleSheet("QPlainTextEdit { background: #1a1a2e; color: #00ff41; border: none; font-family: monospace; font-size: 13px; padding: 8px; }");
        output->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        layout->addWidget(output, 1);
        QHBoxLayout *inputLayout = new QHBoxLayout();
        inputLayout->setContentsMargins(8, 4, 8, 8);
        prompt = new QLabel("$ ", this);
        prompt->setStyleSheet("color: #4a6fa5; font-family: monospace; font-size: 13px; background: transparent;");
        inputLayout->addWidget(prompt);
        cmdEdit = new QLineEdit(this);
        cmdEdit->setStyleSheet("QLineEdit { background: #1a1a2e; color: #00ff41; border: none; font-family: monospace; font-size: 13px; padding: 4px; }");
        inputLayout->addWidget(cmdEdit, 1);
        layout->addLayout(inputLayout);
        process = new QProcess(this);
        process->setProcessChannelMode(QProcess::MergedChannels);
        connect(process, &QProcess::readyReadStandardOutput, this, &TerminalWidget::readOutput);
        connect(cmdEdit, &QLineEdit::returnPressed, this, &TerminalWidget::executeCommand);
        appendOutput("Parrot Terminal v1.0\nType commands below.\n\n");
        cwd = QDir::homePath();
        prompt->setText(cwd + " $ ");
    }
private:
    void executeCommand() {
        QString cmd = cmdEdit->text().trimmed(); if (cmd.isEmpty()) return;
        appendOutput(cwd + " $ " + cmd + "\n"); cmdEdit->clear();
        if (cmd == "clear") { output->clear(); return; }
        if (cmd == "exit") { QWidget *w = parentWidget(); while (w && !dynamic_cast<ManagedWindow*>(w)) w = w->parentWidget(); if (w) w->close(); return; }
        if (cmd.startsWith("cd ")) {
            QString path = cmd.mid(3).trimmed();
            QDir d(path);
            if (d.exists()) { cwd = d.absolutePath(); prompt->setText(cwd + " $ "); }
            else appendOutput("cd: no such directory: " + path + "\n");
            return;
        }
        process->setWorkingDirectory(cwd);
        process->start("sh", {"-c", cmd});
        if (!process->waitForFinished(5000)) { process->kill(); appendOutput("Command timed out\n"); }
    }
    void readOutput() { appendOutput(QString(process->readAllStandardOutput())); }
    void appendOutput(const QString &text) { output->moveCursor(QTextCursor::End); output->insertPlainText(text); output->moveCursor(QTextCursor::End); }

    QPlainTextEdit *output; QLineEdit *cmdEdit; QLabel *prompt; QProcess *process; QString cwd;
};

// ============================================================
// TextEditorWidget
// ============================================================
class TextEditorWidget : public QWidget {
public:
    TextEditorWidget(const QString &filePath = "", QWidget *parent = nullptr) : QWidget(parent), currentFile(filePath) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(0);
        QToolBar *toolbar = new QToolBar(this);
        toolbar->setStyleSheet("QToolBar { background: #333; border: none; }");
        toolbar->addAction("Open", this, &TextEditorWidget::openFile);
        toolbar->addAction("Save", this, &TextEditorWidget::saveFile);
        layout->addWidget(toolbar);
        editor = new QPlainTextEdit(this);
        editor->setStyleSheet("QPlainTextEdit { background: #1e1e1e; color: #d4d4d4; border: none; font-family: monospace; font-size: 13px; padding: 8px; }");
        layout->addWidget(editor, 1);
        if (!filePath.isEmpty()) {
            QFile f(filePath);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) { editor->setPlainText(QString(f.readAll())); f.close(); }
        }
    }
private:
    void openFile() {
        QString f = QFileDialog::getOpenFileName(this, "Open", QDir::homePath());
        if (!f.isEmpty()) { currentFile = f; QFile file(f); if (file.open(QIODevice::ReadOnly | QIODevice::Text)) { editor->setPlainText(QString(file.readAll())); file.close(); } }
    }
    void saveFile() {
        if (currentFile.isEmpty()) { currentFile = QFileDialog::getSaveFileName(this, "Save As", QDir::homePath() + "/untitled.txt"); if (currentFile.isEmpty()) return; }
        QFile f(currentFile); if (f.open(QIODevice::WriteOnly | QIODevice::Text)) { f.write(editor->toPlainText().toUtf8()); f.close(); }
    }
    QPlainTextEdit *editor; QString currentFile;
};

// ============================================================
// SettingsWidget
// ============================================================
class SettingsWidget : public QWidget {
public:
    SettingsWidget(QWidget *parent) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(24, 24, 24, 24); layout->setSpacing(16);
        QLabel *title = new QLabel("Parrot Desktop Settings", this);
        title->setStyleSheet("color: #4a6fa5; font-size: 20px; font-weight: bold; background: transparent;");
        layout->addWidget(title);
        QLabel *info = new QLabel(
            "Parrot Desktop Environment v1.0.0\n\n"
            "Qt Version: " + QString(qVersion()) + "\n"
            "Kernel: " + QSysInfo::kernelVersion() + "\n"
            "OS: " + QSysInfo::prettyProductName() + "\n\n"
            "Features:\n"
            "  - Window manager with drag, minimize, maximize\n"
            "  - Taskbar with running app indicators\n"
            "  - Start menu with application launcher\n"
            "  - Desktop icons\n"
            "  - Customizable wallpaper\n"
            "  - File Manager, Terminal, Text Editor", this);
        info->setStyleSheet("color: #ccc; font-size: 13px; background: transparent; line-height: 1.5;");
        layout->addWidget(info);
        layout->addStretch();
    }
};

// ============================================================
// ParrotDE
// ============================================================
class ParrotDE : public QMainWindow {
public:
    ParrotDE(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Parrot Desktop Environment");
        setMinimumSize(1024, 700);

        QWidget *central = new QWidget(this);
        setCentralWidget(central);
        QVBoxLayout *mainLayout = new QVBoxLayout(central);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        topBar = new TopBar(this);
        mainLayout->addWidget(topBar);

        desktop = new DesktopSurface(this);
        mainLayout->addWidget(desktop, 1);

        dock = new Dock(this);
        mainLayout->addWidget(dock);

        overlay = new QWidget(this);
        overlay->setStyleSheet("background: rgba(0,0,0,100);");
        overlay->hide();
        overlay->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(overlay, &QWidget::customContextMenuRequested, this, [this](QPoint) { startMenu->toggle(); });

        startMenu = new StartMenu(overlay, this);
        startMenu->addApp("File Manager", "Folder", [this]() { launchParrotApp(""); });
        startMenu->addApp("Terminal", ">_", [this]() { openTerminal(); });
        startMenu->addApp("Text Editor", "Edit", [this]() { openTextEditor(); });
        startMenu->addApp("Settings", "Gear", [this]() { openSettings(); });

        // Top bar logo opens start menu
        connect(topBar->logoBtn, &QToolButton::clicked, this, [this]() { startMenu->toggle(); });

        // Add dock apps
        dock->addApp("File Manager", style()->standardIcon(QStyle::SP_DirIcon), [this]() { launchParrotApp(""); });
        dock->addApp("Terminal", style()->standardIcon(QStyle::SP_ComputerIcon), [this]() { openTerminal(); });
        dock->addApp("Text Editor", style()->standardIcon(QStyle::SP_FileIcon), [this]() { openTextEditor(); });
        dock->addApp("Settings", style()->standardIcon(QStyle::SP_FileDialogInfoView), [this]() { openSettings(); });

        desktop->onOpenFileManager = [this]() { launchParrotApp(""); };
        desktop->onOpenTerminal = [this]() { openTerminal(); };
        desktop->onOpenTextEditor = [this]() { openTextEditor(); };
        desktop->onNewFolder = [this]() {
            bool ok; QString n = QInputDialog::getText(this, "New Folder", "Name:", QLineEdit::Normal, "New Folder", &ok);
            if (ok && !n.isEmpty()) { QDir(QDir::homePath() + "/Desktop").mkdir(n); refreshDesktopIcons(); }
        };
        desktop->onSetWallpaper = [this]() {
            QString f = QFileDialog::getOpenFileName(this, "Select Wallpaper", QDir::homePath(), "Images (*.png *.jpg *.jpeg *.bmp *.webp);;All Files (*)");
            if (!f.isEmpty()) { desktop->wallpaper = QPixmap(f); desktop->update(); }
        };
        desktop->onAbout = [this]() {
            QMessageBox::about(this, "About Parrot Desktop", "<h2>Parrot Desktop Environment</h2><p>Version 2.0</p><p>Modern desktop environment with macOS-style top bar and Ubuntu-style dock.</p>");
        };
        desktop->onRefreshDesktop = [this]() { refreshDesktopIcons(); };

        addDesktopIcons();

        resize(1200, 800);
        showMaximized();
    }

    void openFileManager() { launchParrotApp(""); }
    void openFileManagerAt(const QString &path) { launchParrotApp(path); }

    void launchParrotApp(const QString &path) {
        QSettings cfg("parrot-desktop", "parrot-desktop");
        QString parrotApp = cfg.value("parrot-app-path").toString();

        if (parrotApp.isEmpty() || !QFileInfo(parrotApp).exists()) {
            parrotApp.clear();
            QStringList searchPaths = {
                QDir::homePath() + "/.local/share/parrot/Parrot.app",
                "/opt/Parrot.app",
            };
            // Search XDG data dirs
            for (const QString &xdg : QString(qgetenv("XDG_DATA_HOME") + ":/usr/local/share:/usr/share").split(':')) {
                if (!xdg.isEmpty()) searchPaths.append(xdg + "/parrot/Parrot.app");
            }
            // Walk up from CWD
            QString search = QDir::currentPath();
            while (search != "/" && !search.isEmpty()) {
                searchPaths.append(search + "/Parrot/app/Parrot.app");
                search = QFileInfo(search).path();
            }
            // Walk up from HOME
            search = QDir::homePath();
            while (search != "/" && !search.isEmpty()) {
                searchPaths.append(search + "/Parrot/app/Parrot.app");
                search = QFileInfo(search).path();
            }

            for (const QString &p : searchPaths) {
                if (QFileInfo(p).exists()) { parrotApp = p; break; }
            }

            if (!parrotApp.isEmpty()) cfg.setValue("parrot-app-path", parrotApp);
        }

        if (parrotApp.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path.isEmpty() ? QDir::homePath() : path));
            return;
        }

        QString tmpDir = QDir::tempPath() + "/parrot-extracted";
        QDir().mkpath(tmpDir);
        QProcess::execute("unzip", {"-o", "-j", parrotApp, "binary", "-d", tmpDir});
        QString binary = tmpDir + "/binary";
        if (QFileInfo(binary).exists()) {
            QFile::setPermissions(binary, QFileInfo(binary).permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther);
            QStringList args;
            if (!path.isEmpty()) args << path;
            QProcess::startDetached(binary, args);
        } else {
            QProcess::startDetached("pkgr", {"run", parrotApp});
        }
    }
    void openTerminal() {
        QStringList terminals = {"x-terminal-emulator", "gnome-terminal", "konsole", "xfce4-terminal", "alacritty", "foot", "xterm"};
        for (const QString &term : terminals) {
            if (!QStandardPaths::findExecutable(term).isEmpty()) {
                QProcess::startDetached(term);
                return;
            }
        }
        QMessageBox::warning(this, "Error", "No terminal emulator found");
    }

    void openTextEditor() {
        QStringList editors = {"xdg-open", "xdg-mime", "sensible-editor", "nano", "vim"};
        for (const QString &ed : editors) {
            if (!QStandardPaths::findExecutable(ed).isEmpty()) {
                if (ed == "xdg-open" || ed == "xdg-mime") {
                    QProcess::startDetached(ed, {QDir::homePath() + "/.config/parrot-desktop/untitled.txt"});
                } else {
                    QProcess::startDetached(ed);
                }
                return;
            }
        }
        QMessageBox::warning(this, "Error", "No text editor found");
    }
    void openSettings() { openWindow("Settings", new SettingsWidget(desktop)); }

protected:
    void resizeEvent(QResizeEvent *e) override {
        QMainWindow::resizeEvent(e);
        overlay->setGeometry(0, 0, width(), height());
        if (startMenu->isVisible()) startMenu->move(8, 32);
        for (ManagedWindow *w : managedWindows)
            if (w->maximized && w->isVisible()) w->setGeometry(0, 28, width(), height() - 28 - 80);
    }

private:
    void addDesktopIcons() {
        desktopLayout = new QVBoxLayout(desktop);
        desktopLayout->setContentsMargins(16, 16, 16, 16);
        iconsFlowLayout = new QVBoxLayout();
        iconsFlowLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        desktopLayout->addLayout(iconsFlowLayout);
        desktopLayout->addStretch();
        refreshDesktopIcons();

        // Auto-refresh desktop icons every 3 seconds
        refreshTimer = new QTimer(this);
        connect(refreshTimer, &QTimer::timeout, this, &ParrotDE::refreshDesktopIcons);
        refreshTimer->start(3000);
    }

    void refreshDesktopIcons() {
        // Clear old icons
        QLayoutItem *item;
        while ((item = iconsFlowLayout->takeAt(0)) != nullptr) {
            if (item->layout()) {
                QLayoutItem *sub;
                while ((sub = item->layout()->takeAt(0)) != nullptr) {
                    if (sub->widget()) sub->widget()->deleteLater();
                    delete sub;
                }
            }
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }

        QString desktopPath = QDir::homePath() + "/Desktop";
        if (!QDir().exists(desktopPath)) QDir().mkpath(desktopPath);

        QDir dir(desktopPath);
        QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name);

        QHBoxLayout *row = nullptr;
        int colCount = 0;
        const int maxPerRow = 10;

        for (const QFileInfo &fi : entries) {
            if (colCount % maxPerRow == 0) {
                row = new QHBoxLayout();
                row->setSpacing(20);
                row->setAlignment(Qt::AlignLeft);
                iconsFlowLayout->addLayout(row);
            }

            QVBoxLayout *iconBox = new QVBoxLayout();
            iconBox->setAlignment(Qt::AlignTop);
            iconBox->setSpacing(4);

            QString displayName = fi.fileName();
            if (fi.fileName().endsWith(".desktop")) {
                QString parsedName = parseDesktopName(fi.absoluteFilePath());
                if (!parsedName.isEmpty()) displayName = parsedName;
            } else if (fi.fileName().endsWith(".app")) {
                QString parsedName = parseAppName(fi.absoluteFilePath());
                if (!parsedName.isEmpty()) displayName = parsedName;
            }

            QLabel *iconLabel = new QLabel(desktop);
            iconLabel->setAlignment(Qt::AlignCenter);
            iconLabel->setFixedSize(72, 72);
            iconLabel->setStyleSheet("background: rgba(0,0,0,60); border-radius: 8px; padding: 8px;");

            if (fi.fileName().endsWith(".desktop")) {
                iconLabel->setPixmap(parseDesktopIcon(fi.absoluteFilePath()).pixmap(56, 56));
            } else if (fi.fileName().endsWith(".app")) {
                iconLabel->setPixmap(extractAppIcon(fi.absoluteFilePath()).pixmap(56, 56));
            } else if (fi.isDir()) {
                iconLabel->setPixmap(style()->standardIcon(QStyle::SP_DirIcon).pixmap(56, 56));
            } else {
                QFileIconProvider provider;
                QIcon fileIcon = provider.icon(fi);
                if (fileIcon.isNull()) fileIcon = style()->standardIcon(QStyle::SP_FileIcon);
                iconLabel->setPixmap(fileIcon.pixmap(56, 56));
            }

            QLabel *textLabel = new QLabel(displayName, desktop);
            textLabel->setAlignment(Qt::AlignCenter);
            textLabel->setStyleSheet("color: white; font-size: 11px; background: rgba(0,0,0,140); padding: 2px 8px; border-radius: 3px;");
            textLabel->setFixedWidth(76);
            textLabel->setWordWrap(true);

            iconBox->addWidget(iconLabel);
            iconBox->addWidget(textLabel);

            QWidget *container = new QWidget(desktop);
            container->setLayout(iconBox);
            container->setCursor(Qt::PointingHandCursor);
            container->setFixedSize(80, 110);

            QString filePath = fi.absoluteFilePath();
            bool isDir = fi.isDir();

            // Double-click to open
            connect(container, &QWidget::customContextMenuRequested, this, [this, filePath, isDir](QPoint) {
                QMenu m(this);
                m.setStyleSheet("QMenu { background: #3c3c3c; color: #e0e0e0; border: 1px solid #555; padding: 4px; } QMenu::item { padding: 6px 24px; border-radius: 4px; } QMenu::item:selected { background: #4a6fa5; }");
                m.addAction("Open", this, [this, filePath, isDir]() { openDesktopItem(filePath, isDir); });
                m.addAction("Rename", this, [this, filePath]() { renameDesktopItem(filePath); });
                m.addAction("Delete", this, [this, filePath]() { deleteDesktopItem(filePath); });
                m.addSeparator();
                m.addAction("Properties", this, [this, filePath]() { showDesktopItemProperties(filePath); });
                m.exec(QCursor::pos());
            });

            // Click to select, double-click to open
            installClickHandler(container, filePath, isDir);

            if (row) row->addWidget(container);
            colCount++;
        }
    }

    void installClickHandler(QWidget *w, const QString &path, bool isDir) {
        // We use an event filter to detect double-clicks
        w->installEventFilter(this);
        pendingActions[w] = [this, path, isDir]() { openDesktopItem(path, isDir); };
    }

    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::MouseButtonDblClick) {
            QWidget *w = dynamic_cast<QWidget*>(obj);
            if (w && pendingActions.contains(w)) {
                pendingActions[w]();
                return true;
            }
        }
        return QMainWindow::eventFilter(obj, event);
    }

    void openDesktopItem(const QString &path, bool isDir) {
        if (isDir) {
            launchParrotApp(path);
        } else if (path.endsWith(".desktop")) {
            launchDesktopFile(path);
        } else if (path.endsWith(".app")) {
            QProcess::startDetached("pkgr", {"run", path});
        } else {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    }

    QIcon extractAppIcon(const QString &appPath) {
        QProcess proc;
        proc.start("unzip", {"-p", appPath, "icon.png"});
        proc.waitForFinished(3000);
        QByteArray iconData = proc.readAllStandardOutput();
        if (!iconData.isEmpty()) {
            QPixmap pix;
            pix.loadFromData(iconData, "PNG");
            if (!pix.isNull()) return QIcon(pix);
        }
        // Try icon.svg
        proc.start("unzip", {"-p", appPath, "icon.svg"});
        proc.waitForFinished(3000);
        iconData = proc.readAllStandardOutput();
        if (!iconData.isEmpty()) {
            QPixmap pix;
            pix.loadFromData(iconData, "SVG");
            if (!pix.isNull()) return QIcon(pix);
        }
        return style()->standardIcon(QStyle::SP_FileIcon);
    }

    void launchDesktopFile(const QString &desktopPath) {
        QFile file(desktopPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Error", "Cannot open: " + desktopPath);
            return;
        }

        QString name, exec, icon;
        bool noDisplay = false;
        while (!file.atEnd()) {
            QString line = QString::fromLocal8Bit(file.readLine()).trimmed();
            if (line.startsWith("Name=") && name.isEmpty()) name = line.mid(5);
            else if (line.startsWith("Exec=")) exec = line.mid(5);
            else if (line.startsWith("Icon=")) icon = line.mid(5);
            else if (line.startsWith("NoDisplay=true")) noDisplay = true;
        }
        file.close();

        if (noDisplay) return;

        if (exec.isEmpty()) {
            QMessageBox::warning(this, "Error", "No Exec= found in " + (name.isEmpty() ? desktopPath : name));
            return;
        }

        // Remove field codes like %f, %F, %u, %U, etc.
        exec.remove(QRegularExpression(" %[fFdDnNmuvw]"));

        QStringList parts = exec.split(' ', Qt::SkipEmptyParts);
        if (!parts.isEmpty()) {
            QString cmd = parts.takeFirst();
            QProcess::startDetached(cmd, parts);
        }
    }

    QString parseDesktopName(const QString &desktopPath) {
        QFile file(desktopPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return QFileInfo(desktopPath).baseName();
        while (!file.atEnd()) {
            QString line = QString::fromLocal8Bit(file.readLine()).trimmed();
            if (line.startsWith("Name=") && !line.mid(5).isEmpty())
                return line.mid(5);
        }
        return QFileInfo(desktopPath).baseName();
    }

    QIcon parseDesktopIcon(const QString &desktopPath) {
        QFile file(desktopPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return style()->standardIcon(QStyle::SP_FileIcon);
        while (!file.atEnd()) {
            QString line = QString::fromLocal8Bit(file.readLine()).trimmed();
            if (line.startsWith("Icon=")) {
                QString iconName = line.mid(5);
                QIcon ic = QIcon::fromTheme(iconName);
                if (!ic.isNull()) return ic;
                if (QFileInfo(iconName).exists()) {
                    ic = QIcon(iconName);
                    if (!ic.isNull()) return ic;
                }
                break;
            }
        }
        return style()->standardIcon(QStyle::SP_FileIcon);
    }

    QString parseAppName(const QString &appPath) {
        QProcess proc;
        proc.start("unzip", {"-p", appPath, "manifest.json"});
        proc.waitForFinished(3000);
        QByteArray data = proc.readAllStandardOutput();
        if (data.isEmpty()) return QFileInfo(appPath).baseName();
        // Simple JSON parse: look for "name": "..."
        QRegularExpression re("\"name\"\\s*:\\s*\"([^\"]+)\"");
        QRegularExpressionMatch m = re.match(QString::fromUtf8(data));
        if (m.hasMatch()) return m.captured(1);
        return QFileInfo(appPath).baseName();
    }

    void renameDesktopItem(const QString &path) {
        QFileInfo fi(path);
        bool ok;
        QString newName = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal, fi.fileName(), &ok);
        if (ok && !newName.isEmpty()) {
            QString newPath = fi.absolutePath() + "/" + newName;
            QFile::rename(path, newPath);
            refreshDesktopIcons();
        }
    }

    void deleteDesktopItem(const QString &path) {
        if (QMessageBox::question(this, "Delete", "Delete \"" + QFileInfo(path).fileName() + "\"?") == QMessageBox::Yes) {
            QFileInfo fi(path);
            if (fi.isDir()) QDir(path).removeRecursively();
            else QFile::remove(path);
            refreshDesktopIcons();
        }
    }

    void showDesktopItemProperties(const QString &path) {
        QFileInfo fi(path);
        QString msg = "Name: " + fi.fileName()
            + "\nPath: " + fi.absoluteFilePath()
            + "\nSize: " + QString::number(fi.size()) + " bytes"
            + "\nType: " + (fi.isDir() ? "Directory" : "File")
            + "\nReadable: " + QString(fi.isReadable() ? "Yes" : "No")
            + "\nWritable: " + QString(fi.isWritable() ? "Yes" : "No")
            + "\nModified: " + fi.lastModified().toString();
        QMessageBox::information(this, "Properties", msg);
    }

    ManagedWindow *openWindow(const QString &title, QWidget *content) {
        ManagedWindow *w = new ManagedWindow(title, content, desktop);
        managedWindows.append(w);
        int ww = 800, wh = 500;
        if (title == "Terminal") { ww = 700; wh = 400; }
        if (title == "Settings") { ww = 500; wh = 400; }
        int x = (desktop->width() - ww) / 2 + (managedWindows.size() - 1) * 30;
        int y = (desktop->height() - wh) / 2 + (managedWindows.size() - 1) * 30;
        w->setGeometry(x, y, ww, wh);
        w->show(); w->raise();
        dock->setRunning(title, true);
        w->onCloseCallback = [this, w, title]() {
            managedWindows.removeOne(w); dock->setRunning(title, false);
        };
        return w;
    }

    DesktopSurface *desktop;
    QVBoxLayout *desktopLayout;
    QVBoxLayout *iconsFlowLayout;
    TopBar *topBar;
    Dock *dock;
    StartMenu *startMenu;
    QWidget *overlay;
    QList<ManagedWindow*> managedWindows;
    QTimer *refreshTimer;
    QMap<QWidget*, std::function<void()>> pendingActions;
};

// ============================================================
// Main
// ============================================================
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Parrot Desktop Environment");
    app.setOrganizationName("Parrot");
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(45, 45, 45));
    darkPalette.setColor(QPalette::WindowText, QColor(224, 224, 224));
    darkPalette.setColor(QPalette::Base, QColor(42, 42, 42));
    darkPalette.setColor(QPalette::AlternateBase, QColor(50, 50, 50));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(60, 60, 60));
    darkPalette.setColor(QPalette::ToolTipText, QColor(224, 224, 224));
    darkPalette.setColor(QPalette::Text, QColor(224, 224, 224));
    darkPalette.setColor(QPalette::Button, QColor(60, 60, 60));
    darkPalette.setColor(QPalette::ButtonText, QColor(224, 224, 224));
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(74, 111, 165));
    darkPalette.setColor(QPalette::Highlight, QColor(74, 111, 165));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(darkPalette);

    ParrotDE de;
    de.show();
    return app.exec();
}
