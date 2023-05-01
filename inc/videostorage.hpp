#ifndef VIDEOSTORAGE_HPP
#define VIDEOSTORAGE_HPP

#include <QDebug>
#include <QBuffer>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QThread>
#include <QTimer>
#include <QRegExp>
#include <QMimeDatabase>
#include <QCryptographicHash>
#include <QTemporaryFile>
#include <QFuture>
#include <QtConcurrent>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "profilesettings.hpp"

class DLWatcher;
class VideoStorage : public QObject
{
    Q_OBJECT

signals:

    void
    downloadStarted(const QString &address, const QString &temp_file);
    void
    downloadStarted(const QUrl &url, const QString &temp_file);
    void
    downloadStarted(const QString &temp_file); //if required, add int id

    void
    downloadProgressed(double p);

    void
    downloadFailed(const QString &temp_file = QString());

    void
    downloadFinished(const QString &temp_file = QString());

    void
    suggestFilename(const QString &filename);

    void
    fileImported(const QString &filename);

public:

    /**
     * VideoStorage may be initialized multiple times,
     * keeping one instance for each opened video view
     * to make use of temporary files (partial downloads)
     * which are removed automatically when the instance is destroyed.
     */
    VideoStorage(QObject *parent = 0);

    ~VideoStorage();

    QString
    tempPath();

    QString
    importPath();

    QFile*
    makeTempFile(); //makePartFileForDownload

    QString
    makeTempFilePath();

    bool
    downloadToolManaged();

    bool
    downloadToolUpdates();

    bool
    updateDownloadToolOnce(bool again = false);

    bool
    installDownloadTool();

    bool
    removeDownloadTool();

    bool
    setDownloadToolUpdate(bool enabled);

    bool
    checkDownloadTool(QString *version_ptr);

    QString
    downloadToolExe(bool *found_ptr = 0);

    /**
     * Try to download video via external downloader.
     * The download signals must be used to check status and result:
     * downloadStarted(const QUrl &url, const QString &temp_file);
     * If the temporary file is not picked up and copied,
     * it will be removed later (when parent/widget is closed).
     */
    DLWatcher*
    downloadViaTool(const QString &address, QString temp_file = "");

    /**
     * Start HTTP download and run callback (cb) when done.
     * If temp_file is blank, a new temp file is created.
     * The download signals must be used to check status and result:
     * downloadStarted(const QUrl &url, const QString &temp_file);
     * If the temporary file is not picked up and copied,
     * it will be removed later (when parent/widget is closed).
     */
    void
    downloadFile(const QUrl &url, const QString &temp_file = "", QFile *fh = 0);

    /**
     * Start HTTP download and run callback (cb) when done.
     * The callback function gets passed the total size and error flag,
     * it must have this signature:
     * (qint64 bytes, bool failed)
     *
     * This function does not emit the download signals.
     * The filename may be unknown because fh may be a buffer or
     * another file handle without a name.
     */
    void
    downloadFile(const QUrl &url, QIODevice *fh, std::function<void(qint64, bool)> cb);

    void
    downloadFileToByteArray(const QUrl &url, std::function<void(const QByteArray&)> cb);

    //TODO this is one of our cannot-decide functions
    //QVariant
    //findByAddress(const QString &address);
    /**
     * Tries to find downloaded copy of given source address (forward lookup).
     * If no downloaded copy is found, an empty string is returned.
     *
     * We use a simple QString for the path instead of a QFileInfo object
     * because a string is all we need and an empty QFileInfo is undefined:
     * If filePath() is empty the behavior of this function is undefined.
     * https://doc.qt.io/qt-5/qfileinfo.html#absoluteFilePath
     */
    QString
    findFileByAddress(const QString &address);

    //QVariant
    //find(const QVariantMap &context);

    QVariantMap
    findFileContext(const QString &file);

    //bool
    //contains(const QVariantMap &context); //TODO remove this because calculating hash is slow

    QVariantMap
    findMatchingFile(const QString &copy_file_path);

    QString
    determineImportFilename(const QString &file_path, const QVariantMap &context);

    /**
     * The import routine has no return value as it runs async
     * because the file hash has to be calculated which takes time.
     * On import, the specified file is copied to the Videos folder
     * as configured.
     * When importing a temp file, the temp file will be moved (if possible),
     * the name may be changed, a file extension might be added.
     * When the import is complete, a signal will be emitted.
     *
     * The context map is saved alongside the url (that's the url to the video
     * that was used to identify and open it, not the url to the remote file).
     * The context map is saved under the url (as key), not under the file hash
     * because even though it might be useful to have one context map
     * saved for two addresses of the same video (e.g., url and embed_url)
     * as both have the same file hash,
     * this could open the door for spam to be saved
     * if a video is opened from another remote source (torrent)
     * which deliberately provides bad metadata.
     * Certain organizations have perfected this spam technology
     * in the early 2000s (some linked to the lovely music industry, others...).
     */
    void
    importFile(const QString &file_path, const QString &src_address, const QVariantMap &context, bool move_file = false);

    void
    scheduleImportFile(const QString &file_path, const QString &src_address, const QVariantMap &context, bool move_file = false);

private:

    //QPointer<ProfileSettings> //TODO parent pointer
    //m_settings;
    ProfileSettings
    m_settings;

    QList<QTemporaryFile*>
    m_temp_files;

    QPointer<QNetworkAccessManager>
    m_network;

};

class DLWatcher : public QObject
{
    Q_OBJECT

signals:

    void
    startFailed();

    void
    downloadStarted();

    void
    downloadProgressed(double p);

    void
    downloadFailed(const QString &err = "");

    void
    downloadFinished();

    void
    suggestFilename(const QString &filename);

public:

    DLWatcher(const QString &address, QObject *parent = 0);
    DLWatcher(const QString &address, const QString &dst_file, QObject *parent = 0);
    DLWatcher(const QString &address, QFile *dst_file, QObject *parent = 0);

    ~DLWatcher();

private:

    void
    init_proc();

    void
    init_dst(const QString &dst_file);

    void
    init_dst(QFile *dst_file);

    void
    tryDetermineFilename();

public:

    QString
    filePath();

public slots:

    void
    start();

private slots:

    void
    checkStateStarted();

    void
    transferChunk();

    void
    checkStatus(const QString &line);

    void
    checkStatus();

    void
    checkStateError(QProcess::ProcessError error);

    void
    checkStateFinished(int rc, QProcess::ExitStatus status);

    void
    completeDownload();

private:

    QPointer<ProfileSettings>
    m_settings;

    QString
    m_src_addr;

    QString
    m_dst_file_path;

    QPointer<QFile>
    m_dst_file_obj;

    bool
    m_start_confirmed;

    QPointer<QProcess>
    m_proc;

    QTimer
    tmr_status;

    QString
    m_tpl_filename;

};

#endif
