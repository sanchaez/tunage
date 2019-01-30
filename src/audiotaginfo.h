#ifndef AUDIOTAGINFO_H
#define AUDIOTAGINFO_H

#include <QString>

struct AudioTagInfo
{
    QString song;
    QString album;
    QString artist;
    QString coverUrl;
    QString fileName;
    QString path;
    qint64 duration;

    AudioTagInfo() = default;
    explicit AudioTagInfo(const QString &path);
};

#endif // AUDIOTAGINFO_H
