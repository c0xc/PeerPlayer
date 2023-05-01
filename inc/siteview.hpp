#ifndef SITEVIEW_HPP
#define SITEVIEW_HPP

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
#include <QKeyEvent>
#include <QImageReader>
#include <QBuffer>
#include <QPainter>

#include "vsite.hpp"
#include "imagewidget.hpp"
#include "videostorage.hpp"
#include "gui.hpp"

//class ImageWidget;

class SiteView : public QWidget
//class SiteView : public QWidget, protected LogLoggerBase //TODO
{
    Q_OBJECT

signals:

    void
    setName(const QString &name, QWidget *self = 0);

    void
    openVideoSignal(const QString &url, const QVariantMap &item);

    void
    showPageSignal(const QString &url, const ActionContextRef &ctx);

public:

    SiteView(QString address, QWidget *parent = 0);

public slots:

private slots:

    void
    keyPressEvent(QKeyEvent *event);

    void
    loadChannel();

    void
    loadChannelFailed();

    void
    unloadFail();

    void
    setSiteName(const QString &name);

    void
    setChannel(const QString &name);

    void
    setChannel(const QVariantMap &channel);

    //void
    //recvChannels(QStringList &list);

    void
    setVideoList(const QVariantList &items);

private:

    QPointer<VSiteBase>
    m_site; //TODO QSharedPointer

    QString
    m_current_channel;

    NotificationBarWidget
    *m_notifications;

    QLabel
    *lbl_site_name;

    QLabel
    *lbl_channel_name;

    QScrollArea
    *scr_main;

};

class VideoListView : public QFrame
{
    Q_OBJECT

signals:

    void
    openVideoSignal(const QString &url, const QVariantMap &item);

public:

    struct Item
    {
        QPointer<QObject> thumb;
        QPointer<QObject> lbl_title;
        QPointer<QObject> lbl_description;
    };

    //ChannelView(VSiteBase *site, QString name, bool full_view = false, QWidget *parent = 0);
    VideoListView(VSiteBase *site, const QVariantMap &channel, QFrame *parent = 0);

public slots:

    void
    loadThumbnail(const QString &url, ImageWidget *thumb);

    void
    setVideoList(const QVariantList &items);

    void
    loadPage(int index);

    void
    loadPreviousPage();

    void
    loadNextPage();

    void
    openVideo(const QVariantMap &item);

    void
    openVideo();

private slots:

//TODO sort
private:

    QPointer<VSiteBase>
    m_site;

    VideoStorage
    *m_storage;

    QList<Item>
    m_items;

    QVariantMap
    m_ch_info;

    QWidget
    *wid_list;

    QLabel
    *lbl_title;

    int
    m_current_page;

    int
    m_page_size; // 5

};

class VideoLinkWidget : public QFrame
{
    Q_OBJECT

signals:

public:

    VideoLinkWidget(QString address, QWidget *parent = 0);



};

#endif
