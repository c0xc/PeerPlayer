#include "imagewidget.hpp"

ImageWidget::ImageWidget(bool fixed, QWidget *parent)
           : QLabel(parent),
             m_limit_h(false),
             m_last_adjusted_height(0)
{
    //Use the default size policy (expanding) and setColumnStretch()
    //to let the layout resize the image
    //Set the vertical policy to fixed if the vertical space is unlimited
    QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (fixed)
        policy.setVerticalPolicy(QSizePolicy::Fixed); //forced height
    policy.setHeightForWidth(true);
    setSizePolicy(policy);

    setMinimumSize(sizeHint());

    setStyleSheet("QLabel{border: 1px solid black; background: gray;}");
}

void
ImageWidget::limitHeightToWidth(bool limit_h)
{
    m_limit_h = limit_h;
}

void
ImageWidget::enforceHeight(bool fixed)
{
    QSizePolicy policy = sizePolicy();
    if (fixed)
        policy.setVerticalPolicy(QSizePolicy::Fixed); //forced height
    else
        policy.setVerticalPolicy(QSizePolicy::Expanding);
    setSizePolicy(policy);
}

void
ImageWidget::setPixmap(const QPixmap &pixmap)
{
    m_pixmap = pixmap;
    update();
}

void
ImageWidget::setPixmap(const QByteArray &bytes)
{
    QByteArray bytes_copy(bytes);
    QBuffer buffer(&bytes_copy);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    QPixmap pix = QPixmap::fromImageReader(&reader);
    if (pix.isNull())
    {
        //failed to parse pixmap
        //if bytes > 0: check if imageformats dir/link exists in bin/
        qInfo() << "failed to parse pixmap from byte array" << bytes.length() << reader.errorString();
        QList<QByteArray> list = QImageReader::supportedImageFormats();
        QStringList formats;
        for (int i = 0; i < list.size(); i++)
        {
            const QByteArray &x = list.at(i);
            formats << (const char*)x.data();
            //printf("%s\n", (const char*)x.data());
        }
        qInfo() << "supported image formats:" << formats.join(", ").toUtf8().data();
    }
    setPixmap(pix);
}

QSize
ImageWidget::pixmapSize() const // ... QSize max_size ...
{
    //Calculate size of pixmap as it would now fit in widget
    //Almost the same as size hint except if it's too high and therefore shrunk
    int w = width();
    QSize size = m_pixmap.size();
    size.scale(QSize(w, heightForWidth(w)), Qt::KeepAspectRatio);

    return size;
}

QSize
ImageWidget::sizeHint() const
{
    if (m_pixmap.isNull())
        return QSize(1, 1);

    {
        int w = this->width();
        return QSize(w, heightForWidth(w));
    }
}

int
ImageWidget::heightForWidth(int width) const
{
    if (m_pixmap.isNull())
        return height();

    //Calculate wanted/required of this widget relative to width
    //This is probably but may not be the same as the height of the pixmap
    int img_w = m_pixmap.width();
    int img_h = m_pixmap.height();
    int new_h = width * img_h / img_w;

    if (m_limit_h && new_h > width)
    {
        QSize size(width, new_h);
        size.scale(QSize(width, this->width()), Qt::KeepAspectRatio);
        new_h = size.height();
    }

    return new_h;

    //TODO ...
    //limit height to width - height <= width
    //grow - grow/zoom image > 100% if enough space available

}

void
ImageWidget::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);

    if (m_pixmap.isNull())
        return;

    //Create painter
    QPainter painter(this);

    //Scale to width
    //there's also event->rect() and it looks kind of funny
    //QSize size = pixmapSize();
    QSize size = m_pixmap.size();
    size.scale(this->size(), Qt::KeepAspectRatio);

    //Resize pixmap
    QPixmap pixmap = m_pixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    //Draw the pixmap
    painter.drawPixmap(QPoint(), pixmap);

    QTimer::singleShot(0, this, SLOT(updateSize()));
}

void
ImageWidget::resizeEvent(QResizeEvent *event)
{
    //Adjust height to enforce aspect ratio if enabled size policy = fixed
    //This can be used if the vertical space is unlimited like in a scroll area
    //This should not be used if the space is restricted like in a dialog
    updateSize();

    QLabel::resizeEvent(event);
}

void
ImageWidget::updateSize()
{
    //Adjust height to enforce aspect ratio if enabled size policy = fixed
    //This can be used if the vertical space is unlimited like in a scroll area
    //This should not be used if the space is restricted like in a dialog
    bool enforce_height = sizePolicy().verticalPolicy() == QSizePolicy::Fixed;
    if (enforce_height)
    {
        int wanted_height = heightForWidth(this->width());
        if (wanted_height == m_last_adjusted_height)
        {
            //Last call already resized to that height, do not resize again!
            // "preventing double resize to" << wanted_height
        }
        else
        {
            // "resize" << "adjusting height to enforce aspect ratio" << size() << "->" << wanted_height;
            m_last_adjusted_height = wanted_height;
            setFixedHeight(wanted_height);
            return;
        }
    }
}

void
ImageWidget::adjustSizeTo(QSize reference_size)
{
    //img adjusting size to...
}

