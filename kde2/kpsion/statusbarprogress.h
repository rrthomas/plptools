/*-*-c++-*-
 * $Id$
 *
 * Shamelessly stolen from:
 *   khexedit - Versatile hex editor
 *   Copyright (C) 1999  Espen Sand, espensa@online.no
 *   This file is based on the work by Martynas Kunigelis (KProgress)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef _STATUSBAR_PROGRESS_H_
#define _STATUSBAR_PROGRESS_H_

#include <qframe.h>
#include <qrangecontrol.h>

class KPsionStatusBarProgress : public QFrame, public QRangeControl {
    Q_OBJECT

public:
    /**
    * Possible values for orientation
    */
    enum Orientation { Horizontal, Vertical };

    /**
    * Possible values for bar style.
    *
    * Solid means one continuous progress bar, Blocked means a
    * progress bar made up of several blocks.
    */
    enum BarStyle { Solid, Blocked };

    /**
    * Construct a default progress bar. Orientation is horizontal.
    */
    KPsionStatusBarProgress(QWidget *parent=0, const char *name=0);

    /**
    * Construct a KProgress bar with an orientation.
    */
    KPsionStatusBarProgress(Orientation, QWidget *parent=0, const char *name=0);

    /**
    * Construct a KProgress bar with minimum, maximum and initial value.
    */
    KPsionStatusBarProgress(int minValue, int maxValue, int value, Orientation,
			    QWidget *parent=0, const char *name=0);

    /**
    * Destructor
    */
    ~KPsionStatusBarProgress( void );

    /**
    * Set the progress bar style. Allowed values are Solid and Blocked.
    */
    void setBarStyle(BarStyle style);

    /**
    * Set the color of the progress bar.
    */
    void setBarColor(const QColor &);

    /**
    * Set a pixmap to be shown in the progress bar.
    */
    void setBarPixmap(const QPixmap &);

    /**
    * Set the orientation of the progress bar.
    * Allowed values are Horizonzal and Vertical.
    */
    void setOrientation(Orientation);

    /**
    * Retrieve the bar style.
    */
    BarStyle barStyle() const;

    /**
    * Retrieve the bar color.
    */
    const QColor &barColor() const;

    /**
    * Retrieve the bar pixmap.
    */
    const QPixmap *barPixmap() const;

    /**
    * Retrieve the orientation.
    */
    Orientation orientation() const;

    /**
    * Returns TRUE if progress text will be displayed, FALSE otherwise.
    */
    bool textEnabled() const;

    /**
    * Returns the recommended width for vertical progress bars or
    * the recommended height for vertical progress bars
    */
    virtual QSize sizeHint() const;


public slots:
    void setValue( int );
    void setValue( int, int);
    void advance( int );
    void setTextEnabled( bool state );
    void setText( const QString &msg );

signals:
    void percentageChanged(int);
    void pressed( void );

protected:
    void valueChange();
    void rangeChange();
    void styleChange( GUIStyle );
    void paletteChange( const QPalette & );
    void drawContents( QPainter * );
    void mousePressEvent( QMouseEvent *e );

private:
    QPixmap     *mBarPixmap;
    QColor	mBarColor;
    QColor	mBarTextColor;
    QColor	mTextColor;
    QRect       fr;
    BarStyle    mBarStyle;
    Orientation mOrientation;
    bool	mTextEnabled;
    QString     labelMsg;
    QString     labelText;
    int         mCurItem;
    int         mMaxItem;

    void initialize(void);
    int recalcValue(int);
    void drawText(QPainter *);
    void adjustStyle(void);
    void squeezeTextToLabel(void);
};


#endif

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
