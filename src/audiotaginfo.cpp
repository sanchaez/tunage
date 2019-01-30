#include "audiotaginfo.h"

#include "tag.h"
#include "fileref.h"
#include "audioproperties.h"

#include <QFile>
#include <QFileInfo>
#include <QUrl>

AudioTagInfo::AudioTagInfo(const QString& newPath)
    : duration(0)
{
    TagLib::FileRef f{ newPath.toUtf8().constData() };
    QFileInfo filePathInfo(newPath);

    path = newPath;
    fileName = filePathInfo.fileName();

    if(!f.isNull() && f.tag()) {
      TagLib::Tag *tag = f.tag();

      auto props = f.audioProperties();

      if(props) {
          duration = props->lengthInMilliseconds();
      }

      song = TStringToQString(tag->title());
      album = TStringToQString(tag->album());
      artist = TStringToQString(tag->artist());

      // use the cover.jpg as an URL if it exists
      QFile cover(filePathInfo.path() + "/cover.jpg");
      coverUrl = cover.exists() ? QUrl::fromLocalFile(cover.fileName()).toString() : "";
    }
}
