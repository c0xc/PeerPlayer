#ifndef IMAGELABEL_HPP
#define IMAGELABEL_HPP

#include <QDebug>
#include <QPixmap>
#include <QLabel>
#include <QResizeEvent>
#include <QBuffer>
#include <QImageReader>
#include <QPainter>
#include <QTimer>

class ImageWidget : public QLabel
{
    Q_OBJECT

signals:

public:

    ImageWidget(bool fixed = false, QWidget *parent = 0);

    void
    limitHeightToWidth(bool limit_h);

    void
    enforceHeight(bool fixed = true);

    void
    setPixmap(const QPixmap &pixmap);

    void
    setPixmap(const QByteArray &bytes);

    QSize
    sizeHint() const;

    QSize
    pixmapSize() const;

    int
    heightForWidth(int width) const;

    void
    paintEvent(QPaintEvent* event);

    void
    resizeEvent(QResizeEvent *event);

public slots:

    void
    updateSize();

    void
    adjustSizeTo(QSize reference_size); //not implemented

private:

    QPixmap
    m_pixmap;

    bool
    m_limit_h;

    int
    m_last_adjusted_height;

};

#endif
