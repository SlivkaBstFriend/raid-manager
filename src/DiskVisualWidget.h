#pragma once
#include <QWidget>
#include "Models.h"

// Renders a visual disk map similar to Windows Disk Manager:
// each disk is a horizontal bar divided into coloured segments.

class DiskVisualWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DiskVisualWidget(QWidget *parent = nullptr);

    void setDisks(const QList<DiskInfo> &disks);
    void highlightRaid(const QString &raidDev);  // highlight all segs belonging to a RAID

signals:
    void diskClicked(const QString &diskName);
    void segmentClicked(const QString &diskName, const QString &raidDev);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private:
    struct SegRect { QRect rect; QString disk; QString raidDev; SegmentType type; };

    QList<DiskInfo> m_disks;
    QList<SegRect>  m_segRects;  // built during paint, used for hit-testing
    QString         m_highlighted;
    QString         m_hoveredDisk;

    QColor colorForType(SegmentType t) const;
    QColor textColorForType(SegmentType t) const;
    void buildRects(int w);
    static constexpr int ROW_H   = 38;
    static constexpr int LABEL_W = 60;
    static constexpr int SIZE_W  = 48;
    static constexpr int VGAP    = 6;
};
