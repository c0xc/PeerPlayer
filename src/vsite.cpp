#include "vsite.hpp"


QSharedPointer<ActionContext>
ActionContext::create()
{
    return QSharedPointer<ActionContext>(new ActionContext);
}

ActionContext::ActionContext()
             : QObject()
{
    //connect(this, SIGNAL(started()), [this]() { this->timer.start(); });
    //connect(this, SIGNAL(started()), SLOT(startTimer()));
}

ActionContext::~ActionContext()
{
    qDebug() << "dtor" << map["name"].toString() << this;
}

bool
ActionContext::isSignalEnabled() const
{
    return map["_signal_on"].toBool();
}

void
ActionContext::enableSignal()
{
    map["_signal_on"] = true;
}

void
ActionContext::start(ActionContextPtr ctx)
{
    timer.start();
    emit started();
    emit started(ctx);
}

void
ActionContext::scheduleStart(ActionContextPtr ctx)
{
    QTimer::singleShot(0, this, [this, ctx]() { this->start(ctx); });
}

void
ActionContext::setValue(const QVariant &value)
{
    emit gotResult(value, ActionContextRef(this));
}

QByteArray
VSiteBase::encodeJson(const QVariant &var, bool *ok)
{
    QByteArray json_bytes;

    QVariant container = var;
    if (var.canConvert<QString>())
        container = QStringList() << var.toString();
    QJsonDocument j_doc = QJsonDocument::fromVariant(container);
    if (ok) *ok = !j_doc.isNull();
    json_bytes = j_doc.toJson();

    return json_bytes;
}

QVariant
VSiteBase::parseJson(const QByteArray &bytes, bool *ok)
{
    QJsonParseError json_error;
    QJsonDocument j_doc = QJsonDocument::fromJson(bytes, &json_error);
    if (ok) *ok = (json_error.error == QJsonParseError::NoError);
    QVariant var = j_doc.toVariant();

    return var;
}

QVariantMap
VSiteBase::parseJsonToMap(const QByteArray &bytes, bool *ok)
{
    QVariantMap map;

    QJsonParseError json_error;
    QJsonDocument j_doc = QJsonDocument::fromJson(bytes, &json_error);
    if (ok) *ok = (json_error.error == QJsonParseError::NoError);
    QJsonObject j_obj = j_doc.object();
    QVariant var = j_obj.toVariantMap();
    map = var.toMap();

    return map;
}

VSiteBase::VSiteBase(const QString &address, QObject *parent)
         : QObject(parent),
           _page_o(0), _page_size(20),
           _url(address)
{
}

QUrl
VSiteBase::siteUrl() const
{
    return _url;
}

QString
VSiteBase::domain() const
{
    QString domain = this->siteUrl().host();
    return domain;
}

QString
VSiteBase::currentChannel() const
{
    return _channel;
}

int
VSiteBase::pageOffset()
{
    return _page_o;
}

int
VSiteBase::pageSize()
{
    return _page_size;
}

QString
VSiteBase::normalizePlayerAddress(QString address)
{
    return address;
}

QVariantMap
VSite::SiteApiConfig()
{
    QVariantMap map;

    //Map contains supported sites and instructions to use their api
    QFile file(":/site_api_config.json");
    if (!file.open(QIODevice::ReadOnly))
        return map; //error file.errorString()

    return parseJsonToMap(file.readAll());
}

QPointer<VSite>
VSite::load(const QString &address, QObject *parent)
{
    QPointer<VSite> site;

    //Sort available site configurations
    //Those that can immediately identify a site (based on the address) first
    //Those that require an api request last
    //We don't want to send spam api queries to sites that are obviously
    //not compatible...
    QVariantMap full_config = SiteApiConfig();
    QStringList site_keys = full_config.keys();
    foreach (QString name, site_keys)
    {
        bool api_req_for_comp_check = false;
        foreach (QVariant action, full_config[name].toMap().value("compatibility").toList())
        {
            if (action.toMap().contains("api"))
                api_req_for_comp_check = true;
        }
        if (api_req_for_comp_check)
            site_keys.append(site_keys.takeAt(site_keys.indexOf(name)));
    }

    //Try to find compatible site config
    foreach (QString name, site_keys)
    {
        QVariantMap cfg = full_config[name].toMap();
        VSite current_site(cfg, address, parent);
        if (!current_site.checkCompatibility())
            continue;
        //qDebug("compatible %s site detected: %s", name, address); //TODO fix this
        qInfo() << "compatible site detected:" << name << address;

        //Found compatible site config
        //*site = current_site; TODO use copy constructor
        //site = new VSite(cfg, address, parent);
        site = new VSite(current_site, parent);
        break; //stop checking
    }

    return site;
}

//VSite::VSite(const QVariantMap &config, const QString &address, QObject *parent)
//     : VSiteBase(address, parent),
//       _conf(config)
//{
//    //Initialize network client
//    _net = new QNetworkAccessManager(this); //TODO double-check timeout + delete
//    connect(_net, SIGNAL(finished(QNetworkReply*)), this, SLOT(parseReply(QNetworkReply*)));
//    _net->setTransferTimeout(10000); //Qt 5.15?
//
//    //Extract channel name (search name) from url, if available
//    //(If not, loadChannel(search_name) must be used before loading videos)
//    QVariant v_channel = callWait("extract_channel_name");
//    if (!v_channel.isNull())
//    {
//        QString search_name = v_channel.toString();
//        if (!search_name.isEmpty())
//            loadChannel(search_name);
//    }
//
//    //Load 
//
//    //loadChannelName(); //TODO obsolete!
//    //QTimer::singleShot(0, this, SLOT(loadChannelName()));
//    connect(this, SIGNAL(gotChannelName(const QString&)), SLOT(setChannel(const QString&)));
//    connect(this, SIGNAL(gotChannelInfo(const QVariantMap&)), SLOT(setChannel(const QVariantMap&)));
//    loadChannelName(); //may be async; get_channel_videos should be queued
//
//    QTimer *timeout_timer = new QTimer(this);
//    timeout_timer->setInterval(2000);
//    timeout_timer->start();
//    connect(timeout_timer, SIGNAL(timeout()), SLOT(checkTimeouts()));
//
//}
//
//1) wrong: this called channel_info in comp check but not afterwards

//VSite::VSite(const QVariantMap &config, const QString &address, const QString &channel, QObject *parent)
//     : VSiteBase(address, parent),
//       _conf(config)
//{
//    qDebug() << this << "VSite ctor 0";
//    //Initialize network client
//    _net = new QNetworkAccessManager(this); //TODO double-check timeout + delete
//    connect(_net, SIGNAL(finished(QNetworkReply*)), this, SLOT(parseReply(QNetworkReply*)));
//    _net->setTransferTimeout(10000); //Qt 5.15?
//
//    //Initialize timeout timer
//    QTimer *timeout_timer = new QTimer(this);
//    timeout_timer->setInterval(2000);
//    timeout_timer->start();
//    connect(timeout_timer, SIGNAL(timeout()), SLOT(checkTimeouts()));
//
//    //Extract channel name (search name) from url, if available
//    //In this ctor, we don't extract the channel from the url, see other ctor.
//    //Here, we merely load the specified channel (3rd argument) if defined.
//    //If that's not defined (empty), loadChannel() must be called explicitly.
//
//    //Make sure CHANNEL variable is set when channel loaded / received
//    connect(this, SIGNAL(gotChannelName(const QString&)), SLOT(setChannel(const QString&)));
//    connect(this, SIGNAL(gotChannelInfo(const QVariantMap&)), SLOT(setChannel(const QVariantMap&)));
//    //Load channel if specified
//    if (!channel.isEmpty())
//    {
//        qDebug() << "VSite ctor loading channel" << channel;
//        loadChannel(channel);
//    }
//
//}
//
//VSite::VSite(const QVariantMap &config, const QString &address, QObject *parent)
//     //: VSiteBase(address, parent),
//     : VSite(config, address, "", parent) //call default ctor without channel
//{
//    qDebug() << this << "VSite ctor 1";
//    //Extract channel name (search name) from url, if available
//    //This will extract the channel name from the user-provided address
//    //(If not, loadChannel(search_name) must be used before loading videos)
//    QVariant v_channel = callWait("extract_channel_name");
//    if (!v_channel.isNull())
//    {
//        QString search_name = v_channel.toString();
//        if (!search_name.isEmpty())
//        {
//            qDebug() << "VSite ctor loading channel (extracted)" << search_name;
//            loadChannel(search_name);
//        }
//    }
//}
//new:

VSite::VSite(const QVariantMap &config, const QString &address, QObject *parent)
     : VSiteBase(address, parent),
       _conf(config),
       _vars(config.value("vars").toMap())
{
    qDebug() << this << "VSite ctor 0";
    //Initialize network client
    _net = new QNetworkAccessManager(this); //TODO double-check timeout + delete
    connect(_net, SIGNAL(finished(QNetworkReply*)), this, SLOT(parseReply(QNetworkReply*)));
    _net->setTransferTimeout(10000); //Qt 5.15?

    //Initialize timeout timer
    QTimer *timeout_timer = new QTimer(this);
    timeout_timer->setInterval(2000);
    timeout_timer->start();
    connect(timeout_timer, SIGNAL(timeout()), SLOT(checkTimeouts()));

    //Make sure CHANNEL variable is set when channel loaded / received
    connect(this, SIGNAL(gotChannelName(const QString&)), SLOT(setChannel(const QString&))); //TODO obsolete !!!
    connect(this, SIGNAL(gotChannelInfo(const QVariantMap&)), SLOT(setChannel(const QVariantMap&)));

    //Extract channel name (search name) from url, if available
    //This will extract the channel name from the user-provided address
    //and CHANNEL.name will be set so that load_channel can be called next
    //Note that we will not fetch channel info nor emit such a signal
    //it could be that this instance is only used to check for compatibility
    //and when it's copied, the copy won't fetch/emit anything either
    QVariant v_channel = callWait("extract_channel_name");
    if (!v_channel.isNull() && v_channel.userType() != QMetaType::Bool)
    {
        //extract action returned something
        QString search_name = v_channel.toString();
        qDebug() << "VSite ctor extracted channel from url:" << search_name;
        setChannel(search_name);
    }

}

VSite::VSite(const VSite &other, QObject *parent)
     : VSite(other._conf, other.siteUrl().url(), parent)
{
    qDebug() << this << "VSite cpy ctor";
    //Copy ctor, copy address and state (partially), do *not* trigger actions
    //If other is already loaded, it doesn't make sense to load it again.
    //If the site uses a token and if that token has expired,
    //it is not our job to update it. Such a site would have to have a
    //global instance (used by all instances of the same type)
    //and that global instance would update the token
    //when asked by one of the client vsite instances.
    if (!other.currentChannel().isEmpty())
        setChannel(other.currentChannel());
}

VSite::~VSite()
{
    qDebug() << this << "dtor";
}

ActionContextPtr
VSite::download(const QString &address)
{
    QString address_fixed = address;
    if (address.startsWith("/"))
    {
        QUrl url = siteUrl().resolved(QUrl(address));
        address_fixed = url.url();
    }

    QVariantList plan; //= actionPlan(name);
    QVariantMap a;
    a["http"] = address_fixed;
    a["type"] = "get";
    a["raw"] = true;
    a["dest"] = "bytes";
    plan << a;
    QVariantMap a2;
    a2["return"] = "bytes";
    plan << a2;

    ActionContextPtr ctx = ActionContextPtr::create();
    ctx->map["name"] = "download";
    ctx->map["plan"] = plan;
    ctx->enableSignal();

    return call(ctx);
}

QVariantList
VSite::actionPlan(const QString &action)
{
    QVariantList plan;

    //Get configured list of actions
    plan = _conf.value(action).toList();

    return plan;
}

bool
VSite::hasAction(const QString &action)
{
    //TODO enum action-type ...
    return !actionPlan(action).isEmpty();
}

ActionContextPtr
VSite::createActionContext(const QString &name, QVariantMap stash)
{
    QVariantList plan = actionPlan(name);
    if (plan.isEmpty()) return 0; //error - action not found
    QVariantMap stash_vars = globalVariables();
    foreach (QString k, stash.keys())
    {
        if (k == "VARS") continue;
        stash_vars[k] = stash[k];
    }

    //Action context object is created and its map is filled
    //ctx is not yet added to the list of pending action plans!
    //see callAction() (which executes it), it removes it from list when done
    ActionContextPtr ctx = ActionContextPtr::create();
    ctx->map["name"] = name;
    ctx->map["plan"] = plan; //index 0 => start with first action in plan async
    ctx->map["stash"] = stash_vars;
    //ctx is added to list when callAction() starts

    return ctx;
}

ActionContextPtr
VSite::call(ActionContextPtr ctx)
{
    qDebug() << this << "callAsync - initiating action plan" << ctx.data();
    //Schedule action plan to be executed but don't start it immediately
    //This delay is necessary to allow the caller to connect or modify
    //the context object that is returned
    connect(ctx.data(), SIGNAL(started(ActionContextPtr)), SLOT(callAction(ActionContextPtr)), Qt::UniqueConnection);
    ctx->scheduleStart(ctx);
    return ctx;
}

ActionContextPtr
VSite::callAsync(const QString &action, const QVariantMap &params) //ActionContextPtr ctx, bool start_later)
{
    //Create action context for specified action plan
    ActionContextPtr ctx = createActionContext(action, params);
    //call action async (after delay), return context pointer for result signal
    return call(ctx);
}

ActionContextPtr
VSite::callWhenReady(const QString &action, const QVariantMap &params, int repeat, ActionContextPtr ctx)
{
    //If a call is currently pending, reschedule this new execution
    //If idle, it's scheduled to start now
    //But either way - ctx is returned, so the caller can connect to it first
    if (repeat == 0) qDebug() << this << "scheduling action" << action << params;
    if (!ctx) ctx = createActionContext(action, params);
    if (!_act_active.isEmpty())
    {
        //Other action(s) active, reschedule until idle
        QStringList pending_actions;
        foreach (ActionContextPtr ctx, _act_active)
            pending_actions << ctx->map["name"].toString();
        qDebug() << this << "postponing call!" << action << _act_active.count() << "pending, waiting for" << pending_actions.join(", ") << repeat;
        repeat++;
        QTimer::singleShot(1000, this, [this, action, params, repeat, ctx]()
        {
            this->callWhenReady(action, params, repeat, ctx);
        });
        return ctx;
        //TODO semap...mutex...sort by call time
    }
    qDebug() << "call when ready - now ready" << ctx.data();

    //Start execution plan
    return call(ctx);
}

QVariant
VSite::callWait(const QString &action)
{
    auto ctx = createActionContext(action);
    ctx->map["index"] = -1; //-1 makes it run sync/blocking
    QVariant var = callAction(ctx);
    return var;
}

bool
VSite::checkCompatibility()
{
    QVariant var = callWait("compatibility");
    if (!var.isNull() && var.toBool()) return true;
    else return false;
}

QString
VSite::normalizePlayerAddress(QString address)
{
    QString norm_addr = address;
    if (hasAction("normalize_video_url"))
    {
        QVariant var = callWait("normalize_video_url");
        if (!var.isNull() && var.userType() == QMetaType::QString)
            norm_addr = var.toString();
    }
    return norm_addr;
}

void
VSite::setChannel(const QVariantMap &channel)
{
    qDebug() << this << "set channel:" << channel;
    //TODO ucase in globalvars
    //Save channel info in global CHANNEL variable
    _vars["CHANNEL"] = channel;
    _channel = channel.value("name").toString(); //base var
}

void
VSite::setChannel(const QString &channel)
{
    //Channel name set (probably not provided by the site)
    _channel = channel; //base var
    qDebug() << this << "set channel name:" << channel;
    //Save name in global CHANNEL variable
    QVariantMap channel_info;
    channel_info["name"] = channel;
    setChannel(channel_info);
}

ActionContextPtr
VSite::loadChannel(const QString &search_name)
{
    qDebug() << this << "loading channel info" << search_name;
    QVariantMap stash;
    stash["name"] = search_name;
    //run get_channel action plan async
    ActionContextPtr ctx = callAsync("get_channel", stash);
    //when done/ok, got_channel_info will emit gotChannelInfo()
    //calls to get_channel_videos must either wait for the signal
    //or use callWhenReady()
    return ctx;
}

ActionContextPtr
VSite::loadChannel()
{
    //load channel info (fetch from site), channel name previously set
    QString name = currentChannel();
    if (name.isEmpty())
    {
        qWarning() << "cannot load channel - no search name provided";
        return ActionContextPtr(); //return nullpointer
    }
    return loadChannel(name);
}

void
VSite::loadSiteInfo()
{
}

void
VSite::loadSiteName()
{
    callAsync("get_name");
}

void
VSite::loadChannelVideos(int index)
{
    if (index >= 0)
        _page_o = index;
    _get_action = "get_channel_videos"; // remember type of search query
    callWhenReady("get_channel_videos");
}

void
VSite::loadVideos(int index)
{
    if (index >= 0)
        _page_o = index;
    QString get_action = _get_action;
    if (get_action.isEmpty())
    {
        qWarning() << "cannot load video list: no search defined";
        return;
    }
    callWhenReady(get_action);
}

void
VSite::loadThumbnail(const QString &url, QObject *obj)
{
    QUrl req_url = url;
    if (url.startsWith("/"))
    {
        req_url = siteUrl().resolved(QUrl(url));
    }
    QNetworkRequest req(req_url);
    req.setOriginatingObject(obj);
    QNetworkReply *reply = _net->get(req);
    reply->setProperty("name", "get_thumbnail");
    //TODO add action?
    connect(reply, SIGNAL(finished()), SLOT(parseReply()));
}

/**
 * Load video, get sources for download.
 * item is the source object, as returned by the list action (get_channel_videos).
 * It's a QVariantMap which either contains one or more "*url" keys
 * or other, site-specific keys that will have to be assembled somehow
 * to get the download address.
 * The content of this map is put onto the stash,
 * to be used by the get_video action(s).
 * The expected response is one or more maps, one for each available source,
 * where each map must at least contain a "url" and should contain a "title".
 */
ActionContextPtr
VSite::loadVideo(const QVariantMap &item)
{
    QString action = "get_video_url";
    if (!actionPlan("get_video_urls").isEmpty())
        action = "get_video_urls";
    if (actionPlan(action).isEmpty())
        return 0;
    return callAsync(action, item);
}

ActionContextPtr
VSite::loadVideo(const QString &url)
{
    QVariantMap item;
    item["url"] = url;
    return loadVideo(item);
}

void
VSite::checkTimeouts()
{
    foreach (ActionContextPtr ctx, _act_active)
    {
        int timeout = 600;
        //int timeout = 60; //TODO setting
        if (!(ctx->timer.elapsed() > timeout * 1000)) continue;
        QString name = ctx->map["name"].toString();
        qWarning() << "timeout" << ctx->timer.elapsed() << "ms" << name << this << ctx.data();
        _act_active.removeAll(ctx);
    }
    //QMap<QNetworkReply*, ActionContextPtr>
    //_act_reply_state;
}

void
VSite::actionResult(ActionContextPtr ctx, const QVariant &var)
{
    QString action = ctx->map["name"].toString();
    qDebug() << "forwarding completed action" << action << ctx.data() << ctx->isSignalEnabled();
    _act_active.removeAll(ctx); //remove context from queue, allow deletion

    //Keep this (last) result in cache
    //Note that this is the action result, triggered by a yield/return,
    //not any sub result of previous steps
    ctx->map["value"] = var;

    //If enabled (by action initiator, view), result signal is sent via ctx,
    //not to all listeners of site instance.
    //finished is emitted after the last action before ctx goes out of scope
    if (ctx->isSignalEnabled())
    {
        qDebug() << "forwarding via action signal" << ctx.data();
        ctx->map["value"] = var;
        emit ctx->finished(var);
        emit ctx->finishedRef(var, ActionContextRef(ctx.data()));
        return;
    }

    //Forward result of async-triggered action via signal
    if (action == "get_name")
        emit loadedSiteName(var.toString());
    else if (action == "get_channel")
        if (var.canConvert<QVariantMap>())
            emit gotChannelInfo(var.toMap());
        else
            emit gotChannelName(var.toString());
    else if (action == "get_channel_videos")
        if (var.canConvert<QVariantList>())
            emit loadedVideoList(var.toList());
        else
            emit loadedVideoItem(var.toMap());
    else if (action == "get_video_url")
        if (var.canConvert<QVariantList>())
            emit loadedVideoUrlItems(var.toList());
        else
            emit loadedVideoUrl(var.toString());

}

void
VSite::handleResult(const QVariant &var, ActionContextPtr ctx)
{
    if (!ctx)
    {
        qWarning() << "result received but no pending action identified, discarding" << this << ctx;
        return;
    }

    //Collect parameters
    QString name = ctx->map["name"].toString();
    QVariantList plan = ctx->map["plan"].toList();
    int index = ctx->map["index"].toInt();
    QVariantMap stash = ctx->map["stash"].toMap();
    QVariantMap action = plan[index].toMap();
    qDebug() << "got result value for running action plan" << name << "index:" << index << "bytes:" << var.toByteArray().size() << this << ctx;

    //Store result value in stash
    bool store_ok = true;
    actDest(action, var, stash, store_ok);
    ctx->map["stash"] = stash; //save updated stash in context

    //Continue with next step
    //it's important that we keep, pass ctx as the caller may wait for a signal
    ctx->map["index"] = index + 1;
    callAction(ctx);
}

void
VSite::handleResult(const QVariant &var, const ActionContextRef ref)
{
    //Received result for action identified by ctx
    //sub action should not pass/use/keep shared pointer
    //now identify ActionContextPtr which matches ctx
    handleResult(var, getPendingActCtx(ref));
}

void
VSite::handleError(const ActionContextPtr ctx)
{
    if (!ctx)
    {
        qWarning() << "error received but no pending action identified, discarding";
        return;
    }
    qDebug() << "action failed" << this << ctx.data();

    //Clear action to make it impossible to continue
    QVariantMap old_action = ctx->map;
    //not ctx->map.clear() because action loop wouldn't know where to continue
    foreach (QString k, ctx->map.keys())
        if (k != "plan" && k != "index")
            ctx->map.remove(k);
    ctx->map["_old"] = old_action;
    //Set error flag, will be checked in action loop which will emit failed
    ctx->map["failed"] = true;

    //Resume action loop with same index to trigger failed there
    //not callAction(ctx) (action "" failed... just finish it here)
    //Trigger failed, manual cleanup
    emit ctx->failed(ctx);
    _act_active.removeAll(ctx);
}

void
VSite::parseReply(QNetworkReply *reply)
{
    //Remember context from which call was made
    //If found, ActionContext object is removed from run-state-map
    //It's a shared pointer, so its QObject will be auto-deleted
    if (!_act_reply_state.contains(reply)) return;
    ActionContextPtr ctx = _act_reply_state.take(reply);
    reply->deleteLater();

    //Handle response depending on request type
    QVariantMap action = ctx->map["action"].toMap();
    if (action.value("type").toString() == "head")
    {
        //HEAD, just check success
        bool is_ok = reply->errorString().isEmpty(); //TODO set failed flag in error slot
        if (!is_ok)
            qWarning() << "query failed." << reply->errorString();
        handleResult(is_ok, ctx); //put result on stash, continue
    }
    else
    {
        //Parse response object

        //Parse received data, get result value
        //result value is map because it's json
        bool parsed_ok = false;
        QVariant result;
        if (action.value("raw").toBool())
            result = reply->readAll();
        else
            result = parseJson(reply->readAll(), &parsed_ok);
        handleResult(result, ctx); //put result on stash, continue
    }
}

void
VSite::parseReply()
{
    QObject *o = QObject::sender();
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(o);

    if (reply)
    {
        QString name = reply->property("name").toString();
        if (name == "get_thumbnail")
        {
            //TODO special case... make it generic
            QObject *obj = reply->request().originatingObject();
            QByteArray ba = reply->readAll();
            QPixmap pixmap;
            pixmap.loadFromData(ba);
            emit loadedThumbnail(pixmap, obj);
        }
    }

    //TODO no need to forward to parseReply(reply) ?
}

void
VSite::parseReply(QNetworkReply::NetworkError error)
{
    QObject *o = QObject::sender();
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(o);
    qWarning() << "ERROR" << error;
}

//TODO we have multiple bugs using MAP[key] to check for something which is then created !!! 
QVariant
VSite::callAction(ActionContextPtr ctx)
{
    //Execute action (defined in plan)
    //It works with a stash of variables, runs actions which modify variables.
    //An action fetches/loads/generates data and stores the value in the stash.
    QVariantMap stash = ctx->map["stash"].toMap();
    const QVariantList plan = ctx->map["plan"].toList();
    if (plan.isEmpty()) return QVariant();

    //If index == -1, run all actions, wait for them (blocking) and return result
    //If index > -1, run actions starting with action at index and:
    //on async action, return (on reply, this func will be called again)
    //If one action fails, the result is invalid/false (bool, not QMap)

    //TODO IDEA inherit debug object here? to prepend common prefix with pointer

    //check start index, log
    qDebug() << this << "call" << ctx->map.value("name").toString() << ctx.data() << "action signal:" << ctx->isSignalEnabled();
    if (ctx->map["index"].toInt() < 0)
        qDebug() << "running action plan (blocking)" << ctx->map.value("name").toString();
    else if (ctx->map["index"].toInt() == 0)
        qDebug() << "starting action plan" << ctx->map.value("name").toString();
    else
        qDebug() << "resuming action plan" << ctx->map.value("name").toString();
    //add action to list of active actions
    if (ctx->map["index"].toInt() == 0 && !_act_active.contains(ctx))
        _act_active.append(ctx);
    bool blocking = ctx->map["index"].toInt() < 0; //start_index < 0;
    bool all_ok = false;
    for (int i = 0; i <= plan.count(); i++) //plan elements plus 1
    {
        //Get current action; if async called via slot
        if (!blocking) //async calls allowed, start index defined
        {
            if (i < ctx->map["index"].toInt()) continue; //skip already performed actions
        }
        if (i == plan.count())
        {
            all_ok = true;
            break;
        }
        QVariantMap action = plan[i].toMap();
        action["name"] = ctx->map["name"].toString(); //for forwarding, see continue action
        ctx->map["index"] = i; //update index (current action being executed)
        ctx->map["action"] = action;
        qDebug() << this << "iteration" << i << "action" << action;

        //stop if previous action failed
        if (ctx->map.take("timeout").toBool()) break;
        if (ctx->map.take("failed").toBool()) action.clear(); //emit failed...

        //put call context into container variable
        //auto context = createActionContext(name, plan, i, stash);

        //handle action, write result to value
        QVariant value;
        bool ok = false;
        //action handling
        qDebug("step #%d ...", i);

        //if ... then skip/continue
        if (actIf(action, stash, ok) == 0) continue;

        //auth: true -> forward to global auth instance and run there
        //action: "get_..." -> run sub action, continue here with result
        if (actSubAction(ctx, ok)) return QVariant();

        //api: GET|endpoint
        //load data from api
        if (action.contains("api"))
        {
            QPointer<QEventLoop> loop;
            QObject scope; //this will delete loop
            if (blocking) loop = new QEventLoop(&scope);
            value = actApi(ctx, ok, loop);
            if (!blocking) return QVariant(); //continue via slot with higher start index
        }

        //http: url
        if (action.contains("http"))
        {
            actHttp(ctx, ok);
            if (!blocking) return QVariant(); //continue via slot with higher start index
        }

        //load_auth: url
        if (action.contains("load_auth"))
        {
            actShowPage(ctx, ok);
            if (!blocking) return QVariant(); //continue via slot with higher start index
        }

        //match: =
        //compare values (v) and expect match, on mismatch ok will be false
        else if (action.contains("match"))
            value = actMatch(action, stash, ok);

        //get: <key> (nested key possible)
        //get/copy value from stash or result and expect its existence
        else if (action.contains("get"))
            value = actGet(action, stash, ok);

        //array: <key>, dest: item; get: ..., dest: item.foo; yield: itemA
        //yield action will return every array item (see actionResult())
        else if (action.contains("array"))
            value = actArray(action, i, stash, ok);

        //set: ${foo}-${bar}
        else if (action.contains("set"))
            value = actSet(action, stash, ok);

        //append: key, dest: array
        else if (action.contains("append"))
            value = actAppend(action, stash, ok);

        //return
        else if (action.contains("return"))
        {
            QString key = action["return"].toString();
            QVariant value = actGet(key, stash, ok);
            if (ok)
            {
                if (!blocking) actionResult(ctx, value);
                _act_active.removeAll(ctx);
                return value;
            }
        }

        //yield
        else if (action.contains("yield") || action.contains("return-array") ||
            action.contains("continue"))
        {
            //if handler returns int, jump back to that action index (array)
            int goto_i = actYieldContinue(ctx, ok);
            stash = ctx->map["stash"].toMap(); //get updated stash
            if (goto_i > -1)
            {
                ctx->map["index"] = goto_i; //skip to that action
                i = goto_i - 1;
                continue;
            }
        }

        else if (!action.isEmpty()) //do not warn if cleared deliberately
        {
            qWarning() << "unrecognized action:" << action;
        }

        //END IF - end of action handling

        //abort and return nothing/false if action failed
        //for example, if data returned by api does not contain required key
        if (!ok)
        {
            //Action failed
            qDebug() << "stash:" << encodeJson(stash).constData();
            qWarning() << QString("action %1 failed at %2").arg(action["name"].toString()).arg(i).toUtf8().data();
            emit ctx->failed(ctx);
            _act_active.removeAll(ctx);
            return false;
        }

        //store result value in stash with specified key
        //store updated stash in action context (updated by current action)
        actDest(action, value, stash, ok);
        ctx->map["stash"] = stash; //update context
    }
    _act_active.removeAll(ctx); //action object goes out of scope, is deleted

    return all_ok; //no explicit return action, but all actions were ok
}

//QVariant
//VSite::callAction(const QString &name, const QVariantList &plan, QVariantMap stash, int start_index)
//{
//    //put call context into container variable
//    auto context = createActionContext(name, plan, stash, start_index);
//    return callAction(context);
//}

ActionContextPtr
VSite::getPendingActCtx(const ActionContextRef &ref)
{
    ActionContextPtr ctx;
    foreach (const ActionContextPtr &cur_ctx, _act_active)
    {
        if (cur_ctx == ref.data())
        {
            ctx = cur_ctx;
            break;
        }
    }
    return ctx;
}

int
VSite::actIf(const QVariantMap &action, const QVariantMap &stash, bool &ok)
{
    bool ok_continue = true;
    bool invert = false;

    //TODO open/store if block, ">"...

    QString key;
    QVariant value;
    foreach (QString act_k, action.keys())
    {
        if (!act_k.startsWith("if")) continue; //if, if_set
        QString k = action[act_k].toString(); //if: [!]<key>
        if (k.startsWith("!"))
        {
            invert = true;
            k = k.mid(1);
        }
        key = k; //key (stash)
    }
    if (key.isEmpty()) //but key may not exist, checked below
    {
        ok = false;
        return -1;
    }
    value = stash[key];

    if (action.contains("if_set"))
    {
        ok_continue = stash.contains(key);
    }
    else if (action.contains("if_true"))
    {
        ok_continue = value.toBool();
    }
    else if (action.contains("if"))
    {
        if (value.isNull() || !value.isValid()) ok_continue = false;
        else if (value.canConvert<QVariantList>())
            ok_continue = !value.toList().isEmpty();
        else if (value.canConvert<QVariantMap>())
            ok_continue = !value.toMap().isEmpty();
        //else if (v.canConvert<int>())
        //    return v.toInt();
        else if (value.userType() == QMetaType::Bool)
            ok_continue = value.toBool();
        else if (value.userType() == QMetaType::Int)
            ok_continue = value.toInt() != 0;
        else if (value.canConvert<QString>())
            ok_continue = !value.toString().isEmpty();
        else
            return -1;
    }

    if (invert) ok_continue = !ok_continue;

    //0: if statement returned false, skip action
    //-1: no if statement
    return ok_continue ? 1 : 0;
}

QNetworkRequest
VSite::prepApiReq(const QVariantMap &action, const QVariantMap &stash, bool &ok)
{
    //Build request url
    //API_URL is prepared by globalVariables(), e.g., from api_endpoint_prefix
    QUrl req_url = QUrl(stash["API_URL"].toString()); //prefix / base url
    QString api_call = action["api"].toString(); //api: endpoint
    qDebug() << "prepare api request" << "url:" << req_url << "call:" << api_call;
    //api: endpoint - resolve endpoint variable
    if (api_call.contains("${"))
    {
        //Replace placeholder in api call name
        bool replace_ok = false;
        api_call = actSet(api_call, stash, replace_ok).toString();
        if (!replace_ok)
        {
            qWarning() << "api call method cannot be resolved";
            ok = false;
            return QNetworkRequest();
        }
    }
    //api: endpoint - append endpoint to API_URL
    qDebug() << "prepare api request" << "call:" << api_call;
    if (!(api_call.isEmpty() || api_call == "GET"))
    {
        //api endpoint defined in action, append it to url
        qDebug() << "api url:" << req_url << "endpoint:" << api_call;
        req_url = req_url.resolved(QUrl(api_call));
    }
    if (req_url.isEmpty() || !req_url.isValid())
    {
        qWarning() << "no request url defined for api action!";
        ok = false;
        return QNetworkRequest();
    }

    //Request object
    QNetworkRequest req(req_url);
    req.setTransferTimeout();
    foreach (QString header, stash["API_HEADERS"].toStringList())
    {
        QStringList parts = header.split(": ");
        if (parts.count() != 2) continue;
        req.setRawHeader(parts[0].toUtf8(), parts[1].toUtf8());
    }

    return req;
}

/**
 * Send API request to site and schedule next action with result.
 *
 * { api: endpoint_name }
 */
QVariant
VSite::actApi(const ActionContextPtr &ctx, bool &ok, QPointer<QEventLoop> loop)
{
    QString name = ctx->map["name"].toString();
    QVariantMap action = ctx->map["action"].toMap();
    QVariantMap stash = ctx->map["stash"].toMap();

    // Request url
    QString type = "get";
    QByteArray payload;
    if (action.contains("data"))
    {
        type = "post";
        QVariant var = actGet(action["data"].toString(), stash, ok);
        payload = encodeJson(var);
    }
    else if (action.contains("form"))
    {
        type = "post";
        QUrlQuery query;
        QVariantMap form_map = actGet(action["form"].toString(), stash, ok).toMap();
        foreach (QString key, form_map.keys())
            query.addQueryItem(key, form_map[key].toString());
        payload = query.query().toUtf8();
        //application/x-www-form-urlencoded
    }
    QNetworkRequest req = prepApiReq(action, stash, ok);
    if (req.url().isEmpty()) return QVariant();

    // Send request, response handler either sync/here or async/later/slot...
    if (loop) //loop itself is managed by caller
    {
        //Wait, blocking request
        QNetworkAccessManager net; //use separate connection pool
        QPointer<QNetworkReply> reply = net.get(req); //will be deleted when net goes out of scope
        connect(reply, SIGNAL(finished()), loop, SLOT(quit()), Qt::DirectConnection);
        loop->exec();
        //reply object will be deleted along with its manager
        ok = reply->error() == QNetworkReply::NoError;
        if (!ok) return QVariant();
        return parseJsonToMap(reply->readAll(), &ok);
    }
    else
    {
        //Send request
        QPointer<QNetworkReply> reply;
        if (type == "post")
        {
            qDebug() << "POST:" << req.url() << payload;
            reply = _net->post(req, payload);
        }
        else
        {
            qDebug() << "GET:" << req.url();
            reply = _net->get(req);
        }
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(parseReply(QNetworkReply::NetworkError))); //TODO
        reply->setProperty("name", name);
        reply->setProperty("action", action);
        //Async... reply will trigger this func/loop again
        _act_reply_state[reply] = ctx;
        return QVariant(); //null, no return value because async operation
    }

    return QVariant();
}

QVariant
VSite::actHttp(const ActionContextPtr &ctx, bool &ok)
{
    QString name = ctx->map["name"].toString();
    QVariantMap action = ctx->map["action"].toMap();
    QVariantMap stash = ctx->map["stash"].toMap();

    if (action.contains("form"))
    {
        return QVariant();
    }

    // Request url
    QString type = "get";
    QByteArray j_data;
    if (action["type"].toString() == "check" || action["type"].toString() == "head")
    {
        type = "head";
    }
    bool req_url_set = true;
    QString req_url = actSet(action["http"].toString(), stash, req_url_set).toString();
    QNetworkRequest req;
    req.setUrl(QUrl(req_url));
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
    req.setTransferTimeout();

    //Send request
    QPointer<QNetworkReply> reply;
    if (type == "head")
    {
        qDebug() << "HEAD:" << req.url().url();
        reply = _net->head(req);
    }
    else
    {
        qDebug() << "GET:" << req.url().url();
        reply = _net->get(req);
    }
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(parseReply(QNetworkReply::NetworkError))); //TODO
    reply->setProperty("name", name);
    reply->setProperty("action", action);
    _act_reply_state[reply] = ctx;

    return QVariant(); //null, no return value because async operation
}

/**
 * Show interactive login page.
 *
 * When confirmed (final step),
 * both header (url) and content are returned as value for this action.
 * Final step - that's usually the second page that's loaded.
 *
 * This should run in the (per site domain) global VSite instance,
 * not in one of the instances used in the view widgets.
 * This separate instance is used so that only one interactive login page
 * is displayed, even if several VSite instances are active,
 * not one page for each site instance.
 * All VSite instances would wait for this central instance, see sub action.
 */
bool
VSite::actShowPage(const ActionContextPtr &ctx, bool &ok)
{
    QString name = ctx->map["name"].toString();
    QVariantMap action = ctx->map["action"].toMap();
    QVariantMap stash = ctx->map["stash"].toMap();

    //Trigger signal for main window to display page
    //Note that on timeout, ctx is deleted (and with it, the page widget etc.)
    QString url = action["load_auth"].toString();
    url = actGet(url, stash, ok).toString();
    if (url.isEmpty()) return false;
    qInfo() << "show auth page:" << url;
    ActionContextRef act(ctx.data());
    emit showPageSignal(url, act);

    //Connect signal to receive result value from view and continue action plan
    //The view must call ctx->setValue() which emits gotResult
    connect(ctx.data(),
        SIGNAL(gotResult(QVariant, ActionContextRef)),
        SLOT(handleResult(QVariant, ActionContextRef)));

    return true;
}

/**
 * Run another execution plan, use result as result for this action.
 *
 * { action: "get_token" }
 *
 * The specified plan is run in a central instance per domain (if requested).
 * This is used for authentication so that several active VSite instances
 * can call a sub action to acquire an authentication token
 * and only the first call will acquire the token (login interactively)
 * while keeping the other sub action calls waiting,
 * these following sub action calls will then be able to return
 * the previously generated auth token.
 *
 * If no sub action is defined, returns false.
 * Returns true if the specified sub action has been initiated.
 * The result will be put onto the stash in handleResult(),
 * which will then call callAction() to continue execution with the next step.
 *
 */
bool
VSite::actSubAction(const ActionContextPtr &ctx, bool &ok)
{
    QVariantMap action = ctx->map["action"].toMap();
    QVariantMap stash = ctx->map["stash"].toMap();
    if (!action.contains("action")) return false;
    QString name = action["action"].toString();

    //Instantiate global auth site, if needed
    VSite *site = this;
    site = getOrCreateControlInstance();
    if (site != this)
    {
        connect(site, &VSite::showPageSignal,
            this, &VSite::showPageSignal,
            Qt::UniqueConnection);
    }
    else
    {
        qWarning() << "sub action almost tried to connect to itself!?"; //bug
        return false;
    }
    qInfo() << this << "calling sub action via" << site << name;

    //Create and initiate sub action
    //connect sub action's finished signal to our result slot
    //we use ActionPointer* to identify the response, not ActionContextPtr
    //because QSharedPointer would prevent it from being deleted
    //on timeout (action goes out of scope), we want to delete this sub action
    //along with gui items (login tab) created by/for it
    //callWhenReady should be used instead of callAsync
    ActionContextPtr sub_action = site->callWhenReady(name, stash); //queued
    qInfo() << ctx.data() << "->" << sub_action.data();
    sub_action->enableSignal(); //forward result via signal
    //receive sub action's result (other VSite), handle it here (VSite/ctx)
    //note that we're receiving a signal from sub_action
    //but we need to work with the parent ctx in the slot (ctx initiated sub)
    connect(sub_action.data(), &ActionContext::finished,
        this, [this, ctx](const QVariant &v) { this->handleResult(v, ctx); });
    connect(sub_action.data(), &ActionContext::failed,
        this, [this, ctx]() { this->handleError(ctx); });

    return true;
}

/**
 * Get value from stash.
 *
 * { get: "key" }
 * { get: "sub.key" }
 * { get: "key", rx: /.../ }
 *
 * If the key does not exist and ignore is not set, execution will stop.
 * Can be used to check for an expected key in a response object.
 * With ignore set, it can be used to get an optional key
 * and save it to the field named in dest.
 * Can also be used for string transformations using regex (field: rx).
 *
 * Unlike most other actions, which will only access elements in stash,
 * this action can access sub elements in nested maps: sub.map.key
 */
QVariant
VSite::actGet(const QVariantMap &action, const QVariantMap &stash, bool &ok)
{
    QString key_path = action["get"].toString();
    if (key_path.isEmpty())
    {
        ok = false;
        return QVariant();
    }

    //GET: <key>
    //GET: key1.key2.key3
    //the value may be of any type, string, dict, etc.
    QVariant val = stash;
    QStringList keys = key_path.split('.');
    for (int i = 0, ii = keys.count(); i < ii; i++)
    {
        QString key = keys[i];
        QVariantMap cur_map = val.toMap();
        //requested key must exist (that's the point of the "get" action)
        if (!cur_map.contains(key) && !action["ignore"].toBool())
        {
            ok = false;
            return QVariant();
        }
        //last element returns QVariant, may be a string (or map or array)
        val = cur_map[key];
    }

    //RX: ...(\w+)...
    if (action.contains("rx"))
    {
        QString str = val.toString();
        QRegExp rx(action["rx"].toString());
        if (rx.indexIn(str) != -1)
        {
            //regex ok, get first match group
            str = rx.cap(1);
        }
        else
        {
            //regex mismatch, nothing to extract, return empty
            if (!action["ignore"].toBool())
            {
                qDebug() << "GET failed - regex mismatch" << action["rx"].toString() << str;
                ok = false;
            }
            else
            {
                ok = true; //error ignored - return empty but ok
            }
            return QVariant();
        }
        val = str;
    }

    //type: array
    if (action.contains("type"))
    {
        if (action["type"] == "array")
        {
            //variant.userType() == QMetaType::QVariantList;
            if (!val.canConvert<QVariantList>())
            {
                ok = false;
                return QVariant();
            }
        }
    }

    ok = true;
    return val;
}

/**
 * Get value from stash (convenience function).
 *
 * "key" / "dict.key"
 */
QVariant
VSite::actGet(const QString &key, const QVariantMap &stash, bool &ok)
{
    QVariantMap action;
    action["get"] = key;
    return actGet(action, stash, ok);
}

/**
 * Compare two values.
 */
QVariant
VSite::actMatch(const QVariantMap &action, const QVariantMap &stash, bool &ok)
{
    QString match_op = action["match"].toString();

    //match: <operator>, v: [<key1>, value2]
    //match: <operator>, v: [value1, value2]
    //match: =, v: [value1, value2]
    QVariant lval; QVariant rval;
    if (action.contains("var") && action.contains("const"))
    {
        lval = stash[action["var"].toString()];
        rval = stash[action["const"].toString()];
    }
    QStringList vals = action["v"].toStringList();
    QString v1 = vals[0];
    if (stash.contains(v1))
        v1 = stash[v1].toString();
    QString v2 = vals[1];

    bool match = v1 == v2;
    ok = match;
    return match;
}

/**
 * Set value in stash.
 *
 * { set: "<${key}>" }
 *
 * Placeholders are replaced with their corresponding values.
 */
QVariant
VSite::actSet(const QVariantMap &action, const QVariantMap &stash, bool &ok)
{
    QString pattern = action["set"].toString();
    QString dest_value = pattern;

    //SET: J: [{"field_name": "fixed_value"}]
    //parse everything after the "J: " prefix as json and return the result
    if (dest_value.startsWith("J: "))
    {
        return parseJson(QString(dest_value).remove(0, 3).toUtf8(), &ok);
    }

    //SET: ${key} ...
    //replace all occurrences of ${key} with the value of key in stash
    //the values are converted to strings (inserted into destination string)
    QRegExp rx_var("\\$\\{([\\w.]+)\\}");
    int pos = 0;
    while ((pos = rx_var.indexIn(dest_value, pos, QRegExp::CaretAtOffset)) != -1)
    {
        //Find next placeholder, get variable key name
        int len = rx_var.matchedLength();
        QString key = rx_var.cap(1);

        //Create sub context for get routine
        bool get_ok = false;
        QVariant val = actGet(key, stash, get_ok);
        if (!get_ok)
        {
            qWarning() << "set action: key not found" << key;
            ok = false;
            return QVariant();
        }

        //Replace placeholder
        dest_value.replace(pos, len, val.toString());

        pos++; //remove this to heat your room
    }

    ok = true;
    return dest_value;
}

/**
 * Set value in stash (convenience function).
 *
 * "<${key}>"
 */
QVariant
VSite::actSet(const QString &pattern, const QVariantMap &stash, bool &ok)
{
    QVariantMap action;
    action["set"] = pattern;
    return actSet(action, stash, ok);
}

/**
 * Initialize array in stash.
 *
 * { array: key, dest: item }
 *
 * In this example, the array in stash[key] will be read
 * and the first element will be made available via the key "item".
 * To be followed by the "continue" action, which will
 * set "item" to the next array element (if any).
 * If "key" is blank, a new array will be created.
 */
QVariant
VSite::actArray(const QVariantMap &action, int action_index, QVariantMap &stash, bool &ok)
{
    QString array_key = action["array"].toString(); //(key of) array
    QString item_key = action["dest"].toString(); //(key of) current element
    //key of element in stash: action["dest"]
    if (item_key.isEmpty()) item_key = "item"; //convenience...

    //Get specified array from stash (e.g., "res.files")
    bool array_found = false;
    QVariantList array = actGet(array_key, stash, array_found).toList();
    QVariant value; //current array item

    //Get array state map with metadata
    //QVariantMap stash_state = stash["_array"].toMap();
    //QVariantMap array_state = stash_state[array_key].toMap();
    QVariantMap array_state = stash["_array:" + item_key].toMap();
    //if state map new/empty, this will be the first iteration, so i = 0
    int index = 0;
    //if state map contains index, it will be the next index
    //after working with the item, the index will *not* be increased here
    //it'll be increased in the continue action
    //because that'll be easier: continue would return false
    //and execution would continue after the continue action
    if (array_state.contains("index")) //new index
    {
        index = array_state["index"].toInt();
        if (index == 0)
        {
            //error: if state.index defined, this is after yield/return,
            //so index must be > 0
            qWarning() << "array index 0 after return, preventing infinite loop";
            ok = false;
            return QVariant();
        }
    }
    //current index is stored in state map
    array_state["index"] = index;
    if (index == 0)
    {
        //initialize new state map
        array_state["array_key"] = array_key;
        array_state["array"] = array;
        array_state["action_index"] = action_index;
    }
    if (!array_found)
    {
        //requested array not found
        if (!action["ignore"].toBool())
            ok = false;
    }
    else
    {
        ok = true;
        if (!array.isEmpty())
            value = array[index];
    }

    //Save state map
    stash["_array:" + item_key] = array_state;

    return value; //current element / array item
}

/**
 * Append value at key to array.
 *
 * { append: "item", dest: "my_items" }
 *
 * This will get the referenced object (stash[append], e.g, item)
 * and append it to the array at stash[dest].
 * The new array is returned (= new value, will be set as dest).
 * If the key "to" is used to specify the array (instead of "dest"),
 * this array must already exist on the stash.
 */
QVariant
VSite::actAppend(const QVariantMap &action, const QVariantMap &stash, bool &ok)
{
    QString src_key = action["append"].toString(); //source key
    ok = !src_key.isEmpty(); //append+dest ok even if dest not initialized yet
    ok = ok && stash.contains(src_key); //source must be defined
    QVariant src_val = stash[src_key]; //source value to be added to list

    QVariantList arr;
    if (action.contains("to"))
    {
        QString key = action["to"].toString();
        ok = ok && stash[key].canConvert<QVariantList>();
        arr = stash[key].toList();
    }
    else if (action.contains("dest"))
    {
        QString key = action["dest"].toString();
        arr = stash[key].toList();
    }
    arr.append(src_val);

    return arr; //new array should not be used if !ok
}

/**
 * Handle array element, continue with next element (if any).
 *
 * If there are more array elements, this will return the index of the
 * array action to go to, i.e., the beginning of the array loop.
 * If the end of the array has been reached, -1 is returned.
 */
int
VSite::actYieldContinue(ActionContextPtr ctx, bool &ok)
{
    QVariantMap action = ctx->map["action"].toMap();
    QVariantMap stash = ctx->map["stash"].toMap();

    QString name = action["name"].toString();
    QString item_key;
    if (action.contains("continue"))
        item_key = action["continue"].toString();
    else if (action.contains("yield"))
        item_key = action["yield"].toString();
    else if (action.contains("return-array"))
        item_key = action["return-array"].toString();
    else
        return -1; //or die?
    QString state_key = "_array:" + item_key;
    if (!stash.contains(state_key))
        return -1;
    QVariantMap array_state = stash[state_key].toMap();

    //get array item from stash (may have been modified there by other actions)
    //array_mod will consist of modified elements
    QVariant item = stash[item_key]; //value
    QVariantList array = array_state["array_mod"].toList() << item;
    array_state["array_mod"] = array; //update modified array
    if (action.contains("yield"))
    {
        //Forward item
        actionResult(ctx, item);
    }
    //Go back to beginning of array action, continue with next item
    int index = array_state["index"].toInt();
    int count = array_state["array"].toList().count();
    int action_i = array_state["action_index"].toInt();
    if (index < count - 1)
    {
        //there are more elements, go back to array action, next array item...
        array_state["index"] = index + 1; //i++
        stash[state_key] = array_state; //update state
        ctx->map["stash"] = stash;
        int goto_i = action_i; //{ array: array_key}
        return goto_i; //continue with next array item
        //no point in setting ok, won't be evaluated
    }
    else
    {
        //last element reached, invalidate state
        array_state.remove("index");
        stash.remove(state_key);
        ctx->map["stash"] = stash;
    }

    //array yield done (at this point, no more elements left)
    ok = true;
    if (action.contains("yield"))
        actionResult(ctx, QVariant()); //yield blank to signal eof
    else if (action.contains("return-array"))
        actionResult(ctx, array);
    else if (action.contains("continue"))
        ok = true;
    else
        ok = false; //key in action missing

    return -1; //-1 means do not go back to array action; next action
}

/**
 * Save the result value (result from previous actions) in stash.
 */
void
VSite::actDest(const QVariantMap &action, const QVariant &value, QVariantMap &stash, bool &ok)
{
    if (!action.contains("dest")) return;

    //Put new value onto stash
    QString key = action["dest"].toString();
    if (!key.contains('.'))
    {
        //dest: key
        stash[key] = value;
    }
    else
    {
        //dest: item.key
        QStringList keys = key.split('.');
        if (keys.count() != 2)
        {
            ok = false; //not evaluated anymore
            return;
        }
        if (keys[0] == "VARS")
        {
            _vars[keys[1]] = value;
        }
        else
        {
            QVariantMap sub_dict = stash[keys[0]].toMap();
            sub_dict[keys[1]] = value;
            stash[keys[0]] = sub_dict;
        }
    }

    //Debug logging
    ProfileSettings *settings = ProfileSettings::profile();
    bool log_all = settings->setDefaultVariant("log.log_act_dest", false).toBool();
    if (action.contains("debug") || log_all)
    {
        QVariant v_debug = action["debug"];
        //qDebug() << "DEBUG - end of action " << i << ":";
        qDebug() << "end of action";
        //if (v_debug.toString() == "stash" || v_debug.toBool())
        if (v_debug.toString() == "stash")
            qDebug() << "stash:" << encodeJson(stash).constData();
        else
            qDebug() << "result:" << encodeJson(value).constData();
    }

}

/**
 * Produce template map for new stash.
 */
QVariantMap
VSite::globalVariables()
{
    QVariantMap vars;
    vars["VARS"] = _vars;
    vars["URL"] = siteUrl().url();
    vars["DOMAIN"] = domain();
    // channel set by setChannel()
    vars["PAGE_INDEX"] = pageOffset();
    vars["PAGE_NUMBER"] = pageOffset() + 1;
    vars["PAGE_SIZE"] = pageSize();
    vars["PAGE_OFFSET"] = pageOffset() * pageSize();
    qDebug() << this << "initializing global VARS" << _vars;

    QUrl api_url = siteUrl();
    if (_conf.contains("api_url"))
    {
        //API_URL = api_url
        api_url = _conf["api_url"].toString();
    }
    else if (_conf.contains("api_endpoint_prefix"))
    {
        //API_URL = api_url + api_endpoint_prefix
        QString endpoint_prefix = _conf["api_endpoint_prefix"].toString();
        QUrl prefix_url = QUrl(endpoint_prefix);
        if (prefix_url.isRelative())
            api_url.setPath(endpoint_prefix);
        else
            api_url.setUrl(endpoint_prefix);
    }
    qDebug() << this << "initializing global API_URL" << api_url.url();
    if (api_url.isEmpty() || !api_url.isValid())
    {
        qWarning() << this << "initialized blank api url!" << api_url.url();
    }
    vars["API_URL"] = api_url;
    if (_conf.contains("api_header"))
    {
        vars["API_HEADERS"] = 
            vars["API_HEADERS"].toStringList() << _conf["api_header"].toString();
    }

    return vars;
}

VSite*
VSite::getOrCreateControlInstance()
{
    static QMap<QString, QPointer<VSite>>
    _site_global_instance;
    QMap<QString, QPointer<VSite>> &map = _site_global_instance;

    foreach (QString key, map.keys())
        if (!map[key]) map.remove(key);

    QString address = "https://" + domain();
    VSite *site = 0;
    if (map.contains(address))
    {
        site = map[address];
    }
    else
    {
        //Initialize new VSite to be used by all related instances
        //It's important to use the copy constructor, not reload (load channel)
        site = new VSite(*this);
        if (site) map[address] = site;
    }

    return site;
}

