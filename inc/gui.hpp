#ifndef GUI_HPP
#define GUI_HPP

#include <QDebug>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPointer>
#include <QTimer>
#include <QSignalMapper>
#include <QDateTime>

class NotificationBarWidget : public QWidget
{
    Q_OBJECT

signals:

public:

    struct Item
    {
        QString key;
        QLabel *lbl;
        QPointer<QTimer> tmr;
    };

    NotificationBarWidget(QWidget *parent = 0);

    void
    setColor(const QString &color);

public slots:

    void
    showNotification(QString key, const QString &text, int seconds = 0);

    void
    hideNotification(const QString &key);

private slots:

private:

    QLabel*
    createNotificationLabel(const QString &text);

    QMap<QString, Item>
    m_items;

    QVBoxLayout
    *m_vbox;

    QString
    m_css_color;

};

#endif
