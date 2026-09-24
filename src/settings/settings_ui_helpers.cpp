#include "settings/settings_ui_helpers.h"
#include "settings/settings_form_layout.h"

#include "ui/i18n.h"
#include "ui/disclosure_section.h"
#include "ui/theme.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QKeySequence>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QTextCursor>
#include <QVBoxLayout>

namespace markshot::settings {

namespace {

void showLineEditContextMenu(QLineEdit *edit, const QPoint &pos)
{
    QMenu menu(edit);
    const bool hasSelection = edit->hasSelectedText();
    const bool hasText = !edit->text().isEmpty();
    const bool hasClipboard = !QApplication::clipboard()->text().isEmpty();

    auto addAction = [&menu, edit](const QString &label, const QKeySequence &shortcut, bool enabled, auto callback) {
        QAction *action = menu.addAction(label);
        action->setShortcut(shortcut);
        action->setShortcutVisibleInContextMenu(true);
        action->setEnabled(enabled);
        QObject::connect(action, &QAction::triggered, edit, callback);
    };

    addAction(MS_TR("Undo"), QKeySequence::Undo, edit->isUndoAvailable(), [edit] { edit->undo(); });
    addAction(MS_TR("Redo"), QKeySequence::Redo, edit->isRedoAvailable(), [edit] { edit->redo(); });
    menu.addSeparator();
    addAction(MS_TR("Cut"), QKeySequence::Cut, hasSelection, [edit] { edit->cut(); });
    addAction(MS_TR("Copy"), QKeySequence::Copy, hasSelection, [edit] { edit->copy(); });
    addAction(MS_TR("Paste"), QKeySequence::Paste, hasClipboard, [edit] { edit->paste(); });
    addAction(MS_TR("Delete"), QKeySequence(Qt::Key_Delete), hasSelection, [edit] { edit->del(); });
    menu.addSeparator();
    addAction(MS_TR("Select All"), QKeySequence::SelectAll, hasText, [edit] { edit->selectAll(); });

    menu.exec(edit->mapToGlobal(pos));
}

void showPlainTextEditContextMenu(QPlainTextEdit *edit, const QPoint &pos)
{
    QMenu menu(edit);
    const QTextCursor cursor = edit->textCursor();
    const bool hasSelection = cursor.hasSelection();
    const bool hasDocumentText = !edit->document()->isEmpty();
    const bool hasClipboard = !QApplication::clipboard()->text().isEmpty();

    auto addAction = [&menu, edit](const QString &label, const QKeySequence &shortcut, bool enabled, auto callback) {
        QAction *action = menu.addAction(label);
        action->setShortcut(shortcut);
        action->setShortcutVisibleInContextMenu(true);
        action->setEnabled(enabled);
        QObject::connect(action, &QAction::triggered, edit, callback);
    };

    addAction(MS_TR("Undo"), QKeySequence::Undo, edit->document()->isUndoAvailable(), [edit] { edit->undo(); });
    addAction(MS_TR("Redo"), QKeySequence::Redo, edit->document()->isRedoAvailable(), [edit] { edit->redo(); });
    menu.addSeparator();
    addAction(MS_TR("Cut"), QKeySequence::Cut, hasSelection, [edit] { edit->cut(); });
    addAction(MS_TR("Copy"), QKeySequence::Copy, hasSelection, [edit] { edit->copy(); });
    addAction(MS_TR("Paste"), QKeySequence::Paste, hasClipboard, [edit] { edit->paste(); });
    addAction(MS_TR("Delete"), QKeySequence(Qt::Key_Delete), hasSelection, [edit] {
        QTextCursor sel = edit->textCursor();
        sel.removeSelectedText();
        edit->setTextCursor(sel);
    });
    menu.addSeparator();
    addAction(MS_TR("Select All"), QKeySequence::SelectAll, hasDocumentText, [edit] { edit->selectAll(); });

    menu.exec(edit->viewport()->mapToGlobal(pos));
}

} // namespace

QVBoxLayout *createSettingsPageLayout(QWidget *parent)
{
    auto *layout = new QVBoxLayout(parent);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(18);
    return layout;
}

QFrame *createSettingsCard(const QString &title, const QString &description, QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("settingsCard"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto *titleLabel = new QLabel(title, card);
    titleLabel->setObjectName(QStringLiteral("settingsCardTitle"));
    titleLabel->setFont(markshot::theme::uiFont(11, QFont::DemiBold));
    titleLabel->setToolTip(description);
    titleLabel->setAccessibleDescription(description);
    layout->addWidget(titleLabel);

    createSettingsFormLayout(layout);
    return card;
}

QFrame *createAdvancedSettingsCard(const QString &title, const QString &description, QWidget *parent)
{
    auto *section = new markshot::ui::DisclosureSection(title, parent);
    section->setToolTip(description);
    auto *layout = new QVBoxLayout(section->content());
    layout->setContentsMargins(0, 4, 0, 4);
    layout->setSpacing(10);
    if (!description.isEmpty()) {
        auto *help = new QLabel(description, section->content());
        help->setObjectName(QStringLiteral("settingsCardDescription"));
        help->setWordWrap(true);
        layout->addWidget(help);
    }
    createSettingsFormLayout(layout);
    return section;
}

QFormLayout *settingsCardForm(QFrame *card)
{
    return card ? card->findChild<QFormLayout *>(QStringLiteral("settingsCardForm")) : nullptr;
}

QCheckBox *addSwitchRow(QFormLayout *form, const QString &label, const QString &description)
{
    auto *box = new QCheckBox(label);
    box->setProperty("settingsSwitchLabel", label);
    box->setAccessibleName(label);
    box->setAccessibleDescription(description);
    box->setToolTip(description);
    box->setMinimumHeight(28);
    box->setCursor(Qt::PointingHandCursor);
    form->addRow(box);
    return box;
}

QLineEdit *addTextRow(QFormLayout *form, const QString &label, const QString &placeholder)
{
    auto *edit = new QLineEdit;
    edit->setAccessibleName(label);
    edit->setPlaceholderText(placeholder);
    edit->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(edit, &QLineEdit::customContextMenuRequested, edit, [edit](const QPoint &pos) {
        showLineEditContextMenu(edit, pos);
    });
    form->addRow(label, edit);
    return edit;
}

QPlainTextEdit *addPlainTextRow(QFormLayout *form, const QString &label, const QString &placeholder)
{
    auto *edit = new QPlainTextEdit;
    edit->setAccessibleName(label);
    edit->setPlaceholderText(placeholder);
    edit->setMinimumHeight(74);
    edit->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(edit, &QPlainTextEdit::customContextMenuRequested, edit, [edit](const QPoint &pos) {
        showPlainTextEditContextMenu(edit, pos);
    });
    form->addRow(label, edit);
    return edit;
}

QSpinBox *addSpinRow(QFormLayout *form, const QString &label, int minimum, int maximum, const QString &suffix)
{
    auto *spin = new QSpinBox;
    spin->setAccessibleName(label);
    spin->setRange(minimum, maximum);
    spin->setSuffix(suffix);
    spin->setMaximumWidth(144);
    spin->setContextMenuPolicy(Qt::NoContextMenu);
    form->addRow(label, spin);
    return spin;
}

QDoubleSpinBox *addDoubleRow(QFormLayout *form, const QString &label, double minimum, double maximum, int decimals)
{
    auto *spin = new QDoubleSpinBox;
    spin->setAccessibleName(label);
    spin->setRange(minimum, maximum);
    spin->setDecimals(decimals);
    spin->setMaximumWidth(144);
    spin->setContextMenuPolicy(Qt::NoContextMenu);
    form->addRow(label, spin);
    return spin;
}

QComboBox *addComboRow(QFormLayout *form, const QString &label)
{
    auto *combo = new QComboBox;
    combo->setAccessibleName(label);
    combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    combo->setMinimumContentsLength(8);
    combo->setMaximumWidth(360);
    combo->setCursor(Qt::PointingHandCursor);
    combo->setContextMenuPolicy(Qt::NoContextMenu);
    form->addRow(label, combo);
    return combo;
}

QKeySequenceEdit *addShortcutRow(QFormLayout *form, const QString &label)
{
    auto *edit = new QKeySequenceEdit;
    edit->setAccessibleName(label);
    edit->setMaximumWidth(280);
    edit->setContextMenuPolicy(Qt::NoContextMenu);
    if (auto *le = edit->findChild<QLineEdit *>()) {
        le->setContextMenuPolicy(Qt::NoContextMenu);
    }
    form->addRow(label, edit);
    return edit;
}

QString colorButtonStyleSheet(const QColor &color)
{
    const QString name = color.isValid()
        ? color.name(QColor::HexRgb).toUpper()
        : markshot::theme::kDefaultAnnotationColor.name(QColor::HexRgb).toUpper();
    return QStringLiteral(
               "QPushButton {"
               " min-height: 30px;"
               " padding: 2px 8px;"
               " border-radius: 8px;"
               " border: 1px solid #334155;"
               " background: %1;"
               " color: #0F172A;"
               " font-weight: 700;"
               "}"
               "QPushButton:hover { border-color: #5EEAD4; }")
        .arg(name);
}

}  // namespace markshot::settings
