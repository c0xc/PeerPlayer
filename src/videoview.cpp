#include "videoview.hpp"

/**
 * VideoView ctor - this is the widget class for playing videos,
 * both online and offline.
 *
 * For online videos, it expects the video url in a known format or
 * a QVariantMap, which contains the input address
 * or video identifier, as returned by get_channel_videos (or similar).
 * It will be passed (via stash) to the get_video action,
 * which returns a list of maps containing download urls.
 * These returned maps are stored here as sources in _video_items.
 * Furthermore, if a local copy of the video is found/known,
 * it will also be stored there. Though that last part may change, not sure.
 */
VideoView::VideoView(QWidget *parent)
         : QWidget(parent),
           m_src_index(-1),
           m_download_active(false),
           m_import_requested(false),
           m_play_on_select(false)
{
    setAttribute(Qt::WA_DeleteOnClose);
    initWidgets();

    m_storage = new VideoStorage(this);
    //We use strings instead of QFileInfo objects because we need the path and:
    //> Warning: If filePath() is empty the behavior of this function is undefined.
    //https://doc.qt.io/qt-5/qfileinfo.html#absoluteFilePath
    connect(m_storage, SIGNAL(downloadStarted(const QString&)),
        SLOT(handleDownloadStarted(const QString&)));
    connect(m_storage, SIGNAL(downloadProgressed(double)),
        SLOT(handleDownloadProgress(double)));
    connect(m_storage, SIGNAL(downloadFailed()),
        SLOT(handleDownloadFailed()));
    connect(m_storage, SIGNAL(downloadFinished()),
        SLOT(completeDownloadedVideo()));
    connect(m_storage, &VideoStorage::suggestFilename, this, [this](const QString &filename)
    {
        qDebug() << "suggested filename:" << filename;
        m_src_context["_suggested_filename"] = filename;
    });
    connect(m_storage, &VideoStorage::fileImported, this, [this](const QString &filename)
    {
        QMessageBox::information(this, tr("Video imported"),
            tr("The video has been imported."));
        m_notifications->hideNotification("import");
    });

    //Base ctor - no further action, see below
}

VideoView::VideoView(const QString &address, const QVariantMap &context, QPointer<VSiteBase> site, bool play_on_select, QWidget *parent)
         : VideoView(parent)
{
    //Initialize VSite for given video address (with context)
    //address is a video site address, not a file download link (not *.mpg)
    //If metadata has already been collected, it can be provided as context map
    //which will be saved on import (to keep title, description etc. for later
    //when the imported copy is opened).
    m_src_context = context; //use optional context map
    m_src_context["url"] = address; //set relevant url regardless of context
    m_src_address = address;
    qDebug() << "VideoView loading with address" << address;
    if (address.isEmpty())
    {
        QMessageBox::critical(this, tr("Cannot load video"),
            tr("No video address defined."));
        m_notifications->showNotification("online", "Cannot load video - No video address defined!");
        setDisabled(true);
        return;
    }
    //Do not close here! The widget is about to be added to the tab bar, segfault

    //Try to (re)connect to site (prefer copy but ‘VSiteBase’ to ‘const VSite&’)
    //VSite connection/instance required if we don't have a local copy already
    //if (site)
    //    m_site = new VSite(*site.data()); //TODO fix that?
    //else
    //    m_site = VSite::load(address);
    m_site = VSite::load(address);

    //Normalize url, which may contain hash that prevents lookup
    //If the site doesn't have that function, the address remains unchanged
    if (m_site)
    {
        m_src_address = m_site->normalizePlayerAddress(m_src_address);
        qDebug() << "using source address:" << m_src_address;
    }

    //Load downloaded copy if one exists, add it as first source to be played
    //If a copy of the video is already in local storage,
    //it would have to be identified by the source address,
    //so that next time the same video address is opened, it should be found.
    //This lookup by context/address is attempted now.
    //Add downloaded copy, if one exists (no action otherwise)
    if (play_on_select) m_play_on_select = true; //autoplay
    addVideoSource(m_storage->findFileByAddress(m_src_address));

    //Show error, disable view if site could not be loaded
    //and downloaded copy wasn't found either
    if (!m_site)
    {
        qDebug() << "Failed to load VSite in view:" << address;
        //Site could not be loaded, video unavailable except if saved locally
        QMessageBox::critical(this, tr("Cannot load video"),
            tr("Failed to connect to site: %1").arg(address));
        //No site connection, local videos only
        if (isEmpty())
        {
            QMessageBox::critical(this, tr("Cannot load video"),
                tr("No sources found."));
            m_notifications->showNotification("online", "Cannot load video - No video sources found!");
            setDisabled(true);
            return;
        }
    }

    //Title and metadata will be queried from site
    if (context.isEmpty())
        loadContext(); //TODO
    else
        contextLoaded();

    //Load video file urls from site or download it externally
    //We do not need that if we already have a local copy
    //but we should allow the user to start a new download anyway
    if (isEmpty())
    {
        loadUrl(address);
    }
    else
    {
        m_notifications->showNotification("download", "Not loading video from Internet: Video already downloaded (press F5 to download anyway)", 30);
    }

}

/* //TODO this is obsolete before it was used
VideoView::VideoView(const QVariantMap &item, QWidget *parent)
         : VideoView(parent)
{
    //Video item = dict containing video reference and some metadata
    //Video context is a prepared dictionary with pre-loaded stash
    //This was initially written to allow the site function to use
    //variables that have already been collected (claim id, hash...)
    //but that was a bad idea so it's now obsolete because
    //having an unknown dictionary of values returned by the site
    //as video context is clumsy and does not allow us to map it properly.
    //What we need is a single value to allow forward lookups later
    //and that value should be the address/url.
    m_src_context = item;
    qDebug() << "VideoView loading with dict" << item;
    qDebug() << "VideoView loading with dict is obsolete, do not use";

    //Get title and description from info dict
    QString url = item["url"].toString();
    QString title = item["title"].toString();
    QString description = item["description"].toString();
    m_lbl_title->setText(title); //TODO obsolete

    //Add downloaded copy, if one exists (no action otherwise)
    addVideoSource(m_storage->find(m_src_context));

    //Try to connect to site
    QString site_url = item["_site_url"].toString();
    m_site = VSite::load(site_url);
    if (!m_site)
    {
        //Site could not be loaded
        QMessageBox::critical(this, tr("Cannot load video"),
            tr("Failed to reconnect to site: %1").arg(site_url));
        if (isEmpty())
        {
            QMessageBox::critical(this, tr("Cannot load video"),
                tr("No sources found."));
            close();
            return;
        }
    }

    //Load video file urls from site
    loadUrl(url);

}
*/
//TODO this obsolete code above will be removed because we need an identifier
//so an address to know what was opened there, to reload it later

VideoView::VideoView(const QFileInfo &fi, bool play_on_select, QWidget *parent)
         : VideoView(parent)
{
    //Initialize view for local video file (context)
    //which may be a downloaded copy in which case
    //we'd have the original (site) video url in the map table
    //address is a video site address, not a file download link
    m_src_context["file"] = fi.filePath();
    m_src_file = fi.filePath();

    //Add requested local file as first source
    if (play_on_select) m_play_on_select = true; //autoplay
    addVideoSource(fi);

    //No import available for local videos
    m_btn_import->setDisabled(true);

    //If video is known with context, add metadata now.
    //It would not make sense to add the local copy of the video
    //since we're already opening a local file.
    //However it would be helpful to add context that we don't have,
    //so we basically have the filename and that's it
    //but the context map would give us goodies like description or title.
    m_src_context = m_storage->findFileContext(fi.filePath());
    //addContext(m_storage->find(fi));
    //TODO schedule hash in background
    if (!m_src_context.isEmpty()) contextLoaded();

}

void
VideoView::initWidgets()
{
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(0);
    setLayout(vbox);

    //Notification dropdown bar
    m_notifications = new NotificationBarWidget;
    m_notifications->setColor("orange");
    vbox->addWidget(m_notifications);

    //Video title
    //also some menu for more details like video context
    m_lbl_title = new QLabel();
    m_lbl_title->setWordWrap(true);
    m_lbl_title->setSizePolicy(m_lbl_title->sizePolicy().horizontalPolicy(), QSizePolicy::Fixed); //TODO margin: 0px ?
    vbox->addWidget(m_lbl_title);

    //Video player widget in the middle
    //The player widget could be reloaded, for example if it got stuck
    //after it was temporarily hidden (tab switch).
    //Use F4 to reload the player
    m_wid_player = new QWidget; //container widget to make player replaceable
    m_vbox_player_container = new QVBoxLayout;
    m_vbox_player_container->setContentsMargins(QMargins());
    m_wid_player->setLayout(m_vbox_player_container);
    vbox->addWidget(m_wid_player);
    //Create player, see reinit function...
    m_vlc = new VlcPlayer();
    m_vbox_player_container->addWidget(m_vlc);

    //Set dark mode or custom stylesheet
    QString css =
        //"QWidget { background-color:darkgray; } "
        //"QPushButton { background-color:#2f4f4f; border:5px outset #2f4f4f; color:lightblue; } "
        //"QPushButton:hover { background-color:#528989; border:2px outset #2f4f4f; } ";
        "QPushButton { background-color:darkslategray; color:white; } "
        "QPushButton:hover { background-color:steelblue; } "
    ;
    setStyleSheet(css); //css on QWidget causes flicker

    connect(m_vlc, SIGNAL(started()), m_lbl_title, SLOT(hide()));
    connect(m_vlc, SIGNAL(stopped()), m_lbl_title, SLOT(show()));

    //Description preview and control buttons: Import, Source (version/res.)
    QHBoxLayout *hbox_bottom1 = new QHBoxLayout;
    m_txt_desc = new QTextEdit;
    m_txt_desc->setReadOnly(true);
    m_txt_desc->setDisabled(true);
    m_txt_desc->setVisible(false);
    m_txt_desc->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_txt_desc->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    hbox_bottom1->addWidget(m_txt_desc, 70);
    m_btn_show_desc = new QPushButton(tr("Description"));
    m_btn_show_desc->setToolTip(tr("Show video description"));
    m_btn_show_desc->setDisabled(true);
    connect(m_btn_show_desc, SIGNAL(clicked()), SLOT(showDescription()));
    hbox_bottom1->addWidget(m_btn_show_desc, 10);
    m_btn_import = new QPushButton(tr("&Import")); //Video herunterladen und behalten
    m_btn_import->setToolTip(tr("Save and keep video"));
    connect(m_btn_import, SIGNAL(clicked()), SLOT(importVideo()));
    hbox_bottom1->addWidget(m_btn_import, 10);
    m_btn_src = new QPushButton("Video source"); //source of video file (online, local)
    m_btn_src->setToolTip(tr("Available versions of this video (e.g., the original video from the Internet + the downloaded copy)"));
    m_btn_src->setDisabled(true);
    hbox_bottom1->addWidget(m_btn_src, 10);
    vbox->addLayout(hbox_bottom1);

    //Dark color style
    QPalette bg_palette = palette();
    bg_palette.setColor(QPalette::Window, QColor("darkslate"));
    setPalette(bg_palette);
    setAutoFillBackground(true);

    //Timer used to scroll the title (tab bar etc.)
    m_tmr_title = new QTimer;
    m_tmr_title->setInterval(100);
    connect(m_tmr_title, SIGNAL(timeout()), SLOT(updateTitle()));

}

void
VideoView::recreatePlayer()
{
    //Copy VLC player by creating new one which tries to copy playlist item
    VlcPlayer *vlc_old = m_vlc;
    VlcPlayer *vlc_new = new VlcPlayer(*vlc_old);
    m_vlc = vlc_new;

    //Replace player in layout and delete old one
    vlc_old->hide();
    m_vbox_player_container->removeWidget(vlc_old);
    vlc_old->deleteLater();
    m_vbox_player_container->addWidget(vlc_new);
}

void
VideoView::setPlayOnSelect(bool enabled)
{
    m_play_on_select = enabled;
}

QString
VideoView::title()
{
    //Look for a title somewhere, no success guaranteed
    //m_src_context is the context with which the video was opened/identified
    //it could be a local file in which case, well...
    //TODO get current url -> name ... or m_storage->knownTitle(context)
    QString title;
    title = m_src_context.value("title").toString(); //may not exist
    if (title.isEmpty())
        title = m_src_context.value("name").toString(); //may also be missing
    return title;
}

/**
 * loadUrl() takes the address of a video page (or embed url),
 * triggers the site action that will determine the url of the video file
 * and then it adds that remote file url to the list of video sources
 * that can be played here.
 *
 * Our main goal is to get something to play, something to feed the player.
 * That's why no file download is started by default, at least
 * for fully supported sites (i.e., when the real video file url is known).
 * On import, it's downloaded if it hasn't already.
 * For special sites that aren't fully supported,
 * the download is started now (using the external downloader) and
 * the temporary file will be played while being downloaded.
 */
void
VideoView::loadUrl(const QString &address)
{
    if (address.isEmpty())
    {
        m_notifications->showNotification("online", "Cannot load - no video url");
        return;
    }
    //Trigger action to get address of video file(s)
    qInfo() << "init VideoView loading video from site:" << address;
    auto ctx = m_site->loadVideo(address);
    if (!ctx)
    {
        //action missing, site not fully supported; download externally
        //temp file will be used for playback (only, except on import)
        downloadExternally(address);
        return;
    }
    ctx->enableSignal(); //send result to ctx, not to listeners of site
    connect(ctx.data(), SIGNAL(finished(const QVariant&)), SLOT(addVideoSource(const QVariant&)));
    connect(ctx.data(), SIGNAL(failed()), SLOT(loadingFailed()));
    m_notifications->showNotification("online", "Loading video from site...", 5);
}

/**
 * This takes the address of a video page (or embed url),
 * calls the external downloader and then adds the downloaded file as a source.
 * This is basically a workaround for a site that has no working
 * get_video_url function defined.
 * So again, address is not a remote file address but the video page address.
 */
void
VideoView::downloadExternally(const QString &address)
{
    if (m_download_active)
    {
        QMessageBox::critical(this, tr("Download"),
            tr("Cannot start another download, video is already being downloaded."));
    }
    m_download_active = true;

    //Initiate download - using external downloader
    //because we don't have the address of the video file
    //It'll be saved as temp file (cleaned up automatically if not imported)
    //address is the address of the video player page, passed to downloader
    qInfo() << "requesting external download" << address;
    m_notifications->showNotification("download", "External download requested...");
    m_storage->downloadViaTool(address);
    //The name/path of the downloaded file is set in handleDownloadStarted()

    //When download begins, m_temp_file is set to the file being downloaded!
    //m_temp_file used on import
}

void
VideoView::downloadExternally()
{
    downloadExternally(m_src_address);
}

void
VideoView::downloadFile(const QUrl &url)
{
    if (m_download_active)
    {
        QMessageBox::critical(this, tr("Download"),
            tr("Cannot start another download, video is already being downloaded."));
    }
    m_download_active = true;
    m_http_download_requested = true;

    //Initiate file download - we're going to download the (selected) file
    //url is the actual url to the remote video file online
    m_notifications->showNotification("download", "Dwnload requested...");
    m_storage->downloadFile(url);

    //When download begins, m_temp_file is set to the file being downloaded!
    //m_temp_file used on import
}

void
VideoView::loadContext()
{
    QString address = m_src_context.value("url").toString();
    if (address.isEmpty()) return;

    //auto ctx = m_site->loadContext(address);
    //if (!ctx)
    //
    //contextLoaded();

    qDebug() << "loadContext() not implemented yet"; //TODO
}

void
VideoView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F4)
    {
        m_notifications->showNotification("", tr("Reloading player..."), 15);
        recreatePlayer();
    }
    else if (event->key() == Qt::Key_F5)
    {
        m_notifications->showNotification("", tr("Attempting to reload video..."), 15);
        loadUrl(m_src_address);
    }
    else if (event->key() == Qt::Key_F6)
    {
        m_notifications->showNotification("", tr("Attempting to download video..."), 15);
        downloadExternally();
    }

    QWidget::keyPressEvent(event);
}

void
VideoView::contextLoaded()
{
    //Video context loaded, ready to be displayed - title and description
    //Context usually contains metadata from wherever the video was opened
    //If an online video is loaded, one that was found in a channel,
    //the context object may contain the original title and other
    //metadata of the video and channel.
    //If a local video was opened, there may not be any metadata,
    //in which case the title could be the filename.
    //An online video may also be opened without any context,
    //in which case an attempt would be made to find and load metadata
    //into the context object.
    //In each of these cases, after some context has been loaded,
    //this function is called to display it somewhere.
    if (m_src_context.isEmpty()) return;

    //Title (could be blank)
    m_lbl_title->setText(title());

    //Description
    QString desc_text = m_src_context.value("description").toString();
    m_txt_desc->setPlainText(desc_text);
    if (!desc_text.isEmpty())
        m_btn_show_desc->setDisabled(false);

}

void
VideoView::showDescription()
{
    bool full_desc_visible = m_txt_desc->property("in_full").toBool();
    if (!full_desc_visible)
    {
        m_btn_show_desc->setText(tr("Hide"));
        m_btn_import->setVisible(false);
        m_btn_src->setVisible(false);

        m_txt_desc->setVisible(true);
        m_txt_desc->setDisabled(false);
        m_txt_desc->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_txt_desc->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
    else
    {
        m_btn_show_desc->setText(tr("More"));
        m_btn_import->setVisible(true);
        m_btn_src->setVisible(true);

        m_txt_desc->setVisible(false);
        m_txt_desc->setDisabled(true);
        m_txt_desc->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_txt_desc->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
    m_txt_desc->setProperty("in_full", !full_desc_visible);
}

void
VideoView::handleDownloadStarted(const QString &temp_file)
{
    //We use strings instead of QFileInfo objects because we need the path and:
    //> Warning: If filePath() is empty the behavior of this function is undefined.
    //https://doc.qt.io/qt-5/qfileinfo.html#absoluteFilePath
    QString file_path = temp_file; //not: temp_file.absoluteFilePath();
    qInfo() << "download started: video view got temp file to play" << file_path;
    m_temp_file = file_path;
    //m_temp_file_obj = file_obj; //TODO maybe later
    m_download_active = true;
    m_download_ready = false;

    //not yet starting playback at 0%, it would fail at this point
    //see progress handler
    //note that we want to play the temporary file before it's complete
}

void
VideoView::handleDownloadProgress(double p)
{
    m_notifications->showNotification("download", tr("Downloading: %1%").arg(p, 0, 'f', 1));

    //Add partial/temp file as source and play after given % downloaded
    //because if we loaded it at 0%, nothing would happen: cannot pre fill buffer
    double play_after_percent = 3; //play_after_min
    if (!isSourceFile(m_temp_file) && p > play_after_percent) //TODO very ugly, unreliable
    {
        //Try to add partial video file as source and start playback
        //If this fails, it should probably remove the bad source?
        addVideoSource(m_temp_file);
    }

    //TODO check if playback could be started! remove source on error
    //This happens when playback is started too early (file empty):
    //cache_read stream error: cannot pre fill buffer
    //mjpeg demux error: cannot peek
    //main input error: Your input can't be opened
    //main input error: VLC is unable to open the MRL

    //TODO update position bar of player?
}

void
VideoView::handleDownloadFailed()
{
    qInfo() << "video view received download error";
    m_notifications->showNotification("download", "Download failed!");
    m_download_active = false;
    m_download_ready = false;

    bool have_ext_downloader = false;
    m_storage->downloadToolExe(&have_ext_downloader);
    if (m_http_download_requested && have_ext_downloader)
    {
        m_http_download_requested = false;
        if (QMessageBox::question(this, tr("Download"),
            tr("The video could not be downloaded. Do you want to try it again, using the external downloader?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
        {
            downloadExternally();
        }
    }
}

void
VideoView::completeDownloadedVideo()
{
    //Mark download as completed
    m_download_active = false;
    m_download_ready = true;

    //Download Watcher says the video has been downloaded
    //temp_file is the file (path) which we may already be playing now
    //it's a random filename without proper extension
    //because it was defined before the type of video was known
    qInfo() << "video view - download completed" << m_temp_file;
    m_notifications->showNotification("download", "Download completed!", 30);

    //Now import downloaded file if requested
    if (m_import_requested)
    {
        importVideo();
    }

}

//void
//VideoView::addContext(const QVariantMap &ctx)
//{
//}

//add video source item, i.e., dict containing video file url
void
VideoView::addVideoSource(const QVariantMap &item)
{
    if (item.isEmpty()) return; //this is allowed to skip found() elsewhere

    qInfo() << this << "add video source - dict:" << item;
    m_video_items.append(item);

    QTimer::singleShot(0, this, SLOT(useVideoSources()));
}

void
VideoView::addVideoSource(const QUrl &url)
{
    if (url.isEmpty()) return;

    //add video item with url (http://... but could also be local file://...)
    qInfo() << this << "add video source - url:" << url.url();
    QVariantMap item;
    item["url"] = url;
    addVideoSource(item);
}

void
VideoView::addVideoSource(const QFileInfo &file)
{
    //add url to local file to src/playlist
    qInfo() << this << "add video source - file:" << file;
    QVariantMap item;
    item["url"] = QUrl::fromLocalFile(file.filePath());
    addVideoSource(item);
}

void
VideoView::addVideoSource(const QString &file)
{
    if (file.isEmpty()) return; //this is allowed to skip found() elsewhere

    //file is expected to be a local file
    qInfo() << this << "add video source - file:" << file;
    QVariantMap item;
    item["url"] = QUrl::fromLocalFile(file);
    addVideoSource(item);
}

void
VideoView::addVideoSource(const QVariant &var)
{
    if (var.isNull()) return;

    qInfo() << this << "add video source slot - variant...";
    if (var.canConvert<QVariantList>())
    {
        //List with video versions/sources received
        foreach (QVariant v, var.toList())
        {
            if (v.canConvert<QVariantList>())
            {
                //infinity
                loadingFailed();
                return;
            }
            addVideoSource(v);
        }
    }
    else if (var.canConvert<QVariantMap>())
    {
        addVideoSource(var.toMap());
    }
    //else if (var.canConvert<QUrl>())
    else if (var.userType() == QMetaType::QUrl)
    {
        QUrl url = var.toUrl();
        addVideoSource(url);
    }
    else if (var.canConvert<QString>())
    {
        QString file = var.toString();
        addVideoSource(file);
    }
    else
    {
        loadingFailed();
        return;
    }

}

//void
//VideoView::addTempVideoSource(const QString &file)
//{
//    //qInfo() << "video view got temp file to play" << file;
//    //_temp_src_file = file;
//    //addVideoSource(file);
//}

void
VideoView::useVideoSources()
{

    //Hide loader text/animation
    //TODO which one?

    //Update source button, add menu with available video sources
    if (m_btn_src->menu())
        m_btn_src->menu()->deleteLater();
    m_src_acts.clear();
    QMenu *menu = new QMenu;
    m_btn_src->setMenu(menu);
    connect(menu, SIGNAL(triggered(QAction*)), SLOT(selectSource(QAction*)));
    m_btn_src->setDisabled(false);
    for (int i = 0, ii = m_video_items.count(); i < ii; i++)
    {
        //Create action, item in video sources list
        QVariantMap item = m_video_items[i];
        //item is copied (not const&), but we're still afraid to use operator[]
        QString label;
        if (item.contains("file_title"))
            label = item["file_title"].toString();
        else if (item.value("url").canConvert<QUrl>())
            label = item["url"].toUrl().fileName();
        else if (item.value("url").canConvert<QString>())
            label = item["url"].toString();
        else
            label = "?";
        bool is_local_copy = false;
        if (item.value("url").canConvert<QUrl>())
        {
            QUrl url = item["url"].toUrl();
            is_local_copy = url.isLocalFile();
            label.prepend((is_local_copy ? tr("[COMPUTER]") : tr("[INTERNET]")) + " ");
        }
        QAction *action = menu->addAction(label); //label is source/file name
        action->setCheckable(true);
        action->setProperty("i", i);
        m_src_acts[i] = QPointer<QAction>(action);

        //Select first source to be played
        if (i == 0 && !m_vlc->isPlaying())
            selectSource(0);
    }

    //Update title to show video being played
    updateTitle();
    m_tmr_title->start();
}

void
VideoView::loadingFailed()
{
    qWarning() << "video view failed to load (via signal)";
}

void
VideoView::updateTitle()
{
    QString full_title = title();
    if (full_title.isEmpty()) return;
    int max_title = 13;
    if (full_title.size() <= max_title)
    {
        //Show full video title in tab
        emit setName(full_title, "", this);
    }
    else
    {
        //Show truncated part of video title in tab
        QString title_cut = full_title.mid(0, max_title) + "…"; // ...
        if (m_vlc->isPlaying())
        {
            //Slide animation
            //TODO animation disabled because tab size not fixed
            //TODO use monospace font
            //int slide_pos = _tmr_title->property("pos").toInt() + 1;
            //if (slide_pos >= full_title.length()) slide_pos = 0;
            //_tmr_title->setProperty("pos", slide_pos);
            int slide_pos = 0;
            title_cut = full_title.mid(slide_pos, max_title);
            if (slide_pos == 0) title_cut += "…"; // ...
            title_cut = "▷ " + title_cut;
        }
        emit setName(title_cut, full_title, this);
    }
    // ▷ <>…
    // earth emoji: 🌍
    //
    // TODO hSizePolicy = Fixed ?

}

void
VideoView::selectSource(int index)
{
    //safeguard - index must be valid
    if (index < 0 || index >= m_video_items.count())
    {
        qWarning() << "invalid source item selected:" << index; // TODO we need a SHORT alias for QString ... %s ... arg() arg () ...
        return;
    }

    //Get selected source dict
    QVariantMap item = m_video_items[index];
    //url is either a remote file or a local file
    QVariant v_url = item["url"];
    qDebug() << "selected video source" << index << v_url;

    //Remember selection
    m_src_index = index;
    foreach (int i, m_src_acts.keys())
        m_src_acts[i]->setChecked(i == index);
    //Start playback
    //NOTE loading again may cause a new VLC window to pop up
    m_vlc->load(v_url.toUrl());
    if (m_play_on_select)
        m_vlc->play();

}

void
VideoView::selectSource(QAction *action)
{
    int i = action->property("i").toInt();
    selectSource(i);
}

void
VideoView::selectSource()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (action) selectSource(action);
}

/**
 * Import video to local video storage (download location defined in profile).
 * When requested, what happens next depends on the available sources.
 * Depending on the type of site, the list of video sources
 * may contain one or more urls to video files (versions of the same video).
 * If multiple versions are available, the preferred one should be up top;
 * but if one is being played, that one should be imported.
 * However, if no download url could be determined (depending on the site),
 * the external downloader will be used.
 * At this point, the downloader would already be started (or done),
 * unless it's unavailable. It would have saved it to a temporary file
 * which will eventually be added to the list of sources.
 * The temporary file would be: _downloaded_tmp_file
 *
 * So if a download is currently in progress, we cannot do anything yet.
 * If that download is complete, we would import the temp file.
 * The temp file itself will be removed automatically
 * when this video view widget is destroyed.
 * And if we don't have a temp file but instead a video file url,
 * we'll start downloading that now.
 */
void
VideoView::importVideo()
{
    if (m_src_index < 0 || m_src_index >= m_video_items.size()) return;
    QVariantMap src_item = m_video_items[m_src_index];
    QUrl url = src_item["url"].toUrl();

    //No action if download already in progress
    if (m_download_active)
    {
        QMessageBox::warning(this, tr("Download active"),
            tr("This video is currently being downloaded. Please wait for the download to finish before importing it."));
        m_import_requested = true;
        return;
    }

    //Initiate download if we have a remote file url
    //Schedule file to be imported when downloaded
    if (m_temp_file.isEmpty())
    {
        //Check if we have a remote file to be downloaded
        if (url.isEmpty() || url.isLocalFile())
        {
            QMessageBox::critical(this, tr("Download"),
                tr("No video file source available for download/import!"));
            return;
        }
        //Inform user and start download
        m_notifications->showNotification("download", "Download requested...");
        QMessageBox::information(this, tr("Download"),
            tr("The video will be downloaded and imported. Do not close this video tab during the download."));
        downloadFile(url);
        m_import_requested = true;
        return;
    }

    //We have a downloaded (temp) video file - import it
    m_notifications->showNotification("import", "Now importing video...", 15);
    //TODO add channel name to context if missing
    //We should not move it while it's being played, so copy it
    //The temp file will automatically be removed when this view closes
    //Source context is provided to make the context available for later
    //and to allow the storage module to name the file
    bool move_file = false;
    //m_storage->scheduleImportFile(m_temp_file, m_src_address, m_src_context, move_file);
    m_storage->scheduleImportFile(m_temp_file, m_src_address, m_src_context);
    m_btn_import->setDisabled(true);
    //QMessageBox::information(this, tr("Import"), //TODO connect signal
    //    tr("The video has now been imported to your collection."));
    //Notify user when import done
    //TODO

}

bool
VideoView::isEmpty()
{
    return m_video_items.isEmpty();
}

int
VideoView::indexOfSourceItem(const QUrl &url)
{
    int index = -1;
    for (int i = m_video_items.size(); --i >= 0;)
        if (m_video_items[i].value("url").toUrl() == url) index = i;

    return index;
}

bool
VideoView::isSourceFile(const QString &file)
{
    QUrl url = QUrl::fromLocalFile(file);
    return indexOfSourceItem(url) != -1;
}

//select video source: internet (globe) or local
//settings: in profile, check for video url if already downloaded

