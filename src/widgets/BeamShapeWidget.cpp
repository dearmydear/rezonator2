#include "BeamShapeWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>

static const int MIN_W = 50;
static const int MIN_H = 50;
static const int DEFAULT_W = 100;
static const int DEFAULT_H = 100;
static const int DEFAULT_M = 20;
static const int RESIZE_BORDER = 5;
static const int TEXT_TO_BORDER = 3;
static const int TEXT_TO_AXIS = 2;

BeamShapeWidget::BeamShapeWidget(QWidget *parent) : QWidget(parent) {
    setVisible(true);
    setMouseTracking(true);

    _menu = new QMenu(this);
    _menu->addAction(tr("Copy Image"), this, &BeamShapeWidget::copyImage);

    // TODO: set the same font as for axes labels
    const auto tm = QFontMetrics(font()).boundingRect(QStringLiteral("T"));
    _textW = tm.width();
    _textH = tm.height();
}

void BeamShapeWidget::setInitialGeometry(const QRect& r)
{
    if (r.isValid())
    {
        setGeometry(r);
        parentSizeChanged();
    }
    else
        setGeometry(parentWidget()->width() - DEFAULT_W - DEFAULT_M, DEFAULT_M, DEFAULT_W, DEFAULT_H);
}

void BeamShapeWidget::mousePressEvent(QMouseEvent *e)
{
    e->accept();

    auto const pos = e->globalPos();
    updateSite(pos);

    if (e->buttons().testFlag(Qt::LeftButton))
    {
        const auto g = geometry();
        X = g.x();
        Y = g.y();
        H = g.height();
        W = g.width();
        x0 = pos.x();
        y0 = pos.y();
    }
}

void BeamShapeWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton)
    {
        _menu->exec(e->globalPos());
        e->accept();
    }
}

void BeamShapeWidget::enterEvent(QEvent *e)
{
    e->accept();
    _drawResizeBorder = true;
    update();
}


void BeamShapeWidget::leaveEvent(QEvent *e)
{
    e->accept();
    _drawResizeBorder = false;
    update();
}

void BeamShapeWidget::mouseMoveEvent(QMouseEvent *e) {
    e->accept();

    const auto pos = e->globalPos();
    if (!e->buttons().testFlag(Qt::LeftButton))
    {
        updateSite(pos);
        return;
    }

    const int x1 = pos.x();
    const int y1 = pos.y();
    int newW = W;
    int newH = H;
    int newX = X;
    int newY = Y;
    const auto ps = parentWidget()->size();
    const int parentW = ps.width();
    const int parentH = ps.height();
    switch (_site)
    {
    case LEFT:
        newX = X + x1 - x0;
        newW = W + x0 - x1;
        if (newX < 0) { newW += newX; newX = 0; }
        if (newW < MIN_W) { newX -= MIN_W - newW; newW = MIN_W; }
        break;
    case TOP:
        newY = Y + y1 - y0;
        newH = H + y0 - y1;
        if (newY < 0) { newH += newY; newY = 0; }
        if (newH < MIN_H) { newY -= MIN_H - newH; newH = MIN_H; }
        break;
    case RIGHT:
        newW = W + x1 - x0;
        if (newW < MIN_W) newW = MIN_W;
        if (X + newW > parentW) newW -= X + newW - parentW;
        break;
    case BOTTOM:
        newH = H + y1 - y0;
        if (newH < MIN_H) newH = MIN_H;
        if (Y + newH > parentH) newH -= Y + newH - parentH;
        break;
    case TOP_LEFT:
        newX = X - x0 + x1;
        newY = Y - y0 + y1;
        newW = W - x1 + x0;
        newH = H - y1 + y0;
        if (newX < 0) { newW += newX; newX = 0; }
        if (newY < 0) { newH += newY; newY = 0; }
        if (newW < MIN_W) { newX -= MIN_W - newW; newW = MIN_W; }
        if (newH < MIN_H) { newY -= MIN_H - newH; newH = MIN_H; }
        break;
    case TOP_RIGHT:
        newY = Y - y0 + y1;
        newW = W + y0 - y1;
        newH = H - y1 + y0;
        if (newW < MIN_W) newW = MIN_W;
        if (newY < 0) { newH += newY; newY = 0; }
        if (newH < MIN_H) { newY -= MIN_H - newH; newH = MIN_H; }
        if (X + newW > parentW) newW -= X + newW - parentW;
        break;
    case BOTTOM_LEFT:
        newX = X + x1 - x0;
        newW = W + x0 - x1;
        newH = H + y1 - y0;
        if (newH < MIN_H) newH = MIN_H;
        if (newX < 0) { newW += newX; newX = 0; }
        if (newW < MIN_W) { newX -= MIN_W - newW; newW = MIN_W; }
        if (Y + newH > parentH) newH -= Y + newH - parentH;
        break;
    case BOTTOM_RIGHT:
        newH = H + y1 - y0;
        newW = W + x1 - x0;
        if (newW < MIN_W) newW = MIN_W;
        if (newH < MIN_H) newH = MIN_H;
        if (X + newW > parentW) newW -= X + newW - parentW;
        if (Y + newH > parentH) newH -= Y + newH - parentH;
        break;
    case CENTER:
        newX = X + x1 - x0;
        newY = Y + y1 - y0;
        if (newX < 0) newX = 0;
        if (newY < 0) newY = 0;
        if (newX + W > parentW) newX = parentW - W;
        if (newY + H > parentH) newY = parentH - H;
        break;
    }
    setGeometry(newX, newY, newW, newH);
}

void BeamShapeWidget::updateSite(const QPoint &pos)
{
    const int x = pos.x();
    const int y = pos.y();
    auto p = mapToGlobal({0, 0});
    const int x1 = p.x() + RESIZE_BORDER;
    const int y1 = p.y() + RESIZE_BORDER;
    const int x2 = p.x() + width() - RESIZE_BORDER;
    const int y2 = p.y() + height() - RESIZE_BORDER;
         if (y > y2 and x < x1) { _site = BOTTOM_LEFT; setCursor(Qt::SizeBDiagCursor); }
    else if (y > y2 and x > x2) { _site = BOTTOM_RIGHT; setCursor(Qt::SizeFDiagCursor); }
    else if (y < y1 and x < x1) { _site = TOP_LEFT; setCursor(Qt::SizeFDiagCursor); }
    else if (y < y1 and x > x2) { _site = TOP_RIGHT; setCursor(Qt::SizeBDiagCursor); }
    else if (x < x1) { _site = LEFT; setCursor(Qt::SizeHorCursor); }
    else if (x > x2) { _site = RIGHT; setCursor(Qt::SizeHorCursor); }
    else if (y < y1) { _site = TOP; setCursor(Qt::SizeVerCursor); }
    else if (y > y2) { _site = BOTTOM; setCursor(Qt::SizeVerCursor); }
   else { _site = CENTER; setCursor(Qt::ArrowCursor); }
}

void BeamShapeWidget::parentSizeChanged()
{
    const auto g = geometry();
    const int X = g.x();
    const int Y = g.y();
    const int H = g.height();
    const int W = g.width();
    const auto ps = parentWidget()->size();
    const int parentW = ps.width();
    const int parentH = ps.height();
    int newX = X;
    int newY = Y;
    int newW = W;
    int newH = H;
    if (X + W > parentW) newX = X - (X + W - parentW);
    if (Y + H > parentH) newY = Y - (Y + H - parentH);
    if (X < 0) { newW = W + X; newX = 0; }
    if (Y < 0) { newH = H + Y; newY = 0; }
    if (newW < MIN_W) newW = MIN_W;
    if (newH < MIN_H) newH = MIN_H;
    if (newX != X or newY != Y or newW != W or newH != H)
        setGeometry(newX, newY, newW, newH);
}

void BeamShapeWidget::paintEvent(QPaintEvent*)
{
    paint(this, false);
}

void BeamShapeWidget::paint(QPaintDevice *target, bool showBackground) const
{
    const int W = target->width() - 1;
    const int H = target->height() - 1;
    const int R = qMin(W, H)/2 - _textH - TEXT_TO_BORDER;

    QPainter p(target);

    if (showBackground)
    {
        p.fillRect(0, 0, W, H, Qt::white);
    }
    else if (_drawResizeBorder)
    {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(Qt::blue, 1, Qt::DotLine));
        p.drawRect(0, 0, W, H);
    }

    p.setPen(QPen(Qt::red, 2));
    p.setBrush(QColor(255, 205, 205));
    p.setRenderHint(QPainter::Antialiasing);
    p.drawEllipse(W/2 - R, H/2 - R, 2*R, 2*R);

    p.setPen(Qt::black);
    p.drawText(W - _textW - TEXT_TO_BORDER, H/2 - TEXT_TO_AXIS, QStringLiteral("T"));
    p.drawText(W/2 - _textW - TEXT_TO_AXIS, _textH, QStringLiteral("S"));

    p.setPen(QPen(QColor(100, 100, 100), 1));
    p.setRenderHint(QPainter::Antialiasing, false);
    p.drawLine(0, H/2, W, H/2);
    p.drawLine(W/2, 0, W/2, H);
}

void BeamShapeWidget::copyImage() const
{
    QImage image(width(), height(), QImage::Format_RGB32);
    paint(&image, true);
    qApp->clipboard()->setImage(image);
}
