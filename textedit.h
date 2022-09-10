/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TEXTEDIT_H
#define TEXTEDIT_H

//#pragma execution_character_set("utf-8")

#include <QMainWindow>
#include <QMap>
#include <QPointer>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QSpacerItem>
#include <QFileDialog>

#include "textediter.h"

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QFontComboBox;
class QTextEdit;
class QTextCharFormat;
class QMenu;
class QPrinter;
QT_END_NAMESPACE


class InsertLinkDlg : public QDialog
{
    Q_OBJECT

public:
    explicit InsertLinkDlg(QWidget *parent = 0);
    ~InsertLinkDlg();

    void initDlg();

signals:
    void hyperLinkCreateSig(QStringList list);

private slots:
    void onHyperlinkClicked();
    void onFilelinkClicked();

    void onBtnConfirmClicked();
    void onBtnCancelClicked();
    void onOpenFile();

private:
    QLabel *m_showLabel;
    QLineEdit *m_showEdit;
    QLabel *m_linkLabel;
    QLineEdit *m_linkEdit;
    QPushButton *m_btn_openLink;

    QPushButton *m_hyperLink;
    QPushButton *m_fileLink;

    QPushButton *m_btn_confirm;
    QPushButton *m_btn_cancel;
};

class TextEdit : public QMainWindow
{
    Q_OBJECT

public:
    explicit TextEdit(QWidget *parent = 0);

protected:
    virtual void closeEvent(QCloseEvent *e) Q_DECL_OVERRIDE;
//    virtual void dragEnterEvent(QDragEnterEvent *e);
//    virtual void dropEnterEvent(QDropEvent *e);

private:
    void setupFileActions();
    void setupEditActions();
    void setupTextActions();
    void setupInsertActions();

    bool load(const QString &f);
    bool maybeSave();
    void setCurrentFileName(const QString &fileName);

private slots:
    void fileNew();
    void fileOpen();
    bool fileSave();
    bool fileSaveAs();
    void filePrint();
    void filePrintPreview();
    void filePrintPdf();

    void textBold();
    void textUnderline();
    void textItalic();
    void textFamily(const QString &f);
    void textSize(const QString &p);
    void textStyle(int styleIndex);
    void textColor();
    void textAlign(QAction *a);

    void insertImageDlg();
    void insertImage(const QString &imgPath);
    void insertHyperLinkDlg();
    void insertHyperLink(QStringList link);

    void currentCharFormatChanged(const QTextCharFormat &format);
    void cursorPositionChanged();

    void clipboardDataChanged();
    void about();
    void printPreview(QPrinter *);

private:
    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    void fontChanged(const QFont &f);
    void colorChanged(const QColor &c);
    void alignmentChanged(Qt::Alignment a);

    QAction *m_actionSave;
    QAction *m_actionTextBold;
    QAction *m_actionTextUnderline;
    QAction *m_actionTextItalic;
    QAction *m_actionTextColor;
    QAction *m_actionAlignLeft;
    QAction *m_actionAlignCenter;
    QAction *m_actionAlignRight;
    QAction *m_actionAlignJustify;
    QAction *m_actionUndo;
    QAction *m_actionRedo;
    QAction *m_actionCut;
    QAction *m_actionCopy;
    QAction *m_actionPaste;
    QAction *m_actionInsertImg;
    QAction *m_actionInsertLink;

    QComboBox *m_comboStyle;
    QFontComboBox *m_comboFont;
    QComboBox *m_comboSize;

    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_formatMenu;
    QMenu *m_insertMenu;
    QMenu *m_helpMenu;
    QString m_fileName;
    TextEditer *m_textEdit;

    InsertLinkDlg *m_insertDlg;
};

#endif // TEXTEDIT_H
