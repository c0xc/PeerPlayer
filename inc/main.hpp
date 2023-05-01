#ifndef MAIN_HPP
#define MAIN_HPP

//TODO TEST
//#include <qapplication.h>
#include <stdio.h>
#include <stdlib.h>

#include <QApplication>
#include <QTranslator>
#include <QProcessEnvironment>
#include <QFontDatabase>
#include <QLibraryInfo>

#include "version.hpp"

#include "profilesettings.hpp"
#include "peerplayermain.hpp"
#include "vlcplayer.hpp"

//TODO alias QS("", ...)

void initLogger();

void logMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);
//TODO trace()

#endif
