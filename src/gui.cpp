#include "gui.hpp"

NotificationBarWidget::NotificationBarWidget(QWidget *parent)
                     : QWidget(parent)
{
    m_vbox = new QVBoxLayout;
    setLayout(m_vbox);

    //Try to prevent this notification area from being stretched vertically
    //This would not be useful, it would essentially break the layout
    //with notification bars using up most of the vertical space
    //blocking the real content...
    setSizePolicy(sizePolicy().horizontalPolicy(), QSizePolicy::Fixed);
}

void
NotificationBarWidget::setColor(const QString &color)
{
    //Feel free to inject CSS
    m_css_color = color;
}

void
NotificationBarWidget::showNotification(QString key, const QString &text, int seconds)
{
    //Default to random number, allow blank to be used for unique notifications
    if (key.isEmpty())
        key = QString::number(QDateTime::currentMSecsSinceEpoch());

    QLabel *lbl = 0;
    if (!m_items.contains(key))
    {
        //Create item with new notification label
        lbl = createNotificationLabel(text);
        Item item;
        item.lbl = lbl;
        if (!item.tmr)
        {
            QTimer *tmr = new QTimer(lbl);
            QSignalMapper *mapper = new QSignalMapper(tmr);
            mapper->setMapping(tmr, key);
            connect(tmr, SIGNAL(timeout()), mapper, SLOT(map()));
            connect(mapper, SIGNAL(mappedString(const QString&)),
                SLOT(hideNotification(const QString&)));
            item.tmr = tmr;
        }
        //Add item to list
        m_items[key] = item;
        //Add label to layout
        m_vbox->addWidget(lbl);
    }
    else
    {
        //Update notification text
        lbl = m_items[key].lbl;
        lbl->setText(text);
    }
    Item item = m_items[key];

    lbl->show();

    if (seconds > 0)
        item.tmr->start(seconds * 1000);
}

void
NotificationBarWidget::hideNotification(const QString &key)
{
    QLabel *lbl = 0;
    lbl = m_items[key].lbl;
    lbl->hide();

    m_items[key].tmr->stop();
}

QLabel*
NotificationBarWidget::createNotificationLabel(const QString &text)
{
    QLabel *lbl = new QLabel(this);
    lbl->setText(text);
    lbl->hide();
    lbl->setSizePolicy(lbl->sizePolicy().horizontalPolicy(), QSizePolicy::Fixed); //prevent widget being stretched vertically
    lbl->setFrameStyle(QLabel::Panel | QLabel::Raised);
    lbl->setLineWidth(5);
    QPalette palette = lbl->palette();
    palette.setColor(QPalette::Window, QColor("orange"));
    lbl->setPalette(palette);
    lbl->setAutoFillBackground(true);
    if (!m_css_color.isEmpty())
        lbl->setStyleSheet(QString("border:3px outset %1; background-color:%1;").arg(m_css_color));

    return lbl;
}

//This is what happens when I can't decide...
//int
//NotificationBarWidget::indexOf(const QString &key)
//{
//    int index = -1;
//    for (int i = 0; i < m_items.size(); i++)
//    {
//        if (m_items[i].key == key)
//        {
//            index = i;
//            break;
//        }
//    }
//    return index;
//}

