#include "player.h"
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QApplication>
#include <QKeyEvent>
#include <QRectF>
#include <QDebug>
#include <QVector2D>
#include <QFont>
#include "game.h"

#define HITBOX_DEBUG false
#define CANDY_MAX 50
#define QUEUE_PROTECTED_TIME_MS 750

// Constructeur utilisé pour créer les boss
Player::Player(int team, DataLoader *dataLoader) :
    team(static_cast<Team>(team)),
    dataLoader(dataLoader)
{}

Player::Player(
        int id,
        int team,
        int gender,
        QString username,
        DataLoader *dataLoader,
        QList<Tile*> *collisionTiles,
        QGraphicsObject *parent)
    : QGraphicsObject(parent),
      team(static_cast<Team>(team)),
      dataLoader(dataLoader),
      gender(static_cast<Gender>(gender)),
      id(id),
      collisionTiles(collisionTiles)
{
    setPos(
        dataLoader->getTeamSpawnpoint(team).x(),
        dataLoader->getTeamSpawnpoint(team).y() - dataLoader->getTileSize()/2 - (dataLoader->getPlayerSize().y() - dataLoader->getTileSize()));

    loadAnimations();
    setAnimation(idle);
    setZIndex(dataLoader->getPlayerSize().y());
    queueProtected = new QTimer(this);
    queueProtected->setSingleShot(true);
    queueProtected->setInterval(QUEUE_PROTECTED_TIME_MS);
    queueProtected->stop();

    // Noms d'utilisateurs sur les autres joueurs
    this->username = new QGraphicsTextItem(this);
    if(dataLoader->isMultiplayer() && dataLoader->getPlayerIndexInMulti() != id)
        setUsername(username);
}

void Player::setUsername(QString username) {
    this->username->setHtml("<div style='background-color:#65ffffff;'>" + username + "</div>");
    this->username->setFlag(GraphicsItemFlag::ItemIgnoresTransformations);
    QFont font("Helvetica", 17);
    this->username->setFont(font);
    int centerTextX = (this->username->boundingRect().width() - boundingRect().width()) / 2;
    this->username->setPos(-centerTextX, -40);
}

void Player::loadAnimations() {
    animationsLocal.insert(idle, setupAnimation(dataLoader->playerAnimations.value(dataLoader->getPlayerAnimationId(gender, team, idle))));
    animationsLocal.insert(run, setupAnimation(dataLoader->playerAnimations.value(dataLoader->getPlayerAnimationId(gender, team, run))));
}

Player::AnimationsLocalStruct* Player::setupAnimation(DataLoader::PlayerAnimationsStruct* sharedDatas) {
    AnimationsLocalStruct* aStruct = new AnimationsLocalStruct;
    aStruct->frameIndex = 0;
    aStruct->timer = new QTimer();
    aStruct->timer->setInterval(sharedDatas->framerate);
    aStruct->timer->stop();
    connect(aStruct->timer, &QTimer::timeout, this, &Player::animationNextFrame);
    aStruct->sharedDatas = sharedDatas;
    return aStruct;
}

void Player::keyMove(int playerId, int direction, bool value) {
    if(playerId == id) {
        moves[direction] = value;
    }

    // Changer l'animation si nécessaire
    Animations newAnimationType = getAnimationType();
    if(currentAnimation != newAnimationType) {
        // Si l'animation qu'on doit afficher n'est pas la même que l'actuelle
        setAnimation(newAnimationType);
    }

    // Changer la direction du joueur si nécessaire
    facing = getFacing();
    update();
}

void Player::refresh(double delta, int socketDescriptor) {
    /*
     * déterminer le vecteur mouvement
     * s'il y a une collision
     *      vecteur de mouvement = déterminer le vecteur de réponse
     * déplacer le joueur en fonction du vecteur de mouvement
     */

    QVector2D movingVector = calculateMovingVector(delta);

    // On ne calcule la collision avec les candy et les murs que dans 2 conditions
    // - Si on est en local
    // - Si on est en multi et ce joueur est celui qui est joué

    if(collideWithWalls(movingVector))
        movingVector = calculateAnswerVector(movingVector);

    move(movingVector);

    if(getAnimationType() == run) {
        setZIndex(dataLoader->getPlayerSize().y());
    }


    if((dataLoader->isMultiplayer() && id == socketDescriptor) || !dataLoader->isMultiplayer()) {
        // Detecter si le joueur touche un candy
        collideWithCandy();
        // Detecter si le joueur touche sa base
        collideWithSpawn();
    }


}

// COLLISIONS ET DEPLACEMENTS ----------------------------------------------------------

/*
 * déplacer le joueur dans la direction du vecteur mouvement
 * tester s'il y a une collision
 * remettre le joueur dans sa position initiale
 */
bool Player::collideWithWalls(QVector2D movingVector) {

    // On simule une avancée du joueur pour savoir si là où il veut aller il peut y aller
    move(3*movingVector);
    bool returnValue = false;

    // Les items en contacte avec le joueur
    QList<QGraphicsItem*> itemsColliding = collidingItems();

    // Les tiles sur la couche collision autour du joueur
    QList<Tile*> collisionTilesNearby = static_cast<Game*>(scene())->tilesNearby("4-collision", x(), y());

    // S'il y a une tile collision près du joueur
    if(collisionTilesNearby.size() > 0) {
        for(int i = 0; i < itemsColliding.size(); i++) {
            QGraphicsItem *collidingItem = itemsColliding.at(i);

            for(int j = 0; j < collisionTilesNearby.size(); j++) {
                Tile *tileNearby = collisionTilesNearby.at(j);
                // Si un des items avec lesquels on collide se trouve dans la liste des tiles
                // de collisions qui se trouvent à proximité
                if(collidingItem->x() == tileNearby->x() && collidingItem->y() == tileNearby->y()) {
                    returnValue = true;
                    break;
                }
            }
        }
    }
    // On remet le joueur à sa position normale
    move(3*movingVector, true);
    return returnValue;

}

void Player::collideWithSpawn() {

    // Si le joueur n'a pas de candy, on passe
    if(IdsCandiesTaken.length() == 0) return;

    // Si tous les bonbons du joueur sont déjà validés, on ne fait rien
    if(static_cast<Game*>(scene())->hasPlayerAnyCandyValid(id)) return;

    // Les items en contacte avec le joueur
    QList<QGraphicsItem*> itemsColliding = collidingItems();

    // Les tiles sur la couche collision autour du joueur
    QList<Tile*> spawnTilesNearby = static_cast<Game*>(scene())->tilesNearby("1-spawns", x(), y());

    // S'il y a une tile de spawn près du joueur
    if(spawnTilesNearby.size() > 0) {
        for(int i = 0; i < itemsColliding.size(); i++) {
            QGraphicsItem *collidingItem = itemsColliding.at(i);

            for(int j = 0; j < spawnTilesNearby.size(); j++) {
                Tile *tileNearby = spawnTilesNearby.at(j);
                // Si la tile n'est pas celle de notre équipe, on ne fait rien
                if(team == black && dataLoader->getTileType("world/config/spawn-red.png") == tileNearby->getTileType())
                        return;
                if(team == red && dataLoader->getTileType("world/config/spawn-black.png") == tileNearby->getTileType())
                        return;
                // Si un des items avec lesquels on collide se trouve dans la liste des tiles
                // de collisions qui se trouvent à proximité
                if(collidingItem->x() == tileNearby->x() && collidingItem->y() == tileNearby->y()) {
                    if(!atSpawn) {
                        atSpawn = true;
                        emit validateCandies(this->id);
                        break;
                    }
                }
            }
        }
    }
    atSpawn = false;
}

void Player::collideWithCandy() {
    QList<QGraphicsItem*> itemsColliding = collidingItems();
    QList<Candy *> candiesNearby = static_cast<Game*>(scene())->candiesNearby(x(), y());
    if(candiesNearby.length() > 0) {
        for(int i = 0; i < itemsColliding.size(); i++) {
            QGraphicsItem *collidingItem = itemsColliding.at(i);

            for(int j = 0; j < candiesNearby.size(); j++) {
                Candy *candyNearby = candiesNearby.at(j);
                if(collidingItem->x() == candyNearby->x() && collidingItem->y() == candyNearby->y()) {
                    if (IdsCandiesTaken.length() >= CANDY_MAX)
                        return;
                    // si le candy qu'on touche est pris (pas par nous)
                    if(candyNearby->isTaken()) {
                        if(candyNearby->getCurrentPlayerId() != this->id) {
                            // Voler le candy
                            emit stealCandies(candyNearby->getId(), this->id);
                        }
                    }else{
                            // Ramasser le candy
                            if(dataLoader->isMultiplayer()) {
                                // Demander au serveur si on peut prendre le candy
                                emit isCandyFree(candyNearby->getId());
                            }else{
                                // appeler une fonction publique de Candy au lieu d'un signal car utiliser
                                // les signaux / slots demanderait de connecter au préalable tous les joueurs à
                                // tous les candy
                                candyNearby->pickUp(id, team);
                                IdsCandiesTaken.prepend(candyNearby->getId());
                            }
                    }
                }
            }
        }
    }
}

void Player::prependCandiesTaken(QList<int> candiesGained) {
    IdsCandiesTaken = candiesGained + IdsCandiesTaken;
}

QList<int> Player::looseCandies(int candyStolenId) {
    QList<int> candiesStolen;
    // Si le joueur ne contient pas ce candy, on retourne rien
    if(!IdsCandiesTaken.contains(candyStolenId))
        return candiesStolen;
    // Si la queue du joueur est encore protégée
    if(queueProtected->isActive())
        return candiesStolen;
    for(int i = 0; i < IdsCandiesTaken.length(); i++) {
        if(IdsCandiesTaken.at(i) == candyStolenId) {
           candiesStolen = IdsCandiesTaken.mid(i);
           IdsCandiesTaken = IdsCandiesTaken.mid(0, i);
           return candiesStolen;
        }
    }
    return candiesStolen;
}

QVector2D Player::calculateMovingVector(double delta) {
    QVector2D v;
    v.setX(int(moves[moveRight]) - int(moves[moveLeft]));
    v.setY(int(moves[moveDown]) - int(moves[moveUp]));
    v.normalize();
    v *= delta * dataLoader->getPlayerSpeed();
    return v;
}

QVector2D Player::calculateAnswerVector(QVector2D movingVector) {
    bool collideX = collideWithWalls(QVector2D(movingVector.x(), 0));
    bool collideY = collideWithWalls(QVector2D(0, movingVector.y()));

    QVector2D normalVector(
                movingVector.x() * collideX * -1,
                movingVector.y() * collideY * -1);

    QVector2D answerVector = movingVector + normalVector;
    //QVector2D answerVector(0, 0);

    return answerVector;
}

void Player::move(QVector2D vector, bool inverted) {
    if(inverted)
        vector = -vector;
    setX(x() + vector.x());
    setY(y() + vector.y());
}

// -------------------------------------------------------------------------------------

void Player::setZIndex(int yToAdd) {
    setZValue(y() + yToAdd);
}

void Player::setAnimation(Animations a) {
    // Arrêter le timer de l'animation qui se termine
    if(animationsLocal.contains(currentAnimation)) {
        animationsLocal.value(currentAnimation)->timer->stop();
    }
    // Changer l'animation
    currentAnimation = a;
    // Démarer le timer de la nouvelle animation
    animationsLocal.value(a)->timer->start();
}

void Player::animationNextFrame() {
    AnimationsLocalStruct *a = animationsLocal.value(currentAnimation);
    a->frameIndex++;
    if(a->frameIndex >= a->sharedDatas->nbFrame) {
        a->frameIndex = 0;
    }
    update();
}

Player::Animations Player::getAnimationType() {
    if((!moves[moveUp] && !moves[moveRight] && !moves[moveDown] && !moves[moveLeft]) ||
            (moves[moveUp] && moves[moveDown] && !moves[moveLeft] && !moves[moveRight]) ||
            (moves[moveRight] && moves[moveLeft] && !moves[moveUp] && !moves[moveDown]) ||
            (moves[moveRight] && moves[moveLeft] && moves[moveUp] && moves[moveDown])){
        return idle;
    }
    return run;
}

Player::Facing Player::getFacing() {
    if(moves[moveLeft] && !moves[moveRight]) {
        return facingLeft;
    }else if(!moves[moveLeft] && moves[moveRight]) {
        return facingRight;
    }
    return facing;
}

void Player::protectQueue() {
    queueProtected->start();
}

void Player::deleteCandy(int candyId) {
    IdsCandiesTaken.removeAll(candyId);
}

// OVERRIDE REQUIRED

// Paints contents of item in local coordinates
void Player::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {


    if(HITBOX_DEBUG) {
        // Debug rect
        painter->setPen(QPen(Qt::black));
        painter->drawRect(boundingRect());
        painter->drawText(10, 10, "ID : " + QString::number(id));
        painter->setPen(QPen(Qt::red));
        painter->drawPath(shape());
    }

    AnimationsLocalStruct *animToDraw = animationsLocal.value(currentAnimation);
    QPixmap *imageToDraw = animToDraw->sharedDatas->image;
    QTransform trans;
    int centerTextX = (this->username->boundingRect().width() - boundingRect().width()) / 2;

    if(facing == facingLeft) {
        trans.translate(boundingRect().width(), 0).scale(-1, 1);
        setTransform(trans);
        this->username->setPos(centerTextX + boundingRect().width(), -40);
    }else if (facing == facingRight) {
        setTransform(QTransform(1, 0, 0, 1, 1, 1));
        this->username->setPos(-centerTextX, -40);
    }else{
        resetTransform();
    }


    QRectF sourceRect = QRectF(imageToDraw->width() / animToDraw->sharedDatas->nbFrame * animToDraw->frameIndex, 0,
                               imageToDraw->width() / animToDraw->sharedDatas->nbFrame, imageToDraw->height());
    QRectF targetRect = QRectF(0, 0, dataLoader->getPlayerSize().x(), dataLoader->getPlayerSize().y());
    painter->drawPixmap(targetRect, *imageToDraw, sourceRect);

    // Lignes pour le compilateur
    Q_UNUSED(option)
    Q_UNUSED(widget)
}

// Returns outer bounds of item as a rectangle
// Called by QGraphicsView to determine what regions need to be redrawn
// the rect stay at 0:0 !!
QRectF Player::boundingRect() const {
    return QRectF(0, 0, dataLoader->getPlayerSize().x(), dataLoader->getPlayerSize().y());
}

// collisions detection
QPainterPath Player::shape() const {
    double widthRatio = 0.6;
    double shapeHeight = 130;
    QPainterPath path;
    path.addRect(QRectF(
                     (1 - widthRatio) * boundingRect().width() / 2,
                     shapeHeight,
                     boundingRect().width() * widthRatio,
                     boundingRect().height() - shapeHeight));
    return path;
}

int Player::getId() {
    return this->id;
}

int Player::getTeam() {
    return this->team;
}

QList<int> Player::getCandiesTaken() {
    return IdsCandiesTaken;
}

void Player::pickupCandyMulti(int candyId) {
    if(IdsCandiesTaken.length() <= CANDY_MAX)
        IdsCandiesTaken.prepend(candyId);
}

Player::~Player() {

}
