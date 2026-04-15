#pragma once
#include <QTableWidget>
#include "Models.h"

class RaidTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    explicit RaidTableWidget(QWidget *parent = nullptr);
    void setRaids(const QList<RaidInfo> &raids);
    int selectedRaidId() const;   // -1 if none

signals:
    void raidSelected(const QString &dev);

private slots:
    void onSelectionChanged();

private:
    QList<RaidInfo> m_raids;
    void updateRow(int row, const RaidInfo &r);
};
