#pragma once

#include <score/tools/Debug.hpp>

#include <QByteArray>
#include <QColor>
#include <QDataStream>
#include <QPoint>
#include <QPointF>
#include <QPointer>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QVariant>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QtContainerFwd>

#include <score_lib_base_export.h>

#include <memory>
#include <string>
#include <type_traits>

template <typename T>
struct is_shared_ptr : std::false_type
{
};
template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type
{
};
template <typename T>
static constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;
template <typename T>
struct is_qpointer : std::false_type
{
};
template <typename T>
struct is_qpointer<QPointer<T>> : std::true_type
{
};
template <typename T>
static constexpr bool is_qpointer_v = is_qpointer<T>::value;

#if defined(SCORE_DEBUG_DELIMITERS)
#define SCORE_DEBUG_INSERT_DELIMITER \
  do                                 \
  {                                  \
    insertDelimiter();               \
  } while(0)
#define SCORE_DEBUG_INSERT_DELIMITER2(Vis) \
  do                                       \
  {                                        \
    Vis.insertDelimiter();                 \
  } while(0)
#define SCORE_DEBUG_CHECK_DELIMITER \
  do                                \
  {                                 \
    checkDelimiter();               \
  } while(0)
#define SCORE_DEBUG_CHECK_DELIMITER2(Vis) \
  do                                      \
  {                                       \
    Vis.checkDelimiter();                 \
  } while(0)
#else
#define SCORE_DEBUG_INSERT_DELIMITER
#define SCORE_DEBUG_INSERT_DELIMITER2(Vis)
#define SCORE_DEBUG_CHECK_DELIMITER
#define SCORE_DEBUG_CHECK_DELIMITER2(Vis)
#endif

/**
 * \file DataStreamVisitor.hpp
 *
 * This file contains facilities
 * to serialize an object using QDataStream.
 *
 * Generally, it is used with QByteArrays, but it works with any QIODevice.
 */
template <typename T>
concept is_QDataStreamSerializable
    = std::is_arithmetic<T>::value || std::is_same<T, QStringList>::value
      || std::is_same<T, QVector2D>::value || std::is_same<T, QVector3D>::value
      || std::is_same<T, QVector4D>::value || std::is_same<T, QPointF>::value
      || std::is_same<T, QPoint>::value || std::is_same<T, std::string>::value;

SCORE_LIB_BASE_EXPORT QDataStream&
operator<<(QDataStream& stream, const std::string& obj);
SCORE_LIB_BASE_EXPORT QDataStream& operator>>(QDataStream& stream, std::string& obj);

class QIODevice;

struct DataStreamInput
{
  QDataStream& stream;
};
struct DataStreamOutput
{
  QDataStream& stream;
};

#if(INTPTR_MAX == INT64_MAX) && !defined(__APPLE__) && !defined(_WIN32)
#define SCORE_INT64_IS_QINT64
#endif

#if defined(SCORE_INT64_IS_QINT64)
inline QDataStream& operator<<(QDataStream& s, uint64_t val)
{
  s << (quint64)val;
  return s;
}
inline QDataStream& operator>>(QDataStream& s, uint64_t& val)
{
  s >> (quint64&)val;
  return s;
}
inline QDataStream& operator<<(QDataStream& s, int64_t val)
{
  s << (qint64)val;
  return s;
}
inline QDataStream& operator>>(QDataStream& s, int64_t& val)
{
  s >> (qint64&)val;
  return s;
}
#endif

#if defined(__APPLE__) || defined(__EMSCRIPTEN__)
inline QDataStream& operator<<(QDataStream& s, std::size_t val)
{
  s << (quint64)val;
  return s;
}
inline QDataStream& operator>>(QDataStream& s, std::size_t& val)
{
  s >> (quint64&)val;
  return s;
}
#endif

template <typename T>
  requires(!std::is_enum_v<T>)
DataStreamInput& operator<<(DataStreamInput& s, const T& obj);

template <typename T>
  requires(!std::is_enum_v<T>)
DataStreamOutput& operator>>(DataStreamOutput& s, T& obj);

#define DATASTREAM_QT_BUILTIN(T)                                \
  OSSIA_INLINE                                                  \
  DataStreamInput& operator<<(DataStreamInput& s, const T& obj) \
  {                                                             \
    s.stream << obj;                                            \
    return s;                                                   \
  }                                                             \
  OSSIA_INLINE                                                  \
  DataStreamOutput& operator>>(DataStreamOutput& s, T& obj)     \
  {                                                             \
    s.stream >> obj;                                            \
    return s;                                                   \
  }

DATASTREAM_QT_BUILTIN(bool)
DATASTREAM_QT_BUILTIN(char)
DATASTREAM_QT_BUILTIN(qint8)
DATASTREAM_QT_BUILTIN(qint16)
DATASTREAM_QT_BUILTIN(qint32)
DATASTREAM_QT_BUILTIN(qint64)

#if defined(SCORE_INT64_IS_QINT64)
DATASTREAM_QT_BUILTIN(int64_t)
#endif

DATASTREAM_QT_BUILTIN(quint8)
DATASTREAM_QT_BUILTIN(quint16)
DATASTREAM_QT_BUILTIN(quint32)
DATASTREAM_QT_BUILTIN(quint64)

#if defined(SCORE_INT64_IS_QINT64)
DATASTREAM_QT_BUILTIN(uint64_t)
#endif

DATASTREAM_QT_BUILTIN(float)
DATASTREAM_QT_BUILTIN(double)
DATASTREAM_QT_BUILTIN(QString)
DATASTREAM_QT_BUILTIN(QStringList)
DATASTREAM_QT_BUILTIN(QPoint)
DATASTREAM_QT_BUILTIN(QPointF)
DATASTREAM_QT_BUILTIN(QRect)
DATASTREAM_QT_BUILTIN(QRectF)
DATASTREAM_QT_BUILTIN(QVector2D)
DATASTREAM_QT_BUILTIN(QVector3D)
DATASTREAM_QT_BUILTIN(QVector4D)
DATASTREAM_QT_BUILTIN(QByteArray)
DATASTREAM_QT_BUILTIN(QSize)
DATASTREAM_QT_BUILTIN(QSizeF)
DATASTREAM_QT_BUILTIN(QColor)
DATASTREAM_QT_BUILTIN(QVariant)
DATASTREAM_QT_BUILTIN(std::string)

template <typename T>
DataStreamInput& operator<<(DataStreamInput& s, const QList<T>& obj) = delete;
template <typename T>
DataStreamOutput& operator>>(DataStreamOutput& s, const QList<T>& obj) = delete;

template <typename K, typename V>
DataStreamInput& operator<<(DataStreamInput& s, const QMap<K, V>& obj) = delete;
template <typename K, typename V>
DataStreamOutput& operator>>(DataStreamOutput& s, const QMap<K, V>& obj) = delete;

template <typename K, typename V>
DataStreamInput& operator<<(DataStreamInput& s, const QHash<K, V>& obj) = delete;
template <typename K, typename V>
DataStreamOutput& operator>>(DataStreamOutput& s, const QHash<K, V>& obj) = delete;

template <typename T>
  requires(std::is_enum_v<T>)
OSSIA_INLINE DataStreamInput& operator<<(DataStreamInput& s, const T& obj)
{
  s.stream << obj;
  return s;
}

template <typename T>
  requires(std::is_enum_v<T>)
OSSIA_INLINE DataStreamOutput& operator>>(DataStreamOutput& s, T& obj)
{
  s.stream >> obj;
  return s;
}
