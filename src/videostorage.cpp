#include "videostorage.hpp"

VideoStorage::VideoStorage(QObject *parent)
            : QObject(parent)
{
    m_settings = *ProfileSettings::profile();

    if (downloadToolUpdates()) updateDownloadToolOnce();
}

VideoStorage::~VideoStorage()
{
    //Clean up temp files - no action, it happens automatically
    //this is just to add logging
    foreach (QTemporaryFile *file, m_temp_files)
    {
        if (file->exists())
            qDebug() << "temp file needs to be removed now:" << file->fileName();
        else
            qDebug() << "temp file does not need to be removed anymore, already gone:" << file->fileName();
    }
}

QString
VideoStorage::tempPath()
{
    QDir home(QDir::home());
    QFileInfo fi(home, "tmp");
    if (fi.isDir())
        return fi.filePath();
    else
        return QDir::tempPath();
}

QString
VideoStorage::importPath()
{
    QDir home(QDir::home());
    QFileInfo fi(home, "PeerPlayer");
    if (fi.isDir())
        return fi.filePath();
    fi = QFileInfo(home, "Videos");
    if (fi.isDir())
        return fi.filePath();
    return QDir::homePath();
}

QFile*
VideoStorage::makeTempFile()
{
    //Create QTemporaryFile which will be removed automatically by dtor.
    QString temp_file_path = QDir(tempPath()).absoluteFilePath("downloaded_video.part");
    QDir dir = tempPath();
    QString name_template = "video.XXXXXX.part";
    QTemporaryFile *temp_file = new QTemporaryFile(dir.absoluteFilePath(name_template), this);
    //The temp file is opened/created for consistency and:
    //fileName() ... is null before the QTemporaryFile is opened
    temp_file->open();
    m_temp_files << temp_file;
    return temp_file;
}

QString
VideoStorage::makeTempFilePath()
{
    QFile *file = makeTempFile();
    file->close();
    return file->fileName();
}

bool
VideoStorage::downloadToolManaged()
{
    ProfileSettings *settings = ProfileSettings::profile();

    bool is_managed = settings->variant("dlp_installed_copy").toBool();

    return is_managed;
}

bool
VideoStorage::downloadToolUpdates()
{
    ProfileSettings *settings = ProfileSettings::profile();

    bool updates_enabled = settings->variant("dlp_updates_enabled").toBool();

    return updates_enabled;
}

bool
VideoStorage::updateDownloadToolOnce(bool again)
{
    static bool already_updated = false;
    if (!again && already_updated) return true;

    if (!downloadToolManaged()) return false;
    QString our_exe_path = downloadToolExe();

    //Note that this url has an http redirect to the current version
    //http redirects are enabled in downloadFile()
    QString addr = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp";
#ifdef Q_OS_WIN
    addr = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe";
#endif

    //Download yt-dlp and save it in application directory
    //that's all to set it up - we don't want to install anything system-wide
    QUrl url(addr);
    QFile *temp_file = makeTempFile();
    qDebug() << "downloading yt-dlp..." << addr << temp_file->fileName();
    downloadFile(url, temp_file, [our_exe_path, temp_file](qint64 bytes, bool failed)
    {
        //Download complete, quick check to see if it seems ok
        if (failed) return;
        QFile old_file(our_exe_path);
        qDebug() << "downloaded yt-dlp..." << temp_file->fileName() << temp_file->size() << "B" << bytes << failed;
        if (failed || !bytes) return;
        if (old_file.size() == temp_file->size()) return; //assume no change if same size

        //Copy downloaded executable to application directory
        if (old_file.exists()) old_file.remove();
        temp_file->copy(our_exe_path);
        QFile(our_exe_path).setPermissions(QFileDevice::ReadOwner | QFileDevice::ExeOwner | QFileDevice::ReadUser | QFileDevice::ExeUser | QFileDevice::ReadGroup | QFileDevice::ExeGroup);
    });

    already_updated = true;

    return true;
}

bool
VideoStorage::installDownloadTool()
{
    ProfileSettings *settings = ProfileSettings::profile();

    settings->setVariant("dlp_exe_name", "");
    settings->setVariant("dlp_installed_copy", true);

    return updateDownloadToolOnce(true);
}

bool
VideoStorage::removeDownloadTool()
{
    ProfileSettings *settings = ProfileSettings::profile();
    if (!downloadToolManaged()) return false;

    settings->setVariant("dlp_installed_copy", false);

    //Remove executable from application directory
    bool found_exe = false;
    //this will be the full path to our copy of the executable
    QString exe_path = downloadToolExe(&found_exe);
    if (!found_exe) return false;
    return QFile::remove(exe_path);
}

bool
VideoStorage::setDownloadToolUpdate(bool enabled)
{
    ProfileSettings *settings = ProfileSettings::profile();

    settings->setVariant("dlp_updates_enabled", enabled);

    return true;
}

bool
VideoStorage::checkDownloadTool(QString *version_ptr)
{
    //Get name of or path to executable (if we manage our own copy)
    //Then try to run it in order to check if it's working
    //Suppose the file exists and is executable, it might still be bad
    //or fail to start for some other unknown reason
    ProfileSettings *settings = ProfileSettings::profile();
    //This will be the path to our copy, if we have one
    QString exe = downloadToolExe();

    QProcess proc;
    proc.setProgram(exe);
    proc.setArguments(QStringList() << "--version");
    proc.start();

    //Make sure it could be started - fail otherwise (may fail without rc)
    if (!proc.waitForStarted())
        return false;
    //Wait for it to finish
    if (!proc.waitForFinished())
        return false;

    //Check exit status
    if (proc.exitStatus() != QProcess::NormalExit)
        return false;

    //Read additional info
    if (version_ptr)
        *version_ptr = proc.readAllStandardOutput();

    //At this point, the program could be started, it ran and returned
    //We could still check the return code which should be 0
    return proc.exitCode() == 0;
    //but that's optional
    return true;
}

QString
VideoStorage::downloadToolExe(bool *found_ptr)
{
    //Do not check again after file path already checked
    //if (!m_dlp_exe.isEmpty()) return m_dlp_exe;

    //We deliberately use yt-dlp, not ***tube-dl
    QString dlp_name = "yt-dlp";
    ProfileSettings *settings = ProfileSettings::profile();

    //Return full path to our own copy that we have installed in our app dir
    //Only if enabled - this is opt-in
    //Otherwise (by default) return the name and hope it's installed globally
    if (!downloadToolManaged()) return dlp_name; //unchecked

    //Filename of external download tool executable
    //This external downloader is required for some sites
    //Use our copy of that downloader if we have one => return file path
    bool found_exe = false;
    QString our_exe_name = settings->variant("dlp_exe_name").toString();
    if (our_exe_name.contains('/') || our_exe_name.contains(QDir::separator()))
        our_exe_name.clear(); //safeguard - prevent "../../../opt/danger"
    if (our_exe_name.isEmpty())
    {
        //Assume executable has default name
        our_exe_name = dlp_name;
#ifdef Q_OS_WIN
        dlp_name += ".exe";
#endif
    }
    //$HOME/.config/PeerPlayer/yt-dlp
    QString dlp_exe =
        settings->configDirectory().absoluteFilePath(our_exe_name);
    if (QFile::exists(dlp_exe))
        found_exe = true;

    //We manage our own copy of the downloader, so always return its full path
    if (found_ptr) *found_ptr = found_exe;

    return dlp_exe;
}

DLWatcher*
VideoStorage::downloadViaTool(const QString &address, QString temp_file) //startDownloadTool
{
    //Initialize DLWatcher object to control external process
    //It does not need to run in another thread as i/o is already async
    //If no temp file path is provided, one is created, check with filePath()
    DLWatcher *dl_watcher = 0;
    if (temp_file.isEmpty())
        temp_file = makeTempFilePath();
    dl_watcher = new DLWatcher(address, temp_file, this);

    //Hook up signals to notify caller about status and success
    connect(dl_watcher, &DLWatcher::downloadStarted, this, [this, address, dl_watcher]()
    {
        QString file = dl_watcher->filePath();
        emit downloadStarted(address, file);
        emit downloadStarted(file);
    });
    connect(dl_watcher, &DLWatcher::downloadProgressed, this, [this, dl_watcher](double p)
    {
        emit downloadProgressed(p);
    });
    connect(dl_watcher, &DLWatcher::downloadFailed, this, [this, dl_watcher]()
    {
        QString file = dl_watcher->filePath();
        emit downloadFailed(file);
        QTimer::singleShot(60000, dl_watcher, SLOT(deleteLater()));
    });
    connect(dl_watcher, &DLWatcher::downloadFinished, this, [this, dl_watcher]()
    {
        emit downloadFinished(dl_watcher->filePath());
        dl_watcher->deleteLater();
    });
    connect(dl_watcher, &DLWatcher::suggestFilename, this, [this, address](const QString &filename)
    {
        emit suggestFilename(filename);
    });

    //Start process
    QTimer::singleShot(0, dl_watcher, SLOT(start()));

    return dl_watcher;
}

void
VideoStorage::downloadFile(const QUrl &url, const QString &temp_file, QFile *fh)
{
    //Initialize HTTP file download
    //It does not need to run in another thread as i/o is already async
    //If no temp file path is provided, one is created, check with filePath()
    QFile *file = 0;
    if (fh)
        file = fh;
    else if (temp_file.isEmpty())
        file = makeTempFile();
    else
        file = new QFile(temp_file, this);
    if (!file->isOpen()) file->open(QIODevice::WriteOnly | QIODevice::Truncate);

    //Prepare request, acquire handle to network manager (conn pool...)
    if (!m_network) m_network = new QNetworkAccessManager(this);
    QNetworkRequest req(url);

    //Initiate download (GET), write received data to temp file
    QNetworkReply *reply = m_network->get(req);
    reply->setProperty("data_received", false);
    reply->setProperty("is_failed", false);
    connect(reply, &QNetworkReply::downloadProgress, this, [this, reply, url, file]
    (qint64 received, qint64 total)
    {
        if (!reply->property("data_received").toBool())
        {
            reply->setProperty("data_received", true);
            QString file_path = file->fileName();
            emit downloadStarted(url, file_path);
            emit downloadStarted(file_path);
            qDebug() << "http download started:" << url.url() << file_path;
        }

        qint64 now_written = file->write(reply->read(received));
        qint64 total_received = received;
        qint64 total_written = reply->property("written").toLongLong();
        total_written += now_written;
        reply->setProperty("written", total_written);

        double p = std::div(total_written * 100, total).quot;
        emit downloadProgressed(p);
    });
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply, file](QNetworkReply::NetworkError code)
    {
        reply->setProperty("is_failed", true);
        emit downloadFailed(file->fileName());

        reply->deleteLater();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, file]()
    {
        bool is_failed = reply->property("is_failed").toBool();
        if (is_failed) return;
        qDebug() << "http download completed:" << file->fileName();

        file->write(reply->readAll());
        file->close();

        emit downloadFinished(file->fileName());
        reply->deleteLater();
    });

}

void
VideoStorage::downloadFile(const QUrl &url, QIODevice *fh, std::function<void(qint64, bool)> cb)
{
    //Initialize HTTP file download to target file handle
    if (!fh->isOpen()) fh->open(QIODevice::WriteOnly | QIODevice::Truncate);

    //Prepare request, acquire handle to network manager (conn pool...)
    if (!m_network) m_network = new QNetworkAccessManager(this);
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    //Initiate download (GET), write received data to file handle
    QNetworkReply *reply = m_network->get(req);
    reply->setProperty("data_received", false);
    reply->setProperty("is_failed", false);
    connect(reply, &QNetworkReply::downloadProgress, this, [this, reply, url, fh]
    (qint64 received, qint64 total)
    {
        if (!reply->property("data_received").toBool())
        {
            reply->setProperty("data_received", true);
        }

        qint64 written = fh->write(reply->read(received));

        reply->setProperty("data_written", written);
        double p = std::div(written * 100, total).quot;
    });
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply, fh](QNetworkReply::NetworkError code)
    {
        reply->setProperty("is_failed", true);

        reply->deleteLater();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, fh, cb]()
    {
        qint64 written1 = reply->property("data_written").toLongLong();
        bool is_failed = reply->property("is_failed").toBool();

        qint64 written2 = 0;
        if (!is_failed)
        {
            written2 = fh->write(reply->readAll());
            fh->close();
        }

        qint64 written = written1 + written2;
        reply->setProperty("data_written", written);
        cb(written, is_failed);

        reply->deleteLater();
    });
}

void
VideoStorage::downloadFileToByteArray(const QUrl &url, std::function<void(const QByteArray&)> cb)
{
    QBuffer *buffer = new QBuffer(this);
    downloadFile(url, buffer, [buffer, cb](qint64 bytes, bool failed)
    {
        if (!failed)
            cb(buffer->data());
    });
}

//QVariant
//VideoStorage::findByAddress(const QString &address)
//{
//    QVariant result;
//    ProfileSettings *settings = ProfileSettings::profile();
//    QVariantMap addr_map = settings->variant("addr_map").toMap();
//    if (addr_map.contains(address))
//    {
//        result = addr_map[address];
//    }
//    return result;
//    //TODO return QUrl
//}

QString
VideoStorage::findFileByAddress(const QString &address)
{
    QString file_path;
    ProfileSettings *settings = ProfileSettings::profile();
    //addr_map contains vid_dl item(s) src_addr => {file_path + metadata}
    QVariantMap addr_map = settings->variant("addr_map").toMap();
    if (addr_map.contains(address))
    {
        //Get item from map with address as key
        QVariant var = addr_map[address];
        if (var.canConvert<QVariantMap>())
        {
            QVariantMap vid_dl = var.toMap();
            file_path = vid_dl["file"].toString();
        }
        else if (var.canConvert<QString>())
        {
            file_path = var.toString(); //TODO testing only
        }
        //Check if file still exists, otherwise don't return this item
        if (!file_path.isEmpty() && !QFileInfo(file_path).exists())
        {
            //Referenced file does not exist anymore
            qDebug() << "removed deleted file from addr_map" << file_path;
            //Move obsolete item to trash list
            QVariantList trash_list = settings->variant("trash_list").toList(); //TODO settings.getList / append ...
            QVariantMap file_map = settings->variant("file_map").toMap();
            trash_list.append(var);
            addr_map.remove(address);
            file_map.remove(file_path);
            settings->setVariant("trash_list", trash_list); //TODO
            settings->setVariant("addr_map", addr_map);
            settings->setVariant("file_map", file_map);
            //Do not return invalid file path
            file_path = "";
        }
    }
    return file_path;
}

//QVariant
//VideoStorage::find(const QVariantMap &context)
//{
//    QVariant result;
//
//    if (context.contains("url"))
//    {
//        QString address = context["url"].toString();
//        result = findByAddress(address);
//    }
//    else
//    {
//    }
//
//    return result;
//}

QVariantMap
VideoStorage::findFileContext(const QString &file)
{
    QVariantMap result;
    //TODO
    ProfileSettings *settings = ProfileSettings::profile();
    QVariantMap map = settings->variant("file_map").toMap();
    if (map.contains(file))
    {
        result = map[file].toMap();
    }
    return result;
}

//bool
//VideoStorage::contains(const QVariantMap &context)
//{
//    return !find(context).isNull();
//}

QVariantMap
VideoStorage::findMatchingFile(const QString &copy_file_path)
{
    //TODO
    return QVariantMap();
}

QString
VideoStorage::determineImportFilename(const QString &file_path, const QVariantMap &context)
{
    QFile in_file(file_path);

    //Destination directory
    QDir dst_dir(importPath());

    //Destination filename
    QFileInfo fi(file_path);
    QString filename(fi.fileName());
    if (filename.startsWith("."))
    {
        filename = filename.mid(1);
        //TODO use context?
    }
    if (filename.endsWith(".part"))
        filename.chop(5);

    //Append file extension
    //TODO this does not work: mime.preferredSuffix() returns "m2t" for mp4
    QMimeType mime =
        QMimeDatabase().mimeTypeForFile(file_path, QMimeDatabase::MatchContent);
    QString ext = mime.preferredSuffix();
    if (!ext.isEmpty() && filename.right(ext.length()) != ext) //TODO fix
        filename += "." + ext;
    //Apply recommended filename if provided
    if (context.contains("_suggested_filename"))
        filename = context["_suggested_filename"].toString();

    //Append counter or X to filename if it already exists
    if (QFileInfo(dst_dir, filename).exists())
    {
        int e = 2;
        QString name_base = QFileInfo(filename).completeBaseName();
        QString name_ext = QFileInfo(filename).suffix();
        while (QFileInfo(dst_dir, QString("%1%2%3").arg(name_base).arg(e).arg(name_ext)).exists())
            e++;
        filename = QString("%1%2%3").arg(name_base).arg(e).arg(name_ext);
    }

    return filename;
}

void
VideoStorage::importFile(const QString &file_path, const QString &src_address, const QVariantMap &context, bool move_file)
{
    QFile in_file(file_path);
    if (!in_file.exists()) return;
    if (!in_file.size()) return; //ignore empty file

    //Destination directory
    QString dst_dir_path = importPath();
    QDir dst_dir(dst_dir_path);
    if (!dst_dir.exists()) return;

    //Destination filename
    QString filename = determineImportFilename(file_path, context);
    QFileInfo fi_new(dst_dir, filename);

    //Copy file to destination, i.e., storage directory
    //or rename it, if the caller told us that it can be removed (temp file)
    if (move_file)
    {
        if (!in_file.rename(fi_new.filePath()))
        {
            //Rename failed, that's ok though (e.g., different volume)
            if (!in_file.copy(fi_new.filePath())) return; //TODO error signal
        }
    }
    else
    {
        if (!in_file.copy(fi_new.filePath())) return; //TODO error signal
    }

    //TODO use central storage instance (mutex) for safe db access
    //TODO consider QSaveFile
    //
    //TODO open fh first, so we don't lose it if it's removed in the meantime

    //Calculate file hash
    QString md5;
    {
        QFile file(fi_new.filePath());
        QCryptographicHash hash_md5(QCryptographicHash::Md5);
        hash_md5.addData(&file);
        md5 = hash_md5.result().toHex();
    }

    //Access global settings
    ProfileSettings *settings = ProfileSettings::profile();

    //Add context, info for lookup by filename
    QVariantMap file_map = settings->variant("file_map").toMap();
    file_map[fi_new.filePath()] = context;
    settings->setVariant("file_map", file_map);
    qDebug() << "added file to file_map" << fi_new.filePath();
    QVariantMap ctx_mod = context;
    ctx_mod["file"] = fi_new.filePath();
    ctx_mod["hash_md5"] = md5;
    //TODO convenience function to directly add map item

    //Add download item to address map for lookup by source address
    //Download item means this points to the downloaded, imported filename
    QVariantMap addr_map = settings->variant("addr_map").toMap();
    QVariantMap vid_dl;
    vid_dl["url"] = src_address;
    vid_dl["file"] = fi_new.filePath();
    vid_dl["hash_md5"] = md5;
    addr_map[src_address] = vid_dl;
    settings->setVariant("addr_map", addr_map);
    qDebug() << "added file to addr_map" << fi_new.filePath();

    emit fileImported(fi_new.filePath());
}

void
VideoStorage::scheduleImportFile(const QString &file_path, const QString &src_address, const QVariantMap &context, bool move_file)
{
    QFuture<void> future = QtConcurrent::run([this, file_path, src_address, context, move_file]()
    {
        importFile(file_path, src_address, context, move_file);
    });
}

/**
 * DLWatcher is a wrapper class for starting and controlling an external
 * download using the external video downloader.
 * It takes a string for the temp file path,
 * not a QFile file handle because the external program will open the file.
 * Well actually (say that holding your nose)
 * it's probably possible to have the downloader write to stdout (-o -)
 * which we could then read and write to the file handle...
 */
DLWatcher::DLWatcher(const QString &address, QObject *parent)
         : QObject(parent),
           m_src_addr(address),
           m_start_confirmed(false)
{
    connect(&tmr_status, SIGNAL(timeout()), SLOT(checkStatus()));
}
DLWatcher::DLWatcher(const QString &address, const QString &dst_file, QObject *parent)
         : DLWatcher(address, parent)
{
    init_proc();
    init_dst(dst_file);
}
DLWatcher::DLWatcher(const QString &address, QFile *dst_file, QObject *parent)
         : DLWatcher(address, parent)
{
    init_proc();
    init_dst(dst_file);
}

DLWatcher::~DLWatcher()
{
    //On exit, kill subprocess now to prevent exceptions like this:
    //[WARNING] QProcess: Destroyed while process ("yt-dlp") is still running.
    if (m_proc)
    {
        tmr_status.stop();
        m_proc->kill();
    }
}

void
DLWatcher::init_proc()
{
    m_proc = new QProcess(this);

    connect(m_proc, SIGNAL(started()), SLOT(checkStateStarted()));
    connect(m_proc, SIGNAL(readyReadStandardOutput()), SLOT(transferChunk()));
    connect(m_proc, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(checkStateFinished(int, QProcess::ExitStatus)));
    connect(m_proc, SIGNAL(error(QProcess::ProcessError)), SLOT(checkStateError(QProcess::ProcessError)));

    m_proc->setReadChannel(QProcess::StandardOutput);

    tmr_status.stop();
}

void
DLWatcher::init_dst(const QString &dst_file)
{
    m_dst_file_path = dst_file;

    ProfileSettings *settings = ProfileSettings::profile(); //TODO
    QString exe = VideoStorage().downloadToolExe();

    //This is the default call, downloader downloads video to specified file
    m_proc->setProgram(exe);
    QStringList args;
    args << "-o" << dst_file;
    args << "--no-part";
    args << "-q" << "--progress" << "--newline";
    args << m_src_addr;
    m_proc->setArguments(args);
    qDebug() << args;

    tmr_status.stop();
}

void
DLWatcher::init_dst(QFile *dst_file)
{
    m_dst_file_obj = dst_file;

    ProfileSettings *settings = ProfileSettings::profile(); //TODO
    QString exe = VideoStorage().downloadToolExe();

    //alternative call, downloader streams downloaded video
    m_proc->setProgram(exe);
    QStringList args;
    args << "-o" << "-";
    args << "--no-part";
    args << "-q" << "--progress" << "--newline";
    args << m_src_addr;
    m_proc->setArguments(args);
    qDebug() << args;

    tmr_status.stop();
}

void
DLWatcher::tryDetermineFilename()
{
    //Call external downloader to determine filename
    //especially relevant for the file extension, otherwise unknown
    //this is relevant only for videos that are downloaded by the ext. tool

    ProfileSettings *settings = ProfileSettings::profile(); //TODO
    QString exe = VideoStorage().downloadToolExe();

    QProcess *proc = new QProcess(this);
    proc->setProgram(exe);
    QStringList args;
    args << "--print" << "filename";
    args << "-o" << "[%(id)s].%(ext)s";
    args << m_src_addr;
    proc->setArguments(args);
    proc->start();
    if (!proc->waitForFinished()) return;
    if (proc->exitStatus() != QProcess::NormalExit) return;

    QString out(proc->readAllStandardOutput());
    out = out.split("\n")[0];
    out = out.trimmed();
    if (out.isEmpty()) return;
    if (!out.contains(".")) return;

    emit suggestFilename(out);
}

QString
DLWatcher::filePath()
{
    return m_dst_file_path;
}

void
DLWatcher::start()
{
    tryDetermineFilename();

    m_proc->start();
    tmr_status.start(1);
}

void
DLWatcher::checkStateStarted()
{
    qDebug() << "downloader started";
    //emit downloadStarted
    //Wait and check before emitting started signal
    //in order to catch start error (e.g., tool broken or missing)
    //QTimer::singleShot(1000, this, ...
}

void
DLWatcher::transferChunk()
{
    //Read downloaded chunk from process and write it to file handle
    if (!m_dst_file_obj) return;

    QByteArray bytes = m_proc->readAllStandardOutput();
    qint64 written = m_dst_file_obj->write(bytes);
}

void
DLWatcher::checkStatus(const QString &line)
{
    QString percent_str;
    qDebug() << "dl/proc line:" << line;
    QRegExp rx("\\s(\\d+[.]\\d+)%"); //TODO static
    if (rx.indexIn(line) != -1)
    {
        percent_str = rx.cap(1);
    }

    if (!percent_str.isEmpty())
    {
        if (!m_start_confirmed)
        {
            emit downloadStarted();
            m_start_confirmed = true;
        }
        bool ok;
        double p = percent_str.toDouble(&ok);
        if (ok)
            emit downloadProgressed(p);
    }

}

void
DLWatcher::checkStatus()
{
    //Read status from process and report progress IF status in stdout
    //The downloader will print status text to its stdout by default
    //when it is saving to a file; it won't if stdout is used for streaming
    if (m_dst_file_path.isEmpty())
        return;

    //Read line from subprocess, if possible
    if (m_proc->canReadLine())
    {
        QString line(m_proc->readLine());
        checkStatus(line);
    }
}

void
DLWatcher::checkStateError(QProcess::ProcessError error)
{
}

void
DLWatcher::checkStateFinished(int rc, QProcess::ExitStatus status)
{
    //Stop monitoring, collect remaining output from pipe
    tmr_status.stop();
    QByteArray err_output = m_proc->readAllStandardError();
    QByteArray raw_out_rest = m_proc->readAllStandardOutput();
    if (!raw_out_rest.isEmpty())
    {
        //Process remaining lines from stdout, handle status signals
        QTextStream reader(&raw_out_rest);
        while (!reader.atEnd())
            checkStatus(reader.readLine());
    }

    qDebug() << "dl/proc finished, rc:" << rc; //TODO LogLogger ...

    if (status == QProcess::NormalExit && rc == 0)
        completeDownload();
    else
        emit downloadFailed(QString(err_output));

}

void
DLWatcher::completeDownload()
{
    //In streaming mode, read remaining data and flush target
    if (m_dst_file_obj)
    {
        transferChunk();
        m_dst_file_obj->close();
    }

//    //Set new filename
//    if (m_tpl_filename)
//        filename = m_tpl_filename;
//    //Append file extension
//    QMimeType mime =
//        QMimeDatabase().mimeTypeForFile(temp_file, QMimeDatabase::MatchContent);
//    QString ext = mime.preferredSuffix();
//    if (!ext.isEmpty())
//        filename += "." + ext;
//    //Determine new file path
//    QFileInfo fi(fi_temp);
//    fi.setFile(filename);

    //Confirm that download is completed (either specified temp file or fh)
    emit downloadFinished();
}

