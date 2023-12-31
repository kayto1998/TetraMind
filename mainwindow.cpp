#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <fstream>
#include <iostream>
#include <QDebug>
#include <QSoundEffect>
#include <QDir>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    this->setStyleSheet("QMainWindow { border-radius: 5px; }");

    /// Initialize Scenes
    scene_ = new QGraphicsScene(this);
    nextScene_ = new QGraphicsScene(this);
    holdScene_ = new QGraphicsScene(this);

    m_backgroundMusic = new QSoundEffect(this);
    m_backgroundMusic->setSource(QUrl("qrc:/music/background.wav"));
    m_backgroundMusic->setLoopCount(QSoundEffect::Infinite);
    m_backgroundMusic->setVolume(0.1f);


    ui->graphicsView->setScene(scene_);
    ui->nextView->setScene(nextScene_);
    ui->holdView->setScene(holdScene_);

    scene_->setSceneRect(0, 0, BORDER_RIGHT, BORDER_DOWN);
    nextScene_->setSceneRect(0, 0, SMALL_BORDER_RIGHT, SMALL_BORDER_DOWN);
    holdScene_->setSceneRect(0, 0, SMALL_BORDER_RIGHT, SMALL_BORDER_DOWN);

    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    ui->nextView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->nextView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    ui->holdView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->holdView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    int seed = time(0);
    randomEng.seed(seed);
    distr = std::uniform_int_distribution<int>(0, NUMBER_OF_TETROMINOS - 1);
    distr(randomEng);

    timer.setInterval(333);

    connect(&timer, &QTimer::timeout, this, &MainWindow::tick);
    connect(this, &MainWindow::setScore, ui->score,
            qOverload<int>(&QLCDNumber::display));

    nextTetrominion_ = new Tetromino(TetrominoType(distr(randomEng)), this,
                                     Coords(SMALL_BORDER_RIGHT / BLOCK / 2 - 2,
                                            SMALL_BORDER_DOWN / BLOCK / 2 - 1));
    renderTetromino(nextTetrominion_, nextScene_);

    ui->stopButton->setDisabled(true);
    ui->graphicsView->installEventFilter(this);
    ui->graphicsView->show();
    cargarPalabras(datos);
    m_backgroundMusic->play();
}

MainWindow::~MainWindow() {
    for (Block block : blocks_) {
        if (block != nullptr) {
            delete block;
            block = nullptr;
        }
    }

    blocks_.clear();

    delete nextTetrominion_;
    nextTetrominion_ = nullptr;

    if (tetrominion_ != nullptr) {
        delete tetrominion_;
        tetrominion_ = nullptr;
    }

    if (holdTetrominion_ != nullptr) {
        delete holdTetrominion_;
        holdTetrominion_ = nullptr;
    }
    delete ui;
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Right:
    case Qt::Key_D:
        move(RIGHT);
        break;

    case Qt::Key_Left:
    case Qt::Key_A:
        move(LEFT);
        break;

    case Qt::Key_E:
        rotate(1);
        break;

    case Qt::Key_Q:
        rotate(-1);
        break;

    case Qt::Key_Down:
    case Qt::Key_S:
        tick();
        break;

    case Qt::Key_X:
        fallDown();
        break;

    default:
        return;
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event){
    QKeyEvent *keyEvent = NULL;//event data, if this is a keystroke event
    bool result = false;//return true to consume the keystroke

    if (event->type() == QEvent::KeyPress){
        keyEvent = dynamic_cast<QKeyEvent*>(event);
        this->keyPressEvent(keyEvent);
        result = true;
    } else if (event->type() == QEvent::KeyRelease){
        keyEvent = dynamic_cast<QKeyEvent*>(event);
        this->keyReleaseEvent(keyEvent);
        result = true;
    }else
        result = QObject::eventFilter(obj, event);

    return result;
}

void MainWindow::renderTetromino(Tetromino *tetro, QGraphicsScene *scene,
                                 SceneType type) {
    if (type == HOLD) {
        for (Block block : tetro->blocks()) {
            blocks_.erase(std::find(blocks_.begin(), blocks_.end(), block));
        }
    }

    tetro->render();
    for (Block block : tetro->blocks()) {
        scene->addItem(block);
        if (type == MAIN) {
            blocks_.push_back(block);
        }
    }
}

void MainWindow::tick() {
    if (tetrominion_ == nullptr) {
        tetrominion_ = nextTetrominion_;
        tetrominion_->setPos(Coords(TETRO_CENTER, 0));
        renderTetromino(tetrominion_, scene_, MAIN);

        nextTetrominion_ = new Tetromino(TetrominoType(distr(randomEng)), this,
                                         Coords(SMALL_BORDER_RIGHT / BLOCK / 2 - 2,
                                                SMALL_BORDER_DOWN / BLOCK / 2 - 1));
        renderTetromino(nextTetrominion_, nextScene_);

        if (!tetrominion_->moveTo(Coords(0, 0))) {
            gameOver();
        }
    }

    if (!tetrominion_->moveTo(DOWN)) {
        delete tetrominion_;
        tetrominion_ = nullptr;

        justReleased_ = false;
        checkRows();
    }
}



void MainWindow::on_startButton_clicked() {
    timer.start();
    ui->graphicsView->setFocus();
    ui->startButton->setDisabled(true);
    ui->stopButton->setEnabled(true);
    ui->stopButton->setText("DETENER");
}

std::vector<Block> MainWindow::blocks() const { return blocks_; }

void MainWindow::on_stopButton_clicked() {
    timer.stop();
    ui->startButton->setEnabled(true);
    ui->stopButton->setDisabled(true);
    ui->startButton->setText("CONTINUAR");
}

void MainWindow::move(const Directions &direction) {
    if (tetrominion_ != nullptr) {
        tetrominion_->moveTo(direction);
    }
}

void MainWindow::rotate(const int &direction) {
    if (tetrominion_ != nullptr) {
        tetrominion_->rotateIfAble(direction);
    }
}

void MainWindow::cargarPalabras(vector<string>& datos) {
    string texto;
    ifstream archivoLectura;
    cout << QDir::currentPath().toStdString() << endl;
    archivoLectura.open(QDir::currentPath().toStdString() + "/palabras.txt");
    if (archivoLectura.is_open()){
        cout << "leido" << endl;
        while (getline (archivoLectura, texto)) {
            datos.push_back(texto);
        }
    } else {
        cout << "no leido" << endl;
        int result = QMessageBox::warning(this, "Error",
                                          QString("Archivo de palabras no cargado"),
                                          QMessageBox::Ok);

        if (result == QMessageBox::Ok) {
            exit(0);
        }
    }
}

void MainWindow::generarPalabra(vector<string>& datos) {
    int n = datos.size();
    int indice = rand() % n;
    QGraphicsTextItem * io = new QGraphicsTextItem;
    io->setPlainText(QString::fromStdString(datos[indice]));
    io->setDefaultTextColor(QColor("white"));
    io->setFont(QFont(io->font().family(),18,false));
    QRectF bR = io->sceneBoundingRect();
    io->setPos(90 - bR.width()/2, 60 - bR.height()/2);
    holdScene_->addItem(io);
}

void MainWindow::checkRows() {
    std::vector<int> rowSums(ROWS, 0);
    for (Block block : blocks_) {
        int y = block->gridPos().y;
        rowSums.at(y) += 1;
    }
    int amountOfRows = 0;
    for (unsigned int i = 0; i < rowSums.size(); i++) {
        int sum = rowSums.at(i);
        if (sum == COLUMNS) {
            limpiarHoldScene();
            generarPalabra(datos);
            deleteRow(i);
            amountOfRows += 1;
        }
    }
    int baseScore = amountOfRows * SCORE;
    int extraScore = amountOfRows * SCORE / 2;
    score_ = score_ + baseScore + extraScore;
    emit setScore(score_);
    if (amountOfRows == 4) {
        tetrisCount_ += 1;
        ui->tetrisCount->setText("Tetris count: " + QString::number(tetrisCount_));
    }
}

void MainWindow::fallDown() {
    if (tetrominion_ != nullptr) {
        while (tetrominion_->moveTo(DOWN)) {
            continue;
        }
    }
}

void MainWindow::gameOver() {
    timer.stop();
    QMessageBox msgBox;

    msgBox.setText("GAME OVER :(");

    QPushButton *quitButton = msgBox.addButton(QMessageBox::Ok);
    quitButton->setMinimumSize(60, 30);

    msgBox.exec();

    if (msgBox.clickedButton() == quitButton) {
        close();
    }
}

void MainWindow::deleteRow(const int &y) {
    blocks_.erase(std::remove_if(blocks_.begin(), blocks_.end(),
                                 [y](Block p) {
                                     if (p->gridPos().y == y) {
                                         delete p;
                                         return true;
                                     }
                                     return false;
                                 }),
                  blocks_.end());
    for (Block block : blocks_) {
        if (block->gridPos().y < y) {
            block->move(Coords(DOWN));
        }
    }
}

void MainWindow::limpiarHoldScene() {
    holdScene_->clear();
}
