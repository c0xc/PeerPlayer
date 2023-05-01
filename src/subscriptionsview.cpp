#include "subscriptionsview.hpp"

SubscriptionsView::SubscriptionsView(QWidget *parent, Qt::WindowFlags flags)
                 : QWidget(parent, flags)
{
    m_settings = ProfileSettings::profile();

    QVBoxLayout *vbox = new QVBoxLayout;
    setLayout(vbox);

    m_scr_main = new QScrollArea;
    m_scr_main->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    vbox->addWidget(m_scr_main);

    loadSubscriptionList();

}

SubscriptionsView::~SubscriptionsView()
{
    //clean up - closeEvent() is never called

    save();
}

void
SubscriptionsView::save()
{
    QVariantList subscription_list;
    foreach (const QVariantMap &item, m_sub_items)
        subscription_list << item;
    m_settings->setVariant("subscription_list", subscription_list);

    m_settings->save();
}

void
SubscriptionsView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F5)
    {
        loadSubscriptionList();
        return;
    }

    QWidget::keyPressEvent(event);
}

void
SubscriptionsView::loadSubscriptionList()
{
    //Create new container widget in scroll area
    if (m_scr_main->widget()) m_scr_main->widget()->deleteLater();
    QWidget *container = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout;
    container->setLayout(vbox);

    //Load subscription list
    m_sub_items.clear();
    foreach (QVariant v, m_settings->variant("subscription_list").toList())
        m_sub_items << v.toMap();

    //Display subscription items: title, options button, address
    int i = -1;
    foreach (const QVariantMap &sub_info, m_sub_items)
    {
        i++;

        QPushButton *btn_title = new QPushButton(sub_info.value("title").toString());
        btn_title->setFlat(true);
        btn_title->setStyleSheet(
            "QPushButton { font-size:14pt; font-weight:bold; text-decoration:underline; text-align:left; } "
            "QPushButton:hover { color:blue; } "
        );
        connect(btn_title, &QAbstractButton::clicked, this, [this, sub_info]
        ()
        {
            QString addr = sub_info.value("url").toString();
            emit openRequested(addr);
        });

        QLineEdit *txt_addr = new QLineEdit;
        txt_addr->setReadOnly(true);
        txt_addr->setText(sub_info.value("url").toString());
        QPushButton *btn_more = new QPushButton(tr("..."));
        QMenu *menu = new QMenu;
        btn_more->setMenu(menu);
        QAction *act_open = menu->addAction(tr("Open channel"));
        connect(act_open, SIGNAL(triggered()), btn_title, SIGNAL(clicked()));
        QAction *act_rename = menu->addAction(tr("Rename subscription"));
        connect(act_rename, &QAction::triggered, this, [this, i]()
        {
            renameItem(i);
        });
        QAction *act_remove = menu->addAction(tr("Unsubscribe"));
        connect(act_remove, &QAction::triggered, this, [this, i]()
        {
            removeItem(i);
        });

        vbox->addWidget(btn_title);
        QHBoxLayout *hbox1 = new QHBoxLayout;
        hbox1->addWidget(txt_addr);
        hbox1->addWidget(btn_more);
        hbox1->setStretch(0, 90);
        hbox1->setStretch(1, 10);
        vbox->addLayout(hbox1);

        QFrame *line_below = new QFrame;
        line_below->setFrameShape(QFrame::HLine);
        vbox->addWidget(line_below);

    }

    m_txt_addr = new QLineEdit;
    m_btn_add = new QPushButton("+");
    connect(m_btn_add, SIGNAL(clicked()), SLOT(checkAdd()));
    QHBoxLayout *hbox_add_addr = new QHBoxLayout;
    hbox_add_addr->addWidget(m_txt_addr);
    hbox_add_addr->addWidget(m_btn_add);
    hbox_add_addr->setStretch(0, 90);
    hbox_add_addr->setStretch(1, 10);
    vbox->addLayout(hbox_add_addr);

    m_scr_main->setWidget(container);
}

//TODO fix bug when adding site that requires authentication
void
SubscriptionsView::checkAdd()
{
    QString in_addr = m_txt_addr->text();
    if (in_addr.isEmpty()) return;
    m_txt_addr->setDisabled(true);
    m_btn_add->setDisabled(true);

    QObject scope;
    //Try to load VSite - fails if incompatible
    QPointer<VSite> site = VSite::load(in_addr);
    m_txt_addr->setDisabled(false);
    if (!site)
    {
        //Invalid or unknown address
        QMessageBox::critical(this, tr("Unknown address"),
            tr("This address is not recognized. It cannot be added."));
        return;
    }
    //Try to get channel info
    auto ctx = site->loadChannel(); //enableSignal();
    if (ctx)
    {
        connect(ctx.data(), &ActionContext::failed, this, [this, site, ctx]
        ()
        {
            QMessageBox::warning(this, tr("Unknown address"),
                tr("This does not appear to be a valid channel address. Failed to load channel info."));
            //Clean up temporary site instance
            site->deleteLater();
        });
        connect(ctx.data(), &QObject::destroyed, this, [this] //, site, ctx //TODO memory leak!?
        ()
        {
            //TODO never executed - memory leak??
            qInfo() << "ctx for temporary site action deleted";
            //Check if channel info received in meantime
            //if (!ctx->map["value"].isValid())
            //{
            //    QMessageBox::warning(this, tr("Unknown address"),
            //        tr("This does not appear to be a valid channel address. Failed to load channel info."));
            //}
        });
    }
    else
    {
        //Cannot load channel info, channel name not found
        QMessageBox::warning(this, tr("Unknown address"),
            tr("This is not a channel address or address not understood."));
        //Clean up temporary site instance
        site->deleteLater();
    }
    //Site instance deleted
    connect(site, &VSite::destroyed, this, [this, site]
    ()
    {
        //Enable gui elements again
        m_btn_add->setDisabled(false);
    });
    //Action complete, result value returned via site
    connect(site, &VSite::gotChannelInfo, this, [this, in_addr, site]
    (const QVariantMap &info)
    {
        //Get title from channel info received from site
        QString name = info.value("title").toString();
        site->setProperty("got_channel_info", true);
        //Add subscription item to list
        QVariantList subscription_list = m_settings->variant("subscription_list").toList();
        QVariantMap sub_info;
        sub_info["url"] = in_addr;
        sub_info["title"] = name;
        sub_info["channel"] = info;
        subscription_list << sub_info;
        m_settings->setVariant("subscription_list", subscription_list);
        //Show success message
        QMessageBox::information(this, tr("Subscription"),
            tr("Channel added to list!"));
        //Clean up temporary site instance
        site->deleteLater();
        //Reload subscription list
        QTimer::singleShot(0, this, SLOT(loadSubscriptionList()));
    });

}

void
SubscriptionsView::removeItem(int index)
{
    QVariantMap sub_info = m_sub_items[index];

    //Ask for confirmation
    QString name = sub_info.value("title").toString();
    if (QMessageBox::question(this, tr("Unsubscribe"),
        tr("Do you want to unsubscribe from this channel?\n%1").arg(name)) != QMessageBox::Yes)
        return;

    //Remove item from list
    m_sub_items.removeAt(index);
    save();

    //Reload subscription list view
    QTimer::singleShot(0, this, SLOT(loadSubscriptionList()));
}

void
SubscriptionsView::renameItem(int index)
{
    QVariantMap &sub_info = m_sub_items[index];

    //Ask for new title
    QString old_name = sub_info.value("title").toString();
    QString new_name = QInputDialog::getText(this, tr("Subscription"),
        tr("Type in new title for this channel subscription.\n%1").arg(old_name));
    if (new_name.isEmpty()) return;

    //Rename item
    sub_info["title"] = new_name;
    save();

    //Reload subscription list view
    QTimer::singleShot(0, this, SLOT(loadSubscriptionList()));
}

