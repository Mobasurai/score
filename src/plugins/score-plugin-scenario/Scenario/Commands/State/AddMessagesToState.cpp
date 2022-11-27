// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "AddMessagesToState.hpp"

#include <Process/ControlMessage.hpp>
#include <Process/Process.hpp>
#include <Process/State/ProcessStateDataInterface.hpp>

#include <Scenario/Document/State/ItemModel/MessageItemModel.hpp>
#include <Scenario/Document/State/ItemModel/MessageItemModelAlgorithms.hpp>
#include <Scenario/Document/State/StateModel.hpp>

#include <score/model/path/Path.hpp>
#include <score/model/path/PathSerialization.hpp>
#include <score/model/tree/TreeNode.hpp>
#include <score/model/tree/TreeNodeSerialization.hpp>
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/MapSerialization.hpp>

namespace Scenario
{
namespace Command
{

RenameAddressInState::RenameAddressInState(
    const Scenario::StateModel& state, const State::AddressAccessor& old,
    const State::AddressAccessor& replacement)
    : m_state{state}
    , m_oldName{old}
    , m_newName{replacement}
{
}

RenameAddressInState::RenameAddressInState(
    const Scenario::StateModel& state, const State::AddressAccessor& old,
    const State::AddressAccessorHead& name)
    : m_state{state}
    , m_oldName{old}
    , m_newName{old}
{
  if(!m_newName.address.path.isEmpty())
    m_newName.address.path.back() = name.name;
  else
    m_newName.address.device = name.name;

  m_newName.qualifiers = name.qualifiers;
}

void RenameAddressInState::undo(const score::DocumentContext& ctx) const
{
  auto& state = m_state.find(ctx);
  Scenario::renameAddress(state.messages(), m_newName, m_oldName);
}

void RenameAddressInState::redo(const score::DocumentContext& ctx) const
{
  // FIXME! This does not take into account the process messages
  auto& state = m_state.find(ctx);
  Scenario::renameAddress(state.messages(), m_oldName, m_newName);
}

void RenameAddressInState::serializeImpl(DataStreamInput& s) const
{
  s << m_state << m_oldName << m_newName;
}

void RenameAddressInState::deserializeImpl(DataStreamOutput& s)
{
  s >> m_state >> m_oldName >> m_newName;
}

RenameAddressesInState::RenameAddressesInState(
    const Scenario::StateModel& state, const State::Address& find,
    const State::Address& replace)
{
  m_path = state;
  m_oldState = state.messages().rootNode();
  m_newState = state.messages().rootNode();

  Scenario::findAndReplaceAddresses(m_newState, find, replace);
}

ReplaceState::ReplaceState(
    const StateModel& state, const Process::MessageNode& oldnode,
    const Process::MessageNode& newnode, const State::MessageList& messages)
{
  m_path = state;
  m_oldState = oldnode;
  m_newState = newnode;

  // FIXME only difference with the one below is the lack of "newstate", this can be refactored

  // TODO backup all the processes, not just the messages.

  // For all the nodes in m_newState :
  // - if there is a new message in lst, add it as "process" node in the tree
  // - if there is a message both in lst and in the tree, update it in the
  // tree
  // - if there is a message in the tree with the process, but not in lst it
  // means
  // that the process stopped using this address. Hence we remove the
  // "process" part
  // of this node, and if there is no "user" or message from another process,
  // we remove
  // the node altogether

  // TODO this won't work in network editing ???
  for(const ProcessStateWrapper& prevProc : state.previousProcesses())
  {
    const auto& processModel = prevProc.process().process();
    m_previousBackup.insert({processModel.id(), prevProc.process().messages()});

    auto lst = prevProc.process().setMessages(messages, m_oldState);

    updateTreeWithMessageList(
        m_newState, lst, processModel.id(), ProcessPosition::Previous);
  }

  for(const ProcessStateWrapper& nextProc : state.followingProcesses())
  {
    const auto& processModel = nextProc.process().process();
    m_followingBackup.insert({processModel.id(), nextProc.process().messages()});

    auto lst = nextProc.process().setMessages(messages, m_oldState);

    updateTreeWithMessageList(
        m_newState, lst, processModel.id(), ProcessPosition::Following);
  }

  // TODO one day there will also be State functions that will perform
  // some local computation.
  // TODO this day has come
  // WARNING we are in the future

  updateTreeWithMessageList(m_newState, messages);
}

ReplaceState::ReplaceState(
    const Scenario::StateModel& state, const State::MessageList& messages)
    : ReplaceState{state, state.messages().rootNode(), Process::MessageNode{}, messages}
{
}

void ReplaceStateBase::undo(const score::DocumentContext& ctx) const
{
  auto& sm = m_path.find(ctx);
  auto& model = sm.messages();
  model = m_oldState;
  // TODO we should reset the model

  // Restore the state of the processes.
  for(const ProcessStateWrapper& prevProc : sm.previousProcesses())
  {
    prevProc.process().setMessages(
        m_previousBackup.at(prevProc.process().process().id()), m_oldState);
  }
  for(const ProcessStateWrapper& nextProc : sm.followingProcesses())
  {
    nextProc.process().setMessages(
        m_followingBackup.at(nextProc.process().process().id()), m_oldState);
  }
}

void ReplaceStateBase::redo(const score::DocumentContext& ctx) const
{
  auto& model = m_path.find(ctx).messages();
  model = m_newState;
}

void ReplaceStateBase::serializeImpl(DataStreamInput& d) const
{
  d << m_path << m_oldState << m_newState << m_previousBackup << m_followingBackup;
}

void ReplaceStateBase::deserializeImpl(DataStreamOutput& d)
{
  d >> m_path >> m_oldState >> m_newState >> m_previousBackup >> m_followingBackup;
}

AddMessagesToState::AddMessagesToState(
    const Scenario::StateModel& state, const State::MessageList& messages)
    : ReplaceState{
        state, state.messages().rootNode(), state.messages().rootNode(), messages}
{
}

AddControlMessagesToState::AddControlMessagesToState(
    const Scenario::StateModel& state, std::vector<Process::ControlMessage>&& messages)
    : m_path{state}
{
  m_old = state.controlMessages().messages();
  m_new = m_old;
  ControlItemModel::addMessages(m_new, std::move(messages));
}

void AddControlMessagesToState::undo(const score::DocumentContext& ctx) const
{
  auto& sm = m_path.find(ctx);
  sm.controlMessages().replaceWith(m_old);
  sm.sig_controlMessagesUpdated();
}

void AddControlMessagesToState::redo(const score::DocumentContext& ctx) const
{
  auto& sm = m_path.find(ctx);
  sm.controlMessages().replaceWith(m_new);
  sm.sig_controlMessagesUpdated();
}

void AddControlMessagesToState::serializeImpl(DataStreamInput& d) const
{
  d << m_path << m_old << m_new;
}

void AddControlMessagesToState::deserializeImpl(DataStreamOutput& d)
{
  d >> m_path >> m_old >> m_new;
}
}
}
