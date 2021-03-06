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
#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTimeLine>
#include <QtMath>

#include "knmusiccategoryproxymodel.h"
#include "knmusicalbummodel.h"
#include "knmusicalbumdetail.h"

#include "knmusicalbumview.h"

#include <QDebug>

KNMusicAlbumView::KNMusicAlbumView(QWidget *parent) :
    QAbstractItemView(parent)
{
    //Set properties.
    setFocusPolicy(Qt::WheelFocus);

    //Set default scrollbar properties.
    verticalScrollBar()->setRange(0, 0);

    //Set the palette.
    m_backgroundColor.setHsv(m_backgroundColor.hue(),
                             m_backgroundColor.saturation(),
                             m_outBrightness);
    QPalette pal=palette();
    pal.setColor(QPalette::Base, m_backgroundColor);
    pal.setColor(QPalette::Window, m_backgroundColor);
    pal.setColor(QPalette::Button, QColor(0x30, 0x30, 0x30));
    pal.setColor(QPalette::ButtonText, QColor(0xff, 0xff, 0xff));
    pal.setColor(QPalette::Text, QColor(0xff, 0xff, 0xff));
    pal.setColor(QPalette::Highlight, QColor(0x60, 0x60, 0x60));
    pal.setColor(QPalette::HighlightedText, QColor(0xf7, 0xcf, 0x3d));
    setPalette(pal);

    //Initial the shadow source.
    m_shadowSource=QPixmap("://public/shadow.png");

    //Initial the timeline.
    m_scrollTimeLine=new QTimeLine(200, this);
    m_scrollTimeLine->setUpdateInterval(5);
    m_scrollTimeLine->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_scrollTimeLine, &QTimeLine::frameChanged,
            verticalScrollBar(), &QScrollBar::setValue);

    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            this, &KNMusicAlbumView::onActionScrolling);

    //Update parameters.
    updateParameters();
}

QModelIndex KNMusicAlbumView::indexAt(const QPoint &point) const
{
    //Calculate the point content position and the line of the point.
    int pointContentY=verticalScrollBar()->value()+point.y(),
        itemLine=pointContentY/m_itemSpacingHeight;
    //Check whether mouse click on a row spacing part.
    if(pointContentY-itemLine*m_itemSpacingHeight<m_spacing)
    {
        return QModelIndex();
    }
    //Calculate the column of the position.
    int itemColumn=point.x()/m_itemSpacingWidth,
        pointXInItem=point.x()-itemColumn*m_itemSpacingWidth;
    //Check whether mouse click on a column spacing part.
    if(pointXInItem<m_spacing || pointXInItem>m_spacing+m_itemIconSize)
    {
        return QModelIndex();
    }
    //Calculate the item category index.
    int categoryRow=itemLine*m_maxColumnCount+itemColumn;
    //Check if the category row vaild.
    //We should check the category proxy model, because the point is a display
    //variable.
    return (categoryRow>-1 && categoryRow<m_proxyModel->rowCount())?
                m_proxyModel->index(categoryRow, 0, rootIndex()):
                QModelIndex();
}

void KNMusicAlbumView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    //Check the index and the max column count.
    if(!index.isValid() || m_maxColumnCount==0)
    {
        return;
    }
    //Check whether we need to move the vertical scroll bar.
    if(hint==QAbstractItemView::EnsureVisible &&
            rect().contains(visualRect(index), true))
    {
        return;
    }
    //Use timeline to move to the position.
    m_scrollTimeLine->stop();
    m_scrollTimeLine->setFrameRange(verticalScrollBar()->value(),
                                    indexScrollBarValue(index, hint));
    m_scrollTimeLine->start();
    //Update.
    viewport()->update();
}

void KNMusicAlbumView::locateTo(const QModelIndex &index,
                                QAbstractItemView::ScrollHint hint)
{
    //Check the index and the max column count.
    if(!index.isValid() || m_maxColumnCount==0)
    {
        return;
    }
    //Check whether we need to move the vertical scroll bar.
    if(hint==QAbstractItemView::EnsureVisible &&
            rect().contains(visualRect(index), true))
    {
        return;
    }
    //Use timeline to move to the position.
    verticalScrollBar()->setValue(indexScrollBarValue(index, hint));
    //Update.
    viewport()->update();
}

QRect KNMusicAlbumView::visualRect(const QModelIndex &index) const
{
    //Get the item content rect.
    QRect indexRect=itemContentRect(index);
    //If the rect is vaild, remove the value of the scroll bar.
    return indexRect.isValid()?
              QRect(indexRect.left()-horizontalScrollBar()->value(),
                    indexRect.top()-verticalScrollBar()->value(),
                    indexRect.width(),
                    indexRect.height()):
              QRect();
}

void KNMusicAlbumView::setModel(QAbstractItemModel *model)
{
    //Save the proxy model and the category model.
    m_proxyModel=static_cast<KNMusicCategoryProxyModel *>(model);
    m_model=static_cast<KNMusicAlbumModel *>(m_proxyModel->sourceModel());
    //Set the model.
    QAbstractItemView::setModel(m_proxyModel);
    //Update the geometries.
    updateGeometries();
}

void KNMusicAlbumView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    //Initial the painter.
    QPainter painter(viewport());
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::TextAntialiasing |
                           QPainter::SmoothPixmapTransform,
                           true);
    //Get the option view item.
    QStyleOptionViewItem option=viewOptions();
    //Check if we need to paint the background.
    if(autoFillBackground())
    {
        painter.fillRect(rect(), option.palette.base());
    }

    //Update the parameters of the view first.
    updateParameters();
    //Check the model first.
    if(m_proxyModel==nullptr)
    {
        return;
    }
    //Get the row count.
    int albumCount=m_proxyModel->rowCount();
    //Calculate the line count.
    m_lineCount=(albumCount+m_maxColumnCount-1)/m_maxColumnCount;
    //Check the album count first.
    if(albumCount==0)
    {
        return;
    }
    int currentColumn=0,
        currentLeft=m_spacing,
        currentLine=verticalScrollBar()->value()/m_itemSpacingHeight, //Skip the before.
        currentTop=m_spacing+currentLine*m_itemSpacingHeight,
        currentRow=currentLine*m_maxColumnCount,
        heightSurplus=height()+m_itemSpacingHeight;
    //Change the origin of coordinate.
    painter.translate(0, -verticalScrollBar()->value());
    //Draw all the albums.
    while(currentRow < albumCount && heightSurplus > 0)
    {
        //Get the source index of the item.
        QModelIndex proxyIndex=m_proxyModel->index(currentRow, 0);
        //If the source index is not the current index, then draw the album.
        if(m_proxyModel->mapToSource(proxyIndex)!=m_selectedIndex)
        {
            paintAlbum(painter,
                       QPoint(currentLeft, currentTop),
                       proxyIndex);
        }
        //Add current row and column.
        currentRow++;
        currentColumn++;
        //Check if we need to move to next row.
        if(currentColumn>=m_maxColumnCount)
        {
            //Add one line.
            currentLine++;
            //Reset the column.
            currentColumn=0;
            //Change the position.
            currentLeft=m_spacing;
            currentTop+=m_itemSpacingHeight;
            heightSurplus-=m_itemSpacingHeight;
        }
        else
        {
            //Move to next column position.
            currentLeft+=m_itemSpacingWidth;
        }
    }
    //Update the scroll bar value.
    updateGeometries();
}

void KNMusicAlbumView::resizeEvent(QResizeEvent *event)
{
    //Do resize.
    QAbstractItemView::resizeEvent(event);
    //Resize the album detail.
    m_albumDetail->resize(size());
    m_albumDetail->setSizeParameter(qMin(width(), height()));
    //If the current index is not null, must ensure that we can display the
    //selected album.
    if(currentIndex().isValid())
    {
        locateTo(currentIndex());
    }
}

QModelIndex KNMusicAlbumView::moveCursor(QAbstractItemView::CursorAction cursorAction,
                                         Qt::KeyboardModifiers modifiers)
{
    QModelIndex current=currentIndex(), movedIndex;
    switch (cursorAction)
    {
    case QAbstractItemView::MoveUp:
        movedIndex=m_proxyModel->index(current.row()-m_maxColumnCount, 0);
        break;
    case QAbstractItemView::MoveDown:
        movedIndex=m_proxyModel->index(current.row()+m_maxColumnCount, 0);
        break;
    case QAbstractItemView::MoveLeft:
        movedIndex=m_proxyModel->index(current.row()-1, 0);
        break;
    case QAbstractItemView::MoveRight:
        movedIndex=m_proxyModel->index(current.row()+1, 0);
        break;
    case QAbstractItemView::MoveHome:
        movedIndex=m_proxyModel->index(0, 0);
        break;
    case QAbstractItemView::MoveEnd:
        movedIndex=m_proxyModel->index(m_model->rowCount()-1, 0);
        break;
    default:
        break;
    }
    viewport()->update();
    return current;
}

int KNMusicAlbumView::horizontalOffset() const
{
    return horizontalScrollBar()->value();
}

int KNMusicAlbumView::verticalOffset() const
{
    return verticalScrollBar()->value();
}

bool KNMusicAlbumView::isIndexHidden(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return false;
}

void KNMusicAlbumView::setSelection(const QRect &rect,
                                    QItemSelectionModel::SelectionFlags command)
{
    selectionModel()->select(indexAt(QPoint(rect.x(),rect.y())),
                             command);
}

QRegion KNMusicAlbumView::visualRegionForSelection(const QItemSelection &selection) const
{
    //If there's no selection, of course nothing.
    if(selection.size()==0)
    {
        return QRect();
    }
    //Add the only select item to the region, return the region.
    QItemSelectionRange range=selection.at(0);
    QRegion region;
    region+=visualRect(indexAt(QPoint(range.top(), range.left())));
    return region;
}

void KNMusicAlbumView::mousePressEvent(QMouseEvent *event)
{
    QAbstractItemView::mousePressEvent(event);
    //Get the mouse down index.
    m_mouseDownIndex=indexAt(event->pos());
}

void KNMusicAlbumView::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractItemView::mouseReleaseEvent(event);
    //Check whether the released pos index is the pressed index.
    if(m_mouseDownIndex==indexAt(event->pos()))
    {
        //Check the mouse pressed button is left button or not.
        if(event->button()==Qt::LeftButton)
        {
            displayAlbum(event->pos());
            return;
        }
    }
    //If goes here, we need to fold the expanded album.
    displayAlbum(QPoint(-1,-1));
}

void KNMusicAlbumView::updateGeometries()
{
    //Update the range of the vertical scroll bar.
    verticalScrollBar()->setRange(0,
                                  qMax(0,
                                       m_lineCount*m_itemSpacingHeight+m_spacing-height()));
    //Update the page and single step.
    verticalScrollBar()->setPageStep(m_itemSpacingHeight>>1);
    verticalScrollBar()->setSingleStep(m_itemSpacingHeight>>1);
}

void KNMusicAlbumView::displayAlbum(const QPoint &point)
{
    QModelIndex proxyIndex=indexAt(point),
                sourceIndex=m_proxyModel->mapToSource(proxyIndex);
    if(proxyIndex.isValid() && m_selectedIndex!=sourceIndex)
    {
        //Select the album.
        selectAlbum(sourceIndex);
        //Scroll to the album.
        scrollTo(proxyIndex);
    }
    else
    {
        //Show the detail.
        m_albumDetail->setAnimeParameter(visualRect(m_proxyModel->mapFromSource(m_selectedIndex)),
                                         m_itemIconSize);
        //Fold the album.
        m_albumDetail->foldAlbumDetail();
        //Update the viewport.
        viewport()->update();
    }
}

void KNMusicAlbumView::clearSelection()
{
    //When complete the fold, set the selected index to null.
    setCurrentIndex(QModelIndex());
    m_selectedIndex=QModelIndex();
    //Update the viewport.
    viewport()->update();
}

void KNMusicAlbumView::onActionScrolling()
{
    m_albumDetail->updateFoldEndValue(visualRect(m_proxyModel->mapFromSource(m_selectedIndex)),
                                      m_itemIconSize);
}

void KNMusicAlbumView::selectAlbum(QModelIndex albumIndex)
{
    //If the index is vaild, set the initial animation parameters.
    if(albumIndex.isValid())
    {
        //Set current index to the proxy index of the album index.
        setCurrentIndex(m_proxyModel->mapFromSource(albumIndex));
        //Set the selected index.
        m_selectedIndex=albumIndex;
        //Show the detail.
        m_albumDetail->setAnimeParameter(visualRect(m_proxyModel->mapFromSource(m_selectedIndex)),
                                         m_itemIconSize);
        m_albumDetail->displayAlbumDetail(m_selectedIndex);
        //Update the album view.
        viewport()->update();
    }
    else
    {
        //Show the detail.
        m_albumDetail->setAnimeParameter(visualRect(m_proxyModel->mapFromSource(m_selectedIndex)),
                                         m_itemIconSize);
        //Do fold detail animation.
        m_albumDetail->foldAlbumDetail();
        //Update the viewport.
        update();
        viewport()->update();
    }
}

inline void KNMusicAlbumView::paintAlbum(QPainter &painter,
                                         const QPoint &position,
                                         const QModelIndex &index)
{
    //Ensure the index is vaild.
    if(!index.isValid())
    {
        return;
    }
    //Draw the shadow first.
    painter.drawPixmap(position.x()-m_shadowIncrease,
                       position.y()-m_shadowIncrease,
                       m_albumArtShadow);
    //Draw the album art first.
    QIcon currentIcon=
            m_proxyModel->data(index, Qt::DecorationRole).value<QIcon>();
    QPixmap albumArtPixmap=currentIcon.pixmap(m_itemIconSize, m_itemIconSize);
    painter.drawPixmap(QRect(position.x(),
                             position.y(),
                             m_itemIconSize-2,
                             m_itemIconSize-2),
                        albumArtPixmap);
    //Get the option view item.
    QStyleOptionViewItem option=viewOptions();
    //Set the pen as the text color.
    QColor textColor=option.palette.color(QPalette::Text);
    painter.setPen(textColor);
    //Draw the album text.
    //Draw the album name first.
    QRect textRect(position.x(),
                   position.y()+m_itemIconSize+m_imageTextSpacing,
                   m_itemIconSize,
                   fontMetrics().height());
    painter.drawText(textRect,
                     Qt::AlignLeft,
                     fontMetrics().elidedText(index.data(Qt::DisplayRole).toString(),
                                              Qt::ElideRight,
                                              m_itemIconSize));
    //Draw the album artist name.
    //Get the album list.
    QHash<QString, QVariant> artistList=index.data(CategoryArtistList).toHash();
    if(!artistList.isEmpty())
    {
        //Set color.
        textColor.setAlpha(textColor.alpha()>>1);
        painter.setPen(textColor);
        //Draw the text.
        painter.drawText(QRect(textRect.x(),
                               textRect.y()+fontMetrics().height(),
                               m_itemIconSize,
                               fontMetrics().height()),
                         Qt::AlignLeft,
                         fontMetrics().elidedText(artistList.size()==1?
                                                      artistList.keys().first():
                                                      tr("Various Artists"),
                                                  Qt::ElideRight,
                                                  m_itemIconSize));
    }
}

inline int KNMusicAlbumView::indexScrollBarValue(const QModelIndex &index,
                                                 QAbstractItemView::ScrollHint hint)
{
    //Get the top of index position, set it to scroll value.
    int topPositionValue=index.row()/m_maxColumnCount*m_itemSpacingHeight;
    //Change the item content Y according to the hint.
    switch(hint)
    {
    case QAbstractItemView::PositionAtTop:
        //No need to change.
        return topPositionValue;
    case QAbstractItemView::PositionAtCenter:
        //Reduce a half of the viewer height to move up.
        return topPositionValue-((height()-m_itemSpacingHeight)>>1);
    case QAbstractItemView::PositionAtBottom:
        //Reduce the whole viewer height to move up.
        return topPositionValue-height()+m_itemSpacingHeight;
        break;
    default:
        //Now, the index item must be a unvisible one in the viewer.
        //We have already has the top position, calculate the bottom position,
        //and calculate the distence of the current vertical scroll bar's value
        //to these two position.
        int bottomPositionValue=topPositionValue-height()+m_itemSpacingHeight;
        //If to the bottom is lesser than to top, change the value.
        return (qAbs(verticalScrollBar()->value()-bottomPositionValue)<
                qAbs(verticalScrollBar()->value()-topPositionValue))?
                    bottomPositionValue:
                    topPositionValue;
    }
}

inline QRect KNMusicAlbumView::itemContentRect(const QModelIndex &index) const
{
    //Check the index first.
    if(!index.isValid() || m_maxColumnCount==0)
    {
        return QRect();
    }
    //Calculate the item line.
    int itemIndex=index.row(),
        itemLine=itemIndex/m_maxColumnCount;
    //Calculate the rect.
    return QRect((itemIndex-itemLine*m_maxColumnCount)*m_itemSpacingWidth+m_spacing,
                 itemLine*m_itemSpacingHeight+m_spacing,
                 m_itemWidth,
                 m_itemHeight);
}

inline void KNMusicAlbumView::updateParameters()
{
    //Get the usable width.
    int usableWidth=width()-m_spacing,
        minimalWidth=m_itemMinimalWidth+m_spacing;
    //When the usable width is lesser than the minimal width,
    //Force set the max column count to 1.
    if(usableWidth<minimalWidth)
    {
        m_maxColumnCount=1;
        m_itemWidth=width();
    }
    else
    {
        //Calculate how many column we can put in one row.
        m_maxColumnCount=usableWidth/minimalWidth;
        //Calculate the real item width, it's must be larger than minimal width.
        m_itemWidth=usableWidth/m_maxColumnCount-m_spacing;
    }
    //The icon size should be the item width.
    m_itemIconSize=m_itemWidth;
    //Generate the album art shadow pixmap.
    int shadowSize=m_itemIconSize+(m_shadowIncrease<<1);
    m_albumArtShadow=generateShadow(shadowSize, shadowSize);
    //The height of the item should be a item icon size and two text lines.
    m_itemHeight=m_itemIconSize+(fontMetrics().height()<<1);
    //Calcualte the spacing item width and height.
    m_itemSpacingHeight=m_spacing+m_itemHeight;
    m_itemSpacingWidth=m_spacing+m_itemWidth;
}

inline QPixmap KNMusicAlbumView::generateShadow(int shadowWidth, int shadowHeight)
{
    //Initial the shadow pixmap.
    QPixmap shadowPixmap(shadowWidth, shadowHeight);
    shadowPixmap.fill(QColor(255, 255, 255, 0));
    //Prepare the parameters.
    int sourceSize=m_shadowSource.width(),
        blockSize=sourceSize/3,
        blockSize2x=blockSize<<1,
        destinationWidth=shadowWidth-blockSize2x,
        destinationHeight=shadowHeight-blockSize2x;
    //Initial the antialiasing painter.
    QPainter painter(&shadowPixmap);
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::SmoothPixmapTransform,
                           true);
    painter.setOpacity(0.7);
    //Draw Top-left shadow.
    painter.drawPixmap(QRect(0,0,blockSize,blockSize),
                       m_shadowSource,
                       QRect(0,0,blockSize,blockSize));
    //Draw Top-Middle shadow.
    painter.drawPixmap(QRect(blockSize,0,destinationWidth-blockSize,blockSize),
                       m_shadowSource,
                       QRect(blockSize,0,blockSize,blockSize));
    //Draw Top-Right shadow.
    painter.drawPixmap(QRect(destinationWidth,0,blockSize2x, blockSize),
                       m_shadowSource,
                       QRect(blockSize,0,blockSize2x, blockSize));
    //Draw Middle-Left shadow.
    painter.drawPixmap(QRect(0,blockSize,blockSize,destinationHeight-blockSize),
                       m_shadowSource,
                       QRect(0,blockSize,blockSize,blockSize));
    //Draw Middle-Right shadow.
    painter.drawPixmap(QRect(destinationWidth+blockSize,blockSize,blockSize,destinationHeight),
                       m_shadowSource,
                       QRect(blockSize2x,blockSize,blockSize,blockSize));
    //Draw Left-Bottom shadow.
    painter.drawPixmap(QRect(0,destinationHeight,blockSize, blockSize2x),
                       m_shadowSource,
                       QRect(0,blockSize,blockSize,blockSize2x));
    //Draw Middle-Bottom shadow.
    painter.drawPixmap(QRect(blockSize,destinationHeight+blockSize,destinationWidth,blockSize),
                       m_shadowSource,
                       QRect(blockSize,blockSize2x,blockSize,blockSize));
    //Draw Right-Buttom shadow.
    painter.drawPixmap(QRect(destinationWidth+blockSize,destinationHeight+blockSize,blockSize,blockSize),
                       m_shadowSource,
                       QRect(blockSize2x,blockSize2x,blockSize,blockSize));
    painter.end();
    return shadowPixmap;
}

KNMusicAlbumDetail *KNMusicAlbumView::albumDetail() const
{
    return m_albumDetail;
}

void KNMusicAlbumView::setAlbumDetail(KNMusicAlbumDetail *albumDetail)
{
    m_albumDetail = albumDetail;
    m_albumDetail->setFocusProxy(this);
    //Do connection.
    connect(m_albumDetail, &KNMusicAlbumDetail::requireShowAlbum,
            this, &KNMusicAlbumView::displayAlbum);
    connect(m_albumDetail, &KNMusicAlbumDetail::requireClearSelection,
            this, &KNMusicAlbumView::clearSelection);
    //Hide the album detail.
    m_albumDetail->hide();
    //Move it up to the top.
    m_albumDetail->raise();
}
