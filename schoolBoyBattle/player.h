#ifndef PLAYER_H
#define PLAYER_H

#include "dataloader.h"
#include "tile.h"
#include "candy.h"

#include <QMainWindow>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QVector2D>




class Player : public QGraphicsObject
{
public:
    Player(
            int id,
            int team,
            DataLoader *dataLoader,
            QList<Tile*> *collisionTiles,
            int playerWidth, int playerHeight, int playerSpeed,
            QGraphicsObject *parent = nullptr);
    ~Player();
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void refresh(int delta);
    QList<Candy *> looseCandies(Candy *candy);

private:
    enum Team : int {red = 0, black = 1};
    enum Gender : int {girl = 0, boy = 1};
    enum Facing {facingLeft, facingRight};
    enum PlayerMoves {moveUp, moveRight, moveDown, moveLeft};
    enum Animations : int {idle = 0, run = 1};

    typedef struct Animations_s {
        QTimer *timer;
        int frameIndex;
        DataLoader::PlayerAnimationsStruct *sharedDatas;
    } AnimationsLocalStruct;
    QHash<Animations, AnimationsLocalStruct*> animationsLocal;

    Team team;
    Gender gender;
    Animations currentAnimation;
    Facing facing;
    DataLoader *dataLoader;
    QList<Candy *> candiesTaken;
    int id;
    bool moves[4] = {false, false, false, false};
    const int playerWidth;
    const int playerHeight;
    const int playerSpeed;

    QList<Tile*> *collisionTiles;

    void refreshTakenCandies(double delta);
    void move(QVector2D vector, bool inverted = false);
    bool collide(QVector2D movingVector);
    void collideWithCandy();
    QVector2D calculateMovingVector(double delta);
    QVector2D calculateAnswerVector(QVector2D movingVector);
    void validate_candies();
    void takeCandy();
    void animationNextFrame();
    void setAnimation(Animations a);
    QPixmap *getAnimationByTeamAndGender(QString name);
    Animations getAnimationType();
    Player::Facing getFacing();
    void setZIndex();
    void loadAnimations();
    Player::AnimationsLocalStruct *setupAnimation(DataLoader::PlayerAnimationsStruct* sharedDatas);

public slots:
    void keyMove(int playerId, int direction, bool value);

};

#endif // PLAYER_H
