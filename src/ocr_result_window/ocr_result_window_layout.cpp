#include "ocr_result_window/ocr_result_window.h"

#include "app_config_store.h"
#include "ocr_result_window/ocr_result_window_style.h"
#include "ocr_result_window/ocr_source_preview.h"
#include "ocr_result_window/ocr_text_pane.h"
#include "settings/settings_design_tokens.h"
#include "ui/i18n.h"
#include "ui/interface_theme_config.h"
#include "ui/theme.h"

#include <QBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QSizeGrip>
#include <QSplitter>
#include <QStyleOption>
#include <QTextEdit>
#include <QTimer>

namespace markshot::shot {

void OcrResultWindow::initializeUi(const QString &text, QImage sourceImage)
{
    setFont(markshot::theme::uiFont(10));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 8);
    layout->setSpacing(8);

    // 1. 【OCR】【窗口标题】标题栏只保留窗口名称、置顶和关闭操作
    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName(QStringLiteral("ocrTitleBar"));
    m_titleBar->setCursor(Qt::SizeAllCursor);
    auto *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(8);
    m_titleIcon = new QLabel(m_titleBar);
    m_titleIcon->setFixedSize(22, 22);
    m_titleIcon->setAttribute(Qt::WA_TransparentForMouseEvents);
    titleLayout->addWidget(m_titleIcon);
    auto *title = new QLabel(MS_TR("Text Recognition"), m_titleBar);
    title->setFont(markshot::theme::uiFont(11, QFont::DemiBold));
    title->setAttribute(Qt::WA_TransparentForMouseEvents);
    titleLayout->addWidget(title, 1);
    m_pinButton = new QPushButton(m_titleBar);
    m_pinButton->setObjectName(QStringLiteral("ocrPinButton"));
    m_pinButton->setCheckable(true);
    m_pinButton->setChecked(m_alwaysOnTop);
    m_pinButton->setAccessibleName(MS_TR("Always on Top"));
    m_pinButton->setToolTip(m_alwaysOnTop ? MS_TR("Always on Top: On") : MS_TR("Always on Top: Off"));
    m_closeButton = new QPushButton(m_titleBar);
    m_closeButton->setObjectName(QStringLiteral("ocrCloseButton"));
    m_closeButton->setAccessibleName(MS_TR("Close"));
    m_closeButton->setToolTip(MS_TR("Close") + QStringLiteral(" (Esc)"));
    for (QPushButton *button : {m_pinButton, m_closeButton}) {
        button->setProperty("role", QStringLiteral("icon"));
        button->setFixedSize(30, 30);
        button->setIconSize(QSize(18, 18));
        button->setCursor(Qt::ArrowCursor);
        titleLayout->addWidget(button);
    }
    connect(m_pinButton, &QPushButton::toggled, this, &OcrResultWindow::setAlwaysOnTop);
    connect(m_closeButton, &QPushButton::clicked, this, &QWidget::close);
    m_titleBar->installEventFilter(this);
    layout->addWidget(m_titleBar);

    // 2. 【OCR】【紧凑布局】高度不足时只滚动内容，标题和翻译操作保持可见
    auto *contentScroll = new QScrollArea(this);
    contentScroll->setObjectName(QStringLiteral("ocrContentScroll"));
    contentScroll->setFrameShape(QFrame::NoFrame);
    contentScroll->setWidgetResizable(true);
    contentScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *content = new QWidget(contentScroll);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(8);
    contentLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    contentLayout->addWidget(new OcrSourcePreview(std::move(sourceImage), content));
    contentScroll->setWidget(content);
    content->setAutoFillBackground(false);
    contentScroll->viewport()->setAutoFillBackground(false);
    layout->addWidget(contentScroll, 1);

    // 3. 【OCR】【对照编辑】原文和译文各自保留编辑、复制及统计，按需要展开译文
    m_splitter = new QSplitter(Qt::Horizontal, content);
    m_splitter->setObjectName(QStringLiteral("ocrTextSplitter"));
    m_splitter->setHandleWidth(8);
    m_splitter->setChildrenCollapsible(false);
    m_sourcePane = new OcrTextPane(MS_TR("Recognized text"), MS_TR("OCR text appears here"), m_splitter);
    m_sourcePane->setObjectName(QStringLiteral("ocrSourcePane"));
    m_sourcePane->editor()->setObjectName(QStringLiteral("ocrEditor"));
    m_sourcePane->setText(text);
    m_translationPane = new OcrTextPane(MS_TR("Translated text"), MS_TR("Translation appears here"), m_splitter);
    m_translationPane->setObjectName(QStringLiteral("ocrTranslationPane"));
    m_translationPane->editor()->setObjectName(QStringLiteral("ocrTranslationEditor"));
    m_splitter->addWidget(m_sourcePane);
    m_splitter->addWidget(m_translationPane);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);
    m_translationPane->hide();
    contentLayout->addWidget(m_splitter, 1);
    connect(m_sourcePane, &OcrTextPane::copyRequested, this,
            [this](const QString &value) { copyResultText(value, false); });
    connect(m_translationPane, &OcrTextPane::copyRequested, this,
            [this](const QString &value) { copyResultText(value, true); });
    connect(m_sourcePane->editor(), &QTextEdit::textChanged, this, &OcrResultWindow::updateSourceState);

    // 4. 【OCR】【翻译操作】目标语言和翻译按钮相邻，保留完整的语言名称
    auto *actions = new QHBoxLayout;
    actions->setSpacing(8);
    m_languageLabel = new QLabel(MS_TR("Target Language"), this);
    m_languageLabel->setProperty("role", QStringLiteral("muted"));
    m_languageLabel->setFont(markshot::theme::uiFont(9));
    actions->addWidget(m_languageLabel);
    m_targetLanguageCombo = new QComboBox(this);
    setupTargetLanguageCombo();
    m_languageLabel->setBuddy(m_targetLanguageCombo);
    actions->addWidget(m_targetLanguageCombo, 1);
    m_translateButton = new QPushButton(MS_TR("Translate"), this);
    m_translateButton->setObjectName(QStringLiteral("ocrTranslateButton"));
    m_translateButton->setProperty("role", QStringLiteral("primary"));
    m_translateButton->setFont(markshot::theme::uiFont(10, QFont::DemiBold));
    m_translateButton->setMinimumWidth(92);
    m_translateButton->setToolTip(MS_TR("Translate") + QStringLiteral(" (Ctrl+Enter)"));
    connect(m_translateButton, &QPushButton::clicked, this, [this] {
        if (m_translationTask) {
            cancelTranslation();
            m_translationPane->setNotice(MS_TR("Translation canceled"));
        } else {
            startTranslation();
        }
    });
    actions->addWidget(m_translateButton);
    layout->addLayout(actions);

    // 5. 【OCR】【操作反馈】使用固定状态栏承载提示，避免覆盖文本或重复堆叠提示框
    auto *footer = new QHBoxLayout;
    footer->setContentsMargins(0, 0, 0, 0);
    m_statusLabel = new QLabel(MS_TR("Ctrl+Enter to translate · Ctrl+Shift+C to copy all"), this);
    m_statusLabel->setObjectName(QStringLiteral("ocrStatus"));
    m_statusLabel->setProperty("role", QStringLiteral("muted"));
    m_statusLabel->setFont(markshot::theme::uiFont(9));
    m_statusLabel->setTextFormat(Qt::PlainText);
    m_statusLabel->setWordWrap(true);
    footer->addWidget(m_statusLabel, 1);
    auto *grip = new QSizeGrip(this);
    grip->setObjectName(QStringLiteral("ocrResizeGrip"));
    grip->setFixedSize(18, 18);
    grip->setToolTip(MS_TR("Resize window"));
    footer->addWidget(grip, 0, Qt::AlignBottom);
    layout->addLayout(footer);
    m_statusTimer = new QTimer(this);
    m_statusTimer->setSingleShot(true);
    connect(m_statusTimer, &QTimer::timeout, this, [this] {
        m_statusLabel->setText(MS_TR("Ctrl+Enter to translate · Ctrl+Shift+C to copy all"));
        m_statusLabel->setToolTip(QString());
    });

    // 6. 【OCR】【键盘操作】保留编辑器原有复制语义，增加完整文本复制和翻译快捷键
    auto *closeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(closeShortcut, &QShortcut::activated, this, &QWidget::close);
    auto *translateShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this);
    translateShortcut->setKeys({QKeySequence(Qt::CTRL | Qt::Key_Return), QKeySequence(Qt::CTRL | Qt::Key_Enter)});
    connect(translateShortcut, &QShortcut::activated, this, &OcrResultWindow::startTranslation);
    auto *copyShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C), this);
    connect(copyShortcut, &QShortcut::activated, this, [this] {
        const bool translated = m_translationPane->editor()->hasFocus();
        copyResultText(translated ? m_translationPane->text() : m_sourcePane->text(), translated);
    });
    updateSourceState();
}

void OcrResultWindow::applyTheme()
{
    const auto mode = markshot::ui::effectiveUiThemeMode(
        markshot::ui::uiThemeModeFromConfigRoot(markshot::readAppConfigRoot()));
    QPalette colors = markshot::settings::tokens::settingsPalette(mode);
    const QColor muted = mode == markshot::ui::UiThemeMode::Light
        ? QColor(100, 116, 139) : markshot::settings::tokens::kTextSecondary;
    colors.setColor(QPalette::PlaceholderText, muted);
    colors.setColor(QPalette::Disabled, QPalette::Text, muted);
    colors.setColor(QPalette::Disabled, QPalette::ButtonText, muted);
    setPalette(colors);
    setStyleSheet(ocrWindowStyleSheet(colors));
    const QColor ink = colors.color(QPalette::ButtonText);
    const QColor accent = colors.color(QPalette::Highlight);
    m_titleIcon->setPixmap(ocrActionIcon(ShotWindow::Action::OcrCopy, accent).pixmap(QSize(22, 22), devicePixelRatioF()));
    m_pinButton->setIcon(ocrActionIcon(ShotWindow::Action::Pin, ink, accent));
    m_closeButton->setIcon(ocrActionIcon(ShotWindow::Action::Cancel, ink));
    m_sourcePane->setPalette(colors);
    m_translationPane->setPalette(colors);
    m_sourcePane->refreshTheme();
    m_translationPane->refreshTheme();
}

void OcrResultWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    // 1. 【OCR】【窗口绘制】显式绘制自定义窗口的样式背景，避免透明区域透出合成器底色
    QStyleOption option;
    option.initFrom(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);
}

void OcrResultWindow::updateResponsiveLayout()
{
    if (!m_splitter) {
        return;
    }
    const Qt::Orientation orientation = width() >= 620 ? Qt::Horizontal : Qt::Vertical;
    if (m_splitter->orientation() != orientation) {
        m_splitter->setOrientation(orientation);
        m_splitter->setSizes({1, 1});
    }
    m_languageLabel->setVisible(width() >= 480);
}

void OcrResultWindow::showTranslationPane()
{
    if (m_translationPane->isHidden()) {
        m_translationPane->show();
        updateResponsiveLayout();
        m_splitter->setSizes({1, 1});
    }
}

}
