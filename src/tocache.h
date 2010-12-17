
#ifndef TOCACHE_H
#define TOCACHE_H

#include "tothread.h"
#include <QMetaType>
#include <QDate>
#include <QList>
#include <QVariant>
#include <map>
#include <list>
#include <set>

/** Object cache for a connection.
 */
class toCache : public QObject
{
    /** Description of connection for which this instance of cache is used.
     *  This name is used as a filename to store cache content between TOra runs.
     */
    QString ConnectionDescription;
    bool ReadingCache;

public:
    typedef QList<QVariant> Row; // NOTE: first (0th) value in a row is a row number (see variable currRowKey)
    typedef QList<Row> RowList;

    toSemaphore ReadingValues;

    /** Contain information about a tablename.
     */
    struct objectName
    {
        /** The object name
         */
        QString Name;
        /** The schema that owns it
         */
        QString Owner;
        /** Object type
         */
        QString Type;
        /** Comment about this object
         */
        QString Comment;
        /** synonyms (used for faster disk caching...)
         */
        std::list <QString> Synonyms;

        /** A date when information about this particular object was last updated
         */
        QDate Timestamp;

        /** Create an object name with filled in values.
         */
        objectName(const QString &owner, const QString &name, const QString &type = QString("TABLE"), const QString &comment = QString::null)
            : Name(name), Owner(owner), Type(type), Comment(comment)
        { }

        /** Create an empty object name.
         */
        objectName()
        { }
        bool operator < (const objectName &) const;
        bool operator == (const objectName &) const;
    };

    /** This structure is used to describe the resultset of a query.
     */
    struct queryDescribe
    {
        /** Column name
         */
        QString Name;
        /** Datatype of string.
         */
        QString Datatype;
        /** If column can contain null values.
         */
        bool Null;
        /** Preferred alignment of this kind of value.
         */
        bool AlignRight;
        /** Comment on column (Only filled out in column cache.
         */
        QString Comment;
    };

private:
    std::list<objectName> ObjectNames;
    std::map<QString, objectName> SynonymMap;
    void ll(void);

public:
    typedef queryDescribe toQDescribe;
    typedef std::list<toQDescribe> toQDescList;
    std::map<objectName, toQDescList> ColumnCache;

    toCache(QString description);
    ~toCache();

    /** Load cache information for current connection from a file on disk
     * @return True if cache was loaded
     */
    bool loadDiskCache(void);

    /** write disk cache
     */
    void writeDiskCache(void);

    /** Add a new object to the objectlist if it doesn't exist already.
     * @param object The object to add
     */
    void addIfNotExists(objectName &object);

    /** Check if cache is available or not.
     * @param synonyms If synonyms are needed or not.
     * @param block Block until cache is done.
     * @param true True if you need the cache, or just checking.
     * @return True if cache is available.
     */
    bool cacheAvailable(bool synonyms, bool block = false, bool need = true, toTask * t = NULL);

    /** Return the file used to store cache contents for this connection.
     * @return A string representing a full path and filename of cache file
     */
    QString cacheFile();

    /** Return the directory storing files (caches) of all connections.
     * @return A string representing a full path to cache store directory
     */
    static QString cacheDir();

    /**
     * Get the real object name of a synonym.
     * @param object Object name
     * @param synonym Filled with the synonym used to access the object returned or empty.
     * @param block Block if not done caching object.
     */
    const objectName &realName(const QString &object, QString &synonym, bool block, QString user, QString database);

    /**
     * Get the synonyms available for objects. Do not modify the returned list.
     * @param block Indicate wether or not to block until cached objects are available.
     * @return A list of synonyms to objects available for the current user.
     */
    std::map<QString, objectName> &synonyms(bool block);

    void readObjects(toTask * t);

    /**
     * Get the objects available for the current user. Do not modify the returned list.
     * @param block Indicate wether or not to block until cached objects are available.
     * @return A list of object available for the current user. The list is sorted in
     *         owner and name order.
     */
    std::list<objectName> &objects(bool block);

    toQDescList &columns(const objectName &table);

    void addColumns(objectName object, toQDescList list);

    /**
     * Get a list of object names for a owner, typically this is a
     * list of tables for a database or schema.
     *
     */
    std::list<objectName> tables(const objectName &object, bool nocache = false);

    /**
     * Reread the object and column cache.
     */
    void rereadCache(toTask * t);

    /** Set a list of objects. This is used when list of objects is loaded in bulk
     * outside of a cache (for example in connection class). Note that this fucntion
     * will remove any existing objects in cache (for this connection).
     * @param list list of objects to be set
     */
    void setObjectList(std::list<objectName> &list);

    /** Set a list of object synonyms. This is used when list of synonyms is loaded
     * in bulk outside of a cache (for example in connection class). Note that this
     * fucntion will remove any existing synonyms.
     * @param list list of objects to be set
     */
    void setSynonymList(std::map<QString, objectName> &list);

    /** Checks if given object is saved in the cache.
     */
    bool objectExists(const QString &owner, const QString &type, const QString &name);

    /** Returns a list of objects of particular type and schema/owner
     */
    RowList getObjects(const QString &owner, const QString &type);

    /** Update a list of objects of particular type and schema owner in cache.
     * This should add any new object to the list as well as remove no longer
     * existing ones.
     */
    void updateObjects(const QString &owner, const QString &type, const QList<toCache::objectName> rows);
}; // toCache

/** A short representation of a @ref toCache::queryDescribe
 */
typedef toCache::queryDescribe toQDescribe;

/** A short representation of list<toCache::queryDescribe>
 */
typedef std::list<toQDescribe> toQDescList;
Q_DECLARE_METATYPE(toQDescList);

#endif
