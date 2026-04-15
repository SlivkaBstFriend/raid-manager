#include "QueuePanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidgetItem>
#include <QFont>

QueuePanel::QueuePanel(QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);

    // Header
    auto *header = new QLabel("Очередь операций");
    header->setStyleSheet("QLabel { font-size:11px; font-weight:500; "
                          "color: palette(mid); text-transform:uppercase; "
                          "padding:8px 12px 6px; "
                          "border-bottom:1px solid palette(midlight); }");
    root->addWidget(header);

    // List
    m_list = new QListWidget;
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setSpacing(2);
    m_list->setStyleSheet(
        "QListWidget { background: transparent; padding:4px; }"
        "QListWidget::item { background: palette(window); "
        "  border: 1px solid palette(midlight); border-radius:4px;"
        "  padding:5px 8px; font-family: monospace; font-size:11px; }"
        "QListWidget::item:selected { background:#E6F1FB; border-color:#378ADD; }");
    root->addWidget(m_list, 1);

    m_emptyLabel = new QLabel("Очередь пуста.\nВыберите операцию — \nона появится здесь.");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("QLabel { color:palette(mid); font-size:12px; padding:20px; }");
    root->addWidget(m_emptyLabel);

    // Buttons
    auto *btnRow = new QHBoxLayout;
    btnRow->setContentsMargins(8,6,8,8);
    btnRow->setSpacing(6);
    m_clearBtn = new QPushButton("Очистить");
    m_applyBtn = new QPushButton("Применить");
    m_applyBtn->setStyleSheet(
        "QPushButton { background:#185FA5; color:white; border:none;"
        " border-radius:4px; padding:5px 12px; font-weight:500; }"
        "QPushButton:hover { background:#0C447C; }"
        "QPushButton:disabled { background:palette(midlight); color:palette(mid); }");
    btnRow->addWidget(m_clearBtn);
    btnRow->addWidget(m_applyBtn);
    root->addLayout(btnRow);

    connect(m_clearBtn, &QPushButton::clicked, this, [this]{
        clear(); emit cleared();
    });
    connect(m_applyBtn, &QPushButton::clicked, this, &QueuePanel::applyRequested);

    rebuild();
}

void QueuePanel::addCommand(const QString &cmd)
{
    m_commands.append(cmd);
    rebuild();
}

void QueuePanel::clear()
{
    m_commands.clear();
    rebuild();
}

void QueuePanel::rebuild()
{
    m_list->clear();
    bool empty = m_commands.isEmpty();
    m_emptyLabel->setVisible(empty);
    m_list->setVisible(!empty);
    m_applyBtn->setEnabled(!empty);
    m_clearBtn->setEnabled(!empty);

    for (int i = 0; i < m_commands.size(); i++) {
        QString text = QString("%1. %2").arg(i+1).arg(m_commands[i]);
        auto *item = new QListWidgetItem(text);
        item->setToolTip(m_commands[i]);
        m_list->addItem(item);
    }
}
