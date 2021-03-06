/*
 * Copyright (C) Kreogist Dev Team <kreogistdevteam@126.com>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the COPYING file for more details.
 */
#ifndef KNSEARCHBOX_H
#define KNSEARCHBOX_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>

class QToolButton;
class QLineEdit;
class QTimeLine;
class QFocusEvent;
class KNSearchLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit KNSearchLineEdit(QWidget *parent = 0);

signals:
    void getFocus();
    void lostFocus();

protected:
    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);

private:
};

class KNSearchButton : public QLabel
{
    Q_OBJECT
public:
    explicit KNSearchButton(QWidget *parent = 0);

private:
};

class KNSearchBox : public QWidget
{
    Q_OBJECT
public:
    explicit KNSearchBox(QWidget *parent = 0);
    void setPlaceHolderText(const QString &text);
    void clear();
    QString text() const;
    QWidget *escFocusTo() const;
    void setEscFocusTo(QWidget *escFocusTo);
    QWidget *defaultEscFocusTo() const;
    void setDefaultEscFocusTo(QWidget *defaultEscFocusTo);

signals:
    void requireLostFocus();
    void editingFinished();
    void returnPressed();
    void selectionChanged();
    void textChanged(const QString & text);
    void textEdited(const QString & text);

public slots:
    void setText(const QString &text);

protected:
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);

private slots:
    void onActionLostFocus();
    void onActionBackgroundChanged(const int &frame);
    void onActionTextBackgroundChange(const int &frame);
    void onFocusGet();
    void onFocusLost();

private:
    QPalette m_palette, m_textPalette;
    KNSearchLineEdit *m_textContent;
    KNSearchButton *m_button;
    QWidget *m_escFocusTo=nullptr, *m_defaultEscFocusTo=nullptr;
    QColor m_baseColor=QColor(0xff, 0xff, 0xff),
           m_textColor=QColor(0xff, 0xff, 0xff);
    QTimeLine *m_mouseEnterAnime, *m_mouseLeaveAnime, *m_focusGet, *m_focusLost;
    int m_lightness=0x3A, m_minLightness=m_lightness, m_fontGrey;
};

#endif // KNSEARCHBOX_H
