#ifndef PEERPLAYERMAIN_HPP
#define PEERPLAYERMAIN_HPP

#include <QMainWindow>
#include <QDebug>
#include <QCloseEvent>
#include <QBuffer>
#include <QLibraryInfo>
#include <QTabWidget>
#include <QTabBar>

#include <QWebEngineView>
#include <QWebEngineProfile>

#include "subscriptionsview.hpp"
#include "siteview.hpp"
#include "videoview.hpp"
#include "profilesettings.hpp"
#include "gui.hpp"
#include "settingswindow.hpp"

class PeerPlayerMain : public QMainWindow
{
    Q_OBJECT

signals:

public:

    PeerPlayerMain(QWidget *parent = 0, Qt::WindowFlags flags = 0);

private slots:

    void
    closeEvent(QCloseEvent *event);

    void
    addSubscriptionsTab();

    void
    showSettings();

    void
    showOpenChannelUrl();

    SiteView*
    addSiteTab(const QString &address);

    void
    setTabName(const QString &name, const QString &full_title, QWidget *tab = 0);

    void
    setTabName(const QString &name, QWidget *tab = 0);

    void
    closeTab(int index);

    void
    checkLastTabClosed();

    void
    openVideo(const QString &url, const QVariantMap &item = QVariantMap(), bool in_background = false);

    void
    showPage(const QString &url, const ActionContextRef &ctx);

    void
    handleLoadedPage(QWebEngineView *view, bool ok, const ActionContextRef &ctx);

private:

    void
    initNewTabWidget(QWidget *widget, const QString &type, const QString &address, const QVariantMap &item = QVariantMap());

    int
    m_tab_id;

    QTabWidget
    *m_tab_widget;

    QMap<int, QVariantMap>
    m_loaded_tabs;

    QAction
    *m_act_subscriptions;

    QAction
    *m_act_settings;

};

#endif
