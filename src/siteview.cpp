#include "siteview.hpp"

SiteView::SiteView(QString address, QWidget *parent)
        : QWidget(parent)
{
    //Main layout
    //Fixed header with site name, scoll area for half-height channel views ...
    //or - in full view mode - one large channel view only
    QVBoxLayout *vbox = new QVBoxLayout;
    setLayout(vbox);
    m_notifications = new NotificationBarWidget;
    vbox->addWidget(m_notifications);
    QHBoxLayout *hbox_name = new QHBoxLayout;
    vbox->addLayout(hbox_name);
    lbl_site_name = new QLabel(tr("Loading site..."));
    //TODO context menu, address, link (click/copy) etc. (extras dialog site info?)
    hbox_name->addWidget(lbl_site_name);
    lbl_channel_name = new QLabel();
    hbox_name->addWidget(lbl_channel_name);
    QFrame *sep = new QFrame;
    sep->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    vbox->addWidget(sep);
    scr_main = new QScrollArea;
    vbox->addWidget(scr_main);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_DeleteOnClose);

    //Load site
    m_site = VSite::load(address);
    if (!m_site) //|| !_site->isValid()
    {
        QMessageBox::critical(this, tr("Cannot load site"),
            tr("This site cannot be loaded: %1").arg(address));
        setDisabled(true);
        return;
    }
    qDebug() << "VSite initialized in view" << address << m_site->currentChannel();
    //Request/response signals for known site actions
    //showPageSignal used to show login tab, connected in PeerPlayerMain
    connect(m_site,
        SIGNAL(showPageSignal(const QString&, const ActionContextRef&)),
        SIGNAL(showPageSignal(const QString&, const ActionContextRef&)));
    connect(m_site, SIGNAL(gotChannelName(const QString&)), SLOT(setChannel(const QString&)));
    connect(m_site, SIGNAL(gotChannelInfo(const QVariantMap&)), SLOT(setChannel(const QVariantMap&)));
    connect(m_site, SIGNAL(loadedSiteName(const QString&)), SLOT(setSiteName(const QString&)));
    connect(m_site, SIGNAL(loadedVideoList(const QVariantList&)), SLOT(setVideoList(const QVariantList&)));
    m_site->loadSiteName(); //for Peertube
    loadChannel(); //TODO check if name could be extracted from url
    //continue in slot when channel (name/info) is loaded

}

void
SiteView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F5)
    {
        qDebug() << "reload of site view requested";
        loadChannel();
        return;
    }

    //If you reimplement this handler, it is very important that you call the base class implementation if you do not act upon the key.
    //https://doc.qt.io/qt-5/qwidget.html#keyPressEvent
    QWidget::keyPressEvent(event);
}

void
SiteView::loadChannel()
{
    m_notifications->showNotification("load_site", tr("Loading channel..."));
    auto ctx = m_site->loadChannel(); //TODO check if name could be extracted from url
    connect(ctx.data(), SIGNAL(failed()), SLOT(loadChannelFailed()));
}

void
SiteView::loadChannelFailed()
{
    m_notifications->showNotification("load_site", tr("Failed to load channel (press F5 to try again)"));
}

void
SiteView::unloadFail()
{
    lbl_site_name->setText(tr("(failed to load site)"));
    foreach (QObject *child, scr_main->layout()->children())
        child->deleteLater();
    scr_main->layout()->deleteLater();
}

void
SiteView::setSiteName(const QString &name)
{
    //If name is blank, site could not be loaded
    if (name.isEmpty())
    {
        //No site name received = error initializing site
        return unloadFail();
    }

    //Set header - site name only (Peertube), for the channel name see below
    lbl_site_name->setText(name);
    emit setName(name, this);

}

void
SiteView::setChannel(const QString &name)
{
    qDebug() << "site view - set channel name:" << name;
    //Set header label, widgets
    m_current_channel = name;
    m_notifications->hideNotification("load_site");
    lbl_channel_name->setText(name);
    emit setName(name, this);

    //Load channel videos
    //(called here, should not be called before the channel is defined)
    m_site->loadChannelVideos();

}

void
SiteView::setChannel(const QVariantMap &channel)
{
    qDebug() << "site view - set channel info:" << channel;
    QString name = channel.value("name").toString();
    setChannel(name);
    QString title = channel.value("title").toString();
    if (!title.isEmpty())
        lbl_channel_name->setToolTip(title);
}

void
SiteView::setVideoList(const QVariantList &items)
{
    QVariantMap channel_data;
    channel_data["items"] = items;
    channel_data["name"] = m_site->currentChannel();
    VideoListView *view = new VideoListView(m_site, channel_data);
    connect(view, SIGNAL(openVideoSignal(const QString&, const QVariantMap&)), SIGNAL(openVideoSignal(const QString&, const QVariantMap&)));

    scr_main->setWidgetResizable(true);
    scr_main->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    if (scr_main->widget()) scr_main->widget()->deleteLater();
    scr_main->setWidget(view);

}

//ChannelView::ChannelView(VSiteBase *site, QString name, bool full_view, QWidget *parent)
//           : QWidget(parent, flags)
//{
//    QVBoxLayout *vbox = new QVBoxLayout;
//    setLayout(vbox);
//    lbl_title = new QLabel(name);
//    vbox->addWidget(lbl_title);
//
//}

//VSiteBase*
//ChannelView::site()
//{
//    return _site;
//}

VideoListView::VideoListView(VSiteBase *site, const QVariantMap &channel, QFrame *parent)
             : QFrame(parent),
               m_current_page(0),
               m_page_size(10)
{
    m_site = site;
    m_ch_info = channel;
    QVariantList items = channel["items"].toList();

    connect(m_site, SIGNAL(loadedVideoList(const QVariantList&)), SLOT(setVideoList(const QVariantList&)));
    connect(m_site, SIGNAL(loadedThumbnail(const QPixmap&, QObject*)), SLOT(loadThumbnail(const QPixmap&, QObject*)));
    //connect(m_site, SIGNAL(loadedVideoUrl(const QString&, QObject*)), SLOT(loadVideoUrl(const QString&, QObject*))); //TODO

    //Local storage
    m_storage = new VideoStorage(this);

    wid_list = new QWidget;
    QVBoxLayout *vbox_list = new QVBoxLayout;
    QVBoxLayout *vbox = new QVBoxLayout;
    wid_list->setLayout(vbox_list);
    setLayout(vbox);
    vbox->addWidget(wid_list);

    setVideoList(items);

}

void
VideoListView::setVideoList(const QVariantList &items)
{
    foreach (QObject *obj, wid_list->children())
        obj->deleteLater();
    QVBoxLayout *vbox = new QVBoxLayout;
    if (wid_list->layout()) delete wid_list->layout();
    wid_list->setLayout(vbox);

    //Grid used to display two columns, thumbnails on the left
    //We don't want to have a 9x9 grid or similar, it would obfuscate the order
    //With two columns, thumbnail can be grown so you can actually see it well
    QGridLayout *grid = new QGridLayout;
    vbox->addLayout(grid);
    int grid_row = -1;
    foreach (const QVariant &var, items)
    {
        QVariantMap item = var.toMap();

        //QGridLayout *grid_item = new QGridLayout;
        grid_row++;

        //Metadata - title, url, description, thumbnail
        QString name = item["title"].toString();
        QString description = item["description"].toString();
        QString address = item["address"].toString();
        QString thumbnail_url = item["thumbnail"].toString();
        QString video_url = item["url"].toString();
        if (video_url.startsWith("/"))
        {
            video_url = m_site->siteUrl().resolved(video_url).url();
            item["url"] = video_url;
        }

        //Thumbnail widget (image yet to be loaded)
        ImageWidget *wid_thumbnail = new ImageWidget(true);
        wid_thumbnail->setMinimumWidth(300);
        wid_thumbnail->setProperty("url", thumbnail_url);
        grid->addWidget(wid_thumbnail, grid_row, 0);

        //Button link
        QPushButton *btn_name = new QPushButton(name);
        btn_name->setFlat(true);
        //btn_name->setCursor(Qt::PointingHandCursor);
        QPalette pal1 = btn_name->palette();
        pal1.setColor(QPalette::WindowText, QColor("blue"));
        btn_name->setPalette(pal1);
        btn_name->setStyleSheet(
            //"QPushButton { text-align:left; } "
            //"text-align:left "
            "QPushButton { text-decoration:underline; } "
            "QPushButton:hover { color:blue; } "
        );
        btn_name->setProperty("video_item", item);
        //label->setWordWrap(true) doesn't work right
        connect(btn_name, SIGNAL(clicked()), SLOT(openVideo()));
        QHBoxLayout *hbox_title = new QHBoxLayout;
        hbox_title->addWidget(btn_name);
        hbox_title->addStretch(); //stretch right to move link to the left
        grid->addLayout(hbox_title, grid_row, 1);

        //Separator line
        QFrame *line_below = new QFrame;
        line_below->setFrameShape(QFrame::HLine);
        grid->addWidget(line_below, ++grid_row, 0, 1, 2);

        Item container;
        container.thumb = wid_thumbnail;
        m_items << container;

        loadThumbnail(thumbnail_url, wid_thumbnail);
    }
    grid->setColumnStretch(0, 40);
    grid->setColumnStretch(1, 60);

    m_current_page = m_site->pageOffset();
    m_page_size = m_site->pageSize();
    QHBoxLayout *hbox_nav = new QHBoxLayout;
    QPushButton *btn_prev = new QPushButton("<");
    connect(btn_prev, SIGNAL(clicked()), SLOT(loadPreviousPage()));
    hbox_nav->addWidget(btn_prev);
    QLabel *lbl_page = new QLabel(QString("%1").arg(m_current_page + 1));
    lbl_page->setAlignment(Qt::AlignHCenter);
    hbox_nav->addWidget(lbl_page);
    QPushButton *btn_next = new QPushButton(">");
    connect(btn_next, SIGNAL(clicked()), SLOT(loadNextPage()));
    hbox_nav->addWidget(btn_next);
    vbox->addLayout(hbox_nav);

}

void
VideoListView::loadPage(int index)
{
    m_site->loadVideos(index);
}

void
VideoListView::loadPreviousPage()
{
    int page_index = m_current_page;
    if (page_index <= 0) return;
    loadPage(page_index - 1);
}

void
VideoListView::loadNextPage()
{
    int page_index = m_current_page;
    loadPage(page_index + 1);
}

void
VideoListView::loadThumbnail(const QString &url, ImageWidget *thumb)
{
    if (url.isEmpty()) return;
    qDebug() << "site view - fetching thumbnail:" << url;
    //TODO debug() << ... LogLogger

    //Download thumbnail image and (callback) display it in ImageWidget
    //auto ctx = _site->download(url);
    m_storage->downloadFileToByteArray(url, [this, thumb](const QByteArray &b)
    {
        thumb->setPixmap(b);
    });

}

void
VideoListView::openVideo(const QVariantMap &item)
{
    //if (!item.contains("video_url"))
    //{
    //    QObject *ctx = createContextObject(item);
    //    _site->loadVideoUrl(item, ctx);
    //    return;
    //}

    qDebug() << "TODO add vsite to videoview ctor call"; //TODO
    QVariantMap item2 = item;
    item2["_site_url"] = m_site->siteUrl().url(); //TODO move into action
    QString url = item2.value("url").toString();
    emit openVideoSignal(url, item2);
}

void
VideoListView::openVideo()
{
    QObject *obj = QObject::sender();
    if (!obj) return;
    QVariantMap item = obj->property("video_item").toMap();

    //Video open signal - open by url
    openVideo(item);
}

