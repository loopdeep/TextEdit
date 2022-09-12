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

#include <QImageReader>

#include <QDesktopServices>

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

    {
        m_helpMenu = new QMenu(QStringLiteral("帮助"), this);
        menuBar()->addMenu(m_helpMenu);
        m_helpMenu->addAction(QIcon::fromTheme("help-about",
                                               QIcon(rsrcPath + "/about.png")),
                              QStringLiteral("关于"),
                              this,
                              SLOT(about()));
    }

    m_insertDlg = nullptr;

    m_textEdit = new TextEditer(this);
    m_textEdit->setMode(false);
    connect(m_textEdit, SIGNAL(currentCharFormatChanged(QTextCharFormat)),
            this, SLOT(currentCharFormatChanged(QTextCharFormat)));
    connect(m_textEdit, SIGNAL(cursorPositionChanged()),
            this, SLOT(cursorPositionChanged()));

    setCentralWidget(m_textEdit);
    m_textEdit->setFocus();
    setCurrentFileName(QString());

    QFont textFont(QStringLiteral("微软雅黑"));
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

    QList<QByteArray> supportImgFmt = QImageReader::supportedImageFormats();
    for (auto it = supportImgFmt.begin(); it != supportImgFmt.end(); ++it)
    {
        if (it == supportImgFmt.begin())
            m_supportImgFormat.append("*." + *it);
        m_supportImgFormat.append(" *." + *it);
    }

    fileNew();

    int rightMar, leftMar, topMar, bottomMar;
    m_textEdit->getContentsMargins(&leftMar, &topMar, &rightMar, &bottomMar);
    qDebug() << leftMar << topMar << rightMar << bottomMar;
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

    m_fileMenu = new QMenu(QStringLiteral("文件"), this);
    menuBar()->addMenu(m_fileMenu);

    QAction *a;

    QIcon newIcon = QIcon::fromTheme("document-new", QIcon(rsrcPath + "/filenew.png"));
    a = new QAction(newIcon, QStringLiteral("新建"), this);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(QKeySequence::New);
    connect(a, SIGNAL(triggered()), this, SLOT(fileNew()));
    tb->addAction(a);
    m_fileMenu->addAction(a);

    a = new QAction(QIcon::fromTheme("document-open", QIcon(rsrcPath + "/fileopen.png")),
                    QStringLiteral("打开"), this);
    a->setShortcut(QKeySequence::Open);
    connect(a, SIGNAL(triggered()), this, SLOT(fileOpen()));
    tb->addAction(a);
    m_fileMenu->addAction(a);

    m_fileMenu->addSeparator();

    m_actionSave = a = new QAction(QIcon::fromTheme("document-save", QIcon(rsrcPath + "/filesave.png")),
                                 QStringLiteral("保存"), this);
    a->setShortcut(QKeySequence::Save);
    connect(a, SIGNAL(triggered()), this, SLOT(fileSave()));
    a->setEnabled(false);
    tb->addAction(a);
    m_fileMenu->addAction(a);

    a = new QAction(QStringLiteral("另存为"), this);
    a->setPriority(QAction::LowPriority);
    connect(a, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
    m_fileMenu->addAction(a);
    m_fileMenu->addSeparator();

#ifndef QT_NO_PRINTER
    a = new QAction(QIcon::fromTheme("document-print", QIcon(rsrcPath + "/fileprint.png")),
                    QStringLiteral("打印"), this);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(QKeySequence::Print);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrint()));
    tb->addAction(a);
    m_fileMenu->addAction(a);

    a = new QAction(QIcon::fromTheme("fileprint", QIcon(rsrcPath + "/fileprint.png")),
                    QStringLiteral("打印预览"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrintPreview()));
    m_fileMenu->addAction(a);

    a = new QAction(QIcon::fromTheme("exportpdf", QIcon(rsrcPath + "/exportpdf.png")),
                    QStringLiteral("导出PDF"), this);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(Qt::CTRL + Qt::Key_D);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrintPdf()));
    tb->addAction(a);
    m_fileMenu->addAction(a);

    m_fileMenu->addSeparator();
#endif

    a = new QAction(QStringLiteral("退出"), this);
    a->setShortcut(Qt::CTRL + Qt::Key_Q);
    connect(a, SIGNAL(triggered()), this, SLOT(close()));
    m_fileMenu->addAction(a);
}

void TextEdit::setupEditActions()
{
    QToolBar *tb = new QToolBar(this);
    tb->setWindowTitle(QStringLiteral("编辑操作"));
    addToolBar(tb);
    m_editMenu = new QMenu(QStringLiteral("编辑"), this);
    menuBar()->addMenu(m_editMenu);

    QAction *a;
    a = m_actionUndo = new QAction(QIcon::fromTheme("edit-undo", QIcon(rsrcPath + "/editundo.png")),
                                              QStringLiteral("重做"), this);
    a->setShortcut(QKeySequence::Undo);
    tb->addAction(a);
    m_editMenu->addAction(a);
    a = m_actionRedo = new QAction(QIcon::fromTheme("edit-redo", QIcon(rsrcPath + "/editredo.png")),
                                              QStringLiteral("回退"), this);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(QKeySequence::Redo);
    tb->addAction(a);
    m_editMenu->addAction(a);
    m_editMenu->addSeparator();
    a = m_actionCut = new QAction(QIcon::fromTheme("edit-cut", QIcon(rsrcPath + "/editcut.png")),
                                             QStringLiteral("剪切"), this);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(QKeySequence::Cut);
    tb->addAction(a);
    m_editMenu->addAction(a);
    a = m_actionCopy = new QAction(QIcon::fromTheme("edit-copy", QIcon(rsrcPath + "/editcopy.png")),
                                 QStringLiteral("复制"), this);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(QKeySequence::Copy);
    tb->addAction(a);
    m_editMenu->addAction(a);
    a = m_actionPaste = new QAction(QIcon::fromTheme("edit-paste", QIcon(rsrcPath + "/editpaste.png")),
                                  QStringLiteral("粘贴"), this);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(QKeySequence::Paste);
    tb->addAction(a);
    m_editMenu->addAction(a);
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

    m_formatMenu = new QMenu(QStringLiteral("格式"), this);
    menuBar()->addMenu(m_formatMenu);

    m_actionTextBold = new QAction(QIcon::fromTheme("format-text-bold", QIcon(rsrcPath + "/textbold.png")),
                                 QStringLiteral("粗体"), this);
    m_actionTextBold->setShortcut(Qt::CTRL + Qt::Key_B);
    m_actionTextBold->setPriority(QAction::LowPriority);
    QFont bold;
    bold.setBold(true);
    m_actionTextBold->setFont(bold);
    connect(m_actionTextBold, SIGNAL(triggered()), this, SLOT(textBold()));
    tb->addAction(m_actionTextBold);
    m_formatMenu->addAction(m_actionTextBold);
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
    m_formatMenu->addAction(m_actionTextItalic);
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
    m_formatMenu->addAction(m_actionTextUnderline);
    m_actionTextUnderline->setCheckable(true);

    m_formatMenu->addSeparator();



    QPixmap pix(16, 16);
    pix.fill(Qt::black);
    m_actionTextColor = new QAction(pix, QStringLiteral("字体颜色"), this);
    connect(m_actionTextColor, SIGNAL(triggered()),
            this, SLOT(textColor()));
    tb->addAction(m_actionTextColor);
    m_formatMenu->addAction(m_actionTextColor);
    m_formatMenu->addSeparator();

    m_comboFont = new QFontComboBox(tb);
    tb->addWidget(m_comboFont);
    connect(m_comboFont, SIGNAL(activated(QString)),
            this, SLOT(textFamily(QString)));

    m_comboSize = new QComboBox(tb);
    m_comboSize->setObjectName("m_comboSize");
    tb->addWidget(m_comboSize);
    m_comboSize->setEditable(true);

    QFontDatabase db;
    foreach(int size, db.standardSizes())
        m_comboSize->addItem(QString::number(size));

    connect(m_comboSize, SIGNAL(activated(QString)),
            this, SLOT(textSize(QString)));
    m_comboSize->setCurrentIndex(m_comboSize->findText(
                    QString::number(QApplication::font().pointSize())));


    tb->addSeparator();


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
    m_formatMenu->addActions(grp->actions());

    m_formatMenu->addSeparator();
}

void TextEdit::setupInsertActions()
{
    QToolBar *tb = new QToolBar(this);
    tb->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    addToolBarBreak(Qt::TopToolBarArea);
    tb->setWindowTitle(QStringLiteral("插入"));
    addToolBar(tb);

    m_insertMenu = new QMenu(QStringLiteral("插入"), this);

    m_comboStyle = new QComboBox(tb);
    tb->addWidget(m_comboStyle);

    QStringList styleList;

    styleList << QStringLiteral("(无)")
              << QStringLiteral("●")
              << QStringLiteral("○")
              << QStringLiteral("■")
              << QStringLiteral("1,2,3,...")
              << QStringLiteral("a,b,c,...")
              << QStringLiteral("A,B,C,...")
              << QStringLiteral("ⅰ,ⅱ,ⅲ,...")
              << QStringLiteral("Ⅰ,Ⅱ,Ⅲ,...");
    m_comboStyle->addItems(styleList);
    connect(m_comboStyle, SIGNAL(activated(int)), this, SLOT(textStyle(int)));

    m_actionInsertImg = new QAction(QIcon::fromTheme("insert-image",
                                                     QIcon(rsrcPath + "/image.png")),
                                    QStringLiteral("图片"),
                                    this);
    m_actionInsertImg->setShortcut(Qt::CTRL + Qt::Key_I);
    m_actionInsertImg->setPriority(QAction::NormalPriority);
    m_actionInsertImg->setToolTip(QStringLiteral("插入图片"));

    connect(m_actionInsertImg, SIGNAL(triggered()), this, SLOT(insertImageDlg()));
    tb->addAction(m_actionInsertImg);

    m_insertMenu->addAction(m_actionInsertImg);
    m_actionTextBold->setCheckable(false);



    m_actionInsertLink = new QAction(QIcon::fromTheme("insert-link",
                                                     QIcon(rsrcPath + "/link.png")),
                                    QStringLiteral("链接"),
                                    this);
    m_actionInsertLink->setShortcut(Qt::CTRL + Qt::Key_L);
    m_actionInsertLink->setPriority(QAction::NormalPriority);
    m_actionInsertLink->setToolTip(QStringLiteral("插入超链接"));

    connect(m_actionInsertLink, SIGNAL(triggered()), this, SLOT(insertHyperLinkDlg()));
    tb->addAction(m_actionInsertLink);

    m_insertMenu->addAction(m_actionInsertLink);

    menuBar()->addMenu(m_insertMenu);
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
    QMessageBox box(QMessageBox::Warning, QStringLiteral("提示"),
                    QStringLiteral("文件已被修改，是否保存？"),
                    QMessageBox::Save
                    | QMessageBox::Discard
                    | QMessageBox::Cancel);

    box.setButtonText(QMessageBox::Save, QStringLiteral("保存"));
    box.setButtonText(QMessageBox::Discard, QStringLiteral("丢弃"));
    box.setButtonText(QMessageBox::Cancel, QStringLiteral("取消"));

    ret = static_cast<QMessageBox::StandardButton>(box.exec());

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
    if (maybeSave())
    {
        m_textEdit->clear();
        setCurrentFileName(QString());
    }
}

void TextEdit::fileOpen()
{
    QString fn = QFileDialog::getOpenFileName(this, QStringLiteral("打开文件"),
                                              QString(), tr("HTML-Files (*.htm *.html);;"
                                                            "All Files (*)"));
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
    QString fn = QFileDialog::getSaveFileName(this, QStringLiteral("另存为"), QString(),
                                              tr("HTML-Files (*.htm *.html);;"
                                                 "ODF files (*.odt);;"
                                                 "All Files (*)"));
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
    QString m_fileName = QFileDialog::getSaveFileName(this, QStringLiteral("导出PDF"),
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
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择图片"),
                                  ".", m_supportImgFormat);

    QUrl uri(QString("file:///%1").arg(file));
    QImage image = QImageReader(file).read();

    QTextDocument *textDocument = m_textEdit->document();
    textDocument->addResource(QTextDocument::ImageResource, uri, QVariant(image));
    QTextCursor cursor = m_textEdit->textCursor();
    QTextImageFormat imageFormat;


    imageFormat.setWidth(image.width());
    imageFormat.setHeight(image.height());
    imageFormat.setName(uri.toString());
    cursor.insertImage(imageFormat);
}

// 插入图片
void TextEdit::insertImage(const QString &imgPath)
{
    QUrl uri(QString("file:///%1").arg(imgPath));
    QImage image = QImageReader(imgPath).read();

    QTextDocument *textDocument = m_textEdit->document();
    textDocument->addResource(QTextDocument::ImageResource, uri, QVariant(image));
    QTextCursor cursor = m_textEdit->textCursor();
    QTextImageFormat imageFormat;

    imageFormat.setWidth(image.width());
    imageFormat.setHeight(image.height());
    imageFormat.setName(uri.toString());
    cursor.insertImage(imageFormat);
}

void TextEdit::insertHyperLinkDlg()
{
    if (!m_insertDlg)
    {
        m_insertDlg = new InsertLinkDlg(this);
        connect(m_insertDlg, SIGNAL(hyperLinkCreateSig(QStringList)),
                this, SLOT(insertHyperLink(QStringList)));
    }
    else
    {
        m_insertDlg->initDlg();
    }

    m_insertDlg->show();
}

// 插入超链接
void TextEdit::insertHyperLink(QStringList linkList)
{
    m_textEdit->insertHyperLink(linkList);
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
    QMessageBox::about(this, QStringLiteral("关于"), QStringLiteral("文本编辑器\n支持富文本编辑器\n 版权：Copyright @HX 20220908"));
}

void TextEdit::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = m_textEdit->textCursor();
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


InsertLinkDlg::InsertLinkDlg(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(QStringLiteral("超链接设置"));
    QHBoxLayout *hlayout = new QHBoxLayout();

    m_hyperLink = new QPushButton(QIcon(rsrcPath + "/web.png"), "", this);
    m_hyperLink->setToolTip(QStringLiteral("设置网络地址"));

    m_fileLink = new QPushButton(QIcon(rsrcPath + "/openfile.png"), "", this);
    m_fileLink->setToolTip(QStringLiteral("设置文件地址"));
    QSpacerItem *spacerItem1 = new QSpacerItem(40, 16, QSizePolicy::Expanding);

    connect(m_hyperLink, SIGNAL(clicked(bool)), this, SLOT(onHyperlinkClicked()));
    connect(m_fileLink, SIGNAL(clicked(bool)), this, SLOT(onFilelinkClicked()));

    hlayout->addItem(spacerItem1);
    hlayout->addWidget(m_fileLink);
    hlayout->addWidget(m_hyperLink);

    m_showLabel = new QLabel(QStringLiteral("显示文字："), this);
    m_showEdit = new QLineEdit(this);
    QHBoxLayout *hlayout1 = new QHBoxLayout();
    hlayout1->addWidget(m_showLabel);
    hlayout1->addWidget(m_showEdit);

    m_linkLabel = new QLabel(QStringLiteral("链接地址："), this);
    m_linkEdit = new QLineEdit(this);
    m_btn_openLink = new QPushButton(QIcon(rsrcPath + "/fileopen.png"), "", this);
    m_btn_openLink->setToolTip(QStringLiteral("浏览文件"));

    connect(m_btn_openLink, SIGNAL(clicked(bool)),
            this, SLOT(onOpenFile()));

    QHBoxLayout *hlayout2 = new QHBoxLayout();
    hlayout2->addWidget(m_linkLabel);
    hlayout2->addWidget(m_linkEdit);
    hlayout2->addWidget(m_btn_openLink);

    QVBoxLayout *vlayout1 = new QVBoxLayout();
    vlayout1->addLayout(hlayout1);
    vlayout1->addLayout(hlayout2);

    QGroupBox *groupBox = new QGroupBox(QStringLiteral("超链接"), this);
    groupBox->setLayout(vlayout1);

    m_btn_confirm = new QPushButton(QStringLiteral("确定"));
    connect(m_btn_confirm, SIGNAL(clicked(bool)), this, SLOT(onBtnConfirmClicked()));

    m_btn_cancel = new QPushButton(QStringLiteral("取消"));
    connect(m_btn_cancel, SIGNAL(clicked(bool)), this, SLOT(onBtnCancelClicked()));

    QSpacerItem *spacerItem2 = new QSpacerItem(50, 20, QSizePolicy::Expanding);

    QHBoxLayout *hlayout3 = new QHBoxLayout();

    hlayout3->addItem(spacerItem2);
    hlayout3->addWidget(m_btn_confirm);
    hlayout3->addWidget(m_btn_cancel);

    QVBoxLayout *vlayout2 = new QVBoxLayout();
    vlayout2->addLayout(hlayout);
    vlayout2->addWidget(groupBox);
    vlayout2->addLayout(hlayout3);

    this->setLayout(vlayout2);

    setFixedHeight(sizeHint().height());
    setFixedWidth(sizeHint().width());

    // 默认选中文件
    m_hyperLink->setCheckable(true);
    m_fileLink->setCheckable(true);
    m_hyperLink->setAutoExclusive(true);
    m_fileLink->setAutoExclusive(true);

    initDlg();
}

InsertLinkDlg::~InsertLinkDlg()
{

}

void InsertLinkDlg::initDlg()
{
    m_fileLink->setChecked(true);
    m_linkEdit->clear();
    m_linkEdit->setPlaceholderText("file:///D://example.html");
    m_showEdit->clear();
}

void InsertLinkDlg::onHyperlinkClicked()
{
    m_btn_openLink->setVisible(false);
    m_linkEdit->clear();
    m_linkEdit->setPlaceholderText("http[s]/ftp://xxxx/");

    m_showEdit->clear();
}

void InsertLinkDlg::onFilelinkClicked()
{
    m_btn_openLink->setVisible(true);
    m_linkEdit->clear();
    m_linkEdit->setPlaceholderText("D://example.html");

    m_showEdit->clear();
}

void InsertLinkDlg::onBtnConfirmClicked()
{
    QString showName = m_showEdit->text();
    QString hyperText = m_linkEdit->text();

    if (hyperText.isEmpty())
    {

        return;
    }

    if (showName.isEmpty())
    {
        showName = hyperText;
    }

    QStringList hlist;
    hlist << showName << hyperText;
    emit hyperLinkCreateSig(hlist);
    this->close();
}

void InsertLinkDlg::onBtnCancelClicked()
{
    this->close();
}

void InsertLinkDlg::onOpenFile()
{
    m_showEdit->clear();
    m_linkEdit->clear();

    QUrl fileUrl = QFileDialog::getOpenFileUrl(this,
                                QStringLiteral("选择文件"),
                                QUrl("."),
                                "*.*");

    m_linkEdit->setText(fileUrl.toString());
    m_showEdit->setText(fileUrl.fileName());
}
