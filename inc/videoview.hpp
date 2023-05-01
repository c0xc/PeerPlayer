#ifndef VIDEOVIEW_HPP
#define VIDEOVIEW_HPP

#include <QDebug>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPointer>
#include <QScrollArea>
#include <QMessageBox>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QTextEdit>

#include "vlcplayer.hpp"
#include "vsite.hpp"
#include "videostorage.hpp"
#include "gui.hpp"

class VideoView : public QWidget
{
    Q_OBJECT

signals:

    void
    setName(const QString &title, const QString &full_title, QWidget *view);

    void
    loadedSources(const QVariantList &items);

public:

    VideoView(QWidget *parent = 0);
    VideoView(const QString &address, const QVariantMap &context = QVariantMap(), QPointer<VSiteBase> site = 0, bool play_on_select = false, QWidget *parent = 0);
    //VideoView(const QVariantMap &item, QWidget *parent = 0);
    VideoView(const QFileInfo &fi, bool play_on_select = false, QWidget *parent = 0);

private:

    void
    initWidgets();

    void
    recreatePlayer();

public:

    void
    setPlayOnSelect(bool enabled = true);

    QString
    title();

    void
    loadUrl(const QString &address);

    void
    downloadExternally(const QString &address);

    void
    downloadExternally();

    void
    downloadFile(const QUrl &url);

    void
    loadContext();

private slots:

    void
    keyPressEvent(QKeyEvent *event);

    void
    contextLoaded();

    void
    showDescription();

    void
    handleDownloadStarted(const QString &temp_file);

    void
    handleDownloadProgress(double p);

    void
    handleDownloadFailed();

    void
    completeDownloadedVideo();

    //void
    //addContext(const QVariantMap &ctx);

    void
    addVideoSource(const QVariantMap &item);

    void
    addVideoSource(const QUrl &url);

    void
    addVideoSource(const QFileInfo &file);

    void
    addVideoSource(const QString &file);

    void
    addVideoSource(const QVariant &var);

    void
    useVideoSources();

    void
    loadingFailed();

    void
    updateTitle();

    void
    selectSource(int index);

    void
    selectSource(QAction *action);

    void
    selectSource();

    void
    importVideo();

private:

    bool
    isEmpty();

    int
    indexOfSourceItem(const QUrl &url);

    bool
    isSourceFile(const QString &file);

    QPointer<VSiteBase>
    m_site;

    //Video context - how it was found+opened, e.g., remote url or local file
    QVariantMap
    m_src_context;
    QString
    m_src_address;
    QString
    m_src_file;

    //Video source items that were identified
    //If the video context is a remote file, that could be the first source
    //followed by a downloaded copy of that video.
    QList<QVariantMap>
    m_video_items;

    int
    m_src_index;

    QString
    m_temp_file;

    QPointer<QFile>
    m_temp_file_obj; //TODO unused

    bool
    m_http_download_requested;

    bool
    m_download_active;

    bool
    m_download_ready;

    bool
    m_import_requested;

    bool
    m_play_on_select;

    QWidget
    *m_wid_player;

    QVBoxLayout
    *m_vbox_player_container;

    VlcPlayer
    *m_vlc;

    NotificationBarWidget
    *m_notifications;

    QLabel
    *m_lbl_title;

    QLabel
    *m_btn_desc_prev;

    QPushButton
    *m_btn_show_desc;

    QPushButton
    *m_btn_import;

    QPushButton
    *m_btn_src;

    QMap<int, QPointer<QAction>>
    m_src_acts;

    QTextEdit
    *m_txt_desc;

    QPushButton
    *m_btn_hide_desc;

    QTimer
    *m_tmr_title;

    VideoStorage
    *m_storage;

};

#endif
