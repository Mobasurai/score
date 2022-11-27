// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "Expression.hpp"

#include <State/Message.hpp>
#include <State/Relation.hpp>
#include <State/ValueConversion.hpp>

#include <QDebug>
#include <QMap>
#include <QStringBuilder>
namespace State
{
QString Message::toString() const
{
  return address.toString() + " " + State::convert::toPrettyString(value);
}

QDebug operator<<(QDebug s, const Message& mess)
{
  s << mess.toString();
  return s;
}

// That one is needed for std::list::operator< 's instantiation...
bool operator<(const State::ExprData& lhs, const State::ExprData& rhs)
{
  return lhs.toString() < rhs.toString();
}

bool findAddressInExpression(const State::Expression& expr, const State::Address& addr)
{
  if(auto rel = expr.target<Relation>())
  {
    if(auto r = rel->lhs.target<State::Address>())
    {
      if(State::addressIsChildOf(addr, *r))
        return true;
    }
    else if(auto r = rel->lhs.target<State::AddressAccessor>())
    {
      if(State::addressIsChildOf(addr, r->address))
        return true;
    }

    if(auto r = rel->rhs.target<State::Address>())
    {
      if(State::addressIsChildOf(addr, *r))
        return true;
    }
    else if(auto r = rel->rhs.target<State::AddressAccessor>())
    {
      if(State::addressIsChildOf(addr, r->address))
        return true;
    }
    return false;
  }
  else if(auto puls = expr.target<Pulse>())
  {
    return (State::addressIsChildOf(addr, puls->address));
  }
  else if(expr.hasChildren())
  {
    if(expr.childCount() == 1)
    {
      return findAddressInExpression(expr.childAt(0), addr);
    }
    else
    {
      return findAddressInExpression(expr.childAt(0), addr)
             || findAddressInExpression(expr.childAt(1), addr);
    }
  }
  return false;
}

void replaceAddress(
    State::Expression& expr, const State::Address& oldAddr,
    const State::Address& newAddr)
{
  if(auto rel = expr.target<State::Relation>())
  {
    if(auto addr = rel->lhs.target<State::Address>())
    {
      rerootAddress(*addr, oldAddr, newAddr);
    }
    else if(auto addr = rel->lhs.target<State::AddressAccessor>())
    {
      rerootAddress(addr->address, oldAddr, newAddr);
    }

    if(auto addr = rel->rhs.target<State::Address>())
    {
      rerootAddress(*addr, oldAddr, newAddr);
    }
    else if(auto addr = rel->rhs.target<State::AddressAccessor>())
    {
      rerootAddress(addr->address, oldAddr, newAddr);
    }
  }
  else if(auto puls = expr.target<State::Pulse>())
  {
    rerootAddress(puls->address, oldAddr, newAddr);
  }
  else if(expr.hasChildren())
  {
    if(expr.childCount() == 1)
    {
      replaceAddress(expr.childAt(0), oldAddr, newAddr);
    }
    else
    {
      replaceAddress(expr.childAt(0), oldAddr, newAddr);
      replaceAddress(expr.childAt(1), oldAddr, newAddr);
    }
  }
}

}

// That one is needed for Q_DECLARE_METATYPE in Qt 6....
bool operator<(const State::ExprData& lhs, const State::ExprData& rhs)
{
  return State::operator<(lhs, rhs);
}

#if !defined(SCORE_ALL_UNITY)
template class SCORE_LIB_STATE_EXPORT std::list<State::ExprData>;
#endif

QString State::ExprData::toString() const
{
  static const QMap<State::BinaryOperator, QString> binopMap{
      {State::BinaryOperator::AND, "and"},
      {State::BinaryOperator::OR, "or"},
      {State::BinaryOperator::XOR, "xor"},
  };
  static const QMap<State::UnaryOperator, QString> unopMap{
      {State::UnaryOperator::Not, "not"},
  };

  struct vis
  {
  public:
    using return_type = QString;
    return_type operator()(const State::Relation& rel) const
    {
      return State::toString(rel);
    }
    return_type operator()(const State::Pulse& rel) const
    {
      return State::toString(rel);
    }

    return_type operator()(const State::BinaryOperator rel) const
    {
      return binopMap[rel];
    }
    return_type operator()(const State::UnaryOperator rel) const { return unopMap[rel]; }
    return_type operator()(const InvisibleRootNode rel) const { return ""; }
    return_type operator()(ossia::monostate) const
    {
#if !defined(_MSC_VER)
      SCORE_ASSERT(!__PRETTY_FUNCTION__);
#endif
      return {};
    }
  };

  return ossia::visit(vis{}, m_data);
}

QString TreeNode<State::ExprData>::toString() const
{
  QString s;

  auto exprstr = static_cast<const State::ExprData&>(*this).toString();
  if(m_children.empty()) // Relation
  {
    if(this->is<InvisibleRootNode>())
    {
      ;
    }
    else
    {
      s = " { " % exprstr % " } ";
    }
  }
  else if(m_children.size() == 1) // unop
  {
    if(this->is<InvisibleRootNode>())
    {
      s = m_children.front().toString();
    }
    else
    {
      s = " { " % exprstr % " " % m_children.front().toString() % " } ";
    }
  }
  else // binop
  {
    SCORE_ASSERT(m_children.size() == 2);
    int n = 0;
    int max_n = m_children.size() - 1;
    for(const auto& child : m_children)
    {
      s += child.toString() % " ";
      if(n < max_n)
      {
        s += exprstr % " ";
        n++;
      }
    }
  }

  return s;
}

QString TreeNode<State::ExprData>::toPrettyString() const
{
  if(*this == State::defaultTrueExpression())
    return QObject::tr("True");
  if(*this == State::defaultFalseExpression())
    return QObject::tr("False");

  QString s;

  auto exprstr = static_cast<const State::ExprData&>(*this).toString();
  if(m_children.empty()) // Relation
  {
    if(this->is<InvisibleRootNode>())
    {
      ;
    }
    else if(this->parent()->is<InvisibleRootNode>())
    {
      s = exprstr;
    }
    else
    {
      s = "(" % exprstr % ")";
    }
  }
  else if(m_children.size() == 1) // unop
  {
    if(this->is<InvisibleRootNode>())
    {
      s = m_children.front().toPrettyString();
    }
    else
    {
      s = exprstr % m_children.front().toPrettyString();
    }
  }
  else // binop
  {
    SCORE_ASSERT(m_children.size() == 2);
    int n = 0;
    int max_n = m_children.size() - 1;
    for(const auto& child : m_children)
    {
      s += child.toPrettyString() % " ";
      if(n < max_n)
      {
        s += exprstr % " ";
        n++;
      }
    }
  }

  return s;
}

State::Expression State::defaultTrueExpression()
{
  using namespace std::literals;
  static const auto expr = *State::parseExpression("true == true"s);
  return expr;
}

State::Expression State::defaultFalseExpression()
{
  using namespace std::literals;
  static const auto expr = *State::parseExpression("true == false"s);
  return expr;
}

bool State::isTrueExpression(const QString& cond)
{
  return cond.isEmpty() || cond == " { true == true } " || cond == "true == true";
}

bool State::isEmptyExpression(const QString& cond)
{
  return cond.isEmpty();
}

SCORE_LIB_STATE_EXPORT
bool operator==(
    const TreeNode<State::ExprData>& lhs, const TreeNode<State::ExprData>& rhs)
{
  const auto& ltd = static_cast<const State::ExprData&>(lhs);
  const auto& rtd = static_cast<const State::ExprData&>(rhs);

  bool b = (ltd == rtd) && (lhs.m_children.size() == rhs.m_children.size());
  if(!b)
    return false;

  auto l = lhs.begin();
  auto e = lhs.end();
  auto r = rhs.begin();
  while(l != e)
  {
    if(*l != *r)
      return false;
    ++l;
    ++r;
  }

  return true;
}

SCORE_LIB_STATE_EXPORT
bool operator!=(
    const TreeNode<State::ExprData>& lhs, const TreeNode<State::ExprData>& rhs)
{
  return !(lhs == rhs);
}

TreeNode<State::ExprData>::iterator TreeNode<State::ExprData>::begin()
{
  return m_children.begin();
}

TreeNode<State::ExprData>::const_iterator TreeNode<State::ExprData>::begin() const
{
  return m_children.cbegin();
}

TreeNode<State::ExprData>::const_iterator TreeNode<State::ExprData>::cbegin() const
{
  return m_children.cbegin();
}

TreeNode<State::ExprData>& TreeNode<State::ExprData>::front()
{
  return m_children.front();
}

TreeNode<State::ExprData>::iterator TreeNode<State::ExprData>::end()
{
  return m_children.end();
}

TreeNode<State::ExprData>::const_iterator TreeNode<State::ExprData>::end() const
{
  return m_children.cend();
}

TreeNode<State::ExprData>::const_iterator TreeNode<State::ExprData>::cend() const
{
  return m_children.cend();
}

TreeNode<State::ExprData>& TreeNode<State::ExprData>::back()
{
  return m_children.back();
}

TreeNode<State::ExprData>::TreeNode() { }

TreeNode<State::ExprData>::TreeNode(const TreeNode& other)
    : State::ExprData{static_cast<const State::ExprData&>(other)}
    , m_children(other.m_children)
{
  setParent(other.m_parent);
  for(auto& child : m_children)
    child.setParent(this);
}

TreeNode<State::ExprData>::TreeNode(TreeNode&& other)
    : State::ExprData{std::move(static_cast<State::ExprData&&>(std::move(other)))}
    , m_children(std::move(other.m_children))
{
  setParent(other.m_parent);
  for(auto& child : m_children)
    child.setParent(this);
}

TreeNode<State::ExprData>& TreeNode<State::ExprData>::operator=(const TreeNode& source)
{
  static_cast<State::ExprData&>(*this) = static_cast<const State::ExprData&>(source);
  setParent(source.m_parent);

  m_children = source.m_children;
  for(auto& child : m_children)
  {
    child.setParent(this);
  }

  return *this;
}

TreeNode<State::ExprData>&
TreeNode<State::ExprData>::operator=(TreeNode<State::ExprData>&& source)
{
  static_cast<State::ExprData&>(*this) = static_cast<State::ExprData&&>(source);
  setParent(source.m_parent);

  m_children = std::move(source.m_children);
  for(auto& child : m_children)
  {
    child.setParent(this);
  }

  return *this;
}

TreeNode<State::ExprData>::TreeNode(TreeNode source, TreeNode* parent)
    : TreeNode{std::move(source)}
{
  setParent(parent);
}

void TreeNode<State::ExprData>::push_back(const TreeNode& child)
{
  m_children.push_back(child);

  auto& cld = m_children.back();
  cld.setParent(this);
}

void TreeNode<State::ExprData>::push_back(TreeNode&& child)
{
  m_children.push_back(std::move(child));

  auto& cld = m_children.back();
  cld.setParent(this);
}

TreeNode<State::ExprData>* TreeNode<State::ExprData>::parent() const
{
  return m_parent;
}

bool TreeNode<State::ExprData>::hasChild(std::size_t index) const
{
  return m_children.size() > index;
}

TreeNode<State::ExprData>& TreeNode<State::ExprData>::childAt(int index)
{
  return child_at(m_children, index);
}

const TreeNode<State::ExprData>& TreeNode<State::ExprData>::childAt(int index) const
{
  return child_at(m_children, index);
}

int TreeNode<State::ExprData>::indexOfChild(const TreeNode* child) const
{
  return index_of_child(m_children, child);
}

int TreeNode<State::ExprData>::childCount() const
{
  return m_children.size();
}

bool TreeNode<State::ExprData>::hasChildren() const
{
  return !m_children.empty();
}

std::list<TreeNode<State::ExprData>>& TreeNode<State::ExprData>::children()
{
  return m_children;
}

const std::list<TreeNode<State::ExprData>>& TreeNode<State::ExprData>::children() const
{
  return m_children;
}

void TreeNode<State::ExprData>::removeChild(const_iterator it)
{
  m_children.erase(it);
}

void TreeNode<State::ExprData>::setParent(TreeNode* parent)
{
  SCORE_ASSERT(
      !m_parent || (m_parent && !m_parent->is<State::Relation>())
      || (m_parent && !m_parent->is<State::Pulse>()));
  m_parent = parent;
}
