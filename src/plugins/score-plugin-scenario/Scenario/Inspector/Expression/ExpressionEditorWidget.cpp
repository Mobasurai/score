// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "ExpressionEditorWidget.hpp"

#include <Scenario/Inspector/Expression/ExpressionMenu.hpp>
#include <Scenario/Inspector/Expression/SimpleExpressionEditorWidget.hpp>

#include <score/tools/std/Optional.hpp>
#include <score/widgets/MarginLess.hpp>
#include <score/widgets/SetIcons.hpp>

#include <ossia/detail/ssize.hpp>

#include <QDebug>
#include <QVBoxLayout>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Scenario::ExpressionEditorWidget)
W_OBJECT_IMPL(Scenario::ExpressionMenu)
namespace Scenario
{
ExpressionEditorWidget::ExpressionEditorWidget(
    const score::DocumentContext& doc, QWidget* parent)
    : QWidget(parent)
    , m_context{doc}
{
  this->setObjectName("ExpressionEditorWidget");
  m_mainLayout = new score::MarginLess<QVBoxLayout>{this};
}

State::Expression ExpressionEditorWidget::expression()
{

  switch(m_relations.size())
  {
    case 0:
      return {};
    case 1:
      return m_relations[0]->relation();
    case 2: {
      SCORE_ASSERT(m_relations[1]->binOperator());
      State::Expression root;
      State::Expression& op = root.emplace_back(*m_relations[1]->binOperator(), nullptr);
      op.emplace_back(m_relations[0]->relation(), &op);
      op.emplace_back(m_relations[1]->relation(), &op);
      return root;
    }
    default:
      SCORE_ABORT;
      return {};
  }
  /*
  State::Expression exp{};

  State::Expression* lastRel{};

  // will keep containing the last relation added

  for (SimpleExpressionEditorWidget* r : m_relations)
  {
    if (!exp.hasChildren())
    {
      // add the first node : simple relation
      exp.emplace_back(r->relation(), &exp);
      lastRel = &(exp.back());
    }
    else
    {
      if (!lastRel)
      {
        // TODO investigate with scan-build.
        qDebug() << "We shouldn't be in this case";
        continue;
      }

      auto op = r->binOperator();
      if (op == State::BinaryOperator::OR)
      {
        auto pOp = op;
        if (lastRel->parent()->is<State::BinaryOperator>())
          pOp = lastRel->parent()->get<State::BinaryOperator>();

        // we're taking out the child of an "OR" node or of the root
        while (pOp != State::BinaryOperator::OR && lastRel->parent() != &exp)
        {
          lastRel = lastRel->parent();
          if (lastRel->is<State::BinaryOperator>())
            pOp = lastRel->get<State::BinaryOperator>();
        }
      }
      if (op)
      {
        auto p = lastRel->parent();
        // remove link between parent and current
        if (p && !p->hasChildren())
        {
          auto oldC = p->back();
          auto last_it = (++p->children().rbegin()).base();
          p->removeChild(last_it);

          // insert operator
          p->emplace_back(*op, p);
          auto& nOp = p->front();

          // recreate link
          oldC.setParent(&nOp);
          nOp.push_back(oldC);

          // add the relation as child of the inserted operator
          nOp.emplace_back(r->relation(), &nOp);
        }
        else
        {
          // TODO investigate with scan-build.
          qDebug() << "We shouldn't be in this case";
          continue;
        }
      }
    }
  }
  //    qDebug() << "-----------" << exp.toString() << "-----------";
  return exp;*/
}

void ExpressionEditorWidget::setExpression(State::Expression e)
{
  for(auto& elt : m_relations)
  {
    delete elt;
  }
  m_relations.clear();

  exploreExpression(e);
  if(!e.hasChildren())
    addNewTerm();

  m_expression = currentExpr();
}

void ExpressionEditorWidget::setMenu(QMenu* menu)
{
  m_menu = menu;
}

void ExpressionEditorWidget::on_editFinished()
{
  auto ex = currentExpr();
  auto e = State::parseExpression(ex);
  if(m_expression == ex)
    return;
  if(!e && !ex.isEmpty())
  {
    qDebug() << "invalid expression ! " << ex;
    return;
  }

  m_expression = ex;
  editingFinished();
}

void ExpressionEditorWidget::exploreExpression(State::Expression expr)
{
  const struct
  {
  public:
    const State::Expression& e;
    ExpressionEditorWidget& widg;
    using return_type = void;

    return_type operator()(const State::Relation& rel) const
    {
      widg.addNewTerm();
      if(!widg.m_relations.empty())
        widg.m_relations.back()->setRelation(rel);
    }

    return_type operator()(const State::Pulse& p) const
    {
      widg.addNewTerm();
      if(!widg.m_relations.empty())
        widg.m_relations.back()->setPulse(p);
    }

    return_type operator()(const State::BinaryOperator op) const
    {
      auto a = e.childAt(0);
      auto b = e.childAt(1);

      widg.exploreExpression(a);
      widg.exploreExpression(b);

      const int num = widg.m_relations.size();
      for(int i = 1; i < num; i++)
      {
        auto rel = widg.m_relations.at(i);
        if(!rel->binOperator())
          rel->setOperator(op);
      }

      if(!widg.m_relations.empty())
        widg.m_relations.back()->setOperator(op);
    }

    return_type operator()(const State::UnaryOperator) const
    {
      SCORE_TODO_("Implement unary operator");
      widg.exploreExpression(e.childAt(0));
    }

    return_type operator()(const InvisibleRootNode) const
    {
      if(e.childCount() > 0)
        widg.exploreExpression(e.childAt(0));
    }
    return_type operator()(const ossia::monostate&) const { SCORE_ASSERT(false); }
  } visitor{expr, *this};

  return ossia::visit(visitor, expr.impl());
}

QString ExpressionEditorWidget::currentExpr()
{
  auto exp = expression();
  return exp.toString();
}

void ExpressionEditorWidget::addNewTerm()
{
  auto relationEditor = new SimpleExpressionEditorWidget{
      m_context, std::ssize(m_relations), this, m_menu};
  m_relations.push_back(relationEditor);

  m_mainLayout->addWidget(relationEditor);

  connect(
      relationEditor, &SimpleExpressionEditorWidget::addTerm, this,
      &ExpressionEditorWidget::addNewTermAndFinish, Qt::QueuedConnection);
  connect(
      relationEditor, &SimpleExpressionEditorWidget::removeTerm, this,
      &ExpressionEditorWidget::removeTermAndFinish, Qt::QueuedConnection);
  connect(
      relationEditor, &SimpleExpressionEditorWidget::editingFinished, this,
      &ExpressionEditorWidget::on_editFinished, Qt::QueuedConnection);

  if(m_relations.size() == 1)
  {
    m_relations[0]->enableRemoveButton(false);
  }
  else if(m_relations.size() > 1)
  {
    m_relations[m_relations.size() - 2]->enableAddButton(false);
    m_relations[0]->enableRemoveButton(true);
  }

  m_relations.front()->enableMenuButton(true);
  m_relations.back()->enableAddButton(true);
}

void ExpressionEditorWidget::addNewTermAndFinish()
{
  if(m_relations.size() < 2)
  {
    addNewTerm();
    on_editFinished();
  }
}

void ExpressionEditorWidget::removeTermAndFinish(int index)
{
  removeTerm(index);
  on_editFinished();
}

void ExpressionEditorWidget::removeTerm(int index)
{
  if(m_relations.size() > 1)
  {
    const int n = std::ssize(m_relations);
    for(int i = index + 1; i < n; i++)
    {
      m_relations.at(i)->decreaseId();
    }
    delete m_relations.at(index);
    m_relations.erase(m_relations.begin() + index);
    m_relations[0]->enableRemoveButton(m_relations.size() > 1);
    // TODO the model should be updated here.
  }
  else if(m_relations.size() == 1)
  {
    // We just clear the expression
    resetExpression();
    m_relations[0]->enableRemoveButton(false);
  }
  m_relations.back()->enableAddButton(true);
  m_relations.front()->enableMenuButton(true);
}
}
