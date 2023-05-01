#include "peerplayermain.hpp"

PeerPlayerMain::PeerPlayerMain(QWidget *parent, Qt::WindowFlags flags)
              : QMainWindow(parent, flags),
                m_tab_id(0)
{

    /*
     * IDEAS
     *
     * This is the main window, which contains all the widgets,
     * all the panels and areas and views and things,
     * in tabs, sort of like a web browser.
     * Although it doesn't make much sense to play multiple videos
     * at the same time, it should be possible to open multiple videos
     * each in a separate tab, to allow the user to watch one,
     * then pause it (coffee break), come back later and watch another one,
     * then go back to the first one and finish it.
     * Instead of simple tabs, we could show the contents in dock widgets.
     * https://doc.qt.io/qt-5/qtwidgets-mainwindows-dockwidgets-example.html
     */

    //Tab bar
    m_tab_widget = new QTabWidget;
    setCentralWidget(m_tab_widget);
    m_tab_widget->setTabsClosable(true);
    m_tab_widget->setMovable(true);
    connect(m_tab_widget, SIGNAL(tabCloseRequested(int)), SLOT(closeTab(int)));
    QPushButton *btn_menu = new QPushButton("...");
    m_tab_widget->setCornerWidget(btn_menu);
    QMenu *mnu = new QMenu;
    btn_menu->setMenu(mnu);
    QAction *act_open_channel_url = mnu->addAction(tr("Open channel by url"));
    connect(act_open_channel_url, SIGNAL(triggered()), SLOT(showOpenChannelUrl()));
    m_act_subscriptions = mnu->addAction(tr("Subscriptions"));
    connect(m_act_subscriptions, SIGNAL(triggered()), SLOT(addSubscriptionsTab()));
    m_act_settings = mnu->addAction(tr("Settings"));
    connect(m_act_settings, SIGNAL(triggered()), SLOT(showSettings()));

    //Subscriptions view
    addSubscriptionsTab();

    //Restore window state settings
    ProfileSettings *settings = ProfileSettings::profile();
    QVariant v_settings = settings->variant("pos_size");
    if (!v_settings.isNull())
    {
        QBuffer buffer;
        buffer.setData(QByteArray::fromBase64(v_settings.toByteArray()));
        buffer.open(QBuffer::ReadOnly);
        QDataStream ds(&buffer);
        QPoint pos;
        QSize size;
        bool maximized;
        ds >> pos >> size >> maximized;
        if (maximized)
        {
            setWindowState(windowState() ^ Qt::WindowMaximized);
            //TODO Qt::WindowFullScreen
        }
        else
        {
            move(pos);
            resize(size);
        }
    }
    //Restore open video tabs
    v_settings = settings->variant("tabs");
    if (!v_settings.isNull())
    {
        QBuffer buffer;
        buffer.setData(QByteArray::fromBase64(v_settings.toByteArray()));
        buffer.open(QBuffer::ReadOnly);
        QDataStream ds(&buffer);
        QVariantList tab_info_list;
        ds >> tab_info_list;
        foreach (QVariant v, tab_info_list)
        {
            QVariantMap tab_info = v.toMap();
            QString type = tab_info["type"].toString();
            QString address = tab_info["address"].toString();
            //video tabs are restored paused, autoplay off
            //it generally does not make sense to start playback in background
            if (type == "site")
                addSiteTab(address);
            else if (type == "video")
                openVideo(address, tab_info["item"].toMap(), true);
        }
    }

}

void PeerPlayerMain::closeEvent(QCloseEvent *event)
{
    //NOTE closeEvent() is not called for child widgets
    //PeerPlayerMain is currently the top level widget
    //but in case this is changed in any way, this event needs to be moved

    ProfileSettings *settings = ProfileSettings::profile();

    QBuffer buffer;
    buffer.open(QBuffer::ReadWrite);
    {
        QDataStream ds(&buffer);
        ds << pos() << size() << isMaximized();
    }
    settings->setVariant("pos_size", buffer.data().toBase64()); //q_settings
    buffer.close();

    buffer.open(QBuffer::WriteOnly | QIODevice::Truncate);
    {
        QVariantList tab_info_list;
        for (int i = 0; i < m_tab_widget->count(); i++)
        {
            QVariant tab_id_v = m_tab_widget->widget(i)->property("tab_id");
            if (!tab_id_v.isValid()) continue;
            int tab_id = tab_id_v.toInt();
            QVariant tab_info = m_loaded_tabs.value(tab_id);
            tab_info_list << tab_info;
        }
        QDataStream ds(&buffer);
        ds << tab_info_list;
    }
    settings->setVariant("tabs", buffer.data().toBase64()); //q_settings

    settings->save();

    event->accept();
}

void
PeerPlayerMain::addSubscriptionsTab()
{
    SubscriptionsView *subscriptions_tab = new SubscriptionsView;
    subscriptions_tab->setAttribute(Qt::WA_DeleteOnClose); //memleak + bug if missing
    connect(subscriptions_tab,
        SIGNAL(openRequested(const QString&)),
        SLOT(addSiteTab(const QString&)));
    int index = m_tab_widget->count();
    m_tab_widget->addTab(subscriptions_tab, tr("Subscriptions"));
    //m_tab_widget->tabBar()->tabButton(index, QTabBar::RightSide)->hide();

    m_act_subscriptions->setEnabled(false);

    connect(subscriptions_tab, &QObject::destroyed, this, [this]()
    {
        m_act_subscriptions->setEnabled(true);
    });
}

void
PeerPlayerMain::showSettings()
{
    SettingsWindow *settings = new SettingsWindow;
    settings->setAttribute(Qt::WA_DeleteOnClose);
    //TODO close on quit
    settings->show();
    //settings->open();

}

void
PeerPlayerMain::showOpenChannelUrl()
{
    QString in_addr = QInputDialog::getText(this, tr("Open channel"),
        tr("Enter channel address"));
    if (in_addr.isEmpty()) return;

    SiteView *view = addSiteTab(in_addr);
    m_tab_widget->setCurrentWidget(view);

}

SiteView*
PeerPlayerMain::addSiteTab(const QString &address)
{
    SiteView *site_tab = new SiteView(address);
    site_tab->setAttribute(Qt::WA_DeleteOnClose);
    m_tab_widget->addTab(site_tab, tr(""));
    connect(site_tab, SIGNAL(setName(const QString&, QWidget*)), SLOT(setTabName(const QString&, QWidget*)));
    connect(site_tab,
        SIGNAL(openVideoSignal(const QString&, const QVariantMap&)),
        SLOT(openVideo(const QString&, const QVariantMap&)));
    connect(site_tab,
        SIGNAL(showPageSignal(const QString&, const ActionContextRef&)),
        SLOT(showPage(const QString&, const ActionContextRef&)));

    initNewTabWidget(site_tab, "site", address);

    return site_tab;
}

void
PeerPlayerMain::setTabName(const QString &name, const QString &full_title, QWidget *tab)
{
    if (!tab)
    {
        QObject *o = QObject::sender();
        tab = qobject_cast<QWidget*>(o);
    }
    int index = m_tab_widget->indexOf(tab);
    if (index < 0) return;
    m_tab_widget->setTabText(index, name);
    m_tab_widget->setTabToolTip(index, full_title);

}

void
PeerPlayerMain::setTabName(const QString &name, QWidget *tab)
{
    setTabName(name, "", tab);
}

void
PeerPlayerMain::closeTab(int index)
{
    QWidget *view = m_tab_widget->widget(index);
    if (!view) return;
    view->close(); //close() needs delete_on_close to be true

    QTimer::singleShot(0, this, SLOT(checkLastTabClosed()));
}

void
PeerPlayerMain::checkLastTabClosed()
{
    if (m_tab_widget->count()) return;

    //TODO depending on user setting, close window, ask - or open default tab
    addSubscriptionsTab();

}

void
PeerPlayerMain::openVideo(const QString &url, const QVariantMap &item, bool in_background)
{
    //note that item["title"] may be blank/missing
    //the title updated via signal later
    QString name = item["title"].toString();
    //TODO animate tab name, rolling long titles and ">" play icon 

    //Create video tab widget
    VideoView *view = new VideoView(url, item, 0, !in_background); //TODO , site ref
    view->setAttribute(Qt::WA_DeleteOnClose);
    m_tab_widget->addTab(view, name);
    initNewTabWidget(view, "video", url, item); //store open tab
    connect(view, SIGNAL(setName(const QString&, const QString&, QWidget*)), SLOT(setTabName(const QString&, const QString&, QWidget*)));
    //Switch to new tab and allow playback unless opened/restored in background
    //TODO there's no delay when adding local media,
    //it would already be added but paused at this point
    if (!in_background)
    {
        m_tab_widget->setCurrentWidget(view);
    }

    //TODO 
    //Do not auto-play TODO
    //Prevent multiple video tabs from playing at the same time

}

/**
 * Displays an interactive web page in a new tab, returns result to action.
 *
 * QWebEngineProfile is a bit difficult to get running.
 * The QtWebEngineProcess executable should be copied to the bin/ directory,
 * otherwise the application may crash even if QTWEBENGINEPROCESS_PATH is set.
 *
 * The bin/ directory should contain a Qt config file called qt.conf:
 * [Paths]
 * Data = /build/.../qtwebengine/src/core/release
 * Translations = /build/.../qtwebengine/src/core/release
 *
 * qDebug() << QLibraryInfo::location(QLibraryInfo::DataPath);
 * qDebug() << QLibraryInfo::location(QLibraryInfo::TranslationsPath);
 */
void
PeerPlayerMain::showPage(const QString &url, const ActionContextRef &ctx)
{
    //VSite requested interactive page to be displayed
    //The web view will be owned by the action (ctx), deleted on timeout
    //Initialize QWebEngineView, connect it to action ctx
    qInfo() << "main: show page:" << url;
    QWebEngineView *view = new QWebEngineView();
    view->setAttribute(Qt::WA_DeleteOnClose); //timeout => view->deleteLater()
    connect(ctx.data(), SIGNAL(destroyed()), view, SLOT(close())); //ctx = parent
    m_tab_widget->addTab(view, "AUTHENTICATION");
    m_tab_widget->setCurrentWidget(view);
    view->pageAction(QWebEnginePage::ViewSource)->setEnabled(false);
    view->pageAction(QWebEnginePage::SavePage)->setEnabled(false);
    view->pageAction(QWebEnginePage::InspectElement)->setEnabled(false);
    view->pageAction(QWebEnginePage::DownloadMediaToDisk)->setEnabled(false);
    view->pageAction(QWebEnginePage::DownloadImageToDisk)->setEnabled(false);
    view->pageAction(QWebEnginePage::DownloadLinkToDisk)->setEnabled(false);
    //OpenLinkInNewBackgroundTab OpenLinkInNewTab OpenLinkInNewWindow OpenLinkInThisWindow
    //Reload?

    //Initialize page object with off-the-record profile, i.e., private mode
    //profile should not be deleted before page; therefore connecting delete...
    //[WARNING] Release of profile requested but WebEnginePage still not deleted. Expect troubles !
    QWebEngineProfile *profile = new QWebEngineProfile(); //temp. profile
    QWebEnginePage *page = new QWebEnginePage(profile, view);
    connect(view, SIGNAL(destroyed()), profile, SLOT(deleteLater()));
    view->setPage(page);
    page->setUrl(QUrl(url));

    //Install action handlers, load action returns page/result to action
    connect(view,
        &QWebEngineView::titleChanged,
        [this, view](const QString &title) { this->setTabName(title, view); });
    connect(view,
        &QWebEngineView::loadFinished, this,
        [this, view, ctx](bool ok) { this->handleLoadedPage(view, ok, ctx); });

}

void
PeerPlayerMain::handleLoadedPage(QWebEngineView *view, bool ok, const ActionContextRef &ctx)
{
    //A page has been loaded, first load is initial page (login form)
    //second one is usually the expected result page
    int count = view->property("load_count").toInt();
    bool initial_page_loaded = count++ > 0;
    view->setProperty("load_count", count);
    qInfo() << ctx.data() << "page loaded" << ok << view->url().url() << "step" << count;

    //count - first page is login page with username/password fields
    //after entering credentials and confirming, that's the second page
    //then we grab the result and forward it to be evaluated
    //(action plan continues, finishes)
    if (count == 2)
    {
        //final step
        qInfo() << "final step, finishing action...";
        QVariantMap map;
        map["url"] = view->url().url();
        map["ok"] = ok;
        view->page()->toPlainText([map, ctx](const QString &text)
        {
            QVariantMap map2(map);
            map2["text"] = text;
            qDebug() << map2;
            ctx->setValue(QVariant(map2));
        });
        QTimer::singleShot(0, view, SLOT(deleteLater()));
    }

}

void
PeerPlayerMain::initNewTabWidget(QWidget *widget, const QString &type, const QString &address, const QVariantMap &item)
{
    int tab_id = m_tab_id++;
    widget->setProperty("tab_id", tab_id);
    QVariantMap tab_info;
    tab_info["type"] = type;
    tab_info["address"] = address;
    tab_info["item"] = item;
    m_loaded_tabs[tab_id] = tab_info;

    connect(widget, &QObject::destroyed, this, [this, tab_id]()
    {
        m_loaded_tabs.remove(tab_id);
    });
}

