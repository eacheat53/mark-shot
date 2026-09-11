#include "ocr_result_window/ocr_text_pane.h"

#include "ocr_result_window/ocr_result_window_style.h"
#include "ui/i18n.h"
#include "ui/theme.h"

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QClipboard>
#include <QLabel>
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>
#include <QTextBoundaryFinder>
#include <QTextDocument>
#include <QTextEdit>

namespace markshot::shot {

OcrTextPane::OcrTextPane(const QString &title, const QString &placeholder, QWidget *parent)
    : QFrame(parent)
{
    setProperty("ocrPane", true);
    setMinimumSize(180, 112);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(6);

    // 1. 【OCR】【文本分区】将复制入口与所属文本放在同一分区
    auto *header = new QHBoxLayout;
    header->setSpacing(6);
    auto *label = new QLabel(title, this);
    label->setProperty("role", QStringLiteral("sectionTitle"));
    label->setFont(markshot::theme::uiFont(10, QFont::DemiBold));
    header->addWidget(label, 1);
    m_copyButton = new QPushButton(MS_TR("Copy"), this);
    m_copyButton->setObjectName(QStringLiteral("ocrCopyButton"));
    m_copyButton->setProperty("role", QStringLiteral("quiet"));
    m_copyButton->setFont(markshot::theme::uiFont(10));
    m_copyButton->setIconSize(QSize(16, 16));
    m_copyButton->setToolTip(MS_TR("Copy all text"));
    m_copyButton->setAccessibleName(MS_TR("Copy %1").arg(title));
    header->addWidget(m_copyButton);
    layout->addLayout(header);

    // 2. 【OCR】【文本编辑】保持纯文本和原有换行，长行在窗口内自动折行
    m_editor = new QTextEdit(this);
    m_editor->setAcceptRichText(false);
    m_editor->setAccessibleName(title);
    m_editor->setPlaceholderText(placeholder);
    m_editor->setFont(markshot::theme::uiFont(11));
    m_editor->setTabStopDistance(m_editor->fontMetrics().horizontalAdvance(QLatin1Char(' ')) * 4);
    m_editor->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_editor->setMinimumSize(0, 28);
    m_editor->document()->setDocumentMargin(4);
    m_editor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_editor, &QTextEdit::customContextMenuRequested, this, &OcrTextPane::showEditorMenu);
    connect(m_editor, &QTextEdit::textChanged, this, &OcrTextPane::updateTextState);
    layout->addWidget(m_editor, 1);

    m_progress = new QProgressBar(this);
    m_progress->setObjectName(QStringLiteral("ocrTranslationProgress"));
    m_progress->setRange(0, 0);
    m_progress->setTextVisible(false);
    m_progress->setFixedHeight(3);
    m_progress->hide();
    layout->addWidget(m_progress);

    m_notice = new QLabel(this);
    m_notice->setObjectName(QStringLiteral("ocrPaneNotice"));
    m_notice->setTextFormat(Qt::PlainText);
    m_notice->setWordWrap(true);
    m_notice->setFont(markshot::theme::uiFont(9));
    m_notice->setMaximumHeight(m_notice->fontMetrics().lineSpacing() * 3);
    m_notice->hide();
    layout->addWidget(m_notice);

    m_statistics = new QLabel(this);
    m_statistics->setObjectName(QStringLiteral("ocrTextStatistics"));
    m_statistics->setProperty("role", QStringLiteral("muted"));
    m_statistics->setFont(markshot::theme::uiFont(9));
    layout->addWidget(m_statistics);
    connect(m_copyButton, &QPushButton::clicked, this, [this] { emit copyRequested(text()); });
    updateTextState();
}

QTextEdit *OcrTextPane::editor() const
{
    return m_editor;
}

QString OcrTextPane::text() const
{
    return m_editor->toPlainText();
}

void OcrTextPane::setText(const QString &text)
{
    m_editor->setPlainText(text);
}

void OcrTextPane::setNotice(const QString &text, bool error)
{
    m_notice->setText(text);
    m_notice->setToolTip(text);
    m_notice->setProperty("error", error);
    m_notice->style()->unpolish(m_notice);
    m_notice->style()->polish(m_notice);
    m_notice->setVisible(!text.isEmpty());
}

void OcrTextPane::setBusy(bool busy)
{
    m_progress->setVisible(busy);
    m_editor->setReadOnly(busy);
}

void OcrTextPane::refreshTheme()
{
    m_copyButton->setIcon(ocrActionIcon(types::Action::Copy, palette().color(QPalette::ButtonText)));
}

void OcrTextPane::updateTextState()
{
    // 1. 【OCR】【文本统计】按字素统计，避免把代理对或组合字符拆成多个字符
    const QString current = text();
    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, current);
    int characters = 0;
    while (finder.toNextBoundary() > 0) {
        ++characters;
    }
    const int lines = current.isEmpty() ? 0 : current.count(QLatin1Char('\n')) + 1;
    m_statistics->setText(MS_TR("%1 characters · %2 lines").arg(characters).arg(lines));
    m_copyButton->setEnabled(!current.trimmed().isEmpty());
}

void OcrTextPane::showEditorMenu(const QPoint &position)
{
    QMenu menu(this);
    menu.setStyleSheet(ocrMenuStyleSheet(palette()));
    const bool editable = !m_editor->isReadOnly();
    const bool selected = m_editor->textCursor().hasSelection();

    // 1. 【OCR】【文本菜单】复用编辑器操作和应用翻译，显式呈现可用状态
    const auto add = [this, &menu](const QString &label, const QKeySequence &shortcut,
                                  bool enabled, auto callback) {
        QAction *action = menu.addAction(label, this, callback);
        action->setShortcut(shortcut);
        action->setShortcutVisibleInContextMenu(true);
        action->setEnabled(enabled);
    };
    add(MS_TR("Undo"), QKeySequence::Undo, editable && m_editor->document()->isUndoAvailable(),
        [this] { m_editor->undo(); });
    add(MS_TR("Redo"), QKeySequence::Redo, editable && m_editor->document()->isRedoAvailable(),
        [this] { m_editor->redo(); });
    menu.addSeparator();
    add(MS_TR("Cut"), QKeySequence::Cut, editable && selected, [this] { m_editor->cut(); });
    add(MS_TR("Copy"), QKeySequence::Copy, selected, [this] { m_editor->copy(); });
    add(MS_TR("Paste"), QKeySequence::Paste,
        editable && !QApplication::clipboard()->text().isEmpty(), [this] { m_editor->paste(); });
    add(MS_TR("Delete"), QKeySequence(Qt::Key_Delete), editable && selected, [this] {
        QTextCursor cursor = m_editor->textCursor();
        cursor.removeSelectedText();
    });
    menu.addSeparator();
    add(MS_TR("Select All"), QKeySequence::SelectAll, !m_editor->document()->isEmpty(),
        [this] { m_editor->selectAll(); });
    menu.exec(m_editor->mapToGlobal(position));
}

}
