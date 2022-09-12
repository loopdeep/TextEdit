#include "textediter.h"
#include <QMimeData>
#include <QFileInfo>
#include <QImageReader>
#include <QTextCharFormat>
#include <QDebug>

TextEditer::TextEditer(QWidget *parent) : QTextBrowser(parent)
{
    setMode(false);

    auto clist = this->children();

    // 允许点击链接
    for (auto it = clist.begin(); it != clist.end(); ++it)
    {
        QObject *pObj = *it;
        QString cname = pObj->metaObject()->className();
                if (cname == "QWidgetTextControl")
                pObj->setProperty("openExternalLinks", true);
    }

    m_supportTxtList.clear();
    // 支持的文本文件内容
    m_supportTxtList << ".txt";
}

void TextEditer::setMode(bool mode)
{
    m_mode = mode;
//    this->setReadOnly(mode);
    if (mode)
    {
        this->setTextInteractionFlags(Qt::TextBrowserInteraction);
    }
    else
    {
        this->setTextInteractionFlags(Qt::TextEditorInteraction);
    }
}

bool TextEditer::getMode() const
{
    return m_mode;
}

void TextEditer::insertHyperLink(const QStringList &linkList)
{
    if (linkList.size() != 2)
        return;

    QTextCursor cursor(this->document());
    this->setTextCursor(cursor);
    QTextCharFormat linkFormat = cursor.charFormat();
    linkFormat.setAnchor(true);
//    linkFormat.setForeground(QColor('blue'));
    linkFormat.setAnchorName(linkList.at(0));
    linkFormat.setAnchorHref(linkList.at(1));
    linkFormat.setToolTip(linkList.at(0));
//    linkFormat.setFontUnderline(true);

    cursor.insertText(linkList.at(0), linkFormat);
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
            {
                // 拖入图片
                dropImage(url, QImage(info.filePath()));
            }
            else if (m_supportTxtList.contains(
                         info.suffix().toLower().toLatin1()))
            {
                // 拖入文本文件
                dropTextFile(url);
            }
            else
            {
                // 拖入链接
                QStringList linkList;
                linkList << url.fileName() << url.toString();
                insertHyperLink(linkList);
            }
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

