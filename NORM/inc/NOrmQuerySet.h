#ifndef QDJANGO_QUERYSET_H
#define QDJANGO_QUERYSET_H

#include "NOrm.h"
#include "NOrmWhere.h"
#include "NOrmQuerySet_p.h"

template <class T>
    class NOrmQuerySet
{
public:
    /** \cond declarations for STL-style container algorithms */
    typedef int size_type;
    typedef T value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef qptrdiff difference_type;
    /** \endcond */

    class const_iterator
    {
        friend class NOrmQuerySet;

    public:
        /** A synonym for std::bidirectional_iterator_tag indicating this iterator
         *  permits bidirectional access.
         */
        typedef std::bidirectional_iterator_tag  iterator_category;

        /** \cond declarations for STL-style container algorithms */
        typedef qptrdiff difference_type;
        typedef T value_type;
        typedef T *pointer;
        typedef T &reference;
        /** \endcond */

        const_iterator()
            : m_querySet(0)
            , m_fetched(-1)
            , m_offset(0)
        {
        }

        /** Constructs a copy of \p other.
         */
        const_iterator(const const_iterator &other)
            : m_querySet(other.m_querySet)
            , m_fetched(-1)
            , m_offset(other.m_offset)
        {
        }

    private:
        const_iterator(const NOrmQuerySet<T> *querySet, int offset = 0)
            : m_querySet(querySet)
            , m_fetched(-1)
            , m_offset(offset)
        {
        }

    public:
        /** Returns the current item.
         *
         *  \sa operator->()
         */
        const T &operator*() const { return *t(); }

        /** Returns a pointer to the current item.
         *
         *  \sa operator*()
         */
        const T *operator->() const { return t(); }


        /** Returns \c true if \p other points to the same item as this iterator;
         *  otherwise returns \c false.
         *
         *  \sa operator!=()
         */
        bool operator==(const const_iterator &other) const
        {
            return m_querySet == other.m_querySet && m_offset == other.m_offset;
        }

        /** Returns \c true if \p other points to a different item than this iterator;
         *  otherwise returns \c false.
         *
         *  \sa operator==()
         */
        bool operator!=(const const_iterator &other) const
        {
            return m_querySet != other.m_querySet || m_offset != other.m_offset;
        }

        /** Returns \c true if other \p points to a position behind this iterator;
         *  otherwise returns \c false.
         */
        bool operator<(const const_iterator& other) const
        {
            return (m_querySet == other.m_querySet && m_offset < other.m_offset)
                    || m_querySet < other.m_querySet;
        }

        /** Returns \c true if other \p points to a position behind or equal this iterator;
         *  otherwise returns \c false.
         */
        bool operator<=(const const_iterator& other) const
        {
            return (m_querySet == other.m_querySet && m_offset <= other.m_offset)
                    || m_querySet < other.m_querySet;
        }

        /** Returns \c true if other \p points to a position before this iterator;
         *  otherwise returns \c false.
         */
        bool operator>(const const_iterator& other) const
        {
            return (m_querySet == other.m_querySet && m_offset > other.m_offset)
                    || m_querySet > other.m_querySet;
        }

        /** Returns \c true if other \p points to a position before or equal this iterator;
         *  otherwise returns \c false.
         */
        bool operator>=(const const_iterator& other) const
        {
            return (m_querySet == other.m_querySet && m_offset >= other.m_offset)
                    || m_querySet > other.m_querySet;
        }

        /** The prefix ++ operator (\c ++it) advances the iterator to the next item in the set
         *  and returns an iterator to the new current item.
         *
         *  Calling this function on QDjangoQuerySet::end() leads to undefined results.
         *
         *  \sa operator--()
         */
        const_iterator &operator++() { ++m_offset; return *this; }

        /** The postfix ++ operator (\c it++) advances the iterator to the next item in the set and
         *  returns an iterator to the previously current item.
         *
         *  Calling this function on QDjangoQuerySet::end() leads to undefined results.
         *
         *  \sa operator--(int)
         */
        const_iterator operator++(int) { const_iterator n(*this); ++m_offset; return n; }

        /** Advances the iterator by \p i items.
         * (If \p i is negative, the iterator goes backward.)
         *
         *  \sa operator-=() and operator+().
         */
        const_iterator &operator+=(int i) { m_offset += i; return *this; }

        /** Returns an iterator to the item at \p i positions forward from this iterator.
         * (If \p i is negative, the iterator goes backward.)
         *
         *  \sa operator-() and operator+=()
         */
        const_iterator operator+(int i) const { return const_iterator(m_querySet, m_offset + i); }

        /** Makes the iterator go back by \p i items.
         * (If \p i is negative, the iterator goes forward.)
         *
         * \sa operator+=() and operator-()
         */
        const_iterator &operator-=(int i) { m_offset -= i; return *this; }

        /** Returns an iterator to the item at \p i positions backward from this iterator.
         * (If \p i is negative, the iterator goes forward.)
         *
         *  \sa operator+() and operator-=()
         */
        const_iterator operator-(int i) const { return const_iterator(m_querySet, m_offset - i); }

        /** The prefix -- operator (\c --it) makes the preceding item current
         *  and returns an iterator to the new current item.
         *
         *  Calling this function on QDjangoQuerySet::begin() leads to undefined results.
         *
         *  \sa operator++().
         */
        const_iterator &operator--() { --m_offset; return *this; }

        /** The postfix -- operator (\c it--) makes the preceding item current
         *  and returns an iterator to the previously current item.
         *
         *  Calling this function on QDjangoQuerySet::begin() leads to undefined results.
         *
         *  \sa operator++(int).
         */
        const_iterator operator--(int) { const_iterator n(*this); --m_offset; return n; }


        /** Returns the number of items between the item pointed to by \p other
         *  and the item pointed to by this iterator.
         */
        difference_type operator-(const const_iterator &other) const { return m_offset - other.m_offset; }

    private:
        const T *t() const
        {
            if (m_fetched != m_offset && m_querySet) {
                if (const_cast<NOrmQuerySet<T> *>(m_querySet)->at(m_offset, &m_object)) {
                    m_fetched = m_offset;
                }
            }

            return m_fetched == m_offset ? &m_object : 0;
        }

    private:
        const NOrmQuerySet<T> *m_querySet;
        mutable int m_fetched;
        mutable T m_object;

        int m_offset;
    };

    /** Qt-style synonym for QDjangoQuerySet::const_iterator. */
    typedef const_iterator ConstIterator;

    NOrmQuerySet();
    NOrmQuerySet(const NOrmQuerySet<T> &other);
    ~NOrmQuerySet();

    NOrmQuerySet all() const;
    NOrmQuerySet exclude(const NOrmWhere &where) const;
    NOrmQuerySet filter(const NOrmWhere &where) const;
    NOrmQuerySet limit(int pos, int length = -1) const;
    NOrmQuerySet none() const;
    NOrmQuerySet orderBy(const QStringList &keys) const;
    NOrmQuerySet selectRelated(const QStringList &relatedFields = QStringList()) const;

    int count() const;
    QVariant aggregate(const NOrmWhere::AggregateType func, const QString& field) const;
    NOrmWhere where() const;

    bool remove();
    int size();
    int update(const QVariantMap &fields);
    QList<QVariantMap> values(const QStringList &fields = QStringList());
    QList<QVariantList> valuesList(const QStringList &fields = QStringList());

    T *get(const NOrmWhere &where, T *target = 0) const;
    T *at(int index, T *target = 0);

    const_iterator constBegin() const;
    const_iterator begin() const;

    const_iterator constEnd() const;
    const_iterator end() const;

    NOrmQuerySet<T> &operator=(const NOrmQuerySet<T> &other);

private:
    NOrmQuerySetPrivate *d;
};

/** Constructs a new queryset.
 */
template <class T>
NOrmQuerySet<T>::NOrmQuerySet()
{
    d = new NOrmQuerySetPrivate(T::staticMetaObject.className());
}

/** Constructs a copy of \a other.
 *
 * \param other
 */
template <class T>
NOrmQuerySet<T>::NOrmQuerySet(const NOrmQuerySet<T> &other)
{
    other.d->counter.ref();
    d = other.d;
}

/** Destroys the queryset.
 */
template <class T>
NOrmQuerySet<T>::~NOrmQuerySet()
{
    if (!d->counter.deref())
        delete d;
}

/** Returns the object in the QDjangoQuerySet at the given index.
 *
 *  Returns 0 if the index is out of bounds.
 *
 *  If target is 0, a new object instance will be allocated which
 *  you must free yourself.
 *
 * \param index
 * \param target optional existing model instance.
 */
template <class T>
T *NOrmQuerySet<T>::at(int index, T *target)
{
    T *entry = target ? target : new T;
    if (!d->sqlLoad(entry, index))
    {
        if (!target)
            delete entry;
        return 0;
    }
    return entry;
}

/** Returns a const STL-style iterator pointing to the first object in the QDjangoQuerySet.
 *
 *  \sa begin() and constEnd().
 */
template <class T>
typename NOrmQuerySet<T>::const_iterator NOrmQuerySet<T>::constBegin() const
{
    return const_iterator(this);
}

/** Returns a const STL-style iterator pointing to the first object in the QDjangoQuerySet.
 *
 *  \sa constBegin() and end().
 */
template <class T>
typename NOrmQuerySet<T>::const_iterator NOrmQuerySet<T>::begin() const
{
    return const_iterator(this);
}

/** Returns a const STL-style iterator pointing to the imaginary object after the last
 *  object in the QDjangoQuerySet.
 *
 *  \sa constBegin() and end().
 */
template <class T>
typename NOrmQuerySet<T>::const_iterator NOrmQuerySet<T>::constEnd() const
{
    return const_iterator(this, NOrmQuerySet<T>::count());
}

/** Returns a const STL-style iterator pointing to the imaginary object after the last
 *  object in the QDjangoQuerySet.
 *
 *  \sa begin() and constEnd().
 */
template <class T>
typename NOrmQuerySet<T>::const_iterator NOrmQuerySet<T>::end() const
{
    return const_iterator(this, NOrmQuerySet<T>::count());
}

/** Returns a copy of the current QDjangoQuerySet.
 */
template <class T>
NOrmQuerySet<T> NOrmQuerySet<T>::all() const
{
    NOrmQuerySet<T> other;
    other.d->lowMark = d->lowMark;
    other.d->highMark = d->highMark;
    other.d->orderBy = d->orderBy;
    other.d->selectRelated = d->selectRelated;
    other.d->relatedFields = d->relatedFields;
    other.d->whereClause = d->whereClause;
    return other;
}

/** Counts the number of objects in the queryset using an SQL COUNT query,
 *  or -1 if the query failed.
 *
 *  If you intend to iterate over the results, you should consider using
 *  size() instead.
 *
 * \note If the QDjangoQuerySet is already fully fetched, this simply returns
 *  the number of objects.
 */
template <class T>
int NOrmQuerySet<T>::count() const
{
    if (d->hasResults)
        return d->properties.size();

    QVariant count(aggregate(NOrmWhere::COUNT,"*"));
    return count.isValid() ? count.toInt() : -1;
}

/** Counts the objects in the queryset using an SQL [AVG, COUNT, SUM, MIN, MAX] query,
 *  or invalid QVariant if the query failed.
 *
 */
template <class T>
QVariant NOrmQuerySet<T>::aggregate(const NOrmWhere::AggregateType func, const QString& field) const
{
    // execute aggregate query
    NOrmQuery query(d->aggregateQuery(func, field));
    if (!query.exec() || !query.next())
        return QVariant();
    return query.value(0);
}

/** Returns a new QDjangoQuerySet containing objects for which the given key
 *  where condition is false.
 *
 *  You can chain calls to filter() and exclude() to further refine the
 *  filtering conditions.
 *
 * \param where QDjangoWhere expressing the exclude condition
 *
 * \sa filter()
 */
template <class T>
NOrmQuerySet<T> NOrmQuerySet<T>::exclude(const NOrmWhere &where) const
{
    NOrmQuerySet<T> other = all();
    other.d->addFilter(!where);
    return other;
}

/** Returns a new QDjangoQuerySet containing objects for which the given
 *  where condition is true.
 *
 *  You can chain calls to filter() and exclude() to progressively refine
 *  your filtering conditions.
 *
 * \param where QDjangoWhere expressing the filter condition
 *
 * \sa exclude()
 */
template <class T>
NOrmQuerySet<T> NOrmQuerySet<T>::filter(const NOrmWhere &where) const
{
    NOrmQuerySet<T> other = all();
    other.d->addFilter(where);
    return other;
}

/** Returns the object in the QDjangoQuerySet for which the given
 *  where condition is true.
 *
 *  Returns 0 if the number of matching object is not exactly one.
 *
 *  If target is 0, a new object instance will be allocated which
 *  you must free yourself.
 *
 * \param where QDjangoWhere expressing the lookup condition
 * \param target optional existing model instance.
 */
template <class T>
T *NOrmQuerySet<T>::get(const NOrmWhere &where, T *target) const
{
    NOrmQuerySet<T> qs = filter(where);
    return qs.size() == 1 ? qs.at(0, target) : 0;
}

/** Returns a new QDjangoQuerySet containing limiting the number of
 *  records to manipulate.
 *
 *  You can chain calls to limit() to further restrict the number
 *  of returned records.
 *
 *  However, you cannot apply additional restrictions using filter(),
 *  exclude(), get(), orderBy() or remove() on the returned QDjangoQuerySet.
 *
 * \param pos offset of the records
 * \param length maximum number of records
 */
template <class T>
NOrmQuerySet<T> NOrmQuerySet<T>::limit(int pos, int length) const
{
    Q_ASSERT(pos >= 0);
    Q_ASSERT(length >= -1);

    NOrmQuerySet<T> other = all();
    other.d->lowMark += pos;
    if (length > 0)
    {
        // calculate new high mark
        other.d->highMark = other.d->lowMark + length;
        // never exceed the current high mark
        if (d->highMark > 0 && other.d->highMark > d->highMark)
            other.d->highMark = d->highMark;
    }
    return other;
}

/** Returns an empty QDjangoQuerySet.
 */
template <class T>
NOrmQuerySet<T> NOrmQuerySet<T>::none() const
{
    NOrmQuerySet<T> other;
    other.d->whereClause = !NOrmWhere();
    return other;
}

/** Returns a QDjangoQuerySet whose elements are ordered using the given keys.
 *
 *  By default the elements will by in ascending order. You can prefix the key
 *  names with a "-" (minus sign) to use descending order.
 *
 * \param keys
 */
template <class T>
NOrmQuerySet<T> NOrmQuerySet<T>::orderBy(const QStringList &keys) const
{
    // it is not possible to change ordering once a limit has been set
    Q_ASSERT(!d->lowMark && !d->highMark);

    NOrmQuerySet<T> other = all();
    other.d->orderBy << keys;
    return other;
}

/** Deletes all objects in the QDjangoQuerySet.
 *
 * \return true if deletion succeeded, false otherwise
 */
template <class T>
bool NOrmQuerySet<T>::remove()
{
    return d->sqlDelete();
}

/** Returns a QDjangoQuerySet that will automatically "follow" foreign-key
 *  relationships, selecting that additional related-object data when it
 *  executes its query.
 *
 *  \param relatedFields If provided it will follow only the listed foreign
 *  keys. This is handy for very complex DB structures and allows the user
 *  to limit the amount of retrieved data. If omitted, the basic functionality
 *  is preserved and the function will traverse all foreign key relationships.
 *  Each list element is a chain of foreing keys separated by double underscore "__".
 */
template <class T>
NOrmQuerySet<T> NOrmQuerySet<T>::selectRelated(const QStringList &relatedFields) const
{
    NOrmQuerySet<T> other = all();
    other.d->selectRelated = true;
    other.d->relatedFields = relatedFields;
    return other;
}

/** Returns the number of objects in the QDjangoQuerySet, or -1
 *  if the query failed.
 *
 *  If you do not plan to access the objects, you should consider using
 *  count() instead.
 */
template <class T>
int NOrmQuerySet<T>::size()
{
    if (!d->sqlFetch())
        return -1;
    return d->properties.size();
}

/** Performs an SQL update query for the specified \a fields and returns the
 *  number of rows affected, or -1 if the update failed.
 */
template <class T>
int NOrmQuerySet<T>::update(const QVariantMap &fields)
{
    return d->sqlUpdate(fields);
}

/** Returns a list of property hashes for the current QDjangoQuerySet.
 *  If no \a fields are specified, all the model's declared fields are returned.
 *
 * \param fields
 */
template <class T>
QList<QVariantMap> NOrmQuerySet<T>::values(const QStringList &fields)
{
    return d->sqlValues(fields);
}

/** Returns a list of property lists for the current QDjangoQuerySet.
 *  If no \a fields are specified, all the model's fields are returned in the
 *  order they where declared.
 *
 * \param fields
 */
template <class T>
QList<QVariantList> NOrmQuerySet<T>::valuesList(const QStringList &fields)
{
    return d->sqlValuesList(fields);
}

/** Returns the QDjangoWhere expressing the WHERE clause of the
 * QDjangoQuerySet.
 */
template <class T>
NOrmWhere NOrmQuerySet<T>::where() const
{
    return d->resolvedWhere(NOrm::database());
}

/** Assigns the specified queryset to this object.
 *
 * \param other
 */
template <class T>
NOrmQuerySet<T> &NOrmQuerySet<T>::operator=(const NOrmQuerySet<T> &other)
{
    other.d->counter.ref();
    if (!d->counter.deref())
        delete d;
    d = other.d;
    return *this;
}

#endif
