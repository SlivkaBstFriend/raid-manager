#include "MainWindow.h"
#include "CreateRaidDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QInputDialog>
#include <QPlainTextEdit>
#include <QFont>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QProgressDialog>
#include <QLabel>
#include <QScrollArea>
#include <QSplitter>
#include <QGroupBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("RAID Manager");
    resize(900, 600);

    m_backend = new MdadmBackend(this);

    connect(m_backend, &MdadmBackend::refreshDone,      this, &MainWindow::onRefreshDone);
    connect(m_backend, &MdadmBackend::commandStarted,   this, &MainWindow::onCommandStarted);
    connect(m_backend, &MdadmBackend::commandFinished,  this, &MainWindow::onCommandFinished);
    connect(m_backend, &MdadmBackend::allCommandsDone,  this, &MainWindow::onAllDone);

    setupMenuBar();
    setupToolBar();
    setupUi();

    m_backend->refresh();
    statusBar()->showMessage("Сканирование дисков...");
}

// ── UI setup ─────────────────────────────────────────────────────────────────

void MainWindow::setupMenuBar()
{
    auto *raid = menuBar()->addMenu("RAID-массив");
    raid->addAction("Создать массив...", QKeySequence("Ctrl+N"), this, &MainWindow::actionCreateRaid);
    m_actStop  = raid->addAction("Остановить массив", this, &MainWindow::actionStopArray);
    m_actStart = raid->addAction("Запустить массив",   this, &MainWindow::actionStartArray);
    m_actDelete = raid->addAction("Удалить массив",  this, &MainWindow::actionDeleteArray,
                                   QKeySequence::Delete);
    raid->addSeparator();
    m_actAddSpare   = raid->addAction("Добавить запасной диск...", this, &MainWindow::actionAddSpare);
    m_actRemoveDisk = raid->addAction("Извлечь диск...",           this, &MainWindow::actionRemoveDisk);
    raid->addSeparator();
    m_actDetail = raid->addAction("Подробности...", QKeySequence("Ctrl+I"), this, &MainWindow::actionDetail);

    auto *view = menuBar()->addMenu("Вид");
    view->addAction("Обновить", this, [this]{ m_backend->refresh(); },
                    QKeySequence::Refresh);

    auto *tools = menuBar()->addMenu("Инструменты");
    tools->addAction("Ввести команду mdadm вручную...", this, &MainWindow::actionManualCommand);

    updateActions();
}

void MainWindow::setupToolBar()
{
    auto *tb = addToolBar("Основная");
    tb->setMovable(false);
    tb->setIconSize(QSize(16,16));

    tb->addAction("+ Создать RAID", this, &MainWindow::actionCreateRaid);
    tb->addSeparator();
    m_actStop       = tb->addAction("Остановить",       this, &MainWindow::actionStopArray);
    m_actStart      = tb->addAction("Запустить",        this, &MainWindow::actionStartArray);
    m_actDelete     = tb->addAction("Удалить массив",    this, &MainWindow::actionDeleteArray);
    tb->addSeparator();
    m_actAddSpare   = tb->addAction("Доб. запасной",    this, &MainWindow::actionAddSpare);
    m_actRemoveDisk = tb->addAction("Извлечь диск",     this, &MainWindow::actionRemoveDisk);
    tb->addSeparator();
    m_actDetail     = tb->addAction("Подробности",      this, &MainWindow::actionDetail);
    tb->addSeparator();
    tb->addAction("Обновить", this, [this]{ m_backend->refresh(); });

    updateActions();
}

void MainWindow::setupUi()
{
    auto *central = new QWidget;
    setCentralWidget(central);
    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0,0,0,0);
    rootLayout->setSpacing(0);

    // Left: disk map + table
    auto *leftWidget = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0,0,0,0);
    leftLayout->setSpacing(0);

    // Вертикальный сплиттер между картой дисков и таблицей
    auto *vSplitter = new QSplitter(Qt::Vertical);
    vSplitter->setChildrenCollapsible(false);

    // Disk visual area (scrollable)
    m_diskMap = new DiskVisualWidget;
    auto *scroll = new QScrollArea;
    scroll->setWidget(m_diskMap);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *diskGroup = new QGroupBox("Блочные устройства");
    auto *dgl = new QVBoxLayout(diskGroup);
    dgl->setContentsMargins(8,4,8,8);

    // Legend
    auto *legend = new QWidget;
    auto *ll = new QHBoxLayout(legend);
    ll->setContentsMargins(0,0,0,0);
    ll->setSpacing(12);
    auto addLeg = [&](const QString &color, const QString &label){
        auto *dot = new QLabel;
        dot->setFixedSize(12,12);
        dot->setStyleSheet(QString("background:%1;border-radius:2px;").arg(color));
        auto *lbl = new QLabel(label);
        lbl->setStyleSheet("font-size:11px; color:gray;");
        ll->addWidget(dot); ll->addWidget(lbl);
    };
    addLeg("#D3D1C7","Свободно");
    addLeg("#888780","Система");
    addLeg("#378ADD","RAID 0");
    addLeg("#1D9E75","RAID 1");
    addLeg("#BA7517","RAID 5");
    addLeg("#D85A30","RAID 6");
    addLeg("#D4537E","Запасной");
    addLeg("#E24B4A","Сбой");
    ll->addStretch();

    dgl->addWidget(legend);
    dgl->addWidget(scroll);
    // RAID table
    auto *raidGroup = new QGroupBox("RAID-массивы");
    auto *rgl = new QVBoxLayout(raidGroup);
    rgl->setContentsMargins(8,4,8,8);
    m_raidTable = new RaidTableWidget;
    rgl->addWidget(m_raidTable);

    vSplitter->addWidget(diskGroup);
    vSplitter->addWidget(raidGroup);
    vSplitter->setStretchFactor(0, 1);
    vSplitter->setStretchFactor(1, 2);
    leftLayout->addWidget(vSplitter, 1);

    // Right: queue panel
    m_queue = new QueuePanel;
    m_queue->setMinimumWidth(260);
    m_queue->setMaximumWidth(320);
    m_queue->setStyleSheet("border-left:1px solid palette(midlight);");

    auto *hSplitter = new QSplitter(Qt::Horizontal);
    hSplitter->setChildrenCollapsible(false);
    hSplitter->addWidget(leftWidget);
    hSplitter->addWidget(m_queue);
    hSplitter->setStretchFactor(0, 3);
    hSplitter->setStretchFactor(1, 1);
    rootLayout->addWidget(hSplitter);

    // Connections
    connect(m_diskMap, &DiskVisualWidget::segmentClicked,
            this,      &MainWindow::onSegmentClicked);
    connect(m_raidTable, &RaidTableWidget::raidSelected,
            this,        &MainWindow::onRaidSelected);
    connect(m_queue, &QueuePanel::applyRequested, this, &MainWindow::onApplyRequested);
}

// ── Data refresh ──────────────────────────────────────────────────────────────

void MainWindow::onRefreshDone()
{
    m_diskMap->setDisks(m_backend->disks());
    m_raidTable->setRaids(m_backend->raids());

    if (m_selectedRaidId >= 0)
        m_diskMap->highlightRaid(raidDevById());

    // Обновляем статусбар только если ничего не выбрано
    if (m_selectedRaidId < 0) {
        statusBar()->showMessage(
            QString("Дисков: %1   Массивов: %2   Последнее обновление: сейчас")
            .arg(m_backend->disks().size())
            .arg(m_backend->raids().size()));
    }
}

QString MainWindow::raidDevById()
{
    return m_selectedRaidDev;
}

void MainWindow::onRaidSelected(const QString &dev)
{
    m_selectedRaidDev = dev;
    for (const RaidInfo &r : m_backend->raids()) {
        if (r.dev == dev) {
            m_selectedRaidId = r.id;
            m_diskMap->highlightRaid(r.dev);
            QString st;
            switch(r.status) {
                case RaidStatus::Active:   st="Активен"; break;
                case RaidStatus::Stopped:  st="Остановлен"; break;
                case RaidStatus::Degraded: st="Деградация"; break;
                case RaidStatus::Failed:   st="Сбой"; break;
                case RaidStatus::Syncing:  st="Синхронизация"; break;
            }
            statusBar()->showMessage(
                QString("/dev/%1  RAID %2  %3  [%4]  Диски: %5")
                .arg(r.dev).arg(r.level).arg(r.size).arg(st)
                .arg(r.disks.join(", ")));
            break;
        }
    }
    updateActions();
}

void MainWindow::onSegmentClicked(const QString &, const QString &raidDev)
{
    onRaidSelected(raidDev);
}

// ── Actions ───────────────────────────────────────────────────────────────────

RaidInfo *MainWindow::selectedRaid()
{
    if (m_selectedRaidDev.isEmpty()) return nullptr;
    for (RaidInfo &r : const_cast<QList<RaidInfo>&>(m_backend->raids()))
        if (r.dev == m_selectedRaidDev) return &r;
    return nullptr;
}

void MainWindow::updateActions()
{
    RaidInfo *r = selectedRaid();
    bool sel     = (r != nullptr);
    bool stopped = sel && (r->status == RaidStatus::Stopped);
    bool active  = sel && !stopped;

    // RAID 0 не поддерживает запасные диски и горячее извлечение
    bool supportsSpare = active && r && (r->level != 0);

    m_actStop->setEnabled(active);
    m_actStart->setEnabled(stopped);
    m_actDelete->setEnabled(sel);
    m_actAddSpare->setEnabled(supportsSpare);
    m_actRemoveDisk->setEnabled(active);
    m_actDetail->setEnabled(sel);
}

void MainWindow::actionCreateRaid()
{
    CreateRaidDialog dlg(m_backend->disks(), m_backend->raids(), this);
    if (dlg.exec() == QDialog::Accepted)
        enqueue(dlg.buildCommands());
}

void MainWindow::actionStopArray()
{
    RaidInfo *r = selectedRaid(); if (!r) return;
    if (r->status == RaidStatus::Stopped) {
        QMessageBox::information(this, "Массив остановлен",
            QString("/dev/%1 уже остановлен.\nИспользуйте 'Запустить' для повторной сборки.").arg(r->dev));
        return;
    }
    auto ans = QMessageBox::question(this, "Остановить массив",
        QString("Остановить /dev/%1?\nМассив можно будет запустить снова.").arg(r->dev));
    if (ans == QMessageBox::Yes)
        enqueue({"mdadm --stop /dev/" + r->dev});
}

void MainWindow::actionStartArray()
{
    RaidInfo *r = selectedRaid(); if (!r) return;
    if (r->status != RaidStatus::Stopped) {
        QMessageBox::information(this, "Массив активен",
            QString("/dev/%1 уже запущен.").arg(r->dev));
        return;
    }
    // Собираем команду — диски передаются как отдельные аргументы через пробел
    // runNextInQueue сам разобьёт строку по пробелам
    QStringList args;
    args << "mdadm" << "--assemble" << ("/dev/" + r->dev);
    for (const QString &disk : r->disks)
        args << disk;
    enqueue({args.join(" ")});
}

void MainWindow::actionDeleteArray()
{
    RaidInfo *r = selectedRaid(); if (!r) return;
    auto ans = QMessageBox::warning(this,
        "Delete array",
        QString("Delete /dev/%1 and wipe metadata?").arg(r->dev),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ans != QMessageBox::Yes) return;

    QStringList cmds;
    if (r->status != RaidStatus::Stopped)
        cmds << "mdadm --stop /dev/" + r->dev;
    for (const QString &d : r->disks)
        cmds << "mdadm --zero-superblock --force " + d;

    m_devToDelete = r->dev;
    enqueue(cmds);
}

void MainWindow::actionAddSpare()
{
    RaidInfo *r = selectedRaid(); if (!r) return;
    if (r->level == 0) {
        QMessageBox::information(this, "Not supported",
            QString("RAID 0 does not support spare disks.\n"
                    "Use RAID 1, 5, 6 or 10 for redundancy."));
        return;
    }
    if (r->level == 0) {
        QMessageBox::information(this, "Not supported",
            QString("RAID 0 does not support spare disks.\n"
                    "Use RAID 1, 5, 6 or 10 for redundancy."));
        return;
    }

    // Collect free disks
    QStringList free;
    for (const DiskInfo &d : m_backend->disks())
        for (const DiskSegment &s : d.segments)
            if (s.type == SegmentType::Free) { free << "/dev/"+d.name; break; }

    if (free.isEmpty()) {
        QMessageBox::information(this,"Нет свободных дисков","Все диски заняты.");
        return;
    }

    bool ok;
    QString chosen = QInputDialog::getItem(this,
        "Добавить запасной диск",
        QString("Диск для /dev/%1:").arg(r->dev),
        free, 0, false, &ok);
    if (ok && !chosen.isEmpty())
        enqueue({"mdadm /dev/" + r->dev + " --add " + chosen});
}

void MainWindow::actionRemoveDisk()
{
    RaidInfo *r = selectedRaid(); if (!r) return;

    if (r->level == 0) {
        QMessageBox::warning(this, "Not supported",
            "RAID 0 does not support hot disk removal.\n"
            "To change disks you must delete and recreate the array.");
        return;
    }

    bool ok;
    QString chosen = QInputDialog::getItem(this,
        "Remove disk",
        QString("Disk to remove from /dev/%1:").arg(r->dev),
        r->disks, 0, false, &ok);
    if (!ok || chosen.isEmpty()) return;

    // Правильная последовательность: --fail затем --remove
    // mdadm сам выведет диск из массива между командами
    enqueue({
        "mdadm /dev/" + r->dev + " --fail " + chosen,
        "mdadm /dev/" + r->dev + " --remove " + chosen
    });
}

void MainWindow::actionDetail()
{
    RaidInfo *r = selectedRaid(); if (!r) return;

    // Сначала пробуем без root
    QProcess p;
    p.start("mdadm", QStringList() << "--detail" << "/dev/" + r->dev);
    p.waitForFinished(5000);
    QString out = p.readAllStandardOutput();

    // Если пусто — пробуем через pkexec
    if (out.trimmed().isEmpty()) {
        QProcess p2;
        p2.start("pkexec", QStringList() << "mdadm" << "--detail" << "/dev/" + r->dev);
        p2.waitForFinished(10000);
        out = p2.readAllStandardOutput();
        QString err = p2.readAllStandardError();
        if (out.isEmpty()) out = err;
    }

    if (out.trimmed().isEmpty())
        out = QString("Could not get details for /dev/%1\n"
                      "Try running as root.").arg(r->dev);

    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle(QString("Details: /dev/%1").arg(r->dev));
    dlg->resize(600, 450);
    auto *lay = new QVBoxLayout(dlg);
    auto *te = new QPlainTextEdit(out);
    te->setReadOnly(true);
    te->setFont(QFont("monospace", 10));
    lay->addWidget(te);
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(bb, &QDialogButtonBox::rejected, dlg, &QDialog::accept);
    lay->addWidget(bb);
    dlg->exec();
    delete dlg;
}

void MainWindow::actionManualCommand()
{
    bool ok;
    QString cmd = QInputDialog::getText(this,
        "Ввести команду mdadm",
        "Команда (без sudo — будет выполнена через pkexec):",
        QLineEdit::Normal, "mdadm ", &ok);
    if (ok && !cmd.trimmed().isEmpty())
        enqueue({cmd.trimmed()});
}

void MainWindow::enqueue(const QStringList &cmds)
{
    for (const QString &c : cmds)
        m_queue->addCommand(c);
}

// ── Apply queue ───────────────────────────────────────────────────────────────

void MainWindow::onApplyRequested()
{
    if (m_queue->isEmpty()) return;

    QStringList cmds = m_queue->commands();

    // Confirmation dialog
    QString list;
    for (int i=0; i<cmds.size(); i++)
        list += QString("%1. %2\n").arg(i+1).arg(cmds[i]);

    auto ans = QMessageBox::warning(this, "Применить изменения",
        QString("Будут выполнены следующие команды от имени root:\n\n%1\n"
                "Операции с дисками необратимы. Продолжить?").arg(list),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ans != QMessageBox::Yes) return;

    m_progress = new QProgressDialog("Выполнение команд...", QString(), 0, cmds.size(), this);
    m_progress->setWindowModality(Qt::WindowModal);
    m_progress->setMinimumDuration(0);
    m_progress->setValue(0);

    m_queue->clear();
    m_backend->applyQueue(cmds);
}

void MainWindow::onCommandStarted(int idx, const QString &cmd)
{
    if (m_progress) {
        m_progress->setValue(idx);
        m_progress->setLabelText(QString("Выполняется (%1):\n%2").arg(idx+1).arg(cmd));
    }
    statusBar()->showMessage(QString("[%1] %2").arg(idx+1).arg(cmd));
}

void MainWindow::onCommandFinished(int idx, bool ok, const QString &out)
{
    Q_UNUSED(idx); Q_UNUSED(out);
    if (!ok) {
        QMessageBox::critical(this, "Ошибка команды",
            QString("Команда завершилась с ошибкой:\n\n%1").arg(out));
    }
}

void MainWindow::onAllDone(bool anyError)
{
    if (m_progress) { m_progress->close(); delete m_progress; m_progress=nullptr; }

    if (!m_devToDelete.isEmpty()) {
        m_backend->removeStoppedRaid(m_devToDelete);
        if (m_selectedRaidDev == m_devToDelete)
            m_selectedRaidDev.clear();
        m_devToDelete.clear();
        updateActions();
    }

    if (anyError)
        statusBar()->showMessage("Done with errors.");
    else
        statusBar()->showMessage("All operations completed successfully.");
}
