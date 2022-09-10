#include "textediter.h"
#include <QMimeData>
#include <QFileInfo>
#include <QImageReader>

TextEditer::TextEditer(QWidget *parent) : QTextBrowser(parent)
{
    m_mode = false;
}

void TextEditer::setMode(bool mode)
{
    this->setReadOnly(mode);
}

bool TextEditer::getMode() const
{
    return m_mode;
}

bool TextEditer::canInsertFromMimeData(const QMimeData *source) const
{
    if (m_mode)
        return false;
    else
        return (source->hasImage()
                || source->hasUrls()
                || QTextEdit::canInsertFromMimeData(source));
}

void TextEditer::insertFromMimeData(const QMimeData *source)
{
    if (source->hasImage())
    {
        static int i = 1;
        QUrl url(QString("dropped_image_%1").arg(i++));
        dropImage(url, qvariant_cast<QImage>(source->imageData()));
    }
    else if (source->hasUrls())
    {
        foreach (QUrl url, source->urls())
        {
            QFileInfo info(url.toLocalFile());
            if (QImageReader::supportedImageFormats()
                    .contains(info.suffix().toLower().toLatin1()))
                dropImage(url, QImage(info.filePath()));
            else
                dropTextFile(url);
        }
    }
    else
    {
        QTextEdit::insertFromMimeData(source);
    }
}

void TextEditer::dropImage(const QUrl &url, const QImage &image)
{
    if (!image.isNull())
    {
        document()->addResource(QTextDocument::ImageResource, url, image);
        QTextImageFormat imageFormat;
        imageFormat.setWidth(image.width());
        imageFormat.setHeight(image.height());
        imageFormat.setName(url.toString());
        textCursor().insertImage(imageFormat);
    }
}

void TextEditer::dropTextFile(const QUrl &url)
{
    QFile file(url.toLocalFile());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        textCursor().insertText(file.readAll());
}

