#pragma once
#include <QWidget>
#include <QStringList>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>

class QueuePanel : public QWidget
{
    Q_OBJECT
public:
    explicit QueuePanel(QWidget *parent = nullptr);

    void addCommand(const QString &cmd);
    void clear();
    QStringList commands() const { return m_commands; }
    bool isEmpty() const { return m_commands.isEmpty(); }

signals:
    void applyRequested();
    void cleared();

private:
    QStringList  m_commands;
    QListWidget *m_list;
    QPushButton *m_applyBtn;
    QPushButton *m_clearBtn;
    QLabel      *m_emptyLabel;

    void rebuild();
};
