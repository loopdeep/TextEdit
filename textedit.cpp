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

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QComboBox>
#include <QFontComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMenu>
#include <QMenuBar>
#include <QTextCodec>
#include <QTextEdit>
#include <QToolBar>
#include <QTextCursor>
#include <QTextDocumentWriter>
#include <QTextList>
#include <QtDebug>
#include <QCloseEvent>
#include <QMessageBox>
#include <QMimeData>
#ifndef QT_NO_PRINTER
#include <QPrintDialog>
#include <QPrinter>
#include <QPrintPreviewDialog>
#endif

#include "textedit.h"

//#ifdef Q_OS_MAC
const QString rsrcPath = ":/images/mac";
//#else
//const QString rsrcPath = ":/images/win";
//#endif

TextEdit::TextEdit(QWidget *parent)
    : QMainWindow(parent)
{
#ifdef Q_OS_OSX
    setUnifiedTitleAndToolBarOnMac(true);
#endif

    setToolButtonStyle(Qt::ToolButtonFollowStyle);
    setupFileActions();
    setupEditActions();
    setupTextActions();

    setupInsertActions();

//    {
//        QMenu *helpMenu = new QMenu(QStringLiteral("Help"), this);
//        menuBar()->addMenu(helpMenu);
//        helpMenu->addAction(QStringLiteral("About"), this, SLOT(about()));
//        helpMenu->addAction(QStringLiteral("About &Qt"), qApp, SLOT(aboutQt()));
//    }

    m_textEdit = new QTextEdit(this);
    connect(m_textEdit, SIGNAL(currentCharFormatChanged(QTextCharFormat)),
            this, SLOT(currentCharFormatChanged(QTextCharFormat)));
    connect(m_textEdit, SIGNAL(cursorPositionChanged()),
            this, SLOT(cursorPositionChanged()));

    setCentralWidget(m_textEdit);
    m_textEdit->setFocus();
    setCurrentFileName(QString());

    QFont textFont("Helvetica");
    textFont.setStyleHint(QFont::SansSerif);
    m_textEdit->setFont(textFont);
    fontChanged(m_textEdit->font());
    colorChanged(m_textEdit->textColor());
    alignmentChanged(m_textEdit->alignment());

    connect(m_textEdit->document(), SIGNAL(modificationChanged(bool)),
            m_actionSave, SLOT(setEnabled(bool)));
    connect(m_textEdit->document(), SIGNAL(modificationChanged(bool)),
            this, SLOT(setWindowModified(bool)));
    connect(m_textEdit->document(), SIGNAL(undoAvailable(bool)),
            m_actionUndo, SLOT(setEnabled(bool)));
    connect(m_textEdit->document(), SIGNAL(redoAvailable(bool)),
            m_actionRedo, SLOT(setEnabled(bool)));

    setWindowModified(m_textEdit->document()->isModified());
    m_actionSave->setEnabled(m_textEdit->document()->isModified());
    m_actionUndo->setEnabled(m_textEdit->document()->isUndoAvailable());
    m_actionRedo->setEnabled(m_textEdit->document()->isRedoAvailable());

    connect(m_actionUndo, SIGNAL(triggered()), m_textEdit, SLOT(undo()));
    connect(m_actionRedo, SIGNAL(triggered()), m_textEdit, SLOT(redo()));

    m_actionCut->setEnabled(false);
    m_actionCopy->setEnabled(false);

    connect(m_actionCut, SIGNAL(triggered()), m_textEdit, SLOT(cut()));
    connect(m_actionCopy, SIGNAL(triggered()), m_textEdit, SLOT(copy()));
    connect(m_actionPaste, SIGNAL(triggered()), m_textEdit, SLOT(paste()));

    connect(m_textEdit, SIGNAL(copyAvailable(bool)), m_actionCut, SLOT(setEnabled(bool)));
    connect(m_textEdit, SIGNAL(copyAvailable(bool)), m_actionCopy, SLOT(setEnabled(bool)));

#ifndef QT_NO_CLIPBOARD
    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardDataChanged()));
#endif

//    QString initialFile = ":/example.html";
//    const QStringList args = QCoreApplication::arguments();
//    if (args.count() == 2)
//        initialFile = args.at(1);

//    if (!load(initialFile))
//        fileNew();

    fileNew();
}

void TextEdit::closeEvent(QCloseEvent *e)
{
    if (maybeSave())
        e->accept();
    else
        e->ignore();
}

void TextEdit::setupFileActions()
{
    QToolBar *tb = new QToolBar(this);
    tb->setWindowTitle(QStringLiteral("文件操作"));
    addToolBar(tb);

    QMenu *menu = new QMenu(QStringLiteral("文件"), this);
    menuBar()->addMenu(menu);

    QAction *a;

    QIcon newIcon = QIcon::fromTheme("document-new", QIcon(rsrcPath + "/filenew.png"));
    a = new QAction(newIcon, QStringLiteral("新建"), this);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(QKeySequence::New);
    connect(a, SIGNAL(triggered()), this, SLOT(fileNew()));
    tb->addAction(a);
    menu->addAction(a);

    a = new QAction(QIcon::fromTheme("document-open", QIcon(rsrcPath + "/fileopen.png")),
                    QStringLiteral("打开"), this);
    a->setShortcut(QKeySequence::Open);
    connect(a, SIGNAL(triggered()), this, SLOT(fileOpen()));
    tb->addAction(a);
    menu->addAction(a);

    menu->addSeparator();

    m_actionSave = a = new QAction(QIcon::fromTheme("document-save", QIcon(rsrcPath + "/filesave.png")),
                                 QStringLiteral("保存"), this);
    a->setShortcut(QKeySequence::Save);
    connect(a, SIGNAL(triggered()), this, SLOT(fileSave()));
    a->setEnabled(false);
    tb->addAction(a);
    menu->addAction(a);

    a = new QAction(QStringLiteral("另存为"), this);
    a->setPriority(QAction::LowPriority);
    connect(a, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
    menu->addAction(a);
    menu->addSeparator();

#ifndef QT_NO_PRINTER
    a = new QAction(QIcon::fromTheme("document-print", QIcon(rsrcPath + "/fileprint.png")),
                    QStringLiteral("打印"), this);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(QKeySequence::Print);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrint()));
    tb->addAction(a);
    menu->addAction(a);

    a = new QAction(QIcon::fromTheme("fileprint", QIcon(rsrcPath + "/fileprint.png")),
                    QStringLiteral("打印预览"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrintPreview()));
    menu->addAction(a);

    a = new QAction(QIcon::fromTheme("exportpdf", QIcon(rsrcPath + "/exportpdf.png")),
                    QStringLiteral("导出PDF"), this);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(Qt::CTRL + Qt::Key_D);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrintPdf()));
    tb->addAction(a);
    menu->addAction(a);

    menu->addSeparator();
#endif

    a = new QAction(QStringLiteral("&Quit"), this);
    a->setShortcut(Qt::CTRL + Qt::Key_Q);
    connect(a, SIGNAL(triggered()), this, SLOT(close()));
    menu->addAction(a);
}

void TextEdit::setupEditActions()
{
    QToolBar *tb = new QToolBar(this);
    tb->setWindowTitle(QStringLiteral("编辑操作"));
    addToolBar(tb);
    QMenu *menu = new QMenu(QStringLiteral("编辑"), this);
    menuBar()->addMenu(menu);

    QAction *a;
    a = m_actionUndo = new QAction(QIcon::fromTheme("edit-undo", QIcon(rsrcPath + "/editundo.png")),
                                              QStringLiteral("重做"), this);
    a->setShortcut(QKeySequence::Undo);
    tb->addAction(a);
    menu->addAction(a);
    a = m_actionRedo = new QAction(QIcon::fromTheme("edit-redo", QIcon(rsrcPath + "/editredo.png")),
                                              QStringLiteral("回退"), this);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(QKeySequence::Redo);
    tb->addAction(a);
    menu->addAction(a);
    menu->addSeparator();
    a = m_actionCut = new QAction(QIcon::fromTheme("edit-cut", QIcon(rsrcPath + "/editcut.png")),
                                             QStringLiteral("剪切"), this);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(QKeySequence::Cut);
    tb->addAction(a);
    menu->addAction(a);
    a = m_actionCopy = new QAction(QIcon::fromTheme("edit-copy", QIcon(rsrcPath + "/editcopy.png")),
                                 QStringLiteral("复制"), this);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(QKeySequence::Copy);
    tb->addAction(a);
    menu->addAction(a);
    a = m_actionPaste = new QAction(QIcon::fromTheme("edit-paste", QIcon(rsrcPath + "/editpaste.png")),
                                  QStringLiteral("粘贴"), this);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(QKeySequence::Paste);
    tb->addAction(a);
    menu->addAction(a);
#ifndef QT_NO_CLIPBOARD
    if (const QMimeData *md = QApplication::clipboard()->mimeData())
        m_actionPaste->setEnabled(md->hasText());
#endif
}

void TextEdit::setupTextActions()
{
    QToolBar *tb = new QToolBar(this);
    tb->setWindowTitle(QStringLiteral("格式操作"));
    addToolBar(tb);

    QMenu *menu = new QMenu(QStringLiteral("格式"), this);
    menuBar()->addMenu(menu);

    m_actionTextBold = new QAction(QIcon::fromTheme("format-text-bold", QIcon(rsrcPath + "/textbold.png")),
                                 QStringLiteral("粗体"), this);
    m_actionTextBold->setShortcut(Qt::CTRL + Qt::Key_B);
    m_actionTextBold->setPriority(QAction::LowPriority);
    QFont bold;
    bold.setBold(true);
    m_actionTextBold->setFont(bold);
    connect(m_actionTextBold, SIGNAL(triggered()), this, SLOT(textBold()));
    tb->addAction(m_actionTextBold);
    menu->addAction(m_actionTextBold);
    m_actionTextBold->setCheckable(true);

    m_actionTextItalic = new QAction(QIcon::fromTheme("format-text-italic",
                                                    QIcon(rsrcPath + "/textitalic.png")),
                                   QStringLiteral("斜体"), this);
    m_actionTextItalic->setPriority(QAction::LowPriority);
    m_actionTextItalic->setShortcut(Qt::CTRL + Qt::Key_I);
    QFont italic;
    italic.setItalic(true);
    m_actionTextItalic->setFont(italic);
    connect(m_actionTextItalic, SIGNAL(triggered()), this, SLOT(textItalic()));
    tb->addAction(m_actionTextItalic);
    menu->addAction(m_actionTextItalic);
    m_actionTextItalic->setCheckable(true);

    m_actionTextUnderline = new QAction(QIcon::fromTheme("format-text-underline",
                                                       QIcon(rsrcPath + "/textunder.png")),
                                      QStringLiteral("下划线"), this);
    m_actionTextUnderline->setShortcut(Qt::CTRL + Qt::Key_U);
    m_actionTextUnderline->setPriority(QAction::LowPriority);
    QFont underline;
    underline.setUnderline(true);
    m_actionTextUnderline->setFont(underline);
    connect(m_actionTextUnderline, SIGNAL(triggered()), this, SLOT(textUnderline()));
    tb->addAction(m_actionTextUnderline);
    menu->addAction(m_actionTextUnderline);
    m_actionTextUnderline->setCheckable(true);

    menu->addSeparator();

    QActionGroup *grp = new QActionGroup(this);
    connect(grp, SIGNAL(triggered(QAction*)), this, SLOT(textAlign(QAction*)));

    // Make sure the alignLeft  is always left of the alignRight
    if (QApplication::isLeftToRight()) {
        m_actionAlignLeft = new QAction(QIcon::fromTheme("format-justify-left",
                                                       QIcon(rsrcPath + "/textleft.png")),
                                      QStringLiteral("左对齐"), grp);
        m_actionAlignCenter = new QAction(QIcon::fromTheme("format-justify-center",
                                                         QIcon(rsrcPath + "/textcenter.png")),
                                        QStringLiteral("居中"), grp);
        m_actionAlignRight = new QAction(QIcon::fromTheme("format-justify-right",
                                                        QIcon(rsrcPath + "/textright.png")),
                                       QStringLiteral("右对齐"), grp);
    } else {
        m_actionAlignRight = new QAction(QIcon::fromTheme("format-justify-right",
                                                        QIcon(rsrcPath + "/textright.png")),
                                       QStringLiteral("右对齐"), grp);
        m_actionAlignCenter = new QAction(QIcon::fromTheme("format-justify-center",
                                                         QIcon(rsrcPath + "/textcenter.png")),
                                        QStringLiteral("居中"), grp);
        m_actionAlignLeft = new QAction(QIcon::fromTheme("format-justify-left",
                                                       QIcon(rsrcPath + "/textleft.png")),
                                      QStringLiteral("左对齐"), grp);
    }
    m_actionAlignJustify = new QAction(QIcon::fromTheme("format-justify-fill",
                                                      QIcon(rsrcPath + "/textjustify.png")),
                                     QStringLiteral("两端对齐"), grp);

    m_actionAlignLeft->setShortcut(Qt::CTRL + Qt::Key_L);
    m_actionAlignLeft->setCheckable(true);
    m_actionAlignLeft->setPriority(QAction::LowPriority);
    m_actionAlignCenter->setShortcut(Qt::CTRL + Qt::Key_E);
    m_actionAlignCenter->setCheckable(true);
    m_actionAlignCenter->setPriority(QAction::LowPriority);
    m_actionAlignRight->setShortcut(Qt::CTRL + Qt::Key_R);
    m_actionAlignRight->setCheckable(true);
    m_actionAlignRight->setPriority(QAction::LowPriority);
    m_actionAlignJustify->setShortcut(Qt::CTRL + Qt::Key_J);
    m_actionAlignJustify->setCheckable(true);
    m_actionAlignJustify->setPriority(QAction::LowPriority);

    tb->addActions(grp->actions());
    menu->addActions(grp->actions());

    menu->addSeparator();

    QPixmap pix(16, 16);
    pix.fill(Qt::black);
    m_actionTextColor = new QAction(pix, QStringLiteral("字体颜色"), this);
    connect(m_actionTextColor, SIGNAL(triggered()), this, SLOT(textColor()));
    tb->addAction(m_actionTextColor);
    menu->addAction(m_actionTextColor);

//    tb = new QToolBar(this);
//    tb->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
//    tb->setWindowTitle(QStringLiteral("Format Actions"));
//    addToolBarBreak(Qt::TopToolBarArea);
//    addToolBar(tb);

    m_comboStyle = new QComboBox(tb);
    tb->addWidget(m_comboStyle);
    m_comboStyle->addItem("Standard");
    m_comboStyle->addItem("Bullet List (Disc)");
    m_comboStyle->addItem("Bullet List (Circle)");
    m_comboStyle->addItem("Bullet List (Square)");
    m_comboStyle->addItem("Ordered List (Decimal)");
    m_comboStyle->addItem("Ordered List (Alpha lower)");
    m_comboStyle->addItem("Ordered List (Alpha upper)");
    m_comboStyle->addItem("Ordered List (Roman lower)");
    m_comboStyle->addItem("Ordered List (Roman upper)");
    connect(m_comboStyle, SIGNAL(activated(int)), this, SLOT(textStyle(int)));

    m_comboFont = new QFontComboBox(tb);
    tb->addWidget(m_comboFont);
    connect(m_comboFont, SIGNAL(activated(QString)), this, SLOT(textFamily(QString)));

    m_comboSize = new QComboBox(tb);
    m_comboSize->setObjectName("m_comboSize");
    tb->addWidget(m_comboSize);
    m_comboSize->setEditable(true);

    QFontDatabase db;
    foreach(int size, db.standardSizes())
        m_comboSize->addItem(QString::number(size));

    connect(m_comboSize, SIGNAL(activated(QString)), this, SLOT(textSize(QString)));
    m_comboSize->setCurrentIndex(m_comboSize->findText(QString::number(QApplication::font()
                                                                       .pointSize())));
}

void TextEdit::setupInsertActions()
{
    QToolBar *tb = new QToolBar(this);
    tb->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    addToolBarBreak(Qt::TopToolBarArea);
    tb->setWindowTitle(QStringLiteral("插入"));
    addToolBar(tb);
    m_actionInsertImg = new QAction(QIcon::fromTheme("insert-image", QIcon(rsrcPath + "/image.png")),
                                 QStringLiteral("图片"), this);
    m_actionInsertImg->setShortcut(Qt::CTRL + Qt::Key_I);
    m_actionInsertImg->setPriority(QAction::NormalPriority);
    m_actionInsertImg->setToolTip(QStringLiteral("插入图片"));

    connect(m_actionInsertImg, SIGNAL(triggered()), this, SLOT(insertImageDlg()));
    tb->addAction(m_actionInsertImg);
//    menu->addAction(m_actionInsertImg);
    m_actionTextBold->setCheckable(false);
}

bool TextEdit::load(const QString &f)
{
    if (!QFile::exists(f))
        return false;
    QFile file(f);
    if (!file.open(QFile::ReadOnly))
        return false;

    QByteArray data = file.readAll();
    QTextCodec *codec = Qt::codecForHtml(data);
    QString str = codec->toUnicode(data);
    if (Qt::mightBeRichText(str)) {
        m_textEdit->setHtml(str);
    } else {
        str = QString::fromLocal8Bit(data);
        m_textEdit->setPlainText(str);
    }

    setCurrentFileName(f);
    return true;
}

bool TextEdit::maybeSave()
{
    if (!m_textEdit->document()->isModified())
        return true;

    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, QStringLiteral("提示"),
                               QStringLiteral("文件已被修改，是否保存？"),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)
        return fileSave();
    else if (ret == QMessageBox::Cancel)
        return false;
    return true;
}

void TextEdit::setCurrentFileName(const QString &fileName)
{
    this->m_fileName = fileName;
    m_textEdit->document()->setModified(false);

    QString shownName;
    if (m_fileName.isEmpty())
        shownName = "untitled.txt";
    else
        shownName = QFileInfo(m_fileName).fileName();

    setWindowTitle(QStringLiteral("%1[*] - %2").arg(shownName).arg(QStringLiteral("Rich Text")));
    setWindowModified(false);
}

void TextEdit::fileNew()
{
    if (maybeSave()) {
        m_textEdit->clear();
        setCurrentFileName(QString());
    }
}

void TextEdit::fileOpen()
{
    QString fn = QFileDialog::getOpenFileName(this, QStringLiteral("Open File..."),
                                              QString(), QStringLiteral("HTML-Files (*.htm *.html);;All Files (*)"));
    if (!fn.isEmpty())
        load(fn);
}

bool TextEdit::fileSave()
{
    if (m_fileName.isEmpty())
        return fileSaveAs();
    if (m_fileName.startsWith(QStringLiteral(":/")))
        return fileSaveAs();

    QTextDocumentWriter writer(m_fileName);
    bool success = writer.write(m_textEdit->document());
    if (success)
        m_textEdit->document()->setModified(false);
    return success;
}

bool TextEdit::fileSaveAs()
{
    QString fn = QFileDialog::getSaveFileName(this, QStringLiteral("Save as..."), QString(),
                                              tr("ODF files (*.odt);;HTML-Files "
                                                 "(*.htm *.html);;All Files (*)"));
    if (fn.isEmpty())
        return false;
    if (!(fn.endsWith(".odt", Qt::CaseInsensitive)
          || fn.endsWith(".htm", Qt::CaseInsensitive)
          || fn.endsWith(".html", Qt::CaseInsensitive))) {
        fn += ".odt"; // default
    }
    setCurrentFileName(fn);
    return fileSave();
}

void TextEdit::filePrint()
{
#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog *dlg = new QPrintDialog(&printer, this);
    if (m_textEdit->textCursor().hasSelection())
        dlg->addEnabledOption(QAbstractPrintDialog::PrintSelection);
    dlg->setWindowTitle(QStringLiteral("Print Document"));
    if (dlg->exec() == QDialog::Accepted)
        m_textEdit->print(&printer);
    delete dlg;
#endif
}

void TextEdit::filePrintPreview()
{
#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, SIGNAL(paintRequested(QPrinter*)), SLOT(printPreview(QPrinter*)));
    preview.exec();
#endif
}

void TextEdit::printPreview(QPrinter *printer)
{
#ifdef QT_NO_PRINTER
    Q_UNUSED(printer);
#else
    m_textEdit->print(printer);
#endif
}


void TextEdit::filePrintPdf()
{
#ifndef QT_NO_PRINTER
//! [0]
    QString m_fileName = QFileDialog::getSaveFileName(this, "Export PDF",
                                                    QString(), "*.pdf");
    if (!m_fileName.isEmpty()) {
        if (QFileInfo(m_fileName).suffix().isEmpty())
            m_fileName.append(".pdf");
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(m_fileName);
        m_textEdit->document()->print(&printer);
    }
//! [0]
#endif
}

void TextEdit::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(m_actionTextBold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void TextEdit::textUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(m_actionTextUnderline->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void TextEdit::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(m_actionTextItalic->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void TextEdit::textFamily(const QString &f)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(f);
    mergeFormatOnWordOrSelection(fmt);
}

void TextEdit::textSize(const QString &p)
{
    qreal pointSize = p.toFloat();
    if (p.toFloat() > 0) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pointSize);
        mergeFormatOnWordOrSelection(fmt);
    }
}

void TextEdit::textStyle(int styleIndex)
{
    QTextCursor cursor = m_textEdit->textCursor();

    if (styleIndex != 0) {
        QTextListFormat::Style style = QTextListFormat::ListDisc;

        switch (styleIndex) {
            default:
            case 1:
                style = QTextListFormat::ListDisc;
                break;
            case 2:
                style = QTextListFormat::ListCircle;
                break;
            case 3:
                style = QTextListFormat::ListSquare;
                break;
            case 4:
                style = QTextListFormat::ListDecimal;
                break;
            case 5:
                style = QTextListFormat::ListLowerAlpha;
                break;
            case 6:
                style = QTextListFormat::ListUpperAlpha;
                break;
            case 7:
                style = QTextListFormat::ListLowerRoman;
                break;
            case 8:
                style = QTextListFormat::ListUpperRoman;
                break;
        }

        cursor.beginEditBlock();

        QTextBlockFormat blockFmt = cursor.blockFormat();

        QTextListFormat listFmt;

        if (cursor.currentList()) {
            listFmt = cursor.currentList()->format();
        } else {
            listFmt.setIndent(blockFmt.indent() + 1);
            blockFmt.setIndent(0);
            cursor.setBlockFormat(blockFmt);
        }

        listFmt.setStyle(style);

        cursor.createList(listFmt);

        cursor.endEditBlock();
    } else {
        // ####
        QTextBlockFormat bfmt;
        bfmt.setObjectIndex(-1);
        cursor.mergeBlockFormat(bfmt);
    }
}

void TextEdit::textColor()
{
    QColor col = QColorDialog::getColor(m_textEdit->textColor(), this);
    if (!col.isValid())
        return;
    QTextCharFormat fmt;
    fmt.setForeground(col);
    mergeFormatOnWordOrSelection(fmt);
    colorChanged(col);
}

void TextEdit::textAlign(QAction *a)
{
    if (a == m_actionAlignLeft)
        m_textEdit->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
    else if (a == m_actionAlignCenter)
        m_textEdit->setAlignment(Qt::AlignHCenter);
    else if (a == m_actionAlignRight)
        m_textEdit->setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
    else if (a == m_actionAlignJustify)
        m_textEdit->setAlignment(Qt::AlignJustify);
}

void TextEdit::insertImageDlg()
{

}

// 插入图片
void TextEdit::insertImage(const QString &image)
{

}

void TextEdit::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
}

void TextEdit::cursorPositionChanged()
{
    alignmentChanged(m_textEdit->alignment());
}

void TextEdit::clipboardDataChanged()
{
#ifndef QT_NO_CLIPBOARD
    if (const QMimeData *md = QApplication::clipboard()->mimeData())
        m_actionPaste->setEnabled(md->hasText());
#endif
}

void TextEdit::about()
{
    QMessageBox::about(this, tr("About"), tr("This example demonstrates Qt's "
        "rich text editing facilities in m_action, providing an example "
        "document for you to experiment with."));
}

void TextEdit::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = m_textEdit->textCursor();
//    if (!cursor.hasSelection())
//        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    m_textEdit->mergeCurrentCharFormat(format);
}

void TextEdit::fontChanged(const QFont &f)
{
    m_comboFont->setCurrentIndex(m_comboFont->findText(QFontInfo(f).family()));
    m_comboSize->setCurrentIndex(m_comboSize->findText(QString::number(f.pointSize())));
    m_actionTextBold->setChecked(f.bold());
    m_actionTextItalic->setChecked(f.italic());
    m_actionTextUnderline->setChecked(f.underline());
}

void TextEdit::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    m_actionTextColor->setIcon(pix);
}

void TextEdit::alignmentChanged(Qt::Alignment a)
{
    if (a & Qt::AlignLeft)
        m_actionAlignLeft->setChecked(true);
    else if (a & Qt::AlignHCenter)
        m_actionAlignCenter->setChecked(true);
    else if (a & Qt::AlignRight)
        m_actionAlignRight->setChecked(true);
    else if (a & Qt::AlignJustify)
        m_actionAlignJustify->setChecked(true);
}

