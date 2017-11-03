// For license of this file, see <project-root-folder>/LICENSE.md.

#include "external-tools/externaltools.h"

#include "external-tools/externaltool.h"
#include "external-tools/predefinedtools.h"
#include "gui/toolbox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textapplication.h"
#include "network-web/networkfactory.h"

#include <functional>

#include <QAction>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QPointer>
#include <QRegularExpression>

ExternalTools::ExternalTools(QObject* parent) : QObject(parent), m_tools(QList<ExternalTool*>()) {}

ExternalTools::~ExternalTools() {
  qDeleteAll(m_tools);
  qDebug("Destroying ExternalTools.");
}

QList<QAction*> ExternalTools::generateActions(QWidget* parent, TextApplication* app) const {
  QList<QAction*> actions;
  QMap<QString, QMenu*> categories;

  foreach (ExternalTool* tool, m_tools) {
    QAction* act = new QAction(tool->name());

    if (!tool->category().isEmpty()) {
      if (!categories.contains(tool->category())) {
        QMenu* category_menu = new QMenu(parent);

        category_menu->setTitle(tool->category());

        actions.append(category_menu->menuAction());
        categories.insert(tool->category(), category_menu);
      }

      categories[tool->category()]->addAction(act);
    }
    else {
      actions.append(act);
    }

    act->setData(QVariant::fromValue(tool));
    act->setShortcut(QKeySequence::fromString(tool->shortcut(), QKeySequence::SequenceFormat::PortableText));
    act->setShortcutContext(Qt::ApplicationShortcut);

    connect(act, &QAction::triggered, app, &TextApplication::runSelectedExternalTool);
    connect(tool, &ExternalTool::toolFinished, this, &ExternalTools::onToolFinished);
  }

  return actions;
}

const QList<ExternalTool*> ExternalTools::tools() const {
  return m_tools;
}

void ExternalTools::loadPredefinedTools() {
  PredefinedTool* insert_date_time = new PredefinedTool(&PredefinedTools::currentDateTime, this);

  insert_date_time->setCategory(tr("Insert..."));
  insert_date_time->setName(tr("Date/time"));
  insert_date_time->setInput(ToolInput::NoInput);
  insert_date_time->setOutput(ToolOutput::InsertAtCursorPosition);

  m_tools.append(insert_date_time);

  PredefinedTool* send_to_clbin = new PredefinedTool(&PredefinedTools::sendToClbin, this);

  send_to_clbin->setCategory(tr("Upload to..."));
  send_to_clbin->setName(tr("clbin.com"));
  send_to_clbin->setInput(ToolInput::SelectionDocument);
  send_to_clbin->setOutput(ToolOutput::DumpToOutputWindow);

  m_tools.append(send_to_clbin);

  PredefinedTool* send_to_ixio = new PredefinedTool(&PredefinedTools::sendToIxio, this);

  send_to_ixio->setCategory(tr("Upload to..."));
  send_to_ixio->setName(tr("ix.io"));
  send_to_ixio->setInput(ToolInput::SelectionDocument);
  send_to_ixio->setOutput(ToolOutput::DumpToOutputWindow);

  m_tools.append(send_to_ixio);

  PredefinedTool* send_to_sprunge = new PredefinedTool(&PredefinedTools::sendToSprunge, this);

  send_to_sprunge->setCategory(tr("Upload to..."));
  send_to_sprunge->setName(tr("sprunge.us"));
  send_to_sprunge->setInput(ToolInput::SelectionDocument);
  send_to_sprunge->setOutput(ToolOutput::DumpToOutputWindow);

  m_tools.append(send_to_sprunge);

  PredefinedTool* send_to_github = new PredefinedTool(&PredefinedTools::sendToGithub, this);

  send_to_github->setCategory(tr("Upload to..."));
  send_to_github->setName(tr("github.com"));
  send_to_github->setInput(ToolInput::SelectionDocument);
  send_to_github->setOutput(ToolOutput::DumpToOutputWindow);

  m_tools.append(send_to_github);

  PredefinedTool* tobase64 = new PredefinedTool(&PredefinedTools::toBase64, this);

  tobase64->setCategory(tr("MIME tools"));
  tobase64->setName(tr("Text → Base64"));
  tobase64->setInput(ToolInput::SelectionDocument);
  tobase64->setOutput(ToolOutput::ReplaceSelectionDocument);

  m_tools.append(tobase64);

  PredefinedTool* tobase64url = new PredefinedTool(&PredefinedTools::toBase64Url, this);

  tobase64url->setCategory(tr("MIME tools"));
  tobase64url->setName(tr("Text → Base64Url"));
  tobase64url->setInput(ToolInput::SelectionDocument);
  tobase64url->setOutput(ToolOutput::ReplaceSelectionDocument);

  m_tools.append(tobase64url);

  PredefinedTool* frombase64 = new PredefinedTool(&PredefinedTools::fromBase64, this);

  frombase64->setCategory(tr("MIME tools"));
  frombase64->setName(tr("Base64 → text"));
  frombase64->setInput(ToolInput::SelectionDocument);
  frombase64->setOutput(ToolOutput::ReplaceSelectionDocument);

  m_tools.append(frombase64);

  PredefinedTool* frombase64url = new PredefinedTool(&PredefinedTools::fromBase64Url, this);

  frombase64url->setCategory(tr("MIME tools"));
  frombase64url->setName(tr("Base64Url → text"));
  frombase64url->setInput(ToolInput::SelectionDocument);
  frombase64url->setOutput(ToolOutput::ReplaceSelectionDocument);

  m_tools.append(frombase64url);
}

void ExternalTools::loadCustomTools() {}

void ExternalTools::reloadTools() {
  qDeleteAll(m_tools);
  m_tools.clear();

  loadPredefinedTools();
  loadCustomTools();

  emit externalToolsChanged();
}

void ExternalTools::runTool(ExternalTool* tool_to_run, TextEditor* editor) {
  QPointer<TextEditor> ptr_editor = editor;
  QString data;

  switch (tool_to_run->input()) {
    case ToolInput::SelectionDocument:
      data = ptr_editor->hasSelectedText() ? ptr_editor->selectedText() : ptr_editor->text();
      break;

    case ToolInput::CurrentLine:
      int line;
      int index;

      ptr_editor->getCursorPosition(&line, &index);
      data = ptr_editor->text(line);
      break;

    case ToolInput::SavedFile:
      bool ok;

      ptr_editor->save(&ok);
      data = ptr_editor->filePath();
      break;

    case ToolInput::NoInput:
    default:
      break;
  }

  tool_to_run->runTool(ptr_editor, data);
}

void ExternalTools::onToolFinished(QPointer<TextEditor> editor, const QString& output_text) {
  ExternalTool* tool = qobject_cast<ExternalTool*>(sender());
  emit toolFinished(tool, editor, output_text);
}
