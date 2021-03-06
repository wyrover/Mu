/*
 * Copyright (C) Kreogist Dev Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef KNMUSICPLAYLISTLOADER_H
#define KNMUSICPLAYLISTLOADER_H

#include <QList>

//Ports.
#include "knmusicplaylistparser.h"

#include "knmusicglobal.h"

#include <QObject>

class KNMusicPlaylistListItem;
class KNMusicPlaylistLoader : public QObject
{
    Q_OBJECT
public:
    explicit KNMusicPlaylistLoader(QObject *parent = 0);
    ~KNMusicPlaylistLoader();
    void installPlaylistParser(KNMusicPlaylistParser *parser);
    bool parsePlaylist(const QString &filePath,
                       KNMusicPlaylistListItem *playlistItem);
    void getPlaylistTypeAndSuffix(QStringList &types,
                                  QStringList &suffixs);
    bool writePlaylist(const QString &filePath,
                       const QString &suffix,
                       KNMusicPlaylistListItem *playlistItem);

signals:

public slots:

private:
    QList<KNMusicPlaylistParser *> m_parsers;
};

#endif // KNMUSICPLAYLISTLOADER_H
