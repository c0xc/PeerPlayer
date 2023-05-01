#define DEFINE_GLOBALS
#include "main.hpp"

int main(int argc, char *argv[])
{
    //Initialize QApplication
    QApplication app(argc, argv);
    app.setApplicationName(PROGRAM);
    app.setApplicationVersion(APP_VERSION);
    QString program = QString(PROGRAM).toLower();

    //Config
    ProfileSettings::useDefaultProfile();
    //if (config.variant("debug").isValid()) // TODO setDefaultVariant
    //if (config.setDefaultVariant("debug", false))
    //set profile prefix, use group accessor...
    initLogger();

    //Load our own font because we're special (and some Qt builds have no fonts)
    //Note that Qt no longer ships fonts. Deploy some (from https://dejavu-fonts.github.io/ for example) or switch to fontconfig.
    //TODO config
    QString font_path = ":/PTSans-Regular.ttf";
    int font_id = QFontDatabase::addApplicationFont(font_path);
    if (font_id != -1)
    {
        QFont font("PTSans");
        app.setFont(font);
    }

    //TODO Initialize QTranslator, load default translation

    //Load main window
    PeerPlayerMain *main = new PeerPlayerMain;
    main->show();
    int code = app.exec();

    return code;
}

void
initLogger()
{
    ProfileSettings settings;
    // setDefaultGroup
    //TODO ... lambda
    qInstallMessageHandler(logMessage);
}

void logMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    //QDebug details like line number are unavailable in release mode
    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    switch (type)
    {
    case QtDebugMsg:
        fprintf(stderr, "[DEBUG] %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtInfoMsg:
        fprintf(stderr, "[INFO] %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "[WARNING] %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "[CRITICAL] %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "[FATAL] %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    }
}

