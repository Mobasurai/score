#pragma once
#include <State/Domain.hpp>

#include <Process/Dataflow/Port.hpp>
#include <Process/Dataflow/WidgetInlets.hpp>

#include <Gfx/TexturePort.hpp>

#include <score/tools/Debug.hpp>

#include <ossia/detail/math.hpp>
#include <ossia/detail/ssize.hpp>
#include <ossia/network/domain/domain.hpp>

#include <QJSValue>
#include <QObject>
#include <QQmlListProperty>
#include <QQuickItem>
#include <QVariant>
#include <QVector>

#include <libremidi/message.hpp>

#include <verdigris>
W_REGISTER_ARGTYPE(QJSValue)
namespace JS
{
class Inlet : public QObject
{
  W_OBJECT(Inlet)

public:
  using QObject::QObject;
  virtual ~Inlet() override;
  virtual Process::Inlet* make(Id<Process::Port>&& id, QObject*) = 0;
  virtual bool isEvent() const { return false; }

  W_INLINE_PROPERTY_CREF(QString, address, {}, address, setAddress, addressChanged)
};

class Outlet : public QObject
{
  W_OBJECT(Outlet)

public:
  using QObject::QObject;
  virtual ~Outlet() override;
  virtual Process::Outlet* make(Id<Process::Port>&& id, QObject*) = 0;

  W_INLINE_PROPERTY_CREF(QString, address, {}, address, setAddress, addressChanged)
};

struct InValueMessage
{
  W_GADGET(InValueMessage)

public:
  qreal timestamp;
  QVariant value;
  W_PROPERTY(qreal, timestamp MEMBER timestamp)
  W_PROPERTY(QVariant, value MEMBER value)
};

struct OutValueMessage
{
  W_GADGET(OutValueMessage)

public:
  qreal timestamp;
  QJSValue value;
  W_PROPERTY(qreal, timestamp MEMBER timestamp)
  W_PROPERTY(QJSValue, value MEMBER value)
};

class ValueInlet : public Inlet
{
  W_OBJECT(ValueInlet)

  QVariant m_value;
  QVariantList m_values;

public:
  explicit ValueInlet(QObject* parent = nullptr);
  virtual ~ValueInlet() override;
  QVariant value() const;
  QVariantList values() const { return m_values; }

  Process::Inlet* make(Id<Process::Port>&& id, QObject* parent) override
  {
    return new Process::ValueInlet(id, parent);
  }

  int length() const noexcept;
  W_SLOT(length)

  QVariant at(int index) const noexcept;
  W_SLOT(at)

  void clear() { m_values.clear(); }
  void setValue(QVariant value);
  void addValue(QVariant&& val) { m_values.append(std::move(val)); }
  void valueChanged(QVariant value) W_SIGNAL(valueChanged, value);

  W_PROPERTY(QVariantList, values READ values)
  W_PROPERTY(QVariant, value READ value NOTIFY valueChanged)
};

class ControlInlet : public Inlet
{
  W_OBJECT(ControlInlet)

  QVariant m_value;

public:
  explicit ControlInlet(QObject* parent = nullptr);
  virtual ~ControlInlet() override;
  QVariant value() const;

  Process::Inlet* make(Id<Process::Port>&& id, QObject* parent) override
  {
    return new Process::ControlInlet(id, parent);
  }

  void clear() { m_value = QVariant{}; }
  void setValue(QVariant value);
  void valueChanged(QVariant value) W_SIGNAL(valueChanged, value);

  W_PROPERTY(QVariant, value READ value NOTIFY valueChanged)
};

class FloatSlider : public ValueInlet
{
  W_OBJECT(FloatSlider)

public:
  using ValueInlet::ValueInlet;
  virtual ~FloatSlider() override;
  bool isEvent() const override { return true; }

  Process::Inlet* make(Id<Process::Port>&& id, QObject* parent) override
  {
    return new Process::FloatSlider{(float)m_min, (float)m_max, (float)m_init,
                                    objectName(), id,           parent};
  }

  W_INLINE_PROPERTY_VALUE(qreal, init, {0.5}, init, setInit, initChanged)
  W_INLINE_PROPERTY_VALUE(qreal, min, {0.}, getMin, setMin, minChanged)
  W_INLINE_PROPERTY_VALUE(qreal, max, {1.}, getMax, setMax, maxChanged)
};

class IntSlider : public ValueInlet
{
  W_OBJECT(IntSlider)

public:
  using ValueInlet::ValueInlet;
  virtual ~IntSlider() override;
  bool isEvent() const override { return true; }

  Process::Inlet* make(Id<Process::Port>&& id, QObject* parent) override
  {
    return new Process::IntSlider{m_min, m_max, m_init, objectName(), id, parent};
  }

  W_INLINE_PROPERTY_VALUE(int, init, {0}, init, setInit, initChanged)
  W_INLINE_PROPERTY_VALUE(int, min, {0}, getMin, setMin, minChanged)
  W_INLINE_PROPERTY_VALUE(int, max, {127}, getMax, setMax, maxChanged)
};

class Enum : public ValueInlet
{
  W_OBJECT(Enum)

public:
  using ValueInlet::ValueInlet;
  virtual ~Enum() override;
  bool isEvent() const override { return true; }

  Process::Inlet* make(Id<Process::Port>&& id, QObject* parent) override
  {
    return new Process::Enum{m_choices, {}, current(), objectName(), id, parent};
  }

  auto getValues() const { return choices(); }

  std::string current() const
  {
    if(!m_choices.isEmpty() && ossia::valid_index(m_index, m_choices))
    {
      return m_choices[m_index].toStdString();
    }
    return {};
  }

  W_INLINE_PROPERTY_VALUE(int, index, {}, index, setIndex, indexChanged)
  W_INLINE_PROPERTY_CREF(QStringList, choices, {}, choices, setChoices, choicesChanged)
};

class Toggle : public ValueInlet
{
  W_OBJECT(Toggle)

public:
  using ValueInlet::ValueInlet;
  virtual ~Toggle() override;
  bool isEvent() const override { return true; }
  Process::Inlet* make(Id<Process::Port>&& id, QObject* parent) override
  {
    return new Process::Toggle{m_checked, objectName(), id, parent};
  }

  W_INLINE_PROPERTY_VALUE(bool, checked, {}, checked, setChecked, checkedChanged)
};

class Button : public ValueInlet
{
  W_OBJECT(Button)

public:
  using ValueInlet::ValueInlet;
  virtual ~Button() override;
  bool isEvent() const override { return true; }
  Process::Inlet* make(Id<Process::Port>&& id, QObject* parent) override
  {
    return new Process::Button{objectName(), id, parent};
  }

  W_INLINE_PROPERTY_VALUE(bool, checked, {}, checked, setChecked, checkedChanged)
};

class Impulse : public ValueInlet
{
  W_OBJECT(Impulse)

public:
  using ValueInlet::ValueInlet;
  virtual ~Impulse() override;
  bool isEvent() const override { return false; }
  Process::Inlet* make(Id<Process::Port>&& id, QObject* parent) override
  {
    return new Process::ImpulseButton{objectName(), id, parent};
  }

  void impulse() W_SIGNAL(impulse);
};

class LineEdit : public ValueInlet
{
  W_OBJECT(LineEdit)

public:
  using ValueInlet::ValueInlet;
  virtual ~LineEdit() override;
  bool isEvent() const override { return true; }
  Process::Inlet* make(Id<Process::Port>&& id, QObject* parent) override
  {
    return new Process::LineEdit{m_text, objectName(), id, parent};
  }

  W_INLINE_PROPERTY_CREF(QString, text, {}, text, setText, textChanged)
};

class ValueOutlet : public Outlet
{
  W_OBJECT(ValueOutlet)

  QJSValue m_value;

public:
  std::vector<OutValueMessage> values;

  explicit ValueOutlet(QObject* parent = nullptr);
  virtual ~ValueOutlet() override;
  const QJSValue& value() const;
  void clear()
  {
    m_value = QJSValue{};
    values.clear();
  }
  Process::Outlet* make(Id<Process::Port>&& id, QObject* parent) override
  {
    return new Process::ValueOutlet(id, parent);
  }

public:
  void setValue(const QJSValue& value);
  W_SLOT(setValue);
  void addValue(qreal timestamp, QJSValue t);
  W_SLOT(addValue);

  W_PROPERTY(QJSValue, value READ value WRITE setValue)
};

class AudioInlet : public Inlet
{
  W_OBJECT(AudioInlet)

public:
  explicit AudioInlet(QObject* parent = nullptr);
  virtual ~AudioInlet() override;
  const QVector<QVector<double>>& audio() const;
  void setAudio(const QVector<QVector<double>>& audio);

  QVector<double> channel(int i) const
  {
    if(m_audio.size() > i)
      return m_audio[i];
    return {};
  }
  W_INVOKABLE(channel);

  Process::Inlet* make(Id<Process::Port>&& id, QObject* parent) override
  {
    return new Process::AudioInlet(id, parent);
  }

private:
  QVector<QVector<double>> m_audio;
};

class AudioOutlet : public Outlet
{
  W_OBJECT(AudioOutlet)

public:
  explicit AudioOutlet(QObject* parent = nullptr);
  virtual ~AudioOutlet() override;
  Process::Outlet* make(Id<Process::Port>&& id, QObject* parent) override
  {
    auto p = new Process::AudioOutlet(id, parent);
    if(id.val() == 0)
      p->setPropagate(true);
    return p;
  }

  const QVector<QVector<double>>& audio() const;

  void setChannel(int i, const QJSValue& v);
  W_INVOKABLE(setChannel)
private:
  QVector<QVector<double>> m_audio;
};

class MidiMessage
{
  W_GADGET(MidiMessage)

public:
  QByteArray bytes;

  W_PROPERTY(QByteArray, bytes MEMBER bytes)
};

class MidiInlet : public Inlet
{
  W_OBJECT(MidiInlet)

public:
  explicit MidiInlet(QObject* parent = nullptr);
  virtual ~MidiInlet() override;
  template <typename T>
  void setMidi(const T& arr)
  {
    m_midi.clear();
    for(const libremidi::message& mess : arr)
    {
      const auto N = mess.size();
      QVector<int> m;
      m.resize(N);

      for(std::size_t i = 0; i < N; i++)
        m[i] = mess.bytes[i];

      m_midi.push_back(QVariant::fromValue(m));
    }
  }

  QVariantList messages() const { return m_midi; }
  W_INVOKABLE(messages);

  Process::Inlet* make(Id<Process::Port>&& id, QObject* parent) override
  {
    return new Process::MidiInlet(id, parent);
  }

private:
  QVariantList m_midi;
};

class MidiOutlet : public Outlet
{
  W_OBJECT(MidiOutlet)

public:
  explicit MidiOutlet(QObject* parent = nullptr);
  virtual ~MidiOutlet() override;
  Process::Outlet* make(Id<Process::Port>&& id, QObject* parent) override
  {
    return new Process::MidiOutlet(id, parent);
  }

  void clear();
  const QVector<QVector<int>>& midi() const;

  void setMessages(const QVariantList m)
  {
    m_midi.clear();
    for(auto& v : m)
    {
      if(v.canConvert<QVector<int>>())
        m_midi.push_back(v.value<QVector<int>>());
    }
  }
  W_INVOKABLE(setMessages);

  void add(QVector<int> m) { m_midi.push_back(std::move(m)); }
  W_INVOKABLE(add);

private:
  QVector<QVector<int>> m_midi;
};

class TextureOutlet : public Outlet
{
  W_OBJECT(TextureOutlet)

public:
  explicit TextureOutlet(QObject* parent = nullptr);
  virtual ~TextureOutlet() override;
  Process::Outlet* make(Id<Process::Port>&& id, QObject* parent) override
  {
    auto p = new Gfx::TextureOutlet(id, parent);
    return p;
  }

  QQuickItem* item() /*Qt6: const*/ noexcept { return m_item; }
  void setItem(QQuickItem* v) { m_item = v; }

  W_PROPERTY(QQuickItem*, item READ item WRITE setItem CONSTANT)
private:
  QQuickItem* m_item{};
};

class Script : public QObject
{
  W_OBJECT(Script)
  W_CLASSINFO("DefaultProperty", "data")
  W_CLASSINFO(
      "qt_QmlJSWrapperFactoryMethod", "_q_createJSWrapper(QV4::ExecutionEngine*)")

public:
  QQmlListProperty<QObject> data() noexcept { return {this, &m_data}; }

  QJSValue& tick() /*Qt6: const*/ noexcept { return m_tick; }
  void setTick(const QJSValue& v) { m_tick = v; }
  QJSValue& start() /*Qt6: const*/ noexcept { return m_start; }
  void setStart(const QJSValue& v) { m_start = v; }
  QJSValue& stop() /*Qt6: const*/ noexcept { return m_stop; }
  void setStop(const QJSValue& v) { m_stop = v; }
  QJSValue& pause() /*Qt6: const*/ noexcept { return m_pause; }
  void setPause(const QJSValue& v) { m_pause = v; }
  QJSValue& resume() /*Qt6: const*/ noexcept { return m_resume; }
  void setResume(const QJSValue& v) { m_resume = v; }
  W_PROPERTY(QJSValue, tick READ tick WRITE setTick CONSTANT)
  W_PROPERTY(QJSValue, start READ start WRITE setStart CONSTANT)
  W_PROPERTY(QJSValue, stop READ stop WRITE setStop CONSTANT)
  W_PROPERTY(QJSValue, pause READ pause WRITE setPause CONSTANT)
  W_PROPERTY(QJSValue, resume READ resume WRITE setResume CONSTANT)
  W_PROPERTY(QQmlListProperty<QObject>, data READ data)

private:
  QList<QObject*> m_data;
  QJSValue m_tick;
  QJSValue m_start;
  QJSValue m_stop;
  QJSValue m_pause;
  QJSValue m_resume;
};
}

inline QDataStream& operator<<(QDataStream& i, const JS::MidiMessage& sel)
{
  SCORE_ABORT;
  return i;
}
inline QDataStream& operator>>(QDataStream& i, JS::MidiMessage& sel)
{
  SCORE_ABORT;
  return i;
}
inline QDataStream& operator<<(QDataStream& i, const JS::InValueMessage& sel)
{
  SCORE_ABORT;
  return i;
}
inline QDataStream& operator>>(QDataStream& i, JS::InValueMessage& sel)
{
  SCORE_ABORT;
  return i;
}
inline QDataStream& operator<<(QDataStream& i, const JS::OutValueMessage& sel)
{
  SCORE_ABORT;
  return i;
}
inline QDataStream& operator>>(QDataStream& i, JS::OutValueMessage& sel)
{
  SCORE_ABORT;
  return i;
}
Q_DECLARE_METATYPE(JS::ValueInlet*)
Q_DECLARE_METATYPE(JS::ValueOutlet*)
Q_DECLARE_METATYPE(JS::AudioInlet*)
Q_DECLARE_METATYPE(JS::AudioOutlet*)
Q_DECLARE_METATYPE(JS::MidiMessage)
Q_DECLARE_METATYPE(JS::MidiInlet*)
Q_DECLARE_METATYPE(JS::MidiOutlet*)

W_REGISTER_ARGTYPE(JS::ValueInlet*)
W_REGISTER_ARGTYPE(JS::ValueOutlet*)
W_REGISTER_ARGTYPE(JS::AudioInlet*)
W_REGISTER_ARGTYPE(JS::AudioOutlet*)
W_REGISTER_ARGTYPE(JS::MidiMessage)
W_REGISTER_ARGTYPE(JS::MidiInlet*)
W_REGISTER_ARGTYPE(JS::MidiOutlet*)
