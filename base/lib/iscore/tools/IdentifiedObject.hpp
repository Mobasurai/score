#pragma once
#include <iscore/tools/NamedObject.hpp>
#include <iscore/tools/SettableIdentifier.hpp>
#include <iscore/tools/utilsCPP11.hpp>
#include <typeinfo>
#include <random>

// This should maybe be a mixin ?

template<typename tag>
class IdentifiedObject : public IdentifiedObjectAbstract
{

    public:
        template<typename... Args>
        IdentifiedObject(id_type<tag> id,
                         Args&& ... args) :
            IdentifiedObjectAbstract {std::forward<Args> (args)...},
        m_id {id}
        {
        }

        template<typename ReaderImpl, typename... Args>
        IdentifiedObject(Deserializer<ReaderImpl>& v,
                         Args&& ... args) :
            IdentifiedObjectAbstract {v, std::forward<Args> (args)...}
        {
            v.writeTo(*this);
        }

        const id_type<tag>& id() const
        {
            return m_id;
        }

        virtual int32_t id_val() const override
        {
            return *m_id.val();
        }

        void setId(id_type<tag>&& id)
        {
            m_id = id;
        }

    private:
        id_type<tag> m_id {};
};
////////////////////////////////////////////////
///
///Functions that operate on collections of identified objects.
///
////////////////////////////////////////////////
template<typename Container>
typename Container::value_type findById(const Container& c, int32_t id)
{
    auto it = std::find_if(std::begin(c),
                           std::end(c),
                           [&id](typename Container::value_type model)
    {
        return model->id_val() == id;
    });

    if(it != std::end(c))
    {
        return *it;
    }

    throw std::runtime_error(QString("findById : id %1 not found in vector of %2").arg(id).arg(typeid(c).name()).toLatin1().constData());
}

template<typename Container, typename id_T>
typename Container::value_type findById(const Container& c, id_T id)
{
    auto it = std::find_if(std::begin(c),
                           std::end(c),
                           [&id](typename Container::value_type model)
    {
        return model->id() == id;
    });

    if(it != std::end(c))
    {
        return *it;
    }

    throw std::runtime_error(QString("findById : id %1 not found in vector of %2").arg(*id.val()).arg(typeid(c).name()).toLatin1().constData());
}

inline int32_t getNextId()
{
    using namespace std;
    static random_device rd;
    static mt19937 gen(rd());
    static uniform_int_distribution<int32_t>
    dist(numeric_limits<int32_t>::min(),
         numeric_limits<int32_t>::max());

    return dist(gen);
}

inline int getNextId(const std::vector<int>& ids)
{
    int id {};

    do
    {
        id = getNextId();
    }
    while(find(begin(ids),
               end(ids),
               id) != end(ids));

    return id;
}

template<typename T>
id_type<T> getStrongId(const std::vector<T*>& v)
{
    using namespace std;
    vector<int> ids(v.size());   // Map reduce

    transform(begin(v),
              end(v),
              begin(ids),
              [](const T * elt)
    {
        return * (elt->id().val());
    });

    return id_type<T> {getNextId(ids) };
}

template<typename T>
id_type<T> getStrongId(const QVector<T*>& v)
{
    using namespace std;
    vector<int> ids(v.size());   // Map reduce

    transform(begin(v),
              end(v),
              begin(ids),
              [](const T * elt)
    {
        return * (elt->id().val());
    });

    return id_type<T> {getNextId(ids) };
}

template<typename T>
id_type<T> getStrongId(const std::vector<id_type<T>>& ids)
{
    id_type<T> id {};

    do
    {
        id = id_type<T> {getNextId() };
    }
    while(find(begin(ids),
               end(ids),
               id) != end(ids));

    return id;
}

template<typename T>
id_type<T> getStrongId(const QVector<id_type<T>>& ids)
{
    using namespace std;
    id_type<T> id {};

    do
    {
        id = id_type<T> {getNextId() };
    }
    while(find(begin(ids),
               end(ids),
               id) != end(ids));

    return id;
}


template <typename Vector, typename id_T>
void removeById(Vector& c, id_T id)
{
    vec_erase_remove_if(c,
                        [&id](typename Vector::value_type model)
    {
        bool to_delete = model->id() == id;

        if(to_delete)
        {
            delete model;
        }

        return to_delete;
    });
}

template<typename Vector, typename id_T>
void removeFromVectorWithId(Vector& v,
                            id_T id)
{
    auto it = std::find_if(std::begin(v),
                           std::end(v),
                           [id](const typename Vector::value_type& elt)
    {
        return elt->id() == id;
    });

    if(it != std::end(v))
    {
        delete *it;
        v.erase(it);
    }
}

