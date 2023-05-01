#include "vlcplayer.hpp"

VlcPlayer::VlcPlayer(const QUrl &url, QWidget *parent)
         : QWidget(parent),
           m_vlc_instance(0),
           m_vlc_player(0),
           m_vlc_media(0),
           m_last_volume(0)
{

    // Initialize libVLC instance
    m_vlc_instance = libvlc_new(0, 0); //0, NULL
    if (!m_vlc_instance)
    {
        QMessageBox::critical(this, tr("Cannot load video player"),
            tr("Failed to load VLC video player."));
        QTimer::singleShot(0, this, SLOT(unload()));
        return;
    }

    QVBoxLayout *vbox = new QVBoxLayout;
    QHBoxLayout *hbox_buttons = new QHBoxLayout;

    m_btn_play = new QPushButton("Play");
    connect(m_btn_play, SIGNAL(clicked()), this, SLOT(play()));
    hbox_buttons->addWidget(m_btn_play);

    m_btn_stop = new QPushButton("Stop");
    connect(m_btn_stop, SIGNAL(clicked()), this, SLOT(stop()));
    hbox_buttons->addWidget(m_btn_stop);

    hbox_buttons->addStretch();

    m_btn_mute = new QPushButton("Mute");
    connect(m_btn_mute, SIGNAL(clicked()), this, SLOT(mute()));
    hbox_buttons->addWidget(m_btn_mute);

    m_volume_slider = new QSlider(Qt::Horizontal);
    connect(m_volume_slider, SIGNAL(sliderMoved(int)), this, SLOT(setVolume(int)));
    m_volume_slider->setValue(50);
    hbox_buttons->addWidget(m_volume_slider);

    QHBoxLayout *hbox_position = new QHBoxLayout;
    m_position_slider = new QSlider(Qt::Horizontal);
    m_position_slider->setMaximum(1000);
    connect(m_position_slider, SIGNAL(sliderMoved(int)), this, SLOT(setPosition(int)));
    //TODO on (ctrl) hover show a second player in a tiny overlay window
    m_lbl_position = new QLabel;
    m_lbl_duration = new QLabel;
    hbox_position->addWidget(m_lbl_position);
    hbox_position->addWidget(m_position_slider);
    hbox_position->addWidget(m_lbl_duration);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateInterface()));
    timer->start(100);

    m_video_widget = new QWidget;
    m_video_widget->setAutoFillBackground(true);
    QPalette palette = m_video_widget->palette();
    palette.setColor(QPalette::Window, Qt::black);
    m_video_widget->setPalette(palette);

    vbox->addWidget(m_video_widget);
    vbox->addLayout(hbox_position);
    vbox->addLayout(hbox_buttons);
    vbox->setStretch(0, 99);

    setLayout(vbox);

    // Initialize VLC player
    m_vlc_player = libvlc_media_player_new(m_vlc_instance);
    // Integrate VLC player into interface, display video in video widget
    integrateWidget();

    if (!url.isEmpty()) load(url);

}

VlcPlayer::VlcPlayer(const VlcPlayer &other, QWidget *parent)
         : VlcPlayer(other.m_url, parent)
{
    double pos = other.position();
    if (pos > 0) setPosition(pos);
}

VlcPlayer::~VlcPlayer()
{
    stop();
    libvlc_media_player_release(m_vlc_player);
    m_vlc_player = 0;
    if (m_vlc_instance) libvlc_release(m_vlc_instance);
}

bool
VlcPlayer::isPlaying()
{
    if (!m_vlc_player) return false;
    return libvlc_media_player_is_playing(m_vlc_player);
}

double
VlcPlayer::position() const
{
    if (!m_vlc_player) return 0;
    float pos = libvlc_media_player_get_position(m_vlc_player);
    return pos;
}

void
VlcPlayer::load(const QUrl &url)
{
    if (isPlaying()) stop();
    m_url = url;

    // Load media
    QByteArray url_bytes = url.url().toUtf8();
    m_vlc_media = libvlc_media_new_location(m_vlc_instance, url_bytes.constData());
    if (!m_vlc_media)
    {
        QMessageBox::critical(this, tr("Cannot load video"),
            tr("Failed to load video."));
        return;
    }

    // Load media in player
    if (m_vlc_player)
    {
        libvlc_media_player_set_media(m_vlc_player, m_vlc_media);
    }
    else
    {
        // Initialize player
        m_vlc_player = libvlc_media_player_new_from_media(m_vlc_media);

        // Integrate VLC player into interface, display video in video widget
        integrateWidget();
    }

    //play();

}

void
VlcPlayer::integrateWidget()
{
    // Integrate VLC player into interface, display video in video widget
    // If this fails, a new VLC window will appear displaying the video
    // If a previous vlc instance is already integrated into the widget,
    // integrating a new vlc instance into it might fail.
    auto win_id = m_video_widget->winId();
    #if defined(Q_OS_UNIX)
    libvlc_media_player_set_xwindow(m_vlc_player, win_id);
    #elif defined(Q_OS_WIN)
    libvlc_media_player_set_hwnd(m_vlc_player, (HWND)win_id);
    #elif defined(Q_OS_MAC)
    libvlc_media_player_set_nsobject(m_vlc_player, (void*)win_id);
    #endif

}

bool
VlcPlayer::play()
{
    bool ok = false;
    if (!m_vlc_player) return ok;

    if (libvlc_media_player_is_playing(m_vlc_player))
    {
        // Pause
        libvlc_media_player_pause(m_vlc_player);
        m_btn_play->setText("Play");
    }
    else
    {
        // Play
        ok = !libvlc_media_player_play(m_vlc_player);
        if (ok)
        {
            m_btn_play->setText("Pause");
            emit started();
        }
        else
        {
            QMessageBox::critical(this, tr("Cannot play video"),
                tr("Failed to start video playback."));
        }
    }

    return ok;
}

void
VlcPlayer::stop()
{
    if (!m_vlc_player) return;

    //libvlc_media_player_stop_async(_vlc_player); // NOT DECLARED
    libvlc_media_player_stop(m_vlc_player);

    // Release media
    if (m_vlc_media)
    {
        libvlc_media_release(m_vlc_media);
        m_vlc_media = 0;
    }

    m_position_slider->setValue(0);
    m_btn_play->setText(tr("Play"));
    m_position_slider->setToolTip("");
    m_lbl_position->setText("");
    m_lbl_duration->setText("");

    emit stopped();
}

void
VlcPlayer::mute()
{
    if (!m_vlc_player) return;

    int current_volume = m_volume_slider->value();
    if (current_volume == 0)
    {
        int last_volume = m_last_volume;
        if (!last_volume) last_volume = 50;
        setVolume(last_volume);
        m_volume_slider->setValue(last_volume);
        m_volume_slider->setToolTip(tr("Volume: %1%").arg(last_volume));
    }
    else
    {
        m_last_volume = current_volume;
        setVolume(0);
        m_volume_slider->setValue(0);
        m_volume_slider->setToolTip(tr("Volume: muted"));
    }
}

int
VlcPlayer::setVolume(int volume)
{
    if (!m_vlc_player) return 0;

    m_volume_slider->setToolTip(tr("Volume: %1%").arg(volume));
    return libvlc_audio_set_volume(m_vlc_player, volume);
}

void
VlcPlayer::setPosition(double position)
{
    if (!m_vlc_player) return;

    //https://videolan.videolan.me/vlc/group__libvlc__media__player.html#ga26a3f1f824516b17e92b8ea61c3cb79d
    //bool b_fast (according to doc)
    libvlc_media_player_set_position(m_vlc_player, position);
}

void
VlcPlayer::setPosition(int position)
{
    setPosition((float)(position / 1000.0));
}

void
VlcPlayer::updateInterface()
{
    if (!m_vlc_player) return;

    float pos = libvlc_media_player_get_position(m_vlc_player);
    int slider_pos = (int)(pos * 1000.0);
    if (m_position_slider->value() != slider_pos)
    {
        //Update position slider
        m_position_slider->setValue(slider_pos);

        //Update position text
        qint64 duration = 0;
        if (m_vlc_media)
            duration = libvlc_media_get_duration(m_vlc_media) / (qint64)1000;
        qint64 est_pos = (float)duration * pos;
        QString pos_str = QDateTime::fromTime_t(est_pos).toUTC().toString("hh:mm:ss");
        QString dur_str = QDateTime::fromTime_t(duration).toUTC().toString("hh:mm:ss");
        m_position_slider->setToolTip(QString("%1/%2").arg(pos_str).arg(dur_str));
        m_lbl_position->setText(pos_str);
        m_lbl_duration->setText(dur_str);

    }

    if (libvlc_media_player_get_state(m_vlc_player) == libvlc_Stopped)
        stop();

}

