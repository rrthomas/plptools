/*-*-c++-*-
 * $Id$
 *
 *   Shamlessly stolen from:
 *     khexedit - Versatile hex editor
 *     Copyright (C) 1999  Espen Sand, espensa@online.no
 *     This file is based on the work by Martynas Kunigelis (KProgress)
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

#include "statusbarprogress.h"

#include <qpainter.h>
#include <qstring.h>
#include <qpixmap.h>
#include <qtooltip.h>
#include <qstyle.h>

#include <kapp.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <klocale.h>

KPsionStatusBarProgress::
KPsionStatusBarProgress(QWidget *parent,
						 const char *name)
    : QFrame(parent, name), QRangeControl(0, 100, 1, 10, 0),
mOrientation(Horizontal) {
    initialize();
}

KPsionStatusBarProgress::
KPsionStatusBarProgress(Orientation orientation,
						  QWidget *parent,
						 const char *name)
    : QFrame(parent, name), QRangeControl(0, 100, 1, 10, 0),
mOrientation(orientation) {
    initialize();
}

KPsionStatusBarProgress::
KPsionStatusBarProgress(int minValue, int maxValue,
						 int value,
						 Orientation orientation,
						 QWidget *parent,
						 const char *name)
    : QFrame(parent, name), QRangeControl(minValue, maxValue, 1, 10, value),
mOrientation(orientation) {
    initialize();
}

KPsionStatusBarProgress::
~KPsionStatusBarProgress() {
    delete mBarPixmap;
}

void KPsionStatusBarProgress::
advance(int offset) {
    setValue(value() + offset);
}

void KPsionStatusBarProgress::
initialize(void) {
    mBarPixmap    = 0;
    mBarStyle     = Solid;

    mBarColor     = palette().normal().highlight();
    mBarTextColor = palette().normal().highlightedText();
    mTextColor    = palette().normal().text();
    setBackgroundColor(palette().normal().background());

    QFont font(KGlobalSettings::generalFont());
    // font.setBold(true);
    setFont(font);

    mTextEnabled = false;
    adjustStyle();
}


void KPsionStatusBarProgress::
setBarPixmap(const QPixmap &pixmap) {
    if (pixmap.isNull() == true)
	return;
    if (mBarPixmap != 0) {
	delete mBarPixmap;
	mBarPixmap = 0;
    }
    mBarPixmap = new QPixmap(pixmap);
}

void KPsionStatusBarProgress::
setBarColor(const QColor &color) {
    mBarColor = color;
    if (mBarPixmap != 0) {
	delete mBarPixmap;
	mBarPixmap = 0;
    }
}

void KPsionStatusBarProgress::
setBarStyle(BarStyle style) {
    if (mBarStyle != style) {
	mBarStyle = style;
	update();
    }
}

void KPsionStatusBarProgress::
setOrientation(Orientation orientation) {
    if (mOrientation != orientation) {
	mOrientation = orientation;
	update();
    }
}

void KPsionStatusBarProgress::
setValue(int value) {
    mCurItem = mMaxItem = -1;
    QRangeControl::setValue(value);
}

void KPsionStatusBarProgress::
setValue(int curItem, int maxItem) {
    if (curItem <= 0 || maxItem <= 0 || curItem > maxItem) {
	mCurItem = mMaxItem = -1;
	QRangeControl::setValue(0);
    } else {
	mCurItem = curItem;
	mMaxItem = maxItem;
	float fraction = (float)curItem/(float)maxItem;
	QRangeControl::setValue((int)(fraction * 100.0));
    }
}


void KPsionStatusBarProgress::
setTextEnabled(bool state) {
    if (mTextEnabled != state) {
	mTextEnabled = state;
	update();
    }
}

void KPsionStatusBarProgress::
setText(const QString &msg) {
    labelMsg = msg;
    if (mTextEnabled == true)
	update();
}




const QColor & KPsionStatusBarProgress::
barColor(void) const {
    return(mBarColor);
}

const QPixmap * KPsionStatusBarProgress::
barPixmap(void) const {
    return(mBarPixmap);
}

bool KPsionStatusBarProgress::
textEnabled(void) const {
    return(mTextEnabled);
}

QSize KPsionStatusBarProgress::
sizeHint(void) const {
    QSize s(size());

    if (orientation() == KPsionStatusBarProgress::Vertical)
	s.setWidth(fontMetrics().lineSpacing());
    else
	s.setHeight(fontMetrics().lineSpacing());
    return(s);
}


KPsionStatusBarProgress::Orientation KPsionStatusBarProgress::
orientation(void) const {
    return(mOrientation);
}

KPsionStatusBarProgress::BarStyle KPsionStatusBarProgress::
barStyle() const {
    return(mBarStyle);
}

int KPsionStatusBarProgress::
recalcValue(int range) {
    int abs_value = value() - minValue();
    int abs_range = maxValue() - minValue();
    if (abs_range == 0)
	return range;
    return range * abs_value / abs_range;
}

void KPsionStatusBarProgress::
valueChange() {
    repaint(contentsRect(), FALSE);
    emit percentageChanged(recalcValue(100));
}

void KPsionStatusBarProgress::
rangeChange() {
    repaint(contentsRect(), FALSE);
    emit percentageChanged(recalcValue(100));
}

void KPsionStatusBarProgress::
styleChange(GUIStyle) {
    adjustStyle();
}

void KPsionStatusBarProgress::
adjustStyle(void) {
#if (QT_VERSION < 300)
    bool isWinStyle = (style().guiStyle() == WindowsStyle);
#else
    bool isWinStyle = (style().styleHint(QStyle::SH_GUIStyle) == WindowsStyle);
#endif
    if (isWinStyle)
	setFrameStyle(QFrame::NoFrame);
    else {
	setFrameStyle(QFrame::Panel|QFrame::Sunken);
	setLineWidth(1);
    }
    update();
}

void KPsionStatusBarProgress::
paletteChange(const QPalette &) {
    mBarColor     = palette().normal().highlight();
    mBarTextColor = palette().normal().highlightedText();
    mTextColor    = palette().normal().text();
    setBackgroundColor(palette().normal().background());
}

void KPsionStatusBarProgress::
drawText(QPainter *p) {
    QRect r(contentsRect());

    squeezeTextToLabel();
    p->setPen(mTextColor);
    p->drawText(r, AlignCenter, labelText);
    p->setClipRegion(fr);
    p->setPen(mBarTextColor);
    p->drawText(r, AlignCenter, labelText);
}

void KPsionStatusBarProgress::
drawContents(QPainter *p) {
    QRect cr = contentsRect(), er = cr;
    fr = cr;
    QBrush fb(mBarColor), eb(backgroundColor());

    if (mBarPixmap != 0)
	fb.setPixmap(*mBarPixmap);

    if (backgroundPixmap())
	eb.setPixmap(*backgroundPixmap());

    switch(mBarStyle) {
	case Solid:
	    if (mOrientation == Horizontal) {
		fr.setWidth(recalcValue(cr.width()));
		er.setLeft(fr.right() + 1);
	    } else {
		fr.setTop(cr.bottom() - recalcValue(cr.height()));
		er.setBottom(fr.top() - 1);
	    }

	    p->setBrushOrigin(cr.topLeft());
	    p->fillRect(fr, fb);
	    p->fillRect(er, eb);

	    if (mTextEnabled == true)
		drawText(p);
	    break;

	case Blocked:
	    const int margin = 2;
	    int max, num, dx, dy;
	    if (mOrientation == Horizontal) {
		fr.setHeight(cr.height() - 2 * margin);
		fr.setWidth((int)(0.67 * fr.height()));
		fr.moveTopLeft(QPoint(cr.left() + margin, cr.top() + margin));
		dx = fr.width() + margin;
		dy = 0;
		max = (cr.width() - margin) / (fr.width() + margin) + 1;
		num = recalcValue(max);
	    } else {
		fr.setWidth(cr.width() - 2 * margin);
		fr.setHeight((int)(0.67 * fr.width()));
		fr.moveBottomLeft(QPoint(cr.left() + margin, cr.bottom() - margin));
		dx = 0;
		dy = - (fr.height() + margin);
		max = (cr.height() - margin) / (fr.height() + margin) + 1;
		num = recalcValue(max);
	    }
	    p->setClipRect(cr.x() + margin, cr.y() + margin,
			   cr.width() - margin, cr.height() - margin);
	    for (int i = 0; i < num; i++) {
		p->setBrushOrigin(fr.topLeft());
		p->fillRect(fr, fb);
		fr.moveBy(dx, dy);
	    }

	    if (num != max) {
		if (mOrientation == Horizontal)
		    er.setLeft(fr.right() + 1);
		else
		    er.setBottom(fr.bottom() + 1);
		if (!er.isNull()) {
		    p->setBrushOrigin(cr.topLeft());
		    p->fillRect(er, eb);
		}
	    }
	    break;
    }

}

void KPsionStatusBarProgress::
mousePressEvent(QMouseEvent */*e*/) {
    emit pressed();
}

void KPsionStatusBarProgress::
squeezeTextToLabel() {
    QFontMetrics fm(fontMetrics());
    int labelWidth = size().width();
    int percent;
    QString fullText;

    if (labelMsg.isEmpty() == true) {
	labelText = QString("%1%").arg(recalcValue(100));
	return;
    } else {
	if (mCurItem > 0)
	    fullText = i18n("%1 %2 of %3").arg(labelMsg).arg(mCurItem).arg(mMaxItem);
	else {
	    percent = recalcValue(100);
	    fullText = QString("%1 %2%").arg(labelMsg).arg(percent);
	}
    }
    int textWidth = fm.width(fullText);
    if (textWidth > labelWidth) {
	// start with the dots only
	QString squeezedMsg = "...";
	QString squeezedText;

	if (mCurItem > 0)
	    squeezedText = i18n("%1 %2 of %3").arg(squeezedMsg).arg(mCurItem).arg(mMaxItem);
	else
	    squeezedText = QString("%1 %2%").arg(squeezedMsg).arg(percent);
	int squeezedWidth = fm.width(squeezedText);

	// estimate how many letters we can add to the dots on both sides
	int letters = fullText.length() * (labelWidth - squeezedWidth) / textWidth / 2;
	squeezedMsg = labelMsg.left(letters) + "..." + labelMsg.right(letters);
	if (mCurItem > 0)
	    squeezedText = i18n("%1 %2 of %3").arg(squeezedMsg).arg(mCurItem).arg(mMaxItem);
	else
	    squeezedText = QString("%1 %2%").arg(squeezedMsg).arg(percent);
	squeezedWidth = fm.width(squeezedText);

	if (squeezedWidth < labelWidth) {
	    // we estimated too short
	    // add letters while text < label
	    do {
                letters++;
                squeezedMsg = labelMsg.left(letters) + "..." +
		    labelMsg.right(letters);
		if (mCurItem > 0)
		    squeezedText = i18n("%1 %2 of %3").arg(squeezedMsg).arg(mCurItem).arg(mMaxItem);
		else
		    squeezedText = QString("%1 %2%").arg(squeezedMsg).arg(percent);
                squeezedWidth = fm.width(squeezedText);
	    } while (squeezedWidth < labelWidth);
	    letters--;
	    squeezedMsg = labelMsg.left(letters) + "..." +
		labelMsg.right(letters);
	    if (mCurItem > 0)
		squeezedText = i18n("%1 %2 of %3").arg(squeezedMsg).arg(mCurItem).arg(mMaxItem);
	    else
		squeezedText = QString("%1 %2%").arg(squeezedMsg).arg(percent);
	} else if (squeezedWidth > labelWidth) {
	    // we estimated too long
	    // remove letters while text > label
	    do {
		letters--;
                squeezedMsg = labelMsg.left(letters) + "..." +
		    labelMsg.right(letters);
		if (mCurItem > 0)
		    squeezedText = i18n("%1 %2 of %3").arg(squeezedMsg).arg(mCurItem).arg(mMaxItem);
		else
		    squeezedText = QString("%1 %2%").arg(squeezedMsg).arg(percent);
		squeezedWidth = fm.width(squeezedText);
	    } while (squeezedWidth > labelWidth);
	}

	if (letters < 5) {
	    // too few letters added -> we give up squeezing
	    labelText = fullText;
	} else
	    labelText = squeezedText;

	QToolTip::remove(this);
	QToolTip::add(this, fullText);
    } else {
	labelText = fullText;
	QToolTip::remove( this );
	QToolTip::hide();
    };
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
