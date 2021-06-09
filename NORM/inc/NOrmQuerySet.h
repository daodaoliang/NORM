#ifndef NORM_QUERYSET_H
#define NORM_QUERYSET_H

#include "NOrm.h"
#include "NOrmWhere.h"
#include "NOrmQuerySet_p.h"

template <class T> class NOrmQuerySet {
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

    class const_iterator {
        friend class NOrmQuerySet;

    public:
        /** A synonym for std::bidirectional_iterator_tag indicating this iterator
         *  permits bidirectional access.
         */
        typedef std::bidirectional_iterator_tag iterator_category;

        /** \cond declarations for STL-style container algorithms */
        typedef qptrdiff difference_type;
        typedef T value_type;
        typedef T *pointer;
        typedef T &reference;
        /** \endcond */

        const_iterator() : m_querySet(0), m_fetched(-1), m_offset(0) {}

        /** Constructs a copy of \p other.
         */
        const_iterator(const const_iterator &other)
            : m_querySet(other.m_querySet), m_fetched(-1), m_offset(other.m_offset) {}

    private:
        const_iterator(const NOrmQuerySet<T> *querySet, int offset = 0)
            : m_querySet(querySet), m_fetched(-1), m_offset(offset) {}

    public:
        const T &operator*() const { return *t(); }

        const T *operator->() const { return t(); }

        bool operator==(const const_iterator &other) const {
            return m_querySet == other.m_querySet && m_offset == other.m_offset;
        }

        bool operator!=(const const_iterator &other) const {
            return m_querySet != other.m_querySet || m_offset != other.m_offset;
        }

        bool operator<(const const_iterator &other) const {
            return (m_querySet == other.m_querySet && m_offset < other.m_offset) || m_querySet < other.m_querySet;
        }

        bool operator<=(const const_iterator &other) const {
            return (m_querySet == other.m_querySet && m_offset <= other.m_offset) || m_querySet < other.m_querySet;
        }

        bool operator>(const const_iterator &other) const {
            return (m_querySet == other.m_querySet && m_offset > other.m_offset) || m_querySet > other.m_querySet;
        }

        bool operator>=(const const_iterator &other) const {
            return (m_querySet == other.m_querySet && m_offset >= other.m_offset) || m_querySet > other.m_querySet;
        }

        const_iterator &operator++() {
            ++m_offset;
            return *this;
        }
        const_iterator operator++(int) {
            const_iterator n(*this);
            ++m_offset;
            return n;
        }
        const_iterator &operator+=(int i) {
            m_offset += i;
            return *this;
        }
        const_iterator operator+(int i) const { return const_iterator(m_querySet, m_offset + i); }
        const_iterator &operator-=(int i) {
            m_offset -= i;
            return *this;
        }
        const_iterator operator-(int i) const { return const_iterator(m_querySet, m_offset - i); }
        const_iterator &operator--() {
            --m_offset;
            return *this;
        }
        const_iterator operator--(int) {
            const_iterator n(*this);
            --m_offset;
            return n;
        }
        difference_type operator-(const const_iterator &other) const { return m_offset - other.m_offset; }

    private:
        const T *t() const {
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
    QVariant aggregate(const NOrmWhere::AggregateType func, const QString &field) const;
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
template <class T> NOrmQuerySet<T>::NOrmQuerySet() { d = new NOrmQuerySetPrivate(T::staticMetaObject.className()); }

/** Constructs a copy of \a other.
 *
 * \param other
 */
template <class T> NOrmQuerySet<T>::NOrmQuerySet(const NOrmQuerySet<T> &other) {
    other.d->counter.ref();
    d = other.d;
}

/** Destroys the queryset.
 */
template <class T> NOrmQuerySet<T>::~NOrmQuerySet() {
    if (!d->counter.deref())
        delete d;
}

template <class T> T *NOrmQuerySet<T>::at(int index, T *target) {
    T *entry = target ? target : new T;
    if (!d->sqlLoad(entry, index)) {
        if (!target)
            delete entry;
        return 0;
    }
    return entry;
}

template <class T> typename NOrmQuerySet<T>::const_iterator NOrmQuerySet<T>::constBegin() const {
    return const_iterator(this);
}

template <class T> typename NOrmQuerySet<T>::const_iterator NOrmQuerySet<T>::begin() const {
    return const_iterator(this);
}

template <class T> typename NOrmQuerySet<T>::const_iterator NOrmQuerySet<T>::constEnd() const {
    return const_iterator(this, NOrmQuerySet<T>::count());
}

template <class T> typename NOrmQuerySet<T>::const_iterator NOrmQuerySet<T>::end() const {
    return const_iterator(this, NOrmQuerySet<T>::count());
}

template <class T> NOrmQuerySet<T> NOrmQuerySet<T>::all() const {
    NOrmQuerySet<T> other;
    other.d->lowMark = d->lowMark;
    other.d->highMark = d->highMark;
    other.d->orderBy = d->orderBy;
    other.d->selectRelated = d->selectRelated;
    other.d->relatedFields = d->relatedFields;
    other.d->whereClause = d->whereClause;
    return other;
}

template <class T> int NOrmQuerySet<T>::count() const {
    if (d->hasResults)
        return d->properties.size();

    QVariant count(aggregate(NOrmWhere::COUNT, "*"));
    return count.isValid() ? count.toInt() : -1;
}

template <class T>
QVariant NOrmQuerySet<T>::aggregate(const NOrmWhere::AggregateType func, const QString &field) const {
    // execute aggregate query
    NOrmQuery query(d->aggregateQuery(func, field));
    if (!query.exec() || !query.next())
        return QVariant();
    return query.value(0);
}

template <class T> NOrmQuerySet<T> NOrmQuerySet<T>::exclude(const NOrmWhere &where) const {
    NOrmQuerySet<T> other = all();
    other.d->addFilter(!where);
    return other;
}

template <class T> NOrmQuerySet<T> NOrmQuerySet<T>::filter(const NOrmWhere &where) const {
    NOrmQuerySet<T> other = all();
    other.d->addFilter(where);
    return other;
}

template <class T> T *NOrmQuerySet<T>::get(const NOrmWhere &where, T *target) const {
    NOrmQuerySet<T> qs = filter(where);
    return qs.size() == 1 ? qs.at(0, target) : 0;
}

template <class T> NOrmQuerySet<T> NOrmQuerySet<T>::limit(int pos, int length) const {
    Q_ASSERT(pos >= 0);
    Q_ASSERT(length >= -1);

    NOrmQuerySet<T> other = all();
    other.d->lowMark += pos;
    if (length > 0) {
        // calculate new high mark
        other.d->highMark = other.d->lowMark + length;
        // never exceed the current high mark
        if (d->highMark > 0 && other.d->highMark > d->highMark)
            other.d->highMark = d->highMark;
    }
    return other;
}

template <class T> NOrmQuerySet<T> NOrmQuerySet<T>::none() const {
    NOrmQuerySet<T> other;
    other.d->whereClause = !NOrmWhere();
    return other;
}

template <class T> NOrmQuerySet<T> NOrmQuerySet<T>::orderBy(const QStringList &keys) const {
    // it is not possible to change ordering once a limit has been set
    Q_ASSERT(!d->lowMark && !d->highMark);

    NOrmQuerySet<T> other = all();
    other.d->orderBy << keys;
    return other;
}

template <class T> bool NOrmQuerySet<T>::remove() { return d->sqlDelete(); }

template <class T> NOrmQuerySet<T> NOrmQuerySet<T>::selectRelated(const QStringList &relatedFields) const {
    NOrmQuerySet<T> other = all();
    other.d->selectRelated = true;
    other.d->relatedFields = relatedFields;
    return other;
}

template <class T> int NOrmQuerySet<T>::size() {
    if (!d->sqlFetch())
        return -1;
    return d->properties.size();
}

template <class T> int NOrmQuerySet<T>::update(const QVariantMap &fields) { return d->sqlUpdate(fields); }

template <class T> QList<QVariantMap> NOrmQuerySet<T>::values(const QStringList &fields) {
    return d->sqlValues(fields);
}

template <class T> QList<QVariantList> NOrmQuerySet<T>::valuesList(const QStringList &fields) {
    return d->sqlValuesList(fields);
}

template <class T> NOrmWhere NOrmQuerySet<T>::where() const { return d->resolvedWhere(NOrm::database()); }

template <class T> NOrmQuerySet<T> &NOrmQuerySet<T>::operator=(const NOrmQuerySet<T> &other) {
    other.d->counter.ref();
    if (!d->counter.deref())
        delete d;
    d = other.d;
    return *this;
}

#endif
