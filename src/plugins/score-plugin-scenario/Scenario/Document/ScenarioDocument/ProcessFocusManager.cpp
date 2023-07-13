// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "ProcessFocusManager.hpp"

#include <Process/LayerPresenter.hpp>
#include <Process/Process.hpp>

#include <Scenario/Document/ScenarioDocument/ScenarioDocumentPresenter.hpp>

#include <core/document/Document.hpp>
#include <core/document/DocumentPresenter.hpp>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Process::ProcessFocusManager)
namespace Process
{
ProcessFocusManager* ProcessFocusManager::get(const score::DocumentContext& ctx)
{
  if(auto pres = ctx.document.presenter())
  {
    auto bem
        = qobject_cast<Scenario::ScenarioDocumentPresenter*>(pres->presenterDelegate());
    if(bem)
    {
      return &bem->focusManager();
    }
  }
  return nullptr;
}

ProcessFocusManager::ProcessFocusManager(score::FocusManager& fmgr)
    : m_mgr{fmgr}
{
}
ProcessFocusManager::~ProcessFocusManager() { }
ProcessModel* ProcessFocusManager::focusedModel() const
{
  return m_currentModel;
}

LayerPresenter* ProcessFocusManager::focusedPresenter() const
{
  return m_currentPresenter;
}

void ProcessFocusManager::focus(QPointer<Process::LayerPresenter> p)
{
  if(p == m_currentPresenter)
    return;

  if(m_currentPresenter)
  {
    defocusPresenter(m_currentPresenter);
  }
  if(m_currentModel)
  {
    sig_defocusedViewModel(m_currentModel);
  }

  m_currentPresenter = p;

  if(m_currentPresenter)
  {
    m_currentModel = const_cast<Process::ProcessModel*>(&m_currentPresenter->model());

    sig_focusedViewModel(m_currentModel);

    m_deathConnection = connect(
        m_currentModel, &IdentifiedObjectAbstract::identified_object_destroying, this,
        [this]() {
      sig_defocusedViewModel(nullptr);
      sig_defocusedPresenter(nullptr);
      focusNothing();
        });
    focusPresenter(m_currentPresenter);
  }
  else
  {
    m_currentModel = nullptr;
  }

  m_mgr.set(m_currentModel);
}

void ProcessFocusManager::focus(Scenario::ScenarioDocumentPresenter*)
{
  focusNothing();
  sig_focusedRoot();
}

void ProcessFocusManager::focusNothing()
{
  if(m_currentModel)
    sig_defocusedViewModel(m_currentModel);
  if(m_currentPresenter)
    defocusPresenter(m_currentPresenter);

  m_currentModel = nullptr;
  m_currentPresenter = nullptr;

  m_mgr.set(nullptr);
}

void ProcessFocusManager::focusPresenter(LayerPresenter* p)
{
  p->setFocus(true);
  sig_focusedPresenter(p);
}

void ProcessFocusManager::defocusPresenter(LayerPresenter* p)
{
  p->setFocus(false);
  // if (p->model().selection.get())
  //   const_cast<ProcessModel&>(p->model()).selection.set(false);
  m_deathConnection = QMetaObject::Connection{};
  sig_defocusedPresenter(p);
}
}
