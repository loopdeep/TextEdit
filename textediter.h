#ifndef TEXTEDITER_H
#define TEXTEDITER_H


#include <QTextBrowser>
#include <QObject>
#include <QWidget>

class TextEditer : public QTextBrowser
{
    Q_OBJECT
public:
    explicit TextEditer(QWidget *parent = 0);

    void setMode(bool mode);
    bool getMode() const;

protected:
    bool canInsertFromMimeData(const QMimeData* source) const;

    void insertFromMimeData(const QMimeData* source);

private:
    void dropImage(const QUrl& url, const QImage& image);

    void dropTextFile(const QUrl& url);

private:
    bool m_mode; // true-浏览模式/false-编辑模式
};

#endif // TEXTEDITER_H
