#ifndef VLCPLAYER
#define VLCPLAYER

#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QMessageBox>
#include <QTimer>
#include <QUrl>
#include <QDateTime>
#include <QLabel>

#include <vlc/vlc.h>

class VlcPlayer : public QWidget
{
    Q_OBJECT

signals:

    void
    started();

    void
    stopped();

public:

    VlcPlayer(const QUrl &url = QUrl(), QWidget *parent = 0);

    VlcPlayer(const VlcPlayer &other, QWidget *parent = 0);

    ~VlcPlayer();

    bool
    isPlaying();

    double
    position() const;

public slots:

    void
    load(const QUrl &url);

    void
    integrateWidget();

    bool
    play();

    void
    stop();

    void
    mute();

    int
    setVolume(int volume);

    void
    setPosition(double position);

    void
    setPosition(int position);

    void
    updateInterface();

private:

    void
    init();

    libvlc_instance_t
    *m_vlc_instance;

    libvlc_media_player_t
    *m_vlc_player;

    libvlc_media_t
    *m_vlc_media;

    QUrl
    m_url;

    int
    m_last_volume;

    QPushButton
    *m_btn_play;

    QPushButton
    *m_btn_stop;

    QPushButton
    *m_btn_mute;

    QLabel
    *m_lbl_position;

    QLabel
    *m_lbl_duration;

    QSlider
    *m_volume_slider;

    QSlider
    *m_position_slider;

    QWidget
    *m_video_widget;

};

#endif
