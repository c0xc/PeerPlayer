#include "settingswindow.hpp"

//TODO scroll area wrapper for large container widgets?

SettingsWindow::SettingsWindow(QWidget *parent)
              : QWidget(parent)
{
    m_tab_widget = new QTabWidget;
    //m_tab_widget->setShape(QTabBar::TriangularWest);
    m_tab_widget->tabBar()->setShape(QTabBar::TriangularWest);
    m_tab_widget->setTabShape(QTabWidget::Triangular);
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(m_tab_widget);
    setLayout(vbox);

    MainSettingsWidget *main_settings = new MainSettingsWidget;
    m_tab_widget->addTab(main_settings, tr("General")); //main/misc settings

    setWindowTitle(tr("Settings"));
}

MainSettingsWidget::MainSettingsWidget(QWidget *parent)
                  : QWidget(parent)
{
    QVBoxLayout *vbox = new QVBoxLayout;
    setLayout(vbox);

    //External download helper program
    QLabel *lbl_dlp_top = new QLabel(tr("External download helper program"));
    lbl_dlp_top->setStyleSheet("QLabel { font-size:14pt; }");
    vbox->addWidget(lbl_dlp_top);
    QLabel *lbl_dlp_sub = new QLabel(tr("This external download program may be required for some special video sites."));
    lbl_dlp_sub->setText("<sub>" + lbl_dlp_sub->text() + "</sub>");
    vbox->addWidget(lbl_dlp_sub);
    QFormLayout *form_dlp = new QFormLayout;
    vbox->addLayout(form_dlp);
    QVBoxLayout *vbox_manage = new QVBoxLayout;
    btn_dlp_manage_own = new QPushButton(tr("Set up download helper")); //start managing OUR COPY of the helper program / download tool
    btn_dlp_manage_own->setToolTip(tr("Download helper program from the Internet. It will not be installed on your system, it will only be used by PeerPlayer."));
    btn_dlp_manage_own->setDisabled(true);
    connect(btn_dlp_manage_own, SIGNAL(clicked()), SLOT(enableDlpManaged()));
    vbox_manage->addWidget(btn_dlp_manage_own);
    btn_dlp_stop_managing_own = new QPushButton(tr("Remove download helper")); //remove OUR COPY of the download tool
    btn_dlp_stop_managing_own->setToolTip(tr("Remove download helper (only affects the copy used by this program). If the downloader is installed on the system, it will be used. Otherwise downloads are not available in some situations."));
    btn_dlp_stop_managing_own->setDisabled(true);
    connect(btn_dlp_stop_managing_own, SIGNAL(clicked()), SLOT(disableDlpManaged()));
    vbox_manage->addWidget(btn_dlp_stop_managing_own);
    form_dlp->addRow(tr("Set up download helper program"), vbox_manage);
    chk_dlp_auto = new QCheckBox(tr("Update on startup"));
    chk_dlp_auto->setDisabled(true);
    connect(chk_dlp_auto, SIGNAL(toggled(bool)), SLOT(setDlpUpdate(bool)));
    form_dlp->addRow("", chk_dlp_auto);

    vbox->addStretch();

    QTimer::singleShot(0, this, SLOT(updateDlpFields()));

}

void
MainSettingsWidget::updateDlpFields()
{
    bool own_copy_found = false;
    bool own_copy_managed = m_storage.downloadToolManaged();
    QString location = m_storage.downloadToolExe(&own_copy_found);
    bool updates_enabled = m_storage.downloadToolUpdates();

    if (own_copy_managed)
    {
        btn_dlp_manage_own->setDisabled(own_copy_found);
        btn_dlp_stop_managing_own->setDisabled(false);
        chk_dlp_auto->setDisabled(false);
        chk_dlp_auto->setChecked(updates_enabled);

    }
    else
    {
        btn_dlp_manage_own->setDisabled(false);
        btn_dlp_stop_managing_own->setDisabled(true);
        chk_dlp_auto->setDisabled(true);

    }

}

void
MainSettingsWidget::enableDlpManaged()
{
    m_storage.installDownloadTool();

    QMessageBox::information(this, tr("Download helper"),
        tr("The download helper program has been installed locally."));

    updateDlpFields();
}

void
MainSettingsWidget::disableDlpManaged()
{
    if (QMessageBox::question(this, tr("Download helper"),
        tr("Do you want to remove the download helper? It's recommended to keep the helper program.")) != QMessageBox::Yes)
        return;

    m_storage.removeDownloadTool();

    QMessageBox::information(this, tr("Download helper"),
        tr("The download helper program has been removed. From now on, the version found on your system will be used."));

    updateDlpFields();
}

void
MainSettingsWidget::setDlpUpdate(bool enabled)
{
    m_storage.setDownloadToolUpdate(enabled);

    updateDlpFields();
}

