/*
 * Copyright (C) Kreogist Dev Team <kreogistdevteam@126.com>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the COPYING file for more details.
 */
#ifndef KNMUSICTREEVIEWBASE_H
#define KNMUSICTREEVIEWBASE_H

#include "knmusicglobal.h"

#include <QTreeView>

using namespace KNMusic;

class QTimeLine;
class KNConnectionHandler;
class KNMusicModel;
class KNMusicProxyModel;
class KNMusicTab;
class KNMusicTreeViewBase : public QTreeView
{
    Q_OBJECT
public:
    explicit KNMusicTreeViewBase(QWidget *parent = 0);
    KNMusicModel *musicModel();
    void setMusicModel(KNMusicModel *musicModel);
    void backupHeader();
    void enableSearchShortcut();
    void disableSearchShortcut();
    void scrollToSongRow(const int &row);
    inline void scrollToSongIndex(const QModelIndex &songIndex);
    void scrollToSourceSongRow(const int &row);
    KNMusicProxyModel *proxyModel();
    KNMusicTab *musicTab() const;
    void setMusicTab(KNMusicTab *musicTab);

signals:
    void searchComplete();

public slots:
    virtual void resetHeaderState();
    virtual void searchText(QString text);
    void sortMusicColumn(int column,
                         Qt::SortOrder order=Qt::AscendingOrder);

protected:
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    bool event(QEvent *event);
    void startDrag(Qt::DropActions supportedActions);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void moveToFirst(const int &logicalIndex);
    void drawRow(QPainter *painter,
                 const QStyleOptionViewItem &options,
                 const QModelIndex &index) const;
    void setAnimateState(bool on);

private slots:
    void onActionSearch();
    void onActionMouseInOut(const int &frame);
    void playIndex(const QModelIndex &index);
    void removeIndex(const QModelIndex &index);
    void removeSelections();

    void playCurrent();
    void removeCurrent();
    void renameCurrent(const QString &preferName);

private:
    inline void initialActions();
    inline void configureTimeLine(QTimeLine *timeLine);
    inline void showSoloMenu(const QPoint &position);
    inline void showMultiMenu(const QPoint &position);
    QAction *m_findAction;
    QTimeLine *m_mouseIn, *m_mouseOut;
    KNMusicProxyModel *m_proxyModel=nullptr;
    KNConnectionHandler *m_soloConnections,
                        *m_multiConnections;
    QByteArray m_headerState;
    QColor m_alternateColor=QColor(255,255,255,0),
           m_fontColor=QColor(255,255,255),
           m_buttonColor=QColor(255,255,255);
    QString m_seachText;
    KNMusicTab *m_musicTab=nullptr;
    KNMusicGlobal *m_musicGlobal;
    int m_maxOpacity=0x30,
    m_fontBase=0x9f,
    m_buttonBase=0x10;
    bool m_pressed=false, m_initialLoad=true, m_animate=true;
};

#endif // KNMUSICTREEVIEWBASE_H
