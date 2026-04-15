#include "DiskVisualWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFontMetrics>

DiskVisualWidget::DiskVisualWidget(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void DiskVisualWidget::setDisks(const QList<DiskInfo> &disks)
{
    m_disks = disks;
    updateGeometry();
    update();
}

void DiskVisualWidget::highlightRaid(const QString &raidDev)
{
    m_highlighted = raidDev;
    update();
}

QSize DiskVisualWidget::sizeHint() const
{
    int rows = m_disks.size();
    return QSize(400, rows * (ROW_H + VGAP) + 8);
}

QSize DiskVisualWidget::minimumSizeHint() const
{
    return QSize(200, 3 * (ROW_H + VGAP) + 8);
}

QColor DiskVisualWidget::colorForType(SegmentType t) const
{
    switch (t) {
        case SegmentType::Free:   return QColor("#D3D1C7");
        case SegmentType::System: return QColor("#888780");
        case SegmentType::Raid0:  return QColor("#378ADD");
        case SegmentType::Raid1:  return QColor("#1D9E75");
        case SegmentType::Raid5:  return QColor("#BA7517");
        case SegmentType::Raid6:  return QColor("#D85A30");
        case SegmentType::Raid10: return QColor("#534AB7");
        case SegmentType::Spare:  return QColor("#D4537E");
        case SegmentType::Failed: return QColor("#E24B4A");
    }
    return Qt::gray;
}

QColor DiskVisualWidget::textColorForType(SegmentType t) const
{
    switch (t) {
        case SegmentType::Free: return QColor("#5F5E5A");
        default:                return Qt::white;
    }
}

void DiskVisualWidget::buildRects(int w)
{
    m_segRects.clear();
    int barW = w - LABEL_W - SIZE_W - 8;
    for (int i = 0; i < m_disks.size(); i++) {
        const DiskInfo &d = m_disks[i];
        int y   = 4 + i * (ROW_H + VGAP);
        int x   = LABEL_W;
        for (const DiskSegment &s : d.segments) {
            int sw = qMax(1, (int)(barW * s.pct / 100.0));
            SegRect sr;
            sr.rect    = QRect(x, y, sw, ROW_H);
            sr.disk    = d.name;
            sr.raidDev = s.raidDev;
            sr.type    = s.type;
            m_segRects.append(sr);
            x += sw;
        }
    }
}

void DiskVisualWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    buildRects(width());

    QFont labelFont = font();
    labelFont.setPointSize(9);
    p.setFont(labelFont);

    for (int i = 0; i < m_disks.size(); i++) {
        const DiskInfo &d = m_disks[i];
        int y = 4 + i * (ROW_H + VGAP);

        // Disk label
        p.setPen(palette().text().color());
        p.drawText(QRect(0, y, LABEL_W - 4, ROW_H), Qt::AlignVCenter | Qt::AlignRight,
                   "/dev/" + d.name);

        // Size label
        QString sizeStr = d.sizeGB >= 1000
            ? QString::number(d.sizeGB / 1000) + "T"
            : QString::number(d.sizeGB) + "G";
        p.drawText(QRect(width() - SIZE_W, y, SIZE_W - 2, ROW_H),
                   Qt::AlignVCenter | Qt::AlignRight, sizeStr);

        // Hovered row background
        if (m_hoveredDisk == d.name) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0,0,0,15));
            int barW = width() - LABEL_W - SIZE_W - 8;
            p.drawRoundedRect(LABEL_W, y, barW, ROW_H, 3, 3);
        }
    }

    // Draw segments
    QFont segFont = font();
    segFont.setPointSize(8);
    p.setFont(segFont);

    for (const SegRect &sr : m_segRects) {
        QColor base = colorForType(sr.type);

        bool dimmed = !m_highlighted.isEmpty()
                   && sr.raidDev != m_highlighted
                   && sr.type != SegmentType::Free
                   && sr.type != SegmentType::System;
        if (dimmed) base.setAlphaF(0.35);

        // Fill
        p.setPen(Qt::NoPen);
        p.setBrush(base);
        p.drawRoundedRect(sr.rect.adjusted(0,0,-1,0), 3, 3);

        // Border
        p.setPen(QPen(base.darker(130), 0.5));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(sr.rect.adjusted(0,0,-1,0), 3, 3);

        // Label text (only if wide enough)
        if (sr.rect.width() > 30) {
            // Find matching segment label
            for (const DiskInfo &d : m_disks) {
                if (d.name == sr.disk) {
                    for (const DiskSegment &s : d.segments) {
                        if (s.raidDev == sr.raidDev && s.type == sr.type) {
                            p.setPen(dimmed ? QColor(80,80,80) : textColorForType(sr.type));
                            p.drawText(sr.rect.adjusted(4,0,-4,0),
                                       Qt::AlignVCenter | Qt::AlignLeft,
                                       p.fontMetrics().elidedText(s.label, Qt::ElideRight,
                                                                   sr.rect.width() - 8));
                            break;
                        }
                    }
                    break;
                }
            }
        }

        // Highlight outline for selected RAID
        if (!m_highlighted.isEmpty() && sr.raidDev == m_highlighted) {
            p.setPen(QPen(QColor("#185FA5"), 2));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(sr.rect.adjusted(1,1,-2,-1), 3, 3);
        }
    }
}

void DiskVisualWidget::mousePressEvent(QMouseEvent *e)
{
    for (const SegRect &sr : m_segRects) {
        if (sr.rect.contains(e->pos())) {
            emit diskClicked(sr.disk);
            if (!sr.raidDev.isEmpty())
                emit segmentClicked(sr.disk, sr.raidDev);
            return;
        }
    }
}

void DiskVisualWidget::mouseMoveEvent(QMouseEvent *e)
{
    for (const SegRect &sr : m_segRects) {
        if (sr.rect.contains(e->pos())) {
            if (m_hoveredDisk != sr.disk) {
                m_hoveredDisk = sr.disk;
                update();
            }
            return;
        }
    }
    if (!m_hoveredDisk.isEmpty()) {
        m_hoveredDisk.clear();
        update();
    }
}
