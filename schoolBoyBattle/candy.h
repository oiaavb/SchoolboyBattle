#ifndef CANDY_H
#define CANDY_H
#include <QGraphicsObject>
#include <QPixmap>
#include "dataloader.h"
#include "tilecandyplacement.h"

class Candy : public QGraphicsObject
{
    Q_OBJECT
public:
    Candy(
            int x,
            int y,
            int candyType,
            int candySize,
            DataLoader *dataLoader,
            TileCandyPlacement *tilePlacement = nullptr,
            QGraphicsObject *parent = nullptr);
    ~Candy();
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void pickUp(QGraphicsItem *player);
    void refresh(QPointF pos, int posInQueue, double delta);
    bool isTaken();
    QGraphicsItem* getCurrentPlayer();
    void setCurrentPlayer(QGraphicsItem *player);


    enum Type : int {peanut = 0, mandarin = 1};
    enum Size : int {small = 0, big = 1};
    enum Animations {idle = 0};

private:
    typedef struct Animations_s {
        QTimer *timer;
        int frameIndex;
        DataLoader::CandyAnimationsStruct *sharedDatas;
    } AnimationsLocalStruct;


    QHash<Animations, AnimationsLocalStruct*> animationsLocal;
    int id;
    Type candyType;
    Size candySize;
    DataLoader *dataLoader;
    TileCandyPlacement* tilePlacement;
    QGraphicsItem *currentPlayer;
    bool taken;


    void loadAnimations();
    Candy::AnimationsLocalStruct *setupCandyAnimationData(DataLoader::CandyAnimationsStruct *sharedDatas);
    Animations animation;
    void setType(Type t);
    void animationNextFrame();
    void setAnimation(Animations a);
    void setZIndex();

signals:
    void pickedUp();
};


#endif // CANDY_H
