#include "AddArea.hpp"

#include "src/Area/AreaModel.hpp"
#include "src/Area/AreaParser.hpp"
#include "src/Space/SpaceModel.hpp"

#include "src/Area/AreaFactory.hpp"
#include "src/Area/SingletonAreaFactoryList.hpp"

#include "src/SpaceProcess.hpp"

#include <boost/range/algorithm/find_if.hpp>
#include <iscore/tools/SettableIdentifierGeneration.hpp>


AddArea::AddArea(ModelPath<SpaceProcess> &&spacProcess,
                 int areatype,
                 const QString &area,
                 const QMap<QString, QString> &dimMap,
                 const QMap<QString, iscore::FullAddressSettings> &addrMap):
    iscore::SerializableCommand{factoryName(), commandName(), description()},
    m_path{std::move(spacProcess)},
    m_areaType{areatype},
    m_areaFormula{area},
    m_varToDimensionMap{dimMap},
    m_symbolToAddressMap{addrMap}
{
    m_createdAreaId = getStrongId(m_path.find().areas());
}

void AddArea::undo()
{
    auto& proc = m_path.find();
    proc.removeArea(m_createdAreaId);
}

void AddArea::redo()
{
    auto& proc = m_path.find();

    auto facts = SingletonAreaFactoryList::instance().factories();
    auto it = boost::range::find_if(facts,
                                    [&] (const AreaFactory* f) { return f->type() == m_areaType; });

    Q_ASSERT(it != facts.end());

    auto ar = (*it)->makeModel(m_areaFormula, proc.space(), m_createdAreaId, &proc);

    GiNaC::exmap sym_map;
    const auto& syms = ar->area().symbols();
    for(const auto& elt : m_varToDimensionMap.keys())
    {
        auto sym_it = std::find_if(syms.begin(), syms.end(),
                                   [&] (const GiNaC::symbol& sym) { return sym.get_name() == m_varToDimensionMap[elt].toStdString(); });
        Q_ASSERT(sym_it != syms.end());

        sym_map[*sym_it] = proc.space().dimension(elt).sym().symbol();
    }


    AreaModel::ParameterMap addr_map;
    for(const auto& elt : m_symbolToAddressMap.keys())
    {
        auto sym_it = std::find_if(syms.begin(), syms.end(),
                                   [&] (const GiNaC::symbol& sym) { return sym.get_name() == elt.toStdString(); });
        Q_ASSERT(sym_it != syms.end());

        addr_map[elt] = {*sym_it, m_symbolToAddressMap[elt]};
    }

    ar->setSpaceMapping(sym_map);
    ar->setParameterMapping(addr_map);

    proc.addArea(ar);
}

void AddArea::serializeImpl(QDataStream &s) const
{
    s << m_path << m_createdAreaId << m_areaType << m_areaFormula << m_varToDimensionMap << m_symbolToAddressMap;
}

void AddArea::deserializeImpl(QDataStream &s)
{
    s >> m_path >> m_createdAreaId >> m_areaType >> m_areaFormula >> m_varToDimensionMap >> m_symbolToAddressMap;
}
