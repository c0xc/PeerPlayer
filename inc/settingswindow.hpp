#ifndef SETTINGSWINDOW_HPP
#define SETTINGSWINDOW_HPP

#include <QDialog>
#include <QPixmap>
#include <QLabel>
#include <QTabWidget>
#include <QTabBar>
#include <QPushButton>
#include <QCheckBox>
#include <QTabBar>
#include <QFormLayout>
#include <QMessageBox>
#include <QTextEdit>

#include "videostorage.hpp"

class MainSettingsWidget;

class SettingsWindow : public QWidget // QMainWindow is overkill
{
    Q_OBJECT

signals:

public:

    SettingsWindow(QWidget *parent = 0);

private:

    QTabWidget
    *m_tab_widget;


};

class MainSettingsWidget : public QWidget
{
    Q_OBJECT

signals:

public:

    MainSettingsWidget(QWidget *parent = 0);

private slots:

    void
    updateDlpFields();

    void
    enableDlpManaged();

    void
    disableDlpManaged();

    void
    setDlpUpdate(bool enabled);

private:

    QPushButton
    *btn_dlp_manage_own;

    QPushButton
    *btn_dlp_stop_managing_own;

    QCheckBox
    *chk_dlp_auto;

    VideoStorage
    m_storage;



};

#endif
