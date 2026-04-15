#include "CreateRaidDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QMessageBox>
#include <QCheckBox>

CreateRaidDialog::CreateRaidDialog(const QList<DiskInfo> &disks,
                                   const QList<RaidInfo> &raids,
                                   QWidget *parent)
    : QDialog(parent), m_disks(disks), m_raids(raids)
{
    setWindowTitle("Создать RAID-массив");
    setMinimumWidth(480);

    auto *root = new QVBoxLayout(this);

    // ── Form ──
    auto *form = new QFormLayout;
    form->setSpacing(10);

    m_devCombo = new QComboBox;
    for (int n = 0; n < 10; n++) {
        QString dev = QString("/dev/md%1").arg(n);
        bool used = false;
        for (const RaidInfo &r : raids)
            if ("/dev/"+r.dev == dev) { used=true; break; }
        if (!used) m_devCombo->addItem(dev);
    }
    form->addRow("Устройство:", m_devCombo);

    m_levelCombo = new QComboBox;
    m_levelCombo->addItem("RAID 0  — чередование (скорость, нет защиты)", 0);
    m_levelCombo->addItem("RAID 1  — зеркало (2+ дисков)",                  1);
    m_levelCombo->addItem("RAID 5  — чётность (3+ дисков)",                  5);
    m_levelCombo->addItem("RAID 6  — двойная чётность (4+ дисков)",          6);
    m_levelCombo->addItem("RAID 10 — зеркало + чередование (4+ дисков)",    10);
    m_levelCombo->setCurrentIndex(1);
    form->addRow("Уровень RAID:", m_levelCombo);

    m_minDisksLabel = new QLabel;
    m_minDisksLabel->setStyleSheet("color:gray; font-size:11px;");
    form->addRow("", m_minDisksLabel);

    // Chunk size (not relevant for RAID1)
    m_chunkCombo = new QComboBox;
    for (auto s : {"64","128","256","512"}) m_chunkCombo->addItem(QString(s)+" КБ", QString(s));
    form->addRow("Chunk-size:", m_chunkCombo);

    m_spareSpin = new QSpinBox;
    m_spareSpin->setRange(0, 4);
    form->addRow("Запасных дисков:", m_spareSpin);

    root->addLayout(form);

    // ── Disk list ──
    auto *grp = new QGroupBox("Выберите диски (только свободные)");
    auto *gl  = new QVBoxLayout(grp);
    m_diskList = new QListWidget;
    m_diskList->setSelectionMode(QAbstractItemView::MultiSelection);
    m_diskList->setMaximumHeight(160);

    for (const DiskInfo &d : disks) {
        bool hasFree = false;
        for (const DiskSegment &s : d.segments)
            if (s.type == SegmentType::Free) { hasFree=true; break; }

        QString label = QString("/dev/%1  (%2 ГБ)").arg(d.name).arg(d.sizeGB);
        auto *item = new QListWidgetItem(label);
        if (!hasFree) {
            item->setFlags(Qt::NoItemFlags);
            item->setForeground(Qt::gray);
            item->setText(label + "  [занят]");
        }
        m_diskList->addItem(item);
    }
    gl->addWidget(m_diskList);
    root->addWidget(grp);

    // ── Preview ──
    auto *prevGrp = new QGroupBox("Итоговая команда");
    auto *pl = new QVBoxLayout(prevGrp);
    m_preview = new QPlainTextEdit;
    m_preview->setReadOnly(true);
    m_preview->setMaximumHeight(90);
    m_preview->setFont(QFont("monospace", 10));
    pl->addWidget(m_preview);
    root->addWidget(prevGrp);

    // ── Buttons ──
    auto *bbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    bbox->button(QDialogButtonBox::Ok)->setText("В очередь");
    connect(bbox, &QDialogButtonBox::accepted, this, [this]{
        if (selectedDisks().isEmpty()) {
            QMessageBox::warning(this,"Ошибка","Выберите хотя бы один диск.");
            return;
        }
        int need = minDisks(m_levelCombo->currentData().toInt());
        if (selectedDisks().size() < need) {
            QMessageBox::warning(this,"Ошибка",
                QString("Для RAID %1 нужно минимум %2 диска(ов).")
                .arg(m_levelCombo->currentData().toInt()).arg(need));
            return;
        }
        accept();
    });
    connect(bbox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(bbox);

    // Connect signals for live preview
    connect(m_levelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CreateRaidDialog::updatePreview);
    connect(m_devCombo,   QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CreateRaidDialog::updatePreview);
    connect(m_spareSpin,  QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CreateRaidDialog::updatePreview);
    connect(m_chunkCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CreateRaidDialog::updatePreview);
    connect(m_diskList,   &QListWidget::itemSelectionChanged,
            this, &CreateRaidDialog::updatePreview);

    updatePreview();
}

QStringList CreateRaidDialog::selectedDisks() const
{
    QStringList result;
    for (auto *item : m_diskList->selectedItems()) {
        QString name = item->text().split(' ').first(); // "/dev/sdX"
        result << name;
    }
    return result;
}

int CreateRaidDialog::minDisks(int level) const
{
    switch(level) {
        case 0: return 2;
        case 1: return 2;
        case 5: return 3;
        case 6: return 4;
        case 10:return 4;
    }
    return 2;
}

void CreateRaidDialog::updatePreview()
{
    int level = m_levelCombo->currentData().toInt();
    int need  = minDisks(level);
    m_minDisksLabel->setText(QString("Минимум %1 диска(ов)").arg(need));

    // Hide chunk for RAID1
    m_chunkCombo->setEnabled(level != 1);

    QStringList sel = selectedDisks();
    if (sel.isEmpty()) {
        m_preview->setPlainText("(выберите диски)");
        return;
    }

    QString dev    = m_devCombo->currentText();
    int     n      = sel.size();
    int     spares = m_spareSpin->value();
    QString chunk  = m_chunkCombo->currentData().toString();

    QString cmd = QString("mdadm --create %1 --level=%2 --raid-devices=%3 --run")
                  .arg(dev).arg(level).arg(n);
    if (spares > 0) cmd += QString(" --spare-devices=%1").arg(spares);
    if (level != 1) cmd += QString(" --chunk=%1").arg(chunk);
    cmd += " " + sel.join(" ");

    // Additional post-create commands shown in preview
    QString full = cmd + "\n"
        + "mdadm --detail --scan >> /etc/mdadm/mdadm.conf\n"
        + "update-initramfs -u";
    m_preview->setPlainText(full);
}

QStringList CreateRaidDialog::buildCommands() const
{
    int level = m_levelCombo->currentData().toInt();
    QString dev = m_devCombo->currentText();
    QStringList sel = selectedDisks();
    int spares = m_spareSpin->value();
    QString chunk = m_chunkCombo->currentData().toString();

    QString cmd = QString("mdadm --create %1 --level=%2 --raid-devices=%3 --run")
                  .arg(dev).arg(level).arg(sel.size());
    if (spares > 0) cmd += QString(" --spare-devices=%1").arg(spares);
    if (level != 1) cmd += QString(" --chunk=%1").arg(chunk);
    cmd += " " + sel.join(" ");

    return {
        cmd,
        "mdadm-save-conf"
    };
}
