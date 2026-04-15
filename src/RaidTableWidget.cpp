#include "RaidTableWidget.h"
#include <QHeaderView>
#include <QLabel>
#include <QHBoxLayout>
#include <QHBoxLayout>
#include <QColor>

RaidTableWidget::RaidTableWidget(QWidget *parent) : QTableWidget(parent)
{
    setColumnCount(6);
    setHorizontalHeaderLabels({"Устройство","Уровень","Состояние","Размер","Диски","Синхр."});
    horizontalHeader()->setStretchLastSection(false);
    horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    verticalHeader()->setVisible(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setAlternatingRowColors(false);
    setShowGrid(true);
    setGridStyle(Qt::SolidLine);

    connect(this, &QTableWidget::itemSelectionChanged,
            this, &RaidTableWidget::onSelectionChanged);
}

static QWidget *makeBadge(const QString &text, const QString &bg, const QString &fg)
{
    auto *lbl = new QLabel(text);
    lbl->setStyleSheet(QString(
        "QLabel { background:%1; color:%2; border-radius:4px;"
        " padding:2px 8px; font-size:11px; font-weight:500; }").arg(bg, fg));
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    auto *w = new QWidget;
    auto *lay = new QHBoxLayout(w);
    lay->setContentsMargins(4,3,4,3);
    lay->addWidget(lbl);
    lay->addStretch();
    return w;
}

void RaidTableWidget::updateRow(int row, const RaidInfo &r)
{
    auto item = [](const QString &t) {
        auto *i = new QTableWidgetItem(t);
        i->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        return i;
    };

    setItem(row, 0, item("/dev/" + r.dev));
    setItem(row, 1, item(QString("RAID %1").arg(r.level)));

    // Status badge widget
    QString bg, fg, label;
    switch (r.status) {
        case RaidStatus::Active:
            bg="#EAF3DE"; fg="#3B6D11"; label="Активен"; break;
        case RaidStatus::Degraded:
            bg="#FAEEDA"; fg="#854F0B"; label="Деградация"; break;
        case RaidStatus::Failed:
            bg="#FCEBEB"; fg="#A32D2D"; label="Сбой"; break;
        case RaidStatus::Syncing:
            bg="#E6F1FB"; fg="#185FA5"; label="Синхронизация"; break;
        case RaidStatus::Stopped:
            bg="#F1EFE8"; fg="#5F5E5A"; label="Остановлен"; break;
    }
    setCellWidget(row, 2, makeBadge(label, bg, fg));

    setItem(row, 3, item(r.size));
    setItem(row, 4, item(r.disks.join(", ")));
    setItem(row, 5, item(r.syncProgress));

    setRowHeight(row, 32);
}

void RaidTableWidget::setRaids(const QList<RaidInfo> &raids)
{
    m_raids = raids;
    setRowCount(raids.size());
    for (int i = 0; i < raids.size(); i++)
        updateRow(i, raids[i]);
    resizeColumnsToContents();
    horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
}

int RaidTableWidget::selectedRaidId() const
{
    auto rows = selectedItems();
    if (rows.isEmpty()) return -1;
    int row = rows.first()->row();
    if (row < 0 || row >= m_raids.size()) return -1;
    return m_raids[row].id;
}

void RaidTableWidget::onSelectionChanged()
{
    auto rows = selectedItems();
    if (rows.isEmpty()) return;
    int row = rows.first()->row();
    if (row < 0 || row >= m_raids.size()) return;
    emit raidSelected(m_raids[row].dev);
}

