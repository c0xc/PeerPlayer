#ifndef VSITE_HPP
#define VSITE_HPP

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QEventLoop>
#include <QFile>
#include <QPointer>
#include <QRegExp>
#include <QPixmap>
#include <QElapsedTimer>

#include "profilesettings.hpp"

class ActionContext;
typedef QSharedPointer<ActionContext> ActionContextPtr;
typedef QPointer<ActionContext> ActionContextRef;

class VSiteBase : public QObject
{
    Q_OBJECT

public:

    static QByteArray
    encodeJson(const QVariant &var, bool *ok = 0);

    static QVariant
    parseJson(const QByteArray &bytes, bool *ok = 0);

    static QVariantMap
    parseJsonToMap(const QByteArray &bytes, bool *ok = 0);

    static VSiteBase*
    load(const QString &address, QObject *parent = 0);

    VSiteBase(const QString &address, QObject *parent = 0);

    QUrl
    siteUrl() const;

    QString
    domain() const;

    virtual ActionContextPtr
    download(const QString &address) = 0;

    virtual ActionContextPtr
    loadChannel(const QString &search_name) = 0;

    virtual ActionContextPtr
    loadChannel() = 0;

    virtual void
    loadSiteInfo() = 0;

    virtual void
    loadSiteName() = 0;

    virtual void
    loadChannelVideos(int index = -1) = 0;

    virtual void
    loadVideos(int index = -1) = 0;

    QString
    currentChannel() const;

    int
    pageOffset();

    int
    pageSize();

    virtual void
    loadThumbnail(const QString &url, QObject *obj) = 0;

    virtual ActionContextPtr
    loadVideo(const QVariantMap &item) = 0;

    virtual ActionContextPtr
    loadVideo(const QString &url) = 0;

    virtual QString
    normalizePlayerAddress(QString address);

protected:

    QString
    _get_action;

    QString
    _channel;

    int
    _page_o;

    int
    _page_size;

private:

    QUrl
    _url;

};

class ActionContext : public QObject
{
    Q_OBJECT

public:

    static ActionContextPtr
    create();

    ActionContext();

    ~ActionContext();

    bool
    isSignalEnabled() const;

    void
    enableSignal();

    QVariantMap
    map;

    QElapsedTimer
    timer;

    QPointer<QObject>
    originating_object;

    ActionContextPtr
    parent_action_ptr;

public slots:

    void
    start(ActionContextPtr ctx);

    void
    scheduleStart(ActionContextPtr ctx);

    void
    setValue(const QVariant &value);

signals:

    void
    started();

    void
    started(ActionContextPtr ctx);

    void
    failed(ActionContextPtr ctx = 0);

    void
    gotResult(const QVariant &value, ActionContextRef ctx = ActionContextRef());

    void
    finished(const QVariant &value);
    //ctx is passed as QPointer because caller should not prevent deletion
    void
    finishedRef(const QVariant &value, ActionContextRef ctx);

};

class VSite;
class VSite : public VSiteBase
{
    Q_OBJECT

    //TODO auth map static global auth site obj per domain...
    //vk site: uses one auth instance. that instance will handle auth actions
    //async: emit signal showAuthPage(QUrl) handled by main gui
    //which opens a tab or something...

signals:

    void
    loadedSiteName(const QString &name);

    void
    gotChannelName(const QString &name);

    void
    gotChannelInfo(const QVariantMap &channel);

    void
    loadedVideoItem(const QVariantMap &item);

    void
    loadedVideoList(const QVariantList &items);

    void
    loadedThumbnail(const QPixmap &pixmap, QObject *obj = 0);

    void
    loadedVideoUrl(const QString &url);

    void
    loadedVideoUrlItems(const QVariantList &items);

    void
    //showPageSignal(const QString &url, const ActionContextPtr &ctx);
    showPageSignal(const QString &url, const ActionContextRef &ctx);

public:

    static QVariantMap
    SiteApiConfig();

    /**
     * Takes a site channel url and returns an object to access it.
     * Only those sites are supported that are in the site api configuration.
     */
    static QPointer<VSite>
    load(const QString &address, QObject *parent = 0);

    //VSite(const QVariantMap &config, const QString &address, const QString &channel, QObject *parent = 0);
    VSite(const QVariantMap &config, const QString &address, QObject *parent = 0);
    VSite(const VSite &other, QObject *parent = 0);
    ~VSite();

private:

    //void
    //init...

public:

    ActionContextPtr
    download(const QString &address);

    QVariantList
    actionPlan(const QString &action);

    bool
    hasAction(const QString &action);

    ActionContextPtr
    createActionContext(const QString &name, QVariantMap stash = QVariantMap());

    /**
     * callAsync schedules the specified action to be started now.
     */
    ActionContextPtr
    call(ActionContextPtr ctx);

    /**
     * callAsync creates the specified action context and schedules it.
     * This will return the action context ctx and schedule the action
     * to be started after a short delay, to allow the caller
     * to connect to ctx (to receive the final result via ctx->finished).
     * It will be started after a delay, even if other actions are running.
     */
    ActionContextPtr
    callAsync(const QString &action, const QVariantMap &params = QVariantMap());

    /**
     * callAsync creates the specified action context and schedules it.
     * This will return the action context ctx and schedule the action
     * to be started when no other action is running.
     * If another action plan is currently running,
     * it will wait (non-blocking).
     */
    ActionContextPtr
    callWhenReady(const QString &action, const QVariantMap &params = QVariantMap(), int repeat = 0, ActionContextPtr ctx = 0);

    QVariant
    callWait(const QString &action);

    bool
    checkCompatibility();

    QString
    normalizePlayerAddress(QString address);

public slots:

    void
    setChannel(const QVariantMap &channel);
    void
    setChannel(const QString &channel);

    ActionContextPtr
    loadChannel(const QString &search_name);

    ActionContextPtr
    loadChannel();

    void
    loadSiteInfo();

    void
    loadSiteName();

    void
    loadChannelVideos(int index = -1);

    void
    loadVideos(int index = -1);

    void
    loadThumbnail(const QString &url, QObject *obj);

    ActionContextPtr
    loadVideo(const QVariantMap &item); //, ActionContextPtr ctx = 0

    ActionContextPtr
    loadVideo(const QString &url);

private slots:

    void
    checkTimeouts();

    void
    actionResult(ActionContextPtr ctx, const QVariant &var);

    void
    handleResult(const QVariant &var, ActionContextPtr ctx);

    void
    handleResult(const QVariant &var, const ActionContextRef ref);

    void
    handleError(const ActionContextPtr ctx);

    void
    parseReply(QNetworkReply *reply);

    void
    parseReply();

    void
    parseReply(QNetworkReply::NetworkError error);

    QVariant
    callAction(ActionContextPtr context);

private:

    ActionContextPtr
    getPendingActCtx(const ActionContextRef &ref);

    int
    actIf(const QVariantMap &action, const QVariantMap &stash, bool &ok);

    QNetworkRequest
    prepApiReq(const QVariantMap &action, const QVariantMap &stash, bool &ok);

    QVariant
    actApi(const ActionContextPtr &context, bool &ok, QPointer<QEventLoop> loop);

    QVariant
    actHttp(const ActionContextPtr &ctx, bool &ok);

    bool
    actShowPage(const ActionContextPtr &ctx, bool &ok);

    bool
    actSubAction(const ActionContextPtr &ctx, bool &ok);

    QVariant
    actGet(const QVariantMap &action, const QVariantMap &stash, bool &ok);

    QVariant
    actGet(const QString &key, const QVariantMap &stash, bool &ok);

    QVariant
    actMatch(const QVariantMap &action, const QVariantMap &stash, bool &ok);

    QVariant
    actSet(const QVariantMap &action, const QVariantMap &stash, bool &ok);

    QVariant
    actSet(const QString &pattern, const QVariantMap &stash, bool &ok);

    QVariant
    actArray(const QVariantMap &action, int action_index, QVariantMap &stash, bool &ok);

    QVariant
    actAppend(const QVariantMap &action, const QVariantMap &stash, bool &ok);

    int
    actYieldContinue(ActionContextPtr ctx, bool &ok);

    void
    actDest(const QVariantMap &action, const QVariant &value, QVariantMap &stash, bool &ok);

    QVariantMap
    globalVariables();

    VSite*
    getOrCreateControlInstance();

    //static QMap<QString, QPointer<VSite>>
    //_site_global_instance;

    QPointer<QNetworkAccessManager>
    _net;

    QMap<QNetworkReply*, ActionContextPtr>
    _act_reply_state;

    QList<ActionContextPtr>
    _act_active;

    QVariantMap
    _vars;

    QVariantMap
    _conf;

    QPointer<VSite>
    _auth_instance;

};

#endif
