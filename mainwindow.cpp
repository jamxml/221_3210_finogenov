#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <openssl/sha.h>
#include "crypto_utils.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QDateTime>
#include <QCryptographicHash>
#include <QJsonObject>
#include <QByteArray>
#include <QDataStream>

const QByteArray encryptionKey = QByteArray::fromHex("e2fb671f40239a6d2c9ca93ad14734f266a249554e84230b8d99d06798e4bcc0");
const QByteArray iv = QByteArray::fromHex("6A6F686F726E67646F6E6F70786A6764");

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), moveCounter(0) {
    ui->setupUi(this);

    // Создание макета с чекбоксами
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            QCheckBox *checkBox = new QCheckBox(this);
            checkBox->setObjectName(QString("checkbox_%1_%2").arg(i).arg(j));
            checkBox->setFixedSize(50, 50);
            ui->gridLayout_2->addWidget(checkBox, i, j);
            connect(checkBox, &QCheckBox::stateChanged, this, &MainWindow::handleCheckboxClick);
        }
    }

    // Макет для кнопок
    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->addWidget(ui->loadButton);
    buttonLayout->addWidget(ui->resetButton);
    QWidget *buttonWidget = new QWidget(this);
    buttonWidget->setLayout(buttonLayout);

    // Добавление кнопок в главный макет
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(ui->gamePage);
    mainLayout->addWidget(buttonWidget);

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    connect(ui->loadButton, &QPushButton::clicked, this, &MainWindow::loadGameFromTxt);
    connect(ui->resetButton, &QPushButton::clicked, this, &MainWindow::resetGame);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::handleCheckboxClick(int state) {
    if (state != Qt::Checked) return;

    QCheckBox *checkBox = qobject_cast<QCheckBox*>(sender());
    if (!checkBox) return;

    // Меняем цвет чекбокса в зависимости от хода
    QString color = (moveCounter % 2 == 0) ? "green" : "red";
    checkBox->setStyleSheet(QString("background-color: %1;").arg(color));
    checkBox->setEnabled(false);
    moveCounter++;

    int x = checkBox->objectName().split('_')[1].toInt();
    int y = checkBox->objectName().split('_')[2].toInt();
    QString timestamp = QDateTime::currentDateTime().toString("yyyy.MM.dd_HH:mm:ss");

    // Генерация хеша для текущего хода
    QString previousHash = (moveCounter == 1) ? "" : generateChecksum(x, y, color, timestamp, lastMoveHash);
    QString currentHash = generateChecksum(x, y, color, timestamp, previousHash);
    lastMoveHash = currentHash; // Сохраняем хеш текущего хода для следующего

    // Формируем данные для шифрования
    QJsonObject moveData;
    moveData["x"] = x;
    moveData["y"] = y;
    moveData["color"] = color;
    moveData["timestamp"] = timestamp;
    moveData["hash"] = currentHash;

    QByteArray dataToEncrypt;
    QDataStream stream(&dataToEncrypt, QIODevice::WriteOnly);
    stream << moveData["x"].toInt() << moveData["y"].toInt() << moveData["color"].toString()
           << moveData["timestamp"].toString() << moveData["hash"].toString();

    QByteArray encryptedData;
    CryptoUtils cryptoUtils;
    int result = cryptoUtils.encrypt(dataToEncrypt, encryptedData, encryptionKey, iv);

    if (result == 0) {
        qDebug() << "Данные успешно зашифрованы!";
    } else {
        QMessageBox::warning(this, "Ошибка", "Ошибка при шифровании данных.");
        return;
    }

    // Сохраняем зашифрованные данные
    moveData["encryptedData"] = QString(encryptedData.toBase64());
    saveGameToTextFile(moveData);
}


// Генерация контрольной суммы для хода
QString MainWindow::generateChecksum(int x, int y, const QString &color, const QString &timestamp, const QString &previousHash) {
    QByteArray data = QString("%1%2%3%4%5").arg(x).arg(y).arg(color).arg(timestamp).arg(previousHash).toUtf8();
    return QString(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}


void MainWindow::saveGameToTextFile(const QJsonObject &move) {
    QFile file("gameState.txt");

    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Не удалось открыть файл для записи!";
        return;
    }

    QTextStream out(&file);
    QString gameData = QString("x: %1, y: %2, color: %3, timestamp: %4, hash: %5, encryptedData: %6")
                           .arg(move["x"].toInt())
                           .arg(move["y"].toInt())
                           .arg(move["color"].toString())
                           .arg(move["timestamp"].toString())
                           .arg(move["hash"].toString())
                           .arg(move["encryptedData"].toString());

    out << gameData << "\n"; // Просто записываем в файл как строку
    file.close();
}


void MainWindow::loadGameFromTxt() {
    QFile file("gameState.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Не удалось открыть файл для чтения!";
        return;
    }

    QTextStream in(&file);
    QString line;

    while (!in.atEnd()) {
        line = in.readLine();
        QStringList dataParts = line.split(", ");

        // Извлекаем зашифрованные данные
        QByteArray encryptedData = QByteArray::fromBase64(dataParts.last().split(": ")[1].toUtf8());

        QByteArray decryptedData;
        CryptoUtils cryptoUtils;
        int result = cryptoUtils.decrypt(encryptedData, decryptedData, encryptionKey, iv);
        if (result != 0) {
            QMessageBox::warning(this, "Ошибка", "Ошибка при расшифровке данных.");
            return;
        }

        QDataStream stream(&decryptedData, QIODevice::ReadOnly);
        int x, y;
        QString color, timestamp, hash;
        stream >> x >> y >> color >> timestamp >> hash;

        // Восстановление состояния чекбоксов
        QCheckBox *checkBox = findChild<QCheckBox*>(QString("checkbox_%1_%2").arg(x).arg(y));
        if (checkBox) {
            checkBox->setEnabled(false);
            checkBox->setStyleSheet(QString("background-color: %1;").arg(color));
            checkBox->setChecked(true);
        }
    }

    file.close();
}


void MainWindow::resetGame() {
    // Сбрасываем состояние игрового поля
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            QCheckBox *checkBox = findChild<QCheckBox*>(QString("checkbox_%1_%2").arg(i).arg(j));
            if (checkBox) {
                checkBox->setEnabled(true);
                checkBox->setStyleSheet("");
                checkBox->setChecked(false);
            }
        }
    }

    // Перезаписываем состояние в файл (пустое состояние после сброса)
    QFile file("gameState.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.close();
    }

    moveCounter = 0;  // Сброс счетчика ходов
}
