// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "MidiExecutor.hpp"

#include <Midi/MidiProcess.hpp>

#include <ossia/dataflow/nodes/midi.hpp>
namespace Midi
{
namespace Executor
{

using midi_node = ossia::nodes::midi;
static midi_node::note_set to_ossia(Component& c)
{
  midi_node::note_set notes;
  auto& element = c.process();
  notes.reserve(element.notes.size());
  for(const auto& n : element.notes)
  {
    auto data = n.noteData();
    if(data.start() < 0 && data.end() > 0)
    {
      data.setStart(0.);
      data.setDuration(data.duration() + data.start());
    }
    notes.insert(c.to_note(data));
  }
  return notes;
}

using midi_node_process = ossia::nodes::midi_node_process;
Component::Component(
    Midi::ProcessModel& element, const Execution::Context& ctx, QObject* parent)
    : ::Execution::ProcessComponent_T<Midi::ProcessModel, ossia::node_process>{
        element, ctx, "MidiComponent", parent}
{
  auto midi = ossia::make_node<midi_node>(*ctx.execState, element.notes.size());
  this->node = midi;
  m_ossia_process = std::make_shared<midi_node_process>(midi);

  midi->set_channel(element.channel());
  midi->set_notes(to_ossia(*this));

  element.notes.added.connect<&Component::on_noteAdded>(this);
  element.notes.removing.connect<&Component::on_noteRemoved>(this);
  element.notes.replaced.connect<&Component::on_notesReplaced>(this);

  for(auto& note : element.notes)
  {
    QObject::connect(
        &note, &Note::noteChanged, this,
        [&, midi, cur = to_note(note.noteData())]() mutable {
      auto old = cur;
      cur = to_note(note.noteData());
      in_exec([old, cur, midi] { midi->update_note(old, cur); });
        });
  }

  QObject::connect(
      &element, &Midi::ProcessModel::notesChanged, this, &Component::on_notesReplaced);
}

Component::~Component() { }

void Component::on_noteAdded(const Note& n)
{
  auto midi = std::dynamic_pointer_cast<midi_node>(node);
  in_exec([nd = to_note(n.noteData()), midi] { midi->add_note(nd); });

  QObject::connect(
      &n, &Note::noteChanged, this, [&, midi, cur = to_note(n.noteData())]() mutable {
        auto old = cur;
        cur = to_note(n.noteData());
        in_exec([old, cur, midi] { midi->update_note(old, cur); });
      });
}

void Component::on_noteRemoved(const Note& n)
{
  auto midi = std::dynamic_pointer_cast<midi_node>(node);
  in_exec([nd = to_note(n.noteData()), midi] { midi->remove_note(nd); });
}

void Component::on_notesReplaced()
{
  auto midi = std::dynamic_pointer_cast<midi_node>(node);

  in_exec([n = to_ossia(*this), midi]() mutable { midi->replace_notes(std::move(n)); });
}
ossia::nodes::note_data Component::to_note(const NoteData& n)
{
  auto& cv_time = system().time;
  return {
      cv_time(process().duration() * n.start()),
      cv_time(process().duration() * n.duration()), n.pitch(), n.velocity()};
}
}
}
