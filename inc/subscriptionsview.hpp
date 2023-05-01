#ifndef SUBSCRIPTIONSVIEW_HPP
#define SUBSCRIPTIONSVIEW_HPP

#include <QDebug>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLineEdit>
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QKeyEvent>

#include "vsite.hpp"

class SubscriptionsView : public QWidget
{
    Q_OBJECT

signals:

    void
    openRequested(const QString &address);

public:

    SubscriptionsView(QWidget *parent = 0, Qt::WindowFlags flags = 0);

    ~SubscriptionsView();

private slots:

    void
    save();

    void
    keyPressEvent(QKeyEvent *event);

    void
    loadSubscriptionList();

    void
    checkAdd();

    void
    removeItem(int index);

    void
    renameItem(int index);

private:

    ProfileSettings
    *m_settings;

    QList<QVariantMap>
    m_sub_items;

    QScrollArea
    *m_scr_main;

    QLineEdit
    *m_txt_addr;

    QPushButton
    *m_btn_add;


};

#endif
