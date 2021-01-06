#include "candy.h"
#include <QBrush>
#include <QPen>
#include <QDebug>
#include "game.h"

#define CANDY_WIDTH 130
#define CANDY_HEIGHT 130
#define HITBOX_DEBUG true


Candy::Candy(
        int type,
        QHash<int, DataLoader::CandyAnimationsStruct*> *sharedAnimationsDatas,
        QGraphicsItem *parent)
    : QGraphicsItem(parent)
{
    this->type = static_cast<Type>(type);
    loadAnimations(sharedAnimationsDatas);
    setPos(750, 500);
}

// Setup des animations des candies ---------------------------------------------------------

void Candy::loadAnimations(QHash<int, DataLoader::CandyAnimationsStruct*> *sharedAnimationsDatas) {
    animations.insert(peanut, setupCandyAnimationData(-1, sharedAnimationsDatas->value(DataLoader::getCandyAnimationId(type))));
    animations.insert(mandarin, setupCandyAnimationData(-1, sharedAnimationsDatas->value(DataLoader::getCandyAnimationId(type))));
}

Candy::AnimationsLocalDatasStruct* Candy::setupCandyAnimationData(int framerate, DataLoader::CandyAnimationsStruct *sharedDatas) {
    AnimationsLocalDatasStruct *c = new AnimationsLocalDatasStruct();
    c->frameIndex = 0;
    c->sharedDatas = sharedDatas;
    // Si on donne un -1 pour le framerate, il n'y a pas d'animation
    if(framerate >= 0) {
        c->timer = new QTimer();
        c->timer->setInterval(framerate);
        c->timer->start();
    }
    return c;
}

// Setup des placements des candies ---------------------------------------------------------



// OVERRIDE REQUIRED ------------------------------------------------------------------------

// Paints contents of item in local coordinates
void Candy::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {

    if(HITBOX_DEBUG) {
        // Debug rect
        painter->setPen(QPen(Qt::black));
        painter->setBrush(QBrush(Qt::white));
        painter->drawRect(boundingRect());
        painter->drawText(boundingRect().x()+10, boundingRect().y()+10, QString::number(id));
    }

    AnimationsLocalDatasStruct *candyToDraw = animations.value(type);
    QPixmap *imageToDraw = candyToDraw->sharedDatas->image;

    QRectF sourceRect = QRectF(imageToDraw->width() / candyToDraw->sharedDatas->nbFrame * candyToDraw->frameIndex, 0,
                               imageToDraw->width() / candyToDraw->sharedDatas->nbFrame, imageToDraw->height());
    QRectF targetRect = boundingRect();
    painter->drawPixmap(targetRect, *imageToDraw, sourceRect);

    // Lignes pour le compilateur
    Q_UNUSED(option)
    Q_UNUSED(widget)
}

// Returns outer bounds of item as a rectangle
// Called by QGraphicsView to determine what regions need to be redrawn
// the rect stay at 0:0 !!
QRectF Candy::boundingRect() const {
    return QRectF(0, 0, CANDY_WIDTH, CANDY_HEIGHT);
}

// collisions detection
QPainterPath Candy::shape() const {
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}


Candy::~Candy() {

}
